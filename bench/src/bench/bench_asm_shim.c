// bench/src/bench/bench_asm_shim.c
//
// Glue exposing the ASM dispatch layer under the names expected by the
// micro-benchmarks. Enabled via VITTE_ENABLE_ASM_FASTPATHS.

#include <stddef.h>
#include <stdint.h>

void* vitte_memcpy(void* dst, const void* src, size_t n);
uint64_t vitte_hash_fnv1a64(const uint8_t* data, size_t len);

void* vitte_memcpy_fast(void* dst, const void* src, size_t n) {
  return vitte_memcpy(dst, src, n);
}

uint64_t vitte_fnv1a64_fast(const uint8_t* data, size_t len) {
  return vitte_hash_fnv1a64(data, len);
}
