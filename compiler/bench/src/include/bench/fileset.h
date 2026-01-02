

/*
  bench/fileset.h

  File set abstraction for the Vitte benchmark harness.

  Why
  - Benchmarks and generators frequently need to manage a group of related files:
      - input corpora
      - generated artifacts
      - golden outputs
      - per-run temporary files
      - baselines and reports

  This module provides:
  - an arena-backed list of file entries
  - helpers to add/normalize paths
  - helpers to enumerate and to open/read/write through a small I/O vtable

  Constraints
  - Portable C17 (Windows/POSIX).
  - Deterministic ordering and formatting.
  - No global state.

  Notes
  - Path normalization is conservative; it does not resolve symlinks.
  - This header is designed to be used together with bench/detail/fs.h.
*/

#pragma once
#ifndef VITTE_BENCH_FILESET_H
#define VITTE_BENCH_FILESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/arena.h"
#include "bench/detail/format.h"
#include "bench/diag.h"

/* -------------------------------------------------------------------------- */
/* Types                                                                       */
/* -------------------------------------------------------------------------- */

/* File kind / semantic role. */
typedef enum vitte_fileset_kind {
    VITTE_FILESET_KIND_UNKNOWN = 0,

    VITTE_FILESET_KIND_INPUT,
    VITTE_FILESET_KIND_GENERATED,
    VITTE_FILESET_KIND_GOLDEN,
    VITTE_FILESET_KIND_TEMP,

    VITTE_FILESET_KIND_BASELINE,
    VITTE_FILESET_KIND_REPORT,

    VITTE_FILESET_KIND_CONFIG
} vitte_fileset_kind;

/* File entry in a set. */
typedef struct vitte_fileset_entry {
    vitte_fileset_kind kind;

    /* normalized path (UTF-8) */
    const char* path;

    /* optional label for reporting, e.g. "unicode.vitte" */
    const char* label;

    /* optional content hash (e.g., xxh64) for cache/baseline ties */
    uint64_t hash;

    /* user flags */
    uint32_t flags;
} vitte_fileset_entry;

/*
  File set container.

  Memory
  - All strings and arrays live in the provided arena.
*/
typedef struct vitte_fileset {
    vitte_arena* arena;

    vitte_fileset_entry* entries;
    size_t count;
    size_t cap;

    /* optional base directory (normalized) */
    const char* base_dir;

    /* if true, entries are kept sorted by (kind, path). */
    bool keep_sorted;
} vitte_fileset;

/* Flags for entries. */
#define VITTE_FILESET_F_NONE         0u
#define VITTE_FILESET_F_RECURSIVE    (1u << 0) /* for directories when expanding */
#define VITTE_FILESET_F_OPTIONAL     (1u << 1) /* ignore missing file */
#define VITTE_FILESET_F_READONLY     (1u << 2)
#define VITTE_FILESET_F_EXECUTABLE   (1u << 3)

/* -------------------------------------------------------------------------- */
/* I/O interface                                                               */
/* -------------------------------------------------------------------------- */

/* Read whole file into a buffer. */
typedef bool (*vitte_fileset_read_all_fn)(
    void* user,
    const char* path,
    vitte_arena* arena,
    const uint8_t** out_data,
    size_t* out_size,
    vitte_diag* d
);

/* Write whole file from memory. */
typedef bool (*vitte_fileset_write_all_fn)(
    void* user,
    const char* path,
    const void* data,
    size_t size,
    vitte_diag* d
);

/* Ensure directory exists (mkdir -p). */
typedef bool (*vitte_fileset_mkdirs_fn)(
    void* user,
    const char* dir,
    vitte_diag* d
);

/* Stat: detect if path exists and is a file/dir. */
typedef bool (*vitte_fileset_stat_fn)(
    void* user,
    const char* path,
    bool* out_exists,
    bool* out_is_dir,
    uint64_t* out_size,
    vitte_diag* d
);

/* Enumerate directory entries (non-recursive). */
typedef bool (*vitte_fileset_list_dir_fn)(
    void* user,
    const char* dir,
    vitte_arena* arena,
    const char*** out_names,
    size_t* out_count,
    vitte_diag* d
);

/*
  I/O vtable. Plug in bench/detail/fs.h implementation or mocks for tests.
*/
typedef struct vitte_fileset_io {
    void* user;

    vitte_fileset_read_all_fn  read_all;
    vitte_fileset_write_all_fn write_all;
    vitte_fileset_mkdirs_fn    mkdirs;
    vitte_fileset_stat_fn      stat;
    vitte_fileset_list_dir_fn  list_dir;
} vitte_fileset_io;

/* -------------------------------------------------------------------------- */
/* Init / configuration                                                        */
/* -------------------------------------------------------------------------- */

void vitte_fileset_init(vitte_fileset* fs, vitte_arena* arena);

/* base_dir is copied into arena and normalized. */
bool vitte_fileset_set_base_dir(vitte_fileset* fs, const char* base_dir, vitte_diag* d);

/* if true, maintain sorted invariant. */
void vitte_fileset_set_keep_sorted(vitte_fileset* fs, bool keep_sorted);

/* -------------------------------------------------------------------------- */
/* Add entries                                                                 */
/* -------------------------------------------------------------------------- */

/* Adds a path (file or dir). The path is normalized and copied into arena.
   Returns false on OOM or invalid path. */
bool vitte_fileset_add(
    vitte_fileset* fs,
    vitte_fileset_kind kind,
    const char* path,
    const char* label,
    uint32_t flags,
    vitte_diag* d
);

/* Convenience: join base_dir + rel path if base_dir is set. */
bool vitte_fileset_add_rel(
    vitte_fileset* fs,
    vitte_fileset_kind kind,
    const char* rel_path,
    const char* label,
    uint32_t flags,
    vitte_diag* d
);

/*
  Expand directory entries into file entries using io->list_dir and io->stat.
  - If an entry is a directory and has RECURSIVE, recursively expand.
  - Keeps original directory entry unless `remove_dirs` is true.
*/
bool vitte_fileset_expand_dirs(
    vitte_fileset* fs,
    const vitte_fileset_io* io,
    bool remove_dirs,
    vitte_diag* d
);

/* -------------------------------------------------------------------------- */
/* Query / utility                                                             */
/* -------------------------------------------------------------------------- */

size_t vitte_fileset_count(const vitte_fileset* fs);
const vitte_fileset_entry* vitte_fileset_entries(const vitte_fileset* fs);

/* Find by exact normalized path. Returns NULL if not found. */
const vitte_fileset_entry* vitte_fileset_find(const vitte_fileset* fs, vitte_fileset_kind kind, const char* path);

/* Deterministic sort by (kind, path). */
void vitte_fileset_sort(vitte_fileset* fs);

/* Format as newline-separated list: "kind\tpath\tlabel" */
bool vitte_fileset_format(vitte_buf* out, const vitte_fileset* fs);

/* Hash paths + kind to produce stable set signature. */
uint64_t vitte_fileset_hash64(const vitte_fileset* fs);

/* -------------------------------------------------------------------------- */
/* Convenience I/O                                                             */
/* -------------------------------------------------------------------------- */

/* Read all entries of `kind` and return arrays of blobs (arena-backed).
   The caller receives arrays out_data[i], out_size[i] aligned with entries.
*/
bool vitte_fileset_read_kind(
    const vitte_fileset* fs,
    const vitte_fileset_io* io,
    vitte_fileset_kind kind,
    const uint8_t*** out_data,
    size_t** out_sizes,
    size_t* out_count,
    vitte_diag* d
);

/* Write data to a (possibly base_dir-relative) report/baseline entry. */
bool vitte_fileset_write(
    const vitte_fileset_io* io,
    const char* path,
    const void* data,
    size_t size,
    vitte_diag* d
);

/* -------------------------------------------------------------------------- */
/* Inline helpers: kind names                                                  */
/* -------------------------------------------------------------------------- */

static inline const char* vitte_fileset_kind_name(vitte_fileset_kind k)
{
    switch (k) {
        case VITTE_FILESET_KIND_INPUT: return "input";
        case VITTE_FILESET_KIND_GENERATED: return "generated";
        case VITTE_FILESET_KIND_GOLDEN: return "golden";
        case VITTE_FILESET_KIND_TEMP: return "temp";
        case VITTE_FILESET_KIND_BASELINE: return "baseline";
        case VITTE_FILESET_KIND_REPORT: return "report";
        case VITTE_FILESET_KIND_CONFIG: return "config";
        default: return "unknown";
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_FILESET_H */