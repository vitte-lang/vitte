#include "vittec/diag/emitter.h"
#include <stdio.h>

static const char* sev_name(vittec_severity_t s) {
  switch (s) {
    case VITTEC_SEV_ERROR: return "error";
    case VITTEC_SEV_WARNING: return "warning";
    case VITTEC_SEV_NOTE: return "note";
    default: return "unknown";
  }
}

void vittec_emit_human(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags) {
  (void)sm;
  for (uint32_t i = 0; i < diags->len; i++) {
    const vittec_diagnostic_t* d = &diags->diags[i];
    fprintf(stderr, "%s: %.*s\n", sev_name(d->severity), (int)d->message.len, d->message.data);
  }
}
