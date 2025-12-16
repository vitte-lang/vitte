// File: bench/src/bench/bench_main.c
// Vitte benchmark runner (C17/C23 compatible) — "max" version.
//
// Features:
// - Uses bench/bench.h API (registry + time + stats).
// - CLI: --list, --filter, --exclude, --iters, --warmup, --repeat, --min-time-ms,
//        --json, --csv, --seed, --pin-cpu, --sleep-ms, --trim, --no-overhead, -v.
// - Auto calibration (iters=0) to reach min-time target per sample.
// - Stats: min/max/mean/stddev/median/p90/p95/p99/MAD/IQR + outliers counts.
// - Optional trimming for reporting: none/iqr/mad.
// - JSON/CSV output.
// - Best-effort timer overhead compensation.
//
// Build expectation:
//   - bench_registry.c provides bench_register_all() (and optional module hooks).
//   - bench_time.c provides time utilities.
//   - bench_stats.c provides samples+stats.
//   - bench.h provides types/prototypes.
//
// NOTE: This runner does not depend on platform-specific code beyond bench_time.c.

#include "bench/bench.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

// ======================================================================================
// helpers
// ======================================================================================

static bool bench_streq(const char* a, const char* b) {
  if (a == b) return true;
  if (!a || !b) return false;
  return strcmp(a, b) == 0;
}

static bool bench_contains(const char* haystack, const char* needle) {
  if (!needle || needle[0] == '\0') return true;
  if (!haystack) return false;
  return strstr(haystack, needle) != NULL;
}

static void bench_die_usage(const char* argv0, const char* msg) {
  if (msg && msg[0]) fprintf(stderr, "bench: %s\n", msg);
  fprintf(stderr,
    "Usage: %s [options]\n"
    "\n"
    "Options:\n"
    "  --list                    List benchmarks\n"
    "  --filter <substr>          Run only benchmarks containing substring\n"
    "  --exclude <substr>         Exclude benchmarks containing substring\n"
    "  --iters <N>                Fixed iters per sample (0 => auto; default 0)\n"
    "  --warmup <N>               Warmup iters before measuring (0 => auto)\n"
    "  --repeat <K>               Samples per benchmark (default 15)\n"
    "  --min-time-ms <T>          Target ms per sample in auto mode (default 50)\n"
    "  --seed <u64>               Seed exposed to benches (default 1)\n"
    "  --pin-cpu <idx>            Pin current thread to CPU idx (best-effort)\n"
    "  --sleep-ms <ms>            Sleep before running (stabilize turbo) (default 0)\n"
    "  --trim <none|iqr|mad>      Trim samples for reporting (default none)\n"
    "  --no-overhead              Disable timer overhead compensation\n"
    "  --json <path>              Write results JSON\n"
    "  --csv <path>               Write results CSV\n"
    "  -v, --verbose              Verbose\n"
    "  -h, --help                 Help\n"
    "\n",
    argv0
  );
  exit(2);
}

static bool bench_parse_u64(const char* s, uint64_t* out) {
  if (!s || !out) return false;
  errno = 0;
  char* end = NULL;
  unsigned long long v = strtoull(s, &end, 10);
  if (errno != 0 || !end || *end != '\0') return false;
  *out = (uint64_t)v;
  return true;
}

static bool bench_parse_i32(const char* s, int* out) {
  if (!s || !out) return false;
  errno = 0;
  char* end = NULL;
  long v = strtol(s, &end, 10);
  if (errno != 0 || !end || *end != '\0') return false;
  if (v < -2147483647L - 1L || v > 2147483647L) return false;
  *out = (int)v;
  return true;
}

static bool bench_parse_f64(const char* s, double* out) {
  if (!s || !out) return false;
  errno = 0;
  char* end = NULL;
  double v = strtod(s, &end);
  if (errno != 0 || !end || *end != '\0') return false;
  *out = v;
  return true;
}

static bool bench_parse_trim_mode(const char* s, bench_trim_mode_t* out) {
  if (!s || !out) return false;
  if (bench_streq(s, "none")) { *out = BENCH_TRIM_NONE; return true; }
  if (bench_streq(s, "iqr"))  { *out = BENCH_TRIM_IQR;  return true; }
  if (bench_streq(s, "mad"))  { *out = BENCH_TRIM_MAD;  return true; }
  return false;
}

// ======================================================================================
// CLI config
// ======================================================================================

typedef struct bench_cli {
  bool list_only;
  const char* filter;
  const char* exclude;

  uint64_t iters;        // fixed iters per sample (0 => auto)
  uint64_t warmup;       // warmup iters (0 => auto)
  int repeat;            // samples
  double min_time_ms;    // auto target

  uint64_t seed;
  int verbose;

  bool no_overhead;
  uint32_t pin_cpu;      // UINT32_MAX => none
  uint32_t sleep_ms;

  bench_trim_mode_t trim;

  const char* json_out;
  const char* csv_out;
} bench_cli_t;

static bench_cli_t bench_cli_default(void) {
  bench_cli_t c;
  c.list_only = false;
  c.filter = NULL;
  c.exclude = NULL;

  c.iters = 0;
  c.warmup = 0;
  c.repeat = 15;
  c.min_time_ms = 50.0;

  c.seed = 1;
  c.verbose = 0;

  c.no_overhead = false;
  c.pin_cpu = UINT32_MAX;
  c.sleep_ms = 0;

  c.trim = BENCH_TRIM_NONE;

  c.json_out = NULL;
  c.csv_out = NULL;
  return c;
}

static bench_cli_t bench_cli_parse(int argc, char** argv) {
  bench_cli_t c = bench_cli_default();

  for (int i = 1; i < argc; i++) {
    const char* a = argv[i];

    if (bench_streq(a, "--list")) {
      c.list_only = true;

    } else if (bench_streq(a, "--filter")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--filter requires value");
      c.filter = argv[++i];

    } else if (bench_streq(a, "--exclude")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--exclude requires value");
      c.exclude = argv[++i];

    } else if (bench_streq(a, "--iters")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--iters requires value");
      if (!bench_parse_u64(argv[++i], &c.iters)) bench_die_usage(argv[0], "invalid --iters");
      // allow 0 => auto

    } else if (bench_streq(a, "--warmup")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--warmup requires value");
      if (!bench_parse_u64(argv[++i], &c.warmup)) bench_die_usage(argv[0], "invalid --warmup");

    } else if (bench_streq(a, "--repeat")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--repeat requires value");
      if (!bench_parse_i32(argv[++i], &c.repeat) || c.repeat <= 0 || c.repeat > 10000) {
        bench_die_usage(argv[0], "invalid --repeat");
      }

    } else if (bench_streq(a, "--min-time-ms")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--min-time-ms requires value");
      if (!bench_parse_f64(argv[++i], &c.min_time_ms) || c.min_time_ms <= 0.0) {
        bench_die_usage(argv[0], "invalid --min-time-ms");
      }

    } else if (bench_streq(a, "--seed")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--seed requires value");
      if (!bench_parse_u64(argv[++i], &c.seed)) bench_die_usage(argv[0], "invalid --seed");

    } else if (bench_streq(a, "--pin-cpu")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--pin-cpu requires idx");
      uint64_t tmp = 0;
      if (!bench_parse_u64(argv[++i], &tmp) || tmp > 0xffffffffULL) bench_die_usage(argv[0], "invalid --pin-cpu");
      c.pin_cpu = (uint32_t)tmp;

    } else if (bench_streq(a, "--sleep-ms")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--sleep-ms requires ms");
      uint64_t tmp = 0;
      if (!bench_parse_u64(argv[++i], &tmp) || tmp > 0xffffffffULL) bench_die_usage(argv[0], "invalid --sleep-ms");
      c.sleep_ms = (uint32_t)tmp;

    } else if (bench_streq(a, "--trim")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--trim requires value");
      if (!bench_parse_trim_mode(argv[++i], &c.trim)) bench_die_usage(argv[0], "invalid --trim (use none|iqr|mad)");

    } else if (bench_streq(a, "--no-overhead")) {
      c.no_overhead = true;

    } else if (bench_streq(a, "--json")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--json requires path");
      c.json_out = argv[++i];

    } else if (bench_streq(a, "--csv")) {
      if (i + 1 >= argc) bench_die_usage(argv[0], "--csv requires path");
      c.csv_out = argv[++i];

    } else if (bench_streq(a, "-v") || bench_streq(a, "--verbose")) {
      c.verbose++;

    } else if (bench_streq(a, "-h") || bench_streq(a, "--help")) {
      bench_die_usage(argv[0], NULL);

    } else {
      char buf[256];
#if defined(_MSC_VER)
      _snprintf_s(buf, sizeof(buf), _TRUNCATE, "unknown arg: %s", a);
#else
      snprintf(buf, sizeof(buf), "unknown arg: %s", a);
#endif
      bench_die_usage(argv[0], buf);
    }
  }

  return c;
}

// ======================================================================================
// selection + calibration
// ======================================================================================

static bool bench_case_selected(const bench_case_t* bc, const bench_cli_t* cli) {
  if (cli->filter && !bench_contains(bc->name, cli->filter)) return false;
  if (cli->exclude && bench_contains(bc->name, cli->exclude)) return false;
  return true;
}

static uint64_t bench_choose_auto_warmup(uint64_t iters_fixed) {
  if (iters_fixed > 0) {
    uint64_t w = iters_fixed / 10;
    return (w > 0) ? w : 1;
  }
  return 1000;
}

static uint64_t bench_calibrate_iters(
  const bench_clock_t* clk,
  bench_ctx_t* ctx,
  const bench_case_t* bc,
  void* state,
  double min_time_ms,
  uint64_t timer_overhead_ns
) {
  const uint64_t cap = 1ULL << 34;
  uint64_t iters = 1;

  // Exponential search for target time
  for (int step = 0; step < 64; step++) {
    uint64_t t0 = bench_now_ns(clk);
    bc->run(ctx, state, iters);
    uint64_t t1 = bench_now_ns(clk);

    uint64_t dt = t1 - t0;
    dt = (dt > timer_overhead_ns) ? (dt - timer_overhead_ns) : dt;

    double ms = (double)dt / 1000000.0;
    if (ms >= min_time_ms) return iters;

    if (iters >= cap / 2) return cap;
    iters *= 2;
  }
  return iters;
}

// ======================================================================================
// results container
// ======================================================================================

typedef struct bench_result {
  const char* name;
  const char* description;
  uint64_t iters_total;

  bench_samples_t raw_ns_per_op;      // samples collected (ns/op)
  bench_samples_t report_ns_per_op;   // after trim mode applied

  bench_stats_t raw_stats;
  bench_stats_t report_stats;

  double ns_per_op; // representative (median of report)
  double ops_per_s; // 1e9 / ns_per_op
} bench_result_t;

typedef struct bench_results {
  bench_result_t* v;
  size_t len;
  size_t cap;
} bench_results_t;

static void bench_results_init(bench_results_t* r) { r->v = NULL; r->len = 0; r->cap = 0; }
static void bench_results_free(bench_results_t* r) {
  if (!r) return;
  for (size_t i = 0; i < r->len; i++) {
    bench_samples_free(&r->v[i].raw_ns_per_op);
    bench_samples_free(&r->v[i].report_ns_per_op);
  }
  free(r->v);
  r->v = NULL;
  r->len = 0;
  r->cap = 0;
}
static void bench_results_push(bench_results_t* r, bench_result_t x) {
  if (r->len == r->cap) {
    r->cap = (r->cap == 0) ? 16 : (r->cap * 2);
    r->v = (bench_result_t*)realloc(r->v, r->cap * sizeof(bench_result_t));
    if (!r->v) { fprintf(stderr, "bench: OOM\n"); exit(1); }
  }
  r->v[r->len++] = x;
}

// ======================================================================================
// Reporting
// ======================================================================================

static void bench_print_table(const bench_results_t* results) {
  printf("\n%-44s %12s %12s %12s %12s %8s\n",
    "benchmark", "ns/op", "ops/s", "p90", "p99", "n");
  printf("%-44s %12s %12s %12s %12s %8s\n",
    "--------------------------------------------", "------------", "------------", "------------", "------------", "--------");

  for (size_t i = 0; i < results->len; i++) {
    const bench_result_t* r = &results->v[i];
    printf("%-44s %12.2f %12.0f %12.2f %12.2f %8zu\n",
      r->name,
      r->ns_per_op,
      r->ops_per_s,
      r->report_stats.q90,
      r->report_stats.q99,
      r->report_stats.n_finite
    );
  }
  printf("\n");
}

static void bench_write_json(const char* path, const bench_results_t* results, const bench_cli_t* cli) {
  FILE* f = fopen(path, "wb");
  if (!f) {
    fprintf(stderr, "bench: cannot open json output '%s': %s\n", path, strerror(errno));
    exit(3);
  }

  fprintf(f, "{\n");
  fprintf(f, "  \"config\": {\n");
  fprintf(f, "    \"iters\": %" PRIu64 ",\n", cli->iters);
  fprintf(f, "    \"warmup\": %" PRIu64 ",\n", cli->warmup);
  fprintf(f, "    \"repeat\": %d,\n", cli->repeat);
  fprintf(f, "    \"min_time_ms\": %.6f,\n", cli->min_time_ms);
  fprintf(f, "    \"seed\": %" PRIu64 ",\n", cli->seed);
  fprintf(f, "    \"trim\": \"%s\",\n", (cli->trim == BENCH_TRIM_NONE ? "none" : (cli->trim == BENCH_TRIM_IQR ? "iqr" : "mad")));
  fprintf(f, "    \"no_overhead\": %s,\n", cli->no_overhead ? "true" : "false");
  fprintf(f, "    \"pin_cpu\": %s,\n", (cli->pin_cpu == UINT32_MAX ? "null" : "0"));
  if (cli->pin_cpu != UINT32_MAX) fprintf(f, "    \"pin_cpu_value\": %u,\n", cli->pin_cpu);
  fprintf(f, "    \"sleep_ms\": %u\n", cli->sleep_ms);
  fprintf(f, "  },\n");

  fprintf(f, "  \"results\": [\n");
  for (size_t i = 0; i < results->len; i++) {
    const bench_result_t* r = &results->v[i];
    const bench_stats_t* st = &r->report_stats;

    fprintf(f,
      "    {\n"
      "      \"name\": \"%s\",\n"
      "      \"iters_total\": %" PRIu64 ",\n"
      "      \"ns_per_op\": %.9f,\n"
      "      \"ops_per_s\": %.9f,\n"
      "      \"stats\": {\n"
      "        \"min\": %.9f,\n"
      "        \"max\": %.9f,\n"
      "        \"mean\": %.9f,\n"
      "        \"stddev\": %.9f,\n"
      "        \"median\": %.9f,\n"
      "        \"p90\": %.9f,\n"
      "        \"p95\": %.9f,\n"
      "        \"p99\": %.9f,\n"
      "        \"mad\": %.9f,\n"
      "        \"iqr\": %.9f,\n"
      "        \"q25\": %.9f,\n"
      "        \"q75\": %.9f,\n"
      "        \"n\": %zu,\n"
      "        \"n_finite\": %zu,\n"
      "        \"outliers_iqr\": %zu,\n"
      "        \"outliers_mad\": %zu\n"
      "      }\n"
      "    }%s\n",
      r->name,
      r->iters_total,
      r->ns_per_op,
      r->ops_per_s,
      st->min, st->max, st->mean, st->stddev, st->q50, st->q90, st->q95, st->q99,
      st->mad, st->iqr, st->q25, st->q75,
      st->n, st->n_finite, st->n_outliers_iqr, st->n_outliers_mad,
      (i + 1 < results->len) ? "," : ""
    );
  }
  fprintf(f, "  ]\n}\n");
  fclose(f);
}

static void bench_write_csv(const char* path, const bench_results_t* results) {
  FILE* f = fopen(path, "wb");
  if (!f) {
    fprintf(stderr, "bench: cannot open csv output '%s': %s\n", path, strerror(errno));
    exit(3);
  }
  fprintf(f, "name,iters_total,ns_per_op,ops_per_s,min,mean,median,p90,p95,p99,max,stddev,mad,iqr,q25,q75,n_finite,out_iqr,out_mad\n");
  for (size_t i = 0; i < results->len; i++) {
    const bench_result_t* r = &results->v[i];
    const bench_stats_t* st = &r->report_stats;
    fprintf(f,
      "\"%s\",%" PRIu64 ",%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%zu,%zu,%zu\n",
      r->name,
      r->iters_total,
      r->ns_per_op,
      r->ops_per_s,
      st->min, st->mean, st->q50, st->q90, st->q95, st->q99, st->max, st->stddev,
      st->mad, st->iqr, st->q25, st->q75,
      st->n_finite, st->n_outliers_iqr, st->n_outliers_mad
    );
  }
  fclose(f);
}

// ======================================================================================
// Run one bench
// ======================================================================================

static bench_result_t bench_run_one(
  const bench_clock_t* clk,
  const bench_cli_t* cli,
  bench_ctx_t* ctx,
  const bench_case_t* bc,
  uint64_t timer_overhead_ns
) {
  bench_result_t out;
  memset(&out, 0, sizeof(out));
  out.name = bc->name;
  out.description = bc->description ? bc->description : "";
  out.iters_total = 0;

  bench_samples_init(&out.raw_ns_per_op);
  bench_samples_init(&out.report_ns_per_op);

  void* state = NULL;
  if (bc->setup) state = bc->setup(ctx);

  uint64_t warmup_iters = (cli->warmup != 0) ? cli->warmup : bench_choose_auto_warmup(cli->iters);
  if (warmup_iters > 0) {
    bc->run(ctx, state, warmup_iters);
  }

  uint64_t iters_per_sample = cli->iters;

  for (int k = 0; k < cli->repeat; k++) {
    if (iters_per_sample == 0) {
      iters_per_sample = bench_calibrate_iters(clk, ctx, bc, state, cli->min_time_ms, timer_overhead_ns);
      if (iters_per_sample == 0) iters_per_sample = 1;
    }

    uint64_t t0 = bench_now_ns(clk);
    bc->run(ctx, state, iters_per_sample);
    uint64_t t1 = bench_now_ns(clk);

    uint64_t dt = t1 - t0;
    if (!cli->no_overhead) {
      dt = (dt > timer_overhead_ns) ? (dt - timer_overhead_ns) : dt;
    }

    double ns_per_op = (double)dt / (double)iters_per_sample;
    bench_samples_push(&out.raw_ns_per_op, ns_per_op);
    out.iters_total += iters_per_sample;

    if (cli->iters == 0) iters_per_sample = 0;
  }

  if (bc->teardown) bc->teardown(ctx, state);

  out.raw_stats = bench_stats_compute(&out.raw_ns_per_op);

  // trimming for reporting
  if (cli->trim == BENCH_TRIM_NONE) {
    // copy raw into report
    bench_samples_reserve(&out.report_ns_per_op, out.raw_ns_per_op.len);
    for (size_t i = 0; i < out.raw_ns_per_op.len; i++) bench_samples_push(&out.report_ns_per_op, out.raw_ns_per_op.v[i]);
  } else {
    out.report_ns_per_op = bench_samples_trimmed(&out.raw_ns_per_op, cli->trim);
  }

  out.report_stats = bench_stats_compute(&out.report_ns_per_op);

  // Representative: median of report
  out.ns_per_op = out.report_stats.q50;
  out.ops_per_s = bench_stats_ops_per_s_from_ns_per_op(out.ns_per_op);

  return out;
}

// ======================================================================================
// Main
// ======================================================================================

int main(int argc, char** argv) {
  bench_cli_t cli = bench_cli_parse(argc, argv);

  if (cli.pin_cpu != UINT32_MAX) {
    (void)bench_pin_to_single_cpu(cli.pin_cpu);
  }
  if (cli.sleep_ms != 0) {
    bench_sleep_ms(cli.sleep_ms);
  }

  bench_clock_t clk = bench_clock_init();
  uint64_t overhead_ns = cli.no_overhead ? 0 : bench_estimate_timer_overhead_ns(&clk, 20000);

  if (cli.verbose) {
    fprintf(stdout, "bench: cpu logical=%u\n", bench_cpu_logical_count());
    fprintf(stdout, "bench: timer overhead ~ %" PRIu64 " ns\n", overhead_ns);
  }

  bench_ctx_t ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.seed = cli.seed;
  ctx.verbose = cli.verbose;
  ctx.user = NULL;

  bench_registry_t reg;
  bench_registry_api_init(&reg);

  bench_register_all(&reg);

  if (reg.len == 0) {
    fprintf(stderr, "bench: registry empty (no benchmarks registered)\n");
    bench_registry_api_free(&reg);
    return 2;
  }

  if (cli.list_only) {
    for (size_t i = 0; i < reg.len; i++) {
      const bench_case_t* bc = &reg.cases[i];
      if (!bench_case_selected(bc, &cli)) continue;
      if (bc->description && bc->description[0]) {
        printf("%s — %s\n", bc->name, bc->description);
      } else {
        printf("%s\n", bc->name);
      }
    }
    bench_registry_api_free(&reg);
    return 0;
  }

  bench_results_t results;
  bench_results_init(&results);

  int selected = 0;
  for (size_t i = 0; i < reg.len; i++) {
    const bench_case_t* bc = &reg.cases[i];
    if (!bench_case_selected(bc, &cli)) continue;
    selected++;

    if (cli.verbose) {
      fprintf(stdout, "bench: running %s\n", bc->name);
    }

    bench_result_t r = bench_run_one(&clk, &cli, &ctx, bc, overhead_ns);
    bench_results_push(&results, r);

    // per-bench line
    printf("%-44s  %10.2f ns/op  %10.0f ops/s  (p90=%.2f p99=%.2f n=%zu out_iqr=%zu out_mad=%zu)\n",
      r.name,
      r.ns_per_op,
      r.ops_per_s,
      r.report_stats.q90,
      r.report_stats.q99,
      r.report_stats.n_finite,
      r.report_stats.n_outliers_iqr,
      r.report_stats.n_outliers_mad
    );
  }

  if (selected == 0) {
    fprintf(stderr, "bench: no benchmark selected (filter='%s', exclude='%s')\n",
      cli.filter ? cli.filter : "",
      cli.exclude ? cli.exclude : ""
    );
    bench_results_free(&results);
    bench_registry_api_free(&reg);
    return 2;
  }

  bench_print_table(&results);

  if (cli.json_out) {
    bench_write_json(cli.json_out, &results, &cli);
    if (cli.verbose) fprintf(stdout, "bench: wrote json: %s\n", cli.json_out);
  }
  if (cli.csv_out) {
    bench_write_csv(cli.csv_out, &results);
    if (cli.verbose) fprintf(stdout, "bench: wrote csv: %s\n", cli.csv_out);
  }

  bench_results_free(&results);
  bench_registry_api_free(&reg);
  return 0;
}
