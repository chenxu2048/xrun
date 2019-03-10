#ifndef _XR_OPTION_H
#define _XR_OPTION_H
#include <lib/time.h>

typedef struct xr_option_s xr_option_t;
typedef struct xr_limit_s xr_limit_t;
typedef struct xr_file_limit_s xr_file_limit_t;

typedef enum xr_checker_id_e xr_checker_id_t;

struct xr_option_s {
  int nprocess;

  xr_file_limit_t *file_access;
  int n_file_access;
  xr_file_limit_t *dir_access;
  int n_dir_access;
  int call_access[XR_SYSCALL_MAX];

  xr_limit_t limit, limit_per_process;
};

struct xr_limit_s {
  int nthread;
  int memory;
  xr_time_t time;
  int nfile;
  unsigned long long io;
};

struct xr_file_limit_s {
  xr_path_t path;
  long flags;
};

#endif
