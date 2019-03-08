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
  return str;
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
static inline void xr_string_grow(xr_string_t *str, int capcaity) {
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

/**
 * Strcat wrapper of xr_string_t.
 * Note that:
 *  1. head space may be extended if in need.
 *  2. original content of head will be destroyed.
 *
 * @head head part of new string
 * @rest rest part of new string
 */
static inline void xr_string_concat(xr_string_t *head, xr_string_t *rest) {
  if (head->length + rest->length >= head->capacity) {
    xr_string_grow(head, head->length + rest->length + 1);
  }
  strncpy(head->string + head->length, rest->string, rest->length);
  head->length += rest->length;
  head->string[head->length] = 0;
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
  return strncmp(parent->string, child->string, parent->length) == 0;
}

static inline bool xr_string_equal(xr_string_t lhs, xr_string_t *rhs) {
  return lhs->length == rhs->length &&
         strncmp(lhs->string, rhs->string, lhs->length);
}

#endif
