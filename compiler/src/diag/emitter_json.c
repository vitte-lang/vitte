#include "vittec/diag/emitter.h"
#include <stdio.h>

void vittec_emit_json(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags) {
  (void)sm;
  fprintf(stderr, "{\n  \"diagnostics\": [\n");
  for (uint32_t i = 0; i < diags->len; i++) {
    const vittec_diagnostic_t* d = &diags->diags[i];
    fprintf(stderr,
      "    { \"severity\": %d, \"message\": \"%.*s\", \"file\": %u, \"lo\": %u, \"hi\": %u }%s\n",
      (int)d->severity, (int)d->message.len, d->message.data,
      d->span.file_id, d->span.lo, d->span.hi,
      (i + 1 == diags->len) ? "" : ","
    );
  }
  fprintf(stderr, "  ]\n}\n");
}
