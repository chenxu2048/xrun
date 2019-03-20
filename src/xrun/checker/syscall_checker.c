#include "xrun/checkers/syscall_checker.h"
#include "xrun/tracer.h"

bool xr_syscall_checker_setup(xr_checker_t *checker, xr_tracer_t *tracer) {
  return true;
}

bool xr_syscall_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                              xr_trace_trap_t *trap) {
  if (trap->trap != XR_TRACE_TRAP_SYSCALL ||
      trap->thread->syscall_status != XR_THREAD_CALLOUT) {
    return true;
  }

  if (XR_VAR_SYSCALL_MAX < trap->syscall_info.syscall) {
    return tracer->option->call_access[trap->syscall_info.syscall];
  } else {
    return false;
  }
}

void xr_syscall_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                               xr_tracer_result_t *result) {
  if (trap->syscall_info.syscall > XR_VAR_SYSCALL_MAX) {
  }
}

void xr_syscall_checker_delete(xr_checker_t *checker) {
  return;
}

void xr_syscall_checker_init(xr_checker_t *checker) {
  checker->setup = xr_syscall_checker_setup;
  checker->check = xr_syscall_checker_check;
  checker->result = xr_syscall_checker_result;
  checker->_delete = xr_syscall_checker_delete;
  checker->checker_id = XR_CHECKER_SYSCALL;
}
