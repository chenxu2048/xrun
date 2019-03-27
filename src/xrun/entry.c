#define _GNU_SOURCE

#include <unistd.h>

#include "xrun/entry.h"

void xr_entry_execve(xr_entry_t *entry) {
  xr_path_abs(&entry->root);
  if (entry->root.length != 0 &&
      xr_string_equal(&entry->root, &xr_path_slash) == false) {
    if (chroot(entry->root.string) == -1) {
      return;
    }
  }
  if (entry->pwd.length != 0) {
    if (chdir(entry->pwd.string) == -1) {
      return;
    }
  }
  if (entry->environs == NULL) {
    execvp(entry->path.string, entry->argv);
  } else {
    execvpe(entry->path.string, entry->argv, entry->environs);
  }
}
