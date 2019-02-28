#ifndef _XR_PROCESS_H
#define _XR_PROCESS_H

#include "lib/list.h"

typedef struct xr_thread_s xr_thread_t;
typedef struct xr_process_s xr_process_t;

struct xr_process_s {
  int pid;

  xr_list_t processes;
  xr_list_t threads;
};

struct xr_thread_s {
  int tid;

  xr_list_t threads;
};

void xr_process_delete(xr_process_t *process);
void xr_thread_delete(xr_thread_t *thread);

#endif
