// bench_main.c — Vitte benchmark runner (C17, max)
// -----------------------------------------------------------------------------
// Objectif : un binaire CLI unique pour exécuter les micro/benchmarks enregistrés
// via un registry (bench_registry.c), avec filtrage, répétitions, warmup,
// sorties texte/JSON/CSV, stats (min/mean/p50/p90/stddev), et mesures ns/cycles.
//
// Contrat attendu (implémenté ailleurs, typiquement bench_registry.c) :
//   - un tableau statique de cas (suite/name/fn)
//   - chaque fn exécute un cas et renvoie un résultat (elapsed_ns + checksum)
//   - bench_main.c ne dépend pas de ton framework interne, juste de ce contrat.
//
// Si ton repo expose déjà bench.h, adapte les typedef/extern ci-dessous en 1:1.
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

// ============================================================================
// Interface attendue du registry (à fournir dans bench_registry.c)
// ============================================================================

typedef struct bench_result {
  uint64_t elapsed_ns; // temps total (ns)
  uint64_t checksum;   // anti-DCE
  uint64_t iters;      // itérations réellement exécutées (>= demandé si auto-scale)
  size_t   size;       // paramètre size
  uint64_t cycles;     // optionnel : delta cycles (0 si non supporté)
} bench_result;

typedef bench_result (*bench_fn)(uint64_t iters, size_t size, uint64_t seed);

typedef struct bench_case {
  const char* suite;   // ex: "alloc", "micro", "json"
  const char* name;    // ex: "malloc_free"
  bench_fn     fn;     // impl
  uint32_t     flags;  // réservé (0)
} bench_case;

// Doit renvoyer un pointeur sur un tableau statique et écrire out_count.
extern const bench_case* bench_registry_list(size_t* out_count);

// Optionnel : init (si ton registry fait du lazy init / auto-registration)
extern void bench_registry_init(void);

// Optionnel : compteur cycles (bench_asm_shim.c). Si absent, on fallback.
extern uint64_t bench_asm_cycles_begin(void);
extern uint64_t bench_asm_cycles_end(void);

// ============================================================================
// Temps monotone ns (fallback robuste)
// ============================================================================

static uint64_t time_now_ns(void) {
#if defined(_WIN32)
  static LARGE_INTEGER freq;
  static int init = 0;
  if (!init) {
    QueryPerformanceFrequency(&freq);
    init = 1;
  }
  LARGE_INTEGER c;
  QueryPerformanceCounter(&c);

  const uint64_t ticks = (uint64_t)c.QuadPart;
  const uint64_t f     = (uint64_t)freq.QuadPart;
  const uint64_t sec   = ticks / f;
  const uint64_t rem   = ticks - sec * f;

  return sec * UINT64_C(1000000000) + (rem * UINT64_C(1000000000) / f);
#else
  // POSIX
  #include <time.h>
  struct timespec ts;
  (void)clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
#endif
}

// ============================================================================
// Parsing util
// ============================================================================

static int parse_u64(const char* s, uint64_t* out) {
  if (!s || !*s) return 0;
  errno = 0;
  char* end = NULL;
  unsigned long long v = strtoull(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') return 0;
  if (out) *out = (uint64_t)v;
  return 1;
}

static int parse_size(const char* s, size_t* out) {
  uint64_t v = 0;
  if (!parse_u64(s, &v)) return 0;
  if (out) *out = (size_t)v;
  return 1;
}

static const char* safe_str(const char* s) { return s ? s : ""; }

// wildcard match: '*' '?' (ASCII)
static int wildmatch(const char* pat, const char* str) {
  // iterative backtracking
  const char* p = pat;
  const char* s = str;
  const char* star = NULL;
  const char* ss = NULL;

  while (*s) {
    if (*p == '*') {
      star = p++;
      ss = s;
      continue;
    }
    if (*p == '?' || *p == *s) {
      p++; s++;
      continue;
    }
    if (star) {
      p = star + 1;
      s = ++ss;
      continue;
    }
    return 0;
  }
  while (*p == '*') p++;
  return (*p == '\0');
}

// ============================================================================
// Stats
// ============================================================================

typedef struct stats {
  double min;
  double max;
  double mean;
  double stddev;
  double p50;
  double p90;
} stats;

static int cmp_dbl(const void* a, const void* b) {
  const double da = *(const double*)a;
  const double db = *(const double*)b;
  return (da < db) ? -1 : (da > db) ? 1 : 0;
}

static double percentile_sorted(const double* v, size_t n, double p) {
  if (n == 0) return 0.0;
  if (p <= 0.0) return v[0];
  if (p >= 1.0) return v[n - 1];
  const double x = p * (double)(n - 1);
  const size_t i = (size_t)x;
  const double f = x - (double)i;
  if (i + 1 >= n) return v[n - 1];
  return v[i] * (1.0 - f) + v[i + 1] * f;
}

static stats compute_stats(const double* samples, size_t n) {
  stats st;
  st.min = 0; st.max = 0; st.mean = 0; st.stddev = 0; st.p50 = 0; st.p90 = 0;
  if (n == 0) return st;

  double sum = 0.0;
  double mn = samples[0], mx = samples[0];
  for (size_t i = 0; i < n; ++i) {
    const double x = samples[i];
    sum += x;
    if (x < mn) mn = x;
    if (x > mx) mx = x;
  }
  const double mean = sum / (double)n;

  double var = 0.0;
  for (size_t i = 0; i < n; ++i) {
    const double d = samples[i] - mean;
    var += d * d;
  }
  var = (n > 1) ? (var / (double)(n - 1)) : 0.0;

  // percentiles require sorted copy
  double* tmp = (double*)malloc(n * sizeof(double));
  if (tmp) {
    memcpy(tmp, samples, n * sizeof(double));
    qsort(tmp, n, sizeof(double), cmp_dbl);
    st.p50 = percentile_sorted(tmp, n, 0.50);
    st.p90 = percentile_sorted(tmp, n, 0.90);
    free(tmp);
  } else {
    st.p50 = mean;
    st.p90 = mean;
  }

  st.min = mn;
  st.max = mx;
  st.mean = mean;
  st.stddev = sqrt(var);
  return st;
}

// ============================================================================
// Options CLI
// ============================================================================

typedef struct opts {
  const char* suite_pat;     // wildcard
  const char* case_pat;      // wildcard
  uint64_t iters;
  size_t   size;
  uint64_t seed;
  uint32_t warmup;           // nb runs warmup (non comptés)
  uint32_t repeats;          // nb runs mesurés
  uint32_t min_time_ms;      // auto-scale iters pour atteindre ce temps (0 = off)
  int      list;
  int      json;
  int      csv;
  const char* out_path;      // "-" ou NULL = stdout
} opts;

static void print_usage(FILE* f, const char* argv0) {
  fprintf(f,
    "Usage: %s [options]\n"
    "\n"
    "Selection:\n"
    "  --suite <pat>        suite wildcard (default: *)\n"
    "  --case <pat>         case wildcard  (default: *)\n"
    "  --list               list cases and exit\n"
    "\n"
    "Run params:\n"
    "  --iters <N>          iterations per run (default: 500000)\n"
    "  --size <B>           size parameter (default: 64)\n"
    "  --seed <S>           seed (default: 0x1234..)\n"
    "  --warmup <N>         warmup runs (default: 1)\n"
    "  --repeats <N>        measured runs (default: 5)\n"
    "  --min-time-ms <MS>   auto-scale iters to reach min time per run (default: 0)\n"
    "\n"
    "Output:\n"
    "  --json               JSON lines (one per run + one summary)\n"
    "  --csv                CSV (one summary line per case)\n"
    "  --out <path>         output file (default: stdout)\n"
    "\n"
    "Examples:\n"
    "  %s --list\n"
    "  %s --suite alloc --case * --iters 500000 --size 64\n"
    "  %s --suite micro --case memcpy* --min-time-ms 50 --repeats 10 --json\n",
    argv0, argv0, argv0, argv0
  );
}

static int parse_args(int argc, char** argv, opts* o) {
  o->suite_pat = "*";
  o->case_pat  = "*";
  o->iters     = 500000;
  o->size      = 64;
  o->seed      = UINT64_C(0x123456789ABCDEF0);
  o->warmup    = 1;
  o->repeats   = 5;
  o->min_time_ms = 0;
  o->list      = 0;
  o->json      = 0;
  o->csv       = 0;
  o->out_path  = NULL;

  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];

    if (!strcmp(a, "--help") || !strcmp(a, "-h")) {
      print_usage(stdout, argv[0]);
      exit(0);
    } else if (!strcmp(a, "--list")) {
      o->list = 1;
    } else if (!strcmp(a, "--json")) {
      o->json = 1;
    } else if (!strcmp(a, "--csv")) {
      o->csv = 1;
    } else if (!strcmp(a, "--suite") && i + 1 < argc) {
      o->suite_pat = argv[++i];
    } else if (!strcmp(a, "--case") && i + 1 < argc) {
      o->case_pat = argv[++i];
    } else if (!strcmp(a, "--iters") && i + 1 < argc) {
      if (!parse_u64(argv[++i], &o->iters)) return 0;
    } else if (!strcmp(a, "--size") && i + 1 < argc) {
      if (!parse_size(argv[++i], &o->size)) return 0;
    } else if (!strcmp(a, "--seed") && i + 1 < argc) {
      if (!parse_u64(argv[++i], &o->seed)) return 0;
    } else if (!strcmp(a, "--warmup") && i + 1 < argc) {
      uint64_t v = 0; if (!parse_u64(argv[++i], &v)) return 0; o->warmup = (uint32_t)v;
    } else if (!strcmp(a, "--repeats") && i + 1 < argc) {
      uint64_t v = 0; if (!parse_u64(argv[++i], &v)) return 0; o->repeats = (uint32_t)v;
    } else if (!strcmp(a, "--min-time-ms") && i + 1 < argc) {
      uint64_t v = 0; if (!parse_u64(argv[++i], &v)) return 0; o->min_time_ms = (uint32_t)v;
    } else if (!strcmp(a, "--out") && i + 1 < argc) {
      o->out_path = argv[++i];
    } else {
      fprintf(stderr, "Unknown arg: %s\n", a);
      return 0;
    }
  }

  if (o->repeats == 0) o->repeats = 1;
  return 1;
}

// ============================================================================
// Runner
// ============================================================================

static FILE* open_out(const char* path) {
  if (!path || !strcmp(path, "-")) return stdout;
  FILE* f = fopen(path, "wb");
  if (!f) {
    fprintf(stderr, "Failed to open --out '%s'\n", path);
    return NULL;
  }
  return f;
}

static void close_out(FILE* f) {
  if (f && f != stdout) fclose(f);
}

static void list_cases(FILE* out) {
  size_t n = 0;
  const bench_case* cases = bench_registry_list(&n);
  for (size_t i = 0; i < n; ++i) {
    fprintf(out, "%s/%s\n", safe_str(cases[i].suite), safe_str(cases[i].name));
  }
}

static int case_selected(const bench_case* c, const opts* o) {
  if (!wildmatch(o->suite_pat, safe_str(c->suite))) return 0;
  if (!wildmatch(o->case_pat,  safe_str(c->name)))  return 0;
  return 1;
}

static bench_result run_one(const bench_case* c, uint64_t iters, size_t size, uint64_t seed) {
  // Prefer fn-provided elapsed_ns. If elapsed_ns==0, bench_main measures around.
  const uint64_t t0 = time_now_ns();
  uint64_t cy0 = 0, cy1 = 0;

  // cycles begin/end : si bench_asm_shim est linké, ça marche; sinon link error.
  // Pour éviter un link error, tu peux fournir des stubs dans bench_asm_shim.c
  // ou commenter ces lignes.
  cy0 = bench_asm_cycles_begin();
  bench_result r = c->fn(iters, size, seed);
  cy1 = bench_asm_cycles_end();

  const uint64_t t1 = time_now_ns();

  if (r.elapsed_ns == 0) {
    r.elapsed_ns = (t1 >= t0) ? (t1 - t0) : 0;
  }
  if (r.cycles == 0 && cy1 >= cy0) {
    r.cycles = (cy1 - cy0);
  }
  if (r.iters == 0) r.iters = iters;
  r.size = size;
  return r;
}

static uint64_t autoscale_iters(const bench_case* c, const opts* o) {
  if (o->min_time_ms == 0) return o->iters;

  const uint64_t target_ns = (uint64_t)o->min_time_ms * UINT64_C(1000000);
  uint64_t it = (o->iters > 0) ? o->iters : 1;

  // ramp-up : doubler jusqu'à atteindre target ou overflow garde-fou
  for (int step = 0; step < 20; ++step) {
    bench_result r = run_one(c, it, o->size, o->seed ^ (uint64_t)step);
    if (r.elapsed_ns >= target_ns) return it;

    // éviter overflow
    if (it > (UINT64_MAX / 2)) return it;
    it *= 2;
  }
  return it;
}

static void print_json_run(FILE* out, const bench_case* c, const bench_result* r,
                           uint32_t run_idx, uint32_t warmup, uint64_t iters_req) {
  // JSON lines (une par ligne)
  fprintf(out,
    "{"
      "\"type\":\"run\","
      "\"suite\":\"%s\","
      "\"case\":\"%s\","
      "\"run\":%u,"
      "\"warmup\":%u,"
      "\"iters_req\":%llu,"
      "\"iters\":%llu,"
      "\"size\":%llu,"
      "\"elapsed_ns\":%llu,"
      "\"ns_per_iter\":%.6f,"
      "\"ops_per_s\":%.6f,"
      "\"cycles\":%llu,"
      "\"checksum\":%llu"
    "}\n",
    safe_str(c->suite),
    safe_str(c->name),
    (unsigned)run_idx,
    (unsigned)warmup,
    (unsigned long long)iters_req,
    (unsigned long long)r->iters,
    (unsigned long long)r->size,
    (unsigned long long)r->elapsed_ns,
    (r->iters ? ((double)r->elapsed_ns / (double)r->iters) : 0.0),
    (r->elapsed_ns ? (1e9 * (double)r->iters / (double)r->elapsed_ns) : 0.0),
    (unsigned long long)r->cycles,
    (unsigned long long)r->checksum
  );
}

static void print_json_summary(FILE* out, const bench_case* c, const opts* o,
                              uint64_t iters_eff, const stats* st_ns_per_iter,
                              uint64_t checksum_xor) {
  fprintf(out,
    "{"
      "\"type\":\"summary\","
      "\"suite\":\"%s\","
      "\"case\":\"%s\","
      "\"iters\":%llu,"
      "\"size\":%llu,"
      "\"repeats\":%u,"
      "\"warmup\":%u,"
      "\"min_time_ms\":%u,"
      "\"ns_per_iter\":{"
        "\"min\":%.6f,"
        "\"mean\":%.6f,"
        "\"p50\":%.6f,"
        "\"p90\":%.6f,"
        "\"max\":%.6f,"
        "\"stddev\":%.6f"
      "},"
      "\"checksum_xor\":%llu"
    "}\n",
    safe_str(c->suite),
    safe_str(c->name),
    (unsigned long long)iters_eff,
    (unsigned long long)o->size,
    (unsigned)o->repeats,
    (unsigned)o->warmup,
    (unsigned)o->min_time_ms,
    st_ns_per_iter->min,
    st_ns_per_iter->mean,
    st_ns_per_iter->p50,
    st_ns_per_iter->p90,
    st_ns_per_iter->max,
    st_ns_per_iter->stddev,
    (unsigned long long)checksum_xor
  );
}

static void print_text_summary(FILE* out, const bench_case* c, const opts* o,
                              uint64_t iters_eff, const stats* st_ns_per_iter,
                              uint64_t checksum_xor) {
  fprintf(out,
    "%s/%s  iters=%llu size=%zu  ns/iter: min=%.3f mean=%.3f p50=%.3f p90=%.3f max=%.3f sd=%.3f  checksum_xor=%llu\n",
    safe_str(c->suite),
    safe_str(c->name),
    (unsigned long long)iters_eff,
    o->size,
    st_ns_per_iter->min,
    st_ns_per_iter->mean,
    st_ns_per_iter->p50,
    st_ns_per_iter->p90,
    st_ns_per_iter->max,
    st_ns_per_iter->stddev,
    (unsigned long long)checksum_xor
  );
}

static void print_csv_header(FILE* out) {
  fprintf(out,
    "suite,case,iters,size,repeats,warmup,min_time_ms,"
    "ns_per_iter_min,ns_per_iter_mean,ns_per_iter_p50,ns_per_iter_p90,ns_per_iter_max,ns_per_iter_stddev,"
    "checksum_xor\n"
  );
}

static void print_csv_summary(FILE* out, const bench_case* c, const opts* o,
                             uint64_t iters_eff, const stats* st, uint64_t checksum_xor) {
  fprintf(out,
    "%s,%s,%llu,%llu,%u,%u,%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%llu\n",
    safe_str(c->suite),
    safe_str(c->name),
    (unsigned long long)iters_eff,
    (unsigned long long)o->size,
    (unsigned)o->repeats,
    (unsigned)o->warmup,
    (unsigned)o->min_time_ms,
    st->min, st->mean, st->p50, st->p90, st->max, st->stddev,
    (unsigned long long)checksum_xor
  );
}

static int run_all(FILE* out, const opts* o) {
  size_t n = 0;
  const bench_case* cases = bench_registry_list(&n);

  if (n == 0 || !cases) {
    fprintf(stderr, "No bench cases registered.\n");
    return 2;
  }

  if (o->csv) print_csv_header(out);

  int ran_any = 0;
  int rc = 0;

  for (size_t i = 0; i < n; ++i) {
    const bench_case* c = &cases[i];
    if (!case_selected(c, o)) continue;

    ran_any = 1;

    // iters effective (auto-scale)
    const uint64_t iters_eff = autoscale_iters(c, o);

    // Warmup (non compté)
    for (uint32_t w = 0; w < o->warmup; ++w) {
      const uint64_t warm_salt = UINT64_C(0x5741524D); // ASCII "WARM"
      bench_result r = run_one(c, iters_eff, o->size, o->seed ^ (warm_salt + w));
      (void)r;
      if (o->json) {
        print_json_run(out, c, &r, w, 1u, iters_eff);
      }
    }

    // Runs mesurés
    const uint32_t reps = (o->repeats ? o->repeats : 1);
    double* ns_per_iter = (double*)malloc((size_t)reps * sizeof(double));
    if (!ns_per_iter) {
      fprintf(stderr, "OOM allocating samples.\n");
      return 3;
    }

    uint64_t checksum_xor = 0;

    for (uint32_t rix = 0; rix < reps; ++rix) {
      bench_result r = run_one(c, iters_eff, o->size, o->seed ^ (UINT64_C(0xC0DE) + rix));
      checksum_xor ^= r.checksum;

      const double nspi = (r.iters ? ((double)r.elapsed_ns / (double)r.iters) : 0.0);
      ns_per_iter[rix] = nspi;

      if (o->json) {
        print_json_run(out, c, &r, rix, 0u, iters_eff);
      }
    }

    const stats st = compute_stats(ns_per_iter, reps);

    if (o->json) {
      print_json_summary(out, c, o, iters_eff, &st, checksum_xor);
    } else if (o->csv) {
      print_csv_summary(out, c, o, iters_eff, &st, checksum_xor);
    } else {
      print_text_summary(out, c, o, iters_eff, &st, checksum_xor);
    }

    free(ns_per_iter);
  }

  if (!ran_any) {
    fprintf(stderr, "No cases matched --suite '%s' --case '%s'\n", o->suite_pat, o->case_pat);
    rc = 4;
  }

  return rc;
}

// ============================================================================
// main
// ============================================================================

int main(int argc, char** argv) {
  opts o;
  if (!parse_args(argc, argv, &o)) {
    print_usage(stderr, argv[0]);
    return 2;
  }

  // Registry init (si non fourni, linker error => supprime l’appel ou fournis stub)
  bench_registry_init();

  FILE* out = open_out(o.out_path);
  if (!out) return 2;

  if (o.list) {
    list_cases(out);
    close_out(out);
    return 0;
  }

  const int rc = run_all(out, &o);
  close_out(out);
  return rc;
}
