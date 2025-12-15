#include "bench/bench.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static void usage(FILE* out) {
  fprintf(out,
    "benchc [--list|--list-full] [--all] [--filter SUBSTR] [--iters N] [--samples N] [--seconds S] [--warmup N] [--timecheck N] [--csv FILE] <case...>\n"
    "\n"
    "Options:\n"
    "  --help, -h         Show help\n"
    "  --list             List benchmark case ids\n"
    "  --list-full        List ids with kind (micro/macro)\n"
    "  --all              Run all registered benchmarks (can be combined with --filter)\n"
    "  --filter SUBSTR    Only include cases whose id contains SUBSTR\n"
    "  --iters N          Micro: iterations per sample (default 1000000)\n"
    "  --samples N        Number of samples (default 7)\n"
    "  --seconds S        Macro: seconds per sample (default 2.0)\n"
    "  --warmup N         Warmup calls before sampling (default 1000)\n"
    "  --timecheck N      Macro: check clock every N iterations (default 256)\n"
    "  --csv FILE         Write results as CSV\n"
    "\n"
    "Examples:\n"
    "  benchc --list\n"
    "  benchc --iters 2000000 micro:add\n"
    "  benchc --seconds 2 macro:json_parse\n"
    "  benchc --all --filter hash\n"
    "  benchc --csv out.csv micro:add micro:hash\n"
  );
}

static int parse_i32(const char* s, int* out) {
  if(!s || !*s) return 0;
  errno = 0;
  char* end = NULL;
  long v = strtol(s, &end, 10);
  if(errno != 0 || end == s || (end && *end != '\0')) return 0;
  if(v < -2147483647L - 1L || v > 2147483647L) return 0;
  *out = (int)v;
  return 1;
}

static int parse_f64(const char* s, double* out) {
  if(!s || !*s) return 0;
  errno = 0;
  char* end = NULL;
  double v = strtod(s, &end);
  if(errno != 0 || end == s || (end && *end != '\0')) return 0;
  *out = v;
  return 1;
}

static const bench_case* find_case(const char* id) {
  int n = 0;
  const bench_case* all = bench_registry_all(&n);
  for(int i=0;i<n;i++) if(strcmp(all[i].id, id) == 0) return &all[i];
  return NULL;
}

static int list_cases(int full) {
  int n = 0;
  const bench_case* all = bench_registry_all(&n);
  for(int i=0;i<n;i++) {
    if(!full) {
      printf("%s\n", all[i].id);
    } else {
      const char* k = (all[i].kind == BENCH_MICRO) ? "micro" : "macro";
      printf("%-18s %s\n", all[i].id, k);
    }
  }
  return 0;
}

static int id_matches_filter(const char* id, const char* filter) {
  if(!filter || !*filter) return 1;
  return strstr(id, filter) != NULL;
}

static void warmup(const bench_case* c, int warmup_calls) {
  for(int i=0;i<warmup_calls;i++) c->fn(c->ctx);
}

static int run_micro(const bench_case* c, int iters, int samples, int warmup_calls, FILE* csv) {
  warmup(c, warmup_calls);
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

static int run_macro(const bench_case* c, double seconds, int samples, int warmup_calls, int timecheck, FILE* csv) {
  /* macro: estimate ops/s by calling fn in a loop for ~seconds
     Reduce timer overhead by checking time every `timecheck` iterations. */
  if(timecheck <= 0) timecheck = 256;

  warmup(c, warmup_calls);
  double* ss = (double*)malloc((size_t)samples * sizeof(double));
  if(!ss) return 1;

  for(int s=0;s<samples;s++) {
    const uint64_t end = bench_now_ns() + (uint64_t)(seconds * 1e9);
    uint64_t it = 0;
    uint64_t now = bench_now_ns();

    while(now < end) {
      /* Run a small batch, then check clock. */
      for(int k=0;k<timecheck;k++) {
        c->fn(c->ctx);
        it++;
      }
      now = bench_now_ns();
    }

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
  int warmup_calls = 1000;
  int timecheck = 256;
  const char* csv_path = NULL;
  int do_all = 0;
  const char* filter = NULL;

  int i = 1;
  for(; i<argc; i++) {
    const char* a = argv[i];

    if(strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) { usage(stdout); return 0; }
    if(strcmp(a, "--list") == 0) return list_cases(0);
    if(strcmp(a, "--list-full") == 0) return list_cases(1);
    if(strcmp(a, "--all") == 0) { do_all = 1; continue; }

    if(strcmp(a, "--filter") == 0 && i+1<argc) { filter = argv[++i]; continue; }

    if(strcmp(a, "--iters") == 0 && i+1<argc) {
      if(!parse_i32(argv[++i], &iters) || iters <= 0) { fprintf(stderr, "invalid --iters\n"); return 2; }
      continue;
    }

    if(strcmp(a, "--samples") == 0 && i+1<argc) {
      if(!parse_i32(argv[++i], &samples) || samples <= 0) { fprintf(stderr, "invalid --samples\n"); return 2; }
      continue;
    }

    if(strcmp(a, "--seconds") == 0 && i+1<argc) {
      if(!parse_f64(argv[++i], &seconds) || seconds <= 0.0) { fprintf(stderr, "invalid --seconds\n"); return 2; }
      continue;
    }

    if(strcmp(a, "--warmup") == 0 && i+1<argc) {
      if(!parse_i32(argv[++i], &warmup_calls) || warmup_calls < 0) { fprintf(stderr, "invalid --warmup\n"); return 2; }
      continue;
    }

    if(strcmp(a, "--timecheck") == 0 && i+1<argc) {
      if(!parse_i32(argv[++i], &timecheck) || timecheck <= 0) { fprintf(stderr, "invalid --timecheck\n"); return 2; }
      continue;
    }

    if(strcmp(a, "--csv") == 0 && i+1<argc) { csv_path = argv[++i]; continue; }

    if(a[0] == '-') { usage(stderr); return 2; }
    break;
  }

  FILE* csv = NULL;
  if(csv_path) {
    csv = fopen(csv_path, "wb");
    if(!csv) { fprintf(stderr, "cannot open csv: %s\n", csv_path); return 1; }
    fprintf(csv, "id,mean,p50,p95,min,max\n");
  }

  int rc = 0;

  if(do_all) {
    int n = 0;
    const bench_case* all = bench_registry_all(&n);
    for(int j=0;j<n;j++) {
      const bench_case* c = &all[j];
      if(!id_matches_filter(c->id, filter)) continue;
      if(c->kind == BENCH_MICRO) rc |= run_micro(c, iters, samples, warmup_calls, csv);
      else rc |= run_macro(c, seconds, samples, warmup_calls, timecheck, csv);
    }
  } else {
    if(i >= argc) { usage(stderr); if(csv) fclose(csv); return 2; }

    for(; i<argc; i++) {
      if(!id_matches_filter(argv[i], filter)) continue;
      const bench_case* c = find_case(argv[i]);
      if(!c) { fprintf(stderr, "unknown case: %s\n", argv[i]); rc = 1; continue; }
      if(c->kind == BENCH_MICRO) rc |= run_micro(c, iters, samples, warmup_calls, csv);
      else rc |= run_macro(c, seconds, samples, warmup_calls, timecheck, csv);
    }
  }

  if(csv) fclose(csv);
  return rc;
}

int main(int argc, char** argv) { return bench_run(argc, argv); }
