#include <sys/ptrace.h>
#include <sys/user.h>

#include "xrun/calls.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/tracer.h"

#ifdef XR_ARCH_X86_IA32

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
      syscall_info->args[0] = regs.rdi;
      syscall_info->args[1] = regs.rsi;
      syscall_info->args[2] = regs.rdx;
      syscall_info->args[3] = regs.r10;
      syscall_info->args[4] = regs.r8;
      syscall_info->args[5] = regs.r9;
      break;
    case XR_SYSCALL_X86_COMPAT_IA32:
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
#endif
