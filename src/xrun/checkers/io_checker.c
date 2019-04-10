#include "xrun/checkers/io_checker.h"
#include "xrun/calls.h"
#include "xrun/files.h"
#include "xrun/option.h"
#include "xrun/process.h"
#include "xrun/tracer.h"

struct xr_io_checker_data_s {
  xr_path_t path;
  enum xr_io_checker_result_e {
    XR_IO_CHECKER_RESULT_WRITE,
    XR_IO_CHECKER_RESULT_READ,
  } result;
  long long length;
};
typedef struct xr_io_checker_data_s xr_io_checker_data_t;

static inline xr_io_checker_data_t *xr_io_checker_data(xr_checker_t *checker) {
  return (xr_io_checker_data_t *)(checker->checker_data);
}

bool xr_io_checker_setup(xr_checker_t *checker, xr_option_t *option) {
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
  long long total_read = xr_file_set_get_read(fset);
  if (total_read > option->limit_per_process.nread) {
    xr_io_checker_data_t *data = xr_io_checker_data(checker);
    xr_string_copy(&data->path, &file->path);
    data->length = total_read;
    data->result = XR_IO_CHECKER_RESULT_WRITE;
    return false;
  }
  return true;
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
  long long total_write = xr_file_set_get_write(fset);
  if (total_write > option->limit_per_process.nwrite) {
    xr_io_checker_data_t *data = xr_io_checker_data(checker);
    xr_string_copy(&data->path, &file->path);
    data->length = total_write;
    data->result = XR_IO_CHECKER_RESULT_WRITE;
    return false;
  }
  return true;
}

bool xr_io_checker_check(xr_checker_t *checker, xr_tracer_t *tracer,
                         xr_trace_trap_t *trap) {
  if (trap->trap != XR_TRACE_TRAP_SYSCALL ||
      trap->thread->syscall_status != XR_THREAD_CALLOUT) {
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
                          xr_tracer_result_t *result) {
  xr_io_checker_data_t *data = xr_io_checker_data(checker);
  switch (data->result) {
    case XR_IO_CHECKER_RESULT_READ:
      result->status = XR_RESULT_READOUT;
      break;
    case XR_IO_CHECKER_RESULT_WRITE:
      result->status = XR_RESULT_WRITEOUT;
      break;
    default:
      result->status = XR_RESULT_UNKNOWN;
      break;
  }
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
  checker->checker_data = _XR_NEW(xr_io_checker_data_t);
  xr_string_zero(&xr_io_checker_data(checker)->path);
}
