#include "filesystem.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define VITTE_FS_MKDIR(path) _mkdir(path)
#else
#define VITTE_FS_MKDIR(path) mkdir((path), 0777)
#endif

static void vitte_fs_set_error(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (error != NULL) {
        vitte_error_set_details(error, status, code, message, details);
    }
}

static bool vitte_fs_is_separator(char value) {
    return value == '/' || value == '\\';
}

static bool vitte_fs_text_is_valid_path(const char *text) {
    size_t length;

    if (text == NULL || text[0] == '\0') {
        return false;
    }
    length = strlen(text);
    return length < VITTE_FS_MAX_PATH;
}

void vitte_fs_options_init(vitte_fs_options_t *options) {
    if (options == NULL) {
        return;
    }
    memset(options, 0, sizeof(*options));
    options->max_file_bytes = VITTE_FS_DEFAULT_MAX_FILE_BYTES;
    options->null_terminate_reads = true;
}

void vitte_fs_result_init(vitte_fs_result_t *result) {
    if (result == NULL) {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
    result->kind = VITTE_FS_ENTRY_MISSING;
    vitte_error_init(&result->error);
}

void vitte_fs_stats_init(vitte_fs_stats_t *stats) {
    if (stats == NULL) {
        return;
    }
    memset(stats, 0, sizeof(*stats));
}

void vitte_fs_path_init(vitte_fs_path_t *path) {
    if (path == NULL) {
        return;
    }
    path->text[0] = '\0';
    path->length = 0u;
}

vitte_status_t vitte_fs_path_from_cstr(vitte_fs_path_t *path, const char *text) {
    size_t length;

    if (path == NULL || text == NULL || text[0] == '\0') {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    length = strlen(text);
    if (length >= VITTE_FS_MAX_PATH) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    (void)memcpy(path->text, text, length + 1u);
    path->length = length;
    return vitte_fs_path_normalize(path);
}

vitte_status_t vitte_fs_path_copy(vitte_fs_path_t *destination, const vitte_fs_path_t *source) {
    if (destination == NULL || source == NULL || source->length >= VITTE_FS_MAX_PATH) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    (void)memcpy(destination->text, source->text, source->length + 1u);
    destination->length = source->length;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_path_normalize(vitte_fs_path_t *path) {
    size_t read_index;
    size_t write_index = 0u;
    bool previous_separator = false;

    if (path == NULL || path->length >= VITTE_FS_MAX_PATH) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    for (read_index = 0u; read_index < path->length; read_index++) {
        char value = path->text[read_index];
        if (vitte_fs_is_separator(value)) {
            value = '/';
            if (previous_separator && write_index > 1u) {
                continue;
            }
            previous_separator = true;
        } else {
            previous_separator = false;
        }
        path->text[write_index] = value;
        write_index++;
    }
    if (write_index > 1u && path->text[write_index - 1u] == '/') {
        write_index--;
    }
    path->text[write_index] = '\0';
    path->length = write_index;
    return path->length > 0u ? VITTE_STATUS_OK : VITTE_STATUS_ERROR_INVALID_ARGUMENT;
}

vitte_status_t vitte_fs_path_join(vitte_fs_path_t *out, const vitte_fs_path_t *left, const char *right) {
    size_t left_length;
    size_t right_length;
    bool needs_separator;

    if (out == NULL || left == NULL || right == NULL || right[0] == '\0') {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    left_length = left->length;
    right_length = strlen(right);
    needs_separator = left_length > 0u && !vitte_fs_is_separator(left->text[left_length - 1u]);
    while (right_length > 0u && vitte_fs_is_separator(*right)) {
        right++;
        right_length--;
    }
    if (left_length + (needs_separator ? 1u : 0u) + right_length >= VITTE_FS_MAX_PATH) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_fs_path_init(out);
    (void)memcpy(out->text, left->text, left_length);
    out->length = left_length;
    if (needs_separator) {
        out->text[out->length] = '/';
        out->length++;
    }
    (void)memcpy(out->text + out->length, right, right_length);
    out->length += right_length;
    out->text[out->length] = '\0';
    return vitte_fs_path_normalize(out);
}

static size_t vitte_fs_last_separator_index(const vitte_fs_path_t *path, bool *found) {
    size_t index;

    if (found != NULL) {
        *found = false;
    }
    if (path == NULL || path->length == 0u) {
        return 0u;
    }
    for (index = path->length; index > 0u; index--) {
        if (vitte_fs_is_separator(path->text[index - 1u])) {
            if (found != NULL) {
                *found = true;
            }
            return index - 1u;
        }
    }
    return 0u;
}

vitte_status_t vitte_fs_path_basename(const vitte_fs_path_t *path, vitte_fs_path_t *out) {
    bool found;
    size_t separator;
    const char *base;

    if (path == NULL || out == NULL || path->length == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    separator = vitte_fs_last_separator_index(path, &found);
    base = found ? path->text + separator + 1u : path->text;
    if (base[0] == '\0') {
        base = "/";
    }
    return vitte_fs_path_from_cstr(out, base);
}

vitte_status_t vitte_fs_path_dirname(const vitte_fs_path_t *path, vitte_fs_path_t *out) {
    bool found;
    size_t separator;

    if (path == NULL || out == NULL || path->length == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    separator = vitte_fs_last_separator_index(path, &found);
    if (!found) {
        return vitte_fs_path_from_cstr(out, ".");
    }
    if (separator == 0u) {
        return vitte_fs_path_from_cstr(out, "/");
    }
    if (separator >= VITTE_FS_MAX_PATH) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    (void)memcpy(out->text, path->text, separator);
    out->text[separator] = '\0';
    out->length = separator;
    return vitte_fs_path_normalize(out);
}

static const char *vitte_fs_extension_start(const vitte_fs_path_t *path) {
    size_t index;
    size_t start = 0u;

    if (path == NULL || path->length == 0u) {
        return NULL;
    }
    for (index = path->length; index > 0u; index--) {
        if (vitte_fs_is_separator(path->text[index - 1u])) {
            start = index;
            break;
        }
    }
    for (index = path->length; index > start; index--) {
        if (path->text[index - 1u] == '.') {
            if (index - 1u == start) {
                return NULL;
            }
            return path->text + index - 1u;
        }
    }
    return NULL;
}

vitte_status_t vitte_fs_path_extension(const vitte_fs_path_t *path, char *buffer, size_t capacity) {
    const char *extension;
    size_t length;

    if (path == NULL || buffer == NULL || capacity == 0u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    buffer[0] = '\0';
    extension = vitte_fs_extension_start(path);
    if (extension == NULL) {
        return VITTE_STATUS_OK;
    }
    length = strlen(extension);
    if (length >= capacity || length >= VITTE_FS_MAX_EXT) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    (void)memcpy(buffer, extension, length + 1u);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_path_replace_extension(vitte_fs_path_t *path, const char *extension) {
    const char *old_extension;
    size_t base_length;
    size_t extension_length;
    bool needs_dot;

    if (path == NULL || extension == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (extension[0] == '\0') {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    old_extension = vitte_fs_extension_start(path);
    base_length = old_extension != NULL ? (size_t)(old_extension - path->text) : path->length;
    extension_length = strlen(extension);
    needs_dot = extension[0] != '.';
    if (extension_length + (needs_dot ? 1u : 0u) >= VITTE_FS_MAX_EXT ||
        base_length + extension_length + (needs_dot ? 1u : 0u) >= VITTE_FS_MAX_PATH) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    path->length = base_length;
    if (needs_dot) {
        path->text[path->length] = '.';
        path->length++;
    }
    (void)memcpy(path->text + path->length, extension, extension_length);
    path->length += extension_length;
    path->text[path->length] = '\0';
    return VITTE_STATUS_OK;
}

bool vitte_fs_path_is_absolute(const vitte_fs_path_t *path) {
    if (path == NULL || path->length == 0u) {
        return false;
    }
    if (vitte_fs_is_separator(path->text[0])) {
        return true;
    }
    return path->length > 2u &&
        ((path->text[0] >= 'A' && path->text[0] <= 'Z') ||
            (path->text[0] >= 'a' && path->text[0] <= 'z')) &&
        path->text[1] == ':' &&
        vitte_fs_is_separator(path->text[2]);
}

bool vitte_fs_path_is_relative(const vitte_fs_path_t *path) {
    return path != NULL && path->length > 0u && !vitte_fs_path_is_absolute(path);
}

bool vitte_fs_path_has_extension(const vitte_fs_path_t *path, const char *extension) {
    const char *actual;

    if (path == NULL || extension == NULL) {
        return false;
    }
    actual = vitte_fs_extension_start(path);
    if (actual == NULL) {
        return extension[0] == '\0';
    }
    if (extension[0] == '.') {
        return strcmp(actual, extension) == 0;
    }
    return strcmp(actual + 1u, extension) == 0;
}

const char *vitte_fs_entry_kind_name(vitte_fs_entry_kind_t kind) {
    switch (kind) {
        case VITTE_FS_ENTRY_MISSING:
            return "missing";
        case VITTE_FS_ENTRY_FILE:
            return "file";
        case VITTE_FS_ENTRY_DIRECTORY:
            return "directory";
        case VITTE_FS_ENTRY_OTHER:
            return "other";
        default:
            return "unknown";
    }
}

static vitte_fs_entry_kind_t vitte_fs_kind_from_mode(unsigned mode) {
#ifdef _WIN32
    if ((mode & _S_IFDIR) != 0u) {
        return VITTE_FS_ENTRY_DIRECTORY;
    }
    if ((mode & _S_IFREG) != 0u) {
        return VITTE_FS_ENTRY_FILE;
    }
#else
    if (S_ISDIR(mode)) {
        return VITTE_FS_ENTRY_DIRECTORY;
    }
    if (S_ISREG(mode)) {
        return VITTE_FS_ENTRY_FILE;
    }
#endif
    return VITTE_FS_ENTRY_OTHER;
}

vitte_status_t vitte_fs_stat(const char *path, vitte_fs_entry_t *entry, vitte_error_t *error) {
    struct stat info;

    if (entry != NULL) {
        memset(entry, 0, sizeof(*entry));
        entry->kind = VITTE_FS_ENTRY_MISSING;
        if (path != NULL && path[0] != '\0') {
            (void)vitte_fs_path_from_cstr(&entry->path, path);
        }
    }
    if (!vitte_fs_text_is_valid_path(path)) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_PATH", "invalid filesystem path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (stat(path, &info) != 0) {
        if (errno == ENOENT || errno == ENOTDIR) {
            if (entry != NULL) {
                entry->exists = false;
                entry->kind = VITTE_FS_ENTRY_MISSING;
            }
            vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_OPEN", "filesystem entry does not exist", path);
            return VITTE_STATUS_ERROR_IO;
        }
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_OPEN", "failed to stat filesystem entry", path);
        return VITTE_STATUS_ERROR_IO;
    }

    if (entry != NULL) {
        entry->exists = true;
        entry->kind = vitte_fs_kind_from_mode((unsigned)info.st_mode);
        entry->size_bytes = info.st_size < 0 ? 0u : (size_t)info.st_size;
        entry->modified_time = info.st_mtime;
    }
    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}

bool vitte_fs_exists(const char *path) {
    vitte_fs_entry_t entry;
    return vitte_fs_stat(path, &entry, NULL) == VITTE_STATUS_OK && entry.exists;
}

bool vitte_fs_is_file(const char *path) {
    vitte_fs_entry_t entry;
    return vitte_fs_stat(path, &entry, NULL) == VITTE_STATUS_OK && entry.kind == VITTE_FS_ENTRY_FILE;
}

bool vitte_fs_is_directory(const char *path) {
    vitte_fs_entry_t entry;
    return vitte_fs_stat(path, &entry, NULL) == VITTE_STATUS_OK && entry.kind == VITTE_FS_ENTRY_DIRECTORY;
}

vitte_status_t vitte_fs_size_bytes(const char *path, size_t *size, vitte_error_t *error) {
    vitte_fs_entry_t entry;
    vitte_status_t status;

    if (size == NULL) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_ARGUMENT", "missing size output", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    *size = 0u;
    status = vitte_fs_stat(path, &entry, error);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if (entry.kind != VITTE_FS_ENTRY_FILE) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_PATH", "filesystem path is not a file", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    *size = entry.size_bytes;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_fs_file_size(FILE *stream, size_t *size, vitte_error_t *error) {
    long end;

    if (stream == NULL || size == NULL) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_ARGUMENT", "missing stream or size output", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (fseek(stream, 0L, SEEK_END) != 0) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_READ", "failed to seek file", NULL);
        return VITTE_STATUS_ERROR_IO;
    }
    end = ftell(stream);
    if (end < 0L) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_READ", "failed to tell file size", NULL);
        return VITTE_STATUS_ERROR_IO;
    }
    if (fseek(stream, 0L, SEEK_SET) != 0) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_READ", "failed to rewind file", NULL);
        return VITTE_STATUS_ERROR_IO;
    }
    *size = (size_t)end;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_read_all(
    const char *path,
    char *buffer,
    size_t capacity,
    size_t max_bytes,
    size_t *bytes_read,
    vitte_error_t *error
) {
    FILE *stream;
    size_t size;
    size_t read_count;
    vitte_status_t status;

    if (bytes_read != NULL) {
        *bytes_read = 0u;
    }
    if (!vitte_fs_text_is_valid_path(path) || buffer == NULL || capacity == 0u) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_ARGUMENT", "invalid read arguments", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    stream = fopen(path, "rb");
    if (stream == NULL) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_OPEN", "failed to open file for reading", path);
        return VITTE_STATUS_ERROR_IO;
    }
    status = vitte_fs_file_size(stream, &size, error);
    if (status != VITTE_STATUS_OK) {
        (void)fclose(stream);
        return status;
    }
    if ((max_bytes != 0u && size > max_bytes) || size + 1u > capacity) {
        (void)fclose(stream);
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_TOO_LARGE", "file exceeds read buffer or limit", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    read_count = size > 0u ? fread(buffer, 1u, size, stream) : 0u;
    if (read_count != size || ferror(stream) != 0) {
        (void)fclose(stream);
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_READ", "failed to read complete file", path);
        return VITTE_STATUS_ERROR_IO;
    }
    buffer[size] = '\0';
    if (fclose(stream) != 0) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_CLOSE", "failed to close read file", path);
        return VITTE_STATUS_ERROR_IO;
    }
    if (bytes_read != NULL) {
        *bytes_read = size;
    }
    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_read_all_alloc(
    const char *path,
    char **buffer,
    size_t *bytes_read,
    const vitte_fs_options_t *options,
    vitte_error_t *error
) {
    vitte_fs_options_t defaults;
    const vitte_fs_options_t *effective_options = options;
    FILE *stream;
    char *data;
    size_t size;
    size_t allocation_size;
    size_t read_count;
    vitte_status_t status;

    if (buffer != NULL) {
        *buffer = NULL;
    }
    if (bytes_read != NULL) {
        *bytes_read = 0u;
    }
    if (!vitte_fs_text_is_valid_path(path) || buffer == NULL) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_ARGUMENT", "invalid allocated read arguments", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (effective_options == NULL) {
        vitte_fs_options_init(&defaults);
        effective_options = &defaults;
    }

    stream = fopen(path, "rb");
    if (stream == NULL) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_OPEN", "failed to open file for reading", path);
        return VITTE_STATUS_ERROR_IO;
    }
    status = vitte_fs_file_size(stream, &size, error);
    if (status != VITTE_STATUS_OK) {
        (void)fclose(stream);
        return status;
    }
    if (effective_options->max_file_bytes != 0u && size > effective_options->max_file_bytes) {
        (void)fclose(stream);
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_TOO_LARGE", "file exceeds configured maximum size", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    allocation_size = size + (effective_options->null_terminate_reads ? 1u : 0u);
    if (allocation_size < size || allocation_size == 0u) {
        allocation_size = size;
    }
    data = (char *)malloc(allocation_size);
    if (data == NULL) {
        (void)fclose(stream);
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_FS_E_ALLOC", "failed to allocate file buffer", path);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }
    read_count = size > 0u ? fread(data, 1u, size, stream) : 0u;
    if (read_count != size || ferror(stream) != 0) {
        free(data);
        (void)fclose(stream);
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_READ", "failed to read complete file", path);
        return VITTE_STATUS_ERROR_IO;
    }
    if (effective_options->null_terminate_reads) {
        data[size] = '\0';
    }
    if (fclose(stream) != 0) {
        free(data);
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_CLOSE", "failed to close read file", path);
        return VITTE_STATUS_ERROR_IO;
    }
    *buffer = data;
    if (bytes_read != NULL) {
        *bytes_read = size;
    }
    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}

void vitte_fs_free(void *pointer) {
    free(pointer);
}

static vitte_status_t vitte_fs_write_all_mode(
    const char *path,
    const void *data,
    size_t size,
    const char *mode,
    size_t *bytes_written,
    vitte_error_t *error
) {
    FILE *stream;
    size_t written;

    if (bytes_written != NULL) {
        *bytes_written = 0u;
    }
    if (!vitte_fs_text_is_valid_path(path) || (data == NULL && size != 0u) || mode == NULL) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_ARGUMENT", "invalid write arguments", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    stream = fopen(path, mode);
    if (stream == NULL) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_OPEN", "failed to open file for writing", path);
        return VITTE_STATUS_ERROR_IO;
    }
    written = size > 0u ? fwrite(data, 1u, size, stream) : 0u;
    if (written != size || ferror(stream) != 0) {
        (void)fclose(stream);
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_WRITE", "failed to write complete file", path);
        return VITTE_STATUS_ERROR_IO;
    }
    if (fflush(stream) != 0) {
        (void)fclose(stream);
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_WRITE", "failed to flush file", path);
        return VITTE_STATUS_ERROR_IO;
    }
    if (fclose(stream) != 0) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_CLOSE", "failed to close write file", path);
        return VITTE_STATUS_ERROR_IO;
    }
    if (bytes_written != NULL) {
        *bytes_written = written;
    }
    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_write_all(
    const char *path,
    const void *data,
    size_t size,
    size_t *bytes_written,
    vitte_error_t *error
) {
    return vitte_fs_write_all_mode(path, data, size, "wb", bytes_written, error);
}

vitte_status_t vitte_fs_append_all(
    const char *path,
    const void *data,
    size_t size,
    size_t *bytes_written,
    vitte_error_t *error
) {
    return vitte_fs_write_all_mode(path, data, size, "ab", bytes_written, error);
}

vitte_status_t vitte_fs_create_directory(const char *path, vitte_error_t *error) {
    if (!vitte_fs_text_is_valid_path(path)) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_PATH", "invalid directory path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (vitte_fs_is_directory(path)) {
        if (error != NULL) {
            vitte_error_reset(error);
        }
        return VITTE_STATUS_OK;
    }
    if (VITTE_FS_MKDIR(path) != 0) {
        if (errno == EEXIST && vitte_fs_is_directory(path)) {
            if (error != NULL) {
                vitte_error_reset(error);
            }
            return VITTE_STATUS_OK;
        }
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_MKDIR", "failed to create directory", path);
        return VITTE_STATUS_ERROR_IO;
    }
    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_create_directories(const char *path, vitte_error_t *error) {
    vitte_fs_path_t current;
    size_t index;

    if (!vitte_fs_text_is_valid_path(path)) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_PATH", "invalid directory path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (vitte_fs_path_from_cstr(&current, path) != VITTE_STATUS_OK) {
        vitte_fs_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_PATH", "invalid directory path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    for (index = 1u; index <= current.length; index++) {
        char saved;
        if (index < current.length && current.text[index] != '/') {
            continue;
        }
        saved = current.text[index];
        current.text[index] = '\0';
        if (current.text[0] != '\0' && strcmp(current.text, "/") != 0) {
            vitte_status_t status = vitte_fs_create_directory(current.text, error);
            if (status != VITTE_STATUS_OK) {
                current.text[index] = saved;
                return status;
            }
        }
        current.text[index] = saved;
    }
    return vitte_fs_is_directory(path) ? VITTE_STATUS_OK : VITTE_STATUS_ERROR_IO;
}

vitte_status_t vitte_fs_parent_path(const char *path, vitte_fs_path_t *out) {
    vitte_fs_path_t fs_path;
    vitte_status_t status;

    status = vitte_fs_path_from_cstr(&fs_path, path);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    return vitte_fs_path_dirname(&fs_path, out);
}

vitte_status_t vitte_fs_temp_dir(vitte_fs_path_t *out) {
    const char *tmp = getenv("TMPDIR");

    if (tmp == NULL || tmp[0] == '\0') {
        tmp = "/tmp";
    }
    return vitte_fs_path_from_cstr(out, tmp);
}

vitte_status_t vitte_fs_temp_file_path(vitte_fs_path_t *out, const char *prefix, const char *suffix) {
    vitte_fs_path_t dir;
    char name[256];
    int written;

    if (out == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (prefix == NULL) {
        prefix = "vitte";
    }
    if (suffix == NULL) {
        suffix = ".tmp";
    }
    if (strlen(prefix) > 64u || strlen(suffix) > 64u) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    written = snprintf(name, sizeof(name), "%s-%ld%s", prefix, (long)time(NULL), suffix);
    if (written < 0 || (size_t)written >= sizeof(name)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (vitte_fs_temp_dir(&dir) != VITTE_STATUS_OK) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_fs_path_join(out, &dir, name);
}

vitte_status_t vitte_fs_reader_open(vitte_fs_reader_t *reader, const char *path) {
    if (reader == NULL || !vitte_fs_text_is_valid_path(path)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(reader, 0, sizeof(*reader));
    vitte_error_init(&reader->last_error);
    if (vitte_fs_path_from_cstr(&reader->path, path) != VITTE_STATUS_OK) {
        vitte_fs_set_error(&reader->last_error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_PATH", "invalid reader path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    reader->stream = fopen(path, "rb");
    if (reader->stream == NULL) {
        vitte_fs_set_error(&reader->last_error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_OPEN", "failed to open reader", path);
        return VITTE_STATUS_ERROR_IO;
    }
    reader->open = true;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_reader_read(
    vitte_fs_reader_t *reader,
    void *buffer,
    size_t capacity,
    size_t *bytes_read
) {
    size_t count;

    if (bytes_read != NULL) {
        *bytes_read = 0u;
    }
    if (reader == NULL || !reader->open || reader->stream == NULL || buffer == NULL) {
        if (reader != NULL) {
            vitte_fs_set_error(&reader->last_error, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_FS_E_READ", "reader is not open", NULL);
        }
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    count = capacity > 0u ? fread(buffer, 1u, capacity, reader->stream) : 0u;
    if (ferror(reader->stream) != 0) {
        vitte_fs_set_error(&reader->last_error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_READ", "failed to read from reader", reader->path.text);
        return VITTE_STATUS_ERROR_IO;
    }
    reader->bytes_read += count;
    if (bytes_read != NULL) {
        *bytes_read = count;
    }
    vitte_error_reset(&reader->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_reader_close(vitte_fs_reader_t *reader) {
    if (reader == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!reader->open) {
        return VITTE_STATUS_OK;
    }
    if (reader->stream != NULL && fclose(reader->stream) != 0) {
        reader->stream = NULL;
        reader->open = false;
        vitte_fs_set_error(&reader->last_error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_CLOSE", "failed to close reader", reader->path.text);
        return VITTE_STATUS_ERROR_IO;
    }
    reader->stream = NULL;
    reader->open = false;
    vitte_error_reset(&reader->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_writer_open(
    vitte_fs_writer_t *writer,
    const char *path,
    vitte_fs_writer_mode_t mode
) {
    const char *fopen_mode;

    if (writer == NULL || !vitte_fs_text_is_valid_path(path) ||
        (mode != VITTE_FS_WRITER_TRUNCATE && mode != VITTE_FS_WRITER_APPEND)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(writer, 0, sizeof(*writer));
    vitte_error_init(&writer->last_error);
    if (vitte_fs_path_from_cstr(&writer->path, path) != VITTE_STATUS_OK) {
        vitte_fs_set_error(&writer->last_error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_FS_E_PATH", "invalid writer path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    writer->mode = mode;
    fopen_mode = mode == VITTE_FS_WRITER_APPEND ? "ab" : "wb";
    writer->stream = fopen(path, fopen_mode);
    if (writer->stream == NULL) {
        vitte_fs_set_error(&writer->last_error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_OPEN", "failed to open writer", path);
        return VITTE_STATUS_ERROR_IO;
    }
    writer->open = true;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_writer_write(
    vitte_fs_writer_t *writer,
    const void *data,
    size_t size,
    size_t *bytes_written
) {
    size_t count;

    if (bytes_written != NULL) {
        *bytes_written = 0u;
    }
    if (writer == NULL || !writer->open || writer->stream == NULL || (data == NULL && size != 0u)) {
        if (writer != NULL) {
            vitte_fs_set_error(&writer->last_error, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_FS_E_WRITE", "writer is not open", NULL);
        }
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    count = size > 0u ? fwrite(data, 1u, size, writer->stream) : 0u;
    if (count != size || ferror(writer->stream) != 0) {
        vitte_fs_set_error(&writer->last_error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_WRITE", "failed to write complete buffer", writer->path.text);
        return VITTE_STATUS_ERROR_IO;
    }
    writer->bytes_written += count;
    if (bytes_written != NULL) {
        *bytes_written = count;
    }
    vitte_error_reset(&writer->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_fs_writer_close(vitte_fs_writer_t *writer) {
    if (writer == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!writer->open) {
        return VITTE_STATUS_OK;
    }
    if (writer->stream != NULL && fflush(writer->stream) != 0) {
        (void)fclose(writer->stream);
        writer->stream = NULL;
        writer->open = false;
        vitte_fs_set_error(&writer->last_error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_WRITE", "failed to flush writer", writer->path.text);
        return VITTE_STATUS_ERROR_IO;
    }
    if (writer->stream != NULL && fclose(writer->stream) != 0) {
        writer->stream = NULL;
        writer->open = false;
        vitte_fs_set_error(&writer->last_error, VITTE_STATUS_ERROR_IO, "VITTE_FS_E_CLOSE", "failed to close writer", writer->path.text);
        return VITTE_STATUS_ERROR_IO;
    }
    writer->stream = NULL;
    writer->open = false;
    vitte_error_reset(&writer->last_error);
    return VITTE_STATUS_OK;
}
