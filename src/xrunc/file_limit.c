#include <ctype.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "xrunc/file_limit.h"

struct xrn_file_flag {
  int flag;
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

int xrn_file_limit_flag_name(char *flag, size_t len) {
  for (int i = 0; i < sizeof(xrn_file_flags) / sizeof(struct xrn_file_flag);
       ++i) {
    if (xrn_file_flags[i].len == len &&
        strncmp(xrn_file_flags[i].name, flag, len) == 0) {
      return xrn_file_flags[i].flag;
    }
  }
  return -1;
}

bool xrn_file_limit_read_flags(char *flags, xr_file_limit_t *flimit) {
  int flags_ = 0;
  int nflag = 0;
  char *end = flags;
  if (isdigit(*flags)) {
    flags_ = strtol(flags, &end, 10);
    if (*end != '\0') {
      return false;
    }
  } else {
    char *start = flags;
    while (true) {
      if (*end == '+' || *end == '\0') {
        int flag = xrn_file_limit_flag_name(start, end - start);
        if (flag == -1) {
          return false;
        } else {
          flags_ |= flag;
          nflag++;
        }
        start = end + 1;
      }
      if (*end == '\0') {
        break;
      }
      end++;
    }
  }
  if (nflag == 0) {
    return false;
  }
  flimit->flags = flags_;
  return true;
}

bool xrn_file_limit_read(char *path, xr_file_limit_t *flimit) {
  char *flag = path;
  enum xr_file_access_mode mode = XR_FILE_ACCESS_MATCH;
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
  flag++;
  if (*flag == '-') {
    mode = XR_FILE_ACCESS_CONTAIN;
    flag++;
  }
  if (xrn_file_limit_read_flags(flag, flimit) == false) {
    return false;
  }
  xr_string_init(&(flimit->path), flag - path);
  xr_string_concat_raw(&(flimit->path), path, flag - path - 1);
  return true;
}

bool xrn_file_limit_read_all(char **pathes, size_t size,
                             xr_file_limit_t *flimit, xr_string_t *error) {
  for (int i = 0; i < size; ++i) {
    if (xrn_file_limit_read(pathes[i], flimit + i) == false) {
      xr_string_format(error, "Invalid path definition %s.", pathes[i]);
      return false;
    }
  }
  return true;
}
