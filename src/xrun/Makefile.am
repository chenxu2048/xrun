AM_LDFLAGS = -shared -fPIC
AM_CFLAGS = -I $(top_srcdir)/include -fPIC

CHECKERS=checkers/io_checker.c checkers/file_checker.c checkers/fork_checker.c checkers/resource_checker.c checkers/syscall_checker.c

PTRACE_TRACERS=tracers/ptrace/tracer.c tracers/ptrace/arch/ptrace_x86.c tracers/ptrace/arch/ptrace_arm.c
TRACERS=$(PTRACE_TRACERS)

ARM_CALLS=calls/arm/calls.c
X86_CALLS=calls/x86/calls_32.c calls/x86/calls_64.c
CALLS=$(ARM_CALLS) $(X86_CALLS)

UTILS=utils/json.c utils/list.c

xrunlibdir = $(libdir)
xrunlib_PROGRAMS = libxrun.so
libxrun_so_SOURCES = process.c tracer.c entry.c \
   $(UTILS) \
   $(CALLS) \
   $(CHECKERS) \
   $(TRACERS)

xrunlib_LIBRARIES = libxrun.a
libxrun_a_SOURCES = process.c tracer.c entry.c \
   $(UTILS) \
   $(CALLS) \
   $(CHECKERS) \
   $(TRACERS)
