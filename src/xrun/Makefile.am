AM_LDFLAGS = -shared -fPIC
AM_CFLAGS = -I $(top_srcdir)/include -fPIC

CHECKERS=checkers/io_checker.c checkers/file_checker.c checkers/fork_checker.c checkers/resource_checker.c checkers/syscall_checker.c

PTRACE_TRACERS=tracers/ptrace/tracer.c tracers/ptrace/arch/$(XR_ARCH_NAME)/ptrace.c
TRACERS=$(PTRACE_TRACERS)

CALLS=calls/$(XR_ARCH_NAME)/*.c

UTILS=utils/json.c utils/list.c

LIBSOURCE=process.c tracer.c entry.c option.c

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
