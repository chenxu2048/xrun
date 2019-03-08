#ifndef XR_FILE_CHECKER_H
#define XR_FILE_CHECKER_H

#include "lib/utils.h"
#include "tracer/checker.h"

bool xr_file_checker_setup(xr_checker_t *checker, xr_tracer_t *tracer);

bool xr_file_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                           xr_trace_trap_t *trap);

void xr_file_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                            xr_tracer_result_t *result);

void xr_file_checker_delete(xr_checker_t *checker);

xr_checker_t *xr_file_checker_new();

#endif
