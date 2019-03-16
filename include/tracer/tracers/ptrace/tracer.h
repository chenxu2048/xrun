#ifndef _XR_PTRACE_TRACER
#define _XR_PTRACE_TRACER

#include <stdbool.h>

typedef struct xr_tracer_s xr_tracer_t;
typedef struct xr_trace_trap_s xr_trace_trap_t;

#define _XR_STRING_DEFAULT_CAPACITY 64

bool xr_ptrace_tracer_spawn(xr_tracer_t *tracer);

bool xr_ptrace_tracer_step(xr_tracer_t *tracer);

xr_trace_trap_t xr_ptrace_tracer_trap(xr_tracer_t *tracer);

bool xr_ptrace_tracer_get(xr_tracer_t *tracer, int pid, void *address,
                          char *buffer, size_t size);

bool xr_ptrace_tracer_set(xr_tracer_t *tracer, int pid, void *address,
                          const char *buffer, size_t size);

xr_string_t *xr_ptrace_strcpy(xr_tracer_t *tracer, int pid, void *address,
                              size_t max_size);

#endif
