#ifndef VITTEC_INCLUDE_VITTEC_DIAG_DIAGNOSTIC_H
#define VITTEC_INCLUDE_VITTEC_DIAG_DIAGNOSTIC_H

/*
  diagnostic.h

  Diagnostics model (bootstrap-friendly, deterministic).

  This header provides:
  - A legacy minimal diagnostic struct (severity + span + message) used by early code.
  - A richer structured model (code + labels + notes + help) for modern diagnostics.

  IMPORTANT:
  - The legacy API is kept intact for backward compatibility.
  - New code should prefer the structured API (`vittec_diag_t`, `vittec_diag_bag_t`).
*/

#include <stdint.h>
#include <stddef.h>

#include "vittec/diag/severity.h"
#include "vittec/diag/span.h"
#include "vittec/support/str.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Legacy (minimal) API                                                       */
/* ========================================================================== */

typedef struct vittec_diagnostic {
  vittec_severity_t severity;
  vittec_span_t span;
  vittec_sv_t message;
} vittec_diagnostic_t;

typedef struct vittec_diag_sink {
  vittec_diagnostic_t* diags;
  uint32_t len;
  uint32_t cap;
} vittec_diag_sink_t;

void vittec_diag_sink_init(vittec_diag_sink_t* s);
void vittec_diag_sink_free(vittec_diag_sink_t* s);
void vittec_diag_push(vittec_diag_sink_t* s, vittec_severity_t sev, vittec_span_t span, vittec_sv_t msg);

/* ========================================================================== */
/* Structured (modern) diagnostics                                             */
/* ========================================================================== */

#define VITTEC_DIAG_API_VERSION 1u

/* Label style: primary is the main error location; secondary is related context. */
typedef enum vittec_diag_label_style {
  VITTEC_DIAG_LABEL_PRIMARY = 0,
  VITTEC_DIAG_LABEL_SECONDARY = 1,
} vittec_diag_label_style_t;

typedef struct vittec_diag_label {
  vittec_diag_label_style_t style;
  vittec_span_t span;
  /* Optional label message (can be empty). */
  vittec_sv_t message;
} vittec_diag_label_t;

/*
  Structured diagnostic.

  Contract:
  - `labels` must contain exactly one PRIMARY label.
  - `code` should be stable (ex: "E0001", "W0100").
*/
typedef struct vittec_diag {
  vittec_severity_t severity;
  vittec_sv_t code;     /* e.g. "E0001" */
  vittec_sv_t message;  /* short message */

  vittec_diag_label_t* labels;
  uint32_t labels_len;
  uint32_t labels_cap;

  vittec_sv_t* notes;
  uint32_t notes_len;
  uint32_t notes_cap;

  /* Optional: if `help_len==0`, help is absent. */
  vittec_sv_t help;
} vittec_diag_t;

/* A bag of diagnostics collected during one compilation. */
typedef struct vittec_diag_bag {
  vittec_diag_t* diags;
  uint32_t len;
  uint32_t cap;

  /* Cached error count for fast checks. */
  uint32_t errors;
} vittec_diag_bag_t;

/* -------------------------------------------------------------------------- */
/* Construction                                                               */
/* -------------------------------------------------------------------------- */

/* Initialize a diagnostic with a primary label. */
void vittec_diag_init(
  vittec_diag_t* d,
  vittec_severity_t sev,
  vittec_sv_t code,
  vittec_span_t primary_span,
  vittec_sv_t message
);

/* Free internal arrays of a diagnostic. Does not free the `vittec_diag_t` itself. */
void vittec_diag_free(vittec_diag_t* d);

/* Add a label (secondary or primary). Adding a second primary should be rejected by impl. */
int vittec_diag_add_label(vittec_diag_t* d, vittec_diag_label_style_t style, vittec_span_t span, vittec_sv_t msg);

/* Add a note line (optional). */
int vittec_diag_add_note(vittec_diag_t* d, vittec_sv_t note);

/* Set help text (optional). */
void vittec_diag_set_help(vittec_diag_t* d, vittec_sv_t help);

/* -------------------------------------------------------------------------- */
/* Bag helpers                                                                */
/* -------------------------------------------------------------------------- */

void vittec_diag_bag_init(vittec_diag_bag_t* b);
void vittec_diag_bag_free(vittec_diag_bag_t* b);

/* Push a fully built diagnostic; ownership is transferred into the bag. */
int vittec_diag_bag_push(vittec_diag_bag_t* b, const vittec_diag_t* d);

/* Convenience: push and return pointer to the created diagnostic so caller can add labels/notes/help. */
vittec_diag_t* vittec_diag_bag_push_new(
  vittec_diag_bag_t* b,
  vittec_severity_t sev,
  vittec_sv_t code,
  vittec_span_t primary_span,
  vittec_sv_t message
);

static inline int vittec_diag_bag_has_errors(const vittec_diag_bag_t* b) {
  return b && b->errors != 0;
}

/* Convenience helpers (return pointer to the created diag). */
static inline vittec_diag_t* vittec_diag_error(vittec_diag_bag_t* b, vittec_sv_t code, vittec_span_t sp, vittec_sv_t msg) {
  return vittec_diag_bag_push_new(b, VITTEC_SEV_ERROR, code, sp, msg);
}

static inline vittec_diag_t* vittec_diag_warning(vittec_diag_bag_t* b, vittec_sv_t code, vittec_span_t sp, vittec_sv_t msg) {
  return vittec_diag_bag_push_new(b, VITTEC_SEV_WARNING, code, sp, msg);
}

static inline vittec_diag_t* vittec_diag_note(vittec_diag_bag_t* b, vittec_sv_t code, vittec_span_t sp, vittec_sv_t msg) {
  return vittec_diag_bag_push_new(b, VITTEC_SEV_NOTE, code, sp, msg);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_DIAG_DIAGNOSTIC_H */
