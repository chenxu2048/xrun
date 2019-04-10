#include <stdlib.h>

#include "xrun/process.h"

void xr_thread_delete(xr_thread_t *thread) {
  xr_file_set_delete(&thread->fset);
  xr_fs_delete(&thread->fs);
}

void xr_process_delete(xr_process_t *process) {
  xr_list_t *cur, *temp;
  _xr_list_for_each_safe(&process->threads, cur, temp) {
    xr_thread_t *thread = xr_list_entry(cur, xr_thread_t, threads);
    xr_list_del(cur);
    xr_thread_delete(thread);
    free(thread);
  }
}
