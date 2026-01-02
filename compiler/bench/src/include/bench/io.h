/*
  bench/io.h

  I/O helpers for the Vitte benchmark harness.

  Scope
  - Unified read/write primitives for bench subsystems.
  - Small wrappers over the fileset I/O vtable (bench/fileset.h).
  - Convenience for reading fixtures and writing reports/baselines.

  Design
  - No global state.
  - Pluggable backend via vitte_fileset_io.
  - Portable across Windows/POSIX.

  Notes
  - This header provides helpers and default semantics; the default backend
    lives in bench/detail/fs.h.
*/

#pragma once
#ifndef VITTE_BENCH_IO_H
#define VITTE_BENCH_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/arena.h"
#include "bench/diag.h"
#include "bench/fileset.h"
#include "bench/hash.h"
#include "bench/detail/format.h"

/* -------------------------------------------------------------------------- */
/* Read / write convenience                                                     */
/* -------------------------------------------------------------------------- */

/*
  Read an entire file into arena memory.

  - out_data points into arena memory.
  - out_size is the file size.

  Returns false and sets diag on failure.
*/
bool vitte_io_read_all(
    const vitte_fileset_io* io,
    const char* path,
    vitte_arena* arena,
    const uint8_t** out_data,
    size_t* out_size,
    vitte_diag* d
);

/*
  Write an entire file.

  Returns false and sets diag on failure.
*/
bool vitte_io_write_all(
    const vitte_fileset_io* io,
    const char* path,
    const void* data,
    size_t size,
    vitte_diag* d
);

/* Ensure directory exists (mkdir -p). */
bool vitte_io_mkdirs(const vitte_fileset_io* io, const char* dir, vitte_diag* d);

/* Stat wrapper. */
bool vitte_io_stat(
    const vitte_fileset_io* io,
    const char* path,
    bool* out_exists,
    bool* out_is_dir,
    uint64_t* out_size,
    vitte_diag* d
);

/* List dir wrapper (names are arena-backed). */
bool vitte_io_list_dir(
    const vitte_fileset_io* io,
    const char* dir,
    vitte_arena* arena,
    const char*** out_names,
    size_t* out_count,
    vitte_diag* d
);

/* -------------------------------------------------------------------------- */
/* Path helpers                                                                 */
/* -------------------------------------------------------------------------- */

/*
  Normalize path:
  - converts backslashes to slashes
  - removes repeated slashes
  - resolves '.' segments
  - preserves leading '//' (UNC) best-effort

  Does not resolve '..' across root; keeps '..' segments (safe).

  Returns arena-backed normalized string.
*/
const char* vitte_io_norm_path(vitte_arena* arena, const char* path, vitte_diag* d);

/* Join two paths with a single '/' and normalize. */
const char* vitte_io_join(vitte_arena* arena, const char* a, const char* b, vitte_diag* d);

/* Get basename (pointer into input, not copied). */
const char* vitte_io_basename(const char* path);

/*
  Ensure parent directory exists for a given file path.
  If io->mkdirs is NULL, returns true.
*/
bool vitte_io_ensure_parent_dir(const vitte_fileset_io* io, const char* file_path, vitte_diag* d);

/* -------------------------------------------------------------------------- */
/* Hash helpers                                                                 */
/* -------------------------------------------------------------------------- */

/* Hash a file on disk (read all + hash64_bytes). */
bool vitte_io_hash_file64(
    const vitte_fileset_io* io,
    const char* path,
    uint64_t* out_hash,
    vitte_diag* d
);

/* -------------------------------------------------------------------------- */
/* Convenience: read a group of inputs                                          */
/* -------------------------------------------------------------------------- */

/*
  Read all entries in fileset of a given kind.

  Output arrays are arena-backed and aligned with the returned entries array.
*/
bool vitte_io_read_fileset_kind(
    const vitte_fileset* fs,
    const vitte_fileset_io* io,
    vitte_fileset_kind kind,
    const vitte_fileset_entry** out_entries,
    size_t* out_count,
    const uint8_t*** out_data,
    size_t** out_sizes,
    vitte_diag* d
);

/* -------------------------------------------------------------------------- */
/* Implementation                                                               */
/* -------------------------------------------------------------------------- */

static inline bool vitte_io__have(const vitte_fileset_io* io)
{
    return io && io->read_all && io->write_all && io->stat && io->list_dir;
}

bool vitte_io_read_all(
    const vitte_fileset_io* io,
    const char* path,
    vitte_arena* arena,
    const uint8_t** out_data,
    size_t* out_size,
    vitte_diag* d
)
{
    if (!io || !io->read_all || !path || !arena || !out_data || !out_size) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_read_all: invalid args");
        return false;
    }
    return io->read_all(io->user, path, arena, out_data, out_size, d);
}

bool vitte_io_write_all(
    const vitte_fileset_io* io,
    const char* path,
    const void* data,
    size_t size,
    vitte_diag* d
)
{
    if (!io || !io->write_all || !path || (!data && size)) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_write_all: invalid args");
        return false;
    }

    if (!vitte_io_ensure_parent_dir(io, path, d)) {
        return false;
    }

    return io->write_all(io->user, path, data, size, d);
}

bool vitte_io_mkdirs(const vitte_fileset_io* io, const char* dir, vitte_diag* d)
{
    if (!io || !dir) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_mkdirs: invalid args");
        return false;
    }
    if (!io->mkdirs) {
        return true;
    }
    return io->mkdirs(io->user, dir, d);
}

bool vitte_io_stat(
    const vitte_fileset_io* io,
    const char* path,
    bool* out_exists,
    bool* out_is_dir,
    uint64_t* out_size,
    vitte_diag* d
)
{
    if (!io || !io->stat || !path || !out_exists || !out_is_dir) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_stat: invalid args");
        return false;
    }
    return io->stat(io->user, path, out_exists, out_is_dir, out_size, d);
}

bool vitte_io_list_dir(
    const vitte_fileset_io* io,
    const char* dir,
    vitte_arena* arena,
    const char*** out_names,
    size_t* out_count,
    vitte_diag* d
)
{
    if (!io || !io->list_dir || !dir || !arena || !out_names || !out_count) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_list_dir: invalid args");
        return false;
    }
    return io->list_dir(io->user, dir, arena, out_names, out_count, d);
}

const char* vitte_io_basename(const char* path)
{
    if (!path) {
        return "";
    }
    const char* last = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            last = p + 1;
        }
    }
    return last;
}

static inline bool vitte_io__is_sep(char c) { return c == '/' || c == '\\'; }

const char* vitte_io_norm_path(vitte_arena* arena, const char* path, vitte_diag* d)
{
    if (!arena || !path) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_norm_path: invalid args");
        return NULL;
    }

    /* Allocate a temp buffer in arena: worst case same length + 1 */
    size_t n = 0;
    for (const char* p = path; *p; ++p) n++;

    char* tmp = (char*)vitte_arena_alloc(arena, n + 1, 1);
    if (!tmp) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_io_norm_path: oom");
        return NULL;
    }

    /* Pass 1: normalize slashes and collapse repeats */
    size_t w = 0;
    bool prev_sep = false;

    /* Preserve leading UNC // */
    bool unc = (n >= 2 && vitte_io__is_sep(path[0]) && vitte_io__is_sep(path[1]));

    for (size_t i = 0; i < n; ++i) {
        char c = path[i];
        if (c == '\\') c = '/';
        if (c == '/') {
            if (prev_sep) {
                continue;
            }
            prev_sep = true;
            tmp[w++] = '/';
        } else {
            prev_sep = false;
            tmp[w++] = c;
        }
    }

    /* restore UNC if it was present and got collapsed to single '/' */
    if (unc && w > 0 && tmp[0] == '/' && (w == 1 || tmp[1] != '/')) {
        /* shift right by 1 if possible */
        if (w + 1 <= n) {
            for (size_t i = w; i > 0; --i) tmp[i] = tmp[i - 1];
            tmp[0] = '/';
            tmp[1] = '/';
            w++;
        }
    }

    /* Pass 2: resolve '.' segments */
    char* out = tmp;
    size_t out_w = 0;

    size_t i = 0;
    while (i < w) {
        /* skip leading separators */
        bool is_root = false;
        if (i == 0 && out[i] == '/') {
            is_root = true;
        }

        /* read segment */
        size_t seg_start = i;
        while (seg_start < w && out[seg_start] == '/') seg_start++;
        size_t seg_end = seg_start;
        while (seg_end < w && out[seg_end] != '/') seg_end++;

        if (seg_start == seg_end) {
            /* only separators */
            if (i < w && out[i] == '/') {
                if (out_w == 0) out[out_w++] = '/';
                i = seg_end;
                continue;
            }
            break;
        }

        const size_t seg_len = seg_end - seg_start;
        const bool is_dot = (seg_len == 1 && out[seg_start] == '.');

        if (!is_dot) {
            /* ensure separator */
            if (out_w && out[out_w - 1] != '/') {
                out[out_w++] = '/';
            }
            for (size_t k = 0; k < seg_len; ++k) {
                out[out_w++] = out[seg_start + k];
            }
        } else {
            /* keep root only */
            if (is_root && out_w == 0) {
                out[out_w++] = '/';
            }
        }

        i = seg_end;
        if (i < w && out[i] == '/') i++;
    }

    /* trim trailing '/' except root or UNC */
    while (out_w > 1 && out[out_w - 1] == '/') {
        if (out_w >= 2 && out[0] == '/' && out[1] == '/') {
            break; /* keep UNC prefix */
        }
        out_w--;
    }

    out[out_w] = '\0';

    /* Return arena-backed string: tmp already in arena */
    return out;
}

const char* vitte_io_join(vitte_arena* arena, const char* a, const char* b, vitte_diag* d)
{
    if (!arena || !a || !b) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_join: invalid args");
        return NULL;
    }

    /* length */
    size_t na = 0, nb = 0;
    for (const char* p = a; *p; ++p) na++;
    for (const char* p = b; *p; ++p) nb++;

    /* allocate join buffer */
    char* tmp = (char*)vitte_arena_alloc(arena, na + 1 + nb + 1, 1);
    if (!tmp) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_io_join: oom");
        return NULL;
    }

    size_t w = 0;
    for (size_t i = 0; i < na; ++i) tmp[w++] = a[i];
    if (w && !vitte_io__is_sep(tmp[w - 1])) tmp[w++] = '/';
    for (size_t i = 0; i < nb; ++i) tmp[w++] = b[i];
    tmp[w] = '\0';

    return vitte_io_norm_path(arena, tmp, d);
}

bool vitte_io_ensure_parent_dir(const vitte_fileset_io* io, const char* file_path, vitte_diag* d)
{
    if (!io || !file_path) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_ensure_parent_dir: invalid args");
        return false;
    }
    if (!io->mkdirs) {
        return true;
    }

    /* find last separator */
    const char* last = NULL;
    for (const char* p = file_path; *p; ++p) {
        if (*p == '/' || *p == '\\') last = p;
    }
    if (!last) {
        return true;
    }

    /* copy dir into a small arena buffer via vitte_buf */
    vitte_buf b;
    vitte_buf_init(&b, NULL, 0);

    /* We do not assume vitte_buf can allocate; fallback to stack */
    size_t n = (size_t)(last - file_path);
    if (n == 0) {
        return true;
    }

    /* Use a small stack buffer for dir (common case). */
    if (n < 4096) {
        char tmp[4096];
        for (size_t i = 0; i < n; ++i) tmp[i] = file_path[i];
        tmp[n] = '\0';
        return vitte_io_mkdirs(io, tmp, d);
    }

    /* Large path: still best-effort - fail. */
    VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_ensure_parent_dir: path too long");
    (void)b;
    return false;
}

bool vitte_io_hash_file64(
    const vitte_fileset_io* io,
    const char* path,
    uint64_t* out_hash,
    vitte_diag* d
)
{
    if (!io || !path || !out_hash) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_hash_file64: invalid args");
        return false;
    }

    /* Read via a temporary arena if needed. Caller should generally provide an arena.
       Here we require a backend that can allocate via its read_all arena parameter.
    */
    vitte_arena tmp_arena;
    vitte_arena_init(&tmp_arena, NULL, 0);

    const uint8_t* data = NULL;
    size_t size = 0;
    if (!io->read_all(io->user, path, &tmp_arena, &data, &size, d)) {
        vitte_arena_destroy(&tmp_arena);
        return false;
    }

    *out_hash = vitte_bench_hash64_bytes(data, size);
    vitte_arena_destroy(&tmp_arena);
    return true;
}

bool vitte_io_read_fileset_kind(
    const vitte_fileset* fs,
    const vitte_fileset_io* io,
    vitte_fileset_kind kind,
    const vitte_fileset_entry** out_entries,
    size_t* out_count,
    const uint8_t*** out_data,
    size_t** out_sizes,
    vitte_diag* d
)
{
    if (!fs || !io || !out_entries || !out_count || !out_data || !out_sizes) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_io_read_fileset_kind: invalid args");
        return false;
    }

    return vitte_fileset_read_kind(fs, io, kind, out_data, out_sizes, out_count, d);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_IO_H */
