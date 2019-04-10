#include <stdbool.h>
#include <stdlib.h>

#include "xrun/checker.h"
#include "xrun/checkers.h"
#include "xrun/option.h"
#include "xrun/process.h"
#include "xrun/result.h"
#include "xrun/tracer.h"
#include "xrun/utils/utils.h"

#define _XR_TRACER_TRACE_ERROR(ok, tracer, ...) \
  do {                                          \
    ok = false;                                 \
    _XR_TRACER_ERROR(tracer, __VA_ARGS__);      \
  } while (0)

static inline void xr_collect_process(xr_process_t *process,
                                      xr_tracer_process_result_t *presult) {
  presult->nthread = process->nthread;
  presult->memory = process->memory;
  presult->nfile = process->nfile;
  presult->io_read = presult->io_write = 0;
  xr_thread_t *thread;
  _xr_list_for_each_entry(&process->threads, thread, xr_thread_t, threads) {
    long long nread = xr_file_set_get_read(&thread->fset);
    if (nread > presult->io_read) {
      presult->io_read = nread;
    }
    long long nwrite = xr_file_set_get_write(&thread->fset);
    if (nread > presult->io_write) {
      presult->io_write = nwrite;
    }
  }
}

#define XR_RESULT_PROCESS_EXIT_ABORT -1
static inline void xr_result_process(xr_result_t *result, xr_process_t *process,
                                     int exit_code) {
  xr_tracer_process_result_t *presult = _XR_NEW(xr_tracer_process_result_t);
  xr_collect_process(process, presult);
  if (exit_code == XR_RESULT_PROCESS_EXIT_ABORT) {
    presult->next = result->aborted_processes;
    result->aborted_processes = presult;
  } else {
    presult->next = result->exited_processes;
    result->exited_processes = presult;
  }
  return;
}

bool xr_tracer_trace(xr_tracer_t *tracer, xr_entry_t *entry,
                     xr_result_t *result) {
  bool ok = true;
  xr_trace_trap_t trap = {.trap = XR_TRACE_TRAP_NONE};
  result->status = XR_RESULT_UNKNOWN;
  if (tracer->spwan(tracer, entry) == false) {
    _XR_TRACER_TRACE_ERROR(ok, tracer, "tracer spwan error.");
  } else {
    while (ok && xr_list_empty(&tracer->processes) == false) {
      if (tracer->trap(tracer, &trap) == false) {
        _XR_TRACER_TRACE_ERROR(ok, tracer, "tracer trap failed.");
        break;
      }
      if (xr_tracer_check(tracer, result, &trap) == false) {
        ok = false;
        break;
      }

      if (trap.trap == XR_TRACE_TRAP_EXIT) {
        xr_process_t *trap_process = trap.thread->process;
        xr_process_remove_thread(trap_process, trap.thread);
        if (xr_list_empty(&trap_process->threads)) {
          xr_result_process(result, trap_process, trap.exit_code);
          xr_list_del(&trap_process->processes);
          xr_process_delete(trap_process);
        }
        // a exited thread/process do not step again.
        continue;
      }
      if (tracer->step(tracer, &trap) == false) {
        _XR_TRACER_TRACE_ERROR(ok, tracer, "tracer step failed.");
      }
    }
  }
  if (!ok) {
    xr_list_t *cur_process, *tmp_process;
    _xr_list_for_each_safe(&tracer->processes, cur_process, tmp_process) {
      xr_process_t *process =
        xr_list_entry(cur_process, xr_process_t, processes);
      _XR_CALLP(tracer, kill, process->pid);
      xr_result_process(result, process, XR_RESULT_PROCESS_EXIT_ABORT);
    }
    if (result->status == XR_RESULT_UNKNOWN) {
      result->status = XR_RESULT_TRACERERR;
    }
  }
  result->nprocess = tracer->nprocess;
  xr_tracer_clean(tracer);
  return result->status == XR_RESULT_OK;
}

bool xr_tracer_setup(xr_tracer_t *tracer, xr_option_t *option) {
  tracer->option = option;
  xr_checker_t *checker;
  _xr_list_for_each_entry(&(tracer->checkers), checker, xr_checker_t,
                          checkers) {
    if (checker->setup(checker, tracer->option) == false) {
      return _XR_TRACER_ERROR(tracer, "checker with id %d setup failed",
                              checker->checker_id);
    }
  }
  return true;
}

bool xr_tracer_check(xr_tracer_t *tracer, xr_result_t *result,
                     xr_trace_trap_t *trap) {
  xr_checker_t *checker;
  _xr_list_for_each_entry(&(tracer->checkers), checker, xr_checker_t,
                          checkers) {
    if (_XR_CALLP(checker, check, tracer, trap) == false) {
      _XR_CALLP(checker, result, tracer, result);
      result->epid = trap->thread->process->pid;
      result->etid = trap->thread->tid;
      return false;
    };
  }
  return true;
}

void xr_tracer_clean(xr_tracer_t *tracer) {
  xr_list_t *cur, *temp;
  xr_process_t *process;

  // free tracer process list and delete tracer process
  _xr_list_for_each_safe(&(tracer->processes), cur, temp) {
    xr_list_del(cur);
    process = xr_list_entry(cur, xr_process_t, processes);
    tracer->kill(tracer, process->pid);
    xr_process_delete(process);
    free(process);
  }

  _XR_CALLP(tracer, clean);
  tracer->nprocess = 0;
  tracer->nthread = 0;
  tracer->failed_checker = NULL;
}

void xr_tracer_delete(xr_tracer_t *tracer) {
  // delete all checkers
  xr_list_t *cur, *temp;
  xr_checker_t *checker;
  _xr_list_for_each_safe(&(tracer->checkers), cur, temp) {
    xr_list_del(cur);
    checker = xr_list_entry(cur, xr_checker_t, checkers);
    checker->_delete(checker);
    free(checker);
  }
  // clean up all process
  xr_tracer_clean(tracer);
  _XR_CALLP(tracer, _delete);
}

bool xr_tracer_error(xr_tracer_t *tracer, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  if (errno == 0) {
    xr_error_verror(&tracer->error, msg, args);
  } else {
    xr_error_vnerror(&tracer->error, errno, msg, args);
  }
  return false;
}

bool xr_tracer_add_checker(xr_tracer_t *tracer, xr_checker_id_t cid) {
  xr_checker_t *checker = _XR_NEW(xr_checker_t);
  if (cid >= sizeof(xr_checker_init_funcs) / sizeof(xr_checker_init_f)) {
    return _XR_TRACER_ERROR(tracer, "add checker with invalid checker id");
  } else if (xr_checker_init_funcs[cid] == NULL) {
    return _XR_TRACER_ERROR(tracer, "checker init function does not exist.");
  }
  xr_checker_init_funcs[cid](checker);
  xr_list_add(&tracer->checkers, &checker->checkers);
  return true;
}
