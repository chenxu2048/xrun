#include "xrun/option.h"

#define __do_file_flags_check_contain(access, fflags) \
  ((access)->mode == XR_ACCESS_MODE_FLAG_CONTAINS &&  \
   (access)->flags == (fflags | (access)->flags))

#define __do_file_flags_check_match(access, fflags) \
  ((access)->mode == XR_ACCESS_MODE_FLAG_MATCH && (access)->flags == fflags)

// fflags is the subset of flags in bits in contain mode.
// or fflags match the flags in match mode.
#define __do_file_flags_check(access, fflags)       \
  (__do_file_flags_check_contain(access, fflags) || \
   __do_file_flags_check_match(access, fflags))

void xr_access_list_append(xr_access_list_t *alist, const char *path,
                           size_t length, long flags, xr_access_mode_t mode) {
  if (alist->capacity == alist->nentry) {
    if (alist->capacity == 0) {
      alist->capacity = 1;
    } else {
      alist->capacity *= 2;
    }
    alist->entries =
      realloc(alist->entries, sizeof(xr_access_entry_t) * alist->capacity);
  }
  xr_string_zero(&alist->entries[alist->nentry].path);
  xr_string_concat_raw(&alist->entries[alist->nentry].path, path, length);
  alist->entries[alist->nentry].flags = flags;
  alist->entries[alist->nentry].mode = mode;
  alist->nentry++;
}
bool xr_access_list_check(xr_access_list_t *alist, xr_path_t *path, long flags,
                          xr_access_type_t type) {
  for (int i = 0; i < alist->nentry; ++i) {
    bool path_match = ((type == XR_ACCESS_TYPE_DIR)
                         ? xr_path_contains(&alist->entries[i].path, path)
                         : xr_string_equal(&alist->entries[i].path, path));
    if (path_match && __do_file_flags_check(&alist->entries[i], flags)) {
      return true;
    }
  }
  return false;
}
