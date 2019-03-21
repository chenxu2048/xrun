#ifndef XRN_OPTION_H
#define XRN_OPTION_H

#include <getopt.h>

struct xrn_option {
  struct option opt;
  const char *descption;
  const char *def_val;
  const char *format;
};

struct option *xrn_make_option(struct xrn_option *opts);

void xrn_print_options(struct xrn_option *opts);

void xrn_print_option(struct xrn_option *opt);

char *xrn_make_short_option(struct xrn_option *opts);

#endif
