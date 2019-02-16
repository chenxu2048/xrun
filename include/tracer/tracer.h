#ifndef XR_TRACER_H
#define XR_TRACER_H

#include <stdbool.h>

#include "lib/list.h"
#include "tracer/result.h"

struct xr_tracer_s;

struct xr_tracer_s {
  const char *name;

  xr_list_t processes;
};

struct xr_tracer_result_s {
  xr_result_t status;
};

typedef struct xr_tracer_result_s xr_tracer_result_t;
typedef struct xr_tracer_s xr_tracer_t;

xr_tracer_t *xr_tracer_new(const char *name);
bool xr_tracer_trace(xr_tracer_t *tracer, xr_tracer_result_t *result);
bool xr_tracer_step(xr_tracer_t *tracer);
bool xr_tracer_check(xr_tracer_t *tracer, xr_tracer_result_t *result);
void xr_tracer_result(xr_tracer_t *tracer, xr_tracer_result_t *result);
void xr_tracer_delete(xr_tracer_t *tracer);

#endif
