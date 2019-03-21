#include <fcntl.h>

#include "xrun/calls.h"
#include "xrun/checkers/file_checker.h"
#include "xrun/files.h"
#include "xrun/option.h"
#include "xrun/tracer.h"

static struct xr_file_checker_private_s {};

void xr_file_checker_init(xr_checker_t *checker) {
  checker->setup = xr_file_checker_setup;
  checker->check = xr_file_checker_check;
  checker->result = xr_file_checker_result;
  checker->_delete = xr_file_checker_delete;
  checker->checker_id = XR_CHECKER_FILE;
}

bool xr_file_checker_setup(xr_checker_t *checker, xr_tracer_t *tracer) {
  return true;
}

// fflags is the subset of flags in bits in contain mode.
// or fflags match the flags in match mode.
#define __do_file_flags_check(flimit, fflags)       \
  (((flimit).mode == XR_FILE_ACCESS_CONTAIN &&      \
    (flimit).flags == (flimit).flags | (fflags)) || \
   ((flimit).mode == XR_FILE_ACCESS_MATCH && (flimit).flags == (fflags)))

static inline bool __do_file_access_check(xr_tracer_t *tracer, xr_path_t *path,
                                          long flags) {
  xr_file_limit_t *flist = tracer->option->file_access;
  size_t flength = tracer->option->n_file_access;
  for (int i = 0; i < flength; ++i) {
    if (xr_string_equal(&(flist[i].path), path) &&
        __do_file_flags_check(flist[i], flags)) {
      // path in flist and flags is ok
      return true;
    }
  }
  // if (tracer->option->enable_dir == false) {
  //   return false;
  // }
  flist = tracer->option->dir_access;
  flength = tracer->option->n_dir_access;
  for (int i = 0; i < flength; ++i) {
    if (xr_path_contains(&(flist[i].path), path) &&
        __do_file_flags_check(flist[i], flags)) {
      // one directory contains path and flags is ok
      return true;
    }
  }
  return false;
}

/**
 * change process dir to new fd.
 *
 * @trap tracer_trap
 * @fd fd of new pwd
 */
static inline bool __do_process_fchdir(xr_trace_trap_t *trap, int fd) {
  xr_file_t *file = xr_file_set_select_file(&trap->thread->fset, fd);
  if (file != NULL) {
    xr_string_copy(trap->thread->fs.pwd, file->path);
  } else {
    // TODO: internal error
    return false;
  }
  return true;
}

static inline bool __do_process_chdir(xr_trace_trap_t *trap,
                                      xr_tracer_t *tracer) {
  void *path_addr = (void *)(trap->syscall_info.args[0]);
  xr_path_t *pwd = trap->thread->fs.pwd;
  xr_path_t *path = _XR_NEW(xr_path_t);
  xr_string_init(path, XR_PATH_MAX);
  if (tracer->strcpy(tracer, trap->thread->tid, path_addr, path) == false) {
    return false;
  }
  if (xr_path_is_relative(path)) {
    xr_path_join(pwd, path);
    xr_path_abs(pwd);
  } else {
    xr_string_swap(path, pwd);
  }
  xr_path_delete(path);
  free(path);
  return __do_file_access_check(tracer, pwd, 0);
}

static inline bool __do_process_open_file(xr_tracer_t *tracer,
                                          xr_thread_t *thread, xr_path_t *at,
                                          void *fpath, int fd, long flags) {
  xr_path_t *path = _XR_NEW(xr_path_t);
  xr_string_init(path, XR_PATH_MAX);
  if (tracer->strcpy(tracer, thread->tid, fpath, path) == false || at == NULL) {
    return false;
  }
  if (thread->fset.data->file_holding >
      tracer->option->limit_per_process.nfile) {
    return false;
  }
  if (xr_path_is_relative(path)) {
    xr_path_t abs_path = {};
    xr_string_init(&abs_path, at->length + path->length + 2);
    xr_string_copy(&abs_path, at);
    xr_path_join(&abs_path, path);
    xr_path_abs(&abs_path);
    xr_path_delete(path);
    // move content to path, shadow copy.
    *path = abs_path;
  }
  xr_file_t *file = _XR_NEW(xr_file_t);
  xr_file_open(file, fd, flags, path);
  xr_file_set_add_file(&(thread->fset), file);

  return __do_file_access_check(tracer, file->path, file->flags);
}

#define __CREAT_FLAGS (O_CREAT | O_WRONLY | O_TRUNC)

bool xr_file_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                           xr_trace_trap_t *trap) {
  if (trap->trap != XR_TRACE_TRAP_SYSCALL) {
    return true;
  }
  int call = trap->syscall_info.syscall;
  long *call_args = trap->syscall_info.args;
  int retval = trap->syscall_info.retval;
  if (trap->thread->syscall_status == XR_THREAD_CALLOUT && retval == 0) {
    switch (call) {
// handel process working directory changing
#ifdef XR_SYSCALL_CHDIR
      case XR_SYSCALL_CHDIR:
        return __do_process_chdir(trap, tracer);
#endif
#ifdef XR_SYSCALL_FCHDIR
      case XR_SYSCALL_FCHDIR:
        return __do_process_fchdir(trap, call_args[0]);
#endif

// handle fd creating
#ifdef XR_SYSCALL_OPEN
      case XR_SYSCALL_OPEN:
        return __do_process_open_file(
          tracer, trap->thread, trap->thread->fs.pwd, (void *)call_args[0],
          retval, call_args[1]);
#endif
#ifdef XR_SYSCALL_CREAT
      case XR_SYSCALL_CREAT:
        return __do_process_open_file(
          tracer, trap->thread, trap->thread->fs.pwd, (void *)call_args[0],
          retval, __CREAT_FLAGS);
#endif
#ifdef XR_SYSCALL_OPENAT
      case XR_SYSCALL_OPENAT: {
        xr_file_t *at_file =
          xr_file_set_select_file(&trap->thread->fset, call_args[0]);
        return __do_process_open_file(tracer, trap->thread, at_file->path,
                                      (void *)call_args[1], retval,
                                      call_args[2]);
      }
#endif
#ifdef XR_SYSCALL_READ
      case XR_SYSCALL_READ:
#endif
#ifdef XR_SYSCALL_READV
      case XR_SYSCALL_READV:
#endif
#ifdef XR_SYSCALL_PREAD64
      case XR_SYSCALL_PREAD64:
#endif
#ifdef XR_SYSCALL_PREADV
      case XR_SYSCALL_PREADV:
#endif
#ifdef XR_SYSCALL_PREADV2
      case XR_SYSCALL_PREADV2:
#endif
#ifdef XR_SYSCALL_PREADV64
      case XR_SYSCALL_PREADV64:
#endif
// socket read
#ifdef XR_SYSCALL_RECV
      case XR_SYSCALL_RECV:
#endif
#ifdef XR_SYSCALL_RECVFROM
      case XR_SYSCALL_RECVFROM:
#endif
#ifdef XR_SYSCALL_RECVMSG
      case XR_SYSCALL_RECVMSG:
#endif
#ifdef XR_SYSCALL_RECVMMSG
      case XR_SYSCALL_RECVMMSG:
#endif
        return __do_process_read_check(checker, tracer, trap);
#ifdef XR_SYSCALL_WRITE
      case XR_SYSCALL_WRITE:
#endif
#ifdef XR_SYSCALL_WRITEV
      case XR_SYSCALL_WRITEV:
#endif
#ifdef XR_SYSCALL_PWRITE64
      case XR_SYSCALL_PWRITE64:
#endif
#ifdef XR_SYSCALL_PWRITEV
      case XR_SYSCALL_PWRITEV:
#endif
#ifdef XR_SYSCALL_PWRITEV2
      case XR_SYSCALL_PWRITEV2:
#endif
#ifdef XR_SYSCALL_PWRITEV64
      case XR_SYSCALL_PWRITEV64:
#endif
// socket write
#ifdef XR_SYSCALL_SEND
      case XR_SYSCALL_SEND:
#endif
#ifdef XR_SYSCALL_SENDTO
      case XR_SYSCALL_SENDTO:
#endif
#ifdef XR_SYSCALL_SENDMSG
      case XR_SYSCALL_SENDMSG:
#endif
#ifdef XR_SYSCALL_SENDMMSG
      case XR_SYSCALL_SENDMMSG:
#endif
        return __do_process_write_check(checker, tracer, trap);
      default:
        break;
    }
  }
  return true;
}

void xr_file_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                            xr_tracer_result_t *result) {}

void xr_file_checker_delete(xr_checker_t *checker) {
  return;
}
