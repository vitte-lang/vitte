// SPDX-License-Identifier: MIT
// bench_report.c - benchmark reporting (C17)
//
– Provides a small reporting layer for the bench runner.
//
– Features:
//  - Stores per-benchmark summary stats (min/max/mean/stdev + p50/p95) from
//    an array of sample durations (nanoseconds).
//  - Human-readable table output.
//  - JSON output (no external JSON dependency).
//
// Integration model:
//  - The runner measures samples (each sample is a full benchmark run or
//    iteration batch) and calls bench_report_add_case().
//  - Then the runner prints or writes the report.
//
// This file intentionally avoids depending on other project headers; wire the
// declarations into your public bench header and include it instead if needed.

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef enum bench_report_format {
    BENCH_REPORT_HUMAN = 0,
    BENCH_REPORT_JSON  = 1,
} bench_report_format;

typedef struct bench_report_config {
    bench_report_format format;     // default: HUMAN
    bool                show_header; // default: true
    bool                sort_by_mean; // default: false (sort by name)
} bench_report_config;

typedef struct bench_case_stats {
    uint64_t n;        // samples count
    uint64_t min_ns;
    uint64_t max_ns;
    double   mean_ns;
    double   stdev_ns;
    uint64_t p50_ns;
    uint64_t p95_ns;
} bench_case_stats;

typedef struct bench_case_result {
    char* name;   // owned
    char* group;  // owned (optional)

    uint64_t iterations; // logical iterations executed per sample batch (runner-defined)
    uint64_t bytes;      // processed bytes (optional, 0 if n/a)

    bench_case_stats stats;
} bench_case_result;

typedef struct bench_report {
    bench_report_config cfg;
    bench_case_result*  cases;
    size_t              len;
    size_t              cap;
} bench_report;

//------------------------------------------------------------------------------
// Local helpers
//------------------------------------------------------------------------------

static void* bench_xrealloc(void* p, size_t n) {
    void* q = realloc(p, n);
    if (!q && n != 0) {
        fprintf(stderr, "bench_report: OOM realloc(%zu)\n", n);
        abort();
    }
    return q;
}

static char* bench_xstrdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char*  p = (char*)malloc(n);
    if (!p) {
        fprintf(stderr, "bench_report: OOM strdup(%zu)\n", n);
        abort();
    }
    memcpy(p, s, n);
    return p;
}

static int cmp_u64(const void* a, const void* b) {
    const uint64_t ua = *(const uint64_t*)a;
    const uint64_t ub = *(const uint64_t*)b;
    return (ua < ub) ? -1 : (ua > ub) ? 1 : 0;
}

static uint64_t percentile_from_sorted(const uint64_t* sorted, size_t n, double pct01) {
    if (!sorted || n == 0) return 0;
    if (pct01 <= 0.0) return sorted[0];
    if (pct01 >= 1.0) return sorted[n - 1];

    // Nearest-rank with 0-based index.
    // idx = ceil(pct*n) - 1; but clamp.
    double rank = pct01 * (double)n;
    size_t idx = (size_t)ceil(rank);
    if (idx == 0) idx = 1;
    idx -= 1;
    if (idx >= n) idx = n - 1;
    return sorted[idx];
}

static bench_case_stats compute_stats_from_samples_ns(const uint64_t* samples_ns, size_t n) {
    bench_case_stats s;
    memset(&s, 0, sizeof(s));
    s.n = (uint64_t)n;
    if (!samples_ns || n == 0) return s;

    uint64_t minv = samples_ns[0];
    uint64_t maxv = samples_ns[0];

    // Welford
    double mean = 0.0;
    double m2   = 0.0;

    for (size_t i = 0; i < n; ++i) {
        uint64_t x = samples_ns[i];
        if (x < minv) minv = x;
        if (x > maxv) maxv = x;

        double dx = (double)x - mean;
        mean += dx / (double)(i + 1);
        double dx2 = (double)x - mean;
        m2 += dx * dx2;
    }

    double var = 0.0;
    if (n >= 2) var = m2 / (double)(n - 1);

    // Percentiles need a sorted copy
    uint64_t* tmp = (uint64_t*)malloc(n * sizeof(uint64_t));
    if (!tmp) {
        fprintf(stderr, "bench_report: OOM percentiles(%zu)\n", n);
        abort();
    }
    memcpy(tmp, samples_ns, n * sizeof(uint64_t));
    qsort(tmp, n, sizeof(uint64_t), cmp_u64);

    s.min_ns   = minv;
    s.max_ns   = maxv;
    s.mean_ns  = mean;
    s.stdev_ns = (var > 0.0) ? sqrt(var) : 0.0;
    s.p50_ns   = percentile_from_sorted(tmp, n, 0.50);
    s.p95_ns   = percentile_from_sorted(tmp, n, 0.95);

    free(tmp);
    return s;
}

static double ns_to_ms(double ns) { return ns / 1000000.0; }
static double ns_to_us(double ns) { return ns / 1000.0; }

static void print_duration_auto(FILE* out, double ns) {
    // Auto scale: ns / us / ms / s
    if (ns < 1000.0) {
        fprintf(out, "%.0f ns", ns);
    } else if (ns < 1000000.0) {
        fprintf(out, "%.3f us", ns_to_us(ns));
    } else if (ns < 1000000000.0) {
        fprintf(out, "%.3f ms", ns_to_ms(ns));
    } else {
        fprintf(out, "%.3f s", ns / 1000000000.0);
    }
}

static void json_escape(FILE* out, const char* s) {
    fputc('\"', out);
    if (s) {
        for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
            unsigned char c = *p;
            switch (c) {
                case '\\': fputs("\\\\", out); break;
                case '\"':  fputs("\\\"", out); break;
                case '\b': fputs("\\b", out); break;
                case '\f': fputs("\\f", out); break;
                case '\n': fputs("\\n", out); break;
                case '\r': fputs("\\r", out); break;
                case '\t': fputs("\\t", out); break;
                default:
                    if (c < 0x20) {
                        fprintf(out, "\\u%04x", (unsigned)c);
                    } else {
                        fputc((int)c, out);
                    }
                    break;
            }
        }
    }
    fputc('\"', out);
}

static int bench_case_cmp_name(const void* a, const void* b) {
    const bench_case_result* ca = (const bench_case_result*)a;
    const bench_case_result* cb = (const bench_case_result*)b;
    const char* na = ca->name ? ca->name : "";
    const char* nb = cb->name ? cb->name : "";
    return strcmp(na, nb);
}

static int bench_case_cmp_mean(const void* a, const void* b) {
    const bench_case_result* ca = (const bench_case_result*)a;
    const bench_case_result* cb = (const bench_case_result*)b;
    double ma = ca->stats.mean_ns;
    double mb = cb->stats.mean_ns;
    if (ma < mb) return -1;
    if (ma > mb) return 1;
    return bench_case_cmp_name(a, b);
}

static void bench_report_ensure_cap(bench_report* r, size_t need) {
    if (r->cap >= need) return;
    size_t new_cap = (r->cap == 0) ? 16 : r->cap;
    while (new_cap < need) new_cap *= 2;
    r->cases = (bench_case_result*)bench_xrealloc(r->cases, new_cap * sizeof(r->cases[0]));
    for (size_t i = r->cap; i < new_cap; ++i) {
        r->cases[i].name = NULL;
        r->cases[i].group = NULL;
        r->cases[i].iterations = 0;
        r->cases[i].bytes = 0;
        memset(&r->cases[i].stats, 0, sizeof(r->cases[i].stats));
    }
    r->cap = new_cap;
}

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

void bench_report_init(bench_report* r, const bench_report_config* cfg) {
    assert(r);
    memset(r, 0, sizeof(*r));
    r->cfg.format = BENCH_REPORT_HUMAN;
    r->cfg.show_header = true;
    r->cfg.sort_by_mean = false;
    if (cfg) r->cfg = *cfg;
}

void bench_report_destroy(bench_report* r) {
    if (!r) return;
    for (size_t i = 0; i < r->len; ++i) {
        free(r->cases[i].name);
        free(r->cases[i].group);
        r->cases[i].name = NULL;
        r->cases[i].group = NULL;
    }
    free(r->cases);
    r->cases = NULL;
    r->len = 0;
    r->cap = 0;
}

// Adds a benchmark case result from a sample array (nanoseconds).
//
// iterations: logical amount of work per sample (runner-defined). If you pass the
//             number of iterations executed in each sample, throughput can be
//             computed as iter/sec.
// bytes     : processed bytes per sample (optional). If 0, throughput bytes/sec
//             is not printed.
//
// Returns 0 on success, negative errno on error.
int bench_report_add_case(bench_report* r,
                          const char* name,
                          const char* group,
                          uint64_t iterations,
                          uint64_t bytes,
                          const uint64_t* samples_ns,
                          size_t samples_count) {
    if (!r || !name || !*name || !samples_ns || samples_count == 0) {
        return -EINVAL;
    }

    // Prevent duplicates by name (first wins).
    for (size_t i = 0; i < r->len; ++i) {
        if (r->cases[i].name && strcmp(r->cases[i].name, name) == 0) {
            return -EEXIST;
        }
    }

    bench_report_ensure_cap(r, r->len + 1);
    bench_case_result* c = &r->cases[r->len++];
    c->name = bench_xstrdup(name);
    c->group = bench_xstrdup(group);
    c->iterations = iterations;
    c->bytes = bytes;
    c->stats = compute_stats_from_samples_ns(samples_ns, samples_count);
    return 0;
}

void bench_report_sort(bench_report* r) {
    if (!r || r->len <= 1) return;
    if (r->cfg.sort_by_mean) {
        qsort(r->cases, r->len, sizeof(r->cases[0]), bench_case_cmp_mean);
    } else {
        qsort(r->cases, r->len, sizeof(r->cases[0]), bench_case_cmp_name);
    }
}

const bench_case_result* bench_report_cases(const bench_report* r, size_t* out_count) {
    if (out_count) *out_count = r ? r->len : 0;
    return r ? r->cases : NULL;
}

//------------------------------------------------------------------------------
// Human output
//------------------------------------------------------------------------------

static void bench_report_print_human(FILE* out, const bench_report* r) {
    if (!out || !r) return;

    if (r->cfg.show_header) {
        fprintf(out,
                "%-28s  %8s  %14s  %14s  %14s  %14s  %8s\n",
                "benchmark",
                "samples",
                "mean",
                "p50",
                "p95",
                "min..max",
                "iter/s");
        fprintf(out,
                "%-28s  %8s  %14s  %14s  %14s  %14s  %8s\n",
                "----------------------------",
                "--------",
                "--------------",
                "--------------",
                "--------------",
                "--------------",
                "------");
    }

    for (size_t i = 0; i < r->len; ++i) {
        const bench_case_result* c = &r->cases[i];
        const bench_case_stats*  s = &c->stats;

        // Throughput: iter/s based on mean.
        double iter_s = 0.0;
        if (c->iterations > 0 && s->mean_ns > 0.0) {
            double seconds = s->mean_ns / 1000000000.0;
            iter_s = (double)c->iterations / seconds;
        }

        // Print row
        fprintf(out, "%-28s  %8" PRIu64 "  ", c->name ? c->name : "");
        print_duration_auto(out, s->mean_ns);
        fprintf(out, "  ");
        print_duration_auto(out, (double)s->p50_ns);
        fprintf(out, "  ");
        print_duration_auto(out, (double)s->p95_ns);
        fprintf(out, "  ");
        // min..max compact
        {
            // Ensure fixed-ish width by printing the two durations separately.
            // (Not strictly aligned, but readable.)
            print_duration_auto(out, (double)s->min_ns);
            fprintf(out, "..");
            print_duration_auto(out, (double)s->max_ns);
        }
        fprintf(out, "  ");

        if (iter_s > 0.0) {
            if (iter_s < 1000.0) fprintf(out, "%7.1f", iter_s);
            else if (iter_s < 1000000.0) fprintf(out, "%7.0f", iter_s);
            else fprintf(out, "%7.0f", iter_s);
        } else {
            fprintf(out, "%7s", "-");
        }

        // Optional bytes throughput
        if (c->bytes > 0 && s->mean_ns > 0.0) {
            double seconds = s->mean_ns / 1000000000.0;
            double bps = (double)c->bytes / seconds;
            fprintf(out, "  ");
            if (bps < 1024.0) {
                fprintf(out, "%.0f B/s", bps);
            } else if (bps < 1024.0 * 1024.0) {
                fprintf(out, "%.2f KiB/s", bps / 1024.0);
            } else if (bps < 1024.0 * 1024.0 * 1024.0) {
                fprintf(out, "%.2f MiB/s", bps / (1024.0 * 1024.0));
            } else {
                fprintf(out, "%.2f GiB/s", bps / (1024.0 * 1024.0 * 1024.0));
            }
        }

        // Optional group
        if (c->group && *c->group) {
            fprintf(out, "  [%s]", c->group);
        }

        fputc('\n', out);
    }
}

//------------------------------------------------------------------------------
// JSON output
//------------------------------------------------------------------------------

static void bench_report_print_json(FILE* out, const bench_report* r) {
    if (!out || !r) return;

    fputs("{\n  \"benchmarks\": [\n", out);

    for (size_t i = 0; i < r->len; ++i) {
        const bench_case_result* c = &r->cases[i];
        const bench_case_stats*  s = &c->stats;

        // Derived throughputs
        double iter_s = 0.0;
        double bytes_s = 0.0;
        if (s->mean_ns > 0.0) {
            double seconds = s->mean_ns / 1000000000.0;
            if (c->iterations > 0) iter_s = (double)c->iterations / seconds;
            if (c->bytes > 0)      bytes_s = (double)c->bytes / seconds;
        }

        fputs("    {\n", out);

        fputs("      \"name\": ", out);
        json_escape(out, c->name);
        fputs(",\n", out);

        fputs("      \"group\": ", out);
        json_escape(out, c->group ? c->group : "");
        fputs(",\n", out);

        fprintf(out, "      \"samples\": %" PRIu64 ",\n", s->n);
        fprintf(out, "      \"iterations\": %" PRIu64 ",\n", c->iterations);
        fprintf(out, "      \"bytes\": %" PRIu64 ",\n", c->bytes);

        fputs("      \"ns\": {\n", out);
        fprintf(out, "        \"min\": %" PRIu64 ",\n", s->min_ns);
        fprintf(out, "        \"max\": %" PRIu64 ",\n", s->max_ns);
        fprintf(out, "        \"mean\": %.6f,\n", s->mean_ns);
        fprintf(out, "        \"stdev\": %.6f,\n", s->stdev_ns);
        fprintf(out, "        \"p50\": %" PRIu64 ",\n", s->p50_ns);
        fprintf(out, "        \"p95\": %" PRIu64 "\n", s->p95_ns);
        fputs("      },\n", out);

        fputs("      \"throughput\": {\n", out);
        fprintf(out, "        \"iter_per_sec\": %.6f,\n", iter_s);
        fprintf(out, "        \"bytes_per_sec\": %.6f\n", bytes_s);
        fputs("      }\n", out);

        fputs("    }", out);
        if (i + 1 < r->len) fputs(",", out);
        fputs("\n", out);
    }

    fputs("  ]\n}\n", out);
}

//------------------------------------------------------------------------------
// Unified print entrypoint
//------------------------------------------------------------------------------

void bench_report_print(FILE* out, bench_report* r) {
    if (!out || !r) return;
    bench_report_sort(r);
    if (r->cfg.format == BENCH_REPORT_JSON) {
        bench_report_print_json(out, r);
    } else {
        bench_report_print_human(out, r);
    }
}

//------------------------------------------------------------------------------
// Optional singleton (handy for small bench executables)
//------------------------------------------------------------------------------

static bench_report g_bench_report;
static bool         g_bench_report_inited = false;

bench_report* bench_report_global(void) {
    if (!g_bench_report_inited) {
        bench_report_config cfg;
        cfg.format = BENCH_REPORT_HUMAN;
        cfg.show_header = true;
        cfg.sort_by_mean = false;
        bench_report_init(&g_bench_report, &cfg);
        g_bench_report_inited = true;
    }
    return &g_bench_report;
}

void bench_report_global_shutdown(void) {
    if (g_bench_report_inited) {
        bench_report_destroy(&g_bench_report);
        g_bench_report_inited = false;
    }
}
