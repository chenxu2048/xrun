#ifndef XR_FILE_CHECKER_H
#define XR_FILE_CHECKER_H

#include "xrun/checker.h"
#include "xrun/utils/utils.h"

bool xr_file_checker_setup(xr_checker_t *checker, xr_option_t *option);

bool xr_file_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                           xr_trace_trap_t *trap);

void xr_file_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                            xr_result_t *result);

void xr_file_checker_delete(xr_checker_t *checker);

void xr_file_checker_init(xr_checker_t *checker);

#endif
