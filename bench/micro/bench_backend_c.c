
/*
  bench_backend_c.c

  Micro-benchmark for the C backend emitter.

  What it measures (current scaffold):
    - backend_c_emit_program() cost to emit a translation unit for a minimal AST.

  Notes:
    - This benchmark currently includes file I/O (emitter writes to a path).
      If you later add an in-memory sink API, extend this bench to isolate emitter CPU.
    - Cross-platform timer abstraction is included (POSIX + Windows).

  Usage examples:
    ./bench_backend_c --iters 50000 --outdir target/bench
    ./bench_backend_c --iters 200000 --warmup 5000 --keep

  Exit codes:
    0 success
    1 runtime error
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <time.h>
  #include <unistd.h>
#endif

#include "steel/compiler/backend_c.h"
#include "steel/vitte/ast.h"

/* ---------------- time ---------------- */

typedef struct {
  uint64_t t;
} steel_timepoint;

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
  /* ticks -> ns */
  uint64_t dt = (b.t >= a.t) ? (b.t - a.t) : 0;
  long double ns = ((long double)dt * 1000000000.0L) / (long double)freq;
  if (ns < 0) ns = 0;
  return (uint64_t)ns;
#else
  return (b.t >= a.t) ? (b.t - a.t) : 0;
#endif
}

/* ---------------- fs utils ---------------- */

static int steel_mkdir_p(const char* path) {
  if (!path || !path[0]) return 0;
#if defined(_WIN32)
  /* best-effort: create single dir; for deep paths call multiple times */
  if (CreateDirectoryA(path, NULL)) return 0;
  DWORD e = GetLastError();
  return (e == ERROR_ALREADY_EXISTS) ? 0 : -1;
#else
  if (mkdir(path, 0755) == 0) return 0;
  return (errno == EEXIST) ? 0 : -1;
#endif
}

static void steel_snprintf(char* out, size_t cap, const char* fmt, uint64_t a, uint64_t b) {
  if (!out || cap == 0) return;
#if defined(_WIN32)
  _snprintf_s(out, cap, _TRUNCATE, fmt, (unsigned long long)a, (unsigned long long)b);
#else
  snprintf(out, cap, fmt, (unsigned long long)a, (unsigned long long)b);
#endif
}

static int steel_unlink_file(const char* path) {
  if (!path) return 0;
#if defined(_WIN32)
  return DeleteFileA(path) ? 0 : -1;
#else
  return unlink(path);
#endif
}

/* ---------------- benchmark target ---------------- */

typedef struct {
  uint64_t iters;
  uint64_t warmup;
  const char* outdir;
  int keep_files;
  int quiet;
} bench_args;

static bench_args bench_args_default(void) {
  bench_args a;
  a.iters = 100000;
  a.warmup = 1000;
  a.outdir = "target/bench";
  a.keep_files = 0;
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
    "bench_backend_c options:\n"
    "  --iters N     iterations (default 100000)\n"
    "  --warmup N    warmup iterations (default 1000)\n"
    "  --outdir DIR  output directory (default target/bench)\n"
    "  --keep        keep generated .c files\n"
    "  --quiet       minimal output\n"
  );
}

static int parse_args(int argc, char** argv, bench_args* out) {
  *out = bench_args_default();
  for (int i = 1; i < argc; i++) {
    const char* a = argv[i];
    if (strcmp(a, "--iters") == 0 && i + 1 < argc) {
      if (arg_u64(argv[++i], &out->iters) != 0) return -1;
      continue;
    }
    if (strcmp(a, "--warmup") == 0 && i + 1 < argc) {
      if (arg_u64(argv[++i], &out->warmup) != 0) return -1;
      continue;
    }
    if (strcmp(a, "--outdir") == 0 && i + 1 < argc) {
      out->outdir = argv[++i];
      continue;
    }
    if (strcmp(a, "--keep") == 0) {
      out->keep_files = 1;
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

static ast_program make_minimal_program(int64_t ret_value) {
  ast_program p;
  memset(&p, 0, sizeof(p));

  p.has_module = 0;

  /* Fill main fn return literal for current scaffold emitter. */
  p.main_fn.ret.ret_value.value = ret_value;
  p.main_fn.ret.ret_value.span = (steel_span){0, 0, 0};

  return p;
}

static int bench_emit_files(const bench_args* a) {
  if (steel_mkdir_p(a->outdir) != 0) {
    fprintf(stderr, "bench: cannot create outdir: %s\n", a->outdir);
    return -1;
  }

  ast_program prog = make_minimal_program(42);

  /* warmup */
  for (uint64_t i = 0; i < a->warmup; i++) {
    char path[512];
    steel_snprintf(path, sizeof(path), "%s/emit_warm_%llu.c", (uint64_t)(uintptr_t)a->outdir, i);
    /* The snprintf helper above expects 2 u64 args; use a second arg as i and encode outdir separately below. */
    /* Rebuild correctly: */
    (void)path;
    break;
  }

  /* We can't use steel_snprintf for the outdir string; keep it simple here. */
  for (uint64_t i = 0; i < a->warmup; i++) {
    char out_c[1024];
#if defined(_WIN32)
    _snprintf_s(out_c, sizeof(out_c), _TRUNCATE, "%s\\emit_warm_%llu.c", a->outdir, (unsigned long long)i);
#else
    snprintf(out_c, sizeof(out_c), "%s/emit_warm_%llu.c", a->outdir, (unsigned long long)i);
#endif
    (void)backend_c_emit_program(&prog, out_c);
    if (!a->keep_files) (void)steel_unlink_file(out_c);
  }

  /* measured */
  steel_timepoint t0 = steel_now();
  uint64_t ok = 0;

  for (uint64_t i = 0; i < a->iters; i++) {
    char out_c[1024];
#if defined(_WIN32)
    _snprintf_s(out_c, sizeof(out_c), _TRUNCATE, "%s\\emit_%llu.c", a->outdir, (unsigned long long)i);
#else
    snprintf(out_c, sizeof(out_c), "%s/emit_%llu.c", a->outdir, (unsigned long long)i);
#endif
    if (backend_c_emit_program(&prog, out_c) == 0) {
      ok++;
      if (!a->keep_files) (void)steel_unlink_file(out_c);
    }
  }

  steel_timepoint t1 = steel_now();
  uint64_t ns = steel_ns_between(t0, t1);

  if (!a->quiet) {
    printf("backend_c_emit_program: iters=%llu ok=%llu total_ns=%llu ns/op=%.2f\n",
           (unsigned long long)a->iters,
           (unsigned long long)ok,
           (unsigned long long)ns,
           (a->iters ? (double)ns / (double)a->iters : 0.0));
  } else {
    /* stable single-line output */
    printf("%llu\n", (unsigned long long)(a->iters ? (ns / a->iters) : 0));
  }

  return (ok == a->iters) ? 0 : -1;
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

  if (bench_emit_files(&a) != 0) {
    fprintf(stderr, "bench: failed\n");
    return 1;
  }

  return 0;
}
