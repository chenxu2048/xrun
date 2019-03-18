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

static int nthread;
static int nprocess;
static int memory;
static int nfile;
static xr_time_t time;

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

void xr_xrun_parse_opt(int argc, const char *argv[]) {
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
          fputs("--config is not unique.\n", stderr);
        }
        config_path = optarg;
        if (xrn_config_parse(config_path, option, error) == false) {
          retval = 1;
          return;
        }
        break;
      }
      case 'C':
      case 'd':
      case 'f':
      case 'F':
      case 'm':
      case 'n':
      case 't':
      case 'T':
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
  }
}

int main(int argc, const char *argv[]) {
  xr_xrun_parse_opt(argc, argv, &error);
}
