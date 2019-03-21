#include <ctype.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "xrunc/file_limit.h"

struct xrn_file_flag {
  int flag;
  const char *name;
};

static const struct xrn_file_flag xrn_file_flags[] = {
  {.flag = O_RDONLY, .name = "O_RDONLY"},
  {.flag = O_WRONLY, .name = "O_WRONLY"},
  {.flag = O_RDWR, .name = "O_RDWR"},
#ifdef O_APPEND
  {.flag = O_APPEND, .name = "O_APPEND"},
#endif
#ifdef O_ASYNC
  {.flag = O_ASYNC, .name = "O_ASYNC"},
#endif
#ifdef O_CLOEXEC
  {.flag = O_CLOEXEC, .name = "O_CLOEXEC"},
#endif
#ifdef O_CREAT
  {.flag = O_CREAT, .name = "O_CREAT"},
#endif
#ifdef O_DIRECT
  {.flag = O_DIRECT, .name = "O_DIRECT"},
#endif
#ifdef O_DIRECTORY
  {.flag = O_DIRECTORY, .name = "O_DIRECTORY"},
#endif
#ifdef O_DSYNC
  {.flag = O_DSYNC, .name = "O_DSYNC"},
#endif
#ifdef O_EXCL
  {.flag = O_EXCL, .name = "O_EXCL"},
#endif
#ifdef O_LARGEFILE
  {.flag = O_LARGEFILE, .name = "O_LARGEFILE"},
#endif
#ifdef O_NOATIME
  {.flag = O_NOATIME, .name = "O_NOATIME"},
#endif
#ifdef O_NOCTTY
  {.flag = O_NOCTTY, .name = "O_NOCTTY"},
#endif
#ifdef O_NOFOLLOW
  {.flag = O_NOFOLLOW, .name = "O_NOFOLLOW"},
#endif
#ifdef O_NONBLOCK
  {.flag = O_NONBLOCK, .name = "O_NONBLOCK"},
#endif
#ifdef O_NDELAY
  {.flag = O_NDELAY, .name = "O_NDELAY"},
#endif
#ifdef O_PATH
  {.flag = O_PATH, .name = "O_PATH"},
#endif
#ifdef O_SYNC
  {.flag = O_SYNC, .name = "O_SYNC"},
#endif
#ifdef O_TMPFILE
  {.flag = O_TMPFILE, .name = "O_TMPFILE"},
#endif
#ifdef O_TRUNC
  {.flag = O_TRUNC, .name = "O_TRUNC"},
#endif
};

int xrn_find_flag(char *flag, size_t len) {
  for (int i = 0; i < sizeof(xrn_file_flags) / sizeof(struct xrn_file_flag);
       ++i) {
    if (strncmp(xrn_file_flags[i].name, flag, len) == 0) {
      return xrn_file_flags[i].flag;
    }
  }
  return -1;
}

bool xrn_parse_file_limit_flag(const char *flags, xr_file_limit_t *flimit) {
  int flags_ = 0;
  char *end = (char *)flags;
  if (isdigit(flags)) {
    flags_ = strtol(flags, &end, 10);
    if (*end != '\0') {
      return false;
    }
  } else {
    char *start = (char *)flags;
    while (*end != '\0') {
      if (*end == '|') {
        int flag = xrn_find_flag(start, end - start);
        if (flag == -1) {
          return false;
        } else {
          flags_ |= flag;
        }
        start = end;
      }
      end++;
    }
  }
  flimit->flags = flags_;
  return true;
}

bool xrn_read_file_limit(const char *path, xr_file_limit_t *flimit) {
  const char *flag = path;
  enum xr_file_access_mode mode = XR_FILE_ACCESS_MATCH;
  while (*flag != '\0') {
    if (*flag == ':' && flag != path && *(flag - 1) == '\\') {
      break;
    }
  }
  // no flag
  if (*flag == '\0') {
    return false;
  }
  flag++;
  if (*flag == '-') {
    mode = XR_FILE_ACCESS_CONTAIN;
    flag++;
  }
  if (xrn_parse_file_limit_flag(flag, flimit) == false) {
    return false;
  }
  xr_string_init(&(flimit->path), flag - path);
  xr_string_concat_raw(&(flimit->path), path, flag - path - 1);
  return true;
}
