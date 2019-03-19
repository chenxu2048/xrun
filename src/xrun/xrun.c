#include <stdbool.h>
#include <stdio.h>

#include <getopt.h>
#include <unistd.h>

#include "tracer/tracer.h"

#include "xrun/config.h"
#include "xrun/file_limit.h"
#include "xrun/option.h"

static char *config_path = NULL;
static bool version = false, help = false;
static int retval = 0;

static xr_option_t xrn_option = {};
static xr_entry_t xrn_entry = {};

#define _XRN_ACCESS_ENTRY 2048
#define _XRN_CAN_ADD_ENTRY(entry) (entry < _XRN_ACCESS_ENTRY)

static char *dir_access[_XRN_ACCESS_ENTRY] = {};
static size_t n_dir_access = 0;
static char *file_access[_XRN_ACCESS_ENTRY] = {};
static size_t n_file_access = 0;

#define XRN_OPTION_ERROR(msg, ...)                                   \
  do {                                                               \
    char errbuf[256];                                                \
    errbuf[0] = '\0';                                                \
    size_t errsize =                                                 \
      snprintf(errbuf, 256, "%s: " msg "\n", __func__, __VA_ARGS__); \
    xr_string_concat_raw(&xrn_error, errbuf, errsize);               \
  } while (0)

static xr_string_t xrn_error;

static long nthread;
static long nprocess;
static long memory;
static long nfile;
static xr_time_ms_t time;

#define XR_FILE_LIMIT_SIZE 128

const static struct xrn_option options[] = {
  {
    {"config", requried_argument, NULL, 'c'},
    "Tracee configuration in json.",
    NULL,
    "config_path",
    NULL,
  },
  {
    {"call", requried_argument, NULL, 'C'},
    "Permitted syscall number which can be name or number",
    NULL,
    "N|name",
    NULL,
  },
  {
    {"directory", requried_argument, NULL, 'd'},
    "Directory can be accessed. Note that colon(:) in path should be "
    "escaped "
    "as \\:",
    NULL,
    "path:[+]flags",
    NULL,
  },
  {
    {"file", requried_argument, NULL, 'f'},
    "Same as --directory, but for files.",
    NULL,
    NULL,
    NULL,
  },
  {
    {"fork", optionnal_argument, NULL, 'F'},
    "Enable fork and set process number limitation.",
    "1",
    "N",
    NULL,
  },
  {
    {"help", no_argument, NULL, 'h'},
    "Help information",
    NULL,
    NULL,
    NULL,
  },
  {
    {"memory", requried_argument, NULL, 'm'},
    "Memory limitation in byte.",
    NULL,
    "memory",
    NULL,
  },
  {
    {"nfile", required_argument, NULL, 'n'},
    "Opened files limitation.",
    NULL,
    "N",
    NULL,
  },
  {
    {"time", required_argument, NULL, 't'},
    "Time limitation in millisecond." NULL,
    "N",
    NULL,
  },
  {
    {"thread", required_argument, NULL, 'T'},
    "Thread number limitation.",
    NULL,
    "N",
    NULL,
  },
  {
    {"version", no_argument, NULL, 'v'},
    "version information of xrun.",
    NULL,
    NULL,
    NULL,
  },
  {
    {NULL, 0, NULL, 0},
    NULL,
    NULL,
    NULL,
    NULL,
  },
};

#define _XRN_SET_GLOBAL_INT(var, val, option)                     \
  do {                                                            \
    char *endptr = NULL;                                          \
    var = strtol(val, &endptr, 10);                               \
    if (*endptr != '\0') {                                        \
      retval = 1;                                                 \
      XRN_OPTION_ERROR("invalid option %s with %s", option, val); \
    }                                                             \
  } while (0)

void xrn_parse_opt(int argc, const char *argv[]) {
  int option_index = 0;
  opterr = 0;
  xr_string_t error;
  xr_string_init(&error, 256);
  while (1) {
    int opt = getopt_long(argc, argv, option_short, options, &option_index);
    switch (opt) {
      case -1:
        return;
      case 'c': {
        if (config_path != NULL) {
          retval = 1;
          XRN_OPTION_ERROR("--config is not unique.\n");
        }
        config_path = optarg;
        break;
      }
      case 'C':
      case 'd': {
        if (_XRN_CAN_ADD_ENTRY(n_dir_access)) {
          dir_access[n_dir_access++] = optarg;
        } else {
          retval = 1;
          XRN_OPTION_ERROR(
            "--directory/-d too many entries. using config file instead.");
        }
        break;
      }
      case 'f': {
        if (_XRN_CAN_ADD_ENTRY(n_file_access)) {
          file_access[n_file_access++] = optarg;
        } else {
          retval = 1;
          XRN_OPTION_ERROR(
            "--directory/-d too many entries. using config file instead.");
        }
        break;
      }
      case 'F':
        _XRN_SET_GLOBAL_INT(nprocess, optarg, "--fork/-F");
        break;
      case 'm':
        _XRN_SET_GLOBAL_INT(memory, optarg, "--memory/-m");
        break;
      case 'n':
        _XRN_SET_GLOBAL_INT(nfile, optarg, "--nfile/-n");
        break;
      case 't':
        _XRN_SET_GLOBAL_INT(time, optarg, "--time/-t");
        break;
      case 'T':
        _XRN_SET_GLOBAL_INT(nthread, optarg, "--thread/-T");
        break;
      case 'v':
        version = true;
        break;
      case '?':
        retval = 1;
      case 'h':
        help = true;
        break;
      default:
        break;
    }
    if (retval == 1) {
      return;
    }
  }
}

int main(int argc, const char *argv[]) {
  xrn_parse_opt(argc, argv);
  if (help == true) {
    xrn_print_options(&options);
  }
  if (retval == 1) {
    fputs(error.string, stderr);
  } else {
  }
}
