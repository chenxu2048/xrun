#ifndef XRN_FILE_LIMIT_H
#define XRN_FILE_LIMIT_H

#include <stdbool.h>

#include <xrun/option.h>

bool xrn_file_limit_read(char *arg, xr_file_limit_t *flimit);

bool xrn_file_limit_read_flags(char *flags, xr_file_limit_t *flimit);

bool xrn_file_limit_read_all(char **pathes, size_t size,
                             xr_file_limit_t *flimit, xr_string_t *error);

#endif
