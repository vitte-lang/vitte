// File: bench/src/bench/bench.h
// Vitte bench framework — public C API (C17/C23) — "max".
//
// Modules:
//   - Registry: bench_case_t, bench_registry_t, bench_register_all()
//   - Time: bench_clock_t + now/sleep/overhead/format/cpu/pin/spin
//   - Stats: bench_samples_t + bench_stats_t + trimming + helpers
//   - Optional auto-registration macro layer
//
// Build knobs:
//   -DBENCH_ENABLE_AUTOREG=1 : enable constructor-based auto-reg (GCC/Clang best-effort).
//   -DBENCH_NO_AUTOREG=1     : disable auto-reg even if enabled.
//   -DBENCH_API_STATIC=1     : allow static inline for tiny helpers (not heavily used here).
//
// Naming convention:
//   - bench name: "namespace.case" (stable id)
//
// Minimal usage (portable):
//   // bench_runtime.c
//   #include "bench/bench.h"
//   static void run(bench_ctx_t* ctx, void* st, uint64_t iters) { ... }
//   void bench_register_runtime(bench_registry_t* r) {
//     bench_register_case(r, (bench_case_t){ .name="runtime.foo", .description="...", .run=run });
//   }
//
// Runner:
//   #include "bench/bench.h"
//   int main() {
//     bench_registry_t reg; bench_registry_api_init(&reg);
//     bench_register_all(&reg);
//     ...
//   }

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <time.h>
#endif

// ======================================================================================
// Bench context
// ======================================================================================

typedef struct bench_ctx {
  uint64_t seed;
  int verbose;
  void* user; // optional user pointer (runner-defined)
} bench_ctx_t;

// ======================================================================================
// Bench case + registry
// ======================================================================================

typedef void* (*bench_setup_fn)(bench_ctx_t* ctx);
typedef void  (*bench_teardown_fn)(bench_ctx_t* ctx, void* state);
typedef void  (*bench_fn)(bench_ctx_t* ctx, void* state, uint64_t iters);

typedef struct bench_case {
  const char* name;           // required
  const char* description;    // optional
  bench_setup_fn setup;       // optional
  bench_teardown_fn teardown; // optional
  bench_fn run;               // required
  uint32_t flags;             // reserved
} bench_case_t;

typedef struct bench_registry {
  bench_case_t* cases;
  size_t len;
  size_t cap;
} bench_registry_t;

// Registry API (implemented in bench_registry.c)
void bench_registry_api_init(bench_registry_t* r);
void bench_registry_api_free(bench_registry_t* r);
void bench_registry_api_add(bench_registry_t* r, bench_case_t c);

// Convenience (portable): call inside bench_register_<module>()
static inline void bench_register_case(bench_registry_t* r, bench_case_t c) {
  bench_registry_api_add(r, c);
}

// Called by runner (implemented in bench_registry.c)
void bench_register_all(bench_registry_t* registry);

// Optional explicit module registration hooks (typically weak in bench_registry.c)
void bench_register_runtime(bench_registry_t* r);
void bench_register_compiler(bench_registry_t* r);
void bench_register_std(bench_registry_t* r);
void bench_register_vm(bench_registry_t* r);
void bench_register_muf(bench_registry_t* r);
void bench_register_regex(bench_registry_t* r);
void bench_register_unicode(bench_registry_t* r);

// ======================================================================================
// Auto-registration support (optional; best-effort)
// ======================================================================================

#ifndef BENCH_ENABLE_AUTOREG
  #define BENCH_ENABLE_AUTOREG 0
#endif

#ifndef BENCH_NO_AUTOREG
  #define BENCH_NO_AUTOREG 0
#endif

// Implemented in bench_registry.c
void bench_autoreg_register_case(const bench_case_t* c);

#if (BENCH_ENABLE_AUTOREG && !BENCH_NO_AUTOREG)
  #if defined(__GNUC__) || defined(__clang__)
    #define BENCH_CONSTRUCTOR __attribute__((constructor))
    #define BENCH_AUTOREG_ENABLED 1
  #else
    // Non-portable on MSVC without custom section tricks; prefer explicit module hooks.
    #define BENCH_CONSTRUCTOR
    #define BENCH_AUTOREG_ENABLED 0
  #endif
#else
  #define BENCH_CONSTRUCTOR
  #define BENCH_AUTOREG_ENABLED 0
#endif

// Declare a case and optionally auto-register it.
// RUN_FN must be a function with signature: void run(bench_ctx_t*, void*, uint64_t)
#define BENCH_CASE(NAME_STR, DESC_STR, SETUP_FN, TEARDOWN_FN, RUN_FN)              \
  static const bench_case_t RUN_FN##_bench_case = {                                \
    (NAME_STR), (DESC_STR), (SETUP_FN), (TEARDOWN_FN), (RUN_FN), 0u                \
  };                                                                               \
  BENCH_CONSTRUCTOR static void RUN_FN##_bench_autoreg(void) {                     \
    if (BENCH_AUTOREG_ENABLED) bench_autoreg_register_case(&RUN_FN##_bench_case);  \
  }

// ======================================================================================
// Time utilities (bench_time.c)
// ======================================================================================

// bench_clock_t needs full definition because some runners store it by value.
typedef struct bench_clock {
#if defined(_WIN32)
  LARGE_INTEGER qpc_freq;
#else
  int has_clock_gettime;
  clockid_t clock_id;
#endif
} bench_clock_t;

bench_clock_t bench_clock_init(void);
uint64_t bench_now_ns(const bench_clock_t* c);
void bench_sleep_ms(uint32_t ms);
uint64_t bench_estimate_timer_overhead_ns(const bench_clock_t* c, int iters);
const char* bench_format_duration(char* buf, size_t buf_len, uint64_t ns);
uint32_t bench_cpu_logical_count(void);
bool bench_pin_to_single_cpu(uint32_t cpu_index);
void bench_spin_wait_ns(const bench_clock_t* c, uint64_t ns);

// ======================================================================================
// Samples + stats (bench_stats.c)
// ======================================================================================

typedef struct bench_samples {
  double* v;
  size_t len;
  size_t cap;
} bench_samples_t;

void bench_samples_init(bench_samples_t* s);
void bench_samples_free(bench_samples_t* s);
void bench_samples_clear(bench_samples_t* s);
void bench_samples_reserve(bench_samples_t* s, size_t n);
void bench_samples_push(bench_samples_t* s, double x);
size_t bench_samples_len(const bench_samples_t* s);
const double* bench_samples_data(const bench_samples_t* s);

typedef struct bench_stats {
  // classic
  double min;
  double max;
  double mean;
  double stddev;
  double variance;

  // quantiles
  double q50; // median
  double q90;
  double q95;
  double q99;

  // robust
  double mad; // median absolute deviation
  double iqr; // interquartile range
  double q25;
  double q75;

  // counts
  size_t n;
  size_t n_finite;
  size_t n_outliers_iqr;
  size_t n_outliers_mad;
} bench_stats_t;

bench_stats_t bench_stats_compute(const bench_samples_t* s);
void bench_stats_debug_print(const bench_stats_t* st, const char* unit);

typedef enum bench_trim_mode {
  BENCH_TRIM_NONE = 0,
  BENCH_TRIM_IQR  = 1,
  BENCH_TRIM_MAD  = 2,
} bench_trim_mode_t;

bench_samples_t bench_samples_trimmed(const bench_samples_t* s, bench_trim_mode_t mode);

// conversions
double bench_stats_ops_per_s_from_ns_per_op(double ns_per_op);

#ifdef __cplusplus
} // extern "C"
#endif
