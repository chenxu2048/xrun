#ifndef XR_TRACER_H
#define XR_TRACER_H

#include <stdbool.h>

#include "lib/list.h"
#include "tracer/checker.h"
#include "tracer/process.h"
#include "tracer/result.h"

typedef struct xr_tracer_s xr_tracer_t;
typedef struct xr_tracer_result_s xr_tracer_result_t;
typedef struct xr_option_s xr_option_t;
typedef struct xr_trace_trap_s xr_trace_trap_t;

typedef bool xr_tracer_op_spawn_f(xr_tracer_t *tracer);

typedef bool xr_tracer_op_step_f(xr_tracer_t *tracer, xr_trace_trap_t *trap);

typedef bool xr_tracer_op_trap_f(xr_tracer_t *tracer, xr_trace_trap_t *trap);

typedef bool xr_tracer_op_get_f(xr_tracer_t *tracer, int pid, void *address,
                                char *buffer, size_t size);

typedef bool xr_tracer_op_set_f(xr_tracer_t *tracer, int pid, void *address,
                                const char *buffer, size_t size);

typedef void xr_tracer_op_kill_f(xr_tracer_t *tracer, int pid);

struct xr_trace_trap_syscall_s {
  long syscall;
};

struct xr_trace_trap_s {
  enum {
    XR_TRACE_TRAP_SYSCALL,
    XR_TRACE_TRAP_SIGNAL,
    XR_TRACE_TRAP_EXIT,
    XR_TRACE_TRAP_NONE
  } trap;

  union {
    int stop_signal;
  };
  xr_process_t *process;
  xr_thread_t *thread;
};

struct xr_tracer_s {
  const char *name;

  struct xr_tracer_operation {
    xr_tracer_op_spawn_f *spwan;
    xr_tracer_op_step_f *step;
    xr_tracer_op_trap_f *trap;
    xr_tracer_op_get_f *get;
    xr_tracer_op_set_f *set;
  };
  void *tracer_private;

  xr_option_t *option;

  xr_list_t processes;
  xr_list_t checkers;

  xr_checker_t *failed_checker;
  xr_trace_trap_t *trap;
};

xr_tracer_t *xr_tracer_new(const char *name);
void xr_tracer_delete(xr_tracer_t *tracer);

bool xr_tracer_trace(xr_tracer_t *tracer, xr_option_t *option,
                     xr_tracer_result_t *result);

bool xr_tracer_setup(xr_tracer_t *tracer, xr_option_t *option);
bool xr_tracer_check(xr_tracer_t *tracer, xr_tracer_result_t *result);
void xr_tracer_clean(xr_tracer_t *tracer);

#endif
