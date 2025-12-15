#ifndef VITTEC_INCLUDE_VITTEC_DIAG_SPAN_H
    #define VITTEC_INCLUDE_VITTEC_DIAG_SPAN_H

    #include <stdint.h>

    typedef struct vittec_span {
  uint32_t file_id;
  uint32_t lo;
  uint32_t hi;
} vittec_span_t;

static inline vittec_span_t vittec_span(uint32_t file_id, uint32_t lo, uint32_t hi) {
  vittec_span_t s; s.file_id=file_id; s.lo=lo; s.hi=hi; return s;
}

    #endif /* VITTEC_INCLUDE_VITTEC_DIAG_SPAN_H */
