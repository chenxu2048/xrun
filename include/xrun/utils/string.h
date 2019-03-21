#ifndef XR_STRING_H
#define XR_STRING_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct xr_string_s xr_string_t;

struct xr_string_s {
  int length;
  int capacity;
  char *string;
};

/**
 * Init a string
 *
 * @@str
 * @capacity initial capcity of string area
 */
static inline void xr_string_init(xr_string_t *str, int capacity) {
  str->string = (char *)malloc(sizeof(char) * capacity);
  str->capacity = capacity;
  str->length = 0;
  str->string[0] = 0;
}

/**
 * Delete xr_string_t content
 * @@str
 */
static inline void xr_string_delete(xr_string_t *str) {
  str->length = 0;
  free(str->string);
}

/**
 * Extend space of xr_string_t. if str->capatity were greater then capacity,
 * nothing would happen.
 *
 * @@str
 * @capacity new capacity of string
 */
static inline void xr_string_grow(xr_string_t *str, int capacity) {
  if (capacity > str->capacity) {
    str->string = (char *)realloc(str->string, capacity);
    str->capacity = capacity;
  }
}

/**
 * Copy string content from src to dst. Note that dst space may be extended if
 * in need.
 *
 * @dst destination string
 * @src source string
 */
static inline void xr_string_copy(xr_string_t *dst, xr_string_t *src) {
  if (src != dst) {
    xr_string_grow(src, dst->length + 1);
    strncpy(dst->string, src->string, src->length);
    dst->string[src->length] = 0;
    dst->length = src->length;
  }
}

static inline void xr_string_concat_raw(xr_string_t *head, const char *tail,
                                        size_t length) {
  if (head->length + length >= head->capacity) {
    xr_string_grow(head, head->length + length + 1);
  }
  strncpy(head->string + head->length, tail, length);
  head->length += length;
  head->string[head->length] = 0;
}

/**
 * Strcat wrapper of xr_string_t.
 * Note that:
 *  1. head space may be extended if in need.
 *  2. original content of head will be destroyed.
 *
 * @head head part of new string
 * @tail rest part of new string
 */
static inline void xr_string_concat(xr_string_t *head, xr_string_t *tail) {
  xr_string_concat_raw(head, tail->string, tail->length);
}

/**
 * Examinate str starts with head. It is a wapper of strncmp.
 *
 * @@str
 * @head head string
 *
 * @return bool
 */
static inline bool xr_string_start_with(xr_string_t *str, xr_string_t *head) {
  return strncmp(str->string, head->string, str->length) == 0;
}

static inline bool xr_string_equal(xr_string_t *lhs, xr_string_t *rhs) {
  return lhs->length == rhs->length &&
         strncmp(lhs->string, rhs->string, lhs->length);
}

static inline void xr_string_format(xr_string_t *str, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int wrote = vsnprintf(str->string, str->capacity - 1, format, args);
  va_end(args);
  if (wrote >= str->capacity) {
    va_start(args, format);
    xr_string_grow(str, wrote + 1);
    vsnprintf(str->string, str->capacity - 1, format, args);
    va_end(args);
  }
  str->length = wrote;
  str->string[str->length] = 0;
}

static inline void xr_string_swap(xr_string_t *lhs, xr_string_t *rhs) {
  if (lhs == rhs) {
    return;
  }
  xr_string_t str = *lhs;
  *lhs = *rhs;
  *rhs = str;
}

#endif
