#include <signal.h>

#include "xrun/calls.h"
#include "xrun/checkers/resource_checker.h"
#include "xrun/option.h"
#include "xrun/process.h"
#include "xrun/tracer.h"

struct xr_resource_checker_data_s {
  xr_tracer_code_t code;
  xr_limit_t *limit, *process_limit;
};
typedef struct xr_resource_checker_data_s xr_resource_checker_data_t;

static inline xr_resource_checker_data_t *xr_resource_checker_data(
  xr_checker_t *checker) {
  return (xr_resource_checker_data_t *)checker->checker_data;
}

void xr_resource_checker_init(xr_checker_t *checker) {
  checker->setup = xr_resource_checker_setup;
  checker->check = xr_resource_checker_check;
  checker->result = xr_resource_checker_result;
  checker->_delete = xr_resource_checker_delete;
  checker->checker_id = XR_CHECKER_RESOURCE;
  checker->checker_data = _XR_NEW(xr_resource_checker_data_t);
}

bool xr_resource_checker_setup(xr_checker_t *checker, xr_option_t *option) {
  xr_resource_checker_data_t *data = xr_resource_checker_data(checker);
  data->process_limit = &option->limit_per_process;
  data->limit = &option->limit;
  return true;
}

bool xr_resource_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                               xr_trace_trap_t *trap) {
  xr_resource_checker_data_t *data = xr_resource_checker_data(checker);
  if (trap->trap == XR_TRACE_TRAP_SIGNAL) {
    switch (trap->stop_signal) {
      case SIGSEGV:
        if (trap->thread->process->memory > data->process_limit->memory) {
          data->code = XR_RESULT_MEMOUT;
          return false;
        }
        break;
      case SIGALRM:
      case SIGXCPU:
        data->code = XR_RESULT_TIMEOUT;
        return false;
    }
  }
  xr_process_t *process;
  int total_memory = 0;
  _xr_list_for_each_entry(&(tracer->processes), process, xr_process_t,
                          processes) {
    total_memory += process->memory;
    if (total_memory >= data->limit->memory) {
      data->code = XR_RESULT_MEMOUT;
      return false;
    }
    if (process->time.sys_time > data->limit->time.sys_time ||
        process->time.user_time > data->limit->time.user_time) {
      data->code = XR_RESULT_TIMEOUT;
      return false;
    }
  }
  return true;
}

void xr_resource_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                                xr_result_t *result) {
  xr_resource_checker_data_t *data = xr_resource_checker_data(checker);
  result->status = data->code;
}

void xr_resource_checker_delete(xr_checker_t *checker) {
  free(checker->checker_data);
}
