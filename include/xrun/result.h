#ifndef XR_RESULT_H
#define XR_RESULT_H

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
  // syscall deny
  XR_RESULT_CALLDENY,
  // path deny
  XR_RESULT_PATHDENY,
  // ok
  XR_RESULT_OK,
};

struct xr_tracer_result_s {
  xr_tracer_code_t status;
};

#endif
