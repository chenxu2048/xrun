#include <errno.h>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <unistd.h>

#include "xrun/calls.h"
#include "xrun/process.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/elf.h"
#include "xrun/tracers/ptrace/tracer.h"

#ifdef XR_ARCH_X86_IA32

#define XR_X86_SYSCALL_INST_LOW 0x0f
#define XR_X86_SYSCALL_INST_HIGH 0x05
#define XR_X86_INT0X80_INST_LOW 0xcd
#define XR_X86_INT0x80_INST_HIGH 0x80
#define XR_X64_PC_OFFSET (RIP * sizeof(unsigned long long))
#define XR_X64_SCNO_OFFSET (ORIG_RAX * sizeof(unsigned long long))
// offset of rip is 16
#define XR_X64_ALIGN_MASK ~0x07ul

#define XR_X86_SIGNED_32_BIT 0x80000000
#define XR_X86_SIGNED_32_BIT_EXTEND 0xffffffff00000000

#ifdef XR_ARCH_X86_64

int xr_ptrace_tracer_syscall_compat(int pid) {
  long rip = ptrace(PTRACE_PEEKUSER, pid, XR_X64_PC_OFFSET, NULL);
  if (errno) {
    return XR_COMPAT_SYSCALL_INVALID;
  }
  long sc_inst = rip - 2;
  long sc_inst_align = sc_inst & XR_X64_ALIGN_MASK;
  long offset = sc_inst - sc_inst_align;
  long inst[2] = {};
  unsigned char *byte = (unsigned char *)inst;
  inst[0] = ptrace(PTRACE_PEEKTEXT, pid, sc_inst_align, NULL);
  if (errno) {
    return XR_COMPAT_SYSCALL_INVALID;
  }
  if (offset == 0x7) {
    // syscall instruction cross address align
    inst[1] = ptrace(PTRACE_PEEKTEXT, pid, sc_inst_align + 8, NULL);
    if (errno) {
      return XR_COMPAT_SYSCALL_INVALID;
    }
  }
  unsigned char low = byte[offset], high = byte[offset + 1];
  if (low == XR_X86_INT0X80_INST_LOW && high == XR_X86_INT0x80_INST_HIGH) {
    return XR_COMPAT_SYSCALL_X86_IA32;
  } else if (low == XR_X86_SYSCALL_INST_LOW &&
             high == XR_X86_SYSCALL_INST_HIGH) {
    long syscall = ptrace(PTRACE_PEEKUSER, pid, XR_X64_SCNO_OFFSET, NULL);
    if (errno) {
      return XR_COMPAT_SYSCALL_INVALID;
    } else if (syscall & XR_X32_MASK_BIT_SYSCALL) {
      return XR_COMPAT_SYSCALL_X86_X32;
    } else {
      return XR_COMPAT_SYSCALL_X86_64;
    }
  } else {
    return XR_COMPAT_SYSCALL_INVALID;
  }
}

bool xr_ptrace_tracer_peek_syscall(int pid,
                                   xr_trace_trap_syscall_t *syscall_info,
                                   int compat) {
  struct user_regs_struct regs;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
    return false;
  }
  syscall_info->syscall = regs.orig_rax;
  syscall_info->retval = regs.rax;

  switch (compat) {
      // fallback to x64
    case XR_COMPAT_SYSCALL_X86_X32:
      syscall_info->syscall = xr_syscall_x64_from_x32(syscall_info->syscall);
      // x32 is similar to x64, with some difference at call number.
      // fallback to x64
    case XR_COMPAT_SYSCALL_X86_64:
      syscall_info->args[0] = regs.rdi;
      syscall_info->args[1] = regs.rsi;
      syscall_info->args[2] = regs.rdx;
      syscall_info->args[3] = regs.r10;
      syscall_info->args[4] = regs.r8;
      syscall_info->args[5] = regs.r9;
      break;
    case XR_COMPAT_SYSCALL_X86_IA32:
      // if tracer is in 64bit and tracee is in ia32-compat-mode, the kernel
      // space of tracee is in 64 bits, but pass args via i386 syscall abi.
      syscall_info->syscall = xr_syscall_x64_from_x86(syscall_info->syscall);
      syscall_info->args[0] = regs.rbx;
      syscall_info->args[1] = regs.rcx;
      syscall_info->args[2] = regs.rdx;
      syscall_info->args[3] = regs.rsi;
      syscall_info->args[4] = regs.rdi;
      syscall_info->args[5] = regs.rbp;
      if (syscall_info->retval & XR_X86_SIGNED_32_BIT) {
        // do signed patch
        syscall_info->retval |= XR_X86_SIGNED_32_BIT_EXTEND;
      }
      break;
    default:
      return false;
  }
  return true;
}

bool xr_ptrace_tracer_poke_syscall(int pid, long arg, int index, int compat) {
  static const long arg_index[] = {RDI, RSI, RDX, R10, R8, R9};
  static const long arg_index_32[] = {RBX, RCX, RDX, RSI, RDI, RBP};
  if (index > 5 || index < 0) {
    return false;
  }
  long offset = compat == XR_COMPAT_SYSCALL_X86_IA32 ? arg_index_32[index]
                                                     : arg_index[index];
  return ptrace(PTRACE_POKEUSER, pid, offset * 8, 0) != -1;
}

int xr_ptrace_tracer_elf_compat(xr_path_t *elf) {
  e_ident_t eident;
  int elffd = open(elf->string, O_RDONLY);
  if (elffd == -1) {
    // rollback
    return XR_COMPAT_SYSCALL_INVALID;
  }
  int rd = read(elffd, &eident, sizeof(eident));
  close(elffd);
  if (rd != sizeof(eident)) {
    return XR_COMPAT_SYSCALL_INVALID;
  }
  if (ELF_MAGIC_CHECK(eident)) {
    return XR_COMPAT_SYSCALL_INVALID;
  }
  return ELF_CLASS(eident) == ELFCLASS64 ? XR_COMPAT_SYSCALL_X86_64
                                         : XR_COMPAT_SYSCALL_X86_IA32;
}

#else /* XR_ARCH_X86_64 */

int xr_ptrace_tracer_syscall_compat(int pid) {
  return XR_COMPAT_SYSCALL_X86_IA32;
}

bool xr_ptrace_tracer_peek_syscall(int pid,
                                   xr_trace_trap_syscall_t *syscall_info,
                                   int compat) {
  struct user_regs_struct regs;
  if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
    return false;
  }
  syscall_info->syscall = regs.orig_eax;
  syscall_info->retval = regs.eax;
  syscall_info->args[0] = regs.ebx;
  syscall_info->args[1] = regs.ecx;
  syscall_info->args[2] = regs.edx;
  syscall_info->args[3] = regs.esi;
  syscall_info->args[4] = regs.edi;
  syscall_info->args[5] = regs.ebp;
  return true;
}

int xr_ptrace_tracer_elf_compat(xr_path_t *elf) {
  return XR_COMPAT_SYSCALL_X86_IA32;
}

bool xr_ptrace_tracer_poke_syscall(int pid, long arg, int index, int compat) {
  static const long arg_index[] = {RBX, RCX, RDX, RSI, RDI, RBP};
  if (index > 5 || index < 0) {
    return false;
  }
  return ptrace(PTRACE_POKEUSER, pid, arg_index[index] * 4, 0) != -1;
}

#endif /* XR_ARCH_X86_64 */

#endif
