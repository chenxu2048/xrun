#include <stdio.h>
#include <stdlib.h>

#include "xrunc/option.h"

struct option *xrn_make_option(xrn_option_t *opts) {
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

void xrn_print_options(xrn_option_t *opts) {
  int opt_idx = 0;
  while (opts[opt_idx].opt.name != NULL) {
    xrn_print_option(opts + opt_idx);
    opt_idx++;
  }
}

void xrn_print_option(xrn_option_t *opt) {
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

char *xrn_make_short_option(xrn_option_t *opt) {
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

bool xrn_parse_options(int argc, char *argv[], xrn_option_t *option,
                       xr_string_t *error, void *ctx) {
  int option_index = -1;
  opterr = 0;
  char *sopt = xrn_make_short_option(option);
  size_t sopt_len = strlen(sopt);
  struct option *gopt = xrn_make_option(option);
  while (true) {
    option_index = -1;
    int opt = getopt_long(argc, argv, sopt, gopt, &option_index);
    switch (opt) {
      case -1:
        return true;
      case '?':
        xr_string_format(error, "Invalid option.\n", opt);
        return false;
      default: {
        if (option_index == -1) {
          int i = 0;
          while (option[i].opt.name != NULL) {
            if (option[i].opt.val == opt) {
              option_index = i;
              break;
            }
            i++;
          }
          if (option_index == -1) {
            xr_string_format(error, "Option -%c is unrecongized.\n", opt);
            return false;
          }
        }
        xrn_option_f *set = option[option_index].set;
        if (set == NULL) {
          xr_string_format(error, "Option --%s with NULL handle function.\n",
                           option[option_index].opt.name);
          return false;
        }
        if (set(optarg, ctx) == false) {
          return false;
        }
      }
    }
  }
  return true;
}
