#ifndef XRN_FILE_LIMIT_H
#define XRN_FILE_LIMIT_H

#include <stdbool.h>

#include <xrun/option.h>

bool xrn_file_limit_read_flags(char *flags_name, long *flags);

bool xrn_file_access_read(xr_access_list_t *alist, char *path);

#endif
