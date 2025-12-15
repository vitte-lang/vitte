/*
  bench_lexer.c

  Micro-benchmark for the Vitte lexer.

  Measures:
    - vitte_lex_next() throughput while scanning a synthetic Vitte source buffer.

  Notes:
    - This does not measure file I/O (source is generated in-memory).
    - Runtime depends on input size (public).

  Usage:
    bench_lexer --bytes 1048576 --iters 200 --warmup 20
    bench_lexer --bytes 8388608 --iters 50 --quiet

  Output:
    - MB/s and tokens/s estimates (best-effort), plus ns/op per iteration.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <time.h>
#endif

#include "steel/vitte/lexer.h"
#include "steel/vitte/tokens.h"

typedef struct { uint64_t t; } steel_timepoint;

static steel_timepoint steel_now(void) {
  steel_timepoint tp;
#if defined(_WIN32)
  LARGE_INTEGER c;
  QueryPerformanceCounter(&c);
  tp.t = (uint64_t)c.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  tp.t = (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
  return tp;
}

static uint64_t steel_ns_between(steel_timepoint a, steel_timepoint b) {
#if defined(_WIN32)
  static uint64_t freq = 0;
  if (!freq) {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    freq = (uint64_t)f.QuadPart;
    if (!freq) freq = 1;
  }
  uint64_t dt = (b.t >= a.t) ? (b.t - a.t) : 0;
  long double ns = ((long double)dt * 1000000000.0L) / (long double)freq;
  if (ns < 0) ns = 0;
  return (uint64_t)ns;
#else
  return (b.t >= a.t) ? (b.t - a.t) : 0;
#endif
}

typedef struct {
  uint64_t iters;
  uint64_t warmup;
  uint64_t bytes;
  int quiet;
} bench_args;

static bench_args bench_args_default(void) {
  bench_args a;
  a.iters = 200;
  a.warmup = 20;
  a.bytes = 1024ull * 1024ull;
  a.quiet = 0;
  return a;
}

static int arg_u64(const char* s, uint64_t* out) {
  if (!s || !out) return -1;
  char* end = NULL;
  unsigned long long v = strtoull(s, &end, 10);
  if (!end || *end != 0) return -1;
  *out = (uint64_t)v;
  return 0;
}

static void usage(void) {
  puts(
    "bench_lexer options:\n"
    "  --bytes N    input size in bytes (default 1048576)\n"
    "  --iters N    iterations (default 200)\n"
    "  --warmup N   warmup iterations (default 20)\n"
    "  --quiet      print only ns/op (integer)\n"
  );
}

static int parse_args(int argc, char** argv, bench_args* out) {
  *out = bench_args_default();
  for (int i = 1; i < argc; i++) {
    const char* a = argv[i];
    if (strcmp(a, "--bytes") == 0 && i + 1 < argc) {
      if (arg_u64(argv[++i], &out->bytes) != 0) return -1;
      continue;
    }
    if (strcmp(a, "--iters") == 0 && i + 1 < argc) {
      if (arg_u64(argv[++i], &out->iters) != 0) return -1;
      continue;
    }
    if (strcmp(a, "--warmup") == 0 && i + 1 < argc) {
      if (arg_u64(argv[++i], &out->warmup) != 0) return -1;
      continue;
    }
    if (strcmp(a, "--quiet") == 0) {
      out->quiet = 1;
      continue;
    }
    if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      usage();
      exit(0);
    }
    return -1;
  }
  return 0;
}

/* Generate a synthetic Vitte buffer that stresses identifiers, ints, keywords, punctuation and whitespace.

   We keep it ASCII-only and grammar-friendly; the lexer should handle it easily.
*/
static char* make_source(uint64_t bytes, uint32_t* out_len) {
  const char* chunk =
    "module bench.lexer\n"
    "\n"
    "fn main() -> i32\n"
    "  let x: i32 = 12345\n"
    "  let y: i32 = x + 67890\n"
    "  return y\n"
    ".end\n"
    "\n"
    "# comment line\n";

  size_t chunk_len = strlen(chunk);
  if (bytes < 64) bytes = 64;

  char* buf = (char*)malloc((size_t)bytes + 1);
  if (!buf) return NULL;

  size_t filled = 0;
  while (filled + chunk_len <= (size_t)bytes) {
    memcpy(buf + filled, chunk, chunk_len);
    filled += chunk_len;
  }

  /* Tail: repeat a smaller token-y pattern to reach target size. */
  const char* tail = " let z: i32 = x + y\n";
  size_t tail_len = strlen(tail);
  while (filled + tail_len <= (size_t)bytes) {
    memcpy(buf + filled, tail, tail_len);
    filled += tail_len;
  }

  /* Fill remaining with whitespace/newlines (valid). */
  while (filled < (size_t)bytes) {
    buf[filled++] = (filled % 16 == 0) ? '\n' : ' ';
  }

  buf[bytes] = 0;
  if (out_len) *out_len = (uint32_t)bytes;
  return buf;
}

static uint64_t lex_all_tokens(const char* src, uint32_t len) {
  vitte_lexer lx;
  vitte_lexer_init(&lx, src, len, 0);

  uint64_t toks = 0;
  for (;;) {
    vitte_token t = vitte_lex_next(&lx);
    toks++;
    if (t.kind == TK_EOF) break;
  }
  return toks;
}

static void print_stats(const bench_args* a, uint64_t total_ns, uint64_t total_tokens) {
  double iters = (double)a->iters;
  double ns_per = iters ? ((double)total_ns / iters) : 0.0;

  double bytes_total = (double)a->bytes * iters;
  double sec = (double)total_ns / 1e9;
  double mb_s = (sec > 0.0) ? (bytes_total / (1024.0 * 1024.0) / sec) : 0.0;

  double toks_total = (double)total_tokens;
  double toks_s = (sec > 0.0) ? (toks_total / sec) : 0.0;

  printf(
    "lexer: bytes=%llu iters=%llu warmup=%llu\n"
    "  total_ns=%llu  ns/iter=%.2f\n"
    "  MB/s=%.2f  tokens/s=%.0f  tokens/iter=%.0f\n",
    (unsigned long long)a->bytes,
    (unsigned long long)a->iters,
    (unsigned long long)a->warmup,
    (unsigned long long)total_ns,
    ns_per,
    mb_s,
    toks_s,
    (a->iters ? (toks_total / iters) : 0.0)
  );
}

int main(int argc, char** argv) {
  bench_args a;
  if (parse_args(argc, argv, &a) != 0) {
    usage();
    return 1;
  }

  if (a.iters == 0) {
    fprintf(stderr, "bench: --iters must be > 0\n");
    return 1;
  }

  uint32_t len = 0;
  char* src = make_source(a.bytes, &len);
  if (!src) {
    fprintf(stderr, "bench: out of memory (bytes=%llu)\n", (unsigned long long)a.bytes);
    return 1;
  }

  /* Warmup */
  for (uint64_t i = 0; i < a.warmup; i++) {
    (void)lex_all_tokens(src, len);
  }

  /* Measured */
  steel_timepoint t0 = steel_now();
  uint64_t total_tokens = 0;
  for (uint64_t i = 0; i < a.iters; i++) {
    total_tokens += lex_all_tokens(src, len);
  }
  steel_timepoint t1 = steel_now();

  uint64_t total_ns = steel_ns_between(t0, t1);

  if (a.quiet) {
    uint64_t ns_per = (a.iters ? (total_ns / a.iters) : 0);
    printf("%llu\n", (unsigned long long)ns_per);
    free(src);
    return 0;
  }

  print_stats(&a, total_ns, total_tokens);

  free(src);
  return 0;
}
