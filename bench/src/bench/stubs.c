#include "alloc.h"

#include <stddef.h>
#include <stdint.h>

// Optional providers not yet implemented in this configuration.
const vitte_bm_case*
vitte_bm_micro_list_cases(size_t* out_count) {
  if (out_count) *out_count = 0;
  return NULL;
}

const vitte_bm_case*
vitte_bm_macro_list_cases(size_t* out_count) {
  if (out_count) *out_count = 0;
  return NULL;
}

const vitte_bm_case*
vitte_bm_json_list_cases(size_t* out_count) {
  if (out_count) *out_count = 0;
  return NULL;
}

uint64_t
bench_asm_cycles_begin(void) {
  return 0;
}

uint64_t
bench_asm_cycles_end(void) {
  return 0;
}
