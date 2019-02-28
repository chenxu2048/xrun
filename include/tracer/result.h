#ifndef _XR_RESULT_H
#define _XR_RESULT_H

typedef struct xr_tracer_result_s xr_tracer_result_t;
typedef enum xr_tracer_code_e xr_tracer_code_t;

enum xr_tracer_code_e {
  XR_RESULT_UNKNOWN = 0x0,
  XR_RESULT_TIMEOUT = 0x1,
  XR_RESULT_FDOUT = 0x2,
  XR_RESULT_MEMOUT = 0x4,
  XR_RESULT_ACCESSERR = 0x8,
  XR_RESULT_RESOUT = 0x11,
  XR_RESULT_MEMERR = 0x12,
  XR_RESULT_TRACERERR = 0x14,
  XR_RESULT_OK = 0x8000,
};

struct xr_tracer_result_s {
  xr_tracer_code_t status;
  char *stdout, *stderr;
};

#endif
