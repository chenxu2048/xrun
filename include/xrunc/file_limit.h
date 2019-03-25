#ifndef XRN_FILE_LIMIT_H
#define XRN_FILE_LIMIT_H

#include <stdbool.h>

#include <xrun/option.h>

int xrn_find_flag(char *flag, size_t len);

bool xrn_read_file_limit(const char *arg, xr_file_limit_t *flimit);

bool xrn_parse_file_limit_flag(const char *flags, xr_file_limit_t *flimit);

#endif
