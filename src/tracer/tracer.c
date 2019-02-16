#include <stdbool.h>
#include <stdlib.h>

#include "lib/utils.h"
#include "tracer/tracer.h"

xr_tracer_t *xr_tracer_new(const char *name) {
  xr_tracer_t *tracer = _XR_NEW(xr_tracer_t);
  tracer->name = name;
  return tracer;
}

bool xr_tracer_trace(xr_tracer_t *tracer, xr_tracer_result_t *result) {
  result->status = XR_RESULT_OK;
  while (true) {
    bool step = xr_tracer_step(tracer);
    if (!step) {
      result->status = XR_RESULT_UNKNOWN;
    }
    // do check
  }
  if (result->status != XR_RESULT_OK) {
    xr_tracer_result(tracer, result);
  }
}

bool xr_tracer_check(xr_tracer_t *tracer, xr_tracer_result_t *result) {
  return true;
}

bool xr_tracer_step(xr_tracer_t *tracer);

void xr_tracer_delete(xr_tracer_t *tracer) {
  free(tracer);
}
