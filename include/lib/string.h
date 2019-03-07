#ifndef XR_STRING_H
#define XR_STRING_H

#include <stdlib.h>
#include <string.h>

typedef struct xr_string_s xr_string_t;

struct xr_string_s {
  int length;
  int capacity;
  char *string;
};

static inline void xr_string_delete(xr_string_t *str) {
  str->length = 0;
  free(str->string);
}

static inline void xr_string_copy(xr_string_t *src, xr_string_t *dst) {
  if (src != dst) {
    if (dst->capacity <= src->length) {
      dst->string = (char *)realloc(dst->string, src->length + 1);
      dst->capacity = src->length;
    }
    strncpy(dst->string, src->string, src->length);
    dst->string[src->length] = 0;
    dst->length = src->length;
  }
}

#endif
