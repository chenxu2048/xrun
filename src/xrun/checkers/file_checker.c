#include <fcntl.h>

#include "xrun/calls.h"
#include "xrun/checkers/file_checker.h"
#include "xrun/files.h"
#include "xrun/option.h"
#include "xrun/process.h"
#include "xrun/tracer.h"

typedef struct xr_file_checker_data_s {
  xr_access_trigger_mode_t trigger;
  xr_path_t *epath;
  long flags;
  xr_access_list_t *files, *directories;
} xr_file_checker_data_t;

static inline xr_file_checker_data_t *xr_file_checker_data(
  xr_checker_t *checker) {
  return (xr_file_checker_data_t *)(checker->checker_data);
}

#define XR_FILE_OPENED_FD_PREV -1

void xr_file_checker_init(xr_checker_t *checker) {
  checker->setup = xr_file_checker_setup;
  checker->check = xr_file_checker_check;
  checker->result = xr_file_checker_result;
  checker->_delete = xr_file_checker_delete;
  checker->checker_id = XR_CHECKER_FILE;
  checker->checker_data = _XR_NEW(xr_file_checker_data_t);
  memset(checker->checker_data, 0, sizeof(xr_file_checker_data_t));
}

bool xr_file_checker_setup(xr_checker_t *checker, xr_option_t *option) {
  xr_file_checker_data_t *data = xr_file_checker_data(checker);

  data->trigger = option->access_trigger;
  data->files = &option->files;
  data->directories = &option->directories;

  return true;
}

static inline bool __do_file_access_check(xr_checker_t *checker,
                                          xr_path_t *path, long flags) {
  xr_file_checker_data_t *data = xr_file_checker_data(checker);
  bool result =
    xr_access_list_check(data->files, path, flags, XR_ACCESS_TYPE_FILE) ||
    xr_access_list_check(data->files, path, flags, XR_ACCESS_TYPE_DIR);
  if (result == false) {
    xr_file_checker_data(checker)->epath = path;
    xr_file_checker_data(checker)->flags = flags;
  }
  return result;
}

static inline void __do_process_close_file(xr_thread_t *thread, int fd) {
  xr_file_set_close_file(&thread->fset, fd);
}

/**
 * change process dir to new fd.
 *
 * @trap tracer_trap
 * @fd fd of new pwd
 */
static inline bool __do_process_fchdir(xr_thread_t *thread, int fd) {
  xr_file_t *file = xr_file_set_select_file(&thread->fset, fd);
  if (file != NULL) {
    xr_string_copy(xr_fs_pwd(&thread->fs), &file->path);
  } else {
    // TODO: internal error
    return false;
  }
  return true;
}

static inline bool __do_process_chdir(xr_checker_t *checker, xr_path_t *pwd,
                                      xr_path_t *path) {
  if (xr_path_is_relative(path)) {
    xr_path_join(pwd, path);
    xr_path_abs(pwd);
  } else {
    xr_string_swap(path, pwd);
  }
  return __do_file_access_check(checker, pwd, O_RDONLY);
}

static inline bool __do_process_open_file(xr_checker_t *checker,
                                          xr_thread_t *thread, xr_path_t *at,
                                          xr_path_t *path, int fd, long flags) {
  if (at == NULL) {
    return false;
  }
  if (xr_path_is_relative(path)) {
    xr_path_t abs_path = {};
    xr_string_init(&abs_path, at->length + path->length + 2);
    xr_string_copy(&abs_path, at);
    xr_path_join(&abs_path, path);
    xr_path_abs(&abs_path);
    xr_string_swap(&abs_path, path);
    xr_path_delete(path);
  }
  xr_file_t *file = _XR_NEW(xr_file_t);
  xr_file_init(file);
  xr_file_open(file, fd, flags, path);
  xr_file_set_add_file(&thread->fset, file);

  return __do_file_access_check(checker, &file->path, file->flags);
}

#ifndef CLONE_FILES
#define CLONE_FILES 0
#endif
#ifndef CLONE_FS
#define CLONE_FS 0
#endif

static inline void __do_process_unshare(xr_thread_t *thread, int flag) {
  if (flag & CLONE_FILES) {
    xr_file_set_own(&thread->fset);
  }
  if (flag & CLONE_FS) {
    xr_fs_own(&thread->fs);
  }
}

#define XR_CREATE_FLAGS (O_CREAT | O_WRONLY | O_TRUNC)

#ifndef XR_SYSCALL_OPEN
#define XR_SYSCALL_OPEN -1
#endif

#ifndef XR_SYSCALL_OPENAT
#define XR_SYSCALL_OPENAT -1
#endif

#ifndef XR_SYSCALL_CREAT
#define XR_SYSCALL_CREAT -1
#endif

#define XR_NEW_FILE(syscall)                                         \
  ((syscall) == XR_SYSCALL_OPEN || (syscall) == XR_SYSCALL_OPENAT || \
   (syscall) == XR_SYSCALL_CREAT)

#define XR_FILE_CHECK_ENABLE_IT(trigger, thread_status) \
  ((trigger) == XR_ACCESS_TRIGGER_MODE_IN &&            \
   (thread_status) == XR_THREAD_CALLIN)

#define XR_FILE_CHECK_ENABLE_OT(trigger, thread_status, retval) \
  ((trigger) == XR_ACCESS_TRIGGER_MODE_OUT &&                   \
   (thread_status) == XR_THREAD_CALLOUT && (retval) >= 0)

#define XR_FILE_CHECK_ENABLE(trigger, thread_status, retval) \
  (XR_FILE_CHECK_ENABLE_IT(trigger, thread_status) ||        \
   XR_FILE_CHECK_ENABLE_OT(trigger, thread_status, retval))

#define XR_OPEN_PATH_ARG(syscall) (syscall == XR_SYSCALL_OPENAT ? 1 : 0)
#define XR_OPEN_FLAG_ARG(syscall) (syscall == XR_SYSCALL_OPENAT ? 2 : 1)

bool xr_file_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                           xr_trace_trap_t *trap) {
  if (trap->trap != XR_TRACE_TRAP_SYSCALL) {
    return true;
  }

  int call = trap->syscall_info.syscall;
  long *call_args = trap->syscall_info.args;
  int retval = trap->syscall_info.retval;
  xr_thread_t *thread = trap->thread;

  if (thread->syscall_status == XR_THREAD_CALLOUT && retval >= 0) {
    switch (call) {
      // handel process working directory changing
#ifdef XR_SYSCALL_CHDIR
      case XR_SYSCALL_CHDIR: {
        xr_path_t path;
        xr_string_zero(&path);
        bool result =
          tracer->strcpy(tracer, thread->tid, (void *)call_args[0], &path) &&
          __do_process_chdir(checker, xr_fs_pwd(&thread->fs), &path);
        xr_path_delete(&path);
        return result;
      }
#endif
#ifdef XR_SYSCALL_FCHDIR
      case XR_SYSCALL_FCHDIR:
        return __do_process_fchdir(thread, call_args[0]);
#endif
      case XR_SYSCALL_CLOSE:
        __do_process_close_file(thread, retval);
        return true;
      case XR_SYSCALL_UNSHARE:
        __do_process_unshare(thread, call_args[0]);
        return true;
      default:
        break;
    }
  }
  // handle new file syscall.
  if (XR_FILE_CHECK_ENABLE(xr_file_checker_data(checker)->trigger,
                           thread->syscall_status, retval) &&
      XR_NEW_FILE(call)) {
    int fd = retval;
    if (thread->syscall_status == XR_THREAD_CALLIN) {
      fd = XR_FILE_OPENED_FD_PREV;
    }
    xr_string_t path;
    xr_string_zero(&path);
    if (tracer->strcpy(tracer, thread->tid,
                       (void *)call_args[XR_OPEN_PATH_ARG(call)],
                       &path) == false) {
      xr_path_delete(&path);
      return true;
    }

    long flags = call_args[XR_OPEN_FLAG_ARG(call)];
    xr_path_t *at = xr_fs_pwd(&thread->fs);
    if (call == XR_SYSCALL_OPENAT && (int32_t)call_args[0] != AT_FDCWD) {
      xr_file_t *atfile = xr_file_set_select_file(&thread->fset, call_args[0]);
      at = (atfile == NULL ? NULL : &atfile->path);
    }
    bool result = __do_process_open_file(checker, thread, at, &path, fd, flags);
    xr_path_delete(&path);
    return result;
  }

  if (XR_NEW_FILE(call) &&
      xr_file_checker_data(checker)->trigger == XR_ACCESS_TRIGGER_MODE_IN &&
      thread->syscall_status == XR_THREAD_CALLOUT) {
    xr_file_set_select_file(&thread->fset, XR_FILE_OPENED_FD_PREV)->fd = retval;
  }
  return true;
}

void xr_file_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                            xr_tracer_result_t *result) {
  result->status = XR_RESULT_PATHDENY;
  xr_file_checker_data_t *data = xr_file_checker_data(checker);
  xr_string_copy(&result->epath, data->epath);
  result->eflags = data->flags;
}

void xr_file_checker_delete(xr_checker_t *checker) {
  free(checker->checker_data);
  return;
}
