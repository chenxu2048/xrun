#ifndef XR_ENTRY_H
#define XR_ENTRY_H

#include "xrun/utils/path.h"

struct xr_entry_s;
typedef struct xr_entry_s xr_entry_t;
struct xr_entry_s {
  xr_path_t path, pwd, root;
  int user;
  int stdio[3];
  char **argv;
  char **environs;
};

static inline void xr_entry_set_argv(xr_entry_t *entry, int argc,
                                     char *argv[]) {
  entry->argv = (char **)realloc(entry->argv, sizeof(char *) * (argc + 1));
  memcpy(entry->argv, argv, sizeof(char *) * argc);
  entry->argv[argc] = NULL;
}

static inline void xr_entry_set_environ(xr_entry_t *entry, char **env) {
  int nenv = 0;
  while (env[nenv] != NULL) {
    nenv++;
  }
  entry->environs = (char **)realloc(entry->environs, sizeof(char *) * nenv);
  memcpy(entry->environs, env, sizeof(char *) * nenv);
}

static inline void xr_entry_init(xr_entry_t *entry) {
  xr_string_zero(&entry->path);
  xr_string_zero(&entry->pwd);
  xr_string_zero(&entry->root);
  entry->argv = NULL;
  entry->environs = NULL;
}

static inline void xr_entry_delete(xr_entry_t *entry) {
  // if (entry->argv != NULL) {
  //   free(entry->argv);
  // }
  // if (entry->environs != NULL) {
  //   free(entry->environs);
  // }
  xr_path_delete(&entry->path);
  xr_path_delete(&entry->pwd);
  xr_path_delete(&entry->root);
}

void xr_entry_execve(xr_entry_t *entry);

#endif
