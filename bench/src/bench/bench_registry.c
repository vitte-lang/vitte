#include "bench/bench.h"

/* micro */
void bm_add(void* ctx);
void bm_hash(void* ctx);
void bm_memcpy(void* ctx);

/* macro */
void bm_json_parse(void* ctx);

static bench_case g_cases[] = {
  { "micro:add",        BENCH_MICRO, bm_add,        0 },
  { "micro:hash",       BENCH_MICRO, bm_hash,       0 },
  { "micro:memcpy",     BENCH_MICRO, bm_memcpy,     0 },
  { "macro:json_parse", BENCH_MACRO, bm_json_parse, 0 },
};

const bench_case* bench_registry_all(int* out_count) {
  if(out_count) *out_count = (int)(sizeof(g_cases) / sizeof(g_cases[0]));
  return g_cases;
}
