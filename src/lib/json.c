#include <stdarg.h>

#include "lib/json.h"

struct xr_json_ctx_stack_s;
typedef struct xr_json_ctx_stack_s xr_json_ctx_stack_t;
struct xr_json_ctx_stack_s {
  char *key;
  xr_json_t *value;
  xr_json_ctx_stack_t *next;
}

struct xr_json_ctx_s;
typedef struct xr_json_ctx_s xr_json_ctx_t;
struct xr_json_ctx_s {
  xr_json_ctx_stack_t *stack;
  xr_json_t *root;
  xr_string_t *error;
};

static inline bool xr_json_ctx_error(xr_json_ctx_t *ctx, const char *errmsg) {
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
    return xr_json_ctx_error(ctx, __func__ ": out of memory.");
  }
  stack->value = json;
  stack->key = NULL;
  stack->next = ctx->stack;
  ctx->stack = stack;
  return true;
}

static inline xr_jsont_t xr_json_ctx_pop(context_t *ctx) {
  if (ctx->stack == NULL) {
    xr_json_ctx_error(ctx, __func__ ": bottom of stack reached prematurely.");
    return NULL;
  }
  xr_json_ctx_stack_t *stack = ctx->stack;
  ctx->stack = stack->next;
  xr_json_t *json = stack->value;
  free(stack);
  return json;
}

static inline xr_json_t xr_json_alloc(yajl_type type) {
  xr_json_t *v = _XR_NEW(xr_json_t);
  if (v == NULL) {
    return NULL;
  }
  memset(v, 0, sizeof(v));
  v->type = type;
  return v;
}

#define XR_RETURN_IF_NOT(v, type) \
  if (!XR_JSON_IS_##type(v)) {    \
    return;                       \
  }

static inline void xr_json_object_free(xr_json_t *obj) {
  XR_RETURN_IF_NOT(obj, OBJECT);
  for (size_t i = 0; i < XR_JSON_OBJECT(obj)->len; ++i) {
    free(XR_JSON_OBJECT(obj)->keys[i]);
    xr_json_free(XR_JSON_OBJECT(obj)->values[i]);
  }
  free(XR_JSON_OBJECT(obj)->keys);
  free(XR_JSON_OBJECT(obj)->value);
}

static inline void xr_json_array_free(xr_json_t *arr) {
  XR_RETURN_IF_NOT(arr, ARRAY);
  for (size_t i = 0; i < XR_JSON_ARRAY(arr)->len; ++i) {
    xr_json_free(XR_JSON_ARRAY(arr)->values[i]);
  }
  free(XR_JSON_ARRAY(arr)->values);
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
  }
  free(json);
}

static inline bool xr_json_ctx_obj_push(xr_json_ctx_t *ctx, xr_json_t *obj,
                                        char *key, xr_json_t *value) {
  const size_t orig_len = XR_JSON_OBJECT(obj)->len;
  char *keys =
    realloc(XR_JSON_OBJECT(obj)->keys, sizeof(char *) * (orig_len + 1));
  xr_json_t *values =
    realloc(XR_JSON_OBJECT(obj)->values, sizeof(xr_json_t *) * (orig_len + 1));
  if (keys == NULL || values == NULL) {
    return xr_json_ctx_error(ctx, __func__ ": out of memory.");
  }

  keys[orig_len + 1] = key;
  values[orig_len + 1] = value;
  XR_JSON_OBJECT(obj)->len++;
  XR_JSON_OBJECT(obj)->keys = keys;
  XR_JSON_OBJECT(obj)->values = values;
  return true;
}

static inline bool xr_json_ctx_array_append(xr_json_ctx_t *ctx,
                                            xr_json_t *array,
                                            xr_json_t *value) {
  const size_t orig_len = XR_JSON_ARRAY(array)->len;
  xr_json_t *values =
    realloc(XR_JSON_ARRAY(array)->values, sizeof(xr_json_t *) * (orig_len + 1));
  if (values == NULL) {
    return xr_json_ctx_error(ctx, __func__ ": out of memory.");
  }

  values[orig_len + 1] = value;
  XR_JSON_ARRAY(array)->len++;
  XR_JSON_ARRAY(array)->values = values;
  return true;
}

static inline bool xr_json_ctx_add(xr_json_ctx_t *ctx, xr_json_t *json) {
  if (ctx->stack == NULL) {
    assert(ctx->root == NULL);
    ctx->root = json;
    return true;
  } else if (XR_JSON_IS_OBJECT(ctx->stack->value)) {
    if (ctx->stack->key == NULL) {
      XR_RETURN_IF_NOT(json, STRING);
      ctx->stack->key = XR_JSON_STRING(json);
      free(json);
      return true;
    } else {
      char *key = ctx->stack->key;
      ctx->stack->key = NULL;
      return xr_json_ctx_obj_push(ctx, ctx->stack->value, key, json);
    }
  } else if (XR_JSON_IS_ARRAY(ctx->stack->value)) {
    return (xr_json_ctx_array_append(ctx, ctx->stack->value, json));
  }
  return xr_json_ctx_error(ctx, __func__ ": unhandled type");
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
  (xr_json_ctx_error(XR_JSON_CALLBACK_CTX_WRAP(ctx), msg), STATUS_ABORT)

#define XR_JSON_CALLBACK_CHECK_AND_ADD(ctx, json)                     \
  (json == NULL)                                                      \
    ? (XR_JSON_CALLBACK_ERROR_WRAP(ctx), __func__ ": out of memory.") \
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
  yajl_val v;
  char *endptr;

  xr_json_t *json = xr_json_alloc(yajl_t_number);
  if (json == NULL) {
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, __func__ ": out of memory.");
  }

  XR_JSON_NUMBER(json) = malloc(string_length + 1);
  if (XR_JSON_NUMBER(json) == NULL) {
    free(json);
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, __func__ ": out of memory.");
  }
  memcpy(XR_JSON_NUMBER(json), string, string_length);
  XR_JSON_NUMBER(json)[string_length] = 0;

  json->u.number.flags = 0;

  errno = 0;
  json->u.number.i = yajl_parse_integer(
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
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, __func__ ": out of memory.");
  }

  json->u.string = malloc(string_length + 1);
  if (json->u.string == NULL) {
    free(json);
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, __func__ ": out of memory.");
  }
  memcpy(v->u.string, string, string_length);
  v->u.string[string_length] = 0;

  return XR_JSON_CALLBACK_ADD_WRAP(ctx, json);
}

static int xr_json_handle_start_map(void *ctx) {
  xr_json_t *json = xr_json_alloc(yajl_t_object);
  if (json == NULL) {
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, __func__ ": out of memory.");
  }
  return XR_JSON_CALLBACK_RETURN_WRAP(
    xr_json_ctx_push(XR_JSON_CALLBACK_CTX_WRAP(ctx), json));
}

static int xr_json_handle_start_array(void *ctx) {
  xr_json_t *json = xr_json_alloc(yajl_t_array);
  if (json == NULL) {
    return XR_JSON_CALLBACK_ERROR_WRAP(ctx, __func__ ": out of memory.");
  }
  return XR_JSON_CALLBACK_RETURN_WRAP(
    xr_json_ctx_push(XR_JSON_CALLBACK_CTX_WRAP(ctx), json));
}

static int xr_json_handle_end_iterable(void *ctx) {
  xr_json_t *json = xr_json_ctx_pop(XR_JSON_CALLBACK_CTX_WRAP(ctx));
  return json == NULL ? STATUS_ABORT : XR_JSON_CALLBACK_ADD_WRAP(ctx, json);
}

xr_json_t *xr_json_parse(FILE *json, xr_string_t *error) {
  static const yajl_callbacks callbacks = {
    .yajl_null = xr_json_handle_null,
    .yajl_boolean = xr_json_handle_boolean,
    .yajl_number = xr_json_handle_number,
    .yajl_string = xr_json_handle_string,
    .yajl_start_map = xr_json_handle_start_map,
    .yajl_map_key = xr_json_handle_string,
    .yajl_end_map = xr_json_handel_end_iterable,
    .yajl_start_array = xr_json_handle_start_array,
    .yajl_end_array = xr_json_handle_end_iterable,
  };

  const static size_t xr_json_parse_buffer_length = 65535;
  static unsigned char buffer[xrn_config_parse_buffer_length];

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
    nread = fread(buffer, sizeof(unsigned char), xrn_config_parse_buffer_length,
                  config);
    if (nread == 0) {
      if (ferror(config)) {
        xr_json_ctx_error(ctx, __func__ ": read json file error.");
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

    if (status != yajl_status_ok && status != yajl_status_insufficient_data) {
      unsigned char *yajl_error = yajl_get_error(hand, 1, buffer, rd);
      xr_json_ctx_error(ctx, yajl_error);
      yajl_free_error(hand, yajl_error);
      xr_json_free(ctx.root);
      ctx.root == NULL;
      break;
    }
  }
  xr_json_ctx_clean(&ctx);
  yajl_free(handle);
  return ctx.root;
}

xr_json_t *xr_json_get_v(xr_json_t *json, const char *format, va_list ap) {
  char *arg_type = format;
  xr_json_t *target = json;
  while (true) {
    switch (arg_type) {
      case 's': {
        if (!XR_JSON_IS_OBJECT(target)) {
          return NULL;
        }
        const char *key = va_arg(ap, const char *);
        const size_t len = XR_JSON_OBJECT(target)->len;
        bool found = false;
        for (size_t i = 0; i < len; ++i) {
          if (!strcmp(key, XR_JSON_OBJECT(target)->keys[i])) {
            target = XR_JSON_OBJECT(target)->values[i];
            found = true;
            break;
          }
        }
        if (found == false) {
          return NULL;
        }
      }
      case 'd': {
        if (!XR_JSON_IS_ARRAY(target)) {
          return NULL;
        }
        long idx = va_arg(ap, long);
        const size_t len = XR_JSON_OBJECT(target)->len;
        if (idx >= len) {
          return NULL;
        } else {
          target = XR_JSON_ARRAY(target)->values[idx];
        }
      }
      case '\0':
        return target;
      default:
        return NULL;
    }
  }
}
