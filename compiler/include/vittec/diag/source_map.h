

#ifndef VITTEC_INCLUDE_VITTEC_DIAG_SOURCE_MAP_H
#define VITTEC_INCLUDE_VITTEC_DIAG_SOURCE_MAP_H

/*
  source_map.h

  SourceMap = mapping between:
    - FileId
    - raw source bytes
    - byte offsets (spans)
    - (line, col)

  Bootstrap goals:
  - Deterministic
  - Minimal dependencies
  - Fast (precompute line starts)

  Conventions:
  - Offsets are byte offsets in the original source buffer.
  - Display line/col are 1-based.
  - Newlines: supports \n and \r\n (CRLF).
*/

#include <stdint.h>
#include <stddef.h>

#include "vittec/diag/span.h"
#include "vittec/support/str.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTEC_SOURCE_MAP_API_VERSION 1u

typedef uint32_t vittec_file_id_t;

/* Returned by line/col queries (1-based). */
typedef struct vittec_line_col {
  uint32_t line; /* 1-based */
  uint32_t col;  /* 1-based (byte-based for now; UTF-8 aware later) */
} vittec_line_col_t;

/* A single source file tracked by SourceMap. */
typedef struct vittec_source_file {
  vittec_sv_t path;       /* logical path (may be relative) */
  const uint8_t* data;    /* source bytes */
  uint32_t len;           /* byte length */

  /* line_starts[i] is the byte offset of the first byte of line i (0-based). */
  uint32_t* line_starts;
  uint32_t line_count;

  /* Ownership:
     - if owns_data != 0, `data` is heap-owned by the SourceMap.
     - line_starts is always owned by the SourceMap.
  */
  uint8_t owns_data;
} vittec_source_file_t;

/* The global source map for one compilation session. */
typedef struct vittec_source_map {
  vittec_source_file_t* files;
  uint32_t len;
  uint32_t cap;
} vittec_source_map_t;

/* -------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------- */

void vittec_source_map_init(vittec_source_map_t* sm);
void vittec_source_map_free(vittec_source_map_t* sm);

/* -------------------------------------------------------------------------
 * Add sources
 * -------------------------------------------------------------------------
 *
 * Return convention: 0 = success, non-zero = failure.
 * Error codes are intentionally coarse here; callers can translate to diagnostics.
 */

typedef enum vittec_source_map_err {
  VITTEC_SM_OK = 0,
  VITTEC_SM_EINVAL = 1,
  VITTEC_SM_EIO = 2,
  VITTEC_SM_EOOM = 3,
} vittec_source_map_err_t;

/*
  Add a file from disk.
  - path is a C string.
  - file content is loaded into memory and owned by SourceMap.
*/
int vittec_source_map_add_path(vittec_source_map_t* sm, const char* path, vittec_file_id_t* out_id);

/*
  Add a file from an in-memory buffer.

  - If copy != 0: data is copied and owned by SourceMap.
  - If copy == 0: caller retains ownership; buffer must outlive the SourceMap.
*/
int vittec_source_map_add_memory(
  vittec_source_map_t* sm,
  vittec_sv_t path,
  const void* data,
  size_t len,
  int copy,
  vittec_file_id_t* out_id
);

/* -------------------------------------------------------------------------
 * Accessors
 * ------------------------------------------------------------------------- */

static inline uint32_t vittec_source_map_file_count(const vittec_source_map_t* sm) {
  return sm ? sm->len : 0u;
}

/* Returns NULL if file id is invalid. */
const vittec_source_file_t* vittec_source_map_get_file(const vittec_source_map_t* sm, vittec_file_id_t file);

/* Convenience accessors (NULL/empty-safe). */
vittec_sv_t vittec_source_map_file_path(const vittec_source_map_t* sm, vittec_file_id_t file);
const uint8_t* vittec_source_map_file_data(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t* out_len);

/* -------------------------------------------------------------------------
 * Line/column mapping
 * -------------------------------------------------------------------------
 *
 * Important:
 * - For now, col is computed in bytes from the last line start.
 * - UTF-8 aware display columns can be implemented later.
 */

/* Convert (file, offset) to (line, col) (1-based). */
vittec_line_col_t vittec_source_map_line_col(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t offset);

/* Shortcut: compute line/col for the start of a span. */
static inline vittec_line_col_t vittec_source_map_span_line_col(const vittec_source_map_t* sm, vittec_span_t sp) {
  return vittec_source_map_line_col(sm, sp.file, sp.lo);
}

/*
  Get the 0-based line index containing `offset`.
  Returns UINT32_MAX if file invalid.
*/
uint32_t vittec_source_map_line_index(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t offset);

/*
  Return the span of a line (0-based line index).
  - On success returns 1 and writes out_span.
  - On failure returns 0.

  The returned span covers the line content WITHOUT trailing newline characters.
*/
int vittec_source_map_line_span(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t line_index, vittec_span_t* out_span);

/*
  Returns a string view of the line (without trailing newline).
  The view points into the source buffer (no allocation).

  Returns empty view on failure.
*/
vittec_sv_t vittec_source_map_line_text(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t line_index);

/* -------------------------------------------------------------------------
 * Snippets (for emitters)
 * -------------------------------------------------------------------------
 *
 * Emitters often want:
 * - a window of lines around the primary label
 * - the primary line/col
 * - the raw line text
 */

typedef struct vittec_snippet {
  vittec_file_id_t file;

  /* 0-based line range [line_lo, line_hi) */
  uint32_t line_lo;
  uint32_t line_hi;

  /* Primary location (1-based) */
  vittec_line_col_t primary;

  /* Primary span in bytes */
  vittec_span_t primary_span;
} vittec_snippet_t;

/*
  Build a snippet window around the span.

  context_lines:
  - 0 => only the line containing sp.lo
  - 1 => one line above + one line below, etc.

  Returns 1 on success, 0 on failure.
*/
int vittec_source_map_build_snippet(
  const vittec_source_map_t* sm,
  vittec_span_t sp,
  uint32_t context_lines,
  vittec_snippet_t* out
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_DIAG_SOURCE_MAP_H */