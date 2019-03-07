#ifndef _XR_TIME_H
#define _XR_TIME_H

#include <sys/time.h>

typedef unsigned long xr_time_ms_t;

typedef struct xr_time_s xr_time_t;

struct xr_time_s {
  xr_time_ms_t sys_time;
  xr_time_ms_t user_time;
};

static inline xr_time_ms_t xr_time_ms_from_timeval(struct timeval timeval) {
  return timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
}

static inline xr_time_t xr_time_from_timeval(struct timeval sys_time,
                                             struct timeval user_time) {
  xr_time_t time = {
      .sys_time = xr_time_ms_from_timeval(sys_time),
      .user_time = xr_time_ms_from_timeval(user_time),
  };
  return time;
}

#endif
