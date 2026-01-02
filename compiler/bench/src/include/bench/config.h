/*
  bench/config.h

  Central configuration knobs for the Vitte benchmark harness.

  This header defines build-time defaults that can be overridden by:
    - compiler defines (-D...)
    - a higher-level project config header included before this one

  Design goals
  - Stable macro surface: bench modules rely only on these macros.
  - Conservative defaults: safe, deterministic, portable.
  - Allow CI to tune baseline thresholds and runner behavior.

  Naming
  - All macros are prefixed with VITTE_BENCH_.

  Typical overrides
    -DVITTE_BENCH_DEFAULT_ITERS=2000
    -DVITTE_BENCH_DEFAULT_WARMUP=50
    -DVITTE_BENCH_DEFAULT_TIME_BUDGET_MS=1500
    -DVITTE_BENCH_DEFAULT_SAMPLE_CAP=4096
    -DVITTE_BENCH_BASELINE_REL=0.05
    -DVITTE_BENCH_BASELINE_ABS_NS=50
    -DVITTE_BENCH_CLOCK_SOURCE=2
    -DVITTE_BENCH_TRIM_OUTLIERS=1
    -DVITTE_BENCH_TRIM_FRACTION=0.01
*/

#pragma once
#ifndef VITTE_BENCH_CONFIG_H
#define VITTE_BENCH_CONFIG_H

#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* API / schema                                                                */
/* -------------------------------------------------------------------------- */

/* Version the bench report schema independently of the API. */
#ifndef VITTE_BENCH_REPORT_SCHEMA_MAJOR
  #define VITTE_BENCH_REPORT_SCHEMA_MAJOR 1
#endif
#ifndef VITTE_BENCH_REPORT_SCHEMA_MINOR
  #define VITTE_BENCH_REPORT_SCHEMA_MINOR 0
#endif

/* -------------------------------------------------------------------------- */
/* Runner defaults                                                             */
/* -------------------------------------------------------------------------- */

/* Default iteration count when not auto-tuning (iters==0). */
#ifndef VITTE_BENCH_DEFAULT_ITERS
  #define VITTE_BENCH_DEFAULT_ITERS 1000u
#endif

/* Default warmup iterations (not recorded). */
#ifndef VITTE_BENCH_DEFAULT_WARMUP
  #define VITTE_BENCH_DEFAULT_WARMUP 25u
#endif

/* Time budget per case in auto-iter mode (milliseconds). */
#ifndef VITTE_BENCH_DEFAULT_TIME_BUDGET_MS
  #define VITTE_BENCH_DEFAULT_TIME_BUDGET_MS 1000u
#endif

/* Sample buffer capacity per case (max samples stored). */
#ifndef VITTE_BENCH_DEFAULT_SAMPLE_CAP
  #define VITTE_BENCH_DEFAULT_SAMPLE_CAP 1024u
#endif

/* If non-zero, runner attempts to pin to a single CPU/core (best effort). */
#ifndef VITTE_BENCH_DEFAULT_PIN_CPU
  #define VITTE_BENCH_DEFAULT_PIN_CPU 0
#endif

/* If non-zero, runner reduces output noise. */
#ifndef VITTE_BENCH_DEFAULT_QUIET
  #define VITTE_BENCH_DEFAULT_QUIET 0
#endif

/* If non-zero, run cases in a stable order (suite order, then case order). */
#ifndef VITTE_BENCH_STABLE_ORDER
  #define VITTE_BENCH_STABLE_ORDER 1
#endif

/* If non-zero, runner performs an explicit overhead calibration pass. */
#ifndef VITTE_BENCH_CALIBRATE_OVERHEAD
  #define VITTE_BENCH_CALIBRATE_OVERHEAD 1
#endif

/* Minimum number of iterations in auto-iter mode regardless of time budget. */
#ifndef VITTE_BENCH_AUTO_ITERS_MIN
  #define VITTE_BENCH_AUTO_ITERS_MIN 50u
#endif

/* Maximum number of iterations in auto-iter mode to avoid runaway benches. */
#ifndef VITTE_BENCH_AUTO_ITERS_MAX
  #define VITTE_BENCH_AUTO_ITERS_MAX 100000000u
#endif

/*
  Warmup strategy
    0: fixed warmup (DEFAULT_WARMUP)
    1: warmup until stable variance (best-effort)
*/
#ifndef VITTE_BENCH_WARMUP_STRATEGY
  #define VITTE_BENCH_WARMUP_STRATEGY 0
#endif

/* -------------------------------------------------------------------------- */
/* Clock / timing                                                              */
/* -------------------------------------------------------------------------- */

/*
  Clock source selection
    0: auto (best available)
    1: monotonic (coarse)
    2: monotonic (high-res)
    3: rdtsc-like (x86) if supported (requires calibration)

  Note: actual availability is platform-dependent.
*/
#ifndef VITTE_BENCH_CLOCK_SOURCE
  #define VITTE_BENCH_CLOCK_SOURCE 0
#endif

/*
  If non-zero, runner captures wall time (including overhead) per case.
  This is already modeled in vitte_bench_result but can be disabled.
*/
#ifndef VITTE_BENCH_CAPTURE_WALL_NS
  #define VITTE_BENCH_CAPTURE_WALL_NS 1
#endif

/*
  If non-zero, yields / relaxes between iterations in auto-iter mode to reduce
  thermal throttling/scheduler artifacts (best effort).
*/
#ifndef VITTE_BENCH_RELAX_BETWEEN_ITERS
  #define VITTE_BENCH_RELAX_BETWEEN_ITERS 0
#endif

/* -------------------------------------------------------------------------- */
/* Statistics                                                                  */
/* -------------------------------------------------------------------------- */

/*
  Percentile method
    0: nearest-rank
    1: linear interpolation
*/
#ifndef VITTE_BENCH_PERCENTILE_METHOD
  #define VITTE_BENCH_PERCENTILE_METHOD 0
#endif

/*
  Outlier trimming
  - If enabled, trim fraction is removed from each tail after sorting.
*/
#ifndef VITTE_BENCH_TRIM_OUTLIERS
  #define VITTE_BENCH_TRIM_OUTLIERS 0
#endif

/* Fraction in [0, 0.49]. Example: 0.01 trims 1% low and 1% high. */
#ifndef VITTE_BENCH_TRIM_FRACTION
  #define VITTE_BENCH_TRIM_FRACTION 0.01
#endif

/* If non-zero, compute stddev. Disable to reduce overhead. */
#ifndef VITTE_BENCH_COMPUTE_STDEV
  #define VITTE_BENCH_COMPUTE_STDEV 1
#endif

/* If non-zero, compute min/max. */
#ifndef VITTE_BENCH_COMPUTE_MINMAX
  #define VITTE_BENCH_COMPUTE_MINMAX 1
#endif

/* Histogram bins for optional reporters. */
#ifndef VITTE_BENCH_HIST_BINS
  #define VITTE_BENCH_HIST_BINS 32u
#endif

/* -------------------------------------------------------------------------- */
/* Baseline comparison defaults                                                */
/* -------------------------------------------------------------------------- */

/* Relative allowed slowdown (0.05 == +5%). */
#ifndef VITTE_BENCH_BASELINE_REL
  #define VITTE_BENCH_BASELINE_REL 0.05
#endif

/* Absolute allowed slowdown in nanoseconds. */
#ifndef VITTE_BENCH_BASELINE_ABS_NS
  #define VITTE_BENCH_BASELINE_ABS_NS 50ull
#endif

/* Default baseline metric: 0=p50,1=p90,2=p99,3=mean */
#ifndef VITTE_BENCH_BASELINE_METRIC
  #define VITTE_BENCH_BASELINE_METRIC 0
#endif

/* If non-zero, missing baseline entries cause failure. */
#ifndef VITTE_BENCH_BASELINE_REQUIRE_ALL
  #define VITTE_BENCH_BASELINE_REQUIRE_ALL 0
#endif

/* If non-zero, baseline compares use the same unit normalization logic. */
#ifndef VITTE_BENCH_BASELINE_UNIT_STRICT
  #define VITTE_BENCH_BASELINE_UNIT_STRICT 0
#endif

/* -------------------------------------------------------------------------- */
/* Filtering                                                                   */
/* -------------------------------------------------------------------------- */

/* Enable per-suite filtering (tags, pattern). */
#ifndef VITTE_BENCH_ENABLE_FILTER
  #define VITTE_BENCH_ENABLE_FILTER 1
#endif

/* If non-zero, interpret filters as case-insensitive. */
#ifndef VITTE_BENCH_FILTER_CASE_INSENSITIVE
  #define VITTE_BENCH_FILTER_CASE_INSENSITIVE 1
#endif

/* If non-zero, allow simple glob pattern matching (* and ?). */
#ifndef VITTE_BENCH_FILTER_GLOB
  #define VITTE_BENCH_FILTER_GLOB 1
#endif

/* If non-zero, allow regex filters if regex module is available. */
#ifndef VITTE_BENCH_FILTER_REGEX
  #define VITTE_BENCH_FILTER_REGEX 0
#endif

/* -------------------------------------------------------------------------- */
/* Output defaults                                                             */
/* -------------------------------------------------------------------------- */

/* Default report format: 0=text, 1=json */
#ifndef VITTE_BENCH_DEFAULT_REPORT_FORMAT
  #define VITTE_BENCH_DEFAULT_REPORT_FORMAT 0
#endif

/* If non-zero, JSON output is pretty-printed. */
#ifndef VITTE_BENCH_JSON_PRETTY
  #define VITTE_BENCH_JSON_PRETTY 0
#endif

/* If non-zero, text output includes per-sample values. */
#ifndef VITTE_BENCH_TEXT_SHOW_SAMPLES
  #define VITTE_BENCH_TEXT_SHOW_SAMPLES 0
#endif

/* If non-zero, text output includes histogram. */
#ifndef VITTE_BENCH_TEXT_SHOW_HIST
  #define VITTE_BENCH_TEXT_SHOW_HIST 0
#endif

/* If non-zero, emit machine-readable summary line per case (TSV-ish). */
#ifndef VITTE_BENCH_TEXT_EMIT_SUMMARY_LINE
  #define VITTE_BENCH_TEXT_EMIT_SUMMARY_LINE 0
#endif

/* -------------------------------------------------------------------------- */
/* Paths / filenames (for CLI defaults)                                        */
/* -------------------------------------------------------------------------- */

#ifndef VITTE_BENCH_DEFAULT_BASELINE_PATH
  #define VITTE_BENCH_DEFAULT_BASELINE_PATH "bench.baseline"
#endif

#ifndef VITTE_BENCH_DEFAULT_REPORT_PATH
  #define VITTE_BENCH_DEFAULT_REPORT_PATH "bench.report"
#endif

#ifndef VITTE_BENCH_DEFAULT_JSON_PATH
  #define VITTE_BENCH_DEFAULT_JSON_PATH "bench.report.json"
#endif

#ifndef VITTE_BENCH_DEFAULT_TEXT_PATH
  #define VITTE_BENCH_DEFAULT_TEXT_PATH "bench.report.txt"
#endif

/* -------------------------------------------------------------------------- */
/* Safety / determinism                                                        */
/* -------------------------------------------------------------------------- */

/* If non-zero, panic on any allocation failure in bench subsystems. */
#ifndef VITTE_BENCH_PANIC_ON_OOM
  #define VITTE_BENCH_PANIC_ON_OOM 1
#endif

/* If non-zero, disables high-variance OS-dependent features by default. */
#ifndef VITTE_BENCH_DETERMINISTIC
  #define VITTE_BENCH_DETERMINISTIC 1
#endif

/* If non-zero, attempts to reduce jitter (pin, priority, disable turbo best-effort). */
#ifndef VITTE_BENCH_JITTER_REDUCTION
  #define VITTE_BENCH_JITTER_REDUCTION 0
#endif

/* If non-zero, requests elevated scheduling priority (best-effort). */
#ifndef VITTE_BENCH_ELEVATE_PRIORITY
  #define VITTE_BENCH_ELEVATE_PRIORITY 0
#endif

/* If non-zero, request realtime-ish priority (dangerous; best-effort). */
#ifndef VITTE_BENCH_REALTIME_PRIORITY
  #define VITTE_BENCH_REALTIME_PRIORITY 0
#endif

/* -------------------------------------------------------------------------- */
/* Feature toggles                                                             */
/* -------------------------------------------------------------------------- */

/* Enable baseline comparisons. */
#ifndef VITTE_BENCH_ENABLE_BASELINE
  #define VITTE_BENCH_ENABLE_BASELINE 1
#endif

/* Enable JSON reporter. */
#ifndef VITTE_BENCH_ENABLE_JSON
  #define VITTE_BENCH_ENABLE_JSON 1
#endif

/* Enable text reporter. */
#ifndef VITTE_BENCH_ENABLE_TEXT
  #define VITTE_BENCH_ENABLE_TEXT 1
#endif

/* Enable suite discovery/registration system. */
#ifndef VITTE_BENCH_ENABLE_REGISTRY
  #define VITTE_BENCH_ENABLE_REGISTRY 1
#endif

/* Enable multi-process mode (fork/exec) if supported. */
#ifndef VITTE_BENCH_ENABLE_SUBPROCESS
  #define VITTE_BENCH_ENABLE_SUBPROCESS 0
#endif

/* Enable CPU feature probes (cpuid, sysctl, getauxval). */
#ifndef VITTE_BENCH_ENABLE_CPU_PROBE
  #define VITTE_BENCH_ENABLE_CPU_PROBE 1
#endif

/* -------------------------------------------------------------------------- */
/* Limits                                                                      */
/* -------------------------------------------------------------------------- */

/* Maximum length of case id and suite name used in formatting. */
#ifndef VITTE_BENCH_MAX_ID_LEN
  #define VITTE_BENCH_MAX_ID_LEN 128u
#endif

/* Maximum length of diagnostic messages stored in result objects. */
#ifndef VITTE_BENCH_MAX_ERR_LEN
  #define VITTE_BENCH_MAX_ERR_LEN 256u
#endif

/* Maximum number of suites/cases in registry mode (hard caps). */
#ifndef VITTE_BENCH_MAX_SUITES
  #define VITTE_BENCH_MAX_SUITES 256u
#endif

#ifndef VITTE_BENCH_MAX_CASES
  #define VITTE_BENCH_MAX_CASES 4096u
#endif

/* Maximum number of tags characters per case (comma-separated). */
#ifndef VITTE_BENCH_MAX_TAGS_LEN
  #define VITTE_BENCH_MAX_TAGS_LEN 128u
#endif

/* Maximum length of a single filter string in CLI. */
#ifndef VITTE_BENCH_MAX_FILTER_LEN
  #define VITTE_BENCH_MAX_FILTER_LEN 256u
#endif

/* -------------------------------------------------------------------------- */
/* Environment variable names (CLI integration)                                */
/* -------------------------------------------------------------------------- */

#ifndef VITTE_BENCH_ENV_ITERS
  #define VITTE_BENCH_ENV_ITERS "VITTE_BENCH_ITERS"
#endif
#ifndef VITTE_BENCH_ENV_WARMUP
  #define VITTE_BENCH_ENV_WARMUP "VITTE_BENCH_WARMUP"
#endif
#ifndef VITTE_BENCH_ENV_TIME_BUDGET_MS
  #define VITTE_BENCH_ENV_TIME_BUDGET_MS "VITTE_BENCH_TIME_BUDGET_MS"
#endif
#ifndef VITTE_BENCH_ENV_SAMPLE_CAP
  #define VITTE_BENCH_ENV_SAMPLE_CAP "VITTE_BENCH_SAMPLE_CAP"
#endif
#ifndef VITTE_BENCH_ENV_FILTER
  #define VITTE_BENCH_ENV_FILTER "VITTE_BENCH_FILTER"
#endif
#ifndef VITTE_BENCH_ENV_TAGS
  #define VITTE_BENCH_ENV_TAGS "VITTE_BENCH_TAGS"
#endif
#ifndef VITTE_BENCH_ENV_BASELINE
  #define VITTE_BENCH_ENV_BASELINE "VITTE_BENCH_BASELINE"
#endif
#ifndef VITTE_BENCH_ENV_REPORT_FORMAT
  #define VITTE_BENCH_ENV_REPORT_FORMAT "VITTE_BENCH_FORMAT"
#endif

#endif /* VITTE_BENCH_CONFIG_H */
