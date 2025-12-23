// vitte/include/vitte/cpu.h
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Vitte runtime/asm - CPU / Target descriptions
// ============================================================================

typedef enum vitte_cpu_arch_t {
  VITTE_CPU_ARCH_UNKNOWN = 0,
  VITTE_CPU_ARCH_VITTE_VM = 1,
  VITTE_CPU_ARCH_X86 = 2,
  VITTE_CPU_ARCH_X86_64 = 3,
  VITTE_CPU_ARCH_AARCH64 = 4,
  VITTE_CPU_ARCH_ARMV7 = 5,
  VITTE_CPU_ARCH_RISCV64 = 6
} vitte_cpu_arch_t;

typedef enum vitte_cpu_endian_t {
  VITTE_CPU_ENDIAN_LITTLE = 0,
  VITTE_CPU_ENDIAN_BIG = 1
} vitte_cpu_endian_t;

typedef enum vitte_cpu_abi_t {
  VITTE_CPU_ABI_UNKNOWN = 0,
  VITTE_CPU_ABI_SYSV = 1,
  VITTE_CPU_ABI_WIN64 = 2,
  VITTE_CPU_ABI_AAPCS64 = 3,
  VITTE_CPU_ABI_VITTE_VM = 10
} vitte_cpu_abi_t;

typedef enum vitte_cpu_os_t {
  VITTE_CPU_OS_UNKNOWN = 0,
  VITTE_CPU_OS_LINUX = 1,
  VITTE_CPU_OS_DARWIN = 2,
  VITTE_CPU_OS_WINDOWS = 3,
  VITTE_CPU_OS_FREEBSD = 4
} vitte_cpu_os_t;

// x86 / x86_64 (subset)
enum {
  VITTE_CPU_FEAT_X86_SSE2 = 1u << 0,
  VITTE_CPU_FEAT_X86_SSE3 = 1u << 1,
  VITTE_CPU_FEAT_X86_SSSE3 = 1u << 2,
  VITTE_CPU_FEAT_X86_SSE41 = 1u << 3,
  VITTE_CPU_FEAT_X86_SSE42 = 1u << 4,
  VITTE_CPU_FEAT_X86_AVX = 1u << 5,
  VITTE_CPU_FEAT_X86_AVX2 = 1u << 6,
  VITTE_CPU_FEAT_X86_BMI1 = 1u << 7,
  VITTE_CPU_FEAT_X86_BMI2 = 1u << 8,
  VITTE_CPU_FEAT_X86_POPCNT = 1u << 9,
  VITTE_CPU_FEAT_X86_LZCNT = 1u << 10
};

// aarch64 (subset)
enum {
  VITTE_CPU_FEAT_A64_NEON = 1u << 0,
  VITTE_CPU_FEAT_A64_FP = 1u << 1,
  VITTE_CPU_FEAT_A64_AES = 1u << 2,
  VITTE_CPU_FEAT_A64_SHA1 = 1u << 3,
  VITTE_CPU_FEAT_A64_SHA2 = 1u << 4,
  VITTE_CPU_FEAT_A64_CRC32 = 1u << 5,
  VITTE_CPU_FEAT_A64_LSE = 1u << 6
};

// riscv64 (subset)
enum {
  VITTE_CPU_FEAT_RV64_M = 1u << 0,
  VITTE_CPU_FEAT_RV64_A = 1u << 1,
  VITTE_CPU_FEAT_RV64_F = 1u << 2,
  VITTE_CPU_FEAT_RV64_D = 1u << 3,
  VITTE_CPU_FEAT_RV64_C = 1u << 4,
  VITTE_CPU_FEAT_RV64_V = 1u << 5
};

typedef struct vitte_cpu_desc_t {
  vitte_cpu_arch_t arch;
  vitte_cpu_endian_t endian;
  uint16_t bits;
  vitte_cpu_os_t os;
  vitte_cpu_abi_t abi;
  const char* arch_name;
  const char* cpu_name;
  const char* os_name;
  const char* abi_name;
  uint32_t features_lo;
  uint32_t features_hi;
  uint8_t ptr_align;
  uint8_t stack_align;
  uint8_t code_align;
  uint8_t data_align;
} vitte_cpu_desc_t;

void vitte_cpu_desc_default(vitte_cpu_desc_t* out);
uint8_t vitte_cpu_parse_triple(const char* triple, vitte_cpu_desc_t* out);
uint8_t vitte_cpu_parse_arch(const char* s, vitte_cpu_arch_t* out);

const char* vitte_cpu_arch_str(vitte_cpu_arch_t a);
const char* vitte_cpu_os_str(vitte_cpu_os_t os);
const char* vitte_cpu_abi_str(vitte_cpu_abi_t abi);
const char* vitte_cpu_endian_str(vitte_cpu_endian_t e);

static inline uint8_t vitte_cpu_feat_test(uint32_t lo, uint32_t hi, uint32_t bit_index) {
  if (bit_index < 32u) return (uint8_t)((lo >> bit_index) & 1u);
  return (uint8_t)((hi >> (bit_index - 32u)) & 1u);
}

static inline void vitte_cpu_feat_set(uint32_t* lo, uint32_t* hi, uint32_t bit_index) {
  if (bit_index < 32u) *lo |= (1u << bit_index);
  else *hi |= (1u << (bit_index - 32u));
}

uint8_t vitte_cpu_arch_supported(vitte_cpu_arch_t arch);
uint8_t vitte_cpu_detect_host(vitte_cpu_desc_t* out);

#ifdef __cplusplus
} // extern "C"
#endif

