#ifndef _XR_PROCESS_H
#define _XR_PROCESS_H

#include "lib/list.h"

struct xr_process_s;
struct xr_thread_s;

struct xr_process_s {
  int pid;

  xr_list_t threads;
};

struct xr_thread_s {
  int tid;

  xr_list_t threads;
};

typedef struct xr_thread_s xr_thread_t;

#endif
