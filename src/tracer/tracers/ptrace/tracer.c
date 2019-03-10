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
  trap->process.time =
      xr_time_from_timeval(resource.ru_stime, resource.ru_utime);
  trap->process.memory = resource.ru_maxrss;
  return true;
}

static xr_thread_t *create_spawned_process(xr_tracer_t *tracer, pid_t child) {
  xr_process_t *process = _XR_NEW(xr_process_t);
  xr_thread_t *thread = _XR_NEW(xr_thread_t);
  process->pid = child;
  process->nthread = 0;

  xr_list_init(&(process->threads));
  xr_list_add(&(tracer->processes), &(process->processes));
  tracer->nprocess++;
  tracer->nthread++;

  thread->tid = child;
  thread->syscall_status = XR_THREAD_CALLOUT;
  // Current state should be XR_THREAD_CALLIN, Since child process will be
  // trapped when returning from execve. But this syscall should not be
  // reported. Hence syscall_status should be XR_THREAD_CALLOUT and we will skip
  // the first execve syscall by a wait operation before.

  xr_process_add_thread(process, thread);

  return thread;
}

bool xr_ptrace_tracer_spawn(xr_tracer_t *tracer) {
  pid_t fork_ret = vfork();
  if (fork_ret < 0) {
    return false;
  } else if (fork_ret == 0) {
    // TODO: error handle using pipe.
    do_rlimit_setup(tracer->option);
    do_exec(tracer->option);
  } else {
    if (wait_first_child(tracer, fork_ret)) {
      return false;
    }
    create_spawned_process(tracer, fork_ret);
  }
}

bool xr_ptrace_tracer_step(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  return ptrace(PTRACE_SYSCALL, trap->thread->tid, NULL, NULL);
}

// since thread->syscall_status is 0 or 1, fliping it by xor
#define __flip_thread_syscall_status(thread) \
  do {                                       \
    (thread)->syscall_status ^= 1;           \
  } while (0)

bool xr_ptrace_tracer_trap(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  struct rusage ru;
  int status;
  pid_t pid = wait(&status);
  if (pid == -1) {
    return false;
  }

  // trap stopped thread
  xr_thread_t *thread;
  xr_process_t *process;
  trap->process = NULL;
  _xr_list_for_each_entry(&(tracer->processes), process, xr_process_t,
                          processes) {
    _xr_list_for_each_entry(&(process->threads), thread, xr_thread_t, threads) {
      if (thread->tid == pid) {
        trap->thread = thread;
        trap->process = process;
        break;
      }
    }
    if (trap->process) {
      break;
    }
  }

  if (trap->process == NULL) {
    return false;
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
    __flip_thread_syscall_status(trap->thread);
  }

  if (get_resource_info(trap) == false) {
    return false;
  }

  return true;
}

bool xr_ptrace_tracer_get(xr_tracer_t *tracer, int pid, void *address,
                          void *buffer, size_t size) {
  const size_t chunk = for (int i = 0; i < size / s - 1; ++i) {
    if (ptrace(PTRACE_PEEKDATA, )) }
}

bool xr_ptrace_tracer_set(xr_tracer_t *tracer, int pid, void *address,
                          const void *buffer, size_t size) {
  return false;
}
