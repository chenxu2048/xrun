#ifndef XR_CALLS_ARM_H
#define XR_CALLS_ARM_H

#include <string.h>

#include "./calls_arm.h"

static inline int xr_calls_convert_impl(const char *name, int compat) {
  for (int i = 0; i <= XR_SYSCALL_MAX; ++i) {
    if (strcmp(name, xr_syscall_table_arm[i]) == 0) {
      return i;
    }
  }
  return -1;
}

#endif
