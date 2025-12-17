/*
 * toolchain/config/src/clang_target.c
 *
 * Vitte toolchain – target triple modeling, parsing, normalization, and clang flag emission.
 *
 * Provides:
 *  - tc_target: structured {arch, vendor, os, abi, env, triple string}
 *  - host detection (best-effort from compiler macros)
 *  - triple parse/build (simple but robust enough for typical Clang triples)
 *  - mapping to clang flags: --target=, --sysroot=, -march/-mcpu/-mabi where relevant
 *
 * No heap allocations. All strings fixed-size.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"

/* -----------------------------------------------------------------------------
 * String utils
 * -------------------------------------------------------------------------- */

static size_t tc_strlcpy(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return 0;
    if (!src) src = "";
    size_t n = 0;
    while (src[n] && n + 1 < cap) {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
    while (src[n]) n++;
    return n;
}

static bool tc_streq_ci(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        char ca = (char)tolower((unsigned char)*a);
        char cb = (char)tolower((unsigned char)*b);
        if (ca != cb) return false;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

static bool tc_startswith_ci(const char* s, const char* pfx) {
    if (!s || !pfx) return false;
    while (*pfx) {
        if (!*s) return false;
        char cs = (char)tolower((unsigned char)*s++);
        char cp = (char)tolower((unsigned char)*pfx++);
        if (cs != cp) return false;
    }
    return true;
}

static bool tc_is_empty(const char* s) { return !s || s[0] == '\0'; }

/* Split by '-' into up to N tokens. Returns count. */
static size_t tc_split_dash(char* s, char* parts[], size_t max_parts) {
    size_t n = 0;
    if (!s || !parts || max_parts == 0) return 0;

    parts[n++] = s;
    for (char* p = s; *p && n < max_parts; ++p) {
        if (*p == '-') {
            *p = '\0';
            parts[n++] = p + 1;
        }
    }
    return n;
}

/* -----------------------------------------------------------------------------
 * Enum string mapping
 * -------------------------------------------------------------------------- */

static tc_arch tc_arch_from_str(const char* s) {
    if (!s) return TC_ARCH_UNKNOWN;
    if (tc_streq_ci(s, "x86_64") || tc_streq_ci(s, "amd64")) return TC_ARCH_X86_64;
    if (tc_streq_ci(s, "i386") || tc_streq_ci(s, "i686") || tc_streq_ci(s, "x86")) return TC_ARCH_X86_32;
    if (tc_streq_ci(s, "aarch64") || tc_streq_ci(s, "arm64")) return TC_ARCH_AARCH64;
    if (tc_streq_ci(s, "armv7") || tc_streq_ci(s, "armv7a") || tc_streq_ci(s, "arm")) return TC_ARCH_ARMV7;
    if (tc_streq_ci(s, "riscv64")) return TC_ARCH_RISCV64;
    if (tc_streq_ci(s, "wasm32")) return TC_ARCH_WASM32;
    if (tc_streq_ci(s, "wasm64")) return TC_ARCH_WASM64;
    return TC_ARCH_UNKNOWN;
}

static const char* tc_arch_to_str(tc_arch a) {
    switch (a) {
        case TC_ARCH_X86_64:  return "x86_64";
        case TC_ARCH_X86_32:  return "i386";
        case TC_ARCH_AARCH64: return "aarch64";
        case TC_ARCH_ARMV7:   return "armv7";
        case TC_ARCH_RISCV64: return "riscv64";
        case TC_ARCH_WASM32:  return "wasm32";
        case TC_ARCH_WASM64:  return "wasm64";
        default:              return "unknown";
    }
}

static tc_vendor tc_vendor_from_str(const char* s) {
    if (!s) return TC_VENDOR_UNKNOWN;
    if (tc_streq_ci(s, "pc")) return TC_VENDOR_PC;
    if (tc_streq_ci(s, "apple")) return TC_VENDOR_APPLE;
    if (tc_streq_ci(s, "unknown")) return TC_VENDOR_UNKNOWN;
    return TC_VENDOR_UNKNOWN;
}

static const char* tc_vendor_to_str(tc_vendor v) {
    switch (v) {
        case TC_VENDOR_PC:     return "pc";
        case TC_VENDOR_APPLE:  return "apple";
        case TC_VENDOR_UNKNOWN:
        default:               return "unknown";
    }
}

static tc_os tc_os_from_str(const char* s) {
    if (!s) return TC_OS_UNKNOWN;
    if (tc_startswith_ci(s, "windows") || tc_streq_ci(s, "win32")) return TC_OS_WINDOWS;
    if (tc_startswith_ci(s, "linux")) return TC_OS_LINUX;
    if (tc_startswith_ci(s, "darwin") || tc_startswith_ci(s, "macos")) return TC_OS_DARWIN;
    if (tc_startswith_ci(s, "ios")) return TC_OS_IOS;
    if (tc_startswith_ci(s, "android")) return TC_OS_ANDROID;
    if (tc_startswith_ci(s, "freebsd")) return TC_OS_FREEBSD;
    if (tc_startswith_ci(s, "wasm")) return TC_OS_WASM;
    return TC_OS_UNKNOWN;
}

static const char* tc_os_to_str(tc_os o) {
    switch (o) {
        case TC_OS_WINDOWS: return "windows";
        case TC_OS_LINUX:   return "linux";
        case TC_OS_DARWIN:  return "darwin";
        case TC_OS_IOS:     return "ios";
        case TC_OS_ANDROID: return "android";
        case TC_OS_FREEBSD: return "freebsd";
        case TC_OS_WASM:    return "unknown"; /* wasm triples usually encode wasm32-unknown-unknown */
        default:            return "unknown";
    }
}

static tc_abi tc_abi_from_str(const char* s) {
    if (!s) return TC_ABI_UNKNOWN;
    if (tc_streq_ci(s, "gnu")) return TC_ABI_GNU;
    if (tc_streq_ci(s, "gnueabihf")) return TC_ABI_GNUEABIHF;
    if (tc_streq_ci(s, "msvc")) return TC_ABI_MSVC;
    if (tc_streq_ci(s, "musl")) return TC_ABI_MUSL;
    if (tc_streq_ci(s, "eabi")) return TC_ABI_EABI;
    if (tc_streq_ci(s, "android")) return TC_ABI_ANDROID;
    return TC_ABI_UNKNOWN;
}

static const char* tc_abi_to_str(tc_abi a) {
    switch (a) {
        case TC_ABI_GNU:       return "gnu";
        case TC_ABI_GNUEABIHF: return "gnueabihf";
        case TC_ABI_MSVC:      return "msvc";
        case TC_ABI_MUSL:      return "musl";
        case TC_ABI_EABI:      return "eabi";
        case TC_ABI_ANDROID:   return "android";
        default:               return "";
    }
}

/* -----------------------------------------------------------------------------
 * Host detection
 * -------------------------------------------------------------------------- */

void tc_target_zero(tc_target* t) {
    if (!t) return;
    memset(t, 0, sizeof(*t));
}

void tc_target_set_host(tc_target* out) {
    if (!out) return;
    tc_target_zero(out);

    /* arch */
#if defined(__x86_64__) || defined(_M_X64)
    out->arch = TC_ARCH_X86_64;
#elif defined(__i386__) || defined(_M_IX86)
    out->arch = TC_ARCH_X86_32;
#elif defined(__aarch64__) || defined(_M_ARM64)
    out->arch = TC_ARCH_AARCH64;
#elif defined(__arm__) || defined(_M_ARM)
    out->arch = TC_ARCH_ARMV7;
#elif defined(__riscv) && (__riscv_xlen == 64)
    out->arch = TC_ARCH_RISCV64;
#elif defined(__wasm32__)
    out->arch = TC_ARCH_WASM32;
#elif defined(__wasm64__)
    out->arch = TC_ARCH_WASM64;
#else
    out->arch = TC_ARCH_UNKNOWN;
#endif

    /* os + vendor/abi */
#if defined(_WIN32)
    out->os = TC_OS_WINDOWS;
    out->vendor = TC_VENDOR_PC;
# if defined(_MSC_VER)
    out->abi = TC_ABI_MSVC;
# else
    out->abi = TC_ABI_GNU;
# endif
#elif defined(__APPLE__) && defined(__MACH__)
    out->os = TC_OS_DARWIN;
    out->vendor = TC_VENDOR_APPLE;
    out->abi = TC_ABI_UNKNOWN;
#elif defined(__linux__)
    out->os = TC_OS_LINUX;
    out->vendor = TC_VENDOR_PC;
# if defined(__musl__)
    out->abi = TC_ABI_MUSL;
# else
    out->abi = TC_ABI_GNU;
# endif
#elif defined(__FreeBSD__)
    out->os = TC_OS_FREEBSD;
    out->vendor = TC_VENDOR_UNKNOWN;
    out->abi = TC_ABI_UNKNOWN;
#else
    out->os = TC_OS_UNKNOWN;
    out->vendor = TC_VENDOR_UNKNOWN;
    out->abi = TC_ABI_UNKNOWN;
#endif

    (void)tc_target_build_triple(out);
}

bool tc_target_is_known(const tc_target* t) {
    if (!t) return false;
    return t->arch != TC_ARCH_UNKNOWN && t->os != TC_OS_UNKNOWN;
}

/* -----------------------------------------------------------------------------
 * Parse triple (best-effort)
 * -------------------------------------------------------------------------- */

tc_target_err tc_target_parse_triple(tc_target* out, const char* triple) {
    if (!out || !triple || !*triple) return TC_TARGET_EINVAL;

    tc_target_zero(out);
    tc_strlcpy(out->triple, sizeof(out->triple), triple);

    /* work on a local copy for splitting */
    char tmp[TC_TARGET_TRIPLE_MAX];
    tc_strlcpy(tmp, sizeof(tmp), triple);

    char* parts[5] = {0};
    size_t n = tc_split_dash(tmp, parts, 5);

    if (n < 2) return TC_TARGET_EPARSE;

    /* Standard: arch-vendor-os[-abi] (but many variants) */
    out->arch = tc_arch_from_str(parts[0]);
    out->vendor = (n >= 2) ? tc_vendor_from_str(parts[1]) : TC_VENDOR_UNKNOWN;

    /* OS can be in parts[2], sometimes vendor is omitted => arch-os-abi */
    if (n >= 3) {
        out->os = tc_os_from_str(parts[2]);
    }

    if (out->os == TC_OS_UNKNOWN && n >= 2) {
        /* try arch-os-abi: treat parts[1] as os */
        out->os = tc_os_from_str(parts[1]);
        out->vendor = TC_VENDOR_UNKNOWN;
    }

    /* abi typically last */
    if (n >= 4) {
        out->abi = tc_abi_from_str(parts[n - 1]);
    } else {
        out->abi = TC_ABI_UNKNOWN;
    }

    return TC_TARGET_OK;
}

/* -----------------------------------------------------------------------------
 * Normalize: fill missing vendor defaults, ABI heuristics
 * -------------------------------------------------------------------------- */

void tc_target_normalize(tc_target* t) {
    if (!t) return;

    if (t->vendor == TC_VENDOR_UNKNOWN) {
        if (t->os == TC_OS_DARWIN || t->os == TC_OS_IOS) t->vendor = TC_VENDOR_APPLE;
        else if (t->os == TC_OS_WINDOWS || t->os == TC_OS_LINUX) t->vendor = TC_VENDOR_PC;
    }

    if (t->abi == TC_ABI_UNKNOWN) {
        if (t->os == TC_OS_WINDOWS) t->abi = TC_ABI_MSVC; /* common default */
        else if (t->os == TC_OS_ANDROID) t->abi = TC_ABI_ANDROID;
        else if (t->os == TC_OS_LINUX) t->abi = TC_ABI_GNU;
    }

    (void)tc_target_build_triple(t);
}

/* -----------------------------------------------------------------------------
 * Build triple string
 * -------------------------------------------------------------------------- */

const char* tc_target_build_triple(tc_target* t) {
    if (!t) return "";

    const char* a = tc_arch_to_str(t->arch);
    const char* v = tc_vendor_to_str(t->vendor);
    const char* o = tc_os_to_str(t->os);
    const char* abi = tc_abi_to_str(t->abi);

    char buf[TC_TARGET_TRIPLE_MAX];
    buf[0] = '\0';

    if (!tc_is_empty(abi)) {
        (void)snprintf(buf, sizeof(buf), "%s-%s-%s-%s", a, v, o, abi);
    } else {
        (void)snprintf(buf, sizeof(buf), "%s-%s-%s", a, v, o);
    }

    tc_strlcpy(t->triple, sizeof(t->triple), buf);
    return t->triple;
}

/* -----------------------------------------------------------------------------
 * Emit clang flags (owned strings)
 * -------------------------------------------------------------------------- */

tc_target_err tc_target_to_clang_flags(const tc_target* t,
                                       const char* sysroot,
                                       tc_clang_flags* out) {
    if (!t || !out) return TC_TARGET_EINVAL;
    memset(out, 0, sizeof(*out));

    if (!tc_target_is_known(t)) return TC_TARGET_EUNSUPPORTED;

    tc_target tmp = *t;
    tc_target_normalize(&tmp);

    /* --target=triple */
    (void)snprintf(out->target_flag, sizeof(out->target_flag), "--target=%s", tmp.triple);

    /* sysroot */
    if (sysroot && *sysroot) {
        (void)snprintf(out->sysroot_flag, sizeof(out->sysroot_flag), "--sysroot=%s", sysroot);
    }

    /* arch-specific: set minimal -march/-mcpu/-mabi only where it helps */
    switch (tmp.arch) {
        case TC_ARCH_ARMV7:
            (void)snprintf(out->march_flag, sizeof(out->march_flag), "-march=armv7-a");
            break;
        case TC_ARCH_AARCH64:
            /* keep empty by default */
            break;
        case TC_ARCH_RISCV64:
            (void)snprintf(out->march_flag, sizeof(out->march_flag), "-march=rv64gc");
            break;
        default:
            break;
    }

    /* abi hint for ARM hardfloat */
    if (tmp.abi == TC_ABI_GNUEABIHF) {
        (void)snprintf(out->mabi_flag, sizeof(out->mabi_flag), "-mfloat-abi=hard");
    }

    return TC_TARGET_OK;
}
