/*
 * toolchain/config/include/toolchain/clang_target.h
 *
 * Vitte toolchain – Clang target model (triple / arch / os / env / abi).
 *
 * Goals:
 *  - Represent a compilation target (host or cross).
 *  - Parse/format LLVM-style target triples (e.g. x86_64-pc-windows-msvc).
 *  - Provide small helpers to build clang flags: -target, -march, -mcpu, -mabi, --sysroot.
 *
 * This header is standalone (C17). No allocation required; caller provides buffers.
 */

#ifndef VITTE_TOOLCHAIN_CLANG_TARGET_H
#define VITTE_TOOLCHAIN_CLANG_TARGET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* -----------------------------------------------------------------------------
 * Build config
 * -------------------------------------------------------------------------- */

#ifndef TC_TARGET_MAX_STR
#define TC_TARGET_MAX_STR 128u
#endif

#ifndef TC_TARGET_MAX_CPU
#define TC_TARGET_MAX_CPU 64u
#endif

#ifndef TC_TARGET_MAX_ABI
#define TC_TARGET_MAX_ABI 32u
#endif

#ifndef TC_TARGET_MAX_VENDOR
#define TC_TARGET_MAX_VENDOR 32u
#endif

#ifndef TC_TARGET_MAX_OSVER
#define TC_TARGET_MAX_OSVER 32u
#endif

/* Conservative: enough for "x86_64-unknown-linux-gnu" etc. */
#ifndef TC_TARGET_TRIPLE_MAX
#define TC_TARGET_TRIPLE_MAX TC_TARGET_MAX_STR
#endif

/* -----------------------------------------------------------------------------
 * Enums
 * -------------------------------------------------------------------------- */

typedef enum tc_arch_e {
    TC_ARCH_UNKNOWN = 0,

    TC_ARCH_X86_32,
    TC_ARCH_X86_64,

    TC_ARCH_ARMV7,      /* armv7 (32-bit) */
    TC_ARCH_AARCH64,    /* arm64 */

    TC_ARCH_RISCV32,
    TC_ARCH_RISCV64,

    TC_ARCH_PPC64LE,
    TC_ARCH_WASM32,
    TC_ARCH_WASM64
} tc_arch;

typedef enum tc_os_e {
    TC_OS_UNKNOWN = 0,

    TC_OS_WINDOWS,
    TC_OS_LINUX,
    TC_OS_DARWIN,   /* macOS */
    TC_OS_IOS,
    TC_OS_ANDROID,
    TC_OS_FREEBSD,
    TC_OS_NETBSD,
    TC_OS_OPENBSD,
    TC_OS_WASI
} tc_os;

typedef enum tc_env_e {
    TC_ENV_UNKNOWN = 0,

    /* Windows */
    TC_ENV_MSVC,
    TC_ENV_GNU,     /* mingw */
    TC_ENV_UCRT,    /* mingw-ucrt variants */

    /* ELF-ish */
    TC_ENV_GNUC,    /* "gnu" */
    TC_ENV_MUSL,
    TC_ENV_ANDROID,

    /* Apple */
    TC_ENV_MACABI,  /* macosx + macabi style */
    TC_ENV_SIMULATOR,

    /* WASI */
    TC_ENV_WASI
} tc_env;

typedef enum tc_endian_e {
    TC_ENDIAN_UNKNOWN = 0,
    TC_ENDIAN_LITTLE,
    TC_ENDIAN_BIG
} tc_endian;

/* ABI “flavor” is intentionally coarse (Clang mostly infers from triple). */
typedef enum tc_abi_e {
    TC_ABI_UNKNOWN = 0,
    TC_ABI_DEFAULT,

    /* ARM */
    TC_ABI_EABI,
    TC_ABI_EABIHF,

    /* RISC-V */
    TC_ABI_LP64,
    TC_ABI_LP64D,
    TC_ABI_ILP32,
    TC_ABI_ILP32D
} tc_abi;

/* -----------------------------------------------------------------------------
 * Target model
 * -------------------------------------------------------------------------- */

typedef struct tc_target_s {
    tc_arch   arch;
    tc_os     os;
    tc_env    env;
    tc_abi    abi;
    tc_endian endian;

    /* Optional strings (may be empty). */
    char vendor[TC_TARGET_MAX_VENDOR]; /* e.g. "pc", "apple", "unknown" */
    char osver[TC_TARGET_MAX_OSVER];   /* e.g. "13.0", "10.0.19041" */

    /* Optional overrides (may be empty). */
    char cpu[TC_TARGET_MAX_CPU];       /* -mcpu= */
    char abi_str[TC_TARGET_MAX_ABI];   /* -mabi= (string form) */

    /* Cached formatted triple (may be empty if not built). */
    char triple[TC_TARGET_TRIPLE_MAX];
} tc_target;

/* -----------------------------------------------------------------------------
 * Status / errors
 * -------------------------------------------------------------------------- */

typedef enum tc_target_err_e {
    TC_TARGET_OK = 0,
    TC_TARGET_EINVAL,
    TC_TARGET_EOVERFLOW,
    TC_TARGET_EPARSE
} tc_target_err;

/* -----------------------------------------------------------------------------
 * Host detection
 * -------------------------------------------------------------------------- */

static inline tc_arch tc_arch_host(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return TC_ARCH_X86_64;
#elif defined(__i386__) || defined(_M_IX86)
    return TC_ARCH_X86_32;
#elif defined(__aarch64__) || defined(_M_ARM64)
    return TC_ARCH_AARCH64;
#elif defined(__arm__) || defined(_M_ARM)
    return TC_ARCH_ARMV7;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return TC_ARCH_RISCV64;
#elif defined(__riscv) && (__riscv_xlen == 32)
    return TC_ARCH_RISCV32;
#elif defined(__powerpc64__) && defined(__LITTLE_ENDIAN__)
    return TC_ARCH_PPC64LE;
#elif defined(__wasm32__)
    return TC_ARCH_WASM32;
#elif defined(__wasm64__)
    return TC_ARCH_WASM64;
#else
    return TC_ARCH_UNKNOWN;
#endif
}

static inline tc_os tc_os_host(void) {
#if defined(_WIN32)
    return TC_OS_WINDOWS;
#elif defined(__APPLE__) && defined(__MACH__)
    return TC_OS_DARWIN;
#elif defined(__ANDROID__)
    return TC_OS_ANDROID;
#elif defined(__linux__)
    return TC_OS_LINUX;
#elif defined(__FreeBSD__)
    return TC_OS_FREEBSD;
#elif defined(__NetBSD__)
    return TC_OS_NETBSD;
#elif defined(__OpenBSD__)
    return TC_OS_OPENBSD;
#elif defined(__wasi__)
    return TC_OS_WASI;
#else
    return TC_OS_UNKNOWN;
#endif
}

static inline tc_env tc_env_host(void) {
#if defined(_WIN32)
  #if defined(_MSC_VER)
    return TC_ENV_MSVC;
  #else
    return TC_ENV_GNU;
  #endif
#elif defined(__wasi__)
    return TC_ENV_WASI;
#elif defined(__ANDROID__)
    return TC_ENV_ANDROID;
#elif defined(__linux__)
  #if defined(__musl__)
    return TC_ENV_MUSL;
  #else
    return TC_ENV_GNUC;
  #endif
#else
    return TC_ENV_UNKNOWN;
#endif
}

static inline tc_endian tc_endian_host(void) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
  #if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    return TC_ENDIAN_LITTLE;
  #elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    return TC_ENDIAN_BIG;
  #else
    return TC_ENDIAN_UNKNOWN;
  #endif
#elif defined(_WIN32)
    return TC_ENDIAN_LITTLE;
#else
    return TC_ENDIAN_UNKNOWN;
#endif
}

/* -----------------------------------------------------------------------------
 * String helpers
 * -------------------------------------------------------------------------- */

static inline const char* tc_arch_to_str(tc_arch a) {
    switch (a) {
        case TC_ARCH_X86_32:  return "i386";
        case TC_ARCH_X86_64:  return "x86_64";
        case TC_ARCH_ARMV7:   return "armv7";
        case TC_ARCH_AARCH64: return "aarch64";
        case TC_ARCH_RISCV32: return "riscv32";
        case TC_ARCH_RISCV64: return "riscv64";
        case TC_ARCH_PPC64LE: return "powerpc64le";
        case TC_ARCH_WASM32:  return "wasm32";
        case TC_ARCH_WASM64:  return "wasm64";
        default:              return "unknown";
    }
}

static inline const char* tc_os_to_str(tc_os o) {
    switch (o) {
        case TC_OS_WINDOWS: return "windows";
        case TC_OS_LINUX:   return "linux";
        case TC_OS_DARWIN:  return "darwin";
        case TC_OS_IOS:     return "ios";
        case TC_OS_ANDROID: return "android";
        case TC_OS_FREEBSD: return "freebsd";
        case TC_OS_NETBSD:  return "netbsd";
        case TC_OS_OPENBSD: return "openbsd";
        case TC_OS_WASI:    return "wasi";
        default:            return "unknown";
    }
}

static inline const char* tc_env_to_str(tc_env e) {
    switch (e) {
        case TC_ENV_MSVC:      return "msvc";
        case TC_ENV_GNU:       return "gnu";
        case TC_ENV_UCRT:      return "ucrt";
        case TC_ENV_GNUC:      return "gnu";
        case TC_ENV_MUSL:      return "musl";
        case TC_ENV_ANDROID:   return "android";
        case TC_ENV_MACABI:    return "macabi";
        case TC_ENV_SIMULATOR: return "simulator";
        case TC_ENV_WASI:      return "wasi";
        default:               return "unknown";
    }
}

static inline const char* tc_abi_to_str(tc_abi a) {
    switch (a) {
        case TC_ABI_DEFAULT: return "default";
        case TC_ABI_EABI:    return "eabi";
        case TC_ABI_EABIHF:  return "eabihf";
        case TC_ABI_LP64:    return "lp64";
        case TC_ABI_LP64D:   return "lp64d";
        case TC_ABI_ILP32:   return "ilp32";
        case TC_ABI_ILP32D:  return "ilp32d";
        default:             return "unknown";
    }
}

/* -----------------------------------------------------------------------------
 * Core init
 * -------------------------------------------------------------------------- */

static inline void tc_target_zero(tc_target* t) {
    if (!t) return;
    memset(t, 0, sizeof(*t));
    t->arch = TC_ARCH_UNKNOWN;
    t->os   = TC_OS_UNKNOWN;
    t->env  = TC_ENV_UNKNOWN;
    t->abi  = TC_ABI_UNKNOWN;
    t->endian = TC_ENDIAN_UNKNOWN;
}

static inline void tc_target_set_host(tc_target* t) {
    tc_target_zero(t);
    if (!t) return;

    t->arch   = tc_arch_host();
    t->os     = tc_os_host();
    t->env    = tc_env_host();
    t->endian = tc_endian_host();

    /* reasonable defaults */
    if (t->os == TC_OS_WINDOWS) {
        strncpy(t->vendor, "pc", sizeof(t->vendor) - 1u);
        if (t->env == TC_ENV_UNKNOWN) t->env = TC_ENV_MSVC;
    } else if (t->os == TC_OS_DARWIN || t->os == TC_OS_IOS) {
        strncpy(t->vendor, "apple", sizeof(t->vendor) - 1u);
    } else {
        strncpy(t->vendor, "unknown", sizeof(t->vendor) - 1u);
        if (t->os == TC_OS_LINUX && t->env == TC_ENV_UNKNOWN) t->env = TC_ENV_GNUC;
        if (t->os == TC_OS_WASI) t->env = TC_ENV_WASI;
    }
}

/* -----------------------------------------------------------------------------
 * Parsing helpers (minimal, robust)
 * -------------------------------------------------------------------------- */

static inline bool tc_streq(const char* a, const char* b) {
    return (a && b) ? (strcmp(a, b) == 0) : false;
}

static inline tc_arch tc_arch_from_str(const char* s) {
    if (!s || !*s) return TC_ARCH_UNKNOWN;

    if (tc_streq(s, "x86_64") || tc_streq(s, "amd64")) return TC_ARCH_X86_64;
    if (tc_streq(s, "i386") || tc_streq(s, "i686") || tc_streq(s, "x86")) return TC_ARCH_X86_32;

    if (tc_streq(s, "aarch64") || tc_streq(s, "arm64")) return TC_ARCH_AARCH64;
    if (tc_streq(s, "armv7") || tc_streq(s, "armv7a") || tc_streq(s, "arm")) return TC_ARCH_ARMV7;

    if (tc_streq(s, "riscv32")) return TC_ARCH_RISCV32;
    if (tc_streq(s, "riscv64")) return TC_ARCH_RISCV64;

    if (tc_streq(s, "powerpc64le") || tc_streq(s, "ppc64le")) return TC_ARCH_PPC64LE;

    if (tc_streq(s, "wasm32")) return TC_ARCH_WASM32;
    if (tc_streq(s, "wasm64")) return TC_ARCH_WASM64;

    return TC_ARCH_UNKNOWN;
}

static inline tc_os tc_os_from_str(const char* s) {
    if (!s || !*s) return TC_OS_UNKNOWN;

    if (tc_streq(s, "windows") || tc_streq(s, "win32")) return TC_OS_WINDOWS;
    if (tc_streq(s, "linux")) return TC_OS_LINUX;
    if (tc_streq(s, "darwin") || tc_streq(s, "macos") || tc_streq(s, "macosx")) return TC_OS_DARWIN;
    if (tc_streq(s, "ios")) return TC_OS_IOS;
    if (tc_streq(s, "android")) return TC_OS_ANDROID;
    if (tc_streq(s, "freebsd")) return TC_OS_FREEBSD;
    if (tc_streq(s, "netbsd")) return TC_OS_NETBSD;
    if (tc_streq(s, "openbsd")) return TC_OS_OPENBSD;
    if (tc_streq(s, "wasi")) return TC_OS_WASI;

    return TC_OS_UNKNOWN;
}

static inline tc_env tc_env_from_str(const char* s) {
    if (!s || !*s) return TC_ENV_UNKNOWN;

    if (tc_streq(s, "msvc")) return TC_ENV_MSVC;
    if (tc_streq(s, "gnu")) return TC_ENV_GNUC; /* will be refined depending on OS */
    if (tc_streq(s, "mingw32") || tc_streq(s, "mingw") || tc_streq(s, "gnu")) return TC_ENV_GNU;
    if (tc_streq(s, "ucrt")) return TC_ENV_UCRT;

    if (tc_streq(s, "musl")) return TC_ENV_MUSL;
    if (tc_streq(s, "android")) return TC_ENV_ANDROID;

    if (tc_streq(s, "macabi")) return TC_ENV_MACABI;
    if (tc_streq(s, "simulator")) return TC_ENV_SIMULATOR;

    if (tc_streq(s, "wasi")) return TC_ENV_WASI;

    return TC_ENV_UNKNOWN;
}

/* Split "a-b-c-d" into up to 4 parts (in-place, NUL-terminated). */
static inline size_t tc_split4(char* s, char* out[4]) {
    size_t n = 0;
    if (!s) return 0;

    out[0] = out[1] = out[2] = out[3] = NULL;

    char* p = s;
    out[n++] = p;

    while (*p && n < 4) {
        if (*p == '-') {
            *p = '\0';
            out[n++] = p + 1;
        }
        ++p;
    }
    return n;
}

/*
 * Parse LLVM-style triple:
 *   arch-vendor-os[-env]
 *
 * Examples:
 *   x86_64-pc-windows-msvc
 *   aarch64-apple-darwin
 *   x86_64-unknown-linux-gnu
 *   riscv64-unknown-linux-musl
 */
static inline tc_target_err tc_target_parse_triple(tc_target* t, const char* triple) {
    if (!t || !triple || !*triple) return TC_TARGET_EINVAL;

    tc_target_zero(t);

    char tmp[TC_TARGET_TRIPLE_MAX];
    size_t L = strnlen(triple, sizeof(tmp));
    if (L >= sizeof(tmp)) return TC_TARGET_EOVERFLOW;
    memcpy(tmp, triple, L);
    tmp[L] = '\0';

    char* parts[4];
    size_t n = tc_split4(tmp, parts);
    if (n < 3) return TC_TARGET_EPARSE;

    t->arch = tc_arch_from_str(parts[0]);

    /* vendor */
    if (parts[1] && *parts[1]) {
        strncpy(t->vendor, parts[1], sizeof(t->vendor) - 1u);
    } else {
        strncpy(t->vendor, "unknown", sizeof(t->vendor) - 1u);
    }

    /* os (can include version like "darwin20.1.0" in the wild; we keep minimal) */
    t->os = tc_os_from_str(parts[2]);

    /* env */
    if (n >= 4 && parts[3] && *parts[3]) {
        t->env = tc_env_from_str(parts[3]);
    } else {
        t->env = TC_ENV_UNKNOWN;
    }

    /* normalize env based on OS when ambiguous */
    if (t->os == TC_OS_WINDOWS) {
        if (t->env == TC_ENV_UNKNOWN) t->env = TC_ENV_MSVC;
        if (t->env == TC_ENV_GNUC) t->env = TC_ENV_GNU;
        strncpy(t->vendor, (t->vendor[0] ? t->vendor : "pc"), sizeof(t->vendor) - 1u);
    } else if (t->os == TC_OS_LINUX) {
        if (t->env == TC_ENV_GNU) t->env = TC_ENV_GNUC;
        if (t->env == TC_ENV_UNKNOWN) t->env = TC_ENV_GNUC;
    } else if (t->os == TC_OS_DARWIN || t->os == TC_OS_IOS) {
        if (t->vendor[0] == '\0') strncpy(t->vendor, "apple", sizeof(t->vendor) - 1u);
    } else if (t->os == TC_OS_WASI) {
        t->env = TC_ENV_WASI;
    }

    /* endian defaults */
    switch (t->arch) {
        case TC_ARCH_PPC64LE:
        case TC_ARCH_X86_32:
        case TC_ARCH_X86_64:
        case TC_ARCH_ARMV7:
        case TC_ARCH_AARCH64:
        case TC_ARCH_RISCV32:
        case TC_ARCH_RISCV64:
        case TC_ARCH_WASM32:
        case TC_ARCH_WASM64:
            t->endian = TC_ENDIAN_LITTLE;
            break;
        default:
            t->endian = TC_ENDIAN_UNKNOWN;
            break;
    }

    /* store original triple (best-effort) */
    strncpy(t->triple, triple, sizeof(t->triple) - 1u);

    return TC_TARGET_OK;
}

/* -----------------------------------------------------------------------------
 * Triple formatting
 * -------------------------------------------------------------------------- */

static inline tc_target_err tc_target_build_triple(tc_target* t) {
    if (!t) return TC_TARGET_EINVAL;

    const char* arch = tc_arch_to_str(t->arch);
    const char* vend = (t->vendor[0] ? t->vendor : "unknown");

    /* OS token for LLVM triple */
    const char* os_tok = NULL;
    switch (t->os) {
        case TC_OS_WINDOWS: os_tok = "windows"; break;
        case TC_OS_LINUX:   os_tok = "linux"; break;
        case TC_OS_DARWIN:  os_tok = "darwin"; break;
        case TC_OS_IOS:     os_tok = "ios"; break;
        case TC_OS_ANDROID: os_tok = "linux"; break; /* android uses linux kernel; env conveys android */
        case TC_OS_FREEBSD: os_tok = "freebsd"; break;
        case TC_OS_NETBSD:  os_tok = "netbsd"; break;
        case TC_OS_OPENBSD: os_tok = "openbsd"; break;
        case TC_OS_WASI:    os_tok = "wasi"; break;
        default:            os_tok = "unknown"; break;
    }

    /* env token */
    const char* env_tok = NULL;
    switch (t->env) {
        case TC_ENV_MSVC:    env_tok = "msvc"; break;
        case TC_ENV_GNU:     env_tok = "gnu"; break;   /* mingw */
        case TC_ENV_UCRT:    env_tok = "ucrt"; break;  /* some toolchains use "gnu" + ucrt; keep explicit */
        case TC_ENV_GNUC:    env_tok = "gnu"; break;
        case TC_ENV_MUSL:    env_tok = "musl"; break;
        case TC_ENV_ANDROID: env_tok = "android"; break;
        case TC_ENV_MACABI:  env_tok = "macabi"; break;
        case TC_ENV_SIMULATOR: env_tok = "simulator"; break;
        case TC_ENV_WASI:    env_tok = "wasi"; break;
        default:             env_tok = NULL; break;
    }

    /* Construct arch-vendor-os[-env] */
    int n = 0;
    if (env_tok && *env_tok) {
        n = snprintf(t->triple, sizeof(t->triple), "%s-%s-%s-%s", arch, vend, os_tok, env_tok);
    } else {
        n = snprintf(t->triple, sizeof(t->triple), "%s-%s-%s", arch, vend, os_tok);
    }
    if (n < 0) return TC_TARGET_EPARSE;
    if ((size_t)n >= sizeof(t->triple)) return TC_TARGET_EOVERFLOW;
    return TC_TARGET_OK;
}

/* -----------------------------------------------------------------------------
 * Normalization & validation
 * -------------------------------------------------------------------------- */

static inline bool tc_target_is_known(const tc_target* t) {
    return t && t->arch != TC_ARCH_UNKNOWN && t->os != TC_OS_UNKNOWN;
}

static inline void tc_target_normalize(tc_target* t) {
    if (!t) return;

    if (!t->vendor[0]) {
        if (t->os == TC_OS_WINDOWS) strncpy(t->vendor, "pc", sizeof(t->vendor) - 1u);
        else if (t->os == TC_OS_DARWIN || t->os == TC_OS_IOS) strncpy(t->vendor, "apple", sizeof(t->vendor) - 1u);
        else strncpy(t->vendor, "unknown", sizeof(t->vendor) - 1u);
    }

    if (t->os == TC_OS_WINDOWS) {
        if (t->env == TC_ENV_UNKNOWN) t->env = TC_ENV_MSVC;
        if (t->env == TC_ENV_GNUC) t->env = TC_ENV_GNU;
    }

    if (t->os == TC_OS_LINUX) {
        if (t->env == TC_ENV_UNKNOWN) t->env = TC_ENV_GNUC;
        if (t->env == TC_ENV_GNU) t->env = TC_ENV_GNUC;
    }

    if (t->os == TC_OS_ANDROID) {
        t->env = TC_ENV_ANDROID;
    }

    if (t->os == TC_OS_WASI) {
        t->env = TC_ENV_WASI;
        if (!t->vendor[0]) strncpy(t->vendor, "unknown", sizeof(t->vendor) - 1u);
    }

    if (t->endian == TC_ENDIAN_UNKNOWN) {
        t->endian = TC_ENDIAN_LITTLE;
    }

    /* Keep abi_str in sync if abi is set and abi_str empty */
    if (!t->abi_str[0] && t->abi != TC_ABI_UNKNOWN && t->abi != TC_ABI_DEFAULT) {
        strncpy(t->abi_str, tc_abi_to_str(t->abi), sizeof(t->abi_str) - 1u);
    }
}

/* -----------------------------------------------------------------------------
 * Flag builders
 * -------------------------------------------------------------------------- */

typedef struct tc_clang_flags_s {
    /* Individual flags (no quotes). Empty string => not emitted. */
    char target_flag[TC_TARGET_MAX_STR];  /* "-target <triple>" */
    char march_flag[TC_TARGET_MAX_STR];   /* "-march=<arch>" */
    char mcpu_flag[TC_TARGET_MAX_STR];    /* "-mcpu=<cpu>" */
    char mabi_flag[TC_TARGET_MAX_STR];    /* "-mabi=<abi>" */
    char sysroot_flag[TC_TARGET_MAX_STR]; /* "--sysroot=<path>" */
} tc_clang_flags;

static inline void tc_clang_flags_zero(tc_clang_flags* f) {
    if (!f) return;
    memset(f, 0, sizeof(*f));
}

/* Build minimal -target and optional tuning flags. sysroot may be NULL/empty. */
static inline tc_target_err tc_target_to_clang_flags(const tc_target* t,
                                                     const char* sysroot,
                                                     tc_clang_flags* out) {
    if (!t || !out) return TC_TARGET_EINVAL;
    tc_clang_flags_zero(out);

    /* Ensure triple exists. If not, build from fields. */
    char triple_buf[TC_TARGET_TRIPLE_MAX];
    const char* triple = t->triple;

    if (!triple || !*triple) {
        tc_target tmp = *t;
        tc_target_err e = tc_target_build_triple(&tmp);
        if (e != TC_TARGET_OK) return e;
        strncpy(triple_buf, tmp.triple, sizeof(triple_buf) - 1u);
        triple_buf[sizeof(triple_buf) - 1u] = '\0';
        triple = triple_buf;
    }

    int n = snprintf(out->target_flag, sizeof(out->target_flag), "-target %s", triple);
    if (n < 0) return TC_TARGET_EPARSE;
    if ((size_t)n >= sizeof(out->target_flag)) return TC_TARGET_EOVERFLOW;

    /* -march (conservative). For clang, -march generally accepts LLVM arch names. */
    const char* march = NULL;
    switch (t->arch) {
        case TC_ARCH_X86_32:  march = "x86"; break;
        case TC_ARCH_X86_64:  march = "x86-64"; break;
        case TC_ARCH_ARMV7:   march = "armv7-a"; break;
        case TC_ARCH_AARCH64: march = "armv8-a"; break;
        case TC_ARCH_RISCV32: march = "rv32gc"; break;
        case TC_ARCH_RISCV64: march = "rv64gc"; break;
        case TC_ARCH_PPC64LE: march = "powerpc64le"; break;
        case TC_ARCH_WASM32:  march = "wasm32"; break;
        case TC_ARCH_WASM64:  march = "wasm64"; break;
        default:              march = NULL; break;
    }
    if (march) {
        n = snprintf(out->march_flag, sizeof(out->march_flag), "-march=%s", march);
        if (n < 0) return TC_TARGET_EPARSE;
        if ((size_t)n >= sizeof(out->march_flag)) return TC_TARGET_EOVERFLOW;
    }

    if (t->cpu[0]) {
        n = snprintf(out->mcpu_flag, sizeof(out->mcpu_flag), "-mcpu=%s", t->cpu);
        if (n < 0) return TC_TARGET_EPARSE;
        if ((size_t)n >= sizeof(out->mcpu_flag)) return TC_TARGET_EOVERFLOW;
    }

    /* ABI: prefer explicit abi_str if provided */
    const char* abi_s = (t->abi_str[0] ? t->abi_str : NULL);
    if (!abi_s && t->abi != TC_ABI_UNKNOWN && t->abi != TC_ABI_DEFAULT) {
        abi_s = tc_abi_to_str(t->abi);
    }
    if (abi_s && *abi_s && !tc_streq(abi_s, "default") && !tc_streq(abi_s, "unknown")) {
        n = snprintf(out->mabi_flag, sizeof(out->mabi_flag), "-mabi=%s", abi_s);
        if (n < 0) return TC_TARGET_EPARSE;
        if ((size_t)n >= sizeof(out->mabi_flag)) return TC_TARGET_EOVERFLOW;
    }

    if (sysroot && *sysroot) {
        n = snprintf(out->sysroot_flag, sizeof(out->sysroot_flag), "--sysroot=%s", sysroot);
        if (n < 0) return TC_TARGET_EPARSE;
        if ((size_t)n >= sizeof(out->sysroot_flag)) return TC_TARGET_EOVERFLOW;
    }

    return TC_TARGET_OK;
}

/* -----------------------------------------------------------------------------
 * Utility comparisons
 * -------------------------------------------------------------------------- */

static inline bool tc_target_eq(const tc_target* a, const tc_target* b) {
    if (!a || !b) return false;
    if (a->arch != b->arch) return false;
    if (a->os   != b->os)   return false;
    if (a->env  != b->env)  return false;
    if (a->abi  != b->abi)  return false;
    if (a->endian != b->endian) return false;
    if (strcmp(a->vendor, b->vendor) != 0) return false;
    if (strcmp(a->osver,  b->osver)  != 0) return false;
    if (strcmp(a->cpu,    b->cpu)    != 0) return false;
    if (strcmp(a->abi_str,b->abi_str)!= 0) return false;
    return true;
}

/* -----------------------------------------------------------------------------
 * High-level convenience: build host triple quickly
 * -------------------------------------------------------------------------- */

static inline tc_target_err tc_target_host_triple(char out_triple[TC_TARGET_TRIPLE_MAX]) {
    if (!out_triple) return TC_TARGET_EINVAL;
    tc_target t;
    tc_target_set_host(&t);
    tc_target_normalize(&t);
    tc_target_err e = tc_target_build_triple(&t);
    if (e != TC_TARGET_OK) return e;
    strncpy(out_triple, t.triple, TC_TARGET_TRIPLE_MAX - 1u);
    out_triple[TC_TARGET_TRIPLE_MAX - 1u] = '\0';
    return TC_TARGET_OK;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_TOOLCHAIN_CLANG_TARGET_H */
