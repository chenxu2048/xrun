#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include "lib/utils.h"
#include "tracer/checker.h"
#include "tracer/process.h"
#include "tracer/tracer.h"

bool xr_tracer_trace(xr_tracer_t *tracer, xr_option_t *option,
                     xr_tracer_result_t *result) {
  if (xr_tracer_setup(tracer, option) == false) {
    result->status = XR_RESULT_UNKNOWN;
    return xr_tracer_error(tracer, _XR_ADD_FUNC("tracer setup failed."));
  }
  if (tracer->spwan(tracer) == false) {
    return xr_tracer_error(tracer, _XR_ADD_FUNC("tracer spwan error."));
  }
  xr_trace_trap_t trap = {.trap = XR_TRACE_TRAP_NONE};
  while (true) {
    if (tracer->trap(tracer, &trap) == false) {
      return false;
    }

    if (trap.trap == XR_TRACE_TRAP_EXIT ||
        xr_tracer_check(tracer, result, &trap) == false) {
      break;
    }

    if (tracer->step(tracer, &trap) == false) {
      xr_tracer_error(tracer, _XR_ADD_FUNC("tracer step failed."));
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
    if (checker->setup(checker, tracer) == false) {
      return xr_tracer_error(tracer,
                             _XR_ADD_FUNC("checker with id %d setup failed"),
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
    if (_XR_CALLP(checker, check, tracer, result, trap)) {
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
    xr_process_delete(process);
    free(process);
  }

  // all the thread were deleted.
  tracer->threads.next = tracer->threads.prev = &(tracer->threads);

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

  tracer->_delete(tracer);
}

bool xr_tracer_error(xr_tracer_t *tracer, const char *msg, ...) {
  xr_error_t *error = tracer->error;
  xr_string_t *emsg = error->msg;
  xr_string_t buffer;
  xr_string_init(&buffer, strlen(msg) * 2);
  xr_string_format(&buffer, msg, ) if (errno) {
    error->errno = errno;
  }
  size_t rest = emsg->capacity - emsg->length - 1;
  va_list args;
  va_start(msg, args);
  int wrote = vsnprintf(emsg->string + emsg->length, rest, format, args);
  va_end(args);

  if (wrote >= rest) {
    // retry
    va_start(msg, args);
    xr_string_grow(emsg, emsg->length + wrote + 1);
    vsnprintf(emsg->string + emsg->length, rest, format, args);
    va_end(args);
  }
  return false;
}
