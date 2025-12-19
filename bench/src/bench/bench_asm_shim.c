// bench_asm_shim.c — portable asm/intrinsics shim (C17)
//
// Rôle : fournir une couche “asm” minimale pour le framework de bench :
//   - compteur de cycles (TSC x86, CNTVCT aarch64 si dispo, sinon fallback ns)
//   - begin/end sérialisés pour mesures stables
//   - cpu_relax (pause/yield)
//   - compiler barrier
//   - clflush + prefetch (best-effort)
//
// Exporte (à déclarer dans ton bench.h si besoin) :
//   uint64_t bench_asm_time_now_ns(void);
//   uint64_t bench_asm_cycles_now(void);
//   uint64_t bench_asm_cycles_freq_hz(void);
//   uint64_t bench_asm_cycles_begin(void);
//   uint64_t bench_asm_cycles_end(void);
//   void     bench_asm_cpu_relax(void);
//   void     bench_asm_compiler_barrier(void);
//   void     bench_asm_clflush(const void* p);
//   void     bench_asm_prefetch_t0(const void* p);

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

#if defined(_MSC_VER)
  #include <intrin.h>
#endif

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <time.h>
#endif

// ----------------------------------------------------------------------------
// Arch detection
// ----------------------------------------------------------------------------

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  #define BENCH_ASM_X86 1
#else
  #define BENCH_ASM_X86 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
  #define BENCH_ASM_ARM64 1
#else
  #define BENCH_ASM_ARM64 0
#endif

#if defined(__arm__) || defined(_M_ARM)
  #define BENCH_ASM_ARM32 1
#else
  #define BENCH_ASM_ARM32 0
#endif

#if BENCH_ASM_X86 && !defined(_MSC_VER)
  #include <x86intrin.h>
#endif

// ----------------------------------------------------------------------------
// Time fallback (ns)
// ----------------------------------------------------------------------------

uint64_t bench_asm_time_now_ns(void) {
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

  const uint64_t sec = ticks / f;
  const uint64_t rem = ticks - sec * f;

  return sec * UINT64_C(1000000000) + (rem * UINT64_C(1000000000) / f);
#else
  struct timespec ts;
  (void)clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
#endif
}

// ----------------------------------------------------------------------------
// x86 path (TSC)
// ----------------------------------------------------------------------------

#if BENCH_ASM_X86

static inline void bench_asm_x86_lfence(void) {
#if defined(_MSC_VER)
  _mm_lfence();
#else
  __asm__ __volatile__("lfence" ::: "memory");
#endif
}

static inline uint64_t bench_asm_x86_rdtsc(void) {
#if defined(_MSC_VER)
  return (uint64_t)__rdtsc();
#else
  return (uint64_t)__rdtsc();
#endif
}

static inline uint64_t bench_asm_x86_rdtscp(uint32_t* aux) {
#if defined(_MSC_VER)
  unsigned int x = 0;
  uint64_t v = (uint64_t)__rdtscp(&x);
  if (aux) *aux = (uint32_t)x;
  return v;
#else
  unsigned int x = 0;
  uint64_t v = (uint64_t)__rdtscp(&x);
  if (aux) *aux = (uint32_t)x;
  return v;
#endif
}

#endif // BENCH_ASM_X86

// ----------------------------------------------------------------------------
// aarch64 path (CNTVCT) — GCC/Clang only (MSVC: fallback ns)
// ----------------------------------------------------------------------------

#if BENCH_ASM_ARM64 && !defined(_MSC_VER)
static inline uint64_t bench_asm_arm64_cntvct(void) {
  uint64_t v;
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(v));
  return v;
}
static inline uint64_t bench_asm_arm64_cntfrq(void) {
  uint64_t v;
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(v));
  return v;
}
static inline void bench_asm_arm64_isb(void) {
  __asm__ __volatile__("isb" ::: "memory");
}
#endif

// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

uint64_t bench_asm_cycles_now(void) {
#if BENCH_ASM_X86
  return bench_asm_x86_rdtsc();
#elif BENCH_ASM_ARM64 && !defined(_MSC_VER)
  return bench_asm_arm64_cntvct();
#else
  return bench_asm_time_now_ns();
#endif
}

uint64_t bench_asm_cycles_freq_hz(void) {
#if BENCH_ASM_ARM64 && !defined(_MSC_VER)
  return bench_asm_arm64_cntfrq();
#else
  // x86 TSC freq discovery = spécifique OS/plateforme (on laisse 0 par défaut)
  return 0;
#endif
}

uint64_t bench_asm_cycles_begin(void) {
#if BENCH_ASM_X86
  bench_asm_x86_lfence();
  return bench_asm_x86_rdtsc();
#elif BENCH_ASM_ARM64 && !defined(_MSC_VER)
  bench_asm_arm64_isb();
  return bench_asm_arm64_cntvct();
#else
  return bench_asm_time_now_ns();
#endif
}

uint64_t bench_asm_cycles_end(void) {
#if BENCH_ASM_X86
  uint32_t aux = 0;
  uint64_t v = bench_asm_x86_rdtscp(&aux);
  (void)aux;
  bench_asm_x86_lfence();
  return v;
#elif BENCH_ASM_ARM64 && !defined(_MSC_VER)
  bench_asm_arm64_isb();
  uint64_t v = bench_asm_arm64_cntvct();
  bench_asm_arm64_isb();
  return v;
#else
  return bench_asm_time_now_ns();
#endif
}

void bench_asm_cpu_relax(void) {
#if BENCH_ASM_X86
  _mm_pause();
#elif BENCH_ASM_ARM64 || BENCH_ASM_ARM32
  #if defined(_MSC_VER)
    __yield();
  #else
    __asm__ __volatile__("yield" ::: "memory");
  #endif
#else
  atomic_signal_fence(memory_order_seq_cst);
#endif
}

void bench_asm_compiler_barrier(void) {
  atomic_signal_fence(memory_order_seq_cst);
}

void bench_asm_clflush(const void* p) {
  if (!p) return;
#if BENCH_ASM_X86
  #if defined(_MSC_VER)
    _mm_clflush(p);
  #else
    __asm__ __volatile__("clflush (%0)" :: "r"(p) : "memory");
  #endif
  bench_asm_compiler_barrier();
#else
  (void)p;
  bench_asm_compiler_barrier();
#endif
}

void bench_asm_prefetch_t0(const void* p) {
  if (!p) return;
#if BENCH_ASM_X86
  _mm_prefetch((const char*)p, _MM_HINT_T0);
#elif BENCH_ASM_ARM64 && !defined(_MSC_VER)
  __asm__ __volatile__("prfm pldl1keep, [%0]" :: "r"(p));
#else
  (void)p;
#endif
}
