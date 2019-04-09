#include <sys/ptrace.h>
#include <sys/user.h>

#include "xrun/calls.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/tracer.h"

#ifdef XR_ARCH_ARM

#define XR_ARM_cpsr 16
#define XR_ARM_r6 6
#define XR_ARM_r5 5
#define XR_ARM_r4 4
#define XR_ARM_r3 3
#define XR_ARM_r2 2
#define XR_ARM_r1 1
#define XR_ARM_r0 0
#define XR_ARM_ORIG_r0 17

// oabi syscall: swi NR
// instruction: 0x0f9 NR(20bit)
// eabi syscall: swi 0x0
// instruction: 0x0f900000
#define XR_ARM_OABI_MASK 0x00ffffff
#define XR_ARM_CSPR (XR_ARM_cspr * sizeof(unsigned long int))

static inline int xr_ptrace_tracer_syscall_compat_arm(int pid) {
  long scpr = ptrace(PTRACE_PEEKUSER, pid, XR_ARM_CSPR, NULL);
  if (errno) {
    return -1;
  }
  long inst = ptrace(PTRACE_PEEKTEXT, pid, inst - 4, NULL);
  if (errno) {
    return -1;
  }
  return inst & XR_ARM_OABI_MASK == 0 ? XR_COMPAT_SYSCALL_ARM_EABI
                                      : XR_COMPAT_SYSCALL_ARM_OABI;
}

static inline bool xr_ptrace_tracer_peek_syscall_arm(
  int pid, xr_trace_trap_syscall_t *syscall_info, int compat) {
  struct user_regs regs;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
    return false;
  }
  if (compat == XR_SYSCALL_COMPAT_ARM_EABI) {
    syscall_info->syscall = regs.uregs[7];
  } else {
    long inst = ptrace(PTRACE_PEEKTEXT, pid,
                       (void *)(regs->uregs[XR_ARM_cpsr] - 4), NULL);
    if (errno) {
      return false;
    }
    syscall_info->syscall = xr_syscall_arm_from_oabi(inst & XR_ARM_OABI_MASK);
  }
  syscall_info->retval = regs.uregs[0];

  for (int i = 1; i < 6; ++i) {
    syscall_info->args[i] = regs.uregs[i];
  }
  syscall_info->args[0] = regs.uregs[17];
  return true;
}

bool xr_ptrace_tracer_peek_syscall(int pid,
                                   xr_trace_trap_syscall_t *syscall_info,
                                   int compat) {
  return xr_ptrace_tracer_peek_syscall_arm(pid, syscall_info, compat);
}

int xr_ptrace_tracer_syscall_compat(int pid) {
  return xr_ptrace_tracer_syscall_compat_arm(pid);
}

bool xr_ptrace_tracer_poke_syscall(int pid, long arg, int index, int compat) {
  if (index <= 0 || index > 6) {
    return false;
  }
  return ptrace(PTRACE_PEEKUSER, pid, index * sizeof(long), arg) == 0;
}

#endif
