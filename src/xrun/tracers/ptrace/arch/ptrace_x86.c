#include <errno.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include "xrun/calls.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/tracer.h"

#ifdef XR_ARCH_X86_IA32

#define XR_X86_SYSCALL_INST_LOW 0x0f
#define XR_X86_SYSCALL_INST_HIGH 0x05
#define XR_X86_INT0X80_INST_LOW 0xcd
#define XR_X86_INT0x80_INST_HIGH 0x80
#define XR_X64_PC_OFFSET (16 * sizeof(unsigned long long))
// offset of rip is 16
#define XR_X64_ALIGN_MASK ~0x07ul

static inline int xr_ptrace_tracer_syscall_compat_x86(int pid) {
  long rip = ptrace(PTRACE_PEEKUSER, pid, XR_X64_PC_OFFSET, NULL);
  if (errno) {
    return -1;
  }
  long sc_inst = rip - 2;
  long sc_inst_align = sc_inst & XR_X64_ALIGN_MASK;
  long offset = sc_inst - sc_inst_align;
  long inst[2] = {};
  unsigned char *byte = (unsigned char *)inst;
  inst[0] = ptrace(PTRACE_PEEKTEXT, pid, sc_inst_align, NULL);
  if (errno) {
    return -1;
  }
  if (offset == 0x7) {
    // syscall instruction cross address align
    inst[1] = ptrace(PTRACE_PEEKTEXT, pid, sc_inst_align + 8, NULL);
    if (errno) {
      return -1;
    }
  }
  unsigned char low = byte[offset], high = byte[offset + 1];
  if (low == XR_X86_INT0X80_INST_LOW && high == XR_X86_INT0x80_INST_HIGH) {
    return XR_SYSCALL_X86_COMPAT_IA32;
  } else if (low == XR_X86_SYSCALL_INST_LOW &&
             high == XR_X86_SYSCALL_INST_HIGH) {
    return XR_SYSCALL_X86_COMPAT_64;
  } else {
    return -1;
  }
}

static inline bool xr_ptrace_tracer_peek_syscall_x86(
  int pid, xr_trace_trap_syscall_t *syscall_info, int compat) {
  struct user_regs_struct regs;
  if (ptrace(PTRACE_GETREGS, pid, NULL, regs) == -1) {
    return false;
  }
#ifdef XR_ARCH_X86_64
  syscall_info->syscall = regs.orig_rax;
  syscall_info->retval = regs.rax;

  switch (compat) {
    case XR_SYSCALL_X86_COMPAT_64:
    case XR_SYSCALL_X86_COMPAT_X32:
      // x32 is similar to x64, with some difference at call number.
      syscall_info->args[0] = regs.rdi;
      syscall_info->args[1] = regs.rsi;
      syscall_info->args[2] = regs.rdx;
      syscall_info->args[3] = regs.r10;
      syscall_info->args[4] = regs.r8;
      syscall_info->args[5] = regs.r9;
      break;
    case XR_SYSCALL_X86_COMPAT_IA32:
      // if tracer is in 64bit and tracee is in ia32-compat-mode, the kernel
      // space of tracee is in 64 bits, but pass args via i386 syscall abi.
      syscall_info->args[0] = regs.rbx;
      syscall_info->args[1] = regs.rcx;
      syscall_info->args[2] = regs.rdx;
      syscall_info->args[3] = regs.rsi;
      syscall_info->args[4] = regs.rdi;
      syscall_info->args[5] = regs.rbp;
      break;
    default:
      return false;
  }

#else
  syscall_info->syscall = regs.orig_eax;
  syscall_info->retval = regs.eax;
  syscall_info->args[0] = regs.ebx;
  syscall_info->args[1] = regs.ecx;
  syscall_info->args[2] = regs.edx;
  syscall_info->args[3] = regs.esi;
  syscall_info->args[4] = regs.edi;
  syscall_info->args[5] = regs.ebp;
#endif
  return true;
}

bool xr_ptrace_tracer_peek_syscall(int pid,
                                   xr_trace_trap_syscall_t *syscall_info,
                                   int compat) {
  return xr_ptrace_tracer_peek_syscall_x86(pid, syscall_info, compat);
}

int xr_ptrace_tracer_syscall_compat(int pid) {
#ifdef XR_ARCH_X86_64
  return xr_ptrace_tracer_syscall_compat_x86(pid);
#else
  // compat mode is ignored.
  return XR_SYSCALL_COMPAT_MODE_X86;
#endif
}
#endif
