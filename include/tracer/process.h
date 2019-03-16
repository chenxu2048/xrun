#ifndef _XR_PROCESS_H
#define _XR_PROCESS_H

#include "lib/list.h"
#include "lib/time.h"
#include "tracer/files.h"

typedef struct xr_thread_s xr_thread_t;
typedef struct xr_process_s xr_process_t;

struct xr_process_s {
  int pid;

  int memory;
  xr_time_t time;

  xr_list_t processes;
  xr_list_t threads;
  int nthread;

#if defined(__X86_64__) || defined(__X86__) || defined(__X32__)
  enum _xr_syscall_compat_s {
#ifdef __X86_64__
    XR_SYSCALL_COMPAT_MODE_X64,
#elif defined(__X32__)
    XR_SYSCALL_COMPAT_MODE_X32,
#else
    XR_SYSCALL_COMPAT_MODE_X86,
#endif
  } syscall_mode;
#endif
};

struct xr_thread_s {
  int tid;

  enum {
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
  xr_list_add(&(process->threads), &(thread->threads));
  thread->process = process;
}

void xr_process_delete(xr_process_t *process);
void xr_thread_delete(xr_thread_t *thread);

#endif
