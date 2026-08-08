#ifndef VITTE_BOOTSTRAP_FILESYSTEM_H
#define VITTE_BOOTSTRAP_FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "../api/error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_FS_MAX_PATH ((size_t)4096u)
#define VITTE_FS_MAX_EXT ((size_t)64u)
#define VITTE_FS_READ_CHUNK_SIZE ((size_t)8192u)
#define VITTE_FS_DEFAULT_MAX_FILE_BYTES ((size_t)64u * 1024u * 1024u)

typedef struct vitte_fs_path {
    char text[VITTE_FS_MAX_PATH];
    size_t length;
} vitte_fs_path_t;

typedef enum vitte_fs_entry_kind {
    VITTE_FS_ENTRY_MISSING = 0,
    VITTE_FS_ENTRY_FILE,
    VITTE_FS_ENTRY_DIRECTORY,
    VITTE_FS_ENTRY_OTHER
} vitte_fs_entry_kind_t;

typedef struct vitte_fs_entry {
    vitte_fs_path_t path;
    vitte_fs_entry_kind_t kind;
    size_t size_bytes;
    time_t modified_time;
    bool exists;
} vitte_fs_entry_t;

typedef struct vitte_fs_stats {
    size_t bytes_read;
    size_t bytes_written;
    size_t files_opened;
    size_t directories_created;
    size_t failed_operations;
} vitte_fs_stats_t;

typedef struct vitte_fs_options {
    size_t max_file_bytes;
    bool null_terminate_reads;
    bool create_parent_directories;
} vitte_fs_options_t;

typedef struct vitte_fs_result {
    vitte_status_t status;
    size_t bytes;
    vitte_fs_entry_kind_t kind;
    vitte_error_t error;
} vitte_fs_result_t;

typedef struct vitte_fs_reader {
    FILE *stream;
    vitte_fs_path_t path;
    bool open;
    size_t bytes_read;
    vitte_error_t last_error;
} vitte_fs_reader_t;

typedef enum vitte_fs_writer_mode {
    VITTE_FS_WRITER_TRUNCATE = 0,
    VITTE_FS_WRITER_APPEND
} vitte_fs_writer_mode_t;

typedef struct vitte_fs_writer {
    FILE *stream;
    vitte_fs_path_t path;
    vitte_fs_writer_mode_t mode;
    bool open;
    size_t bytes_written;
    vitte_error_t last_error;
} vitte_fs_writer_t;

void vitte_fs_options_init(vitte_fs_options_t *options);
void vitte_fs_result_init(vitte_fs_result_t *result);
void vitte_fs_stats_init(vitte_fs_stats_t *stats);

void vitte_fs_path_init(vitte_fs_path_t *path);
vitte_status_t vitte_fs_path_from_cstr(vitte_fs_path_t *path, const char *text);
vitte_status_t vitte_fs_path_copy(vitte_fs_path_t *destination, const vitte_fs_path_t *source);
vitte_status_t vitte_fs_path_join(vitte_fs_path_t *out, const vitte_fs_path_t *left, const char *right);
vitte_status_t vitte_fs_path_normalize(vitte_fs_path_t *path);
vitte_status_t vitte_fs_path_basename(const vitte_fs_path_t *path, vitte_fs_path_t *out);
vitte_status_t vitte_fs_path_dirname(const vitte_fs_path_t *path, vitte_fs_path_t *out);
vitte_status_t vitte_fs_path_extension(const vitte_fs_path_t *path, char *buffer, size_t capacity);
vitte_status_t vitte_fs_path_replace_extension(vitte_fs_path_t *path, const char *extension);
bool vitte_fs_path_is_absolute(const vitte_fs_path_t *path);
bool vitte_fs_path_is_relative(const vitte_fs_path_t *path);
bool vitte_fs_path_has_extension(const vitte_fs_path_t *path, const char *extension);

const char *vitte_fs_entry_kind_name(vitte_fs_entry_kind_t kind);
vitte_status_t vitte_fs_stat(const char *path, vitte_fs_entry_t *entry, vitte_error_t *error);
bool vitte_fs_exists(const char *path);
bool vitte_fs_is_file(const char *path);
bool vitte_fs_is_directory(const char *path);
vitte_status_t vitte_fs_size_bytes(const char *path, size_t *size, vitte_error_t *error);

vitte_status_t vitte_fs_read_all(
    const char *path,
    char *buffer,
    size_t capacity,
    size_t max_bytes,
    size_t *bytes_read,
    vitte_error_t *error
);
vitte_status_t vitte_fs_read_all_alloc(
    const char *path,
    char **buffer,
    size_t *bytes_read,
    const vitte_fs_options_t *options,
    vitte_error_t *error
);
void vitte_fs_free(void *pointer);

vitte_status_t vitte_fs_write_all(
    const char *path,
    const void *data,
    size_t size,
    size_t *bytes_written,
    vitte_error_t *error
);
vitte_status_t vitte_fs_append_all(
    const char *path,
    const void *data,
    size_t size,
    size_t *bytes_written,
    vitte_error_t *error
);

vitte_status_t vitte_fs_create_directory(const char *path, vitte_error_t *error);
vitte_status_t vitte_fs_create_directories(const char *path, vitte_error_t *error);
vitte_status_t vitte_fs_parent_path(const char *path, vitte_fs_path_t *out);
vitte_status_t vitte_fs_temp_dir(vitte_fs_path_t *out);
vitte_status_t vitte_fs_temp_file_path(vitte_fs_path_t *out, const char *prefix, const char *suffix);

vitte_status_t vitte_fs_reader_open(vitte_fs_reader_t *reader, const char *path);
vitte_status_t vitte_fs_reader_read(
    vitte_fs_reader_t *reader,
    void *buffer,
    size_t capacity,
    size_t *bytes_read
);
vitte_status_t vitte_fs_reader_close(vitte_fs_reader_t *reader);

vitte_status_t vitte_fs_writer_open(
    vitte_fs_writer_t *writer,
    const char *path,
    vitte_fs_writer_mode_t mode
);
vitte_status_t vitte_fs_writer_write(
    vitte_fs_writer_t *writer,
    const void *data,
    size_t size,
    size_t *bytes_written
);
vitte_status_t vitte_fs_writer_close(vitte_fs_writer_t *writer);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_FILESYSTEM_H */
