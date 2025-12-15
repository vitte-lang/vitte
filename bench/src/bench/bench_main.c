#include "bench/bench.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void usage(void) {
  fprintf(stderr,
    "benchc [--list] [--iters N] [--samples N] [--seconds S] [--csv FILE] <case...>\n"
    "Examples:\n"
    "  benchc --list\n"
    "  benchc --iters 2000000 micro:add\n"
    "  benchc --seconds 2 macro:json_parse\n"
    "  benchc --csv out.csv micro:add micro:hash\n"
  );
}

static const bench_case* find_case(const char* id) {
  int n = 0;
  const bench_case* all = bench_registry_all(&n);
  for(int i=0;i<n;i++) if(strcmp(all[i].id, id) == 0) return &all[i];
  return NULL;
}

static int list_cases(void) {
  int n = 0;
  const bench_case* all = bench_registry_all(&n);
  for(int i=0;i<n;i++) printf("%s\n", all[i].id);
  return 0;
}

static void warmup(const bench_case* c) {
  for(int i=0;i<1000;i++) c->fn(c->ctx);
}

static int run_micro(const bench_case* c, int iters, int samples, FILE* csv) {
  warmup(c);
  double* ss = (double*)malloc((size_t)samples * sizeof(double));
  if(!ss) return 1;

  for(int s=0;s<samples;s++) {
    uint64_t t0 = bench_now_ns();
    for(int i=0;i<iters;i++) c->fn(c->ctx);
    uint64_t t1 = bench_now_ns();
    ss[s] = (double)(t1 - t0) / (double)iters; /* ns/op */
  }

  bench_stats st = bench_compute_stats(ss, samples);
  printf("%-16s mean=%.2f ns/op  p50=%.2f  p95=%.2f  min=%.2f  max=%.2f\n",
    c->id, st.mean, st.p50, st.p95, st.min, st.max);

  if(csv) {
    fprintf(csv, "%s,%.6f,%.6f,%.6f,%.6f,%.6f\n", c->id, st.mean, st.p50, st.p95, st.min, st.max);
  }

  free(ss);
  return 0;
}

static int run_macro(const bench_case* c, double seconds, int samples, FILE* csv) {
  /* macro: estimate ops/s by calling fn in a loop for ~seconds */
  warmup(c);
  double* ss = (double*)malloc((size_t)samples * sizeof(double));
  if(!ss) return 1;

  for(int s=0;s<samples;s++) {
    uint64_t end = bench_now_ns() + (uint64_t)(seconds * 1e9);
    uint64_t it = 0;
    while(bench_now_ns() < end) { c->fn(c->ctx); it++; }
    ss[s] = (double)it / seconds; /* ops/s */
  }

  bench_stats st = bench_compute_stats(ss, samples);
  printf("%-16s mean=%.2f ops/s  p50=%.2f  p95=%.2f  min=%.2f  max=%.2f\n",
    c->id, st.mean, st.p50, st.p95, st.min, st.max);

  if(csv) {
    fprintf(csv, "%s,%.6f,%.6f,%.6f,%.6f,%.6f\n", c->id, st.mean, st.p50, st.p95, st.min, st.max);
  }

  free(ss);
  return 0;
}

int bench_run(int argc, char** argv) {
  int iters = 1000000;
  int samples = 7;
  double seconds = 2.0;
  const char* csv_path = NULL;

  int i = 1;
  for(; i<argc; i++) {
    if(strcmp(argv[i], "--list") == 0) return list_cases();
    if(strcmp(argv[i], "--iters") == 0 && i+1<argc) { iters = atoi(argv[++i]); continue; }
    if(strcmp(argv[i], "--samples") == 0 && i+1<argc) { samples = atoi(argv[++i]); continue; }
    if(strcmp(argv[i], "--seconds") == 0 && i+1<argc) { seconds = atof(argv[++i]); continue; }
    if(strcmp(argv[i], "--csv") == 0 && i+1<argc) { csv_path = argv[++i]; continue; }
    if(argv[i][0] == '-') { usage(); return 2; }
    break;
  }

  if(i >= argc) { usage(); return 2; }

  FILE* csv = NULL;
  if(csv_path) {
    csv = fopen(csv_path, "wb");
    if(!csv) { fprintf(stderr, "cannot open csv: %s\n", csv_path); return 1; }
    fprintf(csv, "id,mean,p50,p95,min,max\n");
  }

  int rc = 0;
  for(; i<argc; i++) {
    const bench_case* c = find_case(argv[i]);
    if(!c) { fprintf(stderr, "unknown case: %s\n", argv[i]); rc = 1; continue; }
    if(c->kind == BENCH_MICRO) rc |= run_micro(c, iters, samples, csv);
    else rc |= run_macro(c, seconds, samples, csv);
  }

  if(csv) fclose(csv);
  return rc;
}

int main(int argc, char** argv) { return bench_run(argc, argv); }
