// bench_asm_shim.h — portable asm/intrinsics shim (C17)
//
// Public API for bench_asm_shim.c

#ifndef VITTE_BENCH_ASM_SHIM_H
#define VITTE_BENCH_ASM_SHIM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Monotonic timestamp in nanoseconds (fallback clock).
uint64_t bench_asm_time_now_ns(void);

// Raw counter, best available on this platform.
uint64_t bench_asm_cycles_now(void);

// Counter frequency in Hz when known (0 if unknown).
uint64_t bench_asm_cycles_freq_hz(void);

// Serialized begin/end for stable deltas.
uint64_t bench_asm_cycles_begin(void);
uint64_t bench_asm_cycles_end(void);

// Spin-wait friendly relax.
void bench_asm_cpu_relax(void);

// Compile-time barrier.
void bench_asm_compiler_barrier(void);

// Best-effort cache line flush.
void bench_asm_clflush(const void* p);

// Best-effort prefetch into L1.
void bench_asm_prefetch_t0(const void* p);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_BENCH_ASM_SHIM_H
