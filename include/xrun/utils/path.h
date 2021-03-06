#ifndef XR_PATH_H
#define XR_PATH_H

#include <stdlib.h>

#include "xrun/utils/string.h"
#include "xrun/utils/utils.h"

#define XR_PATH_MAX 4096

typedef xr_string_t xr_path_t;

static inline void xr_path_delete(xr_path_t *path) {
  return xr_string_delete(path);
}

static xr_path_t xr_path_slash = {
  .capacity = 2,
  .length = 1,
  .string = "/",
};

/**
 * Join two path and store it in parent
 *
 * @parent parent directory
 * @child child path. Note: it is undefined if child is a absolute location
 * (starts with /).
 */
static inline void xr_path_join(xr_path_t *parent, xr_path_t *child) {
  xr_string_concat(parent, &xr_path_slash);
  xr_string_concat(parent, child);
}

static inline bool xr_path_is_relative(xr_path_t *path) {
  return path->string[0] != '/';
}

static inline bool xr_path_contains(xr_path_t *parent, xr_path_t *child) {
  if (xr_string_start_with(child, parent) == false) {
    return false;
  }
  if (parent->string[parent->length - 1] == '/') {
    return child->string[parent->length - 1] == '/';
  }
  return child->string[parent->length] == '/';
}

#define __xr_path_is_parent_dir(path, anchor) \
  ((path)->string[anchor + 1] == '.' && (path)->string[anchor + 2] == '.')

/*
 * A stupid implementation of finding absolute path. Result is stored in path.
 *
 * @@path
 */
static inline void xr_path_abs(xr_path_t *path) {
  size_t abs_end = 0, prev_slash = 0;
  for (size_t i = 0; i < path->length; ++i) {
    if (path->string[i] == '/') {
      prev_slash = i;
      if (prev_slash == 1 && path->string[0] == '.') {
        // path is ./xxxx
        abs_end = 0;
      } else {
        // path is /xxx or xxx/xxx
        abs_end = prev_slash + 1;
      }
      break;
    }
  }
  for (size_t i = prev_slash; i <= path->length; ++i) {
    const size_t level_length = i - prev_slash;
    if (path->string[i] != '/' && path->string[i] != 0) {
      continue;
    } else if ((level_length == 2 && path->string[prev_slash + 1] == '.') ||
               level_length == 1) {
      // multiple slash or ./ should be skipped.
      prev_slash = i;
      continue;
    } else if ((level_length == 3 &&
                __xr_path_is_parent_dir(path, prev_slash)) &&
               (abs_end != 0 || abs_end != 1 ||
                strncmp(path->string + abs_end - 2, "..", 2))) {
      // current level is ../ and can go to parent.
      //
      // at top (abs_end == 0), at root (abs_end == 1) and at relative parent
      // (../) can not go to parent.
      for (size_t j = abs_end - 1; j >= 0; --j) {
        if (j <= 1) {
          abs_end = 0;
        }
        if (path->string[j - 1] == '/') {
          abs_end = j;
          break;
        }
      }
    } else {
      // keep current level and copy it.
      if (abs_end != prev_slash + 1) {
        for (size_t j = 0; j < level_length; ++j) {
          path->string[abs_end + j] = path->string[prev_slash + j + 1];
        }
      }
      abs_end += level_length;
    }
    prev_slash = i;
  }
  path->string[abs_end] = 0;
  path->length = abs_end - 1;
}

#endif
