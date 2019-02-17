#ifndef _XR_TIME_H
#define _XR_TIME_H

typedef unsigned long xr_time_ms_t;

typedef struct xr_time_s xr_time_t;

struct xr_time_s {
  xr_time_ms_t sys_time;
  xr_time_ms_t user_time;
};

#endif
