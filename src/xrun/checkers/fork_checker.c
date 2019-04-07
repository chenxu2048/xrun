#define _GNU_SOURCE
#include <sched.h>

#define CLONE_FLAG_ARGS 1

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

void xr_fork_checker_init(xr_checker_t *checker) {
  checker->setup = xr_fork_checker_setup;
  checker->check = xr_fork_checker_check;
  checker->result = xr_fork_checker_result;
  checker->_delete = xr_fork_checker_delete;
  checker->checker_id = XR_CHECKER_FORK;
}

bool xr_fork_checker_setup(xr_checker_t *checker, xr_option_t *option) {
  return true;
}

bool xr_fork_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                           xr_trace_trap_t *trap) {
  if (trap->trap != XR_TRACE_TRAP_SYSCALL ||
      trap->thread->syscall_status == XR_THREAD_CALLIN) {
    return true;
  }

  int tid = trap->syscall_info.retval;
  int syscall = trap->syscall_info.syscall;
  bool fork = false, clone_files = false;

  if (syscall != XR_SYSCALL_CLONE || syscall != XR_SYSCALL_FORK ||
      syscall != XR_SYSCALL_VFORK || tid == 0) {
    // not a clone, fork, vfork syscall
    // or, callee syscall return.
    // checking
    return true;
  } else if (syscall == XR_SYSCALL_CLONE) {
    int clone_flags = trap->syscall_info.args[CLONE_FLAG_ARGS];
    if (clone_flags & CLONE_UNTRACED) {
      // clone with CLONE_UNTRACED is not allow
      return false;
    }
    // CLONE_THREAD make new task be in the same thread group of caller task
    fork = ((clone_flags & CLONE_THREAD) == 0);
    clone_files = ((clone_flags & CLONE_FILES) == 0);
  } else {
    // fork and vfork make new process
    fork = true;
  }

  xr_thread_t *thread = _XR_NEW(xr_thread_t);
  thread->tid = tid;
  thread->tid = XR_THREAD_CALLIN;
  // thread will return from fork or clone, so should be XR_THREAD_CALLIN
  if (fork) {
    // fork will create new thread group
    thread->process = _XR_NEW(xr_process_t);
    xr_list_init(&(thread->process->threads));
    thread->process->nthread = 0;
    tracer->nprocess++;
  } else {
    thread->process = trap->process;
  }
  xr_file_set_share(&(trap->thread->fset), &(thread->fset));
  if (!clone_files) {
    // copy files
    xr_file_set_own(&(thread->fset));
  }

  xr_process_add_thread(thread->process, thread);
  tracer->nthread++;

  if (tracer->nprocess > tracer->option->nprocess ||
      tracer->nthread > tracer->option->limit.nthread ||
      thread->process->nthread > tracer->option->limit_per_process.nthread) {
    return false;
  }

  return true;
}

void xr_fork_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                            xr_tracer_result_t *result, xr_trace_trap_t *trap) {
}

void xr_fork_checker_delete(xr_checker_t *checker) {
  return;
}
