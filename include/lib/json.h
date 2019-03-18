#ifndef XR_JSON_H
#define XR_JSON_H

#include <stdarg.h>
#include <stdio.h>

#include <yajl/yajl_parse.h>
#include <yajl/yajl_tree.h>

#include "lib/string.h"

typedef struct yajl_val_s xr_json_t;

#define XR_JSON_IS_STRING YAJL_IS_STRING
#define XR_JSON_IS_NUMBER YAJL_IS_NUMBER
#define XR_JSON_IS_INTEGER YAJL_IS_INTEGER
#define XR_JSON_IS_DOUBLE YAJL_IS_DOUBLE
#define XR_JSON_IS_OBJECT YAJL_IS_OBJECT
#define XR_JSON_IS_ARRAY YAJL_IS_ARRAY
#define XR_JSON_IS_TRUE YAJL_IS_TRUE
#define XR_JSON_IS_FALSE YAJL_IS_FALSE
#define XR_JSON_IS_NULL YAJL_IS_NULL

#define XR_JSON_STRING YAJL_GET_STRING
#define XR_JSON_NUMBER YAJL_GET_NUMBER
#define XR_JSON_DOUBLE YAJL_GET_DOUBLE
#define XR_JSON_INTEGER YAJL_GET_INTEGER
#define XR_JSON_OBJECT YAJL_GET_OBJECT
#define XR_JSON_ARRAY YAJL_GET_ARRAY

void xr_json_free(xr_json_t *json);

xr_json_t *xr_json_get_v(xr_json_t *json, const char *format, va_list ap);

static inline xr_json_t *xr_json_get(xr_json_t *json, const char *format, ...) {
  va_list args;
  va_start(args, format);
  xr_json_t *target = xr_json_get_v(json, format, args);
  va_end(args);
  return target;
}

xr_json_t *xr_json_parse(FILE *config, xr_string_t *error);

#endif
