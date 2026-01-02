

/*
  bench/report.h

  Report model and reporter interfaces for the Vitte benchmark harness.

  Goals
  - Provide a stable, deterministic, machine-readable and human-readable report model.
  - Support multiple output formats (text, json) without duplicating measurement logic.
  - Keep allocation explicit via vitte_arena.

  Design principles
  - Plain C structs (POD), arena-backed strings/arrays.
  - Deterministic ordering (sort by suite/case/metric name).
  - Versioned report ABI for forward/backward compatibility.

  Typical flow
    1) runner collects results into vitte_report
    2) reporter serializes (text/json)
    3) optional: persist as baseline, compare, emit diff

  Notes
  - This is NOT a tracing format. It is for benchmark aggregates.
  - Time units are always nanoseconds unless otherwise specified.
*/

#pragma once
#ifndef VITTE_BENCH_REPORT_H
#define VITTE_BENCH_REPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/arena.h"
#include "bench/diag.h"
#include "bench/hash.h"
#include "bench/json.h"
#include "bench/platform.h"
#include "bench/detail/format.h"

/* -------------------------------------------------------------------------- */
/* Versioning                                                                  */
/* -------------------------------------------------------------------------- */

#define VITTE_REPORT_VERSION 1u

/* -------------------------------------------------------------------------- */
/* Enums                                                                       */
/* -------------------------------------------------------------------------- */

typedef enum vitte_report_unit {
    VITTE_REPORT_UNIT_NONE = 0,

    VITTE_REPORT_UNIT_NS,
    VITTE_REPORT_UNIT_US,
    VITTE_REPORT_UNIT_MS,
    VITTE_REPORT_UNIT_S,

    VITTE_REPORT_UNIT_BYTES,
    VITTE_REPORT_UNIT_KB,
    VITTE_REPORT_UNIT_MB,

    VITTE_REPORT_UNIT_COUNT,
    VITTE_REPORT_UNIT_PERCENT,

    VITTE_REPORT_UNIT_HZ
} vitte_report_unit;

/* A measurement distribution type. */
typedef enum vitte_report_dist_kind {
    VITTE_REPORT_DIST_UNKNOWN = 0,
    VITTE_REPORT_DIST_SAMPLES,
    VITTE_REPORT_DIST_HISTOGRAM
} vitte_report_dist_kind;

/* Comparison outcome between current run and baseline. */
typedef enum vitte_report_cmp {
    VITTE_REPORT_CMP_UNKNOWN = 0,
    VITTE_REPORT_CMP_EQUAL,
    VITTE_REPORT_CMP_FASTER,
    VITTE_REPORT_CMP_SLOWER,
    VITTE_REPORT_CMP_REGRESSION,
    VITTE_REPORT_CMP_IMPROVEMENT
} vitte_report_cmp;

/* -------------------------------------------------------------------------- */
/* Core report model                                                            */
/* -------------------------------------------------------------------------- */

typedef struct vitte_report_kv {
    const char* key;   /* arena-backed */
    const char* value; /* arena-backed */
} vitte_report_kv;

/*
  A metric is the atomic aggregate reported for a case.

  Example metrics:
  - time.ns.mean
  - time.ns.p50
  - time.ns.p99
  - alloc.bytes
  - throughput.bytes_per_s
*/
typedef struct vitte_report_metric {
    const char* name;        /* stable id */
    const char* display;     /* optional pretty name */

    vitte_report_unit unit;

    /* Aggregates */
    double mean;
    double median;
    double stdev;

    double min;
    double max;

    /* selected percentiles (0..100), optional */
    double p50;
    double p90;
    double p95;
    double p99;

    /* sample count for this metric */
    uint64_t n;

    /* optional absolute thresholding */
    double threshold_regression; /* e.g., +5% triggers regression */
    double threshold_improvement;/* e.g., -5% triggers improvement */

    /* baseline comparison (filled by compare) */
    bool has_baseline;
    double baseline_mean;
    double delta_abs;
    double delta_pct;
    vitte_report_cmp cmp;

    /* free-form tags (arena-backed) */
    vitte_report_kv* tags;
    size_t tags_count;
} vitte_report_metric;

/* Optional distribution for deeper inspection. */
typedef struct vitte_report_dist {
    vitte_report_dist_kind kind;

    /* If SAMPLES: values[n]. */
    const double* values;
    size_t count;

    /* If HISTOGRAM: pairs (bin_edge, count). */
    const double* bin_edges;
    const uint64_t* bin_counts;
    size_t bins;
} vitte_report_dist;

/* A benchmark case result. */
typedef struct vitte_report_case {
    const char* suite;  /* stable suite id */
    const char* name;   /* stable case id */

    const char* display; /* optional */

    /* arbitrary metadata */
    vitte_report_kv* meta;
    size_t meta_count;

    /* metrics */
    vitte_report_metric* metrics;
    size_t metrics_count;

    /* optional distributions aligned with metrics (same count), may be NULL */
    vitte_report_dist* dists;

    /* stable signatures */
    uint64_t suite_hash;
    uint64_t case_hash;
} vitte_report_case;

/* A suite summary (optional top-level aggregation). */
typedef struct vitte_report_suite {
    const char* name;
    const char* display;

    vitte_report_kv* meta;
    size_t meta_count;

    uint64_t suite_hash;

    /* pointers into report->cases range (not owning) */
    const vitte_report_case* const* cases;
    size_t cases_count;
} vitte_report_suite;

/* Top-level report. */
typedef struct vitte_report {
    uint32_t version;   /* VITTE_REPORT_VERSION */

    /* platform */
    vitte_platform_desc platform;

    /* run identity */
    const char* run_name;        /* e.g. "local" */
    const char* run_id;          /* e.g. uuid */
    const char* timestamp_utc;   /* ISO-8601 */

    /* build identity */
    const char* project;         /* e.g. "vitte" */
    const char* component;       /* e.g. "compiler/bench" */

    /* environment */
    vitte_report_kv* env;
    size_t env_count;

    /* raw results */
    vitte_report_case* cases;
    size_t cases_count;

    /* optional suite index (derived) */
    vitte_report_suite* suites;
    size_t suites_count;

    /* stable signature */
    uint64_t report_hash;

    /* internal arena used to allocate this report */
    vitte_arena* arena;
} vitte_report;

/* -------------------------------------------------------------------------- */
/* Construction / utilities                                                     */
/* -------------------------------------------------------------------------- */

/* Initialize an empty report (does not allocate cases/metrics). */
void vitte_report_init(vitte_report* r, vitte_arena* arena);

/* Finalize (compute hashes, build suite index if requested, sort deterministically). */
bool vitte_report_finalize(vitte_report* r, bool build_suite_index, vitte_diag* d);

/* Add a case; returns pointer to newly created case in arena-backed array. */
vitte_report_case* vitte_report_add_case(
    vitte_report* r,
    const char* suite,
    const char* name,
    const char* display,
    vitte_diag* d
);

/* Add a metric to a case. */
vitte_report_metric* vitte_report_case_add_metric(
    vitte_report* r,
    vitte_report_case* c,
    const char* name,
    const char* display,
    vitte_report_unit unit,
    vitte_diag* d
);

/* Add metadata key/value to case. */
bool vitte_report_case_add_meta(vitte_report* r, vitte_report_case* c, const char* key, const char* value, vitte_diag* d);

/* Add tag to metric. */
bool vitte_report_metric_add_tag(vitte_report* r, vitte_report_metric* m, const char* key, const char* value, vitte_diag* d);

/* Convenience: compute stable hash for suite+case+metric names. */
uint64_t vitte_report_hash_name(const char* s);

/* Compare report `cur` vs baseline `base` and fill metric delta fields. */
bool vitte_report_compare(vitte_report* cur, const vitte_report* base, vitte_diag* d);

/* -------------------------------------------------------------------------- */
/* Formatting / serialization                                                   */
/* -------------------------------------------------------------------------- */

typedef enum vitte_report_format {
    VITTE_REPORT_FORMAT_TEXT = 1,
    VITTE_REPORT_FORMAT_JSON = 2
} vitte_report_format;

/* Reporter interface (vtable). */
typedef struct vitte_reporter {
    const char* name;

    bool (*write)(
        void* user,
        vitte_buf* out,
        const vitte_report* r,
        vitte_report_format fmt,
        uint32_t flags,
        vitte_diag* d
    );

    void* user;
} vitte_reporter;

/* Flags for formatting. */
#define VITTE_REPORT_F_NONE            0u
#define VITTE_REPORT_F_PRETTY_JSON     (1u << 0)
#define VITTE_REPORT_F_INCLUDE_ENV     (1u << 1)
#define VITTE_REPORT_F_INCLUDE_META    (1u << 2)
#define VITTE_REPORT_F_INCLUDE_TAGS    (1u << 3)
#define VITTE_REPORT_F_INCLUDE_BASELINE (1u << 4)
#define VITTE_REPORT_F_INCLUDE_DISTS   (1u << 5)

/* Default reporter (text/json). */
const vitte_reporter* vitte_reporter_default(void);

/* Serialize report into vitte_buf (appends). */
bool vitte_report_write(
    vitte_buf* out,
    const vitte_report* r,
    vitte_report_format fmt,
    uint32_t flags,
    vitte_diag* d
);

/* Convenience helpers */
bool vitte_report_write_text(vitte_buf* out, const vitte_report* r, uint32_t flags, vitte_diag* d);
bool vitte_report_write_json(vitte_buf* out, const vitte_report* r, uint32_t flags, vitte_diag* d);

/* -------------------------------------------------------------------------- */
/* Inline helpers                                                               */
/* -------------------------------------------------------------------------- */

static inline const char* vitte_report_unit_name(vitte_report_unit u)
{
    switch (u) {
        case VITTE_REPORT_UNIT_NS: return "ns";
        case VITTE_REPORT_UNIT_US: return "us";
        case VITTE_REPORT_UNIT_MS: return "ms";
        case VITTE_REPORT_UNIT_S: return "s";
        case VITTE_REPORT_UNIT_BYTES: return "bytes";
        case VITTE_REPORT_UNIT_KB: return "kb";
        case VITTE_REPORT_UNIT_MB: return "mb";
        case VITTE_REPORT_UNIT_COUNT: return "count";
        case VITTE_REPORT_UNIT_PERCENT: return "percent";
        case VITTE_REPORT_UNIT_HZ: return "hz";
        default: return "none";
    }
}

static inline const char* vitte_report_cmp_name(vitte_report_cmp c)
{
    switch (c) {
        case VITTE_REPORT_CMP_EQUAL: return "equal";
        case VITTE_REPORT_CMP_FASTER: return "faster";
        case VITTE_REPORT_CMP_SLOWER: return "slower";
        case VITTE_REPORT_CMP_REGRESSION: return "regression";
        case VITTE_REPORT_CMP_IMPROVEMENT: return "improvement";
        default: return "unknown";
    }
}

/* -------------------------------------------------------------------------- */
/* Header-only implementation: report hash name                                */
/* -------------------------------------------------------------------------- */

static inline uint64_t vitte_report_hash_name(const char* s)
{
    return vitte_bench_hash64_str(s);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_REPORT_H */