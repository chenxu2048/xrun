#ifndef _XR_PTRACE_TRACER
#define _XR_PTRACE_TRACER

#include "xrun/utils/string.h"

#include <stdbool.h>

typedef struct xr_tracer_s xr_tracer_t;
typedef struct xr_trace_trap_s xr_trace_trap_t;

#define _XR_STRING_DEFAULT_CAPACITY 64

bool xr_ptrace_tracer_spawn(xr_tracer_t *tracer);

bool xr_ptrace_tracer_step(xr_tracer_t *tracer, xr_trace_trap_t *trap);

bool xr_ptrace_tracer_trap(xr_tracer_t *tracer, xr_trace_trap_t *trap);

bool xr_ptrace_tracer_get(xr_tracer_t *tracer, int pid, void *address,
                          void *buffer, size_t size);

bool xr_ptrace_tracer_set(xr_tracer_t *tracer, int pid, void *address,
                          const void *buffer, size_t size);

bool xr_ptrace_tracer_strcpy(xr_tracer_t *tracer, int pid, void *address,
                             xr_string_t *str);

static inline void xr_tracer_ptrace_init(xr_tracer_t *tracer,
                                         const char *name) {
  xr_tracer_init(tracer, name);
  tracer->spwan = xr_ptrace_tracer_spawn;
  tracer->step = xr_ptrace_tracer_step;
  tracer->trap = xr_ptrace_tracer_trap;
  tracer->get = xr_ptrace_tracer_get;
  tracer->set = xr_ptrace_tracer_set;
  tracer->strcpy = xr_ptrace_tracer_strcpy;
}

#endif
