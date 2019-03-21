#ifndef XRN_CONFIG_H
#define XRN_CONFIG_H

#include "xrun/option.h"

bool xrn_config_parse(const char *config_path, xr_option_t *option,
                      xr_string_t *error);

#endif
