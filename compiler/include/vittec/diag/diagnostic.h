#ifndef VITTEC_INCLUDE_VITTEC_DIAG_DIAGNOSTIC_H
    #define VITTEC_INCLUDE_VITTEC_DIAG_DIAGNOSTIC_H

    #include <stdint.h>

    #include "vittec/diag/severity.h"
#include "vittec/diag/span.h"
#include "vittec/support/str.h"

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

    #endif /* VITTEC_INCLUDE_VITTEC_DIAG_DIAGNOSTIC_H */
