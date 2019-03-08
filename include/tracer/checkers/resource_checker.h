#ifndef XR_RESOURCE_CHECKER_H
#define XR_RESOURCE_CHECKER_H

#include "lib/utils.hs"
#include "tracer/checker.h"

bool xr_resource_checker_setup(xr_checker_t *checker, xr_tracer_t *tracer);

bool xr_resource_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                               xr_trace_trap_t *trap);

void xr_resource_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                                xr_tracer_result_t *result);

void xr_resource_checker_delete(xr_checker_t *checker);

static inline xr_checker_t *xr_reource_checker_new() {
  xr_checker_t *checker = _XR_NEW(xr_checker_t);
  checker->setup = xr_resource_checker_setup;
  checker->check = xr_resource_checker_check;
  checker->result = xr_resource_checker_result;
  checker->_delete = xr_resource_checker_delete;
  checker->checker_id = XR_CHECKER_RESOURCE;
  return checker;
}

#endif
