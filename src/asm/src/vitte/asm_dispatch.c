// vitte/src/asm/src/vitte/asm_dispatch.c
//
// ASM fastpaths dispatch (runtime selection)
//
// Contract:
//   - Exposes stable façade entry points for runtime/stdlib:
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

#if defined(_MSC_VER)
#  include <intrin.h>
#endif

#if !defined(__STDC_NO_ATOMICS__)
#  include <stdatomic.h>
#endif

#include "vitte/src/asm/include/vitte/asm.h"
#include "vitte/src/asm/include/vitte/asm_verify.h"
#include "vitte/src/asm/include/vitte/cpu.h"

// -----------------------------------------------------------------------------
// Public façade (stable API)
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
static atomic_int g_inited = 0; // 0=not init, 1=init
#else
static volatile int g_inited = 0;
#endif

// -----------------------------------------------------------------------------
// Minimal CPU detection helpers
// -----------------------------------------------------------------------------

static uint8_t vitte_has_avx2_sysv(void) {
#if defined(__x86_64__) || defined(_M_X64)
  // Prefer runtime detection if available.
  // GCC/Clang: __builtin_cpu_supports.
#  if defined(__GNUC__) || defined(__clang__)
    // This requires -march=x86-64 (ok) and not -mno-builtin-cpu.
    // It’s safe even if the binary was not compiled with AVX2.
    return __builtin_cpu_supports("avx2") ? 1u : 0u;
#  elif defined(_MSC_VER)
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);
    if (cpuInfo[0] < 7) return 0u;
    __cpuidex(cpuInfo, 7, 0);
    // EBX bit 5 = AVX2
    return (cpuInfo[1] & (1 << 5)) ? 1u : 0u;
#  else
    return 0u;
#  endif
#else
  return 0u;
#endif
}

static uint8_t vitte_has_sse2_sysv(void) {
#if defined(__x86_64__) || defined(_M_X64)
  // SSE2 is mandatory on x86_64.
  return 1u;
#else
  return 0u;
#endif
}

static uint8_t vitte_has_neon_aarch64(void) {
#if defined(__aarch64__) || defined(_M_ARM64)
  // NEON is mandatory in AArch64 ISA.
  return 1u;
#else
  return 0u;
#endif
}

// -----------------------------------------------------------------------------
// Dispatcher init (idempotent)
// -----------------------------------------------------------------------------

static void vitte_asm_dispatch_init_impl(void) {
  // Default assignments (safe)
#if defined(__aarch64__) || defined(_M_ARM64)
  g_fast.memcpy_fn = vitte_memcpy_baseline;
  g_fast.memset_fn = vitte_memset_baseline;
  g_fast.utf8_validate_fn = vitte_utf8_validate_stub;

  // Prefer NEON variants if linked
  if (vitte_has_neon_aarch64()) {
    // If you want build-time gating, wrap these in #if defined(VITTE_ENABLE_NEON)
    g_fast.memcpy_fn = vitte_memcpy_neon;
    g_fast.memset_fn = vitte_memset_neon;
  }

#elif defined(__x86_64__) || defined(_M_X64)
  // Baselines for x86_64
  // Prefer SSE2 version over "baseline" if you keep both; SSE2 is always safe.
  g_fast.memcpy_fn = vitte_memcpy_sse2;
  g_fast.memset_fn = vitte_memset_sse2;
  g_fast.utf8_validate_fn = vitte_utf8_validate_stub;

  // Prefer AVX2 if present and linked
  if (vitte_has_avx2_sysv()) {
    g_fast.memcpy_fn = vitte_memcpy_avx2;
    g_fast.memset_fn = vitte_memset_avx2;
  }

#else
  // Unknown arch: fall back to baseline if available; else leave null
  g_fast.memcpy_fn = vitte_memcpy_baseline;
  g_fast.memset_fn = vitte_memset_baseline;
  g_fast.utf8_validate_fn = vitte_utf8_validate_stub;
#endif

  // Hard fail-safes: if any pointer is NULL, point to safest symbols.
  if (!g_fast.memcpy_fn) g_fast.memcpy_fn = vitte_memcpy_baseline;
  if (!g_fast.memset_fn) g_fast.memset_fn = vitte_memset_baseline;
  if (!g_fast.utf8_validate_fn) g_fast.utf8_validate_fn = vitte_utf8_validate_stub;
}

static inline void vitte_asm_dispatch_init_once(void) {
#if !defined(__STDC_NO_ATOMICS__)
  int expected = 0;
  if (atomic_compare_exchange_strong(&g_inited, &expected, 1)) {
    vitte_asm_dispatch_init_impl();
  }
#else
  // Best-effort non-atomic fallback (single-thread init recommended)
  if (!g_inited) {
    g_inited = 1;
    vitte_asm_dispatch_init_impl();
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
