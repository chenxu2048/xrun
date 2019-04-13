#include "xrun/checkers/syscall_checker.h"
#include "xrun/calls.h"
#include "xrun/option.h"
#include "xrun/process.h"
#include "xrun/tracer.h"

struct xr_syscall_checker_data_s;
typedef struct xr_syscall_checker_data_s xr_syscall_checker_data_t;
struct xr_syscall_checker_data_s {
  long ecall;
  bool *calls;
};

static inline xr_syscall_checker_data_t *xr_syscall_checker_data(
  xr_checker_t *checker) {
  return (xr_syscall_checker_data_t *)checker->checker_data;
}

bool xr_syscall_checker_setup(xr_checker_t *checker, xr_option_t *option) {
  xr_syscall_checker_data(checker)->calls = option->calls;
  return true;
}

bool xr_syscall_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                              xr_trace_trap_t *trap) {
  xr_syscall_checker_data_t *data = xr_syscall_checker_data(checker);
  if (trap->trap != XR_TRACE_TRAP_SYSCALL ||
      trap->thread->syscall_status != XR_THREAD_CALLOUT) {
    return true;
  }

  data->ecall = trap->syscall_info.syscall;
  if (XR_SYSCALL_MAX > data->ecall && data->ecall >= 0) {
    return data->calls[data->ecall];
  }
  return false;
}

void xr_syscall_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                               xr_result_t *result) {
  result->status = XR_RESULT_CALLDENY;
  result->ecall = xr_syscall_checker_data(checker)->ecall;
}

void xr_syscall_checker_delete(xr_checker_t *checker) {
  free(checker->checker_data);
  return;
}

void xr_syscall_checker_init(xr_checker_t *checker) {
  checker->setup = xr_syscall_checker_setup;
  checker->check = xr_syscall_checker_check;
  checker->result = xr_syscall_checker_result;
  checker->_delete = xr_syscall_checker_delete;
  checker->checker_id = XR_CHECKER_SYSCALL;
  checker->checker_data = _XR_NEW(xr_syscall_checker_data_t);
}
