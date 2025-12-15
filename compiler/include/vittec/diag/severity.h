#ifndef VITTEC_INCLUDE_VITTEC_DIAG_SEVERITY_H
#define VITTEC_INCLUDE_VITTEC_DIAG_SEVERITY_H

/*
  severity.h

  Diagnostic severity.

  Contract:
  - Values are stable and must not change (serialized in JSON, tests, tools).
  - Ordering (by importance): error > warning > note.
*/

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VITTEC_SEVERITY_API_VERSION 1u

typedef enum vittec_severity {
  /* Compilation must stop. */
  VITTEC_SEV_ERROR = 0,

  /* Compilation can continue, but the user should fix it. */
  VITTEC_SEV_WARNING = 1,

  /* Informational / extra context. */
  VITTEC_SEV_NOTE = 2,
} vittec_severity_t;

/* Returns a stable lowercase name: "error", "warning", "note". */
static inline const char* vittec_severity_name(vittec_severity_t s) {
  switch(s) {
    case VITTEC_SEV_ERROR:   return "error";
    case VITTEC_SEV_WARNING: return "warning";
    case VITTEC_SEV_NOTE:    return "note";
    default:                 return "unknown";
  }
}

/* Importance rank used for stable sorting (lower is more important). */
static inline int vittec_severity_rank(vittec_severity_t s) {
  switch(s) {
    case VITTEC_SEV_ERROR:   return 0;
    case VITTEC_SEV_WARNING: return 1;
    case VITTEC_SEV_NOTE:    return 2;
    default:                 return 3;
  }
}

static inline int vittec_severity_is_error(vittec_severity_t s) {
  return s == VITTEC_SEV_ERROR;
}

static inline int vittec_severity_is_warning(vittec_severity_t s) {
  return s == VITTEC_SEV_WARNING;
}

static inline int vittec_severity_is_note(vittec_severity_t s) {
  return s == VITTEC_SEV_NOTE;
}

/* Parse severity from ASCII string.

   Accepts:
     - "error" | "warning" | "note"
     - "err"   | "warn"    | "info" (maps to note)

   Returns 1 on success, 0 on failure.
*/
static inline int vittec_severity_from_cstr(const char* s, vittec_severity_t* out) {
  if(!s || !out) return 0;

  /* minimal, ASCII-only compare */
  #define EQ(A,B) (0 == vittec__streq((A),(B)))

  /* local helper: strcmp without <string.h> */
  /* returns 0 if equal, non-zero otherwise */
  static inline int vittec__streq(const char* a, const char* b) {
    if(a == b) return 0;
    if(!a || !b) return 1;
    while(*a && *b) {
      if(*a != *b) return 1;
      a++; b++;
    }
    return (*a == '\0' && *b == '\0') ? 0 : 1;
  }

  if(EQ(s, "error") || EQ(s, "err")) {
    *out = VITTEC_SEV_ERROR;
    return 1;
  }
  if(EQ(s, "warning") || EQ(s, "warn")) {
    *out = VITTEC_SEV_WARNING;
    return 1;
  }
  if(EQ(s, "note") || EQ(s, "info")) {
    *out = VITTEC_SEV_NOTE;
    return 1;
  }

  return 0;

  #undef EQ
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_DIAG_SEVERITY_H */
