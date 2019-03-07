#ifndef _XR_OPTION_H
#define _XR_OPTION_H
#include <lib/time.h>

typedef struct xr_option_s xr_option_t;
typedef struct xr_limit_s xr_limit_t;

typedef enum xr_checker_id_e xr_checker_id_t;

struct xr_option_s {
  int process;

  xr_limit_t limit, limit_per_process;
};

struct xr_limit_s {
  int thread;
  int memory;
  xr_time_t sys_time, user_time;
  int file;
  unsigned long long io;
};

#endif
