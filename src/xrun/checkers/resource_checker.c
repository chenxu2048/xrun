#include <signal.h>

#include "xrun/calls.h"
#include "xrun/checkers/resource_checker.h"
#include "xrun/option.h"
#include "xrun/tracer.h"

void xr_reource_checker_init(xr_checker_t *checker) {
  checker->setup = xr_resource_checker_setup;
  checker->check = xr_resource_checker_check;
  checker->result = xr_resource_checker_result;
  checker->_delete = xr_resource_checker_delete;
  checker->checker_id = XR_CHECKER_RESOURCE;
}

bool xr_resource_checker_setup(xr_checker_t *checker, xr_tracer_t *tracer) {
  return true;
}

bool xr_resource_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                               xr_trace_trap_t *trap) {
  xr_limit_t *process_limit = &tracer->option->limit_per_process,
             *limit = &tracer->option->limit;
  if (trap->trap == XR_TRACE_TRAP_SIGNAL) {
    switch (trap->stop_signal) {
      case SIGSEGV:
        if (trap->process->memory > process_limit->memory) {
          return false;
        }
        break;
      case SIGALRM:
      case SIGXCPU:
        return false;
    }
  }
  xr_process_t *process;
  int total_memory = 0;
  _xr_list_for_each_entry(&(tracer->processes), process, xr_process_t,
                          processes) {
    total_memory += process->memory;
    if (total_memory >= limit->memory) {
      return false;
    }
    if (process->time.sys_time > limit->time.sys_time ||
        process->time.user_time > limit->time.user_time) {
      return false;
    }
  }
  return true;
}

void xr_resource_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                                xr_tracer_result_t *result,
                                xr_trace_trap_t *trap);

void xr_resource_checker_delete(xr_checker_t *checker);
