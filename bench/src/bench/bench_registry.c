#include "bench/bench.h"

/*
  bench_registry.c (max)

  Objectif:
  - Registry simple, stable, utilisant un registre dynamique via bench_registry_add().
  - Les benchmarks sont enregistrés au startup via benchmark_init.c.
  - Pas de hardcoding des benchmarks individuels.
*/

#define MAX_CASES 256

static bench_case_t g_cases[MAX_CASES];
static int g_count = 0;

int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx) {
  if (g_count >= MAX_CASES) {
    return 0;  /* Registry full */
  }
  
  g_cases[g_count].id = id;
  g_cases[g_count].kind = (bench_kind_t)kind;
  g_cases[g_count].fn = fn;
  g_cases[g_count].ctx = ctx;
  g_count++;
  return 1;
}

const bench_case_t* bench_registry_all(int* out_count) {
  if (out_count) *out_count = g_count;
  return g_cases;
}
