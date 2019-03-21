#include "xrun/utils/json.h"
#include "xrun/utils/string.h"

#include "xrunc/config.h"
#include "xrunc/file_limit.h"

#define XRN_CONFIG_ERROR(error, msg, ...)                                 \
  do {                                                                    \
    char errbuf[512];                                                     \
    errbuf[0] = 0;                                                        \
    size_t n = snprintf(errbuf, 512, "%s:" msg, __func__, ##__VA_ARGS__); \
    xr_string_concat_raw(error, errbuf, n);                               \
  } while (0)

static inline bool xrn_config_parse_access_entry(xr_json_t *entry,
                                                 const char *name, size_t index,
                                                 xr_file_limit_t *limit,
                                                 xr_string_t *error) {
  if (entry == NULL || !XR_JSON_IS_OBJECT(entry)) {
    XRN_CONFIG_ERROR(error, "%s entry %ld type error", name, index);
    return false;
  }

  // get path name
  xr_json_t *path = xr_json_get(entry, "s", "path");
  if (path == NULL) {
    XRN_CONFIG_ERROR(error, "%s entry %ld key \"path\" not found", name, index);
    return false;
  } else if (!XR_JSON_IS_STRING(path)) {
    XRN_CONFIG_ERROR(error, "%s entry %ld key \"path\" is not a string", name,
                     index);
    return false;
  } else {
    size_t path_len = strlen(path->u.string);
    xr_string_init(&(limit->path), path_len + 1);
    xr_string_concat_raw(&(limit->path), XR_JSON_STRING(path), path_len);
  }

  // get flag option
  xr_json_t *flags = xr_json_get(entry, "s", "flags");
  if (flags == NULL) {
    XRN_CONFIG_ERROR(error, "%s entry %ld key \"flags\" not found", name,
                     index);
    return false;
  } else if (XR_JSON_IS_STRING(flags)) {
    // parse string flags
    if (xrn_parse_file_limit_flag(XR_JSON_STRING(flags), limit) == false) {
      XRN_CONFIG_ERROR(error, "%s entry %ld unrecognized flags %s", name, index,
                       XR_JSON_STRING(flags));
      return false;
    }
  } else if (XR_JSON_IS_INTEGER(flags)) {
    // read integer flags
    limit->flags = XR_JSON_INTEGER(flags);
  } else {
    // error type
    XRN_CONFIG_ERROR(error,
                     "%s entry %ld key \"flags\" is not a string or integer",
                     name, index);
    return false;
  }

  xr_json_t *match = xr_json_get(entry, "s", "match");
  if (match == NULL || XR_JSON_IS_FALSE(match)) {
    limit->mode = XR_FILE_ACCESS_CONTAIN;
  } else if (XR_JSON_IS_TRUE(match)) {
    limit->mode = XR_FILE_ACCESS_MATCH;
  } else {
    XRN_CONFIG_ERROR(error, "%s entry %ld key \"match\" is not a boolean", name,
                     index);
    return false;
  }
  return true;
}

static inline bool xrn_config_parse_access_list(xr_json_t *config,
                                                const char *name,
                                                xr_file_limit_t **limits,
                                                int *nentries,
                                                xr_string_t *error) {
  if (config == NULL) {
    return true;
  } else if (!XR_JSON_IS_ARRAY(config)) {
    XRN_CONFIG_ERROR(error, "%s must be list", name);
  }
  const size_t list_size = XR_JSON_ARRAY(config)->len;
  if (list_size == 0) {
    return true;
  }
  size_t resize = *nentries + list_size;
  xr_file_limit_t *limit_new =
    realloc(*limits, resize * sizeof(xr_file_limit_t));
  if (limit_new == NULL) {
    XRN_CONFIG_ERROR(error, "out of memory");
    return false;
  }
  for (size_t i = 0; i < list_size; ++i) {
    if (xrn_config_parse_access_entry(XR_JSON_ARRAY(config)->values[i], name, i,
                                      limit_new + i + *nentries,
                                      error) == false) {
      XRN_CONFIG_ERROR(error, "parse %s entry error", name);
      return false;
    }
  }
  *limits = limit_new;
  *nentries = resize;
  return true;
}

bool xrn_config_parse(const char *config_path, xr_option_t *option,
                      xr_string_t *error) {
  bool retval = false;
  FILE *config_file = fopen(config_path, "r");
  xr_json_t *config = xr_json_parse(config_file, error);
  if (config == NULL) {
    return false;
  }
  xr_json_t *access = xr_json_get(config, "s", "files");
  if (xrn_config_parse_access_list(access, "files", &(option->file_access),
                                   &(option->n_file_access), error) == false) {
    goto config_error;
  }
  access = xr_json_get(config, "s", "directories");
  if (xrn_config_parse_access_list(access, "directories", &(option->dir_access),
                                   &(option->n_dir_access), error) == false) {
    goto config_error;
  }

  xr_json_t *calls = xr_json_get(config, "s", "calls");
  if (calls != NULL) {
    // parse calls
  }

  xr_json_t *fork = xr_json_get(config, "s", "limit", "fork");
  if (fork == NULL) {
    option->nprocess = 1;
  } else if (XR_JSON_IS_INTEGER(fork)) {
    option->nprocess = XR_JSON_INTEGER(fork);
  } else {
    XRN_CONFIG_ERROR(error, "limit.fork is not a integer");
    goto config_error;
  }

  xr_json_t *memory = xr_json_get(config, "s", "limit", "memory");
  if (memory == NULL) {
    XRN_CONFIG_ERROR(error, "limit.memory is required.");
    goto config_error;
  } else if (XR_JSON_IS_INTEGER(memory)) {
    option->limit.memory = option->limit_per_process.memory =
      XR_JSON_INTEGER(memory);
  } else {
    XRN_CONFIG_ERROR(error, "limit.memory is not a integer");
    goto config_error;
  }

  xr_json_t *nfile = xr_json_get(config, "s", "limit", "nfile");
  if (nfile == NULL) {
    XRN_CONFIG_ERROR(error, "limit.nfile is required.");
    goto config_error;
  } else if (XR_JSON_IS_INTEGER(nfile)) {
    option->limit.nfile = option->limit_per_process.nfile =
      XR_JSON_INTEGER(nfile);
  } else {
    XRN_CONFIG_ERROR(error, "limit.nfile is not a integer");
    goto config_error;
  }

  xr_json_t *time = xr_json_get(config, "s", "limit", "time");
  if (time == NULL) {
    XRN_CONFIG_ERROR(error, "limit.time is required.");
    goto config_error;
  } else if (XR_JSON_IS_INTEGER(time)) {
    xr_time_t config_time = {
      XR_JSON_INTEGER(time),
      XR_JSON_INTEGER(time),
    };
    option->limit.time = option->limit_per_process.time = config_time;
  } else {
    XRN_CONFIG_ERROR(error, "limit.time is not a integer");
    goto config_error;
  }

  xr_json_t *thread = xr_json_get(config, "s", "limit", "thread");
  if (thread == NULL) {
    XRN_CONFIG_ERROR(error, "limit.thread is required.");
    goto config_error;
  } else if (XR_JSON_IS_INTEGER(thread)) {
    option->limit.nthread = option->limit_per_process.nthread =
      XR_JSON_INTEGER(thread);
  } else {
    XRN_CONFIG_ERROR(error, "limit.thread is not a integer");
    goto config_error;
  }

  retval = true;

config_error:
  xr_json_free(config);
config_parse_error:
  if (config_file != NULL) {
    fclose(config_file);
  }
  return retval;
}
