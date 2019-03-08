#ifndef _XR_PROCESS_H
#define _XR_PROCESS_H

#include "lib/list.h"
#include "tracer/files.h"

typedef struct xr_thread_s xr_thread_t;
typedef struct xr_process_s xr_process_t;

struct xr_process_s {
  int pid;

  xr_file_set_t fset;
  xr_string_t pwd;

  xr_list_t processes;
  xr_list_t threads;
};

struct xr_thread_s {
  int tid;

  enum {
    XR_THREAD_CALLIN,
    XR_THREAD_CALLOUT,
  } syscall_status;

  xr_string_t *pwd;
  xr_file_set_t fset;

  xr_process_t *process;
  xr_list_t threads;
  xr_list_t tracer_threads;
};

void xr_process_delete(xr_process_t *process);
void xr_thread_delete(xr_thread_t *thread);
void xr_file_set_delete(xr_file_set_t *fset);

#endif
