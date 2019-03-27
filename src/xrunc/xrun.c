#include <stdio.h>

#include <getopt.h>
#include <unistd.h>

#include "xrun/entry.h"
#include "xrun/result.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/tracer.h"

#include "xrunc/config.h"
#include "xrunc/file_limit.h"
#include "xrunc/option.h"

extern char **environ;

#define XRN_GLOBAL_OPTION_SLOT_SIZE 128

typedef struct xrn_global_config_set_s xrn_global_config_set_t;
typedef struct xrn_global_config_access_s xrn_global_access_config_t;
struct xrn_global_config_access_s {
  size_t length, capacity;
  char **access;
};

struct xrn_global_config_set_s {
  char *config_path;
  bool version, help;
  xr_option_t option;
  xr_entry_t entry;
  xrn_global_access_config_t faccess, daccess;
  xr_string_t error;
};

void xrn_global_option_access_init(xrn_global_access_config_t *access) {
  access->length = 0;
  access->access = malloc(sizeof(char *) * XRN_GLOBAL_OPTION_SLOT_SIZE);
  access->capacity = XRN_GLOBAL_OPTION_SLOT_SIZE;
}

void xrn_global_option_access_delete(xrn_global_access_config_t *access) {
  free(access->access);
}

void xrn_global_option_access_append(xrn_global_access_config_t *access,
                                     char *entry) {
  if (access->capacity <= access->length) {
    access->capacity *= 2;
    access->access = realloc(access->access, access->capacity);
  }
  access->access[access->length] = entry;
  access->length++;
}

void xrn_global_option_set_init(xrn_global_config_set_t *cfg) {
  cfg->config_path = NULL;
  cfg->version = false;
  cfg->help = false;
  xr_string_init(&cfg->error, XRN_GLOBAL_OPTION_SLOT_SIZE);
  xrn_global_option_access_init(&cfg->faccess);
  xrn_global_option_access_init(&cfg->daccess);

  xr_option_t *xropt = &cfg->option;
  memset(xropt, 0, sizeof(xr_option_t));

  xr_entry_t *entry = &cfg->entry;
  xr_string_zero(&entry->path);
  xr_string_zero(&entry->pwd);
  xr_string_zero(&entry->root);
  entry->argv = NULL;
}

void xrn_global_option_set_delete(xrn_global_config_set_t *cfg) {
  xr_string_delete(&cfg->error);
  xrn_global_option_access_delete(&cfg->faccess);
  xrn_global_option_access_delete(&cfg->daccess);

  xr_option_t *xropt = &cfg->option;
  if (xropt->file_access) {
    for (int i = 0; i < xropt->n_file_access; ++i) {
      xr_file_limit_delete(xropt->file_access + i);
    }
    free(xropt->file_access);
    xropt->n_file_access = 0;
  }
  if (xropt->dir_access) {
    for (int i = 0; i < xropt->n_dir_access; ++i) {
      xr_file_limit_delete(xropt->dir_access + i);
    }
    free(xropt->dir_access);
    xropt->n_dir_access = 0;
  }

  xr_entry_t *xrentry = &cfg->entry;
  xr_path_delete(&xrentry->path);
  xr_path_delete(&xrentry->pwd);
  xr_path_delete(&xrentry->root);
  if (xrentry->argv != NULL) {
    free(xrentry->argv);
  }
}

bool xrn_set_help(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  cfg->help = true;
  return true;
}

bool xrn_set_version(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  cfg->version = true;
  return true;
}

bool xrn_set_config_path(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  if (cfg->config_path != NULL) {
    xr_string_format(&cfg->error, "option --config-path must be unique.\n");
    return false;
  }
  cfg->config_path = arg;
  return true;
}

bool xrn_set_call(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  char *endptr = NULL;
  long call = strtol(arg, &endptr, 10);
  if (*endptr != '\0' || call <= 0 || call >= XR_SYSCALL_MAX) {
    xr_string_format(
      &cfg->error,
      "--call must be a valid number between 0 to %d instead of \"%s\".\n",
      XR_SYSCALL_MAX, arg);
    return false;
  }
  cfg->option.call_access[call] = true;
  return true;
}

bool xrn_set_file_entry(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  xrn_global_option_access_append(&cfg->faccess, arg);
  return true;
}

bool xrn_set_dir_entry(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  xrn_global_option_access_append(&cfg->daccess, arg);
  return true;
}

bool xrn_set_memory(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  char *endptr = NULL;
  long memory = strtol(arg, &endptr, 10);
  if (*endptr != '\0' || memory <= 0) {
    xr_string_format(&cfg->error,
                     "--memory must be a valid number which is greater than 0 "
                     "instead of \"%s\".\n",
                     arg);
    return false;
  }
  cfg->option.limit.memory = memory;
  cfg->option.limit_per_process.memory = memory;
  return true;
}

bool xrn_set_time(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  char *endptr = NULL;
  long t = strtol(arg, &endptr, 10);
  if (*endptr != '\0' || t <= 0) {
    xr_string_format(&cfg->error,
                     "--time must be a valid number which is greater than 0 "
                     "instead of \"%s\".\n",
                     arg);
    return false;
  }
  cfg->option.limit.time.sys_time = t;
  cfg->option.limit_per_process.time.sys_time = t;
  cfg->option.limit.time.user_time = t;
  cfg->option.limit_per_process.time.user_time = t;
  return true;
}

bool xrn_set_nfile(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  char *endptr = NULL;
  long nfile = strtol(arg, &endptr, 10);
  if (*endptr != '\0' || nfile <= 3) {
    xr_string_format(&cfg->error,
                     "--nfile must be a valid number which is greater than 3 "
                     "instead of \"%s\".\n",
                     arg);
    return false;
  }
  cfg->option.limit.nfile = nfile;
  cfg->option.limit_per_process.nfile = nfile;
  return true;
}

bool xrn_set_process(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  if (arg == NULL) {
    cfg->option.nprocess = 1;
    return true;
  }
  char *endptr = NULL;
  long nprocess = strtol(arg, &endptr, 10);
  if (*endptr != '\0' || nprocess <= 1) {
    xr_string_format(&cfg->error,
                     "--fork must be a valid number which is greater than 1 "
                     "instead of \"%s\".\n",
                     arg);
    return false;
  }
  cfg->option.nprocess = nprocess;
  return true;
}

bool xrn_set_thread(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  char *endptr = NULL;
  long nthread = strtol(arg, &endptr, 10);
  if (*endptr != '\0' || nthread <= 1) {
    xr_string_format(&cfg->error,
                     "--thread must be a valid number which is greater than 1 "
                     "instead of \"%s\".\n",
                     arg);
    return false;
  }
  cfg->option.limit.nthread = nthread;
  cfg->option.limit_per_process.nthread = nthread;
  return true;
}

xrn_option_t options[] = {
  {
    {"config", required_argument, NULL, 'c'},
    "Tracee configuration in json.",
    NULL,
    "config_path",
    xrn_set_config_path,
  },
  {
    {"call", required_argument, NULL, 'C'},
    "Permitted syscall number which can be name or number",
    NULL,
    "N|name",
    xrn_set_call,
  },
  {
    {"directory", required_argument, NULL, 'd'},
    "Directory can be accessed. Note that colon(:) in path should be escaped "
    "as \\:. (-) at the begin of flags means a containing mode, and flags can "
    "combine using (+)",
    NULL,
    "path:[!]flags",
    xrn_set_dir_entry,
  },
  {
    {"file", required_argument, NULL, 'f'},
    "Same as --directory, but for files.",
    NULL,
    NULL,
    xrn_set_file_entry,
  },
  {
    {"help", no_argument, NULL, 'h'},
    "Help information",
    NULL,
    NULL,
    xrn_set_help,
  },
  {
    {"memory", required_argument, NULL, 'm'},
    "Memory limitation in byte.",
    NULL,
    "memory",
    xrn_set_memory,
  },
  {
    {"nfile", required_argument, NULL, 'n'},
    "Opened files limitation.",
    NULL,
    "N",
    xrn_set_nfile,
  },
  {
    {"process", required_argument, NULL, 'p'},
    "Enable fork and set process number limitation.",
    "1",
    "N",
    xrn_set_process,
  },
  {
    {"time", required_argument, NULL, 't'},
    "Time limitation in millisecond.",
    NULL,
    "N",
    xrn_set_time,
  },
  {
    {"thread", required_argument, NULL, 'T'},
    "Thread number limitation.",
    NULL,
    "N",
    xrn_set_thread,
  },
  {
    {"version", no_argument, NULL, 'v'},
    "version information of xrun.",
    NULL,
    NULL,
    xrn_set_version,
  },
  {
    {NULL, 0, NULL, 0},
    NULL,
    NULL,
    NULL,
  },
};

static inline void xrn_print_error(xr_string_t *error) {
  if (error->string[error->length - 1] != '\n') {
    xr_string_concat_raw(error, "\n", 2);
  }
  fputs(error->string, stderr);
}

static inline bool xrn_add_file_limit(char **pathes, xr_file_limit_t **limit,
                                      size_t *nlimit, size_t length,
                                      xr_string_t *error) {
  size_t n = *nlimit;
  *nlimit += length;
  *limit = realloc(*limit, sizeof(xr_file_limit_t) * (*nlimit));
  memset((*limit) + n, 0, sizeof(xr_file_limit_t) * length);
  if (xrn_file_limit_read_all(pathes, length, (*limit) + n, error) == false) {
    xrn_print_error(error);
    return false;
  }
}

int main(int argc, char *argv[]) {
  int retval = 0;
  xrn_global_config_set_t cfg;
  xrn_global_option_set_init(&cfg);
  bool parse = xrn_parse_options(argc, argv, options, &cfg.error, &cfg);
  if (cfg.help || parse == false) {
    if (parse == false) {
      xrn_print_error(&cfg.error);
      retval = 1;
    }
    xrn_print_options(options);
    goto xrn_parse_option_error;
  }
  if (cfg.version) {
    // xrn_print_version();
    goto xrn_parse_option_error;
  }

  if (optind >= argc) {
    retval = 1;
    xrn_print_options(options);
    goto xrn_parse_option_error;
  }
  char *prog = argv[optind];
  xr_string_concat_raw(&cfg.entry.path, prog, strlen(prog));
  int entry_argc = argc - optind;
  cfg.entry.argv = malloc(sizeof(char *) * (entry_argc + 1));
  for (int i = 0; i < entry_argc; ++i) {
    cfg.entry.argv[i] = argv[optind + i];
  }
  cfg.entry.argv[entry_argc] = NULL;

  cfg.entry.environs = environ;

  if (cfg.config_path != NULL &&
      xrn_config_parse(cfg.config_path, &cfg.option, &cfg.error) == false) {
    xrn_print_error(&cfg.error);
    retval = 1;
    goto xrn_parse_option_error;
  }

  if (cfg.faccess.length != 0) {
    if (xrn_add_file_limit(cfg.faccess.access, &cfg.option.file_access,
                           &cfg.option.n_file_access, cfg.faccess.length,
                           &cfg.error) == false) {
      goto xrn_parse_option_error;
    }
  }

  if (cfg.daccess.length != 0) {
    if (xrn_add_file_limit(cfg.daccess.access, &cfg.option.dir_access,
                           &cfg.option.n_dir_access, cfg.daccess.length,
                           &cfg.error) == false) {
      goto xrn_parse_option_error;
    }
  }

  xr_string_copy(&cfg.entry.root, &xr_path_slash);
  for (int i = 0; i < 3; ++i) {
    cfg.entry.stdio[i] = i;
  }

  xr_tracer_t tracer;
  xr_tracer_result_t result;
  xr_tracer_ptrace_init(&tracer, "xrunc_tracer");
  if (xr_tracer_setup(&tracer, &cfg.option) == false) {
    xrn_print_error(&tracer.error.msg);
    retval = 1;
    goto xrn_tracer_failed;
  }
  bool res = xr_tracer_trace(&tracer, &cfg.entry, &result);
  xrn_print_error(&tracer.error.msg);
xrn_tracer_failed:
  xr_tracer_delete(&tracer);
xrn_parse_option_error:
  xrn_global_option_set_delete(&cfg);
  return retval;
}
