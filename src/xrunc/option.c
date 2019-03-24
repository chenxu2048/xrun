#include <stdio.h>
#include <stdlib.h>

#include "xrunc/option.h"

struct option *xrn_make_option(struct xrn_option *opts) {
  static struct option opts_[256];
  size_t opts_len = 0;
  while (opts[opts_len].opt.name != NULL) {
    opts_len++;
  }
  for (int i = 0; i < opts_len; ++i) {
    opts_[i] = opts[i].opt;
  }
  return opts_;
}

void xrn_print_options(struct xrn_option *opts) {
  int opt_idx = 0;
  while (opts[opt_idx].opt.name != NULL) {
    xrn_print_option(opts + opt_idx);
    opt_idx++;
  }
}

void xrn_print_option(struct xrn_option *opt) {
  struct option *opt_ = &opt->opt;
  int putlen = 0;
  if (opt_->flag != NULL) {
    putlen += printf("  -%c, ", *opt_->flag);
  } else if (opt_->val != 0) {
    putlen += printf("  -%c, ", opt_->val);
  } else {
    putlen += printf("      ");
  }
  putlen += printf("--%s", opt_->name);
  if (opt_->has_arg == required_argument) {
    putlen += printf(" %s ", opt->format);
  } else if (opt_->has_arg == optional_argument) {
    putlen += printf(" [%s] ", opt->format);
  }
  if (putlen > 28) {
    printf("\n%*c", 30, ' ');
  } else {
    printf("%*c", 30 - putlen, ' ');
  }
  puts(opt->descption);
}

char *xrn_make_short_option(struct xrn_option *opt) {
  static char short_options[256] = {};
  char *sopts = short_options;
  int opt_idx = 0;

  while (opt[opt_idx].opt.name != NULL) {
    if (opt[opt_idx].opt.val != 0) {
      *sopts++ = opt[opt_idx].opt.val;
      if (opt[opt_idx].opt.has_arg == required_argument) {
        *sopts++ = ':';
      }
    }
    opt_idx++;
  }

  *sopts++ = '\0';
  return short_options;
}
