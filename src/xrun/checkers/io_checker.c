#include "xrun/checkers/io_checker.h"
#include "xrun/calls.h"
#include "xrun/files.h"
#include "xrun/option.h"
#include "xrun/tracer.h"

struct __xr_io_checker_private_s {
  xr_file_t *file;
  xr_thread_t *thread;
};
typedef struct __xr_io_checker_private_s __xr_io_checker_private_t;

static inline __xr_io_checker_private_t *xr_io_checker_private(
  xr_checker_t *checker) {
  return (__xr_io_checker_private_t *)(checker->checker_data);
}

static inline bool xr_io_checker_failed(xr_checker_t *checker, xr_file_t *file,
                                        xr_thread_t *thread) {
  xr_io_checker_private(checker)->file = file;
  xr_io_checker_private(checker)->thread = thread;
  return false;
}

bool xr_io_checker_setup(xr_checker_t *checker, xr_tracer_t *tracer) {
  return true;
}

static inline bool __do_process_read_check(xr_checker_t *checker,
                                           xr_option_t *option,
                                           xr_thread_t *thread, int fd,
                                           long nread) {
  xr_file_set_t *fset = &thread->fset;
  xr_file_t *file = xr_file_set_select_file(fset, fd);
  if (file != NULL) {
    file->read_length += nread;
  }
  xr_file_set_read(fset, nread);
  if (xr_file_set_get_read(fset) > option->limit_per_process.nread) {
    return xr_io_checker_failed(checker, file, thread);
  }
}
static inline bool __do_process_write_check(xr_checker_t *checker,
                                            xr_option_t *option,
                                            xr_thread_t *thread, int fd,
                                            long nwrite) {
  xr_file_set_t *fset = &thread->fset;
  xr_file_t *file = xr_file_set_select_file(fset, fd);
  if (file != NULL) {
    file->read_length += nwrite;
  }
  xr_file_set_write(fset, nwrite);
  if (xr_file_set_get_write(fset) > option->limit_per_process.nwrite) {
    return xr_io_checker_failed(checker, file, thread);
  }
}

bool xr_io_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                         xr_trace_trap_t *trap) {
  if (trap->trap != XR_TRACE_TRAP_SYSCALL) {
    return true;
  }
  xr_thread_t *thread = trap->thread;
  long scno = trap->syscall_info.syscall;
  long io = trap->syscall_info.retval;
  long fd = trap->syscall_info.args[0];
  switch (scno) {
    case XR_SYSCALL_READ:
// readv
#ifdef XR_SYSCALL_READV
    case XR_SYSCALL_READV:
#endif
// pread series
#ifdef XR_SYSCALL_PREAD64
    case XR_SYSCALL_PREAD64:
#endif
#ifdef XR_SYSCALL_PREADV
    case XR_SYSCALL_PREADV:
#endif
#ifdef XR_SYSCALL_PREADV2
    case XR_SYSCALL_PREADV2:
#endif
#ifdef XR_SYSCALL_PREADV64
    case XR_SYSCALL_PREADV64:
#endif
// socket read
#ifdef XR_SYSCALL_RECV
    case XR_SYSCALL_RECV:
#endif
#ifdef XR_SYSCALL_RECVFROM
    case XR_SYSCALL_RECVFROM:
#endif
#ifdef XR_SYSCALL_RECVMSG
    case XR_SYSCALL_RECVMSG:
#endif
#ifdef XR_SYSCALL_RECVMMSG
    case XR_SYSCALL_RECVMMSG:
#endif
    {
      return __do_process_read_check(checker, tracer->option, thread, fd, io);
    }
    case XR_SYSCALL_WRITE:
#ifdef XR_SYSCALL_WRITEV
    case XR_SYSCALL_WRITEV:
#endif
// pwrite series
#ifdef XR_SYSCALL_PWRITE64
    case XR_SYSCALL_PWRITE64:
#endif
#ifdef XR_SYSCALL_PWRITEV
    case XR_SYSCALL_PWRITEV:
#endif
#ifdef XR_SYSCALL_PWRITEV2
    case XR_SYSCALL_PWRITEV2:
#endif
#ifdef XR_SYSCALL_PWRITEV64
    case XR_SYSCALL_PWRITEV64:
#endif
// socket write
#ifdef XR_SYSCALL_SEND
    case XR_SYSCALL_SEND:
#endif
#ifdef XR_SYSCALL_SENDTO
    case XR_SYSCALL_SENDTO:
#endif
#ifdef XR_SYSCALL_SENDMSG
    case XR_SYSCALL_SENDMSG:
#endif
#ifdef XR_SYSCALL_SENDMMSG
    case XR_SYSCALL_SENDMMSG:
#endif
    {
      return __do_process_write_check(checker, tracer->option, thread, fd, io);
    }
    default:
      break;
  }
  return true;
}

void xr_io_checker_result(xr_checker_t *checker, xr_tracer_t *tracer,
                          xr_tracer_result_t *result, xr_trace_trap_t *trap) {
  return;
}

void xr_io_checker_delete(xr_checker_t *checker) {
  free(checker->checker_data);
  return;
}

void xr_io_checker_init(xr_checker_t *checker) {
  checker->setup = xr_io_checker_setup;
  checker->check = xr_io_checker_check;
  checker->result = xr_io_checker_result;
  checker->_delete = xr_io_checker_delete;
  checker->checker_id = XR_CHECKER_IO;
  checker->checker_data = _XR_NEW(__xr_io_checker_private_t);
}
