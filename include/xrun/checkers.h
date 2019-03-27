#ifndef XR_CHECKERS_H
#define XR_CHECEKRS_H

#include "xrun/checker.h"

extern void xr_file_checker_init(xr_checker_t *checker);
extern void xr_fork_checker_init(xr_checker_t *checker);
extern void xr_io_checker_init(xr_checker_t *checker);
extern void xr_resource_checker_init(xr_checker_t *checker);
extern void xr_syscall_checker_init(xr_checker_t *checker);

typedef void xr_checker_init_f(xr_checker_t *);

static xr_checker_init_f *const xr_checker_init_funcs[] = {
  [XR_CHECKER_FILE] = xr_file_checker_init,
  [XR_CHECKER_FORK] = xr_fork_checker_init,
  [XR_CHECKER_IO] = xr_io_checker_init,
  [XR_CHECKER_RESOURCE] = xr_resource_checker_init,
  [XR_CHECKER_SYSCALL] = xr_syscall_checker_init,
};

#endif
