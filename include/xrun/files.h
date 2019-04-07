#ifndef XR_FILES_H
#define XR_FILES_H

#include "xrun/utils/list.h"
#include "xrun/utils/path.h"

typedef struct xr_file_s xr_file_t;
typedef struct xr_file_set_s xr_file_set_t;
typedef struct xr_fs_s xr_fs_t;

struct xr_file_s {
  int fd;
  long flags;
  long write_length, read_length;
  xr_path_t path;
  xr_list_t files;
};

struct xr_file_set_shared_s {
  size_t own;
  xr_list_t files;

  int file_opened, file_holding;
  long long total_write, total_read;
};

struct xr_file_set_s {
  struct xr_file_set_shared_s *data;
};

struct xr_fs_shared_s {
  size_t own;
  xr_path_t pwd;
};

struct xr_fs_s {
  struct xr_fs_shared_s *data;
};

static inline void xr_file_init(xr_file_t *file) {
  xr_string_zero(&file->path);
}

static inline void xr_file_delete(xr_file_t *file) {
  xr_path_delete(&file->path);
}

/**
 * Open a new file
 * @@file
 * @fd file descriptor of opened file
 * @flags open flags
 * @path - file path
 */
static inline void xr_file_open(xr_file_t *file, int fd, long flags,
                                xr_path_t *path) {
  xr_string_swap(&file->path, path);
  file->fd = fd;
  file->flags = flags;
  file->write_length = file->read_length = 0;
}

static inline void __xr_file_dup(xr_file_t *file, xr_file_t *dfile) {
  xr_string_copy(&dfile->path, &file->path);
  dfile->flags = file->flags;
  dfile->write_length = file->write_length;
  dfile->read_length = file->write_length;
}

/**
 * Duplicate file.
 *
 * @@file
 * @dfile new file pointer
 * @dup_fd file descriptor of new file
 */
static inline void xr_file_dup(xr_file_t *file, xr_file_t *dfile, int dup_fd) {
  __xr_file_dup(file, dfile);
  dfile->fd = dup_fd;
}

static inline void xr_file_set_create(xr_file_set_t *fset) {
  fset->data = _XR_NEW(struct xr_file_set_shared_s);
  memset(fset->data, 0, sizeof(struct xr_file_set_shared_s));
  fset->data->own = 1;
  xr_list_init(&fset->data->files);
}

/**
 * Delete file set content.
 *
 * @@fset
 */
static inline void xr_file_set_delete(xr_file_set_t *fset) {
  if (fset->data->own-- != 0) {
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

/**
 * Share a file set.
 *
 * @@fset
 * @sfset shared file set
 */
static inline void xr_file_set_share(xr_file_set_t *fset,
                                     xr_file_set_t *sfset) {
  sfset->data = fset->data;
  sfset->data->own++;
}

/**
 * Get ownship of file set assets by cloning shared data.
 *
 * @@fset
 */
static inline void xr_file_set_own(xr_file_set_t *fset) {
  if (fset->data->own == 1) {
    return;
  }
  struct xr_file_set_shared_s *data = _XR_NEW(struct xr_file_set_shared_s);
  data->own = 0;
  data->total_read = fset->data->total_read;
  data->total_write = fset->data->total_write;
  // copy file list
  xr_file_t *file;
  xr_list_init(&(data->files));
  _xr_list_for_each_entry_r(&(fset->data->files), file, xr_file_t, files) {
    xr_file_t *dfile = _XR_NEW(xr_file_t);
    xr_file_dup(file, dfile, file->fd);
    xr_list_add(&data->files, &dfile->files);
  }
  fset->data->own--;
  fset->data = data;
}

/**
 * Select one file via fd
 *
 * @@fset
 * @fd fd of file which is looking for
 *
 * @return target file
 */
static inline xr_file_t *xr_file_set_select_file(xr_file_set_t *fset, int fd) {
  xr_file_t *file;
  _xr_list_for_each_entry(&(fset->data->files), file, xr_file_t, files) {
    if (file->fd == fd) {
      return file;
    }
  }
  return NULL;
}

static inline void xr_file_set_close_file(xr_file_set_t *fset, int fd) {
  xr_list_t *cur, *tmp;
  _xr_list_for_each_safe(&fset->data->files, cur, tmp) {
    xr_file_t *file = xr_list_entry(cur, xr_file_t, files);
    if (file->fd == fd) {
      xr_list_del(cur);
      xr_file_delete(file);
      free(file);
      return;
    }
  }
}

/**
 * Add new file to file set
 *
 * @@fset
 * @file - new file
 */
static inline void xr_file_set_add_file(xr_file_set_t *fset, xr_file_t *file) {
  xr_list_add(&(fset->data->files), &(file->files));
  fset->data->file_opened++;
  fset->data->file_holding++;
}

/**
 * Remove file from fset
 *
 * @@fset
 * @file file to be deleted
 */
static inline void xr_file_set_remove_file(xr_file_set_t *fset,
                                           xr_file_t *file) {
  xr_list_del(&(file->files));
  xr_file_delete(file);
  free(file);
  fset->data->file_holding--;
}

#define xr_file_set_set_read(fset, read) \
  do {                                   \
    (fset)->data->total_read = read;     \
  } while (0)

#define xr_file_set_set_write(fset, write) \
  do {                                     \
    (fset)->data->total_write = write;     \
  } while (0)

#define xr_file_set_get_read(fset) ((fset)->data->total_read)
#define xr_file_set_get_write(fset) ((fset)->data->total_write)

#define xr_file_set_read(fset, read) \
  xr_file_set_set_read(fset, xr_file_set_get_read(fset) + read)

#define xr_file_set_write(fset, write) \
  xr_file_set_set_write(fset, xr_file_set_get_write(fset) + write)

static inline void xr_fs_create(xr_fs_t *fs) {
  fs->data = _XR_NEW(struct xr_fs_shared_s);
  fs->data->own = 1;
  xr_string_zero(&fs->data->pwd);
}

static inline void xr_fs_share(xr_fs_t *fs, xr_fs_t *sfs) {
  fs->data->own++;
  sfs->data = fs->data;
}

static inline void xr_fs_own(xr_fs_t *fs) {
  if (fs->data->own >= 1) {
    struct xr_fs_shared_s *data = _XR_NEW(struct xr_fs_shared_s);
    xr_string_init(&data->pwd, fs->data->pwd.length + 1);
    xr_string_copy(&data->pwd, &fs->data->pwd);
    fs->data->own--;
    fs->data = data;
  }
}

static inline void xr_fs_delete(xr_fs_t *fs) {
  if (fs->data->own-- != 0) {
    return;
  }
  xr_path_delete(&fs->data->pwd);
  free(fs->data);
}

static inline xr_path_t *xr_fs_pwd(xr_fs_t *fs) {
  return &fs->data->pwd;
}

#define xr_fs_clone(fs, sfs) \
  do {                       \
    xr_fs_shared(fs, sfs);   \
    xr_fs_own(sfs);          \
  } while (0)

#endif
