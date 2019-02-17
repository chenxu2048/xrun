#include <stdbool.h>
#include <stdlib.h>

#include "lib/utils.h"
#include "tracer/checker.h"
#include "tracer/process.h"
#include "tracer/tracer.h"

xr_tracer_t *xr_tracer_new(const char *name) {
  xr_tracer_t *tracer = _XR_NEW(xr_tracer_t);
  tracer->name = name;
  return tracer;
}

bool xr_tracer_trace(xr_tracer_t *tracer, xr_option_t *option,
                     xr_tracer_result_t *result) {
  if (xr_tracer_setup(tracer, option) == false) {
    result->status = XR_RESULT_UNKNOWN;
    return false;
  }
  if (tracer->spwan(tracer) == false) {
    return false;
  }
  while (true) {
    if (xr_tracer_step(tracer) == false) {
      xr_tracer_step_error(tracer, result);
      break;
    }
    xr_trace_trap_t trap = tracer->trap(tracer);

    if (trap.trap == XR_TRACE_TRAP_EXIT ||
        xr_tracer_check(tracer, result, &trap) == false) {
      break;
    }
  }
  xr_tracer_clean(tracer);
  return result->status == XR_RESULT_OK;
}

bool xr_tracer_setup(xr_tracer_t *tracer, xr_option_t *option) {
  tracer->option = option;
  xr_checker_t *checker;
  _xr_list_for_each_entry(&(tracer->checkers), checker, xr_checker_t,
                          checkers) {
    checker->setup(checker, tracer);
  }
}

bool xr_tracer_check(xr_tracer_t *tracer, xr_result_t *result,
                     xr_trace_trap_t *trap) {
  xr_checker_t *checker;
  _xr_list_for_each_entry(&(tracer->checkers), checker, xr_checker_t,
                          checkers) {
    if (_XR_CALLP(checker, check, tracer, result, trap)) {
      tracer->failed_checker = checker;
      return false;
    };
  }
  return true;
}

bool xr_tracer_step(xr_tracer_t *tracer) {
  return tracer->step(tracer);
}

void xr_tracer_clean(xr_tracer_t *tracer) {
  xr_process_t *process;
  xr_list_t *process_elem, *temp;
  _xr_list_for_each_safe(&(tracer->processes), process_elem, temp) {
    xr_list_del(process_elem);
    process = xr_list_entry(process_elem, xr_process_t, processes);
    // TODO: process clean
  }
  tracer->failed_checker = NULL;
}

void xr_tracer_delete(xr_tracer_t *tracer) {
  free(tracer);
}
