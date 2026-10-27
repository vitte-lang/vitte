/*
 * Vitte Compiler
 * src/api/version.c
 *
 * Canonical version and build-information subsystem.
 *
 * ISO C17.
 *
 * This translation unit deliberately remains dependency-light so version
 * reporting continues to work even when the compiler cannot initialize.
 *
 * Responsibilities:
 *
 *   - Vitte semantic version
 *   - public API version
 *   - ABI version
 *   - build identity
 *   - build mode
 *   - host C compiler
 *   - host platform
 *   - host architecture
 *   - pointer width
 *   - byte order
 *   - C language standard
 *   - Git/build metadata
 *   - compile-time capabilities
 *   - deterministic textual formatting
 *   - runtime consistency validation
 *
 * Important distinction:
 *
 *   host platform/architecture
 *       describes the machine for which this Vitte executable was built.
 *
 *   Vitte compilation target
 *       belongs to the driver/backend/target subsystem and MUST NOT be
 *       inferred from this module.
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
#define VITTE_VERSION_PATCH 1
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

/*
 * Reproducible builds should override these macros from the build system,
 * preferably using SOURCE_DATE_EPOCH-derived values.
 *
 * Falling back to __DATE__/__TIME__ preserves useful information for local
 * developer builds but naturally makes those builds non-reproducible.
 */
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
/* Internal constants                                                        */
/* ========================================================================= */

#define VITTE_VERSION_STRING_CAPACITY ((size_t)256u)
#define VITTE_VERSION_FULL_CAPACITY   ((size_t)1024u)

#define VITTE_VERSION_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_VERSION_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_VERSION_BUILD_ID_SCHEMA \
    UINT64_C(2)

/* ========================================================================= */
/* Stringification                                                           */
/* ========================================================================= */

#define VITTE_VERSION_STRINGIFY_IMPL(value_) #value_
#define VITTE_VERSION_STRINGIFY(value_) \
    VITTE_VERSION_STRINGIFY_IMPL(value_)

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(
    sizeof(uint32_t) == 4u,
    "Vitte version subsystem requires 32-bit uint32_t"
);

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte version subsystem requires 64-bit uint64_t"
);

_Static_assert(
    VITTE_VERSION_MAJOR >= 0,
    "Vitte major version must be non-negative"
);

_Static_assert(
    VITTE_VERSION_MINOR >= 0,
    "Vitte minor version must be non-negative"
);

_Static_assert(
    VITTE_VERSION_PATCH >= 0,
    "Vitte patch version must be non-negative"
);

_Static_assert(
    VITTE_API_VERSION_MAJOR >= 0,
    "Vitte API major version must be non-negative"
);

_Static_assert(
    VITTE_API_VERSION_MINOR >= 0,
    "Vitte API minor version must be non-negative"
);

_Static_assert(
    VITTE_API_VERSION_PATCH >= 0,
    "Vitte API patch version must be non-negative"
);

_Static_assert(
    VITTE_ABI_VERSION_MAJOR >= 0,
    "Vitte ABI major version must be non-negative"
);

_Static_assert(
    VITTE_ABI_VERSION_MINOR >= 0,
    "Vitte ABI minor version must be non-negative"
);

_Static_assert(
    VITTE_ABI_VERSION_PATCH >= 0,
    "Vitte ABI patch version must be non-negative"
);

/* ========================================================================= */
/* Static version strings                                                    */
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
/* Internal string helpers                                                   */
/* ========================================================================= */

static bool
vitte_version_nonempty(
    const char *text)
{
    return
        text != NULL &&
        text[0] != '\0';
}

static size_t
vitte_version_strlen_bounded(
    const char *text,
    size_t maximum)
{
    size_t length;

    if (text == NULL) {
        return 0u;
    }

    length = 0u;

    while (length < maximum &&
           text[length] != '\0') {
        ++length;
    }

    return length;
}

/*
 * Append text to a fixed-size string.
 *
 * Returns true only when the complete input string was appended.
 * The destination remains NUL-terminated whenever capacity != 0.
 */
static bool
vitte_version_append(
    char *buffer,
    size_t capacity,
    size_t *length,
    const char *text)
{
    size_t text_length;
    size_t available;
    size_t copy_length;
    bool complete;

    if (buffer == NULL ||
        capacity == 0u ||
        length == NULL ||
        text == NULL) {
        return false;
    }

    if (*length >= capacity) {
        buffer[capacity - 1u] = '\0';
        return false;
    }

    available =
        capacity - 1u - *length;

    text_length =
        strlen(text);

    copy_length =
        text_length < available
            ? text_length
            : available;

    complete =
        copy_length == text_length;

    if (copy_length != 0u) {
        (void)memcpy(
            buffer + *length,
            text,
            copy_length);

        *length += copy_length;
    }

    buffer[*length] = '\0';

    return complete;
}

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
/* Host compiler detection                                                   */
/* ========================================================================= */

static const char *
vitte_version_detect_compiler(void)
{
    /*
     * Detection order matters because clang defines __GNUC__ for
     * compatibility, and Intel LLVM defines clang compatibility macros.
     */

#if defined(__INTEL_LLVM_COMPILER)
    return "intel-llvm";

#elif defined(__INTEL_COMPILER)
    return "intel";

#elif defined(__clang__)
    return "clang";

#elif defined(_MSC_VER)
    return "msvc";

#elif defined(__GNUC__)
    return "gcc";

#else
    return "unknown";
#endif
}

static const char *
vitte_version_detect_compiler_version(void)
{
#if defined(__INTEL_LLVM_COMPILER)
    return
        VITTE_VERSION_STRINGIFY(
            __INTEL_LLVM_COMPILER);

#elif defined(__INTEL_COMPILER)
    return
        VITTE_VERSION_STRINGIFY(
            __INTEL_COMPILER);

#elif defined(__clang_version__)
    return __clang_version__;

#elif defined(_MSC_FULL_VER)
    return
        VITTE_VERSION_STRINGIFY(
            _MSC_FULL_VER);

#elif defined(_MSC_VER)
    return
        VITTE_VERSION_STRINGIFY(
            _MSC_VER);

#elif defined(__VERSION__)
    return __VERSION__;

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
    /*
     * Android must precede generic Linux because Android toolchains normally
     * define __linux__ as well.
     *
     * Emscripten/WASI must also precede generic host checks.
     */

#if defined(__EMSCRIPTEN__)
    return "emscripten";

#elif defined(__wasi__)
    return "wasi";

#elif defined(__ANDROID__)
    return "android";

#elif defined(__APPLE__) && defined(__MACH__)
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

#elif defined(__AIX__)
    return "aix";

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
#if defined(__wasm64__)
    return "wasm64";

#elif defined(__wasm32__)
    return "wasm32";

#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";

#elif defined(__arm64ec__)
    return "arm64ec";

#elif defined(__x86_64__) || defined(_M_X64)
    return "x86_64";

#elif defined(__i386__) || defined(_M_IX86)
    return "x86";

#elif defined(__arm__) || defined(_M_ARM)
    return "arm";

#elif defined(__riscv) && \
      defined(__riscv_xlen) && \
      (__riscv_xlen == 64)
    return "riscv64";

#elif defined(__riscv) && \
      defined(__riscv_xlen) && \
      (__riscv_xlen == 32)
    return "riscv32";

#elif defined(__powerpc64__) || \
      defined(__ppc64__) || \
      defined(__PPC64__)
    return "powerpc64";

#elif defined(__powerpc__) || \
      defined(__ppc__) || \
      defined(__PPC__)
    return "powerpc";

#elif defined(__s390x__)
    return "s390x";

#elif defined(__s390__)
    return "s390";

#elif defined(__mips64) || \
      defined(__mips64__)
    return "mips64";

#elif defined(__mips__) || \
      defined(__mips)
    return "mips";

#elif defined(__loongarch64)
    return "loongarch64";

#elif defined(__sparc__) && \
      defined(__arch64__)
    return "sparc64";

#elif defined(__sparc__)
    return "sparc";

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

#elif defined(__BYTE_ORDER) && \
      defined(__LITTLE_ENDIAN) && \
      (__BYTE_ORDER == __LITTLE_ENDIAN)

    return VITTE_ENDIANNESS_LITTLE;

#elif defined(__BYTE_ORDER) && \
      defined(__BIG_ENDIAN) && \
      (__BYTE_ORDER == __BIG_ENDIAN)

    return VITTE_ENDIANNESS_BIG;

#elif defined(_WIN32)

    return VITTE_ENDIANNESS_LITTLE;

#else

    /*
     * Portable runtime fallback.
     *
     * Reading an object's representation through unsigned char is explicitly
     * permitted by C.
     */
    const uint16_t value =
        UINT16_C(0x0102);

    const unsigned char *bytes =
        (const unsigned char *)(const void *)&value;

    if (bytes[0] == 0x02u &&
        bytes[1] == 0x01u) {
        return VITTE_ENDIANNESS_LITTLE;
    }

    if (bytes[0] == 0x01u &&
        bytes[1] == 0x02u) {
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
        case VITTE_ENDIANNESS_UNKNOWN:
            return "unknown";

        case VITTE_ENDIANNESS_LITTLE:
            return "little";

        case VITTE_ENDIANNESS_BIG:
            return "big";
    }

    return "unknown";
}

/* ========================================================================= */
/* C language standard                                                       */
/* ========================================================================= */

static uint32_t
vitte_version_detect_c_standard(void)
{
#if defined(__STDC_VERSION__)

# if __STDC_VERSION__ >= 202311L
    return UINT32_C(2023);

# elif __STDC_VERSION__ >= 201710L
    return UINT32_C(2017);

# elif __STDC_VERSION__ >= 201112L
    return UINT32_C(2011);

# elif __STDC_VERSION__ >= 199901L
    return UINT32_C(1999);

# else
    return UINT32_C(1990);
# endif

#elif defined(_MSC_VER)

    /*
     * MSVC historically reported incomplete __STDC_VERSION__ information.
     * This value describes the minimum language model assumed by this
     * translation unit, not every optional MSVC conformance feature.
     */
    return UINT32_C(2011);

#else

    return UINT32_C(1990);

#endif
}

/* ========================================================================= */
/* Semantic version validation                                               */
/* ========================================================================= */

static bool
vitte_version_identifier_character_valid(
    unsigned char character)
{
    return
        (character >= (unsigned char)'0' &&
         character <= (unsigned char)'9') ||
        (character >= (unsigned char)'A' &&
         character <= (unsigned char)'Z') ||
        (character >= (unsigned char)'a' &&
         character <= (unsigned char)'z') ||
        character == (unsigned char)'-';
}

/*
 * Validate a SemVer prerelease/build identifier list.
 *
 * Empty text is accepted because prerelease/build metadata is optional.
 *
 * This intentionally validates the lexical SemVer shape. Numeric prerelease
 * leading-zero validation is handled separately below.
 */
static bool
vitte_version_identifier_list_valid(
    const char *text,
    bool enforce_numeric_leading_zero_rule)
{
    const char *segment;
    const char *cursor;

    if (text == NULL) {
        return false;
    }

    if (text[0] == '\0') {
        return true;
    }

    segment = text;
    cursor = text;

    for (;;) {
        unsigned char character =
            (unsigned char)*cursor;

        if (character == '\0' ||
            character == (unsigned char)'.') {
            size_t length =
                (size_t)(cursor - segment);

            size_t index;
            bool numeric;

            if (length == 0u) {
                return false;
            }

            numeric = true;

            for (index = 0u;
                 index < length;
                 ++index) {
                unsigned char item =
                    (unsigned char)segment[index];

                if (!vitte_version_identifier_character_valid(
                        item)) {
                    return false;
                }

                if (item < (unsigned char)'0' ||
                    item > (unsigned char)'9') {
                    numeric = false;
                }
            }

            if (enforce_numeric_leading_zero_rule &&
                numeric &&
                length > 1u &&
                segment[0] == '0') {
                return false;
            }

            if (character == '\0') {
                break;
            }

            segment = cursor + 1;
        } else if (!vitte_version_identifier_character_valid(
                       character)) {
            return false;
        }

        ++cursor;
    }

    return true;
}

/* ========================================================================= */
/* Cached semantic version                                                   */
/* ========================================================================= */

/*
 * The semantic version is compile-time constant data.
 *
 * A runtime buffer is retained because prerelease/build metadata are supplied
 * as independent build macros.
 *
 * Initialization is idempotent per thread. The resulting bytes are
 * deterministic and independent for concurrent API callers.
 *
 * No heap allocation is performed.
 */
const char *
vitte_version_string(void)
{
    /* The value is cached per thread so concurrent API users never race on
       the lazy formatting buffer.  It still has thread storage duration and
       remains valid until the calling thread exits. */
    static _Thread_local char buffer[VITTE_VERSION_STRING_CAPACITY];
    static _Thread_local bool initialized = false;

    if (!initialized) {
        size_t length;
        bool complete;

        buffer[0] = '\0';
        length = 0u;
        complete = true;

        complete =
            vitte_version_append(
                buffer,
                sizeof(buffer),
                &length,
                vitte_version_base_string) &&
            complete;

        if (vitte_version_nonempty(
                VITTE_VERSION_PRERELEASE)) {
            complete =
                vitte_version_append(
                    buffer,
                    sizeof(buffer),
                    &length,
                    "-") &&
                complete;

            complete =
                vitte_version_append(
                    buffer,
                    sizeof(buffer),
                    &length,
                    VITTE_VERSION_PRERELEASE) &&
                complete;
        }

        if (vitte_version_nonempty(
                VITTE_VERSION_BUILD_METADATA)) {
            complete =
                vitte_version_append(
                    buffer,
                    sizeof(buffer),
                    &length,
                    "+") &&
                complete;

            complete =
                vitte_version_append(
                    buffer,
                    sizeof(buffer),
                    &length,
                    VITTE_VERSION_BUILD_METADATA) &&
                complete;
        }

        /*
         * Capacity is intentionally large enough for normal build metadata.
         * Validation detects a truncated representation through independent
         * bounded checks.
         */
        (void)complete;

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
    return
        (uint32_t)VITTE_VERSION_MAJOR;
}

uint32_t
vitte_version_minor(void)
{
    return
        (uint32_t)VITTE_VERSION_MINOR;
}

uint32_t
vitte_version_patch(void)
{
    return
        (uint32_t)VITTE_VERSION_PATCH;
}

const char *
vitte_version_prerelease(void)
{
    return
        VITTE_VERSION_PRERELEASE;
}

const char *
vitte_version_build_metadata(void)
{
    return
        VITTE_VERSION_BUILD_METADATA;
}

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

uint32_t
vitte_version_api_major(void)
{
    return
        (uint32_t)VITTE_API_VERSION_MAJOR;
}

uint32_t
vitte_version_api_minor(void)
{
    return
        (uint32_t)VITTE_API_VERSION_MINOR;
}

uint32_t
vitte_version_api_patch(void)
{
    return
        (uint32_t)VITTE_API_VERSION_PATCH;
}

const char *
vitte_version_api_string(void)
{
    return
        vitte_api_version_string_value;
}

/* ========================================================================= */
/* ABI version                                                               */
/* ========================================================================= */

uint32_t
vitte_version_abi_major(void)
{
    return
        (uint32_t)VITTE_ABI_VERSION_MAJOR;
}

uint32_t
vitte_version_abi_minor(void)
{
    return
        (uint32_t)VITTE_ABI_VERSION_MINOR;
}

uint32_t
vitte_version_abi_patch(void)
{
    return
        (uint32_t)VITTE_ABI_VERSION_PATCH;
}

const char *
vitte_version_abi_string(void)
{
    return
        vitte_abi_version_string_value;
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
     * API compatibility contract:
     *
     *   requested major == runtime major
     *   requested minor <= runtime minor
     *
     * Patch releases are assumed to preserve API compatibility within a
     * major/minor line.
     */
    return
        major ==
            (uint32_t)VITTE_API_VERSION_MAJOR &&
        minor <=
            (uint32_t)VITTE_API_VERSION_MINOR;
}

bool
vitte_version_abi_compatible(
    uint32_t major,
    uint32_t minor)
{
    /*
     * ABI policy currently mirrors API policy.
     *
     * If Vitte later adopts stricter ABI semantics, change this function
     * independently rather than coupling callers to API compatibility.
     */
    return
        major ==
            (uint32_t)VITTE_ABI_VERSION_MAJOR &&
        minor <=
            (uint32_t)VITTE_ABI_VERSION_MINOR;
}

/* ========================================================================= */
/* Build information                                                         */
/* ========================================================================= */

const char *
vitte_version_project_name(void)
{
    return
        VITTE_VERSION_PROJECT_NAME;
}

const char *
vitte_version_compiler_name(void)
{
    return
        VITTE_VERSION_COMPILER_NAME;
}

const char *
vitte_version_build_mode(void)
{
    return
        vitte_version_detect_build_mode();
}

const char *
vitte_version_host_compiler(void)
{
    return
        vitte_version_detect_compiler();
}

const char *
vitte_version_host_compiler_version(void)
{
    return
        vitte_version_detect_compiler_version();
}

const char *
vitte_version_platform(void)
{
    return
        vitte_version_detect_platform();
}

const char *
vitte_version_architecture(void)
{
    return
        vitte_version_detect_architecture();
}

uint32_t
vitte_version_pointer_bits(void)
{
    const size_t bits =
        sizeof(void *) * (size_t)8u;

    if (bits > UINT32_MAX) {
        return UINT32_MAX;
    }

    return (uint32_t)bits;
}

vitte_endianness_t
vitte_version_endianness(void)
{
    return
        vitte_version_detect_endianness();
}

uint32_t
vitte_version_c_standard(void)
{
    return
        vitte_version_detect_c_standard();
}

/* ========================================================================= */
/* Git metadata                                                              */
/* ========================================================================= */

const char *
vitte_version_git_commit(void)
{
    return
        VITTE_VERSION_GIT_COMMIT;
}

const char *
vitte_version_git_branch(void)
{
    return
        VITTE_VERSION_GIT_BRANCH;
}

bool
vitte_version_git_dirty(void)
{
    return
        VITTE_VERSION_GIT_DIRTY != 0;
}

/* ========================================================================= */
/* Build timestamp                                                           */
/* ========================================================================= */

const char *
vitte_version_build_date(void)
{
    return
        VITTE_VERSION_BUILD_DATE;
}

const char *
vitte_version_build_time(void)
{
    return
        VITTE_VERSION_BUILD_TIME;
}

/* ========================================================================= */
/* Feature capabilities                                                      */
/* ========================================================================= */

bool
vitte_version_has_feature(
    vitte_version_feature_t feature)
{
    switch (feature) {
        case VITTE_VERSION_FEATURE_NONE:
            return false;

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
#if defined(VITTE_DISABLE_DIAGNOSTICS) || \
    defined(VITTE_DISABLE_JSON_DIAGNOSTICS)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_SARIF:
#if defined(VITTE_DISABLE_DIAGNOSTICS) || \
    defined(VITTE_DISABLE_SARIF)
            return false;
#else
            return true;
#endif

        case VITTE_VERSION_FEATURE_LSP:
#if defined(VITTE_DISABLE_DIAGNOSTICS) || \
    defined(VITTE_DISABLE_LSP)
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

        case VITTE_VERSION_FEATURE_COUNT:
            return false;
    }

    return false;
}

const char *
vitte_version_feature_name(
    vitte_version_feature_t feature)
{
    switch (feature) {
        case VITTE_VERSION_FEATURE_NONE:
            return "none";

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

        case VITTE_VERSION_FEATURE_COUNT:
            return "invalid";
    }

    return "invalid";
}

/* ========================================================================= */
/* Feature count                                                             */
/* ========================================================================= */

size_t
vitte_version_feature_count(void)
{
    size_t count;
    unsigned feature;

    count = 0u;

    for (feature =
             (unsigned)VITTE_VERSION_FEATURE_NONE + 1u;
         feature <
             (unsigned)VITTE_VERSION_FEATURE_COUNT;
         ++feature) {
        if (vitte_version_has_feature(
                (vitte_version_feature_t)feature)) {
            ++count;
        }
    }

    return count;
}

/* ========================================================================= */
/* Version structure                                                         */
/* ========================================================================= */

vitte_version_info_t
vitte_version_info(void)
{
    vitte_version_info_t info;

    memset(
        &info,
        0,
        sizeof(info));

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
/* Full version string                                                       */
/* ========================================================================= */

const char *
vitte_version_full_string(void)
{
    /* Keep the human-readable cache reentrant for concurrent API callers. */
    static _Thread_local char buffer[VITTE_VERSION_FULL_CAPACITY];
    static _Thread_local bool initialized = false;

    if (!initialized) {
        size_t length;
        bool complete;

        buffer[0] = '\0';
        length = 0u;
        complete = true;

#define VITTE_VERSION_APPEND(value_) \
        do { \
            complete = \
                vitte_version_append( \
                    buffer, \
                    sizeof(buffer), \
                    &length, \
                    (value_)) && \
                complete; \
        } while (0)

        VITTE_VERSION_APPEND(
            VITTE_VERSION_COMPILER_NAME);

        VITTE_VERSION_APPEND(" ");

        VITTE_VERSION_APPEND(
            vitte_version_string());

        VITTE_VERSION_APPEND(" (");

        VITTE_VERSION_APPEND(
            vitte_version_platform());

        VITTE_VERSION_APPEND("/");

        VITTE_VERSION_APPEND(
            vitte_version_architecture());

        VITTE_VERSION_APPEND(", ");

        VITTE_VERSION_APPEND(
            vitte_version_build_mode());

        VITTE_VERSION_APPEND(", ");

        VITTE_VERSION_APPEND(
            vitte_version_host_compiler());

        VITTE_VERSION_APPEND(" ");

        VITTE_VERSION_APPEND(
            vitte_version_host_compiler_version());

        VITTE_VERSION_APPEND(")");

        if (vitte_version_nonempty(
                VITTE_VERSION_GIT_COMMIT)) {
            VITTE_VERSION_APPEND(" git:");

            VITTE_VERSION_APPEND(
                VITTE_VERSION_GIT_COMMIT);

            if (VITTE_VERSION_GIT_DIRTY != 0) {
                VITTE_VERSION_APPEND("-dirty");
            }
        }

#undef VITTE_VERSION_APPEND

        (void)complete;

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

    /*
     * snprintf(NULL, 0, ...) is defined by C99/C17 and is useful for callers
     * querying the required size.
     *
     * A nonzero capacity with NULL buffer would be invalid, so normalize it
     * to a pure size query.
     */
    if (buffer == NULL) {
        capacity = 0u;
    }

    written =
        snprintf(
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
/* Deterministic hashing                                                     */
/* ========================================================================= */

static uint64_t
vitte_version_hash_byte(
    uint64_t hash,
    unsigned char byte)
{
    hash ^= (uint64_t)byte;
    hash *= VITTE_VERSION_FNV_PRIME;

    return hash;
}

static uint64_t
vitte_version_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned shift;

    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        hash =
            vitte_version_hash_byte(
                hash,
                (unsigned char)(
                    (value >> shift) &
                    UINT64_C(0xff)));
    }

    return hash;
}

static uint64_t
vitte_version_hash_string(
    uint64_t hash,
    const char *text)
{
    const unsigned char *cursor;

    if (text == NULL) {
        return
            vitte_version_hash_u64(
                hash,
                UINT64_MAX);
    }

    cursor =
        (const unsigned char *)
            (const void *)text;

    while (*cursor != 0u) {
        hash =
            vitte_version_hash_byte(
                hash,
                *cursor);

        ++cursor;
    }

    /*
     * Explicit terminator prevents tuple concatenation ambiguity.
     */
    return
        vitte_version_hash_byte(
            hash,
            0u);
}

/* ========================================================================= */
/* Deterministic build identity                                              */
/* ========================================================================= */

uint64_t
vitte_version_build_id(void)
{
    uint64_t hash;
    unsigned feature;
    uint32_t pointer_bits;
    vitte_endianness_t endianness;
    uint32_t c_standard;

    pointer_bits = vitte_version_pointer_bits();
    endianness = vitte_version_endianness();
    c_standard = vitte_version_c_standard();

    hash =
        VITTE_VERSION_FNV_OFFSET;

    hash =
        vitte_version_hash_u64(
            hash,
            VITTE_VERSION_BUILD_ID_SCHEMA);

    hash =
        vitte_version_hash_string(
            hash,
            VITTE_VERSION_PROJECT_NAME);

    hash =
        vitte_version_hash_string(
            hash,
            VITTE_VERSION_COMPILER_NAME);

    hash =
        vitte_version_hash_string(
            hash,
            vitte_version_string());

    hash =
        vitte_version_hash_string(
            hash,
            vitte_version_api_string());

    hash =
        vitte_version_hash_string(
            hash,
            vitte_version_abi_string());

    hash =
        vitte_version_hash_string(
            hash,
            vitte_version_platform());

    hash =
        vitte_version_hash_string(
            hash,
            vitte_version_architecture());

    hash =
        vitte_version_hash_u64(
            hash,
            (uint64_t)pointer_bits);

    hash =
        vitte_version_hash_u64(
            hash,
            (uint64_t)endianness);

    hash =
        vitte_version_hash_u64(
            hash,
            (uint64_t)c_standard);

    hash =
        vitte_version_hash_string(
            hash,
            vitte_version_build_mode());

    hash =
        vitte_version_hash_string(
            hash,
            VITTE_VERSION_GIT_COMMIT);

    hash =
        vitte_version_hash_u64(
            hash,
            VITTE_VERSION_GIT_DIRTY != 0
                ? UINT64_C(1)
                : UINT64_C(0));

    /*
     * Capabilities are part of build identity. Two compiler binaries with
     * identical semantic versions but different enabled subsystems are not
     * the same build.
     */
    for (feature =
             (unsigned)VITTE_VERSION_FEATURE_NONE + 1u;
         feature <
             (unsigned)VITTE_VERSION_FEATURE_COUNT;
         ++feature) {
        hash =
            vitte_version_hash_u64(
                hash,
                (uint64_t)feature);

        hash =
            vitte_version_hash_u64(
                hash,
                vitte_version_has_feature(
                    (vitte_version_feature_t)feature)
                    ? UINT64_C(1)
                    : UINT64_C(0));
    }

    /*
     * Build date/time are intentionally excluded.
     *
     * This allows reproducible binaries to retain the same build identity
     * when all semantic/compiler inputs are identical.
     */

    return hash;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_version_validate(void)
{
    vitte_version_info_t info;
    const char *version;
    size_t expected_minimum_length;
    size_t actual_length;

    info =
        vitte_version_info();

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

    if (info.build_mode == NULL ||
        info.build_mode[0] == '\0') {
        return false;
    }

    if (info.host_compiler == NULL ||
        info.host_compiler[0] == '\0') {
        return false;
    }

    if (info.host_compiler_version == NULL ||
        info.host_compiler_version[0] == '\0') {
        return false;
    }

    if (info.platform == NULL ||
        info.platform[0] == '\0') {
        return false;
    }

    if (info.architecture == NULL ||
        info.architecture[0] == '\0') {
        return false;
    }

    if (info.pointer_bits == 0u ||
        (info.pointer_bits % 8u) != 0u) {
        return false;
    }

    if (info.endianness !=
            VITTE_ENDIANNESS_UNKNOWN &&
        info.endianness !=
            VITTE_ENDIANNESS_LITTLE &&
        info.endianness !=
            VITTE_ENDIANNESS_BIG) {
        return false;
    }

    if (info.c_standard != 1990u &&
        info.c_standard != 1999u &&
        info.c_standard != 2011u &&
        info.c_standard != 2017u &&
        info.c_standard != 2023u) {
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

    if (!vitte_version_identifier_list_valid(
            VITTE_VERSION_PRERELEASE,
            true)) {
        return false;
    }

    if (!vitte_version_identifier_list_valid(
            VITTE_VERSION_BUILD_METADATA,
            false)) {
        return false;
    }

    /*
     * Detect semantic-version buffer truncation without constructing a second
     * complete temporary version string.
     */
    expected_minimum_length =
        strlen(vitte_version_base_string);

    if (vitte_version_nonempty(
            VITTE_VERSION_PRERELEASE)) {
        if (expected_minimum_length >
            SIZE_MAX - 1u -
            strlen(VITTE_VERSION_PRERELEASE)) {
            return false;
        }

        expected_minimum_length +=
            1u +
            strlen(VITTE_VERSION_PRERELEASE);
    }

    if (vitte_version_nonempty(
            VITTE_VERSION_BUILD_METADATA)) {
        if (expected_minimum_length >
            SIZE_MAX - 1u -
            strlen(VITTE_VERSION_BUILD_METADATA)) {
            return false;
        }

        expected_minimum_length +=
            1u +
            strlen(VITTE_VERSION_BUILD_METADATA);
    }

    version =
        vitte_version_string();

    actual_length =
        vitte_version_strlen_bounded(
            version,
            VITTE_VERSION_STRING_CAPACITY);

    if (actual_length !=
        expected_minimum_length) {
        return false;
    }

    /*
     * Diagnostic-derived capabilities cannot exist when diagnostics
     * themselves are disabled.
     */
    if (!vitte_version_has_feature(
            VITTE_VERSION_FEATURE_DIAGNOSTICS)) {
        if (vitte_version_has_feature(
                VITTE_VERSION_FEATURE_JSON_DIAGNOSTICS) ||
            vitte_version_has_feature(
                VITTE_VERSION_FEATURE_SARIF) ||
            vitte_version_has_feature(
                VITTE_VERSION_FEATURE_LSP)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Architectural contract                                                   */
/* ========================================================================= */

/*
 * HOST VS TARGET
 * --------------
 *
 * Functions in this module report properties of the Vitte executable itself:
 *
 *     vitte_version_platform()
 *     vitte_version_architecture()
 *     vitte_version_pointer_bits()
 *     vitte_version_endianness()
 *
 * They do NOT describe:
 *
 *     --target
 *     generated C ABI
 *     target pointer width
 *     target endianness
 *     linker target
 *     execution environment
 *
 * Those belong to the target/backend/driver layers.
 *
 *
 * VERSION VS API VS ABI
 * ---------------------
 *
 * Vitte semantic version:
 *
 *     user-visible compiler/toolchain release.
 *
 * API version:
 *
 *     compatibility of public programmatic interfaces.
 *
 * ABI version:
 *
 *     compatibility of binary-facing interfaces.
 *
 * These values are intentionally independent.
 *
 *
 * BUILD IDENTITY
 * --------------
 *
 * vitte_version_build_id() is deterministic and excludes wall-clock build
 * timestamps. It includes:
 *
 *     project/compiler identity
 *     semantic version
 *     API/ABI versions
 *     host platform/architecture
 *     pointer width
 *     byte order
 *     C standard
 *     build mode
 *     Git commit
 *     dirty state
 *     enabled feature set
 *
 * The schema number is hashed first so the identity contract can evolve
 * without silently colliding with an older scheme.
 *
 *
 * REPRODUCIBLE BUILDS
 * -------------------
 *
 * __DATE__ and __TIME__ remain fallback metadata only.
 *
 * Production/release packaging should define:
 *
 *     VITTE_VERSION_BUILD_DATE
 *     VITTE_VERSION_BUILD_TIME
 *     VITTE_VERSION_GIT_COMMIT
 *     VITTE_VERSION_GIT_BRANCH
 *     VITTE_VERSION_GIT_DIRTY
 *
 * from deterministic build-system inputs.
 *
 *
 * DEPENDENCY POLICY
 * -----------------
 *
 * Keep this module independent from:
 *
 *     AST
 *     source manager
 *     diagnostics engine
 *     parser
 *     HIR
 *     IR
 *     backend implementation
 *     filesystem initialization
 *     allocator initialization
 *
 * Version reporting must remain available when those systems fail.
 */

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
