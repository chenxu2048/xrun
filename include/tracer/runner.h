#ifndef _XR_RUNNER_H
#define _XR_RUNNER_H
#include <tracer/time.h>

typedef struct xr_runner_s xr_runner_t;
typedef struct xr_limit_s xr_limit_t;

struct xr_runner_s {
  int process;
  xr_limit_t limit, limit_per_process;
};

struct xr_limit_s {
  int thread;
  int memory;
  xr_time_t time;
  int file;
  unsigned long long io;
};

bool xr_runner_setup(xr_runner_t *runner);
int xr_runner_exec(xr_runner_t *runner);

#endif
