#ifndef XR_SYSCALL_CHECKER_H
#define XR_SYSCALL_CHECKER_H

#include "xrun/checker.h"

bool xr_syscall_checker_setup(xr_checker_t *checker, xr_tracer_t *tracer);

bool xr_syscall_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                              xr_trace_trap_t *trap);

void xr_syscall_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                               xr_tracer_result_t *result,
                               xr_trace_trap_t *trap);

void xr_syscall_checker_delete(xr_checker_t *checker);

void xr_syscall_checker_init(xr_checker_t *checker);

#endif
