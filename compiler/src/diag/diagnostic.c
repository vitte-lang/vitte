#include "vittec/diag/diagnostic.h"
#include "vittec/support/assert.h"
#include <stdlib.h>

void vittec_diag_sink_init(vittec_diag_sink_t* s) {
  s->diags = NULL;
  s->len = 0;
  s->cap = 0;
}

void vittec_diag_sink_free(vittec_diag_sink_t* s) {
  if (s->diags) free(s->diags);
  s->diags = NULL;
  s->len = 0;
  s->cap = 0;
}

void vittec_diag_push(vittec_diag_sink_t* s, vittec_severity_t sev, vittec_span_t span, vittec_sv_t msg) {
  if (s->len == s->cap) {
    uint32_t new_cap = s->cap ? s->cap * 2 : 16;
    vittec_diagnostic_t* p = (vittec_diagnostic_t*)realloc(s->diags, (size_t)new_cap * sizeof(vittec_diagnostic_t));
    VITTEC_ASSERT(p != NULL);
    s->diags = p;
    s->cap = new_cap;
  }
  s->diags[s->len].severity = sev;
  s->diags[s->len].span = span;
  s->diags[s->len].message = msg;
  s->len++;
}
