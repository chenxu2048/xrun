#include <sys/ptrace.h>
#include <sys/user.h>

#include "xrun/calls.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/tracer.h"

#ifdef XR_ARCH_ARM

#define XR_ARM_pc 15
#define XR_ARM_r6 6
#define XR_ARM_r5 5
#define XR_ARM_r4 4
#define XR_ARM_r3 3
#define XR_ARM_r2 2
#define XR_ARM_r1 1
#define XR_ARM_r0 0
#define XR_ARM_ORIG_r0 17

// oabi syscall: swi NR + 0x900000
// instruction: 0xef NR(24bit) + 0x900000
// eabi syscall: swi 0x0
// instruction: 0xef000000
#define XR_ARM_EABI_INST 0xef000000
#define XR_ARM_OABI_MASK 0x00ffffff

/* define in /arch/arm/kernel/traps.c */
#define XR_ARM_PRIVATE_CALLS(call) ((call) > 0xf0000 && (call) <= 0xf07ff)

#define XR_ARM_PC (XR_ARM_pc * sizeof(unsigned long int))

static inline int xr_ptrace_tracer_syscall_compat_arm(int pid) {
  long pc = ptrace(PTRACE_PEEKUSER, pid, XR_ARM_PC, NULL);
  if (errno) {
    return XR_COMPAT_SYSCALL_INVALID;
  }
  long inst = ptrace(PTRACE_PEEKTEXT, pid, pc - 4, NULL);
  if (errno) {
    return XR_COMPAT_SYSCALL_INVALID;
  }
  if (inst == XR_ARM_EABI_INST) {
    return XR_COMPAT_SYSCALL_ARM_EABI;
  }
  return ((inst & ~XR_ARM_OABI_MASK) == XR_ARM_EABI_INST)
           ? XR_COMPAT_SYSCALL_ARM_OABI
           : XR_COMPAT_SYSCALL_INVALID;
}

static inline bool xr_ptrace_tracer_peek_syscall_arm(
  int pid, xr_trace_trap_syscall_t *syscall_info, int compat) {
  struct user_regs regs;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
    return false;
  }
  if (compat == XR_COMPAT_SYSCALL_ARM_EABI) {
    syscall_info->syscall = regs.uregs[7];
  } else {
    long inst =
      ptrace(PTRACE_PEEKTEXT, pid, (void *)(regs.uregs[XR_ARM_pc] - 4), NULL);
    if (errno) {
      return false;
    }
    syscall_info->syscall = xr_syscall_arm_from_oabi(inst & XR_ARM_OABI_MASK);
  }
  syscall_info->retval = regs.uregs[0];
  if (XR_ARM_PRIVATE_CALLS(syscall_info->syscall)) {
    syscall_info->syscall =
      xr_syscall_arm_private_convert(syscall_info->syscall);
  }

  for (int i = 1; i <= 6; ++i) {
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
  return ptrace(PTRACE_POKEUSER, pid, index * sizeof(long), arg) == 0;
}

#endif
