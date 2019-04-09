#ifndef XR_RESULT_H
#define XR_RESULT_H

#include "xrun/utils/path.h"
#include "xrun/utils/string.h"

typedef struct xr_tracer_result_s xr_tracer_result_t;
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
  // ok
  XR_RESULT_OK,
};

typedef struct xr_tracer_process_result_s xr_tracer_process_result_t;
struct xr_tracer_process_result_s {
  long memory;
  long time;
  int nthread;
  int nfiles;
  xr_tracer_process_result_t *next;
};

struct xr_tracer_result_s {
  xr_tracer_code_t status;
  xr_string_t msg;
  int nprocess;
  xr_tracer_process_result_t *processes;
  xr_tracer_process_result_t *eprocess;
  union {
    int ecall;
    struct {
      xr_path_t epath;
      long eflags;
    };
  };
  int epid, etid;
};

static inline void xr_tracer_result_init(xr_tracer_result_t *result) {
  memset(result, 0, sizeof(xr_tracer_result_t));
  result->status = XR_RESULT_UNKNOWN;
  xr_string_zero(&result->msg);
}

static inline void xr_tracer_result_delete(xr_tracer_result_t *result) {
  while (result->processes != NULL) {
    xr_tracer_process_result_t *process = result->processes;
    result->processes = process->next;
    free(process);
  }
  if (result->status == XR_RESULT_PATHDENY) {
    xr_path_delete(&result->epath);
  }
}

#endif
