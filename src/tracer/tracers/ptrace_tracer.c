#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include "lib/utils.h"
#include "tracer/option.h"
#include "tracer/tracer.h"
#include "tracer/tracers/ptrace_tracer.h"

static bool rlimit_cpu_time(xr_time_t time) {
  struct rlimit rlimit = {.rlim_cur = time / 1000, .rlim_max = time / 1000 + 1};
  return setrlimit(RLIMIT_CPU, &rlimit) != 0;
}

static bool rlimit_memory(int memory) {
  struct rlimit address = {.rlim_cur = memory * 2, .rlim_max = memory * 2 + 1};
  struct rlimit data = {.rlim_cur = memory, .rlim_max = memory + 1};
  return setrlimit(RLIMIT_AS, &address) != 0 &&
         setrlimit(RLIMIT_DATA, &data) != 0;
}

static bool do_rlimit_setup(xr_option_t *option) {
  return rlimit_memory(option.process.memory) &&
         rlimit_cpu_time(option.process.sys_time);
}

static bool do_ptrace_syscall(int pid) {
  return ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
}

static xr_process_t *create_spawned_process(pid_t child) {
  xr_process_t *process = _XR_NEW(xr_process_t);
  xr_thread_t *thread = _XR_NEW(xr_thread_t);
  process->pid = child;
  thread->tid = child;
  xr_list_add(&(process->threads), &(thread->threads));
  return process;
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
    xr_process_t *process = create_spawned_process(fork_ret);
    xr_list_add(&(tracer->processes), &(process->processes));
  }
}

bool xr_ptrace_tracer_step(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  return do_ptrace_syscall(trap->thread->tid);
}

bool xr_ptrace_tracer_trap(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  struct rusage ru;
  int status;
  pid_t pid = wait(&status);
  if (WIFEXITED(status)) {
    trap->trap = XR_TRACE_TRAP_EXIT;
  } else if (WSTOPSIG(status) != SIGTRAP) {
    trap->trap = XR_TRACE_TRAP_SIGNAL;
  } else {
    trap->trap = XR_TRACE_TRAP_SYSCALL;
  }
}

bool xr_ptrace_tracer_get(xr_tracer_t *tracer, int pid, void *address,
                          char *buffer, size_t size);

bool xr_ptrace_tracer_set(xr_tracer_t *tracer, int pid, void *address,
                          const char *buffer, size_t size);
