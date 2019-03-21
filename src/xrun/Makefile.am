AM_LDFLAGS = -shared -fPIC
AM_CPPFLAGS = -I $(top_srcdir)/include -fPIC

SUBDIRS = checkers tracers

xrunlibdir = $(libdir)
xrunlib_PROGRAMS = libxrun.so
libxrun_so_LDADD = checkers/libcheckers.a tracers/libtracers.a
libxrun_so_SOURCES = process.c tracer.c \
   utils/json.c utils/list.c \
   calls/arm/calls.c \
   calls/x86/calls_32.c calls/x86/calls_64.c

xrunlib_LIBRARIES = libxrun.a
libxrun_a_LIBADD = checkers/libcheckers.a tracers/libtracers.a
libxrun_a_SOURCES = process.c tracer.c \
   utils/json.c utils/list.c \
   calls/arm/calls.c \
   calls/x86/calls_32.c calls/x86/calls_64.c
