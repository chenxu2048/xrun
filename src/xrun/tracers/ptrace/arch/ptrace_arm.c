#include <sys/ptrace.h>
#include <sys/user.h>

#include "xrun/calls.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/tracer.h"

#ifdef XR_ARCH_ARM

static inline bool xr_ptrace_tracer_peek_syscall_arm(
  int pid, xr_trace_trap_syscall_t *syscall_info) {
  struct user_regs regs;
  if (ptrace(PTRACE_GETREGS, pid, NULL, regs) == -1) {
    return false;
  }
  syscall_info->syscall = regs.uregs[7];
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
#endif
