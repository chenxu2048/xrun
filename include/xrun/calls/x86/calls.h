#ifndef XR_CALL_X86_H
#define XR_CALL_X86_H

#include <string.h>

#define XR_COMPAT_SYSCALL_X86_64 1
#define XR_COMPAT_SYSCALL_X86_X32 2
#define XR_COMPAT_SYSCALL_X86_IA32 3

#ifdef XR_ARCH_X86_64
#define XR_ARCH_X86_IA32
#define XR_COMPAT_SYSCALL_DEFAULT XR_COMPAT_SYSCALL_X86_64
#else
#define XR_COMPAT_SYSCALL_DEFAULT XR_COMPAT_SYSCALL_X86_IA32
#endif

#include "./calls_64.h"

#include "./calls_32.h"

static inline int xr_calls_convert_ia32_impl(const char *name) {
  for (int i = 0; i < XR_IA32_SYSCALL_MAX; ++i) {
    if (xr_syscall_table_ia32[i] != NULL &&
        strcmp(name, xr_syscall_table_ia32[i]) == 0) {
      return i;
    }
  }
  return -1;
}

#ifdef XR_ARCH_X86_64

static inline int xr_calls_convert_impl(const char *name, int compat) {
  switch (compat) {
    case XR_COMPAT_SYSCALL_X86_64: {
      for (int i = 0; i < XR_SYSCALL_MAX; ++i) {
        if (xr_syscall_table_x64[i] != NULL &&
            strcmp(name, xr_syscall_table_x64[i]) == 0) {
          return i;
        }
      }
      break;
    }
    case XR_COMPAT_SYSCALL_X86_X32: {
      for (int i = XR_SYSCALL_MAX - 1; i >= 0; --i) {
        if (xr_syscall_table_x64[i] != NULL &&
            strcmp(name, xr_syscall_table_x64[i]) == 0) {
          return i;
        }
      }
      break;
    }
    case XR_COMPAT_SYSCALL_X86_IA32: {
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

static inline const char *const xr_calls_name_impl(long scno, int compat) {
  if (compat == XR_COMPAT_SYSCALL_X86_IA32) {
    return (XR_IA32_SYSCALL_MAX > scno && scno >= 0)
             ? xr_syscall_table_ia32[scno]
             : NULL;
  } else {
    return (XR_X64_SYSCALL_MAX > scno && scno >= 0) ? xr_syscall_table_x64[scno]
                                                    : NULL;
  }
}

#else

static inline const char *const xr_calls_name_impl(long scno, int compat) {
  return (XR_IA32_SYSCALL_MAX > scno && scno >= 0) ? xr_syscall_table_ia32[scno]
                                                   : NULL;
}

static inline int xr_calls_convert_impl(const char *name, int compat) {
  return xr_calls_convert_ia32_impl(name);
}

#endif

#endif
