#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#include "lib/utils.h"
#include "tracer/option.h"
#include "tracer/tracer.h"
#include "tracer/tracers/ptrace/tracer.h"

static inline bool __do_set_stdio(int stdio, int fd, const char *path,
                                  int flag) {
  int fd_flag = fcntl(fd, F_GETFD);
  if (errno != 0) {
    fd_flag = flag;
  }
  /* try dup fd to stdio */
  if (!dup3(fd, stdio, fd_flag)) {
    return true;
  }
  /* try open at path */
  int fd_new = open(path, flag);
  if (fd_new == -1 || dup2(fd_new, stdio) == -1) {
    /* open failed or dup failed */
    return false;
  }
  return true;
}

static inline void do_exec(xr_entry_t *entry) {
  ptrace(PTRACE_TRACEME, 0, NULL, NULL);

  if (__do_set_stdio(0, entry->stdin_fd, entry->stdin->string, O_RDONLY) ==
      false) {
    return false;
  }
  if (__do_set_stdio(1, entry->stdin_fd, entry->stdin->string,
                     O_CREAT | O_TRUNC | O_RDONLY) == false) {
    return false;
  }
  if (__do_set_stdio(2, entry->stdin_fd, entry->stdin->string,
                     O_CREAT | O_TRUNC | O_RDONLY) == false) {
    return false;
  }
}

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
  return rlimit_memory(option.limit_per_process.memory) &&
         rlimit_cpu_time(option.limit_per_process.sys_time);
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
    do_exec(tracer->option);
  } else {
    if (wait_first_child(tracer, fork_ret)) {
      return xr_tracer_error(
          tracer, _XR_ADD_FUNC("waiting first child %d failed."), fork_ret);
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
    return xr_tracer_error(tracer, _XR_ADD_FUNC("waiting child failed."));
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
    return xr_tracer_error(
        tracer, _XR_ADD_FUNC("unhandled process/thread %d found.\n"), pid);
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
      return xr_tracer_error(
          tracer,
          _XR_ADD_FUNC(
              "getting system call infomation of process %d failed.\n"),
          trap->process->pid);
    }
    __flip_thread_syscall_status(trap->thread);
  }

  if (get_resource_info(trap) == false) {
    return xr_tracer_error(
        tracer,
        _XR_ADD_FUNC("getting resource information of process %d failed.\n"),
        trap->process->pid);
  }

  return true;
}

const static unsigned long __xr_address_mask = sizeof(long) - 1;
const static unsigned long __xr_address_align = -sizeof(long);

bool xr_ptrace_tracer_get(xr_tracer_t *tracer, int pid, void *address,
                          void *buffer, size_t size) {
  // aligned address in kernel
  long addr = (long)address & __xr_address_align;
  // offset of needed data
  size_t offset = addr - (long)address;
  long data_ = 0;
  void *data = (void *)&data_;
  while (true) {
    data_ = ptrace(PTRACE_PEEKDATA, pid, addr, NULL);
    if (errno) {
      return xr_tracer_error(
          tracer,
          _XR_ADD_FUNC("ptrace_tracer peeking child %d data at %p failed.\n"),
          pid, addr);
    }
    size_t need = min(sizeof(long) - offset, size);
    memcpy(buffer, *data + offset, need);
    addr += sizeof(long);
    buffer += sizeof(long);
    size -= need;
    offset = 0;
  }
  return true;
}

bool xr_ptrace_tracer_set(xr_tracer_t *tracer, int pid, void *address,
                          const void *buffer, size_t size) {
  // aligned address in kernel
  long addr = (long)address & __xr_address_align;
  // offset of needed data
  size_t offset = addr - (long)address;
  // rest part of data
  size_t rest = size;
  long data_ = 0;
  void *data = (void *)&data_;
  while (true) {
    size_t need = min(sizeof(long) - offset, rest);
    if (need < sizeof(long)) {
      data = ptrace(PTRACE_PEEKDATA, pid, addr, NULL);
    }
    memcpy((void *)(addr + offset), buffer, need);

    ptrace(PTRACE_POKEDATA, pid, addr, NULL);

    addr += sizeof(long);
    buffer += sizeof(long);
    rest -= need;
    offset = 0;
  }
  return true;
}

bool xr_ptrace_strcpy(xr_tracer_t *tracer, int pid, void *address,
                      size_t max_size, xr_string_t *str) {
  str->length = 0;
  // aligned address in kernel
  long addr = (long)address & __xr_address_align;
  // offset of needed data
  size_t offset = addr - (long)address;
  long data_ = 0;
  char *data = (char *)&data_;
  while (true) {
    data_ = ptrace(PTRACE_PEEKDATA, pid, addr, NULL);
    if (errno) {
      return xr_tracer_error(
          tracer,
          _XR_ADD_FUNC("ptrace_tracer peeking child %d data at %p failed.\n"),
          pid, addr);
    }
    size_t need = sizeof(long) - offset;
    void *term = memchr(data + offset, '\0', need);
    if (term != NULL) {
      need = term - data - offset;
    }
    xr_string_concat_raw(str, data + offset, need);

    if (term != NULL) {
      return true;
    }

    addr += sizeof(long);
    offset = 0;
    if (str->capacity - str->length <= sizeof(long)) {
      xr_string_grow(str, str->capacity * 3 / 2);
    }
  }
  return true;
}
