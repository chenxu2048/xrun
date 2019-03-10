#ifndef _XR_PROCESS_H
#define _XR_PROCESS_H

#include "lib/list.h"
#include "tracer/files.h"

typedef struct xr_thread_s xr_thread_t;
typedef struct xr_process_s xr_process_t;

typedef struct xr_process_resource_s xr_process_resource_t;

struct xr_process_resource_s {};

struct xr_process_s {
  int pid;

  int memory;
  xr_time_t time;

  xr_file_set_t fset;
  xr_string_t pwd;

  xr_list_t processes;
  xr_list_t threads;
  int nthread;
};

struct xr_thread_s {
  int tid;

  enum {
    XR_THREAD_CALLIN = 0,
    XR_THREAD_CALLOUT = 1,
  } syscall_status;

  xr_string_t *pwd;
  xr_file_set_t fset;

  xr_process_t *process;
  xr_list_t threads;
};

static inline void xr_process_add_thread(xr_process_t *process,
                                         xr_thread_t *thread) {
  process->nthread++;
  xr_list_add(&(process->threads), &(thread->threads));
  thread->process = process;
}

void xr_process_delete(xr_process_t *process);
void xr_thread_delete(xr_thread_t *thread);
void xr_file_set_delete(xr_file_set_t *fset);

#endif
