#define _GNU_SOURCE

#include <stdbool.h>
#include <stdlib.h>

#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "xrun/calls.h"
#include "xrun/entry.h"
#include "xrun/option.h"
#include "xrun/process.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/tracer.h"
#include "xrun/utils/utils.h"

/*
 * the implementation is in arch/ptrace_*.c depending on XR_ARCH_* marco.
 */
extern bool xr_ptrace_tracer_peek_syscall(int pid,
                                          xr_trace_trap_syscall_t *syscall_info,
                                          int compat);

extern bool xr_ptrace_tracer_poke_syscall(int pid, long args, int index,
                                          int compat);

extern int xr_ptrace_tracer_syscall_compat(int pid);

extern int xr_ptrace_tracer_elf_compat(xr_path_t *elf);

#define XR_TRACER_PTRACE_PENDING_CLONE_DEFAULT 4

struct xr_tracer_ptrace_pending_clone_s;
typedef struct xr_tracer_ptrace_pending_clone_s
  xr_tracer_ptrace_pending_clone_t;
struct xr_tracer_ptrace_pending_clone_s {
  pid_t caller;
  int hit;
  long hack_val;
  xr_tracer_ptrace_pending_clone_t *next;
};

struct xr_tracer_ptrace_data_s;
typedef struct xr_tracer_ptrace_data_s xr_tracer_ptrace_data_t;
struct xr_tracer_ptrace_data_s {
  xr_tracer_ptrace_pending_clone_t *pending;
};

static inline void xr_tracer_ptrace_data_delete(xr_tracer_ptrace_data_t *data) {
  xr_tracer_ptrace_pending_clone_t *pending;
  while (data->pending) {
    pending = data->pending;
    data->pending = pending->next;
    free(pending);
  }
}

static inline xr_tracer_ptrace_data_t *xr_tracer_ptrace_data(
  xr_tracer_t *tracer) {
  return (xr_tracer_ptrace_data_t *)tracer->tracer_data;
}

void xr_tracer_ptrace_init(xr_tracer_t *tracer, const char *name) {
  xr_tracer_init(tracer, name);
  tracer->spwan = xr_ptrace_tracer_spawn;
  tracer->step = xr_ptrace_tracer_step;
  tracer->trap = xr_ptrace_tracer_trap;
  tracer->get = xr_ptrace_tracer_get;
  tracer->set = xr_ptrace_tracer_set;
  tracer->strcpy = xr_ptrace_tracer_strcpy;
  tracer->kill = xr_ptrace_tracer_kill;
  tracer->_delete = xr_ptrace_tracer_delete;
  tracer->clean = xr_ptrace_tracer_clean;
  tracer->tracer_data = _XR_NEW(xr_tracer_ptrace_data_t);
  memset(tracer->tracer_data, 0, sizeof(xr_tracer_ptrace_data_t));
}

void xr_ptrace_tracer_clean(xr_tracer_t *tracer) {
  xr_tracer_ptrace_data_delete(xr_tracer_ptrace_data(tracer));
}

void xr_ptrace_tracer_delete(xr_tracer_t *tracer) {
  xr_ptrace_tracer_clean(tracer);
  free(tracer->tracer_data);
}

static inline bool xr_ptrace_tracer_setopt(int pid) {
  return ptrace(PTRACE_SETOPTIONS, pid, NULL,
                PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE |
                  PTRACE_O_TRACEVFORK | PTRACE_O_TRACEFORK) == -1;
}

// we try to set close on exec for any other file description
static inline void xr_ptrace_try_cloexec() {
  struct rlimit limit;
  if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
    for (int i = 3; i < limit.rlim_cur; ++i) {
      fcntl(i, F_SETFD, FD_CLOEXEC);
    }
  }
}

static inline void do_exec(xr_tracer_t *tracer, xr_entry_t *entry) {
  if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
    _XR_TRACER_ERROR(tracer, "PTRACE_TRACEME error.");
    return;
  }
  if (prctl(PR_SET_PDEATHSIG, SIGKILL) == -1) {
    _XR_TRACER_ERROR(tracer, "prctl PR_SET_PDEATHSIG failed.");
    return;
  }
  for (int i = 0; i < 3; ++i) {
    if (dup2(i, entry->stdio[i]) == -1) {
      _XR_TRACER_ERROR(tracer, "dup file %d error.", i);
      return;
    }
  }
  xr_ptrace_try_cloexec();
  xr_entry_execve(entry);
  _XR_TRACER_ERROR(tracer, "execvpe error.");
}

static inline bool rlimit_cpu_time(xr_time_t time) {
  struct rlimit rlimit = {.rlim_cur = time.user_time / 1000,
                          .rlim_max = time.user_time / 1000 + 1};
  return setrlimit(RLIMIT_CPU, &rlimit) == 0;
}

static inline bool rlimit_memory(int memory) {
  struct rlimit address = {.rlim_cur = memory * 2, .rlim_max = memory * 2 + 1};
  struct rlimit data = {.rlim_cur = memory, .rlim_max = memory + 1};
  return setrlimit(RLIMIT_AS, &address) == 0 &&
         setrlimit(RLIMIT_DATA, &data) == 0;
}

static inline bool do_rlimit_setup(xr_option_t *option) {
  return rlimit_memory(option->limit_per_process.memory) &&
         rlimit_cpu_time(option->limit_per_process.time);
}

static bool get_resource_info(xr_trace_trap_t *trap, struct rusage *ru) {
  trap->thread->process->time =
    xr_time_from_timeval(ru->ru_stime, ru->ru_utime);
  trap->thread->process->memory = ru->ru_maxrss;
  return true;
}

static xr_thread_t *create_spawned_process(xr_tracer_t *tracer, pid_t child,
                                           xr_path_t *pwd) {
  xr_process_t *process = _XR_NEW(xr_process_t);
  xr_thread_t *thread = _XR_NEW(xr_thread_t);
  process->pid = child;
  process->nthread = 0;
  process->compat = XR_COMPAT_SYSCALL_INVALID;

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
  xr_file_set_create(&thread->fset);
  xr_fs_create(&thread->fs);
  xr_string_copy(xr_fs_pwd(&thread->fs), pwd);

  xr_process_add_thread(process, thread);
  return thread;
}

#define __XR_PTRACE_TRACER_PIPE_ERR 256

#define xr_close_pipe(pipe) \
  do {                      \
    close(pipe[0]);         \
    close(pipe[1]);         \
  } while (0)

bool xr_ptrace_tracer_spawn(xr_tracer_t *tracer, xr_entry_t *entry) {
  // open pipe for delivering error
  int error_pipe[2] = {};
  if (pipe2(error_pipe, O_NONBLOCK | O_CLOEXEC) == -1) {
    return _XR_TRACER_ERROR(tracer, "ptrace_tracer popen error pipe failed.");
  }

  // do fork here
  pid_t fork_ret = fork();

  // fork error
  if (fork_ret < 0) {
    return _XR_TRACER_ERROR(tracer, "ptrace_tracer fork failed.");
  }

  // in child process
  if (fork_ret == 0) {
    do_exec(tracer, entry);

    // exec failed. die here
    // send tracer->error into pipe
    xr_string_t estr;
    xr_string_zero(&estr);
    xr_error_tostring(&tracer->error, &estr);
    write(error_pipe[1], &estr.length, sizeof(estr.length));
    write(error_pipe[1], estr.string, sizeof(char) * estr.length);
    _exit(1);
  }

  int status = 0;
  int child = waitpid(fork_ret, &status, 0);

  // try to read error info
  size_t estr_len;
  read(error_pipe[0], &estr_len, sizeof(estr_len));
  if (errno == 0) {
    xr_string_t estr;
    // read rest
    xr_string_init(&estr, estr_len + 1);
    read(error_pipe[0], estr.string, sizeof(char) * estr_len);
    estr.string[estr_len] = 0;
    estr.length = estr_len;
    // clear errno
    errno = 0;
    xr_tracer_error(tracer, estr.string);
    xr_string_delete(&estr);
  }
  // close all pipe
  xr_close_pipe(error_pipe);

  if (child != fork_ret) {
    return _XR_TRACER_ERROR(tracer, "unexpected child %d created.", child);
  }

  // error occurred.
  if (WIFEXITED(status)) {
    return _XR_TRACER_ERROR(tracer, "waiting first child %d failed.", fork_ret);
  }

  // set options here
  if (xr_ptrace_tracer_setopt(fork_ret) == false) {
    return _XR_TRACER_ERROR(
      tracer, "ptrace tracer PTRACE_SETOPTIONS for process %d failed.",
      fork_ret);
  }

  xr_thread_t *thread = create_spawned_process(tracer, fork_ret, &entry->pwd);
  if (thread == NULL) {
    return _XR_TRACER_ERROR(tracer, "ptrace create a process error.");
  }
  xr_string_copy(xr_fs_pwd(&thread->fs), &entry->pwd);
  if (ptrace(PTRACE_SYSCALL, fork_ret, NULL, NULL) == -1) {
    return _XR_TRACER_ERROR(tracer, "ptrace tracer PTRACE_SYSCALL failed.");
  }

  return true;
}

// we expire execve and execveat
// using stub marco to prevent populate
#ifndef XR_SYSCALL_EXECVE
#define XR_SYSCALL_EXECVE -1
#endif
#ifndef XR_SYSCALL_EXECVEAT
#define XR_SYSCALL_EXECVEAT -1
#endif

#define XR_CLONE_UNUSED_ARG 5
#define XR_CLONE2_UNUSED_ARG 1

#ifndef XR_SYSCALL_CLONE
#define XR_SYSCALL_CLONE -1
#endif
#ifndef XR_SYSCALL_FORK
#define XR_SYSCALL_FORK -1
#endif
#ifndef XR_SYSCALL_VFORK
#define XR_SYSCALL_VFORK -1
#endif

#define XR_IS_CLONE(syscall)                                        \
  ((syscall) == XR_SYSCALL_CLONE || (syscall) == XR_SYSCALL_FORK || \
   (syscall) == XR_SYSCALL_VFORK)

static inline bool xr_ptrace_tracer_cloning(xr_tracer_t *tracer,
                                            xr_trace_trap_t *trap) {
  xr_tracer_ptrace_data_t *data = xr_tracer_ptrace_data(tracer);
  xr_tracer_ptrace_pending_clone_t *pending =
    _XR_NEW(xr_tracer_ptrace_pending_clone_t);
  pending->next = data->pending;
  data->pending = pending;
  pending->caller = trap->thread->tid;
  pending->hit = 0;
  pending->hack_val = trap->syscall_info.args[XR_CLONE_UNUSED_ARG];
  // hacking here
  return xr_ptrace_tracer_poke_syscall(pending->caller, pending->caller,
                                       XR_CLONE_UNUSED_ARG,
                                       trap->thread->process->compat);
}

static inline bool xr_ptrace_tracer_clone_return(xr_tracer_t *tracer,
                                                 xr_trace_trap_t *trap) {
  xr_tracer_ptrace_data_t *data = xr_tracer_ptrace_data(tracer);
  for (xr_tracer_ptrace_pending_clone_t *pending = data->pending, *prev = NULL;
       pending != NULL; prev = pending, pending = prev->next) {
    if (pending->caller == trap->syscall_info.args[XR_CLONE_UNUSED_ARG]) {
      pending->hit++;
      // recover to trap
      trap->syscall_info.args[XR_CLONE_UNUSED_ARG] = pending->hack_val;
      // hit twice or clone failed
      // pending release
      bool release = pending->hit == 2 ||
                     (trap->thread->syscall_status == XR_THREAD_CALLOUT &&
                      trap->syscall_info.retval < 0);
      if (release) {
        if (prev == NULL) {
          data->pending = pending->next;
        } else {
          prev->next = pending->next;
        }
        free(pending);
      }
      // recover to tracee
      return xr_ptrace_tracer_poke_syscall(
        trap->thread->tid, trap->syscall_info.args[XR_CLONE_UNUSED_ARG],
        XR_CLONE_UNUSED_ARG, trap->thread->process->compat);
    }
  }
  return false;
}

bool xr_ptrace_tracer_step(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  return ptrace(PTRACE_SYSCALL, trap->thread->tid, NULL, NULL) == 0;
}

// since thread->syscall_status is 0 or 1, fliping it by xor
#define __flip_thread_syscall_status(thread) \
  do {                                       \
    (thread)->syscall_status ^= 1;           \
  } while (0)

static inline xr_thread_t *xr_tracer_select_thread(xr_tracer_t *tracer,
                                                   pid_t pid) {
  // trap stopped thread
  xr_thread_t *thread;
  xr_process_t *process;
  _xr_list_for_each_entry(&(tracer->processes), process, xr_process_t,
                          processes) {
    _xr_list_for_each_entry(&(process->threads), thread, xr_thread_t, threads) {
      if (thread->tid == pid) {
        return thread;
      }
    }
  }
  return NULL;
}

bool xr_ptrace_tracer_trap(xr_tracer_t *tracer, xr_trace_trap_t *trap) {
  struct rusage ru;
  int status;
  pid_t pid = wait3(&status, 0, &ru);
  if (pid == -1) {
    return _XR_TRACER_ERROR(tracer, "waiting child failed.");
  }

  trap->thread = xr_tracer_select_thread(tracer, pid);

  if (WIFEXITED(status)) {
    trap->trap = XR_TRACE_TRAP_EXIT;
    trap->exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    trap->trap = XR_TRACE_TRAP_SIGNAL;
    trap->stop_signal = WSTOPSIG(status);
  } else if (WIFSTOPPED(status)) {
    trap->trap = XR_TRACE_TRAP_SYSCALL;
    __flip_thread_syscall_status(trap->thread);

    int compat = XR_COMPAT_SYSCALL_INVALID;
    if (trap->thread == NULL) {
      // assume cloning
      if (xr_ptrace_tracer_setopt(pid) == false) {
        return false;
      }
      trap->thread = _XR_NEW(xr_thread_t);
      xr_thread_init(trap->thread);
      trap->thread->tid = pid;
      trap->thread->process = NULL;
    } else {
      compat = trap->thread->process->compat;
    }

    if (compat == XR_COMPAT_SYSCALL_INVALID) {
      compat = xr_ptrace_tracer_syscall_compat(pid);
      if (compat == XR_COMPAT_SYSCALL_INVALID) {
        return _XR_TRACER_ERROR(tracer, "dectect system compat mode failed");
      }
      if (trap->thread->process != NULL) {
        trap->thread->process->compat = compat;
      }
    }

    if (xr_ptrace_tracer_peek_syscall(pid, &trap->syscall_info, compat) ==
        false) {
      return _XR_TRACER_ERROR(
        tracer, "getting system call infomation of process %d failed.",
        trap->thread->process->pid);
    }

    if (trap->syscall_info.syscall == XR_SYSCALL_EXECVE ||
        trap->syscall_info.syscall == XR_SYSCALL_EXECVEAT) {
      // we will detect syscall compat mode in next syscall
      trap->thread->process->compat = XR_COMPAT_SYSCALL_INVALID;
    } else if (XR_IS_CLONE(trap->syscall_info.syscall)) {
      if (trap->thread->syscall_status == XR_THREAD_CALLIN &&
          xr_ptrace_tracer_cloning(tracer, trap) == false) {
        // hack on cloning failed
        return _XR_TRACER_ERROR(tracer,
                                "could not hack syscall argument while "
                                "clone/fork/vfork at process/thread %d.",
                                pid);
      } else if (trap->thread->syscall_status == XR_THREAD_CALLOUT) {
        if (trap->syscall_info.retval == 0 && trap->thread->process == NULL) {
          xr_thread_t *caller = xr_tracer_select_thread(
            tracer, trap->syscall_info.args[XR_CLONE_UNUSED_ARG]);
          if (caller == NULL) {
            return _XR_TRACER_ERROR(tracer,
                                    "could not retrieve caller thread while "
                                    "clone/fork/vfork at process/thread %d.",
                                    pid);
          } else {
            xr_process_add_thread(caller->process, trap->thread);
            trap->syscall_info.clone_caller = caller;
          }
        }
        if (xr_ptrace_tracer_clone_return(tracer, trap) == false) {
          return _XR_TRACER_ERROR(
            tracer,
            "could not recover hacked syscall argument while clone/fork/vfork "
            "at process/thread %d.",
            pid);
        }
      }
    }
  }

  if (trap->thread->process == NULL) {
    free(trap->thread);
    return _XR_TRACER_ERROR(tracer, "untraced process/thread %d occured.", pid);
  }

  if (get_resource_info(trap, &ru) == false) {
    return _XR_TRACER_ERROR(
      tracer, "getting resource information of process %d failed.",
      trap->thread->process->pid);
  }

  return true;
}

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
    memcpy((void *)(data + offset), buffer, need);

    ptrace(PTRACE_POKEDATA, pid, addr, NULL);

    addr += sizeof(long);
    buffer += sizeof(long);
    rest -= need;
    offset = 0;
  }
  return true;
}

bool xr_ptrace_tracer_strcpy(xr_tracer_t *tracer, int pid, void *address,
                             xr_string_t *str) {
  str->length = 0;
  // aligned address in kernel
  long addr = (long)address & __xr_address_align;
  // offset of needed data
  size_t offset = (long)address - addr;
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

void xr_ptrace_tracer_kill(xr_tracer_t *tracer, pid_t pid) {
  ptrace(PTRACE_KILL, pid, 0, 0);
}
