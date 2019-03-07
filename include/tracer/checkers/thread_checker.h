#ifndef XR_THREAD_CHECKER_H
#define XR_THREAD_CHECKER_H

#include "tracer/checker.h"

bool xr_thread_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                             xr_trace_trap_t *trap);

void xr_thread_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                              xr_tracer_result_t *result);

bool xr_thread_checker_setup(xr_checker_t *checker, xr_tracer_t *tracer);

void xr_thread_checker_delete(xr_checker_t *checker);

#endif
