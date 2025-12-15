#include "steel/diag/diag.h"
#include <stdio.h>

void steel_diag_emit(steel_diag_level lvl, steel_span sp, const char* msg) {
  const char* k = (lvl==STEEL_DIAG_ERROR) ? "error" : (lvl==STEEL_DIAG_WARN) ? "warn" : "note";
  fprintf(stderr, "%s: file=%u span=[%u..%u]: %s\n", k, sp.file_id, sp.lo, sp.hi, msg ? msg : "(null)");
}
