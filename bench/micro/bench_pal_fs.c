/*
  bench_pal_fs.c

  Micro-benchmark for PAL filesystem primitives.

  Goal
    - Measure overhead of PAL fs operations across platforms (POSIX + Win32).
    - Provide stable, repeatable metrics for regression tracking.

  What it measures
    - create/write/close (small + medium payload)
    - open/read/close
    - stat
    - listdir (best-effort)
    - mkdir/unlink/rmdir

  Notes
    - This file currently benchmarks native OS syscalls directly.
    - When PAL is implemented (pal_posix_fs.c / pal_win32_fs.c), replace the
      syscall wrappers below with PAL calls to measure PAL overhead.
    - All timings are wall-clock monotonic (POSIX clock_gettime, Windows QPC).

  Usage
    bench_pal_fs --iters 2000 --warmup 100 --dir target/bench_fs --payload 4096
    bench_pal_fs --iters 20000 --payload 64 --quiet

  Output
    - ns/op and ops/s per operation, plus aggregate

  Exit
    0 ok
    1 error
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <io.h>
  #include <direct.h>
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <dirent.h>
  #include <fcntl.h>
  #include <unistd.h>
#endif

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

typedef struct {
  uint64_t iters;
  uint64_t warmup;
  const char* dir;
  uint64_t payload;
  int quiet;
} bench_args;

static bench_args bench_args_default(void) {
  bench_args a;
  a.iters = 2000;
  a.warmup = 100;
  a.dir = "target/bench_fs";
  a.payload = 4096;
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
    "bench_pal_fs options:\n"
    "  --iters N     iterations (default 2000)\n"
    "  --warmup N    warmup (default 100)\n"
    "  --dir DIR     base directory (default target/bench_fs)\n"
    "  --payload N   bytes to write/read (default 4096)\n"
    "  --quiet       minimal output (ns/op only)\n"
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
    if (strcmp(a, "--dir") == 0 && i + 1 < argc) {
      out->dir = argv[++i];
      continue;
    }
    if (strcmp(a, "--payload") == 0 && i + 1 < argc) {
      if (arg_u64(argv[++i], &out->payload) != 0) return -1;
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

/* ---------------- platform helpers ---------------- */

static int steel_mkdir_p(const char* path) {
  if (!path || !path[0]) return 0;

  char buf[1024];
  size_t n = 0;
  while (path[n] && n + 1 < sizeof(buf)) { buf[n] = path[n]; n++; }
  buf[n] = 0;

#if defined(_WIN32)
  for (size_t i = 0; buf[i]; i++) if (buf[i] == '/') buf[i] = '\\';
#endif

  for (size_t i = 1; buf[i]; i++) {
#if defined(_WIN32)
    if (buf[i] == '\\') {
#else
    if (buf[i] == '/') {
#endif
      char saved = buf[i];
      buf[i] = 0;
#if defined(_WIN32)
      (void)CreateDirectoryA(buf, NULL);
#else
      (void)mkdir(buf, 0755);
#endif
      buf[i] = saved;
    }
  }

#if defined(_WIN32)
  if (CreateDirectoryA(buf, NULL)) return 0;
  DWORD e = GetLastError();
  return (e == ERROR_ALREADY_EXISTS) ? 0 : -1;
#else
  if (mkdir(buf, 0755) == 0) return 0;
  return (errno == EEXIST) ? 0 : -1;
#endif
}

static int steel_rmdir_dir(const char* path) {
#if defined(_WIN32)
  return RemoveDirectoryA(path) ? 0 : -1;
#else
  return rmdir(path);
#endif
}

static int steel_unlink_file(const char* path) {
#if defined(_WIN32)
  return DeleteFileA(path) ? 0 : -1;
#else
  return unlink(path);
#endif
}

static int steel_write_file(const char* path, const uint8_t* data, size_t n) {
#if defined(_WIN32)
  HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) return -1;
  DWORD written = 0;
  BOOL ok = WriteFile(h, data, (DWORD)n, &written, NULL);
  CloseHandle(h);
  if (!ok || written != (DWORD)n) return -1;
  return 0;
#else
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) return -1;
  size_t off = 0;
  while (off < n) {
    ssize_t w = write(fd, data + off, n - off);
    if (w < 0) { close(fd); return -1; }
    off += (size_t)w;
  }
  close(fd);
  return 0;
#endif
}

static int steel_read_file(const char* path, uint8_t* out, size_t n) {
#if defined(_WIN32)
  HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) return -1;
  DWORD readn = 0;
  BOOL ok = ReadFile(h, out, (DWORD)n, &readn, NULL);
  CloseHandle(h);
  if (!ok || readn != (DWORD)n) return -1;
  return 0;
#else
  int fd = open(path, O_RDONLY);
  if (fd < 0) return -1;
  size_t off = 0;
  while (off < n) {
    ssize_t r = read(fd, out + off, n - off);
    if (r <= 0) { close(fd); return -1; }
    off += (size_t)r;
  }
  close(fd);
  return 0;
#endif
}

static int steel_stat_file(const char* path) {
#if defined(_WIN32)
  WIN32_FILE_ATTRIBUTE_DATA fad;
  return GetFileAttributesExA(path, GetFileExInfoStandard, &fad) ? 0 : -1;
#else
  struct stat st;
  return stat(path, &st);
#endif
}

static int steel_list_dir_best_effort(const char* path) {
#if defined(_WIN32)
  char pat[1024];
  _snprintf_s(pat, sizeof(pat), _TRUNCATE, "%s\\*", path);

  WIN32_FIND_DATAA fd;
  HANDLE h = FindFirstFileA(pat, &fd);
  if (h == INVALID_HANDLE_VALUE) return -1;

  uint64_t count = 0;
  do {
    const char* n = fd.cFileName;
    if (strcmp(n, ".") == 0 || strcmp(n, "..") == 0) continue;
    count++;
  } while (FindNextFileA(h, &fd));

  FindClose(h);
  return (count >= 0) ? 0 : -1;
#else
  DIR* d = opendir(path);
  if (!d) return -1;

  uint64_t count = 0;
  for (;;) {
    struct dirent* e = readdir(d);
    if (!e) break;
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
    count++;
  }

  closedir(d);
  return (count >= 0) ? 0 : -1;
#endif
}

static void make_path(char* out, size_t cap, const char* dir, const char* name) {
#if defined(_WIN32)
  _snprintf_s(out, cap, _TRUNCATE, "%s\\%s", dir, name);
#else
  snprintf(out, cap, "%s/%s", dir, name);
#endif
}

/* ---------------- measurement harness ---------------- */

typedef int (*op_fn)(void* ctx);

typedef struct {
  const char* name;
  op_fn fn;
} op;

static uint64_t run_op(const bench_args* a, const op* o, void* ctx, uint64_t* out_ok) {
  *out_ok = 0;

  for (uint64_t i = 0; i < a->warmup; i++) {
    (void)o->fn(ctx);
  }

  steel_timepoint t0 = steel_now();
  uint64_t ok = 0;
  for (uint64_t i = 0; i < a->iters; i++) {
    if (o->fn(ctx) == 0) ok++;
  }
  steel_timepoint t1 = steel_now();

  *out_ok = ok;
  return steel_ns_between(t0, t1);
}

static void print_row(const bench_args* a, const char* name, uint64_t ns, uint64_t ok) {
  double ns_per = (a->iters ? ((double)ns / (double)a->iters) : 0.0);
  double sec = (double)ns / 1e9;
  double ops_s = (sec > 0.0) ? ((double)a->iters / sec) : 0.0;

  printf("%-18s  ok=%7llu/%7llu  ns/op=%10.2f  ops/s=%10.0f\n",
         name,
         (unsigned long long)ok,
         (unsigned long long)a->iters,
         ns_per,
         ops_s);
}

/* ---------------- ops ---------------- */

typedef struct {
  const bench_args* a;
  char file_a[1024];
  char file_b[1024];
  uint8_t* buf;
  uint8_t* buf2;
  size_t n;
} fs_ctx;

static int op_write_small(void* ctx_) {
  fs_ctx* c = (fs_ctx*)ctx_;
  return steel_write_file(c->file_a, c->buf, 64);
}

static int op_write_payload(void* ctx_) {
  fs_ctx* c = (fs_ctx*)ctx_;
  return steel_write_file(c->file_a, c->buf, c->n);
}

static int op_read_payload(void* ctx_) {
  fs_ctx* c = (fs_ctx*)ctx_;
  return steel_read_file(c->file_a, c->buf2, c->n);
}

static int op_stat_payload(void* ctx_) {
  fs_ctx* c = (fs_ctx*)ctx_;
  return steel_stat_file(c->file_a);
}

static int op_list_dir(void* ctx_) {
  fs_ctx* c = (fs_ctx*)ctx_;
  return steel_list_dir_best_effort(c->a->dir);
}

static int op_unlink_payload(void* ctx_) {
  fs_ctx* c = (fs_ctx*)ctx_;
  return steel_unlink_file(c->file_a);
}

static int op_mkdir_temp(void* ctx_) {
  fs_ctx* c = (fs_ctx*)ctx_;
  return steel_mkdir_p(c->file_b);
}

static int op_rmdir_temp(void* ctx_) {
  fs_ctx* c = (fs_ctx*)ctx_;
  return steel_rmdir_dir(c->file_b);
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

  if (steel_mkdir_p(a.dir) != 0) {
    fprintf(stderr, "bench: cannot create dir: %s\n", a.dir);
    return 1;
  }

  fs_ctx ctx;
  memset(&ctx, 0, sizeof(ctx));
  ctx.a = &a;
  ctx.n = (size_t)a.payload;

  make_path(ctx.file_a, sizeof(ctx.file_a), a.dir, "bench_payload.bin");
  make_path(ctx.file_b, sizeof(ctx.file_b), a.dir, "tmpdir");

  ctx.buf = (uint8_t*)malloc(ctx.n < 64 ? 64 : ctx.n);
  ctx.buf2 = (uint8_t*)malloc(ctx.n < 64 ? 64 : ctx.n);
  if (!ctx.buf || !ctx.buf2) {
    fprintf(stderr, "bench: OOM (payload=%llu)\n", (unsigned long long)a.payload);
    return 1;
  }

  /* deterministic fill */
  for (size_t i = 0; i < (ctx.n < 64 ? 64 : ctx.n); i++) {
    ctx.buf[i] = (uint8_t)(i * 131u + 17u);
    ctx.buf2[i] = 0;
  }

  /* Ensure payload exists for read/stat ops */
  if (steel_write_file(ctx.file_a, ctx.buf, ctx.n) != 0) {
    fprintf(stderr, "bench: initial write failed\n");
    return 1;
  }

  op ops[] = {
    { "write_small",   op_write_small },
    { "write_payload", op_write_payload },
    { "read_payload",  op_read_payload },
    { "stat",          op_stat_payload },
    { "listdir",       op_list_dir },
    { "unlink",        op_unlink_payload },
    { "mkdir",         op_mkdir_temp },
    { "rmdir",         op_rmdir_temp },
  };

  uint64_t agg_ns = 0;

  for (size_t i = 0; i < sizeof(ops)/sizeof(ops[0]); i++) {
    uint64_t ok = 0;

    /* Prep for specific ops */
    if (strcmp(ops[i].name, "read_payload") == 0 || strcmp(ops[i].name, "stat") == 0) {
      (void)steel_write_file(ctx.file_a, ctx.buf, ctx.n);
    }
    if (strcmp(ops[i].name, "unlink") == 0) {
      (void)steel_write_file(ctx.file_a, ctx.buf, ctx.n);
    }
    if (strcmp(ops[i].name, "rmdir") == 0) {
      (void)steel_mkdir_p(ctx.file_b);
    }

    uint64_t ns = run_op(&a, &ops[i], &ctx, &ok);
    agg_ns += ns;

    if (!a.quiet) print_row(&a, ops[i].name, ns, ok);

    /* Clean after ops */
    if (strcmp(ops[i].name, "write_small") == 0 || strcmp(ops[i].name, "write_payload") == 0) {
      (void)steel_unlink_file(ctx.file_a);
    }
    if (strcmp(ops[i].name, "mkdir") == 0) {
      (void)steel_rmdir_dir(ctx.file_b);
    }
  }

  if (a.quiet) {
    uint64_t ns_per = (a.iters ? (agg_ns / a.iters) : 0);
    printf("%llu\n", (unsigned long long)ns_per);
  } else {
    double sec = (double)agg_ns / 1e9;
    double total_ops = (double)a.iters * (double)(sizeof(ops)/sizeof(ops[0]));
    double ops_s = (sec > 0.0) ? (total_ops / sec) : 0.0;
    printf("aggregate: ops=%llu total_ns=%llu ops/s=%.0f\n",
           (unsigned long long)total_ops,
           (unsigned long long)agg_ns,
           ops_s);
  }

  /* Best-effort cleanup */
  (void)steel_unlink_file(ctx.file_a);
  (void)steel_rmdir_dir(ctx.file_b);

  free(ctx.buf);
  free(ctx.buf2);

  return 0;
}
