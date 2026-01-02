

// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// bench_baseline.c
// -----------------------------------------------------------------------------
// Baseline benchmarks for the compiler bench harness.
//
// Purpose:
//   - Measure harness overhead and provide reference points.
//   - Provide common blackhole functions for suites that want to call them.
//
// This file defines a suite named "baseline" with a few micro-benchmarks:
//   - noop / loop overhead
//   - arithmetic mix
//   - branchy workload
//   - FNV1a hashing (small/large)
//   - memcpy (small/large)
//   - malloc/free (small)
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Bench ABI (keep in sync with suite_common.h)
// -----------------------------------------------------------------------------

typedef void (*bench_fn)(uint64_t iters, void* user);

typedef struct bench_case
{
    const char* name;
    bench_fn fn;
    void* user;
} bench_case_t;

typedef struct bench_suite
{
    const char* name;
    const bench_case_t* cases;
    size_t case_count;
} bench_suite_t;

// Implemented by the bench runner.
void bench_register_suite(const bench_suite_t* suite);

#if defined(__GNUC__) || defined(__clang__)
#define VITTE_BENCH_CONSTRUCTOR __attribute__((constructor))
#else
#define VITTE_BENCH_CONSTRUCTOR
#endif

// -----------------------------------------------------------------------------
// Blackhole (strong definitions)
// -----------------------------------------------------------------------------

static volatile uint64_t g_sink_u64;
static volatile uint8_t  g_sink_u8;

void bench_blackhole_u64(uint64_t v)
{
    // Mix in v so the optimizer can't prove anything away.
    g_sink_u64 ^= (v + 0x9e3779b97f4a7c15ULL) ^ (g_sink_u64 << 7) ^ (g_sink_u64 >> 3);
}

void bench_blackhole_bytes(const void* p, size_t n)
{
    const uint8_t* b = (const uint8_t*)p;
    uint8_t acc = (uint8_t)g_sink_u8;
    if (n)
    {
        acc ^= b[0];
        acc ^= b[n >> 1];
        acc ^= b[n - 1];
    }
    g_sink_u8 = (uint8_t)(acc + 0x2B);
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static inline uint64_t fnv1a64(const void* data, size_t n)
{
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < n; ++i)
    {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static inline uint64_t xorshift64(uint64_t* s)
{
    uint64_t x = *s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *s = x;
    return x;
}

static uint8_t* g_buf_small;
static uint8_t* g_buf_large;
static bool g_bufs_inited;

enum { kSmallN = 256, kLargeN = 256 * 1024 };

static void init_bufs_once(void)
{
    if (g_bufs_inited) return;
    g_bufs_inited = true;

    g_buf_small = (uint8_t*)malloc(kSmallN);
    g_buf_large = (uint8_t*)malloc(kLargeN);
    if (!g_buf_small || !g_buf_large) abort();

    for (size_t i = 0; i < kSmallN; ++i) g_buf_small[i] = (uint8_t)(i * 131u + 7u);
    for (size_t i = 0; i < kLargeN; ++i) g_buf_large[i] = (uint8_t)(i * 17u + 3u);
}

// -----------------------------------------------------------------------------
// Bench cases
// -----------------------------------------------------------------------------

static void bm_noop(uint64_t iters, void* user)
{
    (void)user;
    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
        acc += i;
    bench_blackhole_u64(acc);
}

static void bm_blackhole_u64(uint64_t iters, void* user)
{
    (void)user;
    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
    {
        acc ^= (i + 0x9e3779b97f4a7c15ULL) ^ (acc << 5) ^ (acc >> 2);
        bench_blackhole_u64(acc);
    }
    bench_blackhole_u64(acc);
}

static void bm_arith_mix(uint64_t iters, void* user)
{
    (void)user;
    uint64_t a = 1, b = 0x9e3779b97f4a7c15ULL, c = 0xD1B54A32D192ED03ULL;
    for (uint64_t i = 0; i < iters; ++i)
    {
        a += (i | 1u);
        b ^= a + (b << 7) + (b >> 3);
        c = c * 6364136223846793005ULL + (a ^ (b >> 1));
        a ^= (c + (a << 11) + (a >> 5));
    }
    bench_blackhole_u64(a ^ b ^ c);
}

static void bm_branchy(uint64_t iters, void* user)
{
    (void)user;
    uint64_t rng = 0x12345678ABCDEF01ULL;
    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
    {
        const uint64_t x = xorshift64(&rng);
        if ((x & 3u) == 0u) acc += (x ^ (acc << 1));
        else if ((x & 3u) == 1u) acc ^= (x + 0x9e3779b97f4a7c15ULL);
        else if ((x & 3u) == 2u) acc -= (x | 1u);
        else acc = (acc << 1) | (acc >> 63);
    }
    bench_blackhole_u64(acc);
}

static void bm_hash_small(uint64_t iters, void* user)
{
    (void)user;
    init_bufs_once();
    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
    {
        acc ^= fnv1a64(g_buf_small, kSmallN) + i;
    }
    bench_blackhole_u64(acc);
}

static void bm_hash_large(uint64_t iters, void* user)
{
    (void)user;
    init_bufs_once();
    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
    {
        acc ^= fnv1a64(g_buf_large, kLargeN) + (acc << 1) + i;
    }
    bench_blackhole_u64(acc);
}

static void bm_memcpy_small(uint64_t iters, void* user)
{
    (void)user;
    init_bufs_once();

    uint8_t dst[kSmallN];
    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        memcpy(dst, g_buf_small, kSmallN);
        acc ^= (uint64_t)dst[i & (kSmallN - 1)];
    }

    bench_blackhole_bytes(dst, sizeof(dst));
    bench_blackhole_u64(acc);
}

static void bm_memcpy_large(uint64_t iters, void* user)
{
    (void)user;
    init_bufs_once();

    uint8_t* dst = (uint8_t*)malloc(kLargeN);
    if (!dst) abort();

    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
    {
        memcpy(dst, g_buf_large, kLargeN);
        acc ^= (uint64_t)dst[(i * 131u) & (kLargeN - 1)];
    }

    bench_blackhole_bytes(dst, kLargeN);
    bench_blackhole_u64(acc);

    free(dst);
}

static void bm_malloc_free_small(uint64_t iters, void* user)
{
    (void)user;
    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        const size_t n = (size_t)((i & 63u) + 1u);
        uint8_t* p = (uint8_t*)malloc(n);
        if (!p) abort();
        p[0] = (uint8_t)n;
        p[n - 1] ^= (uint8_t)(i & 0xFFu);
        acc ^= (uint64_t)p[0] + (uint64_t)p[n - 1];
        free(p);
    }

    bench_blackhole_u64(acc);
}

// -----------------------------------------------------------------------------
// Suite registration
// -----------------------------------------------------------------------------

static const bench_case_t k_cases[] = {
    { "noop",             bm_noop,             NULL },
    { "blackhole_u64",    bm_blackhole_u64,    NULL },
    { "arith_mix",        bm_arith_mix,        NULL },
    { "branchy",          bm_branchy,          NULL },
    { "hash_small",       bm_hash_small,       NULL },
    { "hash_large",       bm_hash_large,       NULL },
    { "memcpy_small",     bm_memcpy_small,     NULL },
    { "memcpy_large",     bm_memcpy_large,     NULL },
    { "malloc_free_small", bm_malloc_free_small, NULL },
};

static const bench_suite_t k_suite = {
    "baseline",
    k_cases,
    sizeof(k_cases) / sizeof(k_cases[0]),
};

VITTE_BENCH_CONSTRUCTOR static void baseline_suite_autoregister(void)
{
    bench_register_suite(&k_suite);
}

// Explicit symbol for runners that register suites manually.
void vitte_bench_register_baseline_suite(void)
{
    bench_register_suite(&k_suite);
}

// Optional teardown for baseline.
void vitte_bench_baseline_teardown(void)
{
    if (g_bufs_inited)
    {
        free(g_buf_small);
        free(g_buf_large);
        g_buf_small = NULL;
        g_buf_large = NULL;
        g_bufs_inited = false;
    }
}