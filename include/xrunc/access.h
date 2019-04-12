#ifndef XRN_ACCESS_H
#define XRN_ACCESS_H

#include <stdbool.h>

#include <xrun/option.h>

bool xrn_access_read_flags(char *flags_name, long *flags);

bool xrn_access_read(xr_access_list_t *alist, char *path);

#endif
