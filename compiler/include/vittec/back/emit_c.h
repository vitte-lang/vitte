#ifndef VITTEC_INCLUDE_VITTEC_BACK_EMIT_C_H
#define VITTEC_INCLUDE_VITTEC_BACK_EMIT_C_H

/*
  emit_c.h

  Back-end: C code emission (bootstrap-friendly)

  Goals:
  - Deterministic, readable C output in early bootstrap phases.
  - Allow two modes:
      * stubs-only: emit signatures + placeholder bodies (fast, minimal parser requirements)
      * full: emit a subset of statements/expressions when available
  - Provide an options struct to extend behavior without breaking ABI.

  Contract:
  - Input is a parse unit (bootstrap index or richer AST).
  - Output is a single .c file (and optionally a matching .h header).
  - The emitter should not perform heavy semantic checks (that belongs to sema/).
*/

#include <stddef.h>
#include <stdint.h>

#include "vittec/front/parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Versioning
 * ------------------------------------------------------------------------- */

#define VITTEC_EMIT_C_API_VERSION 1u

/* -------------------------------------------------------------------------
 * Error codes
 * -------------------------------------------------------------------------
 * Return convention: 0 = success, non-zero = failure.
 * Keep these stable; add new codes at the end.
 */

typedef enum vittec_emit_c_err {
  VITTEC_EMIT_C_OK = 0,
  VITTEC_EMIT_C_EINVAL = 1,       /* invalid argument */
  VITTEC_EMIT_C_EIO = 2,          /* could not open/write output */
  VITTEC_EMIT_C_EINTERNAL = 100,  /* unexpected internal error */
} vittec_emit_c_err_t;

/* -------------------------------------------------------------------------
 * Emission configuration
 * ------------------------------------------------------------------------- */

typedef enum vittec_emit_c_mode {
  /* Emit prototypes + stub bodies only (bootstrap). */
  VITTEC_EMIT_C_MODE_STUBS = 0,

  /* Emit full function bodies for supported AST nodes (later phases). */
  VITTEC_EMIT_C_MODE_FULL = 1,
} vittec_emit_c_mode_t;

typedef enum vittec_emit_c_mangle_style {
  /* e.g. vitte_mod_fn */
  VITTEC_EMIT_C_MANGLE_SIMPLE = 0,

  /* e.g. vitte_mod_fn__<hash> (more stable against collisions) */
  VITTEC_EMIT_C_MANGLE_HASHED = 1,
} vittec_emit_c_mangle_style_t;

typedef enum vittec_emit_c_runtime_style {
  /* Emit pure C without linking any Vitte runtime header by default. */
  VITTEC_EMIT_C_RUNTIME_NONE = 0,

  /* Emit includes / calls targeting the Vitte runtime (when available). */
  VITTEC_EMIT_C_RUNTIME_VITTE = 1,
} vittec_emit_c_runtime_style_t;

/* Optional callback: custom sink for generated files.
   If NULL, emitter writes to `out_path` directly.

   Contract:
   - Return 0 on success, non-zero on error.
   - `data` is valid for the duration of the call only.
*/

typedef int (*vittec_emit_c_write_file_fn)(
  void* user,
  const char* path,
  const void* data,
  size_t size
);

/* -------------------------------------------------------------------------
 * Options
 * -------------------------------------------------------------------------
 *
 * IMPORTANT:
 * - The emitter must tolerate opt==NULL by using defaults.
 * - When `size` is set, the emitter may use it to preserve forward/backward
 *   compatibility when new fields are added.
 */

typedef struct vittec_emit_c_options {
  /* Size of this struct in bytes (set by caller). If 0, treated as sizeof(vittec_emit_c_options). */
  uint32_t size;

  vittec_emit_c_mode_t mode;
  vittec_emit_c_mangle_style_t mangle;
  vittec_emit_c_runtime_style_t runtime;

  /* Output toggles */
  int emit_header;          /* also emit a .h (same base name as out_path) */
  int emit_line_directives; /* emit #line for easier debugging */
  int emit_debug_comments;  /* emit extra comments (spans, names, etc.) */

  /* Naming */
  const char* namespace_prefix;   /* e.g. "vitte_"; NULL -> default */
  const char* header_guard_prefix;/* e.g. "VITTE_"; NULL -> default */

  /* Runtime */
  const char* runtime_header;     /* e.g. "vitte/runtime.h"; NULL -> none */

  /* Output sink override */
  vittec_emit_c_write_file_fn write_file;
  void* write_file_user;
} vittec_emit_c_options_t;

/* Initialize options with safe defaults.
   Caller may then override fields as needed.
*/
void vittec_emit_c_options_init(vittec_emit_c_options_t* opt);

/* -------------------------------------------------------------------------
 * API
 * ------------------------------------------------------------------------- */

/* Backward compatible convenience API.
   Equivalent to vittec_emit_c_file_ex(u, out_path, NULL).
*/
int vittec_emit_c_file(const vittec_parse_unit_t* u, const char* out_path);

/* Extended API with options.

   out_path:
   - If opt->emit_header == 0 : path to the .c file to generate.
   - If opt->emit_header != 0 : base path, emitter generates:
        <out_path>.c (or out_path if already ends with .c)
        <out_path>.h (same base, .h)

   Return:
   - 0 on success
   - non-zero on failure (see vittec_emit_c_err_t)
*/
int vittec_emit_c_file_ex(
  const vittec_parse_unit_t* u,
  const char* out_path,
  const vittec_emit_c_options_t* opt
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_BACK_EMIT_C_H */
