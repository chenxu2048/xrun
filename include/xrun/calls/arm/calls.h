#ifndef XR_CALLS_ARM_H
#define XR_CALLS_ARM_H

#include <string.h>

#include "./calls_arm.h"

#define XR_COMPAT_SYSCALL_ARM_EABI 1
#define XR_COMPAT_SYSCALL_ARM_OABI 2

static inline int xr_calls_convert_impl(const char *name, int compat) {
  switch (compat) {
    case XR_COMPAT_SYSCALL_ARM_EABI:
      for (int i = 0; i < XR_SYSCALL_MAX; ++i) {
        if (strcmp(name, xr_syscall_table_arm[i]) == 0) {
          return i;
        }
      }
      break;
    case XR_COMPAT_SYSCALL_ARM_OABI:
      for (int i = XR_SYSCALL_MAX - 1; i >= 0; --i) {
        if (strcmp(name, xr_syscall_table_arm[i])) {
          return i;
        }
      }
      break;
    default:
      return -1;
  }
  return -1;
}

static inline const char *const xr_calls_name_impl(long scno, int compat) {
  if (scno < 0 || scno >= XR_SYSCALL_MAX) {
    return NULL;
  }
  return xr_syscall_table_arm[scno];
}

#endif
