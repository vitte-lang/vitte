
/*
  bench_parser.c

  Micro-benchmark for the Vitte parser (core + optional phrase).

  Measures
    - vitte_parse_program() wall time over synthetic sources.

  Workload
    - in-memory source generation (no file I/O)
    - repeated parse iterations with fresh parser state

  Notes
    - Current repository scaffold might implement only a subset of the grammar.
      This bench is written to degrade gracefully:
        * It can benchmark core parser only.
        * If phrase parser is implemented later, extend with a --mode phrase option.
    - If your parser allocates, this bench measures allocator cost too.
      Add an arena reset API later to isolate parse CPU.

  Usage
    bench_parser --bytes 1048576 --iters 200 --warmup 20
    bench_parser --mode core --funcs 2000 --stmts 8
    bench_parser --quiet

  Output
    - parses/s, MB/s, ns/parse, tokens estimate (best-effort)

  Exit
    0 ok
    1 error
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

#include "steel/vitte/parser_core.h"
#include "steel/vitte/ast.h"

/* ---------------- time ---------------- */

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

/* ---------------- args ---------------- */

typedef enum {
  BENCH_MODE_CORE = 1,
  BENCH_MODE_PHRASE = 2,
} bench_mode;

typedef struct {
  uint64_t iters;
  uint64_t warmup;
  uint64_t bytes;
  uint64_t funcs;
  uint64_t stmts;
  bench_mode mode;
  int quiet;
} bench_args;

static bench_args bench_args_default(void) {
  bench_args a;
  a.iters = 200;
  a.warmup = 20;
  a.bytes = 1024ull * 1024ull;
  a.funcs = 512;
  a.stmts = 6;
  a.mode = BENCH_MODE_CORE;
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

static int arg_mode(const char* s, bench_mode* out) {
  if (!s || !out) return -1;
  if (strcmp(s, "core") == 0) { *out = BENCH_MODE_CORE; return 0; }
  if (strcmp(s, "phrase") == 0) { *out = BENCH_MODE_PHRASE; return 0; }
  return -1;
}

static void usage(void) {
  puts(
    "bench_parser options:\n"
    "  --bytes N     approx input size in bytes (default 1048576)\n"
    "  --funcs N     functions to generate (default 512)\n"
    "  --stmts N     statements per function (default 6)\n"
    "  --mode core|phrase (default core)\n"
    "  --iters N     iterations (default 200)\n"
    "  --warmup N    warmup (default 20)\n"
    "  --quiet       print only ns/parse (integer)\n"
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
    if (strcmp(a, "--funcs") == 0 && i + 1 < argc) {
      if (arg_u64(argv[++i], &out->funcs) != 0) return -1;
      continue;
    }
    if (strcmp(a, "--stmts") == 0 && i + 1 < argc) {
      if (arg_u64(argv[++i], &out->stmts) != 0) return -1;
      continue;
    }
    if (strcmp(a, "--mode") == 0 && i + 1 < argc) {
      if (arg_mode(argv[++i], &out->mode) != 0) return -1;
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

/* ---------------- workload generation ---------------- */

static void append_str(char** p, size_t* cap, size_t* len, const char* s) {
  size_t n = strlen(s);
  if (*len + n + 1 > *cap) {
    size_t nc = (*cap == 0) ? 4096 : (*cap * 2);
    while (nc < *len + n + 1) nc *= 2;
    char* np = (char*)realloc(*p, nc);
    if (!np) {
      fprintf(stderr, "bench: OOM\n");
      exit(1);
    }
    *p = np;
    *cap = nc;
  }
  memcpy(*p + *len, s, n);
  *len += n;
  (*p)[*len] = 0;
}

static void append_u64(char** p, size_t* cap, size_t* len, uint64_t v) {
  char buf[64];
#if defined(_WIN32)
  _snprintf_s(buf, sizeof(buf), _TRUNCATE, "%llu", (unsigned long long)v);
#else
  snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
#endif
  append_str(p, cap, len, buf);
}

/* Core source (matches typical subset): module + many functions + .end blocks.

   Each function is shaped like:
     fn f123() -> i32
       let x: i32 = 1
       ...
       return 0
     .end
*/
static char* make_core_source(uint64_t funcs, uint64_t stmts, uint64_t target_bytes, uint32_t* out_len) {
  char* s = NULL;
  size_t cap = 0, len = 0;

  append_str(&s, &cap, &len, "module bench.parser\n\n");

  for (uint64_t i = 0; i < funcs; i++) {
    append_str(&s, &cap, &len, "fn f");
    append_u64(&s, &cap, &len, i);
    append_str(&s, &cap, &len, "() -> i32\n");

    for (uint64_t j = 0; j < stmts; j++) {
      append_str(&s, &cap, &len, "  let x: i32 = ");
      append_u64(&s, &cap, &len, (i + j) & 1023ull);
      append_str(&s, &cap, &len, "\n");
    }

    append_str(&s, &cap, &len, "  return 0\n");
    append_str(&s, &cap, &len, ".end\n\n");

    if ((uint64_t)len >= target_bytes) break;
  }

  if (len < 64) append_str(&s, &cap, &len, "fn main() -> i32\n  return 0\n.end\n");

  if (out_len) *out_len = (uint32_t)len;
  return s;
}

/* Phrase source: uses surface keywords mod/use/prog/set/say/ret/when/loop.
   This is optional; if phrase parser isn't implemented, the bench will still run
   but parse will likely fail (we count failures).
*/
static char* make_phrase_source(uint64_t funcs, uint64_t stmts, uint64_t target_bytes, uint32_t* out_len) {
  (void)stmts;
  char* s = NULL;
  size_t cap = 0, len = 0;

  append_str(&s, &cap, &len, "mod bench/parser\n");
  append_str(&s, &cap, &len, "use std.core as core\n\n");

  append_str(&s, &cap, &len, "prog bench/app.main\n");
  append_str(&s, &cap, &len, "  say \"bench phrase\"\n");
  append_str(&s, &cap, &len, "  set x = 1\n");
  append_str(&s, &cap, &len, "  when x == 1\n");
  append_str(&s, &cap, &len, "    say \"ok\"\n");
  append_str(&s, &cap, &len, "  .end\n");
  append_str(&s, &cap, &len, "  ret 0\n");
  append_str(&s, &cap, &len, ".end\n\n");

  /* Add many trivial blocks to scale input size. */
  for (uint64_t i = 0; i < funcs; i++) {
    append_str(&s, &cap, &len, "scn s");
    append_u64(&s, &cap, &len, i);
    append_str(&s, &cap, &len, "\n");
    append_str(&s, &cap, &len, "  say \"x\"\n");
    append_str(&s, &cap, &len, "  ret 0\n");
    append_str(&s, &cap, &len, ".end\n\n");
    if ((uint64_t)len >= target_bytes) break;
  }

  if (out_len) *out_len = (uint32_t)len;
  return s;
}

/* ---------------- benchmark ---------------- */

static int parse_once_core(const char* src, uint32_t len) {
  vitte_parser p;
  vitte_parser_init(&p, src, len, 0);

  ast_program prog;
  memset(&prog, 0, sizeof(prog));

  int ok = vitte_parse_program(&p, &prog);
  return (ok == 0 && !p.had_error) ? 0 : -1;
}

static void print_stats(const bench_args* a, uint64_t total_ns, uint64_t ok) {
  double iters = (double)a->iters;
  double ns_per = iters ? ((double)total_ns / iters) : 0.0;
  double sec = (double)total_ns / 1e9;

  double parses_s = (sec > 0.0) ? (iters / sec) : 0.0;
  double mb_total = ((double)a->bytes * iters) / (1024.0 * 1024.0);
  double mb_s = (sec > 0.0) ? (mb_total / sec) : 0.0;

  printf(
    "parser: mode=%s bytes~=%llu iters=%llu warmup=%llu\n"
    "  ok=%llu/%llu  total_ns=%llu  ns/parse=%.2f\n"
    "  parses/s=%.2f  MB/s=%.2f\n",
    (a->mode == BENCH_MODE_CORE) ? "core" : "phrase",
    (unsigned long long)a->bytes,
    (unsigned long long)a->iters,
    (unsigned long long)a->warmup,
    (unsigned long long)ok,
    (unsigned long long)a->iters,
    (unsigned long long)total_ns,
    ns_per,
    parses_s,
    mb_s
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
  char* src = NULL;

  if (a.mode == BENCH_MODE_CORE) {
    src = make_core_source(a.funcs, a.stmts, a.bytes, &len);
  } else {
    src = make_phrase_source(a.funcs, a.stmts, a.bytes, &len);
  }

  if (!src) {
    fprintf(stderr, "bench: OOM\n");
    return 1;
  }

  /* Warmup */
  for (uint64_t i = 0; i < a.warmup; i++) {
    (void)parse_once_core(src, len);
  }

  /* Measured */
  steel_timepoint t0 = steel_now();
  uint64_t ok = 0;
  for (uint64_t i = 0; i < a.iters; i++) {
    if (parse_once_core(src, len) == 0) ok++;
  }
  steel_timepoint t1 = steel_now();

  uint64_t total_ns = steel_ns_between(t0, t1);

  if (a.quiet) {
    uint64_t ns_per = (a.iters ? (total_ns / a.iters) : 0);
    printf("%llu\n", (unsigned long long)ns_per);
    free(src);
    return 0;
  }

  print_stats(&a, total_ns, ok);

  free(src);
  return 0;
}
