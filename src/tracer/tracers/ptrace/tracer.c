#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include "lib/utils.h"
#include "tracer/option.h"
#include "tracer/tracer.h"
#include "tracer/tracers/ptrace/tracer.h"

static inline bool rlimit_cpu_time(xr_time_t time) {
  struct rlimit rlimit = {.rlim_cur = time / 1000, .rlim_max = time / 1000 + 1};
  return setrlimit(RLIMIT_CPU, &rlimit) != 0;
}

static inline bool rlimit_memory(int memory) {
  struct rlimit address = {.rlim_cur = memory * 2, .rlim_max = memory * 2 + 1};
  struct rlimit data = {.rlim_cur = memory, .rlim_max = memory + 1};
  return setrlimit(RLIMIT_AS, &address) != 0 &&
         setrlimit(RLIMIT_DATA, &data) != 0;
}

static inline bool do_rlimit_setup(xr_option_t *option) {
  return rlimit_memory(option.process.memory) &&
         rlimit_cpu_time(option.process.sys_time);
}

static bool get_syscall_info(xr_trace_trap_t *trap) {
  return false;
}

static bool get_resource_info(xr_trace_trap_t *trap) {
  struct rusage resource;
  if (getrusage(trap->thread->tid, &rusage) == -1) {
    return false;
  }
  trap->resource_info.time =
      xr_time_from_timeval(resource.ru_stime, resource.ru_utime);
  trap->resource_info.memory_size = resource.ru_maxrss;
  return true;
}

static xr_thread_t *create_spawned_process(xr_tracer_t *tracer, pid_t child) {
  xr_process_t *process = _XR_NEW(xr_process_t);
  xr_thread_t *thread = _XR_NEW(xr_thread_t);
  process->pid = child;
  thread->tid = child;
  thread->process = process;
  xr_list_add(&(process->threads), &(thread->threads));
  xr_list_add(&(thread->tracer_threads), &(tracer->threads));
  xr_list_add(&(process->processes), &(tracer->processes));
  return thread;
}

bool xr_ptrace_tracer_spawn(xr_tracer_t *tracer) {
  pid_t fork_ret = vfork();
  if (fork_ret < 0) {
    return false;
  } else if (fork_ret == 0) {
    // TODO
    do_rlimit_setup(tracer->option);
    do_exec(tracer->option);
  } else {
    create_spawned_process(tracer, fork_ret);
  }
}

bool xr_ptrace_tracer_step(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  return ptrace(PTRACE_SYSCALL, trap->thread->tid, NULL, NULL);
}

bool xr_ptrace_tracer_trap(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  struct rusage ru;
  int status;
  pid_t pid = wait(&status);
  if (pid == -1) {
    return false;
  }

  // trap stopped thread
  bool new_thread = true;
  _xr_list_for_each_entry(&(tracer->threads), trap->thread, xr_thread_t,
                          tracer_threads) {
    if (trap->thread->tid == pid) {
      new_thread = false;
      break;
    }
  }
  if (new_thread) {
    trap->thread = create_spawned_process(tracer, pid);
  }

  if (WIFEXITED(status)) {
    trap->trap = XR_TRACE_TRAP_EXIT;
    trap->exit_code = WEXITSTATUS(status);
  } else if (WSTOPSIG(status) != SIGTRAP) {
    trap->trap = XR_TRACE_TRAP_SIGNAL;
    trap->stop_signal = WSTOPSIG(status);
  } else {
    trap->trap = XR_TRACE_TRAP_SYSCALL;
    if (get_syscall_info(trap) == false) {
      return false;
    }
  }

  if (get_resource_info(trap) == false) {
    return false;
  }

  return true;
}

bool xr_ptrace_tracer_get(xr_tracer_t *tracer, int pid, void *address,
                          char *buffer, size_t size) {
  return false;
}

bool xr_ptrace_tracer_set(xr_tracer_t *tracer, int pid, void *address,
                          const char *buffer, size_t size) {
  return false;
}
