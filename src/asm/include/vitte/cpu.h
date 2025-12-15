// vitte/src/asm/include/vitte/cpu.h
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Vitte ASM — CPU / Target descriptions
//   - Décrit CPU, ISA, features, ABI de base
//   - Utilisé par l'assembleur et/ou le codegen
// ============================================================================

// ---------------------------
// Enums
// ---------------------------

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
  VITTE_CPU_ABI_SYSV = 1,       // x86_64 System V
  VITTE_CPU_ABI_WIN64 = 2,      // Windows x64
  VITTE_CPU_ABI_AAPCS64 = 3,    // AArch64 ELF
  VITTE_CPU_ABI_VITTE_VM = 10   // ABI VM Vitte
} vitte_cpu_abi_t;

typedef enum vitte_cpu_os_t {
  VITTE_CPU_OS_UNKNOWN = 0,
  VITTE_CPU_OS_LINUX = 1,
  VITTE_CPU_OS_DARWIN = 2,
  VITTE_CPU_OS_WINDOWS = 3,
  VITTE_CPU_OS_FREEBSD = 4
} vitte_cpu_os_t;

// ---------------------------
// Feature bitsets (generic)
// ---------------------------

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

// ---------------------------
// CPU description
// ---------------------------

typedef struct vitte_cpu_desc_t {
  vitte_cpu_arch_t arch;
  vitte_cpu_endian_t endian;

  // "bits" = 32/64 pour CPU natif. Pour VM: 0 ou 64 selon ta spec.
  uint16_t bits;

  // OS/ABI context (utile pour conventions d'appel / reloc)
  vitte_cpu_os_t os;
  vitte_cpu_abi_t abi;

  // string hints (owned by caller or static)
  const char* arch_name;    // e.g. "x86_64", "aarch64"
  const char* cpu_name;     // e.g. "znver4", "skylake", "apple-m2"
  const char* os_name;      // e.g. "linux", "darwin"
  const char* abi_name;     // e.g. "sysv", "win64"

  // feature bitsets (arch-specific meaning)
  uint32_t features_lo;
  uint32_t features_hi;

  // alignments (ABI-level)
  uint8_t ptr_align;        // bytes
  uint8_t stack_align;      // bytes
  uint8_t code_align;       // bytes
  uint8_t data_align;       // bytes
} vitte_cpu_desc_t;

// ---------------------------
// Defaults / parsing helpers
// ---------------------------

void vitte_cpu_desc_default(vitte_cpu_desc_t* out);

// Parse "triple" style: arch-vendor-os-abi (accepts partial).
// Example: "x86_64-unknown-linux-gnu", "aarch64-apple-darwin", "x86_64-pc-windows-msvc".
// Returns 1 on success, 0 on failure.
uint8_t vitte_cpu_parse_triple(const char* triple, vitte_cpu_desc_t* out);

// Parse arch string: "x86_64", "aarch64", "riscv64", "vitte-vm"...
uint8_t vitte_cpu_parse_arch(const char* s, vitte_cpu_arch_t* out);

// Map enum -> stable string (never NULL, returns "unknown" if needed)
const char* vitte_cpu_arch_str(vitte_cpu_arch_t a);
const char* vitte_cpu_os_str(vitte_cpu_os_t os);
const char* vitte_cpu_abi_str(vitte_cpu_abi_t abi);
const char* vitte_cpu_endian_str(vitte_cpu_endian_t e);

// ---------------------------
// Feature helpers
// ---------------------------

// Generic feature test/set for (lo, hi) bitsets.
static inline uint8_t vitte_cpu_feat_test(uint32_t lo, uint32_t hi, uint32_t bit_index) {
  if (bit_index < 32u) return (uint8_t)((lo >> bit_index) & 1u);
  return (uint8_t)((hi >> (bit_index - 32u)) & 1u);
}

static inline void vitte_cpu_feat_set(uint32_t* lo, uint32_t* hi, uint32_t bit_index) {
  if (bit_index < 32u) *lo |= (1u << bit_index);
  else *hi |= (1u << (bit_index - 32u));
}

// Arch-specific: retourne 1 si le backend encodeur est implémenté/activé.
uint8_t vitte_cpu_arch_supported(vitte_cpu_arch_t arch);

// Detect host CPU (best-effort). If not supported, fills defaults and returns 0.
uint8_t vitte_cpu_detect_host(vitte_cpu_desc_t* out);

#ifdef __cplusplus
} // extern "C"
#endif
