#include <stdbool.h>
#include <stdlib.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "xrun/calls.h"
#include "xrun/option.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/tracer.h"
#include "xrun/utils/utils.h"

static inline bool __do_set_stdio(int stdio, int fd, int flag) {
  int fd_flag = fcntl(fd, F_GETFD);
  if (errno != 0) {
    fd_flag = flag;
  }
  fd_flag &= ~O_CLOEXEC;
  /* try dup fd to stdio */
  return dup2(fd, stdio) != -1 && fcntl(stdio, F_SETFD, fd_flag) != 1;
}

static inline bool do_exec(xr_tracer_t *tracer, xr_entry_t *entry) {
  ptrace(PTRACE_TRACEME, 0, NULL, NULL);
  const int flag = O_CREAT | O_TRUNC | O_WRONLY;
  for (int i = 0; i < 3; ++i) {
    if (__do_set_stdio(i, entry->stdio[i], i & flag) == false) {
      _XR_TRACER_ERROR(tracer, "dup file %d error.", i);
      return false;
    }
  }
}

static inline bool rlimit_cpu_time(xr_time_t time) {
  struct rlimit rlimit = {.rlim_cur = time.user_time / 1000,
                          .rlim_max = time.user_time / 1000 + 1};
  return setrlimit(RLIMIT_CPU, &rlimit) != 0;
}

static inline bool rlimit_memory(int memory) {
  struct rlimit address = {.rlim_cur = memory * 2, .rlim_max = memory * 2 + 1};
  struct rlimit data = {.rlim_cur = memory, .rlim_max = memory + 1};
  return setrlimit(RLIMIT_AS, &address) != 0 &&
         setrlimit(RLIMIT_DATA, &data) != 0;
}

static inline bool do_rlimit_setup(xr_option_t *option) {
  return rlimit_memory(option->limit_per_process.memory) &&
         rlimit_cpu_time(option->limit_per_process.time);
}

static bool get_syscall_info(xr_trace_trap_t *trap) {
  return false;
}

static bool get_resource_info(xr_trace_trap_t *trap) {
  struct rusage resource;
  if (getrusage(trap->thread->tid, &resource) == -1) {
    return false;
  }
  trap->process->time =
    xr_time_from_timeval(resource.ru_stime, resource.ru_utime);
  trap->process->memory = resource.ru_maxrss;
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

  thread->compat = xr_ptrace_tracer_syscall_compat(child);
  thread->tid = child;
  thread->syscall_status = XR_THREAD_CALLOUT;
  // Current state should be XR_THREAD_CALLIN, Since child process will be
  // trapped when returning from execve. But this syscall should not be
  // reported. Hence syscall_status should be XR_THREAD_CALLOUT and we will skip
  // the first execve syscall by a wait operation before.

  xr_process_add_thread(process, thread);
  return thread;
}

#define __XR_PTRACE_TRACER_PIPE_ERR 256

struct __xr_ptrace_tracer_pipe_info {
  int eno;
  char msg[__XR_PTRACE_TRACER_PIPE_ERR];
};

#define xr_close_pipe(pipe) \
  do {                      \
    close(pipe[0]);         \
    close(pipe[1]);         \
  } while (0)

bool xr_ptrace_tracer_spawn(xr_tracer_t *tracer) {
  // open pipe for delivering error
  int error_pipe[2] = {};
  if (pipe(error_pipe) == -1 ||
      fcntl(error_pipe[0], F_SETFD, O_CLOEXEC) == -1 ||
      fcntl(error_pipe[1], F_SETFD, O_CLOEXEC) != -1) {
    return _XR_TRACER_ERROR(tracer, "ptrace_tracer popen error pipe failed.");
  }

  // do fork here
  pid_t fork_ret = vfork();

  // fork error
  if (fork_ret < 0) {
    return _XR_TRACER_ERROR(tracer, "ptrace_tracer vfork failed.");
  }

  // in child process
  if (fork_ret == 0) {
    do_exec(tracer, NULL);

    // exec failed. die here
    // send tracer->error into pipe
    struct __xr_ptrace_tracer_pipe_info error = {
      .eno = tracer->error->eno,
    };
    strncpy(error.msg, tracer->error->msg.string, __XR_PTRACE_TRACER_PIPE_ERR);
    write(error_pipe[1], &error, sizeof(error));
    _exit(1);
  }

  // try to read error info
  struct __xr_ptrace_tracer_pipe_info error = {};
  read(error_pipe[0], &error, sizeof(error));
  // close all pipe
  xr_close_pipe(error_pipe);

  int status = 0;
  int child = waitpid(fork_ret, &status, 0);

  // error occurred.
  if (WIFEXITED(status)) {
    xr_tracer_error(tracer, error.msg);
    errno = error.eno;
    return _XR_TRACER_ERROR(tracer, "waiting first child %d failed.", fork_ret);
  }

  // set options here
  if (ptrace(PTRACE_SETOPTIONS, fork_ret, NULL,
             PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE | PTRACE_O_TRACEVFORK |
               PTRACE_O_TRACEFORK) == 0) {
    return _XR_TRACER_ERROR(
      tracer, "ptrace tracer PTRACE_SETOPTIONS for process %d failed.",
      fork_ret);
  }

  if (ptrace(PTRACE_SYSCALL, fork_ret, NULL, NULL)) {
    return _XR_TRACER_ERROR(tracer, "ptrace tracer PTRACE_SYSCALL failed.");
  }

  xr_thread_t *thread = create_spawned_process(tracer, fork_ret);
  return true;
}

bool xr_ptrace_tracer_step(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  return ptrace(PTRACE_SYSCALL, trap->thread->tid, NULL, NULL);
}

// since thread->syscall_status is 0 or 1, fliping it by xor
#define __flip_thread_syscall_status(thread) \
  do {                                       \
    (thread)->syscall_status ^= 1;           \
  } while (0)

/*
 * the implementation is in arch/ptrace_*.c depending on XR_ARCH_* marco.
 */
extern bool xr_ptrace_tracer_peek_syscall(int pid,
                                          xr_trace_trap_syscall_t *syscall_info,
                                          int compat);
extern int xr_ptrace_tracer_syscall_compat(int pid);

// we expire execve and execveat
// using stub marco to prevent populate
#ifndef XR_SYSCALL_EXECVE
#define XR_SYSCALL_EXECVE__STUB -1
#else
#define XR_SYSCALL_EXECVE__STUB XR_SYSCALL_EXECVE
#endif
#ifndef XR_SYSCALL_EXECVEAT
#define XR_SYSCALL_EXECVEAT__STUB -1
#else
#define XR_SYSCALL_EXECVEAT__STUB XR_SYSCALL_EXECVEAT
#endif

bool xr_ptrace_tracer_trap(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  struct rusage ru;
  int status;
  pid_t pid = wait(&status);
  if (pid == -1) {
    return _XR_TRACER_ERROR(tracer, "waiting child failed.");
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
    return _XR_TRACER_ERROR(tracer, "unhandled process/thread %d found.", pid);
  }

  if (WIFEXITED(status)) {
    trap->trap = XR_TRACE_TRAP_EXIT;
    trap->exit_code = WEXITSTATUS(status);
  } else if (WSTOPSIG(status) != SIGTRAP) {
    trap->trap = XR_TRACE_TRAP_SIGNAL;
    trap->stop_signal = WSTOPSIG(status);
  } else {
    trap->trap = XR_TRACE_TRAP_SYSCALL;
    if (xr_ptrace_tracer_peek_syscall(pid, &trap->syscall_info,
                                      thread->compat) == false) {
      return _XR_TRACER_ERROR(
        tracer, "getting system call infomation of process %d failed.",
        trap->process->pid);
    }

    if (trap->syscall_info.syscall == XR_SYSCALL_EXECVE__STUB ||
        trap->syscall_info.syscall == XR_SYSCALL_EXECVEAT__STUB) {
      if (xr_ptrace_tracer_syscall_compat(pid) == -1) {
        return _XR_TRACER_ERROR(tracer, "getting system compat mode failed");
      }
    }
    __flip_thread_syscall_status(trap->thread);
  }

  if (get_resource_info(trap) == false) {
    return _XR_TRACER_ERROR(
      tracer, "getting resource information of process %d failed.",
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
      return _XR_TRACER_ERROR(
        tracer, "ptrace_tracer peeking child %d data at %p failed.", pid, addr);
    }
    size_t need = XR_MIN(sizeof(long) - offset, size);
    memcpy(buffer, data + offset, need);
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
    size_t need = XR_MIN(sizeof(long) - offset, rest);
    if (need < sizeof(long)) {
      data_ = ptrace(PTRACE_PEEKDATA, pid, addr, NULL);
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
                      xr_string_t *str) {
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
      return _XR_TRACER_ERROR(
        tracer, "ptrace_tracer peeking child %d data at %p failed.", pid, addr);
    }
    size_t need = sizeof(long) - offset;
    void *term = memchr(data + offset, '\0', need);
    if (term != NULL) {
      need = term - (void *)data - offset;
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
