#ifndef XRN_FILE_LIMIT_H
#define XRN_FILE_LIMIT_H

#include <tracer/option.h>

bool xrn_read_file_limit(char *arg, xr_file_limit_t *flimit);

bool xrn_parse_file_limit_flag(const char *flags, xr_file_limit_t *flimit);

#endif