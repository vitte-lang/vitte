// vitte/src/asm/src/vitte/cpu.c
//
// CPU description + minimal feature detection for Vitte ASM dispatch
//
// Goal:
//   - Provide vitte_cpu_desc_default / parse helpers declared in include/vitte/cpu.h
//   - Provide host detection (best-effort)
//   - Keep deps minimal; no heap allocation; no IO
//
// Notes:
//   - On x86_64, uses CPUID + XGETBV when available.
//   - On AArch64, NEON is mandatory; optional features not detected here.
//   - Triple parsing is permissive and partial.
//

#include <stdint.h>
#include <stddef.h>

#include "vitte/src/asm/include/vitte/asm.h"
#include "vitte/src/asm/include/vitte/asm_verify.h"
#include "vitte/src/asm/include/vitte/cpu.h"
// -----------------------------------------------------------------------------
// Small string helpers (no libc dependency beyond basic char ops)
// -----------------------------------------------------------------------------

static int vitte_streq(const char* a, const char* b) {
  if (!a || !b) return 0;
  while (*a && *b) {
    if (*a != *b) return 0;
    a++; b++;
  }
  return *a == *b;
}

static uint8_t vitte_starts_with(const char* s, const char* pfx) {
  if (!s || !pfx) return 0;
  while (*pfx) {
    if (*s++ != *pfx++) return 0;
  }
  return 1;
}

// Returns pointer to next '-' or '\0'
static const char* vitte_scan_to_dash(const char* s) {
  while (*s && *s != '-') s++;
  return s;
}

// Copy token [s, e) into out (null-terminated), truncating if needed.
// Returns 1 if non-empty.
static uint8_t vitte_copy_tok(char* out, size_t out_cap, const char* s, const char* e) {
  if (!out || out_cap == 0) return 0;
  size_t n = (size_t)(e - s);
  if (n == 0) { out[0] = 0; return 0; }
  if (n >= out_cap) n = out_cap - 1;
  for (size_t i = 0; i < n; i++) out[i] = s[i];
  out[n] = 0;
  return 1;
}

// -----------------------------------------------------------------------------
// Enum string mappers
// -----------------------------------------------------------------------------

const char* vitte_cpu_arch_str(vitte_cpu_arch_t a) {
  switch (a) {
    case VITTE_CPU_ARCH_VITTE_VM: return "vitte-vm";
    case VITTE_CPU_ARCH_X86: return "x86";
    case VITTE_CPU_ARCH_X86_64: return "x86_64";
    case VITTE_CPU_ARCH_AARCH64: return "aarch64";
    case VITTE_CPU_ARCH_ARMV7: return "armv7";
    case VITTE_CPU_ARCH_RISCV64: return "riscv64";
    default: return "unknown";
  }
}

const char* vitte_cpu_os_str(vitte_cpu_os_t os) {
  switch (os) {
    case VITTE_CPU_OS_LINUX: return "linux";
    case VITTE_CPU_OS_DARWIN: return "darwin";
    case VITTE_CPU_OS_WINDOWS: return "windows";
    case VITTE_CPU_OS_FREEBSD: return "freebsd";
    default: return "unknown";
  }
}

const char* vitte_cpu_abi_str(vitte_cpu_abi_t abi) {
  switch (abi) {
    case VITTE_CPU_ABI_SYSV: return "sysv";
    case VITTE_CPU_ABI_WIN64: return "win64";
    case VITTE_CPU_ABI_AAPCS64: return "aapcs64";
    case VITTE_CPU_ABI_VITTE_VM: return "vitte-vm";
    default: return "unknown";
  }
}

const char* vitte_cpu_endian_str(vitte_cpu_endian_t e) {
  switch (e) {
    case VITTE_CPU_ENDIAN_BIG: return "big";
    default: return "little";
  }
}

// -----------------------------------------------------------------------------
// Defaults
// -----------------------------------------------------------------------------

void vitte_cpu_desc_default(vitte_cpu_desc_t* out) {
  if (!out) return;

  out->arch = VITTE_CPU_ARCH_UNKNOWN;
  out->endian = VITTE_CPU_ENDIAN_LITTLE;
  out->bits = 0;

  out->os = VITTE_CPU_OS_UNKNOWN;
  out->abi = VITTE_CPU_ABI_UNKNOWN;

  out->arch_name = "unknown";
  out->cpu_name = "";
  out->os_name = "unknown";
  out->abi_name = "unknown";

  out->features_lo = 0;
  out->features_hi = 0;

  out->ptr_align = 0;
  out->stack_align = 16;
  out->code_align = 16;
  out->data_align = 16;
}

// -----------------------------------------------------------------------------
// Parsing helpers
// -----------------------------------------------------------------------------

uint8_t vitte_cpu_parse_arch(const char* s, vitte_cpu_arch_t* out_arch) {
  if (!s || !out_arch) return 0;

  if (vitte_streq(s, "x86_64") || vitte_streq(s, "amd64")) {
    *out_arch = VITTE_CPU_ARCH_X86_64;
    return 1;
  }
  if (vitte_streq(s, "x86") || vitte_streq(s, "i386") || vitte_streq(s, "i686")) {
    *out_arch = VITTE_CPU_ARCH_X86;
    return 1;
  }
  if (vitte_streq(s, "aarch64") || vitte_streq(s, "arm64")) {
    *out_arch = VITTE_CPU_ARCH_AARCH64;
    return 1;
  }
  if (vitte_streq(s, "armv7") || vitte_streq(s, "arm")) {
    *out_arch = VITTE_CPU_ARCH_ARMV7;
    return 1;
  }
  if (vitte_streq(s, "riscv64")) {
    *out_arch = VITTE_CPU_ARCH_RISCV64;
    return 1;
  }
  if (vitte_streq(s, "vitte-vm") || vitte_streq(s, "vitte_vm") || vitte_streq(s, "vitte")) {
    *out_arch = VITTE_CPU_ARCH_VITTE_VM;
    return 1;
  }

  *out_arch = VITTE_CPU_ARCH_UNKNOWN;
  return 0;
}

static uint8_t vitte_cpu_parse_os(const char* s, vitte_cpu_os_t* out_os) {
  if (!s || !out_os) return 0;

  if (vitte_streq(s, "linux")) { *out_os = VITTE_CPU_OS_LINUX; return 1; }
  if (vitte_streq(s, "darwin") || vitte_streq(s, "macos") || vitte_streq(s, "apple")) {
    *out_os = VITTE_CPU_OS_DARWIN; return 1;
  }
  if (vitte_streq(s, "windows") || vitte_streq(s, "win32") || vitte_streq(s, "mingw")) {
    *out_os = VITTE_CPU_OS_WINDOWS; return 1;
  }
  if (vitte_streq(s, "freebsd")) { *out_os = VITTE_CPU_OS_FREEBSD; return 1; }

  *out_os = VITTE_CPU_OS_UNKNOWN;
  return 0;
}

static uint8_t vitte_cpu_guess_abi(vitte_cpu_arch_t arch, vitte_cpu_os_t os, vitte_cpu_abi_t* out_abi) {
  if (!out_abi) return 0;

  if (arch == VITTE_CPU_ARCH_AARCH64) { *out_abi = VITTE_CPU_ABI_AAPCS64; return 1; }
  if (arch == VITTE_CPU_ARCH_VITTE_VM) { *out_abi = VITTE_CPU_ABI_VITTE_VM; return 1; }

  if (arch == VITTE_CPU_ARCH_X86_64) {
    if (os == VITTE_CPU_OS_WINDOWS) { *out_abi = VITTE_CPU_ABI_WIN64; return 1; }
    *out_abi = VITTE_CPU_ABI_SYSV;
    return 1;
  }

  *out_abi = VITTE_CPU_ABI_UNKNOWN;
  return 0;
}

uint8_t vitte_cpu_parse_triple(const char* triple, vitte_cpu_desc_t* out) {
  if (!triple || !out) return 0;

  vitte_cpu_desc_default(out);

  // Extract tokens: arch - vendor - os - abi (optional)
  char tok0[32] = {0};
  char tok1[32] = {0};
  char tok2[32] = {0};
  char tok3[32] = {0};

  const char* p = triple;
  const char* e0 = vitte_scan_to_dash(p);
  vitte_copy_tok(tok0, sizeof(tok0), p, e0);
  p = (*e0 == '-') ? (e0 + 1) : e0;

  const char* e1 = vitte_scan_to_dash(p);
  vitte_copy_tok(tok1, sizeof(tok1), p, e1);
  p = (*e1 == '-') ? (e1 + 1) : e1;

  const char* e2 = vitte_scan_to_dash(p);
  vitte_copy_tok(tok2, sizeof(tok2), p, e2);
  p = (*e2 == '-') ? (e2 + 1) : e2;

  const char* e3 = vitte_scan_to_dash(p);
  vitte_copy_tok(tok3, sizeof(tok3), p, e3);

  // arch
  vitte_cpu_parse_arch(tok0, &out->arch);
  out->arch_name = vitte_cpu_arch_str(out->arch);

  // os: usually tok2 on "arch-vendor-os-abi"
  vitte_cpu_parse_os(tok2, &out->os);
  out->os_name = vitte_cpu_os_str(out->os);

  // abi: if tok3 is known, keep string; else guess
  if (tok3[0]) {
    if (vitte_streq(tok3, "gnu") || vitte_streq(tok3, "sysv") || vitte_streq(tok3, "elf")) {
      out->abi = VITTE_CPU_ABI_SYSV;
    } else if (vitte_streq(tok3, "msvc") || vitte_streq(tok3, "win64")) {
      out->abi = VITTE_CPU_ABI_WIN64;
    } else if (vitte_streq(tok3, "aapcs64")) {
      out->abi = VITTE_CPU_ABI_AAPCS64;
    } else {
      vitte_cpu_guess_abi(out->arch, out->os, &out->abi);
    }
  } else {
    vitte_cpu_guess_abi(out->arch, out->os, &out->abi);
  }
  out->abi_name = vitte_cpu_abi_str(out->abi);

  // bits + alignments
  switch (out->arch) {
    case VITTE_CPU_ARCH_X86_64:
    case VITTE_CPU_ARCH_AARCH64:
    case VITTE_CPU_ARCH_RISCV64:
      out->bits = 64;
      out->ptr_align = 8;
      out->stack_align = 16;
      out->code_align = 16;
      out->data_align = 16;
      break;
    case VITTE_CPU_ARCH_X86:
    case VITTE_CPU_ARCH_ARMV7:
      out->bits = 32;
      out->ptr_align = 4;
      out->stack_align = 16;
      out->code_align = 16;
      out->data_align = 16;
      break;
    case VITTE_CPU_ARCH_VITTE_VM:
      out->bits = 64;
      out->ptr_align = 8;
      out->stack_align = 16;
      out->code_align = 16;
      out->data_align = 16;
      break;
    default:
      break;
  }

  // Features not inferred from triple (runtime detect does that)
  return 1;
}

// -----------------------------------------------------------------------------
// Support matrix
// -----------------------------------------------------------------------------

uint8_t vitte_cpu_arch_supported(vitte_cpu_arch_t arch) {
  switch (arch) {
    case VITTE_CPU_ARCH_X86_64:
    case VITTE_CPU_ARCH_AARCH64:
    case VITTE_CPU_ARCH_VITTE_VM:
      return 1;
    default:
      return 0;
  }
}

// -----------------------------------------------------------------------------
// x86_64 CPUID helpers (SysV / GCC/Clang / MSVC compatible)
// -----------------------------------------------------------------------------

#if defined(__x86_64__) || defined(_M_X64)

#if defined(_MSC_VER)
#  include <intrin.h>
static void vitte_cpuid(int out[4], int leaf, int subleaf) {
  __cpuidex(out, leaf, subleaf);
}
static uint64_t vitte_xgetbv(unsigned int xcr) {
  return _xgetbv(xcr);
}
#elif defined(__GNUC__) || defined(__clang__)
#  include <cpuid.h>
static void vitte_cpuid(int out[4], int leaf, int subleaf) {
  unsigned int a, b, c, d;
  __cpuid_count((unsigned int)leaf, (unsigned int)subleaf, a, b, c, d);
  out[0] = (int)a; out[1] = (int)b; out[2] = (int)c; out[3] = (int)d;
}
static uint64_t vitte_xgetbv(unsigned int xcr) {
  uint32_t eax = 0, edx = 0;
  __asm__ volatile (".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(xcr));
  return ((uint64_t)edx << 32) | (uint64_t)eax;
}
#else
static void vitte_cpuid(int out[4], int leaf, int subleaf) {
  (void)out; (void)leaf; (void)subleaf;
}
static uint64_t vitte_xgetbv(unsigned int xcr) {
  (void)xcr;
  return 0;
}
#endif

static uint8_t vitte_osxsave_enabled(void) {
  int info[4] = {0};
  vitte_cpuid(info, 1, 0);
  // ECX bit 27 = OSXSAVE
  return (info[2] & (1 << 27)) ? 1u : 0u;
}

static uint8_t vitte_avx_state_enabled(void) {
  if (!vitte_osxsave_enabled()) return 0u;
  // XCR0: bit 1 = XMM, bit 2 = YMM
  uint64_t xcr0 = vitte_xgetbv(0);
  return ((xcr0 & 0x6) == 0x6) ? 1u : 0u;
}

static void vitte_detect_x86_64(vitte_cpu_desc_t* out) {
  // Baseline: SSE2 always present on x86_64
  out->features_lo |= VITTE_CPU_FEAT_X86_SSE2;

  int info[4] = {0};

  // Leaf 1: ECX flags
  vitte_cpuid(info, 1, 0);
  int ecx = info[2];

  if (ecx & (1 << 0))  out->features_lo |= VITTE_CPU_FEAT_X86_SSE3;
  if (ecx & (1 << 9))  out->features_lo |= VITTE_CPU_FEAT_X86_SSSE3;
  if (ecx & (1 << 19)) out->features_lo |= VITTE_CPU_FEAT_X86_SSE41;
  if (ecx & (1 << 20)) out->features_lo |= VITTE_CPU_FEAT_X86_SSE42;
  if (ecx & (1 << 23)) out->features_lo |= VITTE_CPU_FEAT_X86_POPCNT;

  // AVX requires OS support for YMM state
  uint8_t avx_ok = 0;
  if (ecx & (1 << 28)) { // AVX bit
    if (vitte_avx_state_enabled()) avx_ok = 1;
  }
  if (avx_ok) out->features_lo |= VITTE_CPU_FEAT_X86_AVX;

  // Leaf 7 subleaf 0: EBX flags
  vitte_cpuid(info, 7, 0);
  int ebx = info[1];

  if (ebx & (1 << 3))  out->features_lo |= VITTE_CPU_FEAT_X86_BMI1;
  if (ebx & (1 << 8))  out->features_lo |= VITTE_CPU_FEAT_X86_BMI2;

  if (avx_ok && (ebx & (1 << 5))) out->features_lo |= VITTE_CPU_FEAT_X86_AVX2;

  // LZCNT is in extended leaf 0x80000001 ECX bit 5 (AMD) and/or via ABM
  vitte_cpuid(info, (int)0x80000000u, 0);
  if ((uint32_t)info[0] >= 0x80000001u) {
    vitte_cpuid(info, (int)0x80000001u, 0);
    int ecx2 = info[2];
    if (ecx2 & (1 << 5)) out->features_lo |= VITTE_CPU_FEAT_X86_LZCNT;
  }
}

#endif // x86_64

// -----------------------------------------------------------------------------
// AArch64 detection (minimal)
// -----------------------------------------------------------------------------

static void vitte_detect_aarch64(vitte_cpu_desc_t* out) {
  // NEON + FP are mandatory in AArch64 ISA profiles typically.
  out->features_lo |= VITTE_CPU_FEAT_A64_NEON;
  out->features_lo |= VITTE_CPU_FEAT_A64_FP;
  // Optional features (AES/SHA/CRC/LSE/SVE) not detected in this minimal file.
}

// -----------------------------------------------------------------------------
// Host detect entry
// -----------------------------------------------------------------------------

uint8_t vitte_cpu_detect_host(vitte_cpu_desc_t* out) {
  if (!out) return 0;

  vitte_cpu_desc_default(out);

#if defined(__aarch64__) || defined(_M_ARM64)
  out->arch = VITTE_CPU_ARCH_AARCH64;
  out->arch_name = "aarch64";
  out->endian = VITTE_CPU_ENDIAN_LITTLE;
  out->bits = 64;

#  if defined(__APPLE__)
  out->os = VITTE_CPU_OS_DARWIN;
  out->os_name = "darwin";
#  elif defined(_WIN32)
  out->os = VITTE_CPU_OS_WINDOWS;
  out->os_name = "windows";
#  elif defined(__linux__)
  out->os = VITTE_CPU_OS_LINUX;
  out->os_name = "linux";
#  elif defined(__FreeBSD__)
  out->os = VITTE_CPU_OS_FREEBSD;
  out->os_name = "freebsd";
#  endif

  out->abi = VITTE_CPU_ABI_AAPCS64;
  out->abi_name = "aapcs64";
  out->ptr_align = 8;
  out->stack_align = 16;
  out->code_align = 16;
  out->data_align = 16;

  vitte_detect_aarch64(out);
  return 1;

#elif defined(__x86_64__) || defined(_M_X64)
  out->arch = VITTE_CPU_ARCH_X86_64;
  out->arch_name = "x86_64";
  out->endian = VITTE_CPU_ENDIAN_LITTLE;
  out->bits = 64;

#  if defined(__APPLE__)
  out->os = VITTE_CPU_OS_DARWIN;
  out->os_name = "darwin";
#  elif defined(_WIN32)
  out->os = VITTE_CPU_OS_WINDOWS;
  out->os_name = "windows";
#  elif defined(__linux__)
  out->os = VITTE_CPU_OS_LINUX;
  out->os_name = "linux";
#  elif defined(__FreeBSD__)
  out->os = VITTE_CPU_OS_FREEBSD;
  out->os_name = "freebsd";
#  endif

  out->abi = (out->os == VITTE_CPU_OS_WINDOWS) ? VITTE_CPU_ABI_WIN64 : VITTE_CPU_ABI_SYSV;
  out->abi_name = vitte_cpu_abi_str(out->abi);

  out->ptr_align = 8;
  out->stack_align = 16;
  out->code_align = 16;
  out->data_align = 16;

#if defined(__x86_64__) || defined(_M_X64)
  vitte_detect_x86_64(out);
#endif
  return 1;

#else
  return 0;
#endif
}
