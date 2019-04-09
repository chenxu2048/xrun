AM_LDFLAGS = -shared -fPIC
AM_CFLAGS = -I $(top_srcdir)/include -fPIC

CHECKERS = checkers/io_checker.c \
   checkers/file_checker.c \
   checkers/fork_checker.c \
   checkers/resource_checker.c \
   checkers/syscall_checker.c

PTRACE_TRACERS = tracers/ptrace/tracer.c
CALLS=

if BUILD_ARM
   PTRACE_TRACERS += tracers/ptrace/arm/ptrace.c
   CALLS += calls/arm/calls.c
endif

if BUILD_X86
   PTRACE_TRACERS += tracers/ptrace/x86/ptrace.c
   CALLS += calls/x86/calls_32.c calls/x86/calls_64.c
endif

TRACERS = $(PTRACE_TRACERS)

UTILS = utils/json.c utils/list.c

LIBSOURCE = process.c tracer.c entry.c option.c

xrunlibdir = $(libdir)
xrunlib_PROGRAMS = libxrun.so
libxrun_so_SOURCES = $(LIBSOURCE) \
   $(UTILS) \
   $(CALLS) \
   $(CHECKERS) \
   $(TRACERS)

xrunlib_LIBRARIES = libxrun.a
libxrun_a_SOURCES = $(LIBSOURCE) \
   $(UTILS) \
   $(CALLS) \
   $(CHECKERS) \
   $(TRACERS)
