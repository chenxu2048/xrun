#include <ctype.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "xrunc/file_limit.h"

struct xrn_file_flag {
  long flag;
  const char *name;
  size_t len;
};

static const struct xrn_file_flag xrn_file_flags[] = {
  {.flag = O_RDONLY, .name = "O_RDONLY", .len = 8},
  {.flag = O_WRONLY, .name = "O_WRONLY", .len = 8},
  {.flag = O_RDWR, .name = "O_RDWR", .len = 6},
#ifdef O_APPEND
  {.flag = O_APPEND, .name = "O_APPEND", .len = 8},
#endif
#ifdef O_ASYNC
  {.flag = O_ASYNC, .name = "O_ASYNC", .len = 7},
#endif
#ifdef O_CLOEXEC
  {.flag = O_CLOEXEC, .name = "O_CLOEXEC", .len = 9},
#endif
#ifdef O_CREAT
  {.flag = O_CREAT, .name = "O_CREAT", .len = 7},
#endif
#ifdef O_DIRECT
  {.flag = O_DIRECT, .name = "O_DIRECT", .len = 8},
#endif
#ifdef O_DIRECTORY
  {.flag = O_DIRECTORY, .name = "O_DIRECTORY", .len = 11},
#endif
#ifdef O_DSYNC
  {.flag = O_DSYNC, .name = "O_DSYNC", .len = 7},
#endif
#ifdef O_EXCL
  {.flag = O_EXCL, .name = "O_EXCL", .len = 6},
#endif
#ifdef O_LARGEFILE
  {.flag = O_LARGEFILE, .name = "O_LARGEFILE", .len = 11},
#endif
#ifdef O_NOATIME
  {.flag = O_NOATIME, .name = "O_NOATIME", .len = 9},
#endif
#ifdef O_NOCTTY
  {.flag = O_NOCTTY, .name = "O_NOCTTY", .len = 8},
#endif
#ifdef O_NOFOLLOW
  {.flag = O_NOFOLLOW, .name = "O_NOFOLLOW", .len = 10},
#endif
#ifdef O_NONBLOCK
  {.flag = O_NONBLOCK, .name = "O_NONBLOCK", .len = 10},
#endif
#ifdef O_NDELAY
  {.flag = O_NDELAY, .name = "O_NDELAY", .len = 8},
#endif
#ifdef O_PATH
  {.flag = O_PATH, .name = "O_PATH", .len = 6},
#endif
#ifdef O_SYNC
  {.flag = O_SYNC, .name = "O_SYNC", .len = 6},
#endif
#ifdef O_TMPFILE
  {.flag = O_TMPFILE, .name = "O_TMPFILE", .len = 9},
#endif
#ifdef O_TRUNC
  {.flag = O_TRUNC, .name = "O_TRUNC", .len = 7},
#endif
};

const static size_t xrn_file_flag_num =
  sizeof(xrn_file_flags) / sizeof(struct xrn_file_flag);

bool xrn_file_limit_flag_name(char *name, size_t len, long *flag) {
  for (int i = 0; i < xrn_file_flag_num; ++i) {
    if (xrn_file_flags[i].len == len &&
        strncmp(xrn_file_flags[i].name, name, len) == 0) {
      *flag = xrn_file_flags[i].flag;
      return true;
    }
  }
  return false;
}

bool xrn_file_limit_read_flags(char *names, long *flags) {
  int nflag = 0;
  char *end = names;
  if (isdigit(*names)) {
    *flags = strtol(names, &end, 10);
    return *end == '\0';
  }
  long flag = 0;
  char *start = names;
  while (true) {
    if (*end == '+' || *end == '\0') {
      if (xrn_file_limit_flag_name(start, end - start, &flag)) {
        *flags |= flag;
        nflag++;
      } else {
        return false;
      }
      start = end + 1;
    }
    if (*end == '\0') {
      break;
    }
    end++;
  }
  return nflag != 0;
}

bool xrn_file_access_read(xr_access_list_t *alist, char *path) {
  char *flag = path;
  xr_access_mode_t mode = XR_ACCESS_MODE_FLAG_MATCH;
  while (*flag != '\0') {
    if (*flag == ':' && flag != path && *(flag - 1) != '\\') {
      break;
    }
    flag++;
  }
  // no flag
  if (*flag == '\0') {
    return false;
  }
  size_t path_len = flag - path;

  flag++;
  if (*flag == '-') {
    mode = XR_ACCESS_MODE_FLAG_CONTAINS;
    flag++;
  }
  long flags = 0;
  if (xrn_file_limit_read_flags(flag, &flags) == false) {
    return false;
  }
  xr_access_list_append(alist, path, path_len, flags, mode);
  return true;
}
