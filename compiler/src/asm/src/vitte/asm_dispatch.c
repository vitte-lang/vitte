// vitte/src/asm/src/vitte/asm_dispatch.c
//
// ASM fastpaths dispatch (runtime selection)
//
// Contract:
//   - Exposes stable facade entry points for runtime/stdlib:
//       vitte_memcpy, vitte_memset, vitte_utf8_validate, vitte_fnv1a64
//   - Selects best implementation based on arch + CPU caps.
//   - Safe defaults: baseline versions (or stub for utf8).
//
// Notes:
//   - This file intentionally avoids any heavy dependencies.
//   - CPU detection is “best-effort”: if not available, stays on baseline.
//   - Thread-safety: minimal once-init via atomic CAS; re-entrant safe.
//
// Build:
//   - Compile this TU once into your asm/static runtime lib.
//   - Ensure asm symbols are linked in (aarch64 and/or x86_64 objects).
//

#include <stddef.h>
#include <stdint.h>

#if defined(VITTE_RUNTIME_ENABLE_STDIO)
#  include <stdio.h>
#endif

#if defined(_MSC_VER)
#  include <intrin.h>
#endif

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>
#elif !defined(__STDC_NO_THREADS__)
#  include <time.h>
#  include <sched.h>
#endif

#if !defined(__STDC_NO_ATOMICS__)
#  include <stdatomic.h>
#endif

#include "vitte/runtime.h"
#include "vitte/asm_fastpaths.h"

// Note: avoid referencing non-native arch symbols from this TU; that keeps the
// link clean without needing "other arch" stubs.

// -----------------------------------------------------------------------------
// Public facade (stable API)
// -----------------------------------------------------------------------------

typedef void* (*vitte_memcpy_fn)(void*, const void*, size_t);
typedef void* (*vitte_memset_fn)(void*, int, size_t);
typedef int   (*vitte_utf8_validate_fn)(const uint8_t*, size_t);

typedef struct vitte_asm_fast_t {
  vitte_memcpy_fn memcpy_fn;
  vitte_memset_fn memset_fn;
  vitte_utf8_validate_fn utf8_validate_fn;
} vitte_asm_fast_t;

// Global dispatch table
static vitte_asm_fast_t g_fast = {
  /* memcpy_fn */ (vitte_memcpy_fn)0,
  /* memset_fn */ (vitte_memset_fn)0,
  /* utf8_validate_fn */ (vitte_utf8_validate_fn)0,
};

#if !defined(__STDC_NO_ATOMICS__)
// 0 = uninitialized, 1 = initializing, 2 = ready
static atomic_int g_state = 0;
#else
static volatile int g_state = 0;
#endif

// -----------------------------------------------------------------------------
// Spin helpers
// -----------------------------------------------------------------------------

static inline void vitte_spin_pause(void) {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
  _mm_pause();
#elif (defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))
  __builtin_ia32_pause();
#else
  // best-effort: no-op
#endif
}

static inline void vitte_thread_yield(void) {
#if defined(_WIN32)
  (void)SwitchToThread();
#elif !defined(__STDC_NO_THREADS__)
  (void)sched_yield();
#else
  // best-effort: no-op
#endif
}

static inline void vitte_thread_sleep_1ms(void) {
#if defined(_WIN32)
  Sleep(1);
#elif !defined(__STDC_NO_THREADS__)
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 1000000L;
  (void)nanosleep(&ts, NULL);
#else
  // best-effort: no-op
#endif
}

static inline void vitte_spin_wait_backoff(uint32_t* iters) {
  uint32_t n = iters ? *iters : 0u;

  if (n < 64u) {
    vitte_spin_pause();
  } else if (n < 256u) {
    vitte_thread_yield();
  } else {
    // After a while, yield frequently and occasionally sleep a bit.
    vitte_thread_yield();
    if ((n & 63u) == 0u)
      vitte_thread_sleep_1ms();
  }

  if (iters)
    *iters = n + 1u;
}

// -----------------------------------------------------------------------------
// Dispatcher init (idempotent)
// -----------------------------------------------------------------------------

static void vitte_select_fastpaths_from_cpu(const vitte_cpu_desc_t* cpu) {
  g_fast.memcpy_fn = vitte_memcpy_baseline;
  g_fast.memset_fn = vitte_memset_baseline;
  g_fast.utf8_validate_fn = vitte_utf8_validate_stub;

  if (!cpu)
    return;

#if defined(__aarch64__) || defined(_M_ARM64)
  if (cpu->arch == VITTE_CPU_ARCH_AARCH64) {
    if (cpu->features_lo & VITTE_CPU_FEAT_A64_NEON) {
      g_fast.memcpy_fn = vitte_memcpy_neon;
      g_fast.memset_fn = vitte_memset_neon;
    }
    return;
  }
#elif defined(__x86_64__) || defined(_M_X64)
  if (cpu->arch == VITTE_CPU_ARCH_X86_64) {
    g_fast.memcpy_fn = vitte_memcpy_sse2;
    g_fast.memset_fn = vitte_memset_sse2;
    if (cpu->features_lo & VITTE_CPU_FEAT_X86_AVX2) {
      g_fast.memcpy_fn = vitte_memcpy_avx2;
      g_fast.memset_fn = vitte_memset_avx2;
    }
    return;
  }
#endif
}

static void vitte_asm_dispatch_init_impl(void) {
  vitte_cpu_desc_t cpu;
  uint8_t ok = vitte_cpu_detect_host(&cpu);
  vitte_select_fastpaths_from_cpu(ok ? &cpu : NULL);

  // Hard fail-safes: if any pointer is NULL, point to safest symbols.
  if (!g_fast.memcpy_fn) g_fast.memcpy_fn = vitte_memcpy_baseline;
  if (!g_fast.memset_fn) g_fast.memset_fn = vitte_memset_baseline;
  if (!g_fast.utf8_validate_fn) g_fast.utf8_validate_fn = vitte_utf8_validate_stub;
}

static inline void vitte_asm_dispatch_init_once(void) {
#if !defined(__STDC_NO_ATOMICS__)
  int s = atomic_load_explicit(&g_state, memory_order_acquire);
  if (s == 2)
    return;

  int expected = 0;
  if (atomic_compare_exchange_strong_explicit(&g_state, &expected, 1, memory_order_acq_rel,
                                              memory_order_acquire)) {
    vitte_asm_dispatch_init_impl();
    atomic_store_explicit(&g_state, 2, memory_order_release);
    return;
  }

  uint32_t wait_iters = 0;
  while (atomic_load_explicit(&g_state, memory_order_acquire) != 2) {
    vitte_spin_wait_backoff(&wait_iters);
  }
#else
  // Best-effort non-atomic fallback (single-thread init recommended)
  if (g_state == 2)
    return;
  if (g_state == 0) {
    g_state = 1;
    vitte_asm_dispatch_init_impl();
    g_state = 2;
    return;
  }
  uint32_t wait_iters = 0;
  while (g_state != 2) {
    vitte_spin_wait_backoff(&wait_iters);
  }
#endif
}

// -----------------------------------------------------------------------------
// Public façade functions
// -----------------------------------------------------------------------------

void* vitte_memcpy(void* dst, const void* src, size_t n) {
  vitte_asm_dispatch_init_once();
  return g_fast.memcpy_fn(dst, src, n);
}

void* vitte_memset(void* dst, int c, size_t n) {
  vitte_asm_dispatch_init_once();
  return g_fast.memset_fn(dst, c, n);
}

int vitte_utf8_validate(const uint8_t* data, size_t len) {
  vitte_asm_dispatch_init_once();
  return g_fast.utf8_validate_fn(data, len);
}

// FNV is already a single stable symbol (arch-specific impl linked)
uint64_t vitte_hash_fnv1a64(const uint8_t* data, size_t len) {
  return vitte_fnv1a64(data, len);
}

// Optional: expose current selected impls for debugging/telemetry
const void* vitte_asm_selected_memcpy(void) {
  vitte_asm_dispatch_init_once();
  return (const void*)(uintptr_t)g_fast.memcpy_fn;
}

const void* vitte_asm_selected_memset(void) {
  vitte_asm_dispatch_init_once();
  return (const void*)(uintptr_t)g_fast.memset_fn;
}

const void* vitte_asm_selected_utf8_validate(void) {
  vitte_asm_dispatch_init_once();
  return (const void*)(uintptr_t)g_fast.utf8_validate_fn;
}

vitte_asm_selected_caps_t vitte_asm_selected_caps(void) {
  vitte_asm_selected_caps_t out;
  out.abi = 1u;
  out.arch = VITTE_CPU_ARCH_UNKNOWN;
  out.features_lo = 0u;
  out.features_hi = 0u;

  vitte_cpu_desc_t cpu;
  uint8_t ok = vitte_cpu_detect_host(&cpu);
  if (!ok)
    return out;

  out.arch = cpu.arch;
  out.features_lo = cpu.features_lo;
  out.features_hi = cpu.features_hi;
  return out;
}

static vitte_asm_impl_id_t vitte_impl_id_for_memcpy(vitte_memcpy_fn fn) {
  if (!fn) return VITTE_ASM_IMPL_UNKNOWN;
  if (fn == vitte_memcpy_baseline) return VITTE_ASM_IMPL_BASELINE;
#if defined(__x86_64__) || defined(_M_X64)
  if (fn == vitte_memcpy_sse2) return VITTE_ASM_IMPL_SSE2;
  if (fn == vitte_memcpy_avx2) return VITTE_ASM_IMPL_AVX2;
#elif defined(__aarch64__) || defined(_M_ARM64)
  if (fn == vitte_memcpy_neon) return VITTE_ASM_IMPL_NEON;
#endif
  return VITTE_ASM_IMPL_UNKNOWN;
}

static vitte_asm_impl_id_t vitte_impl_id_for_memset(vitte_memset_fn fn) {
  if (!fn) return VITTE_ASM_IMPL_UNKNOWN;
  if (fn == vitte_memset_baseline) return VITTE_ASM_IMPL_BASELINE;
#if defined(__x86_64__) || defined(_M_X64)
  if (fn == vitte_memset_sse2) return VITTE_ASM_IMPL_SSE2;
  if (fn == vitte_memset_avx2) return VITTE_ASM_IMPL_AVX2;
#elif defined(__aarch64__) || defined(_M_ARM64)
  if (fn == vitte_memset_neon) return VITTE_ASM_IMPL_NEON;
#endif
  return VITTE_ASM_IMPL_UNKNOWN;
}

static vitte_asm_impl_id_t vitte_impl_id_for_utf8(vitte_utf8_validate_fn fn) {
  if (!fn) return VITTE_ASM_IMPL_UNKNOWN;
  if (fn == vitte_utf8_validate_stub) return VITTE_ASM_IMPL_BASELINE;
  return VITTE_ASM_IMPL_UNKNOWN;
}

vitte_asm_selected_info_t vitte_asm_selected_info(void) {
  vitte_asm_selected_info_t out;
  out.abi = 1u;
  out.arch = VITTE_CPU_ARCH_UNKNOWN;
  out.features_lo = 0u;
  out.features_hi = 0u;
  out.flags = 0u;
  out.memcpy_impl = VITTE_ASM_IMPL_UNKNOWN;
  out.memset_impl = VITTE_ASM_IMPL_UNKNOWN;
  out.utf8_impl = VITTE_ASM_IMPL_UNKNOWN;

  vitte_asm_dispatch_init_once();

  out.memcpy_impl = vitte_impl_id_for_memcpy(g_fast.memcpy_fn);
  out.memset_impl = vitte_impl_id_for_memset(g_fast.memset_fn);
  out.utf8_impl = vitte_impl_id_for_utf8(g_fast.utf8_validate_fn);

  if (out.memcpy_impl != VITTE_ASM_IMPL_BASELINE && out.memcpy_impl != VITTE_ASM_IMPL_UNKNOWN)
    out.flags |= VITTE_ASM_SELECTED_F_MEMCPY_ASM;
  if (out.memset_impl != VITTE_ASM_IMPL_BASELINE && out.memset_impl != VITTE_ASM_IMPL_UNKNOWN)
    out.flags |= VITTE_ASM_SELECTED_F_MEMSET_ASM;
  if (out.utf8_impl != VITTE_ASM_IMPL_BASELINE && out.utf8_impl != VITTE_ASM_IMPL_UNKNOWN)
    out.flags |= VITTE_ASM_SELECTED_F_UTF8_ASM;

  vitte_cpu_desc_t cpu;
  uint8_t ok = vitte_cpu_detect_host(&cpu);
  if (!ok)
    return out;

  out.arch = cpu.arch;
  out.features_lo = cpu.features_lo;
  out.features_hi = cpu.features_hi;
  return out;
}

static const char* vitte_impl_id_str(vitte_asm_impl_id_t id) {
  switch (id) {
    case VITTE_ASM_IMPL_BASELINE: return "baseline";
    case VITTE_ASM_IMPL_SSE2: return "sse2";
    case VITTE_ASM_IMPL_AVX2: return "avx2";
    case VITTE_ASM_IMPL_NEON: return "neon";
    default: return "unknown";
  }
}

static size_t vitte__buf_append(char* dst, size_t cap, size_t pos, const char* s) {
  if (!s) s = "";
  for (; *s; s++) {
    if (dst && cap && pos + 1 < cap) dst[pos] = *s;
    pos++;
  }
  return pos;
}

static size_t vitte__buf_append_u32_hex8(char* dst, size_t cap, size_t pos, uint32_t v) {
  static const char* hex = "0123456789abcdef";
  for (int shift = 28; shift >= 0; shift -= 4) {
    char c = hex[(v >> (uint32_t)shift) & 0xFu];
    if (dst && cap && pos + 1 < cap) dst[pos] = c;
    pos++;
  }
  return pos;
}

size_t vitte_asm_dump_selected_to_string(char* dst, size_t cap) {
  size_t pos = 0;

  vitte_asm_selected_info_t sel = vitte_asm_selected_info();

  pos = vitte__buf_append(dst, cap, pos, "vitte asm selected:\n");
  pos = vitte__buf_append(dst, cap, pos, "  arch=");
  pos = vitte__buf_append(dst, cap, pos, vitte_cpu_arch_str(sel.arch));
  pos = vitte__buf_append(dst, cap, pos, " features_lo=0x");
  pos = vitte__buf_append_u32_hex8(dst, cap, pos, (uint32_t)sel.features_lo);
  pos = vitte__buf_append(dst, cap, pos, " features_hi=0x");
  pos = vitte__buf_append_u32_hex8(dst, cap, pos, (uint32_t)sel.features_hi);
  pos = vitte__buf_append(dst, cap, pos, "\n");

  pos = vitte__buf_append(dst, cap, pos, "  memcpy=");
  pos = vitte__buf_append(dst, cap, pos, vitte_impl_id_str(sel.memcpy_impl));
  pos = vitte__buf_append(dst, cap, pos, " memset=");
  pos = vitte__buf_append(dst, cap, pos, vitte_impl_id_str(sel.memset_impl));
  pos = vitte__buf_append(dst, cap, pos, " utf8=");
  pos = vitte__buf_append(dst, cap, pos, vitte_impl_id_str(sel.utf8_impl));
  pos = vitte__buf_append(dst, cap, pos, " flags=0x");
  pos = vitte__buf_append_u32_hex8(dst, cap, pos, (uint32_t)sel.flags);
  pos = vitte__buf_append(dst, cap, pos, "\n");

  if (dst && cap) {
    dst[(pos < cap) ? pos : (cap - 1)] = '\0';
  }
  return pos;
}

#if defined(VITTE_RUNTIME_ENABLE_STDIO)
int vitte_asm_dump_selected(FILE* file) {
  if (!file) return 0;

  vitte_asm_selected_info_t sel = vitte_asm_selected_info();
  (void)fprintf(file, "vitte asm selected:\n");
  (void)fprintf(file, "  arch=%s features_lo=0x%08x features_hi=0x%08x\n",
                vitte_cpu_arch_str(sel.arch), (unsigned)sel.features_lo, (unsigned)sel.features_hi);
  (void)fprintf(file, "  memcpy=%s memset=%s utf8=%s flags=0x%08x\n",
                vitte_impl_id_str(sel.memcpy_impl),
                vitte_impl_id_str(sel.memset_impl),
                vitte_impl_id_str(sel.utf8_impl),
                (unsigned)sel.flags);
  return 1;
}
#endif
