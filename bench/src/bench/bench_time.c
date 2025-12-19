// bench_time.c — timing utilities for benchmarking (C17, max)

#include "bench_time.h"

#include <stddef.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <time.h>
  #include <unistd.h>
  #if defined(__APPLE__)
    #include <mach/mach_time.h>
  #endif
#endif

#if defined(_MSC_VER)
  #include <intrin.h>
#endif

// ----------------------------------------------------------------------------
// Arch detection
// ----------------------------------------------------------------------------

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  #define BENCH_TIME_X86 1
#else
  #define BENCH_TIME_X86 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
  #define BENCH_TIME_ARM64 1
#else
  #define BENCH_TIME_ARM64 0
#endif

#if defined(__arm__) || defined(_M_ARM)
  #define BENCH_TIME_ARM32 1
#else
  #define BENCH_TIME_ARM32 0
#endif

#if BENCH_TIME_X86 && !defined(_MSC_VER)
  #include <x86intrin.h>
#endif

// ----------------------------------------------------------------------------
// Internal helpers
// ----------------------------------------------------------------------------

static inline void bench_cpu_relax(void) {
#if BENCH_TIME_X86
  #if defined(_MSC_VER)
    _mm_pause();
  #else
    __asm__ __volatile__("pause" ::: "memory");
  #endif
#elif BENCH_TIME_ARM64 || BENCH_TIME_ARM32
  #if defined(_MSC_VER)
    __yield();
  #else
    __asm__ __volatile__("yield" ::: "memory");
  #endif
#else
  // no-op
#endif
}

static uint64_t bench_time_now_ns_fallback(void) {
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
  #if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
      return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
    }
  #endif

  #if defined(__APPLE__)
    static mach_timebase_info_data_t tb;
    static int init = 0;
    if (!init) {
      (void)mach_timebase_info(&tb);
      init = 1;
    }
    const uint64_t t = mach_absolute_time();
    return (t * (uint64_t)tb.numer) / (uint64_t)tb.denom;
  #endif

  return 0;
#endif
}

// ----------------------------------------------------------------------------
// Public: monotonic ns
// ----------------------------------------------------------------------------

uint64_t bench_time_now_ns(void) {
  return bench_time_now_ns_fallback();
}

// ----------------------------------------------------------------------------
// x86: TSC helpers + CPUID freq (best-effort)
// ----------------------------------------------------------------------------

#if BENCH_TIME_X86

static inline void bench_x86_lfence(void) {
#if defined(_MSC_VER)
  _mm_lfence();
#else
  __asm__ __volatile__("lfence" ::: "memory");
#endif
}

static inline uint64_t bench_x86_rdtsc(void) {
#if defined(_MSC_VER)
  return (uint64_t)__rdtsc();
#else
  return (uint64_t)__rdtsc();
#endif
}

static inline uint64_t bench_x86_rdtscp(uint32_t* aux) {
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

static inline void bench_x86_cpuid(uint32_t leaf, uint32_t subleaf,
                                   uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
#if defined(_MSC_VER)
  int regs[4];
  __cpuidex(regs, (int)leaf, (int)subleaf);
  if (a) *a = (uint32_t)regs[0];
  if (b) *b = (uint32_t)regs[1];
  if (c) *c = (uint32_t)regs[2];
  if (d) *d = (uint32_t)regs[3];
#else
  uint32_t ra, rb, rc, rd;
  __asm__ __volatile__("cpuid"
                       : "=a"(ra), "=b"(rb), "=c"(rc), "=d"(rd)
                       : "a"(leaf), "c"(subleaf)
                       : "memory");
  if (a) *a = ra;
  if (b) *b = rb;
  if (c) *c = rc;
  if (d) *d = rd;
#endif
}

// Try CPUID leaf 0x15 (TSC/crystal relationship) then 0x16 (base MHz).
static uint64_t bench_x86_tsc_freq_hz_cpuid(void) {
  uint32_t a=0,b=0,c=0,d=0;

  // Max leaf?
  bench_x86_cpuid(0, 0, &a, &b, &c, &d);
  const uint32_t max_leaf = a;

  if (max_leaf >= 0x15) {
    uint32_t denom=0, numer=0, crystal=0;
    bench_x86_cpuid(0x15, 0, &denom, &numer, &crystal, &d);

    // 0x15: EAX=denom, EBX=numer, ECX=crystal Hz (may be 0).
    if (denom != 0 && numer != 0 && crystal != 0) {
      // tsc_hz = crystal_hz * numer / denom
      const uint64_t hz = ((uint64_t)crystal * (uint64_t)numer) / (uint64_t)denom;
      if (hz) return hz;
    }
  }

  if (max_leaf >= 0x16) {
    // 0x16: EAX=base MHz (Intel doc; not universal)
    uint32_t base_mhz=0;
    bench_x86_cpuid(0x16, 0, &base_mhz, &b, &c, &d);
    if (base_mhz) return (uint64_t)base_mhz * UINT64_C(1000000);
  }

  return 0;
}

#endif // BENCH_TIME_X86

// ----------------------------------------------------------------------------
// AArch64: CNTVCT/CNTFRQ (GCC/Clang inline asm only; MSVC fallback)
// ----------------------------------------------------------------------------

#if BENCH_TIME_ARM64 && !defined(_MSC_VER)

static inline uint64_t bench_arm64_cntvct(void) {
  uint64_t v;
  __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(v));
  return v;
}

static inline uint64_t bench_arm64_cntfrq(void) {
  uint64_t v;
  __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(v));
  return v;
}

static inline void bench_arm64_isb(void) {
  __asm__ __volatile__("isb" ::: "memory");
}

#endif

// ----------------------------------------------------------------------------
// Public: cycles counters
// ----------------------------------------------------------------------------

uint64_t bench_time_cycles_now(void) {
#if BENCH_TIME_X86
  return bench_x86_rdtsc();
#elif BENCH_TIME_ARM64 && !defined(_MSC_VER)
  return bench_arm64_cntvct();
#else
  return bench_time_now_ns();
#endif
}

uint64_t bench_time_cycles_begin(void) {
#if BENCH_TIME_X86
  bench_x86_lfence();
  return bench_x86_rdtsc();
#elif BENCH_TIME_ARM64 && !defined(_MSC_VER)
  bench_arm64_isb();
  return bench_arm64_cntvct();
#else
  return bench_time_now_ns();
#endif
}

uint64_t bench_time_cycles_end(void) {
#if BENCH_TIME_X86
  uint32_t aux = 0;
  const uint64_t v = bench_x86_rdtscp(&aux);
  (void)aux;
  bench_x86_lfence();
  return v;
#elif BENCH_TIME_ARM64 && !defined(_MSC_VER)
  bench_arm64_isb();
  const uint64_t v = bench_arm64_cntvct();
  bench_arm64_isb();
  return v;
#else
  return bench_time_now_ns();
#endif
}

// ----------------------------------------------------------------------------
// Public: frequency (Hz)
// ----------------------------------------------------------------------------

static uint64_t bench_time_estimate_cycles_hz(uint32_t sample_ms) {
  if (sample_ms < 25) sample_ms = 25;
  if (sample_ms > 1000) sample_ms = 1000;

  const uint64_t t0 = bench_time_now_ns();
  const uint64_t c0 = bench_time_cycles_begin();

  // Sleep best-effort (can oversleep).
  bench_time_sleep_ms(sample_ms);

  const uint64_t c1 = bench_time_cycles_end();
  const uint64_t t1 = bench_time_now_ns();

  const uint64_t dt = (t1 >= t0) ? (t1 - t0) : 0;
  const uint64_t dc = (c1 >= c0) ? (c1 - c0) : 0;

  if (dt == 0 || dc == 0) return 0;
  return (dc * UINT64_C(1000000000)) / dt;
}

uint64_t bench_time_cycles_freq_hz(void) {
  static uint64_t cached = 0;
  if (cached) return cached;

#if BENCH_TIME_ARM64 && !defined(_MSC_VER)
  cached = bench_arm64_cntfrq();
  return cached;
#elif BENCH_TIME_X86
  cached = bench_x86_tsc_freq_hz_cpuid();
  if (cached) return cached;

  // fallback: estimate (works if TSC is invariant enough)
  cached = bench_time_estimate_cycles_hz(200);
  return cached;
#else
  cached = 0;
  return 0;
#endif
}

// ----------------------------------------------------------------------------
// Public: sleep / busy wait
// ----------------------------------------------------------------------------

void bench_time_sleep_ms(uint32_t ms) {
#if defined(_WIN32)
  Sleep((DWORD)ms);
#else
  struct timespec ts;
  ts.tv_sec  = (time_t)(ms / 1000u);
  ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
  (void)nanosleep(&ts, NULL);
#endif
}

void bench_time_busy_wait_ns(uint64_t ns) {
  const uint64_t start = bench_time_now_ns();
  if (start == 0) return;

  for (;;) {
    const uint64_t now = bench_time_now_ns();
    if (now == 0) return;
    if (now - start >= ns) break;
    bench_cpu_relax();
  }
}
