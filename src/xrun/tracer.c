#include <stdbool.h>
#include <stdlib.h>

#include "xrun/checker.h"
#include "xrun/checkers.h"
#include "xrun/option.h"
#include "xrun/process.h"
#include "xrun/result.h"
#include "xrun/tracer.h"
#include "xrun/utils/utils.h"

bool xr_tracer_trace(xr_tracer_t *tracer, xr_entry_t *entry,
                     xr_tracer_result_t *result) {
  if (tracer->spwan(tracer, entry) == false) {
    _XR_TRACER_ERROR(tracer, "tracer spwan error.");
  } else {
    xr_trace_trap_t trap = {.trap = XR_TRACE_TRAP_NONE};
    while (true) {
      if (tracer->trap(tracer, &trap) == false) {
        return false;
      }

      if (xr_tracer_check(tracer, result, &trap) == false) {
        break;
      }

      if (tracer->step(tracer, &trap) == false) {
        _XR_TRACER_ERROR(tracer, "tracer step failed.");
        break;
      }
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
    if (checker->setup(checker, tracer->option) == false) {
      return _XR_TRACER_ERROR(tracer, "checker with id %d setup failed",
                              checker->checker_id);
    }
  }
  return true;
}

bool xr_tracer_check(xr_tracer_t *tracer, xr_tracer_result_t *result,
                     xr_trace_trap_t *trap) {
  xr_checker_t *checker;
  _xr_list_for_each_entry(&(tracer->checkers), checker, xr_checker_t,
                          checkers) {
    if (_XR_CALLP(checker, check, tracer, trap) == false) {
      tracer->failed_checker = checker;
      return false;
    };
  }
  return true;
}

void xr_tracer_clean(xr_tracer_t *tracer) {
  xr_list_t *cur, *temp;
  xr_process_t *process;

  // free tracer process list and delete tracer process
  _xr_list_for_each_safe(&(tracer->processes), cur, temp) {
    xr_list_del(cur);
    process = xr_list_entry(cur, xr_process_t, processes);
    tracer->kill(tracer, process->pid);
    xr_process_delete(process);
    free(process);
  }

  tracer->failed_checker = NULL;
}

void xr_tracer_delete(xr_tracer_t *tracer) {
  // delete all checkers
  xr_list_t *cur, *temp;
  xr_checker_t *checker;
  _xr_list_for_each_safe(&(tracer->checkers), cur, temp) {
    xr_list_del(cur);
    checker = xr_list_entry(cur, xr_checker_t, checkers);
    checker->_delete(checker);
    free(checker);
  }
  // clean up all process
  xr_tracer_clean(tracer);
}

bool xr_tracer_error(xr_tracer_t *tracer, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  if (errno == 0) {
    xr_error_verror(&tracer->error, msg, args);
  } else {
    xr_error_vnerror(&tracer->error, errno, msg, args);
  }
  return false;
}

bool xr_tracer_add_checker(xr_tracer_t *tracer, xr_checker_id_t cid) {
  xr_checker_t *checker = _XR_NEW(xr_checker_t);
  if (cid >= sizeof(xr_checker_init_funcs) / sizeof(xr_checker_init_f)) {
    return _XR_TRACER_ERROR(tracer, "add checker with invalid checker id");
  } else if (xr_checker_init_funcs[cid] == NULL) {
    return _XR_TRACER_ERROR(tracer, "checker init function does not exist.");
  }
  xr_checker_init_funcs[cid](checker);
  xr_list_add(&tracer->checkers, &checker->checkers);
  return true;
}
