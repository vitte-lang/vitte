#ifndef VITTEC_INCLUDE_VITTEC_DIAG_EMITTER_H
#define VITTEC_INCLUDE_VITTEC_DIAG_EMITTER_H

/*
  emitter.h

  Diagnostic emitters.

  - Legacy emitters operate on `vittec_diag_sink_t` (minimal model).
  - Modern emitters operate on `vittec_diag_bag_t` (structured model).

  The `_ex` API is options-based and forward-compatible.
*/

#include <stdint.h>
#include <stddef.h>

#include "vittec/diag/diagnostic.h"
#include "vittec/diag/source_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTEC_EMITTER_API_VERSION 1u

/* -------------------------------------------------------------------------
 * Options
 * -------------------------------------------------------------------------
 *
 * Notes:
 * - If `opt == NULL`, the emitter uses defaults.
 * - If `opt->size == 0`, treated as sizeof(vittec_emit_options_t).
 * - Emitters must be deterministic; if sorting is enabled, it must be stable.
 */

typedef enum vittec_emit_format {
  VITTEC_EMIT_FORMAT_HUMAN = 0,
  VITTEC_EMIT_FORMAT_JSON  = 1,
} vittec_emit_format_t;

typedef struct vittec_emit_options {
  uint32_t size;

  /* Rendering */
  int use_color;          /* -1: auto, 0: off, 1: on */
  int context_lines;      /* number of source lines around primary span (default 1) */
  int show_line_numbers;  /* human: show "12 |" gutter (default 1) */
  int show_notes;         /* human/json: include notes array (default 1) */
  int show_help;          /* human/json: include help field (default 1) */

  /* Determinism */
  int sort_by_location;   /* stable sort by (file, lo, severity) before emitting (default 0) */

  /* Output tweaks */
  int json_pretty;        /* json: pretty print (default 0) */
  int json_one_per_line;  /* json: one JSON object per line (NDJSON) (default 1) */

  /* Future: output stream override (if NULL, emitters use stdout/stderr internally). */
  void* out_stream;
} vittec_emit_options_t;

/* Initialize options with safe defaults. */
void vittec_emit_options_init(vittec_emit_options_t* opt);

/* -------------------------------------------------------------------------
 * Legacy emitters (minimal diagnostics) — KEEP
 * ------------------------------------------------------------------------- */

/*
  Legacy API: emits to stdout (human) / stdout (json) by default.
  This API is kept for backward compatibility.
*/
void vittec_emit_human(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags);
void vittec_emit_json(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags);

/* Extended legacy emitters with options. */
void vittec_emit_human_ex(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags, const vittec_emit_options_t* opt);
void vittec_emit_json_ex(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags, const vittec_emit_options_t* opt);

/* -------------------------------------------------------------------------
 * Modern emitters (structured diagnostics)
 * ------------------------------------------------------------------------- */

/*
  Emits diagnostics collected in a structured bag.

  Human format:
    error[E0001]: message
      --> file:line:col
       |
      ...

  JSON format:
    - NDJSON (default): one object per line
    - Pretty JSON array (opt-in): json_pretty=1 & json_one_per_line=0
*/
void vittec_emit_human_bag(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag);
void vittec_emit_json_bag(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag);

/* Options-based variants. */
void vittec_emit_human_bag_ex(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag, const vittec_emit_options_t* opt);
void vittec_emit_json_bag_ex(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag, const vittec_emit_options_t* opt);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_DIAG_EMITTER_H */
