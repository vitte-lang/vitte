// bench_registry.c — benchmark case registry (C17, max)
//
// Responsibilities:
//   - Aggregate benchmark cases from multiple suites (alloc/micro/...)
//   - Expose a stable list to the runner (bench_main.c)
//   - Keep dependencies minimal and purely C17
//
// Design:
//   Providers expose:  const vitte_bm_case* <provider>_list_cases(size_t* out_count)
//   Each vitte_bm_case is { name, fn } where fn signature matches:
//       vitte_bm_result fn(uint64_t iters, size_t size, uint64_t seed)
//
//   The runner consumes bench_case { suite, name, fn } where fn returns bench_result.
//   Since vitte_bm_fn returns a different struct type, we use per-case trampolines:
//     - At init, store provider fn pointers into tables
//     - Expose compile-time wrapper functions that call fn_table[provider][index]
//
// Optional providers:
//   - On GCC/Clang, optional providers are declared weak and detected at runtime.
//   - On MSVC, weak symbols are not portable; enable providers via build defines.
//
// Exported API:
//   void bench_registry_init(void);
//   const bench_case* bench_registry_list(size_t* out_count);

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Types expected by bench_main.c
// -----------------------------------------------------------------------------

typedef struct bench_result {
  uint64_t elapsed_ns;
  uint64_t checksum;
  uint64_t iters;
  size_t   size;
  uint64_t cycles; // optional; runner may fill if 0
} bench_result;

typedef bench_result (*bench_fn)(uint64_t iters, size_t size, uint64_t seed);

typedef struct bench_case {
  const char* suite;
  const char* name;
  bench_fn     fn;
  uint32_t     flags;
} bench_case;

// -----------------------------------------------------------------------------
// Provider-side types (must match alloc.c / other suites)
// -----------------------------------------------------------------------------

typedef struct vitte_bm_result {
  uint64_t elapsed_ns;
  uint64_t checksum;
  uint64_t iters;
  size_t   size;
} vitte_bm_result;

typedef vitte_bm_result (*vitte_bm_fn)(uint64_t iters, size_t size, uint64_t seed);

typedef struct vitte_bm_case {
  const char* name;
  vitte_bm_fn fn;
} vitte_bm_case;

// -----------------------------------------------------------------------------
// Weak symbol helper for optional providers
// -----------------------------------------------------------------------------

#if defined(__GNUC__) || defined(__clang__)
  #define BENCH_WEAK __attribute__((weak))
#else
  #define BENCH_WEAK
#endif

// -----------------------------------------------------------------------------
// Provider declarations
// -----------------------------------------------------------------------------

// alloc is expected (you generated alloc.c already).
extern const vitte_bm_case* vitte_bm_alloc_list_cases(size_t* out_count);

// Optional providers (enable by linking objects + optionally defining build flags)
// Expected naming convention (adjust if your project uses different names):
//   vitte_bm_micro_list_cases
//   vitte_bm_macro_list_cases
//   vitte_bm_json_list_cases
//
// On GCC/Clang, these are weak so registry links even if not present.
// On MSVC, define VITTE_BENCH_ENABLE_* and provide the symbols.

#if defined(VITTE_BENCH_ENABLE_MICRO)
extern const vitte_bm_case* vitte_bm_micro_list_cases(size_t* out_count);
#else
extern const vitte_bm_case* vitte_bm_micro_list_cases(size_t* out_count) BENCH_WEAK;
#endif

#if defined(VITTE_BENCH_ENABLE_MACRO)
extern const vitte_bm_case* vitte_bm_macro_list_cases(size_t* out_count);
#else
extern const vitte_bm_case* vitte_bm_macro_list_cases(size_t* out_count) BENCH_WEAK;
#endif

#if defined(VITTE_BENCH_ENABLE_JSON)
extern const vitte_bm_case* vitte_bm_json_list_cases(size_t* out_count);
#else
extern const vitte_bm_case* vitte_bm_json_list_cases(size_t* out_count) BENCH_WEAK;
#endif

// -----------------------------------------------------------------------------
// Registry configuration
// -----------------------------------------------------------------------------

enum { BENCH_PROVIDER_MAX = 4 };
enum { BENCH_MAX_CASES_PER_PROVIDER = 32 };

typedef struct provider_slot {
  const char* suite;
  const vitte_bm_case* (*list_cases)(size_t* out_count);
  size_t count;
} provider_slot;

static provider_slot g_providers[BENCH_PROVIDER_MAX];

// fn tables loaded at init
static vitte_bm_fn   g_fn[BENCH_PROVIDER_MAX][BENCH_MAX_CASES_PER_PROVIDER];
static const char*   g_name[BENCH_PROVIDER_MAX][BENCH_MAX_CASES_PER_PROVIDER];

// final registry
static bench_case* g_cases = NULL;
static size_t      g_cases_count = 0;
static int         g_inited = 0;

// -----------------------------------------------------------------------------
// Trampoline generation
// -----------------------------------------------------------------------------

static inline bench_result bench_wrap_call(int prov, int idx, uint64_t iters, size_t size, uint64_t seed) {
  bench_result out;
  out.elapsed_ns = 0;
  out.checksum = UINT64_C(0);
  out.iters = iters;
  out.size = size;
  out.cycles = 0;

  if (prov < 0 || prov >= (int)BENCH_PROVIDER_MAX) {
    out.checksum = UINT64_C(0xBADC0DE);
    return out;
  }
  if (idx < 0 || idx >= (int)BENCH_MAX_CASES_PER_PROVIDER) {
    out.checksum = UINT64_C(0xBADC0DE) ^ (uint64_t)(unsigned)idx;
    return out;
  }

  vitte_bm_fn fn = g_fn[prov][idx];
  if (!fn) {
    out.checksum = UINT64_C(0xDEAD) ^ (uint64_t)((unsigned)prov << 8) ^ (uint64_t)(unsigned)idx;
    return out;
  }

  vitte_bm_result r = fn(iters, size, seed);
  out.elapsed_ns = r.elapsed_ns;
  out.checksum = r.checksum;
  out.iters = r.iters ? r.iters : iters;
  out.size = r.size;
  out.cycles = 0;
  return out;
}

#define DECL_WRAP(P, I) \
  static bench_result bench_wrap_p##P##_##I(uint64_t iters, size_t size, uint64_t seed) { \
    return bench_wrap_call((P), (I), iters, size, seed); \
  }

// Generate wrappers: 4 providers * 32 indices
// clang-format off
DECL_WRAP(0,0)  DECL_WRAP(0,1)  DECL_WRAP(0,2)  DECL_WRAP(0,3)
DECL_WRAP(0,4)  DECL_WRAP(0,5)  DECL_WRAP(0,6)  DECL_WRAP(0,7)
DECL_WRAP(0,8)  DECL_WRAP(0,9)  DECL_WRAP(0,10) DECL_WRAP(0,11)
DECL_WRAP(0,12) DECL_WRAP(0,13) DECL_WRAP(0,14) DECL_WRAP(0,15)
DECL_WRAP(0,16) DECL_WRAP(0,17) DECL_WRAP(0,18) DECL_WRAP(0,19)
DECL_WRAP(0,20) DECL_WRAP(0,21) DECL_WRAP(0,22) DECL_WRAP(0,23)
DECL_WRAP(0,24) DECL_WRAP(0,25) DECL_WRAP(0,26) DECL_WRAP(0,27)
DECL_WRAP(0,28) DECL_WRAP(0,29) DECL_WRAP(0,30) DECL_WRAP(0,31)

DECL_WRAP(1,0)  DECL_WRAP(1,1)  DECL_WRAP(1,2)  DECL_WRAP(1,3)
DECL_WRAP(1,4)  DECL_WRAP(1,5)  DECL_WRAP(1,6)  DECL_WRAP(1,7)
DECL_WRAP(1,8)  DECL_WRAP(1,9)  DECL_WRAP(1,10) DECL_WRAP(1,11)
DECL_WRAP(1,12) DECL_WRAP(1,13) DECL_WRAP(1,14) DECL_WRAP(1,15)
DECL_WRAP(1,16) DECL_WRAP(1,17) DECL_WRAP(1,18) DECL_WRAP(1,19)
DECL_WRAP(1,20) DECL_WRAP(1,21) DECL_WRAP(1,22) DECL_WRAP(1,23)
DECL_WRAP(1,24) DECL_WRAP(1,25) DECL_WRAP(1,26) DECL_WRAP(1,27)
DECL_WRAP(1,28) DECL_WRAP(1,29) DECL_WRAP(1,30) DECL_WRAP(1,31)

DECL_WRAP(2,0)  DECL_WRAP(2,1)  DECL_WRAP(2,2)  DECL_WRAP(2,3)
DECL_WRAP(2,4)  DECL_WRAP(2,5)  DECL_WRAP(2,6)  DECL_WRAP(2,7)
DECL_WRAP(2,8)  DECL_WRAP(2,9)  DECL_WRAP(2,10) DECL_WRAP(2,11)
DECL_WRAP(2,12) DECL_WRAP(2,13) DECL_WRAP(2,14) DECL_WRAP(2,15)
DECL_WRAP(2,16) DECL_WRAP(2,17) DECL_WRAP(2,18) DECL_WRAP(2,19)
DECL_WRAP(2,20) DECL_WRAP(2,21) DECL_WRAP(2,22) DECL_WRAP(2,23)
DECL_WRAP(2,24) DECL_WRAP(2,25) DECL_WRAP(2,26) DECL_WRAP(2,27)
DECL_WRAP(2,28) DECL_WRAP(2,29) DECL_WRAP(2,30) DECL_WRAP(2,31)

DECL_WRAP(3,0)  DECL_WRAP(3,1)  DECL_WRAP(3,2)  DECL_WRAP(3,3)
DECL_WRAP(3,4)  DECL_WRAP(3,5)  DECL_WRAP(3,6)  DECL_WRAP(3,7)
DECL_WRAP(3,8)  DECL_WRAP(3,9)  DECL_WRAP(3,10) DECL_WRAP(3,11)
DECL_WRAP(3,12) DECL_WRAP(3,13) DECL_WRAP(3,14) DECL_WRAP(3,15)
DECL_WRAP(3,16) DECL_WRAP(3,17) DECL_WRAP(3,18) DECL_WRAP(3,19)
DECL_WRAP(3,20) DECL_WRAP(3,21) DECL_WRAP(3,22) DECL_WRAP(3,23)
DECL_WRAP(3,24) DECL_WRAP(3,25) DECL_WRAP(3,26) DECL_WRAP(3,27)
DECL_WRAP(3,28) DECL_WRAP(3,29) DECL_WRAP(3,30) DECL_WRAP(3,31)
// clang-format on

#undef DECL_WRAP

static const bench_fn g_wrap[BENCH_PROVIDER_MAX][BENCH_MAX_CASES_PER_PROVIDER] = {
  {
    bench_wrap_p0_0,  bench_wrap_p0_1,  bench_wrap_p0_2,  bench_wrap_p0_3,
    bench_wrap_p0_4,  bench_wrap_p0_5,  bench_wrap_p0_6,  bench_wrap_p0_7,
    bench_wrap_p0_8,  bench_wrap_p0_9,  bench_wrap_p0_10, bench_wrap_p0_11,
    bench_wrap_p0_12, bench_wrap_p0_13, bench_wrap_p0_14, bench_wrap_p0_15,
    bench_wrap_p0_16, bench_wrap_p0_17, bench_wrap_p0_18, bench_wrap_p0_19,
    bench_wrap_p0_20, bench_wrap_p0_21, bench_wrap_p0_22, bench_wrap_p0_23,
    bench_wrap_p0_24, bench_wrap_p0_25, bench_wrap_p0_26, bench_wrap_p0_27,
    bench_wrap_p0_28, bench_wrap_p0_29, bench_wrap_p0_30, bench_wrap_p0_31,
  },
  {
    bench_wrap_p1_0,  bench_wrap_p1_1,  bench_wrap_p1_2,  bench_wrap_p1_3,
    bench_wrap_p1_4,  bench_wrap_p1_5,  bench_wrap_p1_6,  bench_wrap_p1_7,
    bench_wrap_p1_8,  bench_wrap_p1_9,  bench_wrap_p1_10, bench_wrap_p1_11,
    bench_wrap_p1_12, bench_wrap_p1_13, bench_wrap_p1_14, bench_wrap_p1_15,
    bench_wrap_p1_16, bench_wrap_p1_17, bench_wrap_p1_18, bench_wrap_p1_19,
    bench_wrap_p1_20, bench_wrap_p1_21, bench_wrap_p1_22, bench_wrap_p1_23,
    bench_wrap_p1_24, bench_wrap_p1_25, bench_wrap_p1_26, bench_wrap_p1_27,
    bench_wrap_p1_28, bench_wrap_p1_29, bench_wrap_p1_30, bench_wrap_p1_31,
  },
  {
    bench_wrap_p2_0,  bench_wrap_p2_1,  bench_wrap_p2_2,  bench_wrap_p2_3,
    bench_wrap_p2_4,  bench_wrap_p2_5,  bench_wrap_p2_6,  bench_wrap_p2_7,
    bench_wrap_p2_8,  bench_wrap_p2_9,  bench_wrap_p2_10, bench_wrap_p2_11,
    bench_wrap_p2_12, bench_wrap_p2_13, bench_wrap_p2_14, bench_wrap_p2_15,
    bench_wrap_p2_16, bench_wrap_p2_17, bench_wrap_p2_18, bench_wrap_p2_19,
    bench_wrap_p2_20, bench_wrap_p2_21, bench_wrap_p2_22, bench_wrap_p2_23,
    bench_wrap_p2_24, bench_wrap_p2_25, bench_wrap_p2_26, bench_wrap_p2_27,
    bench_wrap_p2_28, bench_wrap_p2_29, bench_wrap_p2_30, bench_wrap_p2_31,
  },
  {
    bench_wrap_p3_0,  bench_wrap_p3_1,  bench_wrap_p3_2,  bench_wrap_p3_3,
    bench_wrap_p3_4,  bench_wrap_p3_5,  bench_wrap_p3_6,  bench_wrap_p3_7,
    bench_wrap_p3_8,  bench_wrap_p3_9,  bench_wrap_p3_10, bench_wrap_p3_11,
    bench_wrap_p3_12, bench_wrap_p3_13, bench_wrap_p3_14, bench_wrap_p3_15,
    bench_wrap_p3_16, bench_wrap_p3_17, bench_wrap_p3_18, bench_wrap_p3_19,
    bench_wrap_p3_20, bench_wrap_p3_21, bench_wrap_p3_22, bench_wrap_p3_23,
    bench_wrap_p3_24, bench_wrap_p3_25, bench_wrap_p3_26, bench_wrap_p3_27,
    bench_wrap_p3_28, bench_wrap_p3_29, bench_wrap_p3_30, bench_wrap_p3_31,
  },
};

// -----------------------------------------------------------------------------
// Build registry
// -----------------------------------------------------------------------------

static void provider_load(int slot) {
  provider_slot* p = &g_providers[slot];
  p->count = 0;

  if (!p->suite || !p->list_cases) return;

  size_t n = 0;
  const vitte_bm_case* cases = p->list_cases(&n);
  if (!cases || n == 0) return;

  if (n > BENCH_MAX_CASES_PER_PROVIDER) n = BENCH_MAX_CASES_PER_PROVIDER;

  for (size_t i = 0; i < n; ++i) {
    g_name[slot][i] = cases[i].name;
    g_fn[slot][i]   = cases[i].fn;
  }
  p->count = n;
}

static void registry_free(void) {
  free(g_cases);
  g_cases = NULL;
  g_cases_count = 0;
}

static void registry_build(void) {
  registry_free();

  // Load all providers.
  for (int s = 0; s < (int)BENCH_PROVIDER_MAX; ++s) {
    provider_load(s);
  }

  // Count total cases.
  size_t total = 0;
  for (int s = 0; s < (int)BENCH_PROVIDER_MAX; ++s) {
    total += g_providers[s].count;
  }

  if (total == 0) {
    g_cases = NULL;
    g_cases_count = 0;
    return;
  }

  g_cases = (bench_case*)calloc(total, sizeof(bench_case));
  if (!g_cases) {
    g_cases_count = 0;
    return;
  }

  size_t k = 0;
  for (int s = 0; s < (int)BENCH_PROVIDER_MAX; ++s) {
    const provider_slot* p = &g_providers[s];
    for (size_t i = 0; i < p->count; ++i) {
      bench_case* bc = &g_cases[k++];
      bc->suite = p->suite;
      bc->name  = g_name[s][i] ? g_name[s][i] : "";
      bc->fn    = g_wrap[s][i];
      bc->flags = 0;
    }
  }

  g_cases_count = k;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void bench_registry_init(void) {
  if (g_inited) return;
  g_inited = 1;

  // Slot 0: alloc (required)
  g_providers[0].suite = "alloc";
  g_providers[0].list_cases = vitte_bm_alloc_list_cases;

  // Slot 1: micro (optional)
  if (vitte_bm_micro_list_cases) {
    g_providers[1].suite = "micro";
    g_providers[1].list_cases = vitte_bm_micro_list_cases;
  }

  // Slot 2: macro (optional)
  if (vitte_bm_macro_list_cases) {
    g_providers[2].suite = "macro";
    g_providers[2].list_cases = vitte_bm_macro_list_cases;
  }

  // Slot 3: json (optional)
  if (vitte_bm_json_list_cases) {
    g_providers[3].suite = "json";
    g_providers[3].list_cases = vitte_bm_json_list_cases;
  }

  registry_build();
}

const bench_case* bench_registry_list(size_t* out_count) {
  if (!g_inited) bench_registry_init();
  if (out_count) *out_count = g_cases_count;
  return g_cases;
}
