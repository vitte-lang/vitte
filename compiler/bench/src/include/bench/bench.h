

/*
  bench/bench.h

  Benchmark harness public API (C17).

  This header defines the stable interface for the Vitte benchmark subsystem.

  Scope
  - Define benchmark cases and suites.
  - Provide a runner that:
      - discovers and executes cases
      - records timing samples
      - computes summary statistics
      - emits reports (text/JSON)

  Design constraints
  - Portable C17 (Windows/macOS/Linux/BSD).
  - Deterministic formatting and parsing.
  - Minimal dependencies: relies on bench/* modules.

  Build model
  - This API is intended to be linked into the `vitte-bench` CLI and CI tooling.
*/

#pragma once
#ifndef VITTE_BENCH_BENCH_H
#define VITTE_BENCH_BENCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/assert.h"
#include "bench/arena.h"
#include "bench/detail/format.h"
#include "bench/time.h"

/* -------------------------------------------------------------------------- */
/* Version                                                                     */
/* -------------------------------------------------------------------------- */

#define VITTE_BENCH_API_VERSION_MAJOR 1
#define VITTE_BENCH_API_VERSION_MINOR 0
#define VITTE_BENCH_API_VERSION_PATCH 0

/* -------------------------------------------------------------------------- */
/* Types                                                                       */
/* -------------------------------------------------------------------------- */

typedef struct vitte_bench_case vitte_bench_case;

typedef struct vitte_bench_ctx {
    /* Allocations for the duration of a single run */
    vitte_arena* arena;

    /* Optional scratch buffer */
    void* scratch;
    size_t scratch_cap;

    /* Per-run user pointer (suite/case can use it). */
    void* user;

    /*
      Runtime config for the runner.
      (These values are mirrored from runner options.)
    */
    uint64_t iters;
    uint64_t warmup;
    uint64_t time_budget_ms;

    bool     pin_cpu;
    bool     quiet;
} vitte_bench_ctx;

/*
  A benchmark function.
  - Should run the measured payload exactly once.
  - Runner handles iteration/warmup loops.
*/
typedef void (*vitte_bench_fn)(vitte_bench_ctx* ctx);

/* Optional setup/teardown hooks for a case. */
typedef bool (*vitte_bench_setup_fn)(vitte_bench_ctx* ctx);

typedef void (*vitte_bench_teardown_fn)(vitte_bench_ctx* ctx);

/*
  Case definition.

  Conventions
  - `id` must be stable across time (used as baseline key).
  - `name` is human readable.
*/
struct vitte_bench_case {
    const char* suite; /* suite name (group) */
    const char* id;    /* stable id, e.g. "alloc.bump" */
    const char* name;  /* display */

    vitte_bench_fn fn;

    vitte_bench_setup_fn setup;
    vitte_bench_teardown_fn teardown;

    /* Optional parameterization label (e.g. "N=1024"). */
    const char* variant;

    /* Optional tag filtering. */
    const char* tags; /* comma-separated */
};

/* A suite is just a span of cases. */
typedef struct vitte_bench_suite {
    const char* name;
    const vitte_bench_case* cases;
    size_t case_count;
} vitte_bench_suite;

/*
  Samples captured for one case.

  - `ns` is a pointer to an array of nanosecond durations.
  - capacity is owned by the runner.
*/
typedef struct vitte_bench_samples {
    uint64_t* ns;
    size_t count;
    size_t cap;
} vitte_bench_samples;

/* Summary statistics for one case run. */
typedef struct vitte_bench_stats {
    uint64_t p50_ns;
    uint64_t p90_ns;
    uint64_t p99_ns;
    double   mean_ns;
    double   stdev_ns;

    uint64_t min_ns;
    uint64_t max_ns;

    uint64_t iters;
    uint64_t warmup;
} vitte_bench_stats;

/* Result object for one case. */
typedef struct vitte_bench_result {
    const vitte_bench_case* c;

    vitte_bench_samples samples;
    vitte_bench_stats   stats;

    /* wall-clock time spent including overhead */
    uint64_t wall_ns;

    /* errors */
    bool ok;
    char err_msg[256];
} vitte_bench_result;

/*
  Runner configuration.

  Notes
  - If `iters` is 0, runner auto-selects it based on time_budget_ms.
  - warmup defaults to small number.
*/
typedef struct vitte_bench_run_cfg {
    uint64_t iters;
    uint64_t warmup;

    uint64_t time_budget_ms;

    /* sample collection cap */
    size_t sample_cap;

    /* behavior */
    bool pin_cpu;
    bool quiet;
} vitte_bench_run_cfg;

/*
  Reporter interface.
  - The runner will call these callbacks.
  - Implementations can build text/JSON, write files, etc.
*/
typedef struct vitte_bench_reporter {
    void* user;

    /* Called once at the start. */
    bool (*begin)(void* user);

    /* Called per suite. */
    bool (*suite_begin)(void* user, const vitte_bench_suite* s);

    /* Called per case result. */
    bool (*case_result)(void* user, const vitte_bench_result* r);

    /* Called per suite. */
    bool (*suite_end)(void* user, const vitte_bench_suite* s);

    /* Called once at the end. */
    bool (*end)(void* user, bool overall_ok);
} vitte_bench_reporter;

/* -------------------------------------------------------------------------- */
/* Stats                                                                        */
/* -------------------------------------------------------------------------- */

/*
  Compute statistics from `samples`.
  - Does not allocate.
  - `samples->ns` is sorted in-place (ascending).
*/
bool vitte_bench_compute_stats(vitte_bench_samples* samples, vitte_bench_stats* out);

/* -------------------------------------------------------------------------- */
/* Running                                                                      */
/* -------------------------------------------------------------------------- */

/*
  Run a single case.

  - Uses cfg to determine iteration counts and sample capture.
  - Returns true on success.
*/
bool vitte_bench_run_case(
    const vitte_bench_case* c,
    const vitte_bench_run_cfg* cfg,
    vitte_arena* arena,
    vitte_bench_result* out
);

/*
  Run a suite. Emits results through `rep` if provided.
*/
bool vitte_bench_run_suite(
    const vitte_bench_suite* s,
    const vitte_bench_run_cfg* cfg,
    vitte_arena* arena,
    const vitte_bench_reporter* rep
);

/*
  Run multiple suites.
*/
bool vitte_bench_run_all(
    const vitte_bench_suite* suites,
    size_t suite_count,
    const vitte_bench_run_cfg* cfg,
    vitte_arena* arena,
    const vitte_bench_reporter* rep
);

/* -------------------------------------------------------------------------- */
/* Default reporters                                                            */
/* -------------------------------------------------------------------------- */

/*
  Text reporter.
  - Writes human readable results to a vitte_buf.
*/
typedef struct vitte_bench_text_reporter {
    vitte_buf* out;
    bool show_samples;
} vitte_bench_text_reporter;

bool vitte_bench_text_reporter_init(vitte_bench_reporter* rep, vitte_bench_text_reporter* state, vitte_buf* out);

/*
  JSON reporter.
  - Emits JSON array of results.
*/
typedef struct vitte_bench_json_reporter {
    vitte_buf* out;
    bool pretty;
} vitte_bench_json_reporter;

bool vitte_bench_json_reporter_init(vitte_bench_reporter* rep, vitte_bench_json_reporter* state, vitte_buf* out);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_BENCH_H */