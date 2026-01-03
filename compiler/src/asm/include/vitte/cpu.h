

// cpu.h
//
// Vitte CPU feature probing (public header).
//
// This header defines a small, stable API for querying the current machine's
// CPU architecture, endianness, and optional ISA extensions (x86_64, aarch64).
//
// Design goals:
// - C ABI and cross-platform.
// - Best-effort probing: when unsupported/unavailable, report UNKNOWN/0.
// - No allocations; caller provides buffers.
//
// Notes:
// - This API is intended for selecting codegen/assembler backends and optional
//   fast paths.
// - For deterministic builds, probing should be optional and avoid affecting
//   artifact hashes.

#ifndef VITTE_CPU_H
#define VITTE_CPU_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Versioning
// -----------------------------------------------------------------------------

#define VITTE_CPU_ABI_VERSION 1

// -----------------------------------------------------------------------------
// Platform helpers
// -----------------------------------------------------------------------------

#if defined(_WIN32)
  #if defined(VITTE_CPU_BUILD_DLL)
    #define VITTE_CPU_API __declspec(dllexport)
  #elif defined(VITTE_CPU_USE_DLL)
    #define VITTE_CPU_API __declspec(dllimport)
  #else
    #define VITTE_CPU_API
  #endif
#else
  #define VITTE_CPU_API
#endif

#if defined(_WIN32) && !defined(__clang__)
  #define VITTE_CPU_CALL __cdecl
#else
  #define VITTE_CPU_CALL
#endif

// -----------------------------------------------------------------------------
// Basic enums
// -----------------------------------------------------------------------------

typedef enum vitte_cpu_arch {
  VITTE_CPU_ARCH_UNKNOWN = 0,
  VITTE_CPU_ARCH_X86_64  = 1,
  VITTE_CPU_ARCH_AARCH64 = 2,
  VITTE_CPU_ARCH_WASM32  = 3,
  VITTE_CPU_ARCH_WASM64  = 4,
  VITTE_CPU_ARCH_RISCV64 = 5
} vitte_cpu_arch;

typedef enum vitte_cpu_endian {
  VITTE_CPU_ENDIAN_UNKNOWN = 0,
  VITTE_CPU_ENDIAN_LITTLE  = 1,
  VITTE_CPU_ENDIAN_BIG     = 2
} vitte_cpu_endian;

// Stable string conversions (never returns NULL).
VITTE_CPU_API const char* VITTE_CPU_CALL vitte_cpu_arch_str(vitte_cpu_arch a);
VITTE_CPU_API const char* VITTE_CPU_CALL vitte_cpu_endian_str(vitte_cpu_endian e);

// -----------------------------------------------------------------------------
// Feature bits
// -----------------------------------------------------------------------------

// x86_64 feature bitset
enum {
  VITTE_CPU_X86_SSE2      = 1u << 0,
  VITTE_CPU_X86_SSE3      = 1u << 1,
  VITTE_CPU_X86_SSSE3     = 1u << 2,
  VITTE_CPU_X86_SSE41     = 1u << 3,
  VITTE_CPU_X86_SSE42     = 1u << 4,
  VITTE_CPU_X86_POPCNT    = 1u << 5,
  VITTE_CPU_X86_AVX       = 1u << 6,
  VITTE_CPU_X86_AVX2      = 1u << 7,
  VITTE_CPU_X86_BMI1      = 1u << 8,
  VITTE_CPU_X86_BMI2      = 1u << 9,
  VITTE_CPU_X86_AESNI     = 1u << 10,
  VITTE_CPU_X86_PCLMULQDQ = 1u << 11,
  VITTE_CPU_X86_FMA       = 1u << 12,
  VITTE_CPU_X86_F16C      = 1u << 13,
  VITTE_CPU_X86_LZCNT     = 1u << 14
};

// aarch64 feature bitset
enum {
  VITTE_CPU_ARM_NEON      = 1u << 0,
  VITTE_CPU_ARM_AES       = 1u << 1,
  VITTE_CPU_ARM_PMULL     = 1u << 2,
  VITTE_CPU_ARM_SHA1      = 1u << 3,
  VITTE_CPU_ARM_SHA2      = 1u << 4,
  VITTE_CPU_ARM_CRC32     = 1u << 5
};

// -----------------------------------------------------------------------------
// CPU info
// -----------------------------------------------------------------------------

typedef struct vitte_cpu_info {
  uint32_t abi_version;     // VITTE_CPU_ABI_VERSION
  vitte_cpu_arch arch;
  vitte_cpu_endian endian;

  // Optional descriptive strings (best-effort):
  // - vendor: e.g., "GenuineIntel", "AuthenticAMD", "Apple"
  // - brand:  e.g., "Intel(R) Core(TM) i7-..."
  //
  // Populated only if the caller provided buffers.
  char* vendor;
  size_t vendor_cap;
  char* brand;
  size_t brand_cap;

  // Feature bitsets (interpretation depends on arch)
  uint32_t features0; // x86: x86 bits; arm: arm bits
  uint32_t features1; // reserved for future expansion

  // Core counts (best-effort). 0 if unknown.
  uint32_t logical_cores;
  uint32_t physical_cores;
} vitte_cpu_info;

// Initialize a vitte_cpu_info struct with defaults.
VITTE_CPU_API void VITTE_CPU_CALL vitte_cpu_info_init(
  vitte_cpu_info* info,
  char* vendor_buf, size_t vendor_cap,
  char* brand_buf, size_t brand_cap
);

// Probe the current CPU. Best-effort; fills info fields.
VITTE_CPU_API void VITTE_CPU_CALL vitte_cpu_probe(vitte_cpu_info* info);

// Convenience checks
VITTE_CPU_API bool VITTE_CPU_CALL vitte_cpu_has(vitte_cpu_info* info, uint32_t feature_bit);

// -----------------------------------------------------------------------------
// Compile-time (build) target info
// -----------------------------------------------------------------------------

// Return the compile-time architecture/endian (from predefined macros), which
// may differ from runtime on emulation.
VITTE_CPU_API vitte_cpu_arch VITTE_CPU_CALL vitte_cpu_build_arch(void);
VITTE_CPU_API vitte_cpu_endian VITTE_CPU_CALL vitte_cpu_build_endian(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_CPU_H