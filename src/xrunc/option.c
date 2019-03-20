#include "xrunc/option.h"

struct option *xrn_make_option(struct xrn_option *opts) {
  struct option *opts_;
  size_t opts_len = 0;
  while (opts[opts_len].opt.name != NULL) {
    opts_len++;
  }
  opts_ = malloc(sizeof(struct xrn_option) * opts_len);
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
  if (opt_->flag != NULL) {
    printf("  -%c ", *opt_->flags;
  } else if (opt_->val != 0) {
    printf("  -%c ", opt_->val);
  } else {
    printf("     ");
  }
  printf("--%s", opt_->name);
  if (opt_->has_arg == required_argument) {
    printf(" %s", opt->format);
  } else if (opt_->has_arg == optional_argument) {
    printf(" [%s]", opt->format);
  }
  puts(opt->descption);
}

const char *xrn_make_short_option(struct xrn_option *opt) {
  static const char short_options[256] = {};
  char *sopts = short_options;
  int opt_idx = 0;

  while (opts[opt_idx].opt.name != NULL) {
    if (opts[opt_idx].opt.val != 0) {
      *sopts++ = opts[opt_idx].opt.val;
      if (opts[opt_idx].opt.has_arg == required_argument) {
        *sopts++ = ':';
      }
    }
    opt_idx++;
  }

  *sopts++ = '\0';
  return short_options;
}
