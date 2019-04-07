#ifndef XR_ERROR_H
#define XR_ERROR_H

#include <errno.h>
#include <stdarg.h>

#include "xrun/utils/list.h"
#include "xrun/utils/string.h"
#include "xrun/utils/utils.h"

typedef struct xr_error_s xr_error_t;
typedef struct xr_error_msg_s xr_error_msg_t;

struct xr_error_s {
  size_t estack;
  xr_list_t errors;
};

struct xr_error_msg_s {
  xr_string_t emsg;
  xr_list_t errors;
};

#define _XR_ERROR_MSG_SEP " -- "
#define _XR_ERROR_MSG_NSEP (sizeof(_XR_ERROR_MSG_SEP) / sizeof(char) - 1)

static inline void xr_error_msg_vnerror(xr_error_msg_t *emsg, int eno,
                                        const char *format, va_list args) {
  char *estr = strerror(eno);
  size_t nestr = strlen(estr);
  xr_string_vformat(&emsg->emsg, format, args);
  xr_string_grow(&emsg->emsg,
                 emsg->emsg.capacity + nestr + _XR_ERROR_MSG_NSEP + 1);
  xr_string_concat_raw(&emsg->emsg, _XR_ERROR_MSG_SEP, _XR_ERROR_MSG_NSEP);
  xr_string_concat_raw(&emsg->emsg, estr, nestr);
}

static inline void xr_error_msg_verror(xr_error_msg_t *emsg, const char *format,
                                       va_list args) {
  xr_string_vformat(&emsg->emsg, format, args);
}

static inline void xr_error_msg_init(xr_error_msg_t *emsg) {
  xr_list_init(&emsg->errors);
  xr_string_zero(&emsg->emsg);
}

static inline void xr_error_msg_delete(xr_error_msg_t *emsg) {
  xr_string_delete(&emsg->emsg);
}

static inline void xr_error_init(xr_error_t *error) {
  xr_list_init(&error->errors);
  error->estack = 0;
}

static inline void xr_error_delete(xr_error_t *error) {
  xr_list_t *cur, *temp;
  _xr_list_for_each_safe(&error->errors, cur, temp) {
    xr_error_msg_delete(xr_list_entry(cur, xr_error_msg_t, errors));
    xr_list_del(cur);
  }
  error->estack = 0;
}

static inline void xr_error_verror(xr_error_t *error, const char *format,
                                   va_list args) {
  xr_error_msg_t *emsg = _XR_NEW(xr_error_msg_t);
  xr_error_msg_init(emsg);
  xr_list_add(&error->errors, &emsg->errors);
  error->estack++;
  xr_error_msg_verror(emsg, format, args);
}

static inline void xr_error_vnerror(xr_error_t *error, int eno,
                                    const char *format, va_list args) {
  xr_error_msg_t *emsg = _XR_NEW(xr_error_msg_t);
  xr_error_msg_init(emsg);
  xr_list_add(&error->errors, &emsg->errors);
  error->estack++;
  xr_error_msg_vnerror(emsg, eno, format, args);
}
static inline void xr_error_error(xr_error_t *error, const char *format, ...) {
  va_list args;
  va_start(args, format);
  xr_error_verror(error, format, args);
  va_end(args);
}

static inline void xr_error_nerror(xr_error_t *error, int eno,
                                   const char *format, ...) {
  va_list args;
  va_start(args, format);
  xr_error_vnerror(error, eno, format, args);
  va_end(args);
}

static inline void xr_error_tostring(xr_error_t *error, xr_string_t *str) {
  static xr_string_t emsg_sep = {
    .length = 1,
    .capacity = 2,
    .string = "\n",
  };
  xr_string_grow(str, (_XR_STRING_DEFAULT_CAPACITY + 1) * error->estack);
  xr_error_msg_t *emsg;
  _xr_list_for_each_entry(&error->errors, emsg, xr_error_msg_t, errors) {
    xr_string_concat(str, &emsg->emsg);
    xr_string_concat(str, &emsg_sep);
  }
}

#endif
