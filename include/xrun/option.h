#ifndef XR_OPTION_H
#define XR_OPTION_H

#include <limits.h>

#include "xrun/calls.h"
#include "xrun/utils/path.h"
#include "xrun/utils/time.h"

#define XR_IO_UNLIMITED ULLONG_MAX
#define XR_NTHREAD_UNLIMITED INT_MAX
#define XR_NFILE_UNLIMITED INT_MAX
#define XR_MEMORY_UNLIMITED LONG_MAX

#define _XR_TIME_UNLIMITED \
  { .sys_time = ULONG_MAX, .user_time = ULONG_MAX }

static const xr_time_t __xr_time_unlimited = _XR_TIME_UNLIMITED;

#define XR_TIME_UNLIMITED __xr_time_unlimited

struct xr_limit_s;
typedef struct xr_limit_s xr_limit_t;

struct xr_limit_s {
  int nthread;
  long memory;
  xr_time_t time;
  int nfile;
  unsigned long long nread, nwrite;
};

#define _XR_LIMIT_UNLIMITED                                         \
  {                                                                 \
    .nthread = XR_NTHREAD_UNLIMITED, .memory = XR_MEMORY_UNLIMITED, \
    .time = _XR_TIME_UNLIMITED, .nfile = XR_NFILE_UNLIMITED,        \
    .nread = XR_IO_UNLIMITED, .nwrite = XR_IO_UNLIMITED,            \
  }

static const xr_limit_t __xr_limit_unlimited = _XR_LIMIT_UNLIMITED;

#define XR_LIMIT_UNLIMITED __xr_limit_unlimited

typedef struct xr_access_entry_s xr_access_entry_t;
typedef struct xr_access_list_s xr_access_list_t;
typedef enum xr_access_type_e xr_access_type_t;
typedef enum xr_access_mode_e xr_access_mode_t;

enum xr_access_mode_e {
  XR_ACCESS_MODE_FLAG_MATCH = 0x0,
  XR_ACCESS_MODE_FLAG_CONTAINS = 0x1,
};

enum xr_access_type_e {
  XR_ACCESS_TYPE_FILE = 0x0,
  XR_ACCESS_TYPE_DIR = 0x1,
};

struct xr_access_entry_s {
  xr_path_t path;
  long flags;
  xr_access_mode_t mode;
};

struct xr_access_list_s {
  xr_access_type_t type;
  size_t nentry;
  size_t capacity;
  xr_access_entry_t *entries;
};

static inline void xr_access_list_init(xr_access_list_t *alist,
                                       xr_access_type_t type) {
  memset(alist, 0, sizeof(xr_access_list_t));
  alist->type = type;
}

static inline void xr_access_list_delete(xr_access_list_t *alist) {
  for (size_t i = 0; i < alist->nentry; ++i) {
    xr_path_delete(&alist->entries[i].path);
  }
  if (alist->nentry != 0) {
    free(alist->entries);
  }
}

void xr_access_list_append(xr_access_list_t *alist, const char *path,
                           size_t length, long flags, xr_access_mode_t mode);
bool xr_access_list_check(xr_access_list_t *alist, xr_path_t *path, long flags);

typedef enum xr_access_trigger_mode_s xr_access_trigger_mode_t;
enum xr_access_trigger_mode_s {
  XR_ACCESS_TRIGGER_MODE_IN,
  XR_ACCESS_TRIGGER_MODE_OUT,
};

struct xr_option_s;
typedef struct xr_option_s xr_option_t;

struct xr_option_s {
  int nprocess;
  bool calls[XR_SYSCALL_MAX];
  xr_limit_t limit, limit_per_process;
  xr_access_trigger_mode_t access_trigger;
  xr_access_list_t files, directories;
};

static inline void xr_option_init(xr_option_t *option) {
  memset(option->calls, 0, sizeof(bool) * XR_SYSCALL_MAX);
  xr_access_list_init(&option->files, XR_ACCESS_TYPE_FILE);
  xr_access_list_init(&option->directories, XR_ACCESS_TYPE_DIR);
}

static inline void xr_option_delete(xr_option_t *option) {
  xr_access_list_delete(&option->files);
  xr_access_list_delete(&option->directories);
}
#endif
