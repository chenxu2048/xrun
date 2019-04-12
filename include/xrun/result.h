#ifndef XR_RESULT_H
#define XR_RESULT_H

#include "xrun/utils/path.h"
#include "xrun/utils/string.h"
#include "xrun/utils/time.h"

typedef struct xr_result_s xr_result_t;
typedef enum xr_tracer_code_e xr_tracer_code_t;

enum xr_tracer_code_e {
  // unknown status
  XR_RESULT_UNKNOWN = 0x0,
  // tracer error
  XR_RESULT_TRACERERR,
  // time out
  XR_RESULT_TIMEOUT,
  // memory out
  XR_RESULT_MEMOUT,
  // file out
  XR_RESULT_FDOUT,
  // thread or process out
  XR_RESULT_TASKOUT,
  // clone with malicious flags
  XR_RESULT_CLONEDENY,
  // syscall deny
  XR_RESULT_CALLDENY,
  // path deny
  XR_RESULT_PATHDENY,
  // write too much
  XR_RESULT_WRITEOUT,
  // read too much
  XR_RESULT_READOUT,
  // ok
  XR_RESULT_OK,
};

typedef struct xr_tracer_process_result_s xr_tracer_process_result_t;
struct xr_tracer_process_result_s {
  long memory;
  xr_time_t time;
  int nthread;
  int nfile;
  long long io_read, io_write;
  xr_tracer_process_result_t *next;
};

struct xr_result_s {
  xr_tracer_code_t status;
  int nprocess;
  union {
    int ecall;
    struct {
      xr_path_t epath;
      long eflags;
    };
  };
  int epid, etid;
  xr_tracer_process_result_t *exited_processes;
  xr_tracer_process_result_t *aborted_processes;
  xr_tracer_process_result_t error_process;
};

static inline void xr_result_init(xr_result_t *result) {
  memset(result, 0, sizeof(xr_result_t));
  result->status = XR_RESULT_UNKNOWN;
}

static inline void xr_result_delete(xr_result_t *result) {
  while (result->exited_processes != NULL) {
    xr_tracer_process_result_t *process = result->exited_processes;
    result->exited_processes = process->next;
    free(process);
  }
  while (result->aborted_processes != NULL) {
    xr_tracer_process_result_t *process = result->aborted_processes;
    result->aborted_processes = process->next;
    free(process);
  }
  if (result->status == XR_RESULT_PATHDENY ||
      result->status == XR_RESULT_WRITEOUT ||
      result->status == XR_RESULT_READOUT) {
    xr_path_delete(&result->epath);
  }
}

#endif
