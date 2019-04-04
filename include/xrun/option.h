#ifndef XR_OPTION_H
#define XR_OPTION_H

#include "xrun/calls.h"
#include "xrun/utils/path.h"
#include "xrun/utils/time.h"

struct xr_limit_s;
typedef struct xr_limit_s xr_limit_t;

struct xr_limit_s {
  int nthread;
  int memory;
  xr_time_t time;
  int nfile;
  unsigned long long nread, nwrite;
};

struct xr_file_limit_s;
typedef struct xr_file_limit_s xr_file_limit_t;

struct xr_file_limit_s {
  xr_path_t path;
  long flags;
  enum xr_file_access_mode {
    XR_FILE_ACCESS_MATCH,
    XR_FILE_ACCESS_CONTAIN,
  } mode;
};

struct xr_option_s;
typedef struct xr_option_s xr_option_t;

struct xr_option_s {
  int nprocess;

  xr_file_limit_t *file_access;
  size_t n_file_access;
  xr_file_limit_t *dir_access;
  size_t n_dir_access;
  enum xr_file_access_mode_e {
    XR_FILE_ACCESS_MODE_IT = 0x0,
    XR_FILE_ACCESS_MODE_OT = 0x1,
  } access_mode;
  bool call_access[XR_SYSCALL_MAX];

  xr_limit_t limit, limit_per_process;
};

static inline void xr_file_limit_delete(xr_file_limit_t *limit) {
  xr_path_delete(&limit->path);
}

#endif
