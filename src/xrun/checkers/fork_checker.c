#define _GNU_SOURCE
#include <sched.h>

#define CLONE_FLAG_ARGS(call) 1

#include "xrun/calls.h"
#include "xrun/checkers/fork_checker.h"
#include "xrun/option.h"
#include "xrun/process.h"
#include "xrun/tracer.h"

#ifndef XR_SYSCALL_VFORK
#define XR_SYSCALL_VFORK -1
#endif
#ifndef XR_SYSCALL_FORK
#define XR_SYSCALL_FORK -1
#endif
#ifndef XR_SYSCALL_CLONE
#define XR_SYSCALL_CLONE -1
#endif

struct xr_fork_checker_data_s {
  xr_tracer_code_t code;
  size_t nprocess, nthread, total_thread;
};
typedef struct xr_fork_checker_data_s xr_fork_checker_data_t;
static inline xr_fork_checker_data_t *xr_fork_checker_data(
  xr_checker_t *checker) {
  return (xr_fork_checker_data_t *)checker->checker_data;
}

void xr_fork_checker_init(xr_checker_t *checker) {
  checker->setup = xr_fork_checker_setup;
  checker->check = xr_fork_checker_check;
  checker->result = xr_fork_checker_result;
  checker->_delete = xr_fork_checker_delete;
  checker->checker_id = XR_CHECKER_FORK;
  checker->checker_data = _XR_NEW(xr_fork_checker_data_t);
}

bool xr_fork_checker_setup(xr_checker_t *checker, xr_option_t *option) {
  xr_fork_checker_data(checker)->nprocess = option->nprocess;
  xr_fork_checker_data(checker)->nthread = option->limit_per_process.nthread;
  xr_fork_checker_data(checker)->total_thread = option->limit.nthread;
  return true;
}

bool xr_fork_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                           xr_trace_trap_t *trap) {
  if (trap->trap != XR_TRACE_TRAP_SYSCALL ||
      trap->thread->syscall_status == XR_THREAD_CALLIN) {
    return true;
  }

  int retval = trap->syscall_info.retval;
  int syscall = trap->syscall_info.syscall;
  bool fork = false, clone_files = false, clone_fs = false;

  if (syscall != XR_SYSCALL_CLONE && syscall != XR_SYSCALL_FORK &&
      syscall != XR_SYSCALL_VFORK) {
    // not a clone, fork, vfork syscall
    // or, caller syscall return.
    // checking
    return true;
  } else if (retval != 0) {
    if (syscall == XR_SYSCALL_CLONE &&
        (trap->syscall_info.args[CLONE_FLAG_ARGS(syscall)] & CLONE_UNTRACED)) {
      xr_fork_checker_data(checker)->code = XR_RESULT_CLONEDENY;
      return false;
    }
    return true;
  } else if (syscall == XR_SYSCALL_CLONE) {
    int clone_flags = trap->syscall_info.args[CLONE_FLAG_ARGS(syscall)];
    if (clone_flags & CLONE_UNTRACED) {
      // clone with CLONE_UNTRACED is not allow
      xr_fork_checker_data(checker)->code = XR_RESULT_CLONEDENY;
      return false;
    }
    // CLONE_THREAD make new task be in the same thread group of caller task
    fork = ((clone_flags & CLONE_THREAD) == 0);
    clone_files = ((clone_flags & CLONE_FILES) != 0);
    clone_fs = ((clone_flags & CLONE_FS) != 0);
  } else {
    // fork and vfork make new process
    fork = true;
  }

  xr_thread_t *thread = trap->thread;
  xr_thread_t *caller = trap->thread->from;

  // clone files
  xr_file_set_share(&caller->fset, &thread->fset);
  if (!clone_files) {
    // copy files
    xr_file_set_own(&thread->fset);
  }

  xr_fs_share(&caller->fs, &thread->fs);
  if (!clone_fs) {
    // copy fs
    xr_fs_own(&thread->fs);
  }

  tracer->nthread++;

  // thread will return from fork or clone, so should be XR_THREAD_CALLIN
  if (fork) {
    // fork will create new thread group
    xr_process_remove_thread(thread->process, thread, true);
    thread->process = _XR_NEW(xr_process_t);
    xr_process_init(thread->process);
    thread->process->pid = thread->tid;
    xr_list_add(&tracer->processes, &thread->process->processes);
    xr_process_add_thread(thread->process, thread);
    tracer->nprocess++;
  }

  if (tracer->nprocess > xr_fork_checker_data(checker)->nprocess ||
      tracer->nthread > xr_fork_checker_data(checker)->total_thread ||
      thread->process->nthread > xr_fork_checker_data(checker)->nthread) {
    xr_fork_checker_data(checker)->code = XR_RESULT_TASKOUT;
    return false;
  }

  return true;
}

void xr_fork_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                            xr_result_t *result) {
  result->status = xr_fork_checker_data(checker)->code;
}

void xr_fork_checker_delete(xr_checker_t *checker) {
  free(checker->checker_data);
  return;
}
