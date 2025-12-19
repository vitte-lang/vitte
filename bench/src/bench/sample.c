

// sample.c - minimal benchmark registration + runner invocation (C17)
//
// This file demonstrates how to:
//   - implement a few benchmark functions
//   - register them into the registry
//   - execute them via bench_runner_run(argc, argv)
//
// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bench/registry.h"
#include "bench/runner.h"

// Optional JSON benchmark.
// Define implementation here if no other TU does.
#ifndef JSON_PARSER_IMPLEMENTATION
#define JSON_PARSER_IMPLEMENTATION
#endif
#include "bench/json_parser.h"

// Prevent the compiler from optimizing away work.
static volatile uint64_t g_sink_u64 = 0;

// -----------------------------------------------------------------------------
// Benchmarks (default calling convention expected by runner.c)
//   int (*)(void* ctx, int64_t iters) -> 0 on success
// -----------------------------------------------------------------------------

typedef struct bm_add_ctx
{
    uint64_t seed;
} bm_add_ctx;

static int bm_add(void* vctx, int64_t iters)
{
    bm_add_ctx* ctx = (bm_add_ctx*)vctx;
    uint64_t x = ctx ? ctx->seed : 0x9e3779b97f4a7c15ull;

    // simple arithmetic loop
    for (int64_t i = 0; i < iters; ++i)
    {
        x = x * 6364136223846793005ull + 1442695040888963407ull;
        x ^= (x >> 33);
    }

    g_sink_u64 ^= x;
    return 0;
}

typedef struct bm_memcpy_ctx
{
    uint8_t* src;
    uint8_t* dst;
    size_t size;
} bm_memcpy_ctx;

static int bm_memcpy(void* vctx, int64_t iters)
{
    bm_memcpy_ctx* ctx = (bm_memcpy_ctx*)vctx;
    if (!ctx || !ctx->src || !ctx->dst || ctx->size == 0) return -1;

    for (int64_t i = 0; i < iters; ++i)
    {
        memcpy(ctx->dst, ctx->src, ctx->size);
        // touch one byte to keep the copy observable
        g_sink_u64 ^= (uint64_t)ctx->dst[(size_t)(i % (int64_t)ctx->size)];
    }

    return 0;
}

typedef struct bm_json_ctx
{
    const char* json;
    size_t json_len;
    jp_tok* tokens;
    int32_t tok_cap;
} bm_json_ctx;

static int bm_json_parse(void* vctx, int64_t iters)
{
    bm_json_ctx* ctx = (bm_json_ctx*)vctx;
    if (!ctx || !ctx->json || !ctx->tokens || ctx->tok_cap <= 0) return -1;

    jp_cfg cfg = jp_cfg_default();
    cfg.strict = true;

    for (int64_t i = 0; i < iters; ++i)
    {
        jp_parser p;
        jp_init(&p, cfg, ctx->tokens, ctx->tok_cap);
        int32_t root = -1;
        int32_t ntok = jp_parse(&p, ctx->json, ctx->json_len, &root);
        if (ntok < 0) return -2;

        // touch a few token fields to prevent dead-code elimination
        const jp_tok* t0 = jp_token(&p, root);
        if (t0) g_sink_u64 ^= (uint64_t)(uint32_t)t0->size;
    }

    return 0;
}

// -----------------------------------------------------------------------------
// Registration
// -----------------------------------------------------------------------------

static int register_benchmarks(void)
{
    // 1) add
    static bm_add_ctx add_ctx = { 123456789ull };
    if (bench_registry_add("micro/add", 0, (bench_fn_t)(void*)bm_add, &add_ctx) != 0)
        return -1;

    // 2) memcpy (64 KiB)
    enum { BUF_SIZE = 64 * 1024 };
    static uint8_t* src = NULL;
    static uint8_t* dst = NULL;

    if (!src)
    {
        src = (uint8_t*)malloc(BUF_SIZE);
        dst = (uint8_t*)malloc(BUF_SIZE);
        if (!src || !dst) return -2;
        for (int i = 0; i < BUF_SIZE; ++i) src[i] = (uint8_t)(i * 131u);
        memset(dst, 0, BUF_SIZE);
    }

    static bm_memcpy_ctx memcpy_ctx;
    memcpy_ctx.src = src;
    memcpy_ctx.dst = dst;
    memcpy_ctx.size = BUF_SIZE;

    if (bench_registry_add("micro/memcpy_64k", 0, (bench_fn_t)(void*)bm_memcpy, &memcpy_ctx) != 0)
        return -3;

    // 3) json parse
    static const char* json =
        "{"
        "\"name\":\"vitte\","
        "\"version\":1,"
        "\"values\":[1,2,3,4,5,6,7,8,9,10],"
        "\"nested\":{\"a\":true,\"b\":false,\"c\":null}"
        "}";

    // token capacity: small JSON, keep it generous
    enum { TOK_CAP = 128 };
    static jp_tok tokens[TOK_CAP];

    static bm_json_ctx json_ctx;
    json_ctx.json = json;
    json_ctx.json_len = strlen(json);
    json_ctx.tokens = tokens;
    json_ctx.tok_cap = TOK_CAP;

    if (bench_registry_add("micro/json_parse", 0, (bench_fn_t)(void*)bm_json_parse, &json_ctx) != 0)
        return -4;

    return 0;
}

int main(int argc, char** argv)
{
    if (register_benchmarks() != 0)
        return 3;

    int rc = bench_runner_run(argc, argv);

    // Cleanup registry-owned ids. Note: sample allocations (src/dst) are process-lifetime.
    bench_registry_shutdown();

    // Make sink observable.
    if (g_sink_u64 == 0xdeadbeefULL) return 42;

    return rc;
}
