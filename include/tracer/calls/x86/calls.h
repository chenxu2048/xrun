#ifndef XR_CALL_X86_H
#define XR_CALL_X86_H

#include <string.h>

#include "./calls_32.h"

#include "./calls_64.h"

#define XR_SYSCALL_X86_COMPAT_X64 0
#define XR_SYSCALL_X86_COMPAT_X32 1
#define XR_SYSCALL_X86_COMPAT_IA32 2

static inline int xr_calls_convert_ia32_impl(const char *name) {
  for (int i = 0; i <= XR_SYSCALL_IA32_MAX; ++i) {
    if (strcmp(name, xr_syscall_table_ia32[i]) == 0) {
      return i;
    }
  }
  return -1;
}

static inline int xr_calls_convert_impl(const char *name, int compat) {
  switch (compat) {
    case XR_SYSCALL_X86_COMPAT_X64: {
      for (int i = 0; i <= XR_SYSCALL_MAX; ++i) {
        if (strcmp(name, xr_syscall_table_x64[i]) == 0) {
          return i;
        }
      }
      break;
    }
    case XR_SYSCALL_X86_COMPAT_X32: {
      for (int i = XR_SYSCALL_MAX; i >= 0; --i) {
        if (strcmp(name, xr_syscall_table_x64[i]) == 0) {
          return i;
        }
      }
      break;
    }
    case XR_SYSCALL_X86_COMPAT_IA32: {
      int scno = xr_calls_convert_ia32_impl(name);
      if (scno != -1) {
        return xr_syscall_table_x86_to_x64[scno];
      }
      break;
    }
    default:
      break;
  }
  return -1;
}

#if !defined(XR_ARCH_X64) && !defined(XR_ARCH_X32)
#undef XR_CALLS_CONVERT
#define XR_CALLS_CONVERT(name, compat) xr_calls_convert_ia32_impl(name)
#endif

#endif
