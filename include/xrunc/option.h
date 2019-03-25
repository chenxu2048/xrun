#ifndef XRN_OPTION_H
#define XRN_OPTION_H

#include <stdbool.h>

#include <getopt.h>

#include "xrun/utils/string.h"

typedef bool(xrn_option_f)(char *, void *ctx);

typedef struct xrn_option_s xrn_option_t;
struct xrn_option_s {
  struct option opt;
  const char *descption;
  const char *def_val;
  const char *format;
  xrn_option_f *set;
};

struct option *xrn_make_option(xrn_option_t *opts);

void xrn_print_options(xrn_option_t *opts);

void xrn_print_option(xrn_option_t *opt);

char *xrn_make_short_option(xrn_option_t *opts);

bool xrn_parse_options(int argc, char *argv[], xrn_option_t *option,
                       xr_string_t *error, void *ctx);

#endif
