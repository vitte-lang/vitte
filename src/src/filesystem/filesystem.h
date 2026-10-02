#ifndef VITTE_FILESYSTEM_FILESYSTEM_H
#define VITTE_FILESYSTEM_FILESYSTEM_H

/*
 * Vitte Compiler
 * src/filesystem/filesystem.h
 *
 * Public filesystem abstraction.
 *
 * This interface provides the compiler with a small, deterministic and
 * ownership-explicit filesystem layer.
 *
 * Supported operations:
 *
 *   - metadata queries;
 *   - existence/type queries;
 *   - bounded whole-file reads;
 *   - complete whole-file writes;
 *   - atomic replacement writes;
 *   - directory creation;
 *   - recursive directory creation;
 *   - file/directory removal;
 *   - rename;
 *   - deterministic directory enumeration;
 *   - path joining;
 *   - lexical path normalization;
 *   - canonical path resolution;
 *   - current working directory;
 *   - stable path hashing;
 *   - filesystem statistics.
 *
 * Ownership
 * ---------
 *
 * Functions returning char * allocate with malloc().
 * The caller owns the returned string and releases it with free().
 *
 * vitte_filesystem_read_file() owns the memory stored in its output buffer.
 * Release it with vitte_filesystem_buffer_destroy().
 *
 * vitte_filesystem_list_directory() owns all entry names in its output list.
 * Release them with vitte_filesystem_directory_list_destroy().
 *
 * Threading
 * ---------
 *
 * Filesystem operations themselves do not retain per-call mutable state.
 *
 * Statistics are process-global and are not synchronized. Concurrent access
 * to the statistics API therefore requires external synchronization.
 *
 * Platform
 * --------
 *
 * The current implementation targets POSIX-compatible systems, including
 * macOS and Linux.
 *
 * Language: ISO C17 + POSIX filesystem interfaces.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

#define VITTE_FILESYSTEM_API_VERSION_MAJOR 1u
#define VITTE_FILESYSTEM_API_VERSION_MINOR 0u
#define VITTE_FILESYSTEM_API_VERSION_PATCH 0u

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

/*
 * Default upper bound used by vitte_filesystem_read_file() when max_bytes is
 * zero.
 *
 * 1 GiB.
 */
#define VITTE_FILESYSTEM_DEFAULT_MAX_FILE_BYTES \
    ((size_t)(1024u * 1024u * 1024u))

/*
 * Compiler-side safety limit for path strings.
 *
 * This is deliberately independent of the host PATH_MAX because:
 *
 *   - PATH_MAX is not universally meaningful;
 *   - some filesystems support dynamically sized paths;
 *   - compiler code benefits from an explicit allocation limit.
 *
 * 1 MiB.
 */
#define VITTE_FILESYSTEM_DEFAULT_MAX_PATH_BYTES \
    ((size_t)(1024u * 1024u))

/*
 * Default maximum number of entries accepted by directory enumeration when
 * max_entries == 0.
 */
#define VITTE_FILESYSTEM_DEFAULT_MAX_DIRECTORY_ENTRIES \
    ((size_t)1000000u)

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_filesystem_error {
    VITTE_FILESYSTEM_ERROR_NONE = 0,

    VITTE_FILESYSTEM_ERROR_INVALID_ARGUMENT,
    VITTE_FILESYSTEM_ERROR_INVALID_PATH,

    VITTE_FILESYSTEM_ERROR_NOT_FOUND,
    VITTE_FILESYSTEM_ERROR_ALREADY_EXISTS,
    VITTE_FILESYSTEM_ERROR_NOT_FILE,
    VITTE_FILESYSTEM_ERROR_NOT_DIRECTORY,
    VITTE_FILESYSTEM_ERROR_DIRECTORY_NOT_EMPTY,

    VITTE_FILESYSTEM_ERROR_PERMISSION_DENIED,
    VITTE_FILESYSTEM_ERROR_READ_ONLY,

    VITTE_FILESYSTEM_ERROR_OPEN,
    VITTE_FILESYSTEM_ERROR_READ,
    VITTE_FILESYSTEM_ERROR_WRITE,
    VITTE_FILESYSTEM_ERROR_FLUSH,
    VITTE_FILESYSTEM_ERROR_CLOSE,

    VITTE_FILESYSTEM_ERROR_STAT,
    VITTE_FILESYSTEM_ERROR_MKDIR,
    VITTE_FILESYSTEM_ERROR_REMOVE,
    VITTE_FILESYSTEM_ERROR_RENAME,
    VITTE_FILESYSTEM_ERROR_DIRECTORY_READ,
    VITTE_FILESYSTEM_ERROR_CURRENT_DIRECTORY,
    VITTE_FILESYSTEM_ERROR_CANONICALIZE,

    VITTE_FILESYSTEM_ERROR_TOO_LARGE,
    VITTE_FILESYSTEM_ERROR_TOO_MANY_ENTRIES,
    VITTE_FILESYSTEM_ERROR_PATH_TOO_LONG,

    VITTE_FILESYSTEM_ERROR_OVERFLOW,
    VITTE_FILESYSTEM_ERROR_OUT_OF_MEMORY,

    VITTE_FILESYSTEM_ERROR_INTERRUPTED,
    VITTE_FILESYSTEM_ERROR_IO,
    VITTE_FILESYSTEM_ERROR_UNSUPPORTED,

    VITTE_FILESYSTEM_ERROR_COUNT
} vitte_filesystem_error_t;

/* ========================================================================= */
/* File kinds                                                                */
/* ========================================================================= */

typedef enum vitte_filesystem_kind {
    VITTE_FILESYSTEM_KIND_UNKNOWN = 0,

    VITTE_FILESYSTEM_KIND_REGULAR,
    VITTE_FILESYSTEM_KIND_DIRECTORY,
    VITTE_FILESYSTEM_KIND_SYMLINK,
    VITTE_FILESYSTEM_KIND_OTHER,

    VITTE_FILESYSTEM_KIND_COUNT
} vitte_filesystem_kind_t;

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

typedef struct vitte_filesystem_metadata {
    vitte_filesystem_kind_t kind;

    /*
     * File size in bytes.
     *
     * For non-regular files this value should not be interpreted as source
     * content length.
     */
    uint64_t size;

    /*
     * Last modification timestamp.
     *
     * Seconds and nanoseconds are stored separately so callers do not depend
     * on the representation of time_t.
     */
    uint64_t modified_seconds;
    uint64_t modified_nanoseconds;

    /*
     * Best-effort access queries for the current process.
     */
    bool readable;
    bool writable;
    bool executable;
} vitte_filesystem_metadata_t;

/* ========================================================================= */
/* File buffer                                                               */
/* ========================================================================= */

/*
 * Binary-safe owned file buffer.
 *
 * Invariant after a successful read:
 *
 *     data != NULL
 *     length <= capacity
 *     data[length] == '\0'
 *
 * The trailing NUL is convenience storage only. It is NOT included in length
 * and does not imply that file contents are textual.
 */
typedef struct vitte_filesystem_buffer {
    unsigned char *data;
    size_t length;
    size_t capacity;
} vitte_filesystem_buffer_t;

/* ========================================================================= */
/* Directory entries                                                         */
/* ========================================================================= */

typedef struct vitte_filesystem_directory_entry {
    /*
     * Owned entry basename.
     *
     * This is not the complete path.
     */
    char *name;

    /*
     * Determined without following the final symlink.
     *
     * UNKNOWN may be returned when metadata cannot be obtained.
     */
    vitte_filesystem_kind_t kind;
} vitte_filesystem_directory_entry_t;

typedef struct vitte_filesystem_directory_list {
    vitte_filesystem_directory_entry_t *entries;

    size_t count;
    size_t capacity;
} vitte_filesystem_directory_list_t;

/* ========================================================================= */
/* Result                                                                    */
/* ========================================================================= */

/*
 * Every mutating or potentially failing filesystem operation returns a
 * compiler-facing error plus the original host errno value when available.
 *
 * Success:
 *
 *     error        == VITTE_FILESYSTEM_ERROR_NONE
 *     system_error == 0
 */
typedef struct vitte_filesystem_result {
    vitte_filesystem_error_t error;

    /*
     * Host errno captured at the point of failure.
     *
     * Zero means either success or that no meaningful host errno exists for
     * the compiler-side error.
     */
    int system_error;
} vitte_filesystem_result_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_filesystem_stats {
    uint64_t stat_calls;
    uint64_t read_calls;
    uint64_t write_calls;

    uint64_t mkdir_calls;
    uint64_t remove_calls;
    uint64_t rename_calls;
    uint64_t directory_list_calls;

    uint64_t bytes_read;
    uint64_t bytes_written;

    uint64_t allocation_failures;
    uint64_t io_failures;
} vitte_filesystem_stats_t;

/* ========================================================================= */
/* Error / kind names                                                        */
/* ========================================================================= */

/*
 * Returned strings have static storage duration.
 */
const char *
vitte_filesystem_error_name(
    vitte_filesystem_error_t error);

const char *
vitte_filesystem_kind_name(
    vitte_filesystem_kind_t kind);

/* ========================================================================= */
/* Metadata                                                                  */
/* ========================================================================= */

/*
 * Query filesystem metadata.
 *
 * follow_symlink == true:
 *     use stat-like semantics.
 *
 * follow_symlink == false:
 *     use lstat-like semantics.
 */
vitte_filesystem_result_t
vitte_filesystem_stat(
    const char *path,
    bool follow_symlink,
    vitte_filesystem_metadata_t *metadata);

/* ========================================================================= */
/* Existence and type queries                                                */
/* ========================================================================= */

/*
 * Existence check does not follow the final symbolic link.
 *
 * A dangling symbolic link therefore counts as existing.
 */
bool
vitte_filesystem_exists(const char *path);

/*
 * is_file() and is_directory() follow symbolic links.
 */
bool
vitte_filesystem_is_file(const char *path);

bool
vitte_filesystem_is_directory(const char *path);

/*
 * Checks the final path component itself.
 */
bool
vitte_filesystem_is_symlink(const char *path);

/* ========================================================================= */
/* File reading                                                              */
/* ========================================================================= */

/*
 * Read a complete regular file.
 *
 * max_bytes:
 *
 *   0
 *       use VITTE_FILESYSTEM_DEFAULT_MAX_FILE_BYTES.
 *
 *   > 0
 *       reject files larger than this value.
 *
 * On success:
 *
 *   - buffer owns the resulting allocation;
 *   - buffer->length is the exact file size;
 *   - buffer->data[buffer->length] is '\0';
 *   - binary zero bytes inside the file are preserved.
 *
 * On failure the buffer is empty.
 */
vitte_filesystem_result_t
vitte_filesystem_read_file(
    const char *path,
    size_t max_bytes,
    vitte_filesystem_buffer_t *buffer);

/* ========================================================================= */
/* File writing                                                              */
/* ========================================================================= */

/*
 * Write exactly length bytes.
 *
 * Existing contents are truncated.
 *
 * The implementation performs a complete write loop and fsync() before
 * returning success.
 *
 * data may be NULL only when length == 0.
 */
vitte_filesystem_result_t
vitte_filesystem_write_file(
    const char *path,
    const void *data,
    size_t length);

/*
 * Atomically replace path where supported by host rename semantics.
 *
 * Strategy:
 *
 *     create temporary sibling
 *          |
 *          v
 *       write all
 *          |
 *          v
 *        fsync
 *          |
 *          v
 *        close
 *          |
 *          v
 *     rename(temp, path)
 *
 * The temporary file is removed when a pre-rename failure occurs.
 *
 * Atomicity applies to the final rename operation. This API does not promise
 * crash-consistent directory metadata persistence.
 */
vitte_filesystem_result_t
vitte_filesystem_write_file_atomic(
    const char *path,
    const void *data,
    size_t length);

/* ========================================================================= */
/* Directory creation                                                        */
/* ========================================================================= */

/*
 * Create one directory.
 *
 * If path already exists and is a directory, success is returned.
 */
vitte_filesystem_result_t
vitte_filesystem_create_directory(
    const char *path);

/*
 * mkdir -p semantics.
 *
 * Missing parent directories are created from left to right.
 */
vitte_filesystem_result_t
vitte_filesystem_create_directories(
    const char *path);

/* ========================================================================= */
/* Removal                                                                   */
/* ========================================================================= */

/*
 * Remove a non-directory filesystem object.
 *
 * Symbolic links are removed as links and are not followed.
 */
vitte_filesystem_result_t
vitte_filesystem_remove_file(
    const char *path);

/*
 * Remove one empty directory.
 *
 * This operation is deliberately not recursive.
 */
vitte_filesystem_result_t
vitte_filesystem_remove_directory(
    const char *path);

/* ========================================================================= */
/* Rename                                                                    */
/* ========================================================================= */

vitte_filesystem_result_t
vitte_filesystem_rename(
    const char *source,
    const char *destination);

/* ========================================================================= */
/* Directory enumeration                                                     */
/* ========================================================================= */

/*
 * Enumerate a directory.
 *
 * "." and ".." are excluded.
 *
 * Results are sorted lexicographically by raw entry name so compiler behavior
 * does not depend on host directory enumeration order.
 *
 * max_entries:
 *
 *   0
 *       use VITTE_FILESYSTEM_DEFAULT_MAX_DIRECTORY_ENTRIES.
 *
 *   > 0
 *       reject enumeration beyond this count.
 *
 * On success list owns all entries and entry names.
 *
 * On failure list is empty.
 */
vitte_filesystem_result_t
vitte_filesystem_list_directory(
    const char *path,
    size_t max_entries,
    vitte_filesystem_directory_list_t *list);

/* ========================================================================= */
/* Path operations                                                           */
/* ========================================================================= */

/*
 * Join two path fragments.
 *
 * If right is absolute, right replaces left.
 *
 * The result is allocated and owned by the caller.
 *
 * This operation joins strings; it does not access the filesystem.
 */
vitte_filesystem_result_t
vitte_filesystem_join(
    const char *left,
    const char *right,
    char **result);

/*
 * Lexically normalize a path.
 *
 * Operations include:
 *
 *   - collapse repeated '/';
 *   - remove '.';
 *   - resolve '..' where lexically possible;
 *   - preserve leading unresolved '..' in relative paths;
 *   - prevent '..' from escaping above an absolute root.
 *
 * This operation does not access the filesystem and does not resolve symlinks.
 *
 * Empty/fully-collapsed relative paths normalize to ".".
 */
vitte_filesystem_result_t
vitte_filesystem_normalize(
    const char *path,
    char **result);

/*
 * Resolve an existing path to a canonical absolute path.
 *
 * Unlike normalize(), this operation accesses the filesystem and resolves
 * symbolic links.
 */
vitte_filesystem_result_t
vitte_filesystem_canonicalize(
    const char *path,
    char **result);

/*
 * Obtain the current process working directory.
 *
 * The result is allocated and owned by the caller.
 */
vitte_filesystem_result_t
vitte_filesystem_current_directory(
    char **result);

/*
 * POSIX absolute-path test.
 */
bool
vitte_filesystem_path_is_absolute(
    const char *path);

/* ========================================================================= */
/* Deterministic path hash                                                   */
/* ========================================================================= */

/*
 * Stable FNV-1a hash of the path's raw bytes.
 *
 * No normalization or canonicalization is performed automatically.
 *
 * Consequently:
 *
 *     "a/b"
 *
 * and:
 *
 *     "a/./b"
 *
 * intentionally produce different hashes unless normalized first.
 *
 * NULL returns zero.
 */
uint64_t
vitte_filesystem_path_hash(
    const char *path);

/* ========================================================================= */
/* Destruction                                                               */
/* ========================================================================= */

void
vitte_filesystem_buffer_destroy(
    vitte_filesystem_buffer_t *buffer);

void
vitte_filesystem_directory_list_destroy(
    vitte_filesystem_directory_list_t *list);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_filesystem_stats_t
vitte_filesystem_stats(void);

void
vitte_filesystem_stats_reset(void);

/* ========================================================================= */
/* Inline result helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_filesystem_result_is_ok(
    vitte_filesystem_result_t result)
{
    return result.error == VITTE_FILESYSTEM_ERROR_NONE;
}

static inline bool
vitte_filesystem_result_is_error(
    vitte_filesystem_result_t result)
{
    return result.error != VITTE_FILESYSTEM_ERROR_NONE;
}

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_filesystem_error_is_valid(
    vitte_filesystem_error_t error)
{
    return
        error >= VITTE_FILESYSTEM_ERROR_NONE &&
        error < VITTE_FILESYSTEM_ERROR_COUNT;
}

static inline bool
vitte_filesystem_kind_is_valid(
    vitte_filesystem_kind_t kind)
{
    return
        kind >= VITTE_FILESYSTEM_KIND_UNKNOWN &&
        kind < VITTE_FILESYSTEM_KIND_COUNT;
}

/* ========================================================================= */
/* Inline metadata queries                                                   */
/* ========================================================================= */

static inline bool
vitte_filesystem_metadata_is_regular(
    const vitte_filesystem_metadata_t *metadata)
{
    return
        metadata != NULL &&
        metadata->kind == VITTE_FILESYSTEM_KIND_REGULAR;
}

static inline bool
vitte_filesystem_metadata_is_directory(
    const vitte_filesystem_metadata_t *metadata)
{
    return
        metadata != NULL &&
        metadata->kind == VITTE_FILESYSTEM_KIND_DIRECTORY;
}

static inline bool
vitte_filesystem_metadata_is_symlink(
    const vitte_filesystem_metadata_t *metadata)
{
    return
        metadata != NULL &&
        metadata->kind == VITTE_FILESYSTEM_KIND_SYMLINK;
}

/* ========================================================================= */
/* Inline buffer helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_filesystem_buffer_is_empty(
    const vitte_filesystem_buffer_t *buffer)
{
    return
        buffer == NULL ||
        buffer->length == 0u;
}

static inline const unsigned char *
vitte_filesystem_buffer_data(
    const vitte_filesystem_buffer_t *buffer)
{
    if (buffer == NULL) {
        return NULL;
    }

    return buffer->data;
}

static inline size_t
vitte_filesystem_buffer_length(
    const vitte_filesystem_buffer_t *buffer)
{
    if (buffer == NULL) {
        return 0u;
    }

    return buffer->length;
}

/* ========================================================================= */
/* Inline directory helpers                                                  */
/* ========================================================================= */

static inline bool
vitte_filesystem_directory_list_is_empty(
    const vitte_filesystem_directory_list_t *list)
{
    return
        list == NULL ||
        list->count == 0u;
}

static inline size_t
vitte_filesystem_directory_list_count(
    const vitte_filesystem_directory_list_t *list)
{
    if (list == NULL) {
        return 0u;
    }

    return list->count;
}

static inline const vitte_filesystem_directory_entry_t *
vitte_filesystem_directory_list_at(
    const vitte_filesystem_directory_list_t *list,
    size_t index)
{
    if (list == NULL ||
        index >= list->count) {
        return NULL;
    }

    return &list->entries[index];
}

/* ========================================================================= */
/* Empty initializers                                                        */
/* ========================================================================= */

static inline vitte_filesystem_buffer_t
vitte_filesystem_buffer_empty(void)
{
    vitte_filesystem_buffer_t buffer;

    buffer.data = NULL;
    buffer.length = 0u;
    buffer.capacity = 0u;

    return buffer;
}

static inline vitte_filesystem_directory_list_t
vitte_filesystem_directory_list_empty(void)
{
    vitte_filesystem_directory_list_t list;

    list.entries = NULL;
    list.count = 0u;
    list.capacity = 0u;

    return list;
}

static inline vitte_filesystem_metadata_t
vitte_filesystem_metadata_empty(void)
{
    vitte_filesystem_metadata_t metadata;

    metadata.kind = VITTE_FILESYSTEM_KIND_UNKNOWN;
    metadata.size = UINT64_C(0);

    metadata.modified_seconds = UINT64_C(0);
    metadata.modified_nanoseconds = UINT64_C(0);

    metadata.readable = false;
    metadata.writable = false;
    metadata.executable = false;

    return metadata;
}

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(
    VITTE_FILESYSTEM_DEFAULT_MAX_FILE_BYTES > 0u,
    "filesystem maximum file size must be non-zero");

_Static_assert(
    VITTE_FILESYSTEM_DEFAULT_MAX_PATH_BYTES > 0u,
    "filesystem maximum path size must be non-zero");

_Static_assert(
    VITTE_FILESYSTEM_DEFAULT_MAX_DIRECTORY_ENTRIES > 0u,
    "filesystem maximum directory entries must be non-zero");

_Static_assert(
    VITTE_FILESYSTEM_ERROR_COUNT >
        VITTE_FILESYSTEM_ERROR_UNSUPPORTED,
    "filesystem error enum invariant");

_Static_assert(
    VITTE_FILESYSTEM_KIND_COUNT >
        VITTE_FILESYSTEM_KIND_OTHER,
    "filesystem kind enum invariant");

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_FILESYSTEM_FILESYSTEM_H */
