#ifndef XR_PROCESS_H
#define XR_PROCESS_H

#include "xrun/files.h"
#include "xrun/utils/list.h"
#include "xrun/utils/time.h"

typedef struct xr_thread_s xr_thread_t;
typedef struct xr_process_s xr_process_t;

struct xr_process_s {
  int pid;

  int nfile;
  int memory;
  xr_time_t time;

  xr_list_t processes;
  xr_list_t threads;
  int nthread;
  int compat;
};

struct xr_thread_s {
  int tid;
  enum xr_thread_syscall_status_e {
    XR_THREAD_CALLIN = 0,
    XR_THREAD_CALLOUT = 1,
  } syscall_status;

  xr_fs_t fs;
  xr_file_set_t fset;

  xr_process_t *process;
  xr_list_t threads;
};

static inline void xr_process_add_thread(xr_process_t *process,
                                         xr_thread_t *thread) {
  process->nthread++;
  xr_list_add(&process->threads, &thread->threads);
  thread->process = process;
}

static inline void xr_process_remove_thread(xr_process_t *process,
                                            xr_thread_t *thread) {
  if (process != thread->process) {
    return;
  }
  xr_list_del(&thread->threads);
}

static inline void xr_process_init(xr_process_t *process) {
  xr_list_init(&process->processes);
  xr_list_init(&process->threads);
  process->nfile = 0;
  process->nthread = 0;
}
static inline void xr_thread_init(xr_thread_t *thread) {
  xr_list_init(&thread->threads);
  thread->syscall_status = XR_THREAD_CALLOUT;
  thread->tid = 0;
}
void xr_process_delete(xr_process_t *process);
void xr_thread_delete(xr_thread_t *thread);

#endif
