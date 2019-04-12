#include "xrun/option.h"
#include "xrun/utils/json.h"
#include "xrun/utils/string.h"

#include "xrunc/access.h"
#include "xrunc/config.h"

#define XRN_CONFIG_PARSE_ERROR_BUFFER 64

#define XRN_CONFIG_SIGN_IF_NOT_ZERO(name, val) \
  do {                                         \
    if ((name) != 0) {                         \
      name = val;                              \
    }                                          \
  } while (0)

bool xrn_config_access_validate(xr_json_t *access, const char *json_path,
                                xr_string_t *error) {
  if (XR_JSON_IS_OBJECT(access) == false) {
    xr_string_format(error, "%s is not a object.", json_path);
    return false;
  }
  if (_XR_JSON_OBJECT(access)->len != 2 && _XR_JSON_OBJECT(access)->len != 3) {
    xr_string_format(error, "%s has invalid key.", json_path);
    return false;
  }
  return true;
}

bool xrn_config_parse_access(xr_json_t *entry, const char *json_path,
                             xr_access_list_t *alist, xr_string_t *error) {
  if (xrn_config_access_validate(entry, json_path, error) == false) {
    return false;
  }
  char *path = NULL;
  long flags = 0;
  xr_access_mode_t mode = XR_ACCESS_MODE_FLAG_MATCH;
  for (int i = 0; i < _XR_JSON_OBJECT(entry)->len; ++i) {
    const char *key = entry->u.object.keys[i];
    xr_json_t *value = entry->u.object.values[i];
    if (strcmp("path", key) == 0) {
      if (!XR_JSON_IS_STRING(value)) {
        xr_string_format(error, "%s.path is not a string.", json_path);
        return false;
      }
      path = _XR_JSON_STRING(value);
    } else if (strcmp("flags", key) == 0) {
      if (XR_JSON_IS_STRING(value)) {
        if (xrn_access_read_flags(_XR_JSON_STRING(value), &flags) == false) {
          xr_string_format(error, "%s.flags(%s) is invalid.", json_path,
                           _XR_JSON_STRING(value));
          return false;
        }
      } else if (XR_JSON_IS_INTEGER(value)) {
        flags = XR_JSON_INTEGER(value);
      } else {
        xr_string_format(error, "%s.flags is not a string or integer.",
                         json_path);
        return false;
      }
    } else if (strcmp("contains", key) == 0) {
      if (!XR_JSON_IS_TRUE(value) && !XR_JSON_IS_FALSE(value)) {
        xr_string_format(error, "%s.contains is not a boolean.", json_path);
        return false;
      }
      mode = XR_JSON_IS_TRUE(value) ? XR_ACCESS_MODE_FLAG_CONTAINS
                                    : XR_ACCESS_MODE_FLAG_MATCH;
    } else {
      xr_string_format(error, "%s.%s is not a valid field.", json_path, key);
      return false;
    }
  }
  if (path == NULL) {
    xr_string_format(error, "%s.path is required.", json_path);
    return false;
  }
  xr_access_list_append(alist, path, strlen(path), flags, mode);
  return true;
}

bool xrn_config_parse_access_list(xr_json_t *entries, const char *json_path,
                                  xr_access_list_t *alist, xr_string_t *error) {
  if (!XR_JSON_IS_ARRAY(entries)) {
    xr_string_format(error, "config.%s is not an array.", json_path);
    return false;
  }

  size_t len = _XR_JSON_ARRAY(entries)->len;
  xr_string_t access_path;
  xr_string_zero(&access_path);
  for (int i = 0; i < len; ++i) {
    xr_json_t *entry = XR_JSON_ARRAY(entries)->values[i];
    xr_string_format(&access_path, "%s[%d]", json_path, i);
    if (xrn_config_parse_access(entry, access_path.string, alist, error) ==
        false) {
      xr_string_delete(&access_path);
      return false;
    }
  }
  xr_string_delete(&access_path);
  return true;
}

bool xrn_config_parse_calls(xr_json_t *entries, bool *calls,
                            xr_string_t *error) {
  if (!XR_JSON_IS_ARRAY(entries)) {
    xr_string_format(error, "config.calls is not an array.");
    return false;
  }
  size_t len = _XR_JSON_ARRAY(entries)->len;
  for (int i = 0; i < len; ++i) {
    xr_json_t *call = _XR_JSON_ARRAY(entries)->values[i];
    long v = 0;
    if (XR_JSON_IS_INTEGER(call)) {
      v = XR_JSON_INTEGER(call);
    } else if (XR_JSON_IS_STRING(call)) {
      // TODO: compat dectect.
      v = XR_CALLS_CONVERT(_XR_JSON_STRING(call), 1);
    } else {
      xr_string_format(error, "config.calls[%d] is not a string or number.", i);
      return false;
    }
    if (v < 0 || v >= XR_SYSCALL_MAX) {
      xr_string_format(
        error, "config.calls[%d] is not a valid system call number.", i);
      return false;
    }
    calls[v] = true;
  }
  return true;
}

#define __xrn_parse_failed(cfg) (xr_json_free(cfg), fclose(config), false)

bool xrn_config_parse(const char *config_path, xr_option_t *option,
                      xr_string_t *error) {
  FILE *config = fopen(config_path, "r");
  if (config == NULL) {
    xr_string_format(error, "config in %s does not exist.", config_path);
    return false;
  }
  xr_json_t *cfg_json = xr_json_parse(config, error);
  if (cfg_json == NULL) {
    fclose(config);
    return false;
  }

  xr_json_t *files = xr_json_get(cfg_json, "s", "files");
  if (files) {
    if (xrn_config_parse_access_list(files, "files", &option->files, error) ==
        false) {
      return __xrn_parse_failed(cfg_json);
    }
  }

  xr_json_t *directories = xr_json_get(cfg_json, "s", "directories");
  if (directories) {
    if (xrn_config_parse_access_list(directories, "directories",
                                     &option->directories, error) == false) {
      return __xrn_parse_failed(cfg_json);
    }
  }

  xr_json_t *calls = xr_json_get(cfg_json, "s", "calls");
  if (calls) {
    if (xrn_config_parse_calls(calls, option->calls, error) == false) {
      return __xrn_parse_failed(cfg_json);
    }
  }

  xr_json_t *memory = xr_json_get(cfg_json, "s", "memory");
  if (memory) {
    if (!XR_JSON_IS_INTEGER(memory)) {
      xr_string_format(error, "config.memory is not a number.");
      return __xrn_parse_failed(cfg_json);
    }
    long v = XR_JSON_INTEGER(memory);
    if (v <= 0) {
      xr_string_format(error, "config.memory must be greater than 0.");
      return __xrn_parse_failed(cfg_json);
    }
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit.memory, v);
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit_per_process.memory, v);
  }

  xr_json_t *process = xr_json_get(cfg_json, "s", "process");
  if (process) {
    if (!XR_JSON_IS_INTEGER(process)) {
      xr_string_format(error, "config.process is not a number.");
      return __xrn_parse_failed(cfg_json);
    }
    long v = XR_JSON_INTEGER(process);
    if (v <= 0) {
      xr_string_format(error, "config.process must be greater than 0.");
      return __xrn_parse_failed(cfg_json);
    }
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->nprocess, v);
  }

  xr_json_t *nfile = xr_json_get(cfg_json, "s", "nfile");
  if (nfile) {
    if (!XR_JSON_IS_INTEGER(nfile)) {
      xr_string_format(error, "config.nfile is not a number.");
      return __xrn_parse_failed(cfg_json);
    }
    long v = XR_JSON_INTEGER(nfile);
    if (v <= 0) {
      xr_string_format(error, "config.nfile must be greater than 0.");
      return __xrn_parse_failed(cfg_json);
    }
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit.nfile, v);
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit_per_process.nfile, v);
  }

  xr_json_t *time_limit = xr_json_get(cfg_json, "s", "time");
  if (time_limit) {
    if (!XR_JSON_IS_INTEGER(time_limit)) {
      xr_string_format(error, "config.time is not a number.");
      return __xrn_parse_failed(cfg_json);
    }
    long v = XR_JSON_INTEGER(time_limit);
    if (v <= 0) {
      xr_string_format(error, "config.time must be greater than 0.");
      return __xrn_parse_failed(cfg_json);
    }
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit.time.sys_time, v);
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit_per_process.time.sys_time, v);
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit.time.user_time, v);
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit_per_process.time.user_time, v);
  }

  xr_json_t *thread = xr_json_get(cfg_json, "s", "thread");
  if (thread) {
    if (!XR_JSON_IS_INTEGER(thread)) {
      xr_string_format(error, "config.thread is not a number.");
      return __xrn_parse_failed(cfg_json);
    }
    long v = XR_JSON_INTEGER(thread);
    if (v <= 0) {
      xr_string_format(error, "config.thread must be greater than 0.");
      return __xrn_parse_failed(cfg_json);
    }
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit.nthread, v);
    XRN_CONFIG_SIGN_IF_NOT_ZERO(option->limit_per_process.nthread, v);
  }

  xr_json_free(cfg_json);
  return true;
}

#undef __xrn_parse_failed
