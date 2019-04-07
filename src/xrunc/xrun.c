#include <stdio.h>

#include <getopt.h>
#include <unistd.h>

#include "xrun/checkers.h"

#include "xrun/entry.h"
#include "xrun/result.h"
#include "xrun/tracer.h"
#include "xrun/tracers/ptrace/tracer.h"

#include "xrunc/config.h"
#include "xrunc/file_limit.h"
#include "xrunc/option.h"

extern char **environ;

#define XRN_GLOBAL_OPTION_SLOT_SIZE 128

struct xrn_global_config_set_s {
  char *config_path;
  bool version, help;
  xr_option_t option;
  xr_entry_t entry;
  xr_string_t error;
};
typedef struct xrn_global_config_set_s xrn_global_config_set_t;

void xrn_global_option_set_init(xrn_global_config_set_t *cfg) {
  cfg->config_path = NULL;
  cfg->version = false;
  cfg->help = false;
  xr_string_zero(&cfg->error);

  xr_option_t *xropt = &cfg->option;
  xr_option_init(xropt);

  xr_entry_t *entry = &cfg->entry;
  xr_entry_init(entry);
}

void xrn_global_option_set_delete(xrn_global_config_set_t *cfg) {
  xr_string_delete(&cfg->error);
  xr_option_delete(&cfg->option);
  xr_entry_delete(&cfg->entry);
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
  cfg->option.calls[call] = true;
  return true;
}

bool xrn_set_file_entry(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  xrn_file_access_read(&cfg->option.files, arg);
  return true;
}

bool xrn_set_dir_entry(char *arg, void *ctx) {
  xrn_global_config_set_t *cfg = (xrn_global_config_set_t *)ctx;
  xrn_file_access_read(&cfg->option.directories, arg);
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
  static xr_string_t newline = {
    .length = 1,
    .string = "\n",
    .capacity = 2,
  };
  if (xr_string_end_with(error, &newline) == false) {
    xr_string_concat(error, &newline);
  }
  fputs(error->string, stderr);
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

  cfg.option.access_trigger = XR_ACCESS_TRIGGER_MODE_IN;

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

  xr_string_copy(&cfg.entry.root, &xr_path_slash);
  for (int i = 0; i < 3; ++i) {
    cfg.entry.stdio[i] = i;
  }

  xr_tracer_t tracer;
  xr_tracer_result_t result;
  xr_tracer_ptrace_init(&tracer, "xrunc_tracer");

  xr_checker_id_t checkers[5] = {XR_CHECKER_FILE, XR_CHECKER_RESOURCE,
                                 XR_CHECKER_IO, XR_CHECKER_FORK,
                                 XR_CHECKER_FILE};
  for (int i = 0; i < 5; ++i) {
    if (xr_tracer_add_checker(&tracer, checkers[i]) == false) {
      goto xrn_tracer_failed;
    }
  }

  if (xr_tracer_setup(&tracer, &cfg.option) == false) {
    xr_error_tostring(&tracer.error, &cfg.error);
    xrn_print_error(&cfg.error);
    retval = 1;
    goto xrn_tracer_failed;
  }
  bool res = xr_tracer_trace(&tracer, &cfg.entry, &result);
  if (res == false) {
    xr_error_tostring(&tracer.error, &cfg.error);
    xrn_print_error(&cfg.error);
  }
xrn_tracer_failed:
  xr_tracer_delete(&tracer);
xrn_parse_option_error:
  xrn_global_option_set_delete(&cfg);
  return retval;
}
