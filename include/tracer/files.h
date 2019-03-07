#ifndef XR_FILES_H
#define XR_FILES_H

#include "lib/list.h"
#include "lib/path.h"

typedef struct xr_file_s xr_file_t;
typedef struct xr_file_set_s xr_file_set_t;

struct xr_file_s {
  int fd;
  long mode;
  long write_length, read_length;
  xr_path_t *path;
  xr_list_t files;
};

struct xr_file_set_shared_s {
  xr_list_t files;

  long long total_write, total_read;
};

struct xr_file_set_s {
  bool own;
  struct xr_file_set_shared_s *data;
};

static inline void xr_file_delete(xr_file_t *file) {
  xr_path_delete(file->path);
}

static inline void xr_file_open(xr_file_t *file, int fd, long mode,
                                xr_path_t *path) {
  file->path = path;
  file->fd = fd;
  file->mode = mode;
  file->write_length = file->read_length = 0;
}

static inline xr_file_t *__xr_file_dup(xr_file_t *file) {
  xr_file_t *dup_file =
      _XR_NEW(xr_file_t) xr_string_copy(file->path, dup_file->path);
  dup_file->mode = file->mode;
  dup_file->write_length = file->write_length;
  dup_file->read_length = file->write_length;
  return dup_file;
}

static inline xr_file_t *xr_file_dup(xr_file_t *file, int dup_fd) {
  xr_file_t *dup_file = __xr_file_dup(file);
  dup_file->fd = dup_fd;
  return dup_file;
}

static inline void xr_file_set_delete(xr_file_set_t *fset) {
  if (!fset->own) {
    return;
  }
  xr_list_t *cur, *temp;
  xr_file_t *file;
  _xr_list_for_each_safe(&(fset->data->files), cur, temp) {
    xr_list_del(cur);
    file = xr_list_entry(cur, xr_file_t, files);
    xr_file_delete(file);
    free(file);
  }
  free(fset->data);
}

static inline xr_file_set_t *xr_file_set_share(xr_file_set_t *fset) {
  xr_file_set_t *ret = _XR_NEW(xr_file_set_t);
  ret->data = fset->data;
  ret->own = false;
}

static inline void xr_file_set_own(xr_file_set_t *fset) {
  if (fset->own) {
    return;
  }
  struct xr_file_set_shared_s *data = _XR_NEW(struct xr_file_set_shared_s);
  data->total_read = fset->data->total_read;
  data->total_write = fset->data->total_write;

  // copy file list
  xr_file_t *file;
  xr_list_init(&(data->files));
  _xr_list_for_each_entry_r(&(fset->data->files), file, xr_file_t, files) {
    xr_list_add(&(data->files), &(xr_file_dup(file, file->fd)->files));
  }

  fset->own = true;
  fset->data = data;
}

#endif
