#ifndef _XR_PTRACE_TRACER
#define _XR_PTRACE_TRACER

#include <stdbool.h>

#include "xrun/entry.h"
#include "xrun/tracer.h"
#include "xrun/utils/string.h"

typedef struct xr_tracer_s xr_tracer_t;
typedef struct xr_trace_trap_s xr_trace_trap_t;

bool xr_ptrace_tracer_spawn(xr_tracer_t *tracer, xr_entry_t *entry);

bool xr_ptrace_tracer_step(xr_tracer_t *tracer, xr_trace_trap_t *trap);

bool xr_ptrace_tracer_trap(xr_tracer_t *tracer, xr_trace_trap_t *trap);

bool xr_ptrace_tracer_get(xr_tracer_t *tracer, int pid, void *address,
                          void *buffer, size_t size);

bool xr_ptrace_tracer_set(xr_tracer_t *tracer, int pid, void *address,
                          const void *buffer, size_t size);

bool xr_ptrace_tracer_strcpy(xr_tracer_t *tracer, int pid, void *address,
                             xr_string_t *str);

void xr_ptrace_tracer_kill(xr_tracer_t *tracer, pid_t pid);

void xr_ptrace_tracer_clean(xr_tracer_t *tracer);

void xr_ptrace_tracer_delete(xr_tracer_t *tracer);

void xr_tracer_ptrace_init(xr_tracer_t *tracer, const char *name);

#endif
