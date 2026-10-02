/*
 * Vitte Compiler
 * src/api/version.c
 *
 * Version and build-information subsystem.
 *
 * This module provides a single authoritative runtime view of:
 *
 *   - Vitte semantic version
 *   - public API version
 *   - ABI version
 *   - build mode
 *   - host compiler
 *   - host platform
 *   - host architecture
 *   - pointer width
 *   - byte order
 *   - C language standard
 *   - build metadata
 *   - feature capabilities
 *
 * Keep this translation unit dependency-light. Version reporting must remain
 * available even when the rest of the compiler cannot initialize.
 */

#include "version.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

#ifndef VITTE_VERSION_MAJOR
#define VITTE_VERSION_MAJOR 0
#endif

#ifndef VITTE_VERSION_MINOR
#define VITTE_VERSION_MINOR 1
#endif

#ifndef VITTE_VERSION_PATCH
#define VITTE_VERSION_PATCH 0
#endif

#ifndef VITTE_VERSION_PRERELEASE
#define VITTE_VERSION_PRERELEASE ""
#endif

#ifndef VITTE_VERSION_BUILD_METADATA
#define VITTE_VERSION_BUILD_METADATA ""
#endif

#ifndef VITTE_API_VERSION_MAJOR
#define VITTE_API_VERSION_MAJOR 1
#endif

#ifndef VITTE_API_VERSION_MINOR
#define VITTE_API_VERSION_MINOR 0
#endif

#ifndef VITTE_API_VERSION_PATCH
#define VITTE_API_VERSION_PATCH 0
#endif

#ifndef VITTE_ABI_VERSION_MAJOR
#define VITTE_ABI_VERSION_MAJOR 1
#endif

#ifndef VITTE_ABI_VERSION_MINOR
#define VITTE_ABI_VERSION_MINOR 0
#endif

#ifndef VITTE_ABI_VERSION_PATCH
#define VITTE_ABI_VERSION_PATCH 0
#endif

#ifndef VITTE_VERSION_GIT_COMMIT
#define VITTE_VERSION_GIT_COMMIT ""
#endif

#ifndef VITTE_VERSION_GIT_BRANCH
#define VITTE_VERSION_GIT_BRANCH ""
#endif

#ifndef VITTE_VERSION_GIT_DIRTY
#define VITTE_VERSION_GIT_DIRTY 0
#endif

#ifndef VITTE_VERSION_BUILD_DATE
#define VITTE_VERSION_BUILD_DATE __DATE__
#endif

#ifndef VITTE_VERSION_BUILD_TIME
#define VITTE_VERSION_BUILD_TIME __TIME__
#endif

#ifndef VITTE_VERSION_PROJECT_NAME
#define VITTE_VERSION_PROJECT_NAME "Vitte"
#endif

#ifndef VITTE_VERSION_COMPILER_NAME
#define VITTE_VERSION_COMPILER_NAME "vitte"
#endif

/* ========================================================================= */
/* Stringification                                                           */
/* ========================================================================= */

#define VITTE_VERSION_STRINGIFY_IMPL(value) #value
#define VITTE_VERSION_STRINGIFY(value) \
    VITTE_VERSION_STRINGIFY_IMPL(value)

/* ========================================================================= */
/* Semantic version                                                          */
/* ========================================================================= */

static const char vitte_version_base_string[] =
    VITTE_VERSION_STRINGIFY(VITTE_VERSION_MAJOR)
    "."
    VITTE_VERSION_STRINGIFY(VITTE_VERSION_MINOR)
    "."
    VITTE_VERSION_STRINGIFY(VITTE_VERSION_PATCH);

static const char vitte_api_version_string_value[] =
    VITTE_VERSION_STRINGIFY(VITTE_API_VERSION_MAJOR)
    "."
    VITTE_VERSION_STRINGIFY(VITTE_API_VERSION_MINOR)
    "."
    VITTE_VERSION_STRINGIFY(VITTE_API_VERSION_PATCH);

static const char vitte_abi_version_string_value[] =
    VITTE_VERSION_STRINGIFY(VITTE_ABI_VERSION_MAJOR)
    "."
    VITTE_VERSION_STRINGIFY(VITTE_ABI_VERSION_MINOR)
    "."
    VITTE_VERSION_STRINGIFY(VITTE_ABI_VERSION_PATCH);

/* ========================================================================= */
/* Build mode                                                                */
/* ========================================================================= */

static const char *
vitte_version_detect_build_mode(void)
{
#if defined(VITTE_BUILD_MODE)
    return VITTE_BUILD_MODE;
#elif defined(NDEBUG)
    return "release";
#else
    return "debug";
#endif
}

/* ========================================================================= */
/* Compiler detection                                                        */
/* ========================================================================= */

static const char *
vitte_version_detect_compiler(void)
{
#if defined(__clang__)
    return "clang";
#elif defined(__GNUC__)
    return "gcc";
#elif defined(_MSC_VER)
    return "msvc";
#elif defined(__INTEL_LLVM_COMPILER)
    return "intel-llvm";
#elif defined(__INTEL_COMPILER)
    return "intel";
#else
    return "unknown";
#endif
}

static const char *
vitte_version_detect_compiler_version(void)
{
#if defined(__clang_version__)
    return __clang_version__;
#elif defined(__GNUC__)
    return __VERSION__;
#elif defined(_MSC_FULL_VER)
    return VITTE_VERSION_STRINGIFY(_MSC_FULL_VER);
#elif defined(_MSC_VER)
    return VITTE_VERSION_STRINGIFY(_MSC_VER);
#elif defined(__INTEL_LLVM_COMPILER)
    return VITTE_VERSION_STRINGIFY(
        __INTEL_LLVM_COMPILER);
#elif defined(__INTEL_COMPILER)
    return VITTE_VERSION_STRINGIFY(
        __INTEL_COMPILER);
#else
    return "unknown";
#endif
}

/* ========================================================================= */
/* Platform detection                                                        */
/* ========================================================================= */

static const char *
vitte_version_detect_platform(void)
{
#if defined(__APPLE__) && defined(__MACH__)
    return "darwin";
#elif defined(__linux__)
    return "linux";
#elif defined(_WIN32)
    return "windows";
#elif defined(__FreeBSD__)
    return "freebsd";
#elif defined(__OpenBSD__)
    return "openbsd";
#elif defined(__NetBSD__)
    return "netbsd";
#elif defined(__DragonFly__)
    return "dragonfly";
#elif defined(__sun) && defined(__SVR4)
    return "solaris";
#elif defined(__HAIKU__)
    return "haiku";
#elif defined(__ANDROID__)
    return "android";
#elif defined(__EMSCRIPTEN__)
    return "emscripten";
#elif defined(__wasi__)
    return "wasi";
#else
    return "unknown";
#endif
}

/* ========================================================================= */
/* Architecture detection                                                    */
/* ========================================================================= */

static const char *
vitte_version_detect_architecture(void)
{
#if defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#elif defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#elif defined(__riscv) && (__riscv_xlen == 64)
    return "riscv64";
#elif defined(__riscv) && (__riscv_xlen == 32)
    return "riscv32";
#elif defined(__powerpc64__) || defined(__ppc64__)
    return "powerpc64";
#elif defined(__powerpc__) || defined(__ppc__)
    return "powerpc";
#elif defined(__s390x__)
    return "s390x";
#elif defined(__s390__)
    return "s390";
#elif defined(__mips64)
    return "mips64";
#elif defined(__mips__)
    return "mips";
#elif defined(__loongarch64)
    return "loongarch64";
#elif defined(__wasm64__)
    return "wasm64";
#elif defined(__wasm32__)
    return "wasm32";
#else
    return "unknown";
#endif
}

/* ========================================================================= */
/* Endianness                                                                */
/* ========================================================================= */

static vitte_endianness_t
vitte_version_detect_endianness(void)
{
#if defined(__BYTE_ORDER__) && \
    defined(__ORDER_LITTLE_ENDIAN__) && \
    (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

    return VITTE_ENDIANNESS_LITTLE;

#elif defined(__BYTE_ORDER__) && \
      defined(__ORDER_BIG_ENDIAN__) && \
      (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

    return VITTE_ENDIANNESS_BIG;

#elif defined(_WIN32)

    return VITTE_ENDIANNESS_LITTLE;

#else
    /*
     * Runtime fallback for platforms where the compiler does not expose
     * byte-order macros.
     */
    const uint16_t value = UINT16_C(0x0102);
    const unsigned char *bytes;

    bytes = (const unsigned char *)&value;

    if (bytes[0] == 0x02u) {
        return VITTE_ENDIANNESS_LITTLE;
    }

    if (bytes[0] == 0x01u) {
        return VITTE_ENDIANNESS_BIG;
    }

    return VITTE_ENDIANNESS_UNKNOWN;
#endif
}

const char *
vitte_endianness_name(
    vitte_endianness_t endianness)
{
    switch (endianness) {
        case VITTE_ENDIANNESS_LITTLE:
            return "little";

        case VITTE_ENDIANNESS_BIG:
            return "big";

        case VITTE_ENDIANNESS_UNKNOWN:
        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* C standard                                                                */
/* ========================================================================= */

static uint32_t
vitte_version_detect_c_standard(void)
{
#if defined(__STDC_VERSION__)
# if __STDC_VERSION__ >= 202311L
    return 2023u;
# elif __STDC_VERSION__ >= 201710L
    return 2017u;
# elif __STDC_VERSION__ >= 201112L
    return 2011u;
# elif __STDC_VERSION__ >= 199901L
    return 1999u;
# else
    return 1990u;
# endif
#elif defined(_MSC_VER)
    return 2011u;
#else
    return 1990u;
#endif
}

/* ========================================================================= */
/* Version formatting                                                        */
/* ========================================================================= */

static bool
vitte_version_nonempty(
    const char *text)
{
    return text != NULL &&
           text[0] != '\0';
}

static void
vitte_version_append(
    char *buffer,
    size_t capacity,
    size_t *length,
    const char *text)
{
    size_t text_length;
    size_t available;
    size_t copy_length;

    if (buffer == NULL ||
        capacity == 0u ||
        length == NULL ||
        text == NULL) {
        return;
    }

    if (*length >= capacity - 1u) {
        return;
    }

    text_length = strlen(text);
    available = capacity - 1u - *length;

    copy_length =
        text_length < available
            ? text_length
            : available;

    if (copy_length != 0u) {
        memcpy(
            buffer + *length,
            text,
            copy_length);

        *length += copy_length;
    }

    buffer[*length] = '\0';
}

/* ========================================================================= */
/* Cached semantic version                                                   */
/* ========================================================================= */

const char *
vitte_version_string(void)
{
    static char buffer[256];
    static bool initialized = false;

    if (!initialized) {
        size_t length;

        buffer[0] = '\0';
        length = 0u;

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            vitte_version_base_string);

        if (vitte_version_nonempty(
                VITTE_VERSION_PRERELEASE)) {
            vitte_version_append(
                buffer,
                sizeof(buffer),
                &length,
                "-");

            vitte_version_append(
                buffer,
                sizeof(buffer),
                &length,
                VITTE_VERSION_PRERELEASE);
        }

        if (vitte_version_nonempty(
                VITTE_VERSION_BUILD_METADATA)) {
            vitte_version_append(
                buffer,
                sizeof(buffer),
                &length,
                "+");

            vitte_version_append(
                buffer,
                sizeof(buffer),
                &length,
                VITTE_VERSION_BUILD_METADATA);
        }

        initialized = true;
    }

    return buffer;
}

/* ========================================================================= */
/* Semantic version access                                                   */
/* ========================================================================= */

uint32_t
vitte_version_major(void)
{
    return (uint32_t)VITTE_VERSION_MAJOR;
}

uint32_t
vitte_version_minor(void)
{
    return (uint32_t)VITTE_VERSION_MINOR;
}

uint32_t
vitte_version_patch(void)
{
    return (uint32_t)VITTE_VERSION_PATCH;
}

const char *
vitte_version_prerelease(void)
{
    return VITTE_VERSION_PRERELEASE;
}

const char *
vitte_version_build_metadata(void)
{
    return VITTE_VERSION_BUILD_METADATA;
}

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

uint32_t
vitte_version_api_major(void)
{
    return (uint32_t)VITTE_API_VERSION_MAJOR;
}

uint32_t
vitte_version_api_minor(void)
{
    return (uint32_t)VITTE_API_VERSION_MINOR;
}

uint32_t
vitte_version_api_patch(void)
{
    return (uint32_t)VITTE_API_VERSION_PATCH;
}

const char *
vitte_version_api_string(void)
{
    return vitte_api_version_string_value;
}

/* ========================================================================= */
/* ABI version                                                               */
/* ========================================================================= */

uint32_t
vitte_version_abi_major(void)
{
    return (uint32_t)VITTE_ABI_VERSION_MAJOR;
}

uint32_t
vitte_version_abi_minor(void)
{
    return (uint32_t)VITTE_ABI_VERSION_MINOR;
}

uint32_t
vitte_version_abi_patch(void)
{
    return (uint32_t)VITTE_ABI_VERSION_PATCH;
}

const char *
vitte_version_abi_string(void)
{
    return vitte_abi_version_string_value;
}

/* ========================================================================= */
/* Compatibility                                                             */
/* ========================================================================= */

bool
vitte_version_api_compatible(
    uint32_t major,
    uint32_t minor)
{
    /*
     * Semantic API compatibility:
     *
     *   - major must match exactly
     *   - runtime minor must be at least the requested minor
     */
    return major ==
               (uint32_t)VITTE_API_VERSION_MAJOR &&
           (uint32_t)VITTE_API_VERSION_MINOR >=
               minor;
}

bool
vitte_version_abi_compatible(
    uint32_t major,
    uint32_t minor)
{
    return major ==
               (uint32_t)VITTE_ABI_VERSION_MAJOR &&
           (uint32_t)VITTE_ABI_VERSION_MINOR >=
               minor;
}

/* ========================================================================= */
/* Build information                                                         */
/* ========================================================================= */

const char *
vitte_version_project_name(void)
{
    return VITTE_VERSION_PROJECT_NAME;
}

const char *
vitte_version_compiler_name(void)
{
    return VITTE_VERSION_COMPILER_NAME;
}

const char *
vitte_version_build_mode(void)
{
    return vitte_version_detect_build_mode();
}

const char *
vitte_version_host_compiler(void)
{
    return vitte_version_detect_compiler();
}

const char *
vitte_version_host_compiler_version(void)
{
    return vitte_version_detect_compiler_version();
}

const char *
vitte_version_platform(void)
{
    return vitte_version_detect_platform();
}

const char *
vitte_version_architecture(void)
{
    return vitte_version_detect_architecture();
}

uint32_t
vitte_version_pointer_bits(void)
{
    return (uint32_t)(sizeof(void *) * 8u);
}

vitte_endianness_t
vitte_version_endianness(void)
{
    return vitte_version_detect_endianness();
}

uint32_t
vitte_version_c_standard(void)
{
    return vitte_version_detect_c_standard();
}

/* ========================================================================= */
/* Git metadata                                                              */
/* ========================================================================= */

const char *
vitte_version_git_commit(void)
{
    return VITTE_VERSION_GIT_COMMIT;
}

const char *
vitte_version_git_branch(void)
{
    return VITTE_VERSION_GIT_BRANCH;
}

bool
vitte_version_git_dirty(void)
{
    return VITTE_VERSION_GIT_DIRTY != 0;
}

/* ========================================================================= */
/* Build timestamp                                                           */
/* ========================================================================= */

const char *
vitte_version_build_date(void)
{
    return VITTE_VERSION_BUILD_DATE;
}

const char *
vitte_version_build_time(void)
{
    return VITTE_VERSION_BUILD_TIME;
}

/* ========================================================================= */
/* Capabilities                                                              */
/* ========================================================================= */

bool
vitte_version_has_feature(
    vitte_version_feature_t feature)
{
    switch (feature) {
        case VITTE_VERSION_FEATURE_C17_BACKEND:
#if defined(VITTE_DISABLE_C17_BACKEND)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_DIAGNOSTICS:
#if defined(VITTE_DISABLE_DIAGNOSTICS)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_JSON_DIAGNOSTICS:
#if defined(VITTE_DISABLE_JSON_DIAGNOSTICS)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_SARIF:
#if defined(VITTE_DISABLE_SARIF)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_LSP:
#if defined(VITTE_DISABLE_LSP)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_ASYNC:
#if defined(VITTE_DISABLE_ASYNC)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_FFI:
#if defined(VITTE_DISABLE_FFI)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_COMPTIME:
#if defined(VITTE_DISABLE_COMPTIME)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_CONTRACTS:
#if defined(VITTE_DISABLE_CONTRACTS)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_MACROS:
#if defined(VITTE_DISABLE_MACROS)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_UNSAFE:
#if defined(VITTE_DISABLE_UNSAFE)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_TESTS:
#if defined(VITTE_DISABLE_TESTS)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_NONE:
        case VITTE_VERSION_FEATURE_COUNT:
        default:
            return false;
    }
}

const char *
vitte_version_feature_name(
    vitte_version_feature_t feature)
{
    switch (feature) {
        case VITTE_VERSION_FEATURE_C17_BACKEND:
            return "c17-backend";

        case VITTE_VERSION_FEATURE_DIAGNOSTICS:
            return "diagnostics";

        case VITTE_VERSION_FEATURE_JSON_DIAGNOSTICS:
            return "json-diagnostics";

        case VITTE_VERSION_FEATURE_SARIF:
            return "sarif";

        case VITTE_VERSION_FEATURE_LSP:
            return "lsp";

        case VITTE_VERSION_FEATURE_ASYNC:
            return "async";

        case VITTE_VERSION_FEATURE_FFI:
            return "ffi";

        case VITTE_VERSION_FEATURE_COMPTIME:
            return "comptime";

        case VITTE_VERSION_FEATURE_CONTRACTS:
            return "contracts";

        case VITTE_VERSION_FEATURE_MACROS:
            return "macros";

        case VITTE_VERSION_FEATURE_UNSAFE:
            return "unsafe";

        case VITTE_VERSION_FEATURE_TESTS:
            return "tests";

        case VITTE_VERSION_FEATURE_NONE:
            return "none";

        case VITTE_VERSION_FEATURE_COUNT:
        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Version structure                                                         */
/* ========================================================================= */

vitte_version_info_t
vitte_version_info(void)
{
    vitte_version_info_t info;

    memset(&info, 0, sizeof(info));

    info.version_major =
        (uint32_t)VITTE_VERSION_MAJOR;

    info.version_minor =
        (uint32_t)VITTE_VERSION_MINOR;

    info.version_patch =
        (uint32_t)VITTE_VERSION_PATCH;

    info.api_major =
        (uint32_t)VITTE_API_VERSION_MAJOR;

    info.api_minor =
        (uint32_t)VITTE_API_VERSION_MINOR;

    info.api_patch =
        (uint32_t)VITTE_API_VERSION_PATCH;

    info.abi_major =
        (uint32_t)VITTE_ABI_VERSION_MAJOR;

    info.abi_minor =
        (uint32_t)VITTE_ABI_VERSION_MINOR;

    info.abi_patch =
        (uint32_t)VITTE_ABI_VERSION_PATCH;

    info.version =
        vitte_version_string();

    info.prerelease =
        VITTE_VERSION_PRERELEASE;

    info.build_metadata =
        VITTE_VERSION_BUILD_METADATA;

    info.project_name =
        VITTE_VERSION_PROJECT_NAME;

    info.compiler_name =
        VITTE_VERSION_COMPILER_NAME;

    info.build_mode =
        vitte_version_build_mode();

    info.host_compiler =
        vitte_version_host_compiler();

    info.host_compiler_version =
        vitte_version_host_compiler_version();

    info.platform =
        vitte_version_platform();

    info.architecture =
        vitte_version_architecture();

    info.pointer_bits =
        vitte_version_pointer_bits();

    info.endianness =
        vitte_version_endianness();

    info.c_standard =
        vitte_version_c_standard();

    info.git_commit =
        VITTE_VERSION_GIT_COMMIT;

    info.git_branch =
        VITTE_VERSION_GIT_BRANCH;

    info.git_dirty =
        VITTE_VERSION_GIT_DIRTY != 0;

    info.build_date =
        VITTE_VERSION_BUILD_DATE;

    info.build_time =
        VITTE_VERSION_BUILD_TIME;

    return info;
}

/* ========================================================================= */
/* Full build string                                                         */
/* ========================================================================= */

const char *
vitte_version_full_string(void)
{
    static char buffer[1024];
    static bool initialized = false;

    if (!initialized) {
        size_t length;

        buffer[0] = '\0';
        length = 0u;

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            VITTE_VERSION_COMPILER_NAME);

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            " ");

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            vitte_version_string());

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            " (");

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            vitte_version_platform());

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            "/");

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            vitte_version_architecture());

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            ", ");

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            vitte_version_build_mode());

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            ", ");

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            vitte_version_host_compiler());

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            " ");

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            vitte_version_host_compiler_version());

        vitte_version_append(
            buffer,
            sizeof(buffer),
            &length,
            ")");

        if (vitte_version_nonempty(
                VITTE_VERSION_GIT_COMMIT)) {
            vitte_version_append(
                buffer,
                sizeof(buffer),
                &length,
                " git:");

            vitte_version_append(
                buffer,
                sizeof(buffer),
                &length,
                VITTE_VERSION_GIT_COMMIT);

            if (VITTE_VERSION_GIT_DIRTY != 0) {
                vitte_version_append(
                    buffer,
                    sizeof(buffer),
                    &length,
                    "-dirty");
            }
        }

        initialized = true;
    }

    return buffer;
}

/* ========================================================================= */
/* Detailed formatting                                                       */
/* ========================================================================= */

size_t
vitte_version_format(
    char *buffer,
    size_t capacity)
{
    int written;

    written = snprintf(
        buffer,
        capacity,
        "%s %s\n"
        "API: %s\n"
        "ABI: %s\n"
        "build: %s\n"
        "platform: %s\n"
        "architecture: %s\n"
        "pointer-width: %u\n"
        "endianness: %s\n"
        "host-compiler: %s %s\n"
        "C-standard: C%u\n"
        "git-commit: %s\n"
        "git-branch: %s\n"
        "git-dirty: %s\n"
        "build-date: %s\n"
        "build-time: %s",
        VITTE_VERSION_COMPILER_NAME,
        vitte_version_string(),
        vitte_version_api_string(),
        vitte_version_abi_string(),
        vitte_version_build_mode(),
        vitte_version_platform(),
        vitte_version_architecture(),
        (unsigned)vitte_version_pointer_bits(),
        vitte_endianness_name(
            vitte_version_endianness()),
        vitte_version_host_compiler(),
        vitte_version_host_compiler_version(),
        (unsigned)vitte_version_c_standard(),
        vitte_version_nonempty(
            VITTE_VERSION_GIT_COMMIT)
                ? VITTE_VERSION_GIT_COMMIT
                : "unknown",
        vitte_version_nonempty(
            VITTE_VERSION_GIT_BRANCH)
                ? VITTE_VERSION_GIT_BRANCH
                : "unknown",
        VITTE_VERSION_GIT_DIRTY != 0
            ? "yes"
            : "no",
        VITTE_VERSION_BUILD_DATE,
        VITTE_VERSION_BUILD_TIME);

    if (written < 0) {
        if (buffer != NULL &&
            capacity != 0u) {
            buffer[0] = '\0';
        }

        return 0u;
    }

    return (size_t)written;
}

/* ========================================================================= */
/* Deterministic build identity                                              */
/* ========================================================================= */

uint64_t
vitte_version_build_id(void)
{
    const char *parts[] = {
        VITTE_VERSION_PROJECT_NAME,
        vitte_version_string(),
        vitte_version_api_string(),
        vitte_version_abi_string(),
        vitte_version_platform(),
        vitte_version_architecture(),
        vitte_version_build_mode(),
        VITTE_VERSION_GIT_COMMIT
    };

    uint64_t hash;
    size_t part_index;

    hash = UINT64_C(14695981039346656037);

    for (part_index = 0u;
         part_index <
             sizeof(parts) / sizeof(parts[0]);
         ++part_index) {
        const unsigned char *text;

        text =
            (const unsigned char *)
                parts[part_index];

        while (*text != 0u) {
            hash ^= (uint64_t)*text++;
            hash *= UINT64_C(1099511628211);
        }

        /*
         * Separator prevents accidental concatenation ambiguity:
         *
         *     ["ab", "c"] != ["a", "bc"]
         */
        hash ^= UINT64_C(0xff);
        hash *= UINT64_C(1099511628211);
    }

    return hash;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_version_validate(void)
{
    vitte_version_info_t info;

    info = vitte_version_info();

    if (info.version == NULL ||
        info.version[0] == '\0') {
        return false;
    }

    if (info.project_name == NULL ||
        info.project_name[0] == '\0') {
        return false;
    }

    if (info.compiler_name == NULL ||
        info.compiler_name[0] == '\0') {
        return false;
    }

    if (info.pointer_bits != 32u &&
        info.pointer_bits != 64u &&
        info.pointer_bits != 128u) {
        return false;
    }

    if (info.endianness !=
            VITTE_ENDIANNESS_LITTLE &&
        info.endianness !=
            VITTE_ENDIANNESS_BIG &&
        info.endianness !=
            VITTE_ENDIANNESS_UNKNOWN) {
        return false;
    }

    if (strcmp(
            vitte_version_api_string(),
            vitte_api_version_string_value) != 0) {
        return false;
    }

    if (strcmp(
            vitte_version_abi_string(),
            vitte_abi_version_string_value) != 0) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    VITTE_VERSION_MAJOR >= 0,
    "Vitte major version must be non-negative");

_Static_assert(
    VITTE_VERSION_MINOR >= 0,
    "Vitte minor version must be non-negative");

_Static_assert(
    VITTE_VERSION_PATCH >= 0,
    "Vitte patch version must be non-negative");

_Static_assert(
    VITTE_API_VERSION_MAJOR >= 0,
    "Vitte API major version must be non-negative");

_Static_assert(
    VITTE_ABI_VERSION_MAJOR >= 0,
    "Vitte ABI major version must be non-negative");

_Static_assert(
    sizeof(uint32_t) == 4u,
    "Vitte version subsystem requires 32-bit uint32_t");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte version subsystem requires 64-bit uint64_t");

#endif

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
