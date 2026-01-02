\
/*
  bench_runner.c - runs registered cases and collects samples
*/
#include "bench/bench.h"
#include "suites/suites_init.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* simple reusable arena */
typedef struct arena_blk {
  struct arena_blk *next;
  size_t used;
  size_t cap;
  unsigned char data[1];
} arena_blk;

struct bench_ctx {
  arena_blk *head;
  uint32_t rng;
  unsigned char *cache_buf;
  size_t cache_buf_sz;
};

static arena_blk *arena_new_blk(size_t cap) {
  arena_blk *b = (arena_blk *)malloc(sizeof(arena_blk) + cap);
  if (!b) abort();
  b->next = NULL;
  b->used = 0;
  b->cap  = cap;
  return b;
}

static void arena_reset(bench_ctx *ctx) {
  for (arena_blk *b = ctx->head; b; b = b->next) b->used = 0;
}

static void arena_free_all(bench_ctx *ctx) {
  arena_blk *b = ctx->head;
  while (b) {
    arena_blk *n = b->next;
    free(b);
    b = n;
  }
  ctx->head = NULL;
}

void *bench_alloc(bench_ctx *ctx, size_t n) {
  if (n == 0) n = 1;

  arena_blk *b = ctx->head;
  if (!b) {
    size_t cap = (n < (1u<<16)) ? (1u<<16) : n;
    ctx->head = arena_new_blk(cap);
    b = ctx->head;
  }

  /* find a block with room */
  while (b && (b->used + n > b->cap)) {
    if (!b->next) {
      size_t cap = (n < (1u<<16)) ? (1u<<16) : n;
      b->next = arena_new_blk(cap);
    }
    b = b->next;
  }

  void *p = b->data + b->used;
  b->used += n;
  return p;
}

void bench_free_all(bench_ctx *ctx) {
  arena_reset(ctx);
}

static uint32_t xorshift32(uint32_t *s) {
  uint32_t x = *s;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *s = x;
  return x;
}

uint32_t bench_rand_u32(bench_ctx *ctx) {
  return xorshift32(&ctx->rng);
}

static bool match_filter(const bench_opts *opts, const bench_case *c) {
  if (!opts || !opts->filter || !*opts->filter) return true;
  if (strstr(c->name, opts->filter)) return true;
  if (strstr(c->suite, opts->filter)) return true;
  return false;
}

static FILE *open_out(const bench_opts *opts) {
  if (!opts || !opts->out_path || !*opts->out_path) return stdout;
  return fopen(opts->out_path, "wb");
}

static void ensure_cache_buf(bench_ctx *ctx) {
  if (ctx->cache_buf) return;
  /* touch ~8 MiB (best-effort) */
  size_t sz = 8u * 1024u * 1024u;
  ctx->cache_buf = (unsigned char *)malloc(sz);
  if (!ctx->cache_buf) return;
  ctx->cache_buf_sz = sz;
  memset(ctx->cache_buf, 0, sz);
}

static void flush_cache_best_effort(bench_ctx *ctx) {
  if (!ctx->cache_buf) return;
  volatile unsigned acc = 0;
  for (size_t i = 0; i < ctx->cache_buf_sz; i += 64) acc += ctx->cache_buf[i];
  (void)acc;
}

int bench_run_all(const bench_opts *opts) {
  bench_register_all_suites();

  if (opts && opts->pin_cpu >= 0) {
    (void)bench_pin_to_cpu(opts->pin_cpu);
  }

  size_t n = 0;
  const bench_case *cases = bench_cases(&n);
  if (!cases || n == 0) return 0;

  bench_ctx ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.rng = opts ? opts->seed : 1;

  if (opts && opts->flush_cache) ensure_cache_buf(&ctx);

  bench_result *results = (bench_result *)malloc(sizeof(bench_result) * n);
  if (!results) abort();
  size_t out_count = 0;

  for (size_t i = 0; i < n; ++i) {
    const bench_case *c = &cases[i];
    if (!match_filter(opts, c)) continue;

    /* warmup */
    uint32_t warm = opts ? opts->warmup : 0;
    for (uint32_t w = 0; w < warm; ++w) {
      c->fn(&ctx);
      bench_free_all(&ctx);
      if (opts && opts->flush_cache) flush_cache_best_effort(&ctx);
    }

    uint32_t iters = opts ? opts->iters : 1;
    if (iters == 0) iters = 1;

    bench_sample *samples = (bench_sample *)malloc(sizeof(bench_sample) * iters);
    if (!samples) abort();

    for (uint32_t k = 0; k < iters; ++k) {
      double t0 = bench_now_s();
      c->fn(&ctx);
      double t1 = bench_now_s();
      samples[k].seconds = (t1 - t0);
      bench_free_all(&ctx);
      if (opts && opts->flush_cache) flush_cache_best_effort(&ctx);
    }

    bench_compute_result(&results[out_count], c->suite, c->name, iters, samples, iters);
    out_count++;

    free(samples);
  }

  FILE *fp = open_out(opts);
  if (!fp) {
    free(results);
    arena_free_all(&ctx);
    free(ctx.cache_buf);
    return 2;
  }

  const char *fmt = (opts && opts->format) ? opts->format : "text";
  if (strcmp(fmt, "json") == 0) {
    bench_report_json(fp, results, out_count, opts);
  } else if (strcmp(fmt, "csv") == 0) {
    bench_report_csv(fp, results, out_count, opts);
  } else {
    bench_report_text(fp, results, out_count, opts);
  }

  /* baseline compare (writes to stderr for CI readability) */
  int brc = bench_compare_with_baseline(results, out_count,
                                       opts ? opts->baseline_json : NULL,
                                       opts ? opts->fail_above_pct : 0.0,
                                       stderr);

  if (fp != stdout) fclose(fp);

  free(results);
  arena_free_all(&ctx);
  free(ctx.cache_buf);

  return (brc != 0) ? brc : 0;
}
