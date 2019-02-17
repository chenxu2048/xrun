#ifndef _XR_CHECKER_H
#define _XR_CHECKER_H

#include "lib/list.h"
struct xr_checker_s;
typedef struct xr_checker_s xr_checker_t;

typedef struct xr_tracer_s xr_tracer_t;
typedef struct xr_tracer_result_s xr_tracer_result_t;
typedef struct xr_trace_trap_s xr_trace_trap_t;

typedef bool xr_checker_check_f(xr_checker_t *checker, xr_tracer_t *tracer,
                                xr_trace_trap_t *trap);

typedef void xr_checker_result_f(xr_checker_t *checker, xr_tracer_t *tracer,
                                 xr_tracer_result_t *result);

typedef bool xr_checker_setup_f(xr_checker_t *checker, xr_tracer_t *tracer);

struct xr_checker_s {
  xr_list_t checkers;
  xr_checker_check_f *check;
  xr_checker_result_f *result;
  xr_checker_setup_f *setup;
};

#endif
