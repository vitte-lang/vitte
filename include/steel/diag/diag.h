#pragma once
#include <stdint.h>
#include "steel/diag/span.h"

typedef enum {
  STEEL_DIAG_ERROR = 1,
  STEEL_DIAG_WARN  = 2,
  STEEL_DIAG_NOTE  = 3,
} steel_diag_level;

void steel_diag_emit(steel_diag_level lvl, steel_span sp, const char* msg);
