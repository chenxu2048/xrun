#ifndef XR_CALL_X86_H
#define XR_CALL_X86_H

#include <string.h>

#ifdef XR_ARCH_X86_64

#define XR_SYSCALL_X86_COMPAT_64 0
#define XR_SYSCALL_X86_COMPAT_X32 1
#define XR_SYSCALL_X86_COMPAT_IA32 2

#define XR_ARCH_X86_IA32
#endif

#include "./calls_64.h"

#include "./calls_32.h"

static inline int xr_calls_convert_ia32_impl(const char *name) {
  for (int i = 0; i <= XR_IA32_SYSCALL_MAX; ++i) {
    if (strcmp(name, xr_syscall_table_ia32[i]) == 0) {
      return i;
    }
  }
  return -1;
}

static inline int xr_calls_convert_impl(const char *name, int compat) {
  switch (compat) {
    case XR_SYSCALL_X86_COMPAT_64: {
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

static const char *const xr_calls_name_impl(long scno, int compat) {
  if (compat == XR_SYSCALL_X86_COMPAT_IA32) {
    return XR_IA32_SYSCALL_MAX > scno ? xr_syscall_table_ia32[scno] : "invalid";
  } else {
    return XR_X64_SYSCALL_MAX > scno ? xr_syscall_table_x64[scno] : "invalid";
  }
}

#if !defined(XR_ARCH_X86_64)
#undef XR_CALLS_CONVERT
#define XR_CALLS_CONVERT(name, compat) xr_calls_convert_ia32_impl(name)

#undef XR_CALLS_NAME
#define XR_CALLS_NAME(scno, compat) \
  xr_calls_name_impl(scno, XR_SYSCALL_X86_COMPAT_IA32)

#endif

#endif
