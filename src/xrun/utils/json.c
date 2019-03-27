#include <assert.h>
#include <limits.h>
#include <stdarg.h>

#include <errno.h>

#include "xrun/utils/json.h"
#include "xrun/utils/utils.h"

#define MAX_VALUE_TO_MULTIPLY ((LLONG_MAX / 10) + (LLONG_MAX % 10))

/* copy from yajl lib */
/* same semantics as strtol */
static inline long long xr_json_parse_integer(const unsigned char *number,
                                              unsigned int length) {
  long long ret = 0;
  long sign = 1;
  const unsigned char *pos = number;
  if (*pos == '-') {
    pos++;
    sign = -1;
  }
  if (*pos == '+') {
    pos++;
  }

  while (pos < number + length) {
    if (ret > MAX_VALUE_TO_MULTIPLY) {
      errno = ERANGE;
      return sign == 1 ? LLONG_MAX : LLONG_MIN;
    }
    ret *= 10;
    if (LLONG_MAX - ret < (*pos - '0')) {
      errno = ERANGE;
      return sign == 1 ? LLONG_MAX : LLONG_MIN;
    }
    if (*pos < '0' || *pos > '9') {
      errno = ERANGE;
      return sign == 1 ? LLONG_MAX : LLONG_MIN;
    }
    ret += (*pos++ - '0');
  }

  return sign * ret;
}

#define STATUS_CONTINUE 1
#define STATUS_ABORT 0

struct xr_json_ctx_stack_s;
typedef struct xr_json_ctx_stack_s xr_json_ctx_stack_t;
struct xr_json_ctx_stack_s {
  char *key;
  xr_json_t *value;
  xr_json_ctx_stack_t *next;
};

struct xr_json_ctx_s;
typedef struct xr_json_ctx_s xr_json_ctx_t;
struct xr_json_ctx_s {
  xr_json_ctx_stack_t *stack;
  xr_json_t *root;
  xr_string_t *error;
};

#define _XR_JSON_CTX_ERROR(ctx, err) (xr_json_ctx_error(ctx, __func__, err))

static inline bool xr_json_ctx_error(xr_json_ctx_t *ctx, const char *func,
                                     const char *errmsg) {
  xr_string_concat_raw(ctx->error, func, strlen(func));
  xr_string_concat_raw(ctx->error, ": ", 2);
  xr_string_concat_raw(ctx->error, errmsg, strlen(errmsg));
  return false;
}

static inline void xr_json_ctx_stack_clean(xr_json_ctx_t *ctx) {
  while (ctx->stack) {
    xr_json_ctx_stack_t *stack = ctx->stack;
    ctx->stack = stack->next;
    if (stack->key != NULL) {
      free(stack->key);
    }
    if (stack->value != NULL) {
      xr_json_free(stack->value);
    }
    free(stack);
  }
}

static inline bool xr_json_ctx_push(xr_json_ctx_t *ctx, xr_json_t *json) {
  xr_json_ctx_stack_t *stack = _XR_NEW(xr_json_ctx_stack_t);
  if (stack == NULL) {
    return _XR_JSON_CTX_ERROR(ctx, "out of memory.");
  }
  stack->value = json;
  stack->key = NULL;
  stack->next = ctx->stack;
  ctx->stack = stack;
  return true;
}

static inline xr_json_t *xr_json_ctx_pop(xr_json_ctx_t *ctx) {
  if (ctx->stack == NULL) {
    _XR_JSON_CTX_ERROR(ctx, "bottom of stack reached prematurely.");
    return NULL;
  }
  xr_json_ctx_stack_t *stack = ctx->stack;
  ctx->stack = stack->next;
  xr_json_t *json = stack->value;
  free(stack);
  return json;
}

static inline xr_json_t *xr_json_alloc(yajl_type type) {
  xr_json_t *v = _XR_NEW(xr_json_t);
  if (v == NULL) {
    return NULL;
  }
  memset(v, 0, sizeof(xr_json_t));
  v->type = type;
  return v;
}

#define XR_RETURN_IF_NOT(v, type) \
  if (!XR_JSON_IS_##type(v)) {    \
    return;                       \
  }

static inline void xr_json_object_free(xr_json_t *obj) {
  XR_RETURN_IF_NOT(obj, OBJECT);
  for (size_t i = 0; i < _XR_JSON_OBJECT(obj)->len; ++i) {
    free((void *)(_XR_JSON_OBJECT(obj)->keys[i]));
    xr_json_free(_XR_JSON_OBJECT(obj)->values[i]);
  }
  free(_XR_JSON_OBJECT(obj)->keys);
  free(_XR_JSON_OBJECT(obj)->values);
}

static inline void xr_json_array_free(xr_json_t *arr) {
  XR_RETURN_IF_NOT(arr, ARRAY);
  for (size_t i = 0; i < _XR_JSON_ARRAY(arr)->len; ++i) {
    xr_json_free(_XR_JSON_ARRAY(arr)->values[i]);
  }
  free(_XR_JSON_ARRAY(arr)->values);
}

void xr_json_free(xr_json_t *json) {
  if (json == NULL) return;
  switch (json->type) {
    case yajl_t_string:
      free(XR_JSON_STRING(json));
      break;
    case yajl_t_number:
      free(XR_JSON_NUMBER(json));
      break;
    case yajl_t_object:
      xr_json_object_free(json);
      break;
    case yajl_t_array:
      xr_json_array_free(json);
      break;
    default:
      return;
  }
  free(json);
}

static inline bool xr_json_ctx_obj_push(xr_json_ctx_t *ctx, xr_json_t *obj,
                                        char *key, xr_json_t *value) {
  const size_t orig_len = _XR_JSON_OBJECT(obj)->len;
  const char **keys =
    realloc(_XR_JSON_OBJECT(obj)->keys, sizeof(char *) * (orig_len + 1));
  xr_json_t **values =
    realloc(_XR_JSON_OBJECT(obj)->values, sizeof(xr_json_t *) * (orig_len + 1));
  if (keys == NULL || values == NULL) {
    return _XR_JSON_CTX_ERROR(ctx, "out of memory.");
  }

  keys[orig_len] = key;
  values[orig_len] = value;
  _XR_JSON_OBJECT(obj)->len++;
  _XR_JSON_OBJECT(obj)->keys = keys;
  _XR_JSON_OBJECT(obj)->values = values;
  return true;
}

static inline bool xr_json_ctx_array_append(xr_json_ctx_t *ctx,
                                            xr_json_t *array,
                                            xr_json_t *value) {
  const size_t orig_len = _XR_JSON_ARRAY(array)->len;
  xr_json_t **values = realloc(_XR_JSON_ARRAY(array)->values,
                               sizeof(xr_json_t *) * (orig_len + 1));
  if (values == NULL) {
    return _XR_JSON_CTX_ERROR(ctx, "out of memory.");
  }

  values[orig_len] = value;
  _XR_JSON_ARRAY(array)->len++;
  _XR_JSON_ARRAY(array)->values = values;
  return true;
}

static inline bool xr_json_ctx_add(xr_json_ctx_t *ctx, xr_json_t *json) {
  if (ctx->stack == NULL) {
    assert(ctx->root == NULL);
    ctx->root = json;
    return true;
  } else if (XR_JSON_IS_OBJECT(ctx->stack->value)) {
    if (ctx->stack->key == NULL) {
      if (!XR_JSON_IS_STRING(json)) {
        return false;
      }
      ctx->stack->key = _XR_JSON_STRING(json);
      free(json);
      return true;
    } else {
      char *key = ctx->stack->key;
      ctx->stack->key = NULL;
      return xr_json_ctx_obj_push(ctx, ctx->stack->value, key, json);
    }
  } else if (XR_JSON_IS_ARRAY(ctx->stack->value)) {
    return xr_json_ctx_array_append(ctx, ctx->stack->value, json);
  }
  return _XR_JSON_CTX_ERROR(ctx, "unhandled type");
}

// wrapping context cast
#define XR_JSON_CALLBACK_CTX_WRAP(ctx) ((xr_json_ctx_t *)(ctx))

// wrapping boolean return type to handle status
#define XR_JSON_CALLBACK_RETURN_WRAP(expr) \
  ((expr) == true ? STATUS_CONTINUE : STATUS_ABORT)

#define XR_JSON_CALLBACK_ADD_WRAP(ctx, json) \
  XR_JSON_CALLBACK_RETURN_WRAP(              \
    xr_json_ctx_add(XR_JSON_CALLBACK_CTX_WRAP(ctx), json))

// wrapping error return
#define XR_JSON_CALLBACK_ERROR_WRAP(ctx, msg) \
  (_XR_JSON_CTX_ERROR(XR_JSON_CALLBACK_CTX_WRAP(ctx), msg), STATUS_ABORT)

#define XR_JSON_CALLBACK_CHECK_AND_ADD(ctx, json)                         \
  (json == NULL) ? (XR_JSON_CALLBACK_ERROR_WRAP((ctx), "out of memory.")) \
                 : (XR_JSON_CALLBACK_ADD_WRAP(ctx, json))

static int xr_json_handle_null(void *ctx) {
  xr_json_t *json = xr_json_alloc(yajl_t_null);
  return XR_JSON_CALLBACK_CHECK_AND_ADD(ctx, json);
}

static int xr_json_handle_boolean(void *ctx, int b) {
  xr_json_t *json = xr_json_alloc(b ? yajl_t_true : yajl_t_false);
  return XR_JSON_CALLBACK_CHECK_AND_ADD(ctx, json);
}

static int xr_json_handle_number(void *ctx, const char *string,
                                 size_t string_length) {
  char *endptr;

  xr_json_t *json = xr_json_alloc(yajl_t_number);
  if (json == NULL) {
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, "out of memory.");
  }

  XR_JSON_NUMBER(json) = malloc(string_length + 1);
  if (XR_JSON_NUMBER(json) == NULL) {
    free(json);
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, "out of memory.");
  }
  memcpy(XR_JSON_NUMBER(json), string, string_length);
  XR_JSON_NUMBER(json)[string_length] = 0;

  json->u.number.flags = 0;

  errno = 0;
  json->u.number.i = xr_json_parse_integer(
    (const unsigned char *)XR_JSON_NUMBER(json), strlen(XR_JSON_NUMBER(json)));
  if (errno == 0) {
    json->u.number.flags |= YAJL_NUMBER_INT_VALID;
  }
  endptr = NULL;
  errno = 0;
  json->u.number.d = strtod(json->u.number.r, &endptr);
  if ((errno == 0) && (endptr != NULL) && (*endptr == 0)) {
    json->u.number.flags |= YAJL_NUMBER_DOUBLE_VALID;
  }

  return XR_JSON_CALLBACK_ADD_WRAP(ctx, json);
}

static int xr_json_handle_string(void *ctx, const unsigned char *string,
                                 size_t string_length) {
  xr_json_t *json = xr_json_alloc(yajl_t_string);
  if (json == NULL) {
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, "out of memory.");
  }

  json->u.string = malloc(string_length + 1);
  if (json->u.string == NULL) {
    free(json);
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, "out of memory.");
  }
  memcpy(json->u.string, string, string_length);
  json->u.string[string_length] = 0;

  return XR_JSON_CALLBACK_ADD_WRAP(ctx, json);
}

static int xr_json_handle_start_map(void *ctx) {
  xr_json_t *json = xr_json_alloc(yajl_t_object);
  if (json == NULL) {
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, "out of memory.");
  }
  return XR_JSON_CALLBACK_RETURN_WRAP(
    xr_json_ctx_push(XR_JSON_CALLBACK_CTX_WRAP(ctx), json));
}

static int xr_json_handle_start_array(void *ctx) {
  xr_json_t *json = xr_json_alloc(yajl_t_array);
  if (json == NULL) {
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, "out of memory.");
  }
  return XR_JSON_CALLBACK_RETURN_WRAP(
    xr_json_ctx_push(XR_JSON_CALLBACK_CTX_WRAP(ctx), json));
}

static int xr_json_handle_end_iterable(void *ctx) {
  xr_json_t *json = xr_json_ctx_pop(XR_JSON_CALLBACK_CTX_WRAP(ctx));
  return json == NULL ? STATUS_ABORT : XR_JSON_CALLBACK_ADD_WRAP(ctx, json);
}

#define XRN_JSON_PARSE_BUFFER 65535

xr_json_t *xr_json_parse(FILE *json, xr_string_t *error) {
  static const yajl_callbacks callbacks = {
    .yajl_null = xr_json_handle_null,
    .yajl_boolean = xr_json_handle_boolean,
    .yajl_number = xr_json_handle_number,
    .yajl_string = xr_json_handle_string,
    .yajl_start_map = xr_json_handle_start_map,
    .yajl_map_key = xr_json_handle_string,
    .yajl_end_map = xr_json_handle_end_iterable,
    .yajl_start_array = xr_json_handle_start_array,
    .yajl_end_array = xr_json_handle_end_iterable,
  };

  static unsigned char buffer[XRN_JSON_PARSE_BUFFER];

  yajl_handle handle;
  yajl_status status;
  xr_json_ctx_t ctx = {
    .stack = NULL,
    .root = NULL,
    .error = error,
  };
  size_t nread = 0;
  bool done = false;

  if (json == NULL) {
    return NULL;
  }

  handle = yajl_alloc(&callbacks, NULL, &ctx);
  yajl_config(handle, yajl_allow_comments, 1);

  while (done == false) {
    nread = fread(buffer, sizeof(unsigned char), XRN_JSON_PARSE_BUFFER, json);
    if (nread == 0) {
      if (ferror(json)) {
        _XR_JSON_CTX_ERROR(&ctx, "read json file error.");
        break;
      }
      done = true;
    }
    buffer[nread] = '\0';

    if (done) {
      status = yajl_complete_parse(handle);
    } else {
      status = yajl_parse(handle, buffer, nread);
    }

    if (status != yajl_status_ok) {
      unsigned char *yajl_error = yajl_get_error(handle, 1, buffer, nread);
      _XR_JSON_CTX_ERROR(&ctx, (char *)yajl_error);
      yajl_free_error(handle, yajl_error);
      xr_json_free(ctx.root);
      ctx.root = NULL;
      break;
    }
  }
  xr_json_ctx_stack_clean(&ctx);
  yajl_free(handle);
  return ctx.root;
}

xr_json_t *xr_json_get_v(xr_json_t *json, const char *format, va_list ap) {
  char *arg_type = (char *)format;
  xr_json_t *target = json;
  while (true) {
    switch (*arg_type++) {
      case 's': {
        if (!XR_JSON_IS_OBJECT(target)) {
          return NULL;
        }
        const char *key = va_arg(ap, const char *);
        const size_t len = _XR_JSON_OBJECT(target)->len;
        bool found = false;
        for (size_t i = 0; i < len; ++i) {
          if (!strcmp(key, _XR_JSON_OBJECT(target)->keys[i])) {
            target = _XR_JSON_OBJECT(target)->values[i];
            found = true;
            break;
          }
        }
        if (found == false) {
          return NULL;
        } else {
          break;
        }
      }
      case 'd': {
        if (!XR_JSON_IS_ARRAY(target)) {
          return NULL;
        }
        long idx = va_arg(ap, long);
        const size_t len = _XR_JSON_ARRAY(target)->len;
        if (idx >= len) {
          return NULL;
        } else {
          target = _XR_JSON_ARRAY(target)->values[idx];
          break;
        }
      }
      case '\0':
        return target;
      default:
        return NULL;
    }
  }
}
