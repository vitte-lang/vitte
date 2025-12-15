#ifndef VITTEC_INCLUDE_VITTEC_DIAG_SPAN_H
#define VITTEC_INCLUDE_VITTEC_DIAG_SPAN_H

/*
  span.h

  A Span locates a byte range in a source file.

  Conventions:
  - Offsets are byte offsets into the original source buffer.
  - Range is half-open: [lo, hi)
  - Invariants: lo <= hi

  Backward compatibility:
  - Older code used `span.file_id`.
  - Newer code uses `span.file`.
  - We provide both via a union on mainstream compilers.
*/

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VITTEC_SPAN_API_VERSION 1u

typedef uint32_t vittec_file_id_t;

typedef struct vittec_span {
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  union {
    vittec_file_id_t file;    /* canonical */
    vittec_file_id_t file_id; /* legacy */
  };
#else
  vittec_file_id_t file_id;  /* legacy fallback */
#endif
  uint32_t lo;
  uint32_t hi;
} vittec_span_t;

/* Constructor (canonical). */
static inline vittec_span_t vittec_span(vittec_file_id_t file, uint32_t lo, uint32_t hi) {
  vittec_span_t s;
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  s.file = file;
#else
  s.file_id = file;
#endif
  s.lo = lo;
  s.hi = hi;
  return s;
}

/* Legacy constructor alias (kept for readability in older code). */
static inline vittec_span_t vittec_span_with_file_id(vittec_file_id_t file_id, uint32_t lo, uint32_t hi) {
  return vittec_span(file_id, lo, hi);
}

/* Helpers */
static inline uint32_t vittec_span_len(vittec_span_t s) {
  return (s.hi >= s.lo) ? (s.hi - s.lo) : 0u;
}

static inline int vittec_span_is_empty(vittec_span_t s) {
  return s.lo >= s.hi;
}

static inline int vittec_span_is_valid(vittec_span_t s) {
  return s.lo <= s.hi;
}

static inline int vittec_span_contains(vittec_span_t s, uint32_t off) {
  return off >= s.lo && off < s.hi;
}

static inline int vittec_span_intersects(vittec_span_t a, vittec_span_t b) {
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  if(a.file != b.file) return 0;
#else
  if(a.file_id != b.file_id) return 0;
#endif
  return (a.lo < b.hi) && (b.lo < a.hi);
}

static inline vittec_span_t vittec_span_merge(vittec_span_t a, vittec_span_t b) {
  /* Caller is responsible for ensuring same file. */
  uint32_t lo = (a.lo < b.lo) ? a.lo : b.lo;
  uint32_t hi = (a.hi > b.hi) ? a.hi : b.hi;
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
  return vittec_span(a.file, lo, hi);
#else
  return vittec_span(a.file_id, lo, hi);
#endif
}

static inline vittec_span_t vittec_span_clamp(vittec_span_t s, uint32_t max_len) {
  if(s.lo > max_len) s.lo = max_len;
  if(s.hi > max_len) s.hi = max_len;
  if(s.hi < s.lo) s.hi = s.lo;
  return s;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTEC_INCLUDE_VITTEC_DIAG_SPAN_H */
