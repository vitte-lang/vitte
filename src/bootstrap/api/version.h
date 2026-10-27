#ifndef VITTE_API_VERSION_H
#define VITTE_API_VERSION_H

/*
 * Vitte Compiler
 * src/api/version.h
 *
 * Canonical public version and build-information API.
 *
 * This interface exposes immutable information describing the Vitte compiler
 * executable itself:
 *
 *   - semantic version
 *   - public API version
 *   - ABI version
 *   - project/compiler identity
 *   - build mode
 *   - host compiler
 *   - host platform
 *   - host architecture
 *   - pointer width
 *   - byte order
 *   - C language standard
 *   - Git metadata
 *   - build timestamp
 *   - compiled capabilities
 *   - deterministic build identity
 *
 * IMPORTANT
 * ---------
 *
 * Host information exposed here describes the Vitte executable.
 *
 * It does NOT describe the compilation target selected through the driver or
 * backend subsystem.
 *
 * This header intentionally remains dependency-light and may be used by the
 * CLI, API, packaging tools, tests and embedding applications without pulling
 * in the compiler frontend.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Default version configuration                                             */
/* ========================================================================= */

/*
 * Build systems may define these before compiling version.c.
 *
 * They are repeated here so clients compiling against the public API observe
 * the same default version policy when no generated configuration header is
 * present.
 */

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

#ifndef VITTE_VERSION_PROJECT_NAME
#define VITTE_VERSION_PROJECT_NAME "Vitte"
#endif

#ifndef VITTE_VERSION_COMPILER_NAME
#define VITTE_VERSION_COMPILER_NAME "vitte"
#endif

/* ========================================================================= */
/* Endianness                                                                */
/* ========================================================================= */

/*
 * Host byte order of the Vitte executable.
 *
 * UNKNOWN exists for unusual targets where the byte order cannot be
 * determined reliably.
 */
typedef enum vitte_endianness {
    VITTE_ENDIANNESS_UNKNOWN = 0,
    VITTE_ENDIANNESS_LITTLE,
    VITTE_ENDIANNESS_BIG
} vitte_endianness_t;

/*
 * Return a stable lowercase name:
 *
 *     unknown
 *     little
 *     big
 *
 * Invalid enum values return "unknown".
 */
const char *
vitte_endianness_name(
    vitte_endianness_t endianness
);

/* ========================================================================= */
/* Features                                                                  */
/* ========================================================================= */

/*
 * Capabilities compiled into this Vitte executable.
 *
 * NONE and COUNT are sentinels and never represent enabled capabilities.
 *
 * Keep the order stable when practical because feature values may appear in
 * logs, tests, telemetry-free build metadata and deterministic build identity
 * calculations.
 */
typedef enum vitte_version_feature {
    VITTE_VERSION_FEATURE_NONE = 0,

    VITTE_VERSION_FEATURE_C17_BACKEND,
    VITTE_VERSION_FEATURE_DIAGNOSTICS,
    VITTE_VERSION_FEATURE_JSON_DIAGNOSTICS,
    VITTE_VERSION_FEATURE_SARIF,
    VITTE_VERSION_FEATURE_LSP,
    VITTE_VERSION_FEATURE_ASYNC,
    VITTE_VERSION_FEATURE_FFI,
    VITTE_VERSION_FEATURE_COMPTIME,
    VITTE_VERSION_FEATURE_CONTRACTS,
    VITTE_VERSION_FEATURE_MACROS,
    VITTE_VERSION_FEATURE_UNSAFE,
    VITTE_VERSION_FEATURE_TESTS,

    VITTE_VERSION_FEATURE_COUNT
} vitte_version_feature_t;

/*
 * Return true when feature is compiled into this executable.
 *
 * Invalid feature values, NONE and COUNT return false.
 */
bool
vitte_version_has_feature(
    vitte_version_feature_t feature
);

/*
 * Return a stable lowercase capability name.
 *
 * Examples:
 *
 *     c17-backend
 *     diagnostics
 *     json-diagnostics
 *     contracts
 *
 * NONE returns "none".
 * COUNT and invalid values return "invalid".
 */
const char *
vitte_version_feature_name(
    vitte_version_feature_t feature
);

/*
 * Return the number of currently enabled capabilities.
 *
 * NONE and COUNT are excluded.
 */
size_t
vitte_version_feature_count(void);

/* ========================================================================= */
/* Complete version information                                              */
/* ========================================================================= */

/*
 * Immutable snapshot of compiler/build information.
 *
 * All string pointers refer to immutable thread-local storage or build-defined
 * string literals. The caller must not free or modify them and must copy a
 * thread-local result before sharing it with another thread.
 *
 * platform/architecture/pointer_bits/endianness describe the host executable,
 * not the selected Vitte compilation target.
 */
typedef struct vitte_version_info {
    uint32_t version_major;
    uint32_t version_minor;
    uint32_t version_patch;

    uint32_t api_major;
    uint32_t api_minor;
    uint32_t api_patch;

    uint32_t abi_major;
    uint32_t abi_minor;
    uint32_t abi_patch;

    const char *version;
    const char *prerelease;
    const char *build_metadata;

    const char *project_name;
    const char *compiler_name;

    const char *build_mode;

    const char *host_compiler;
    const char *host_compiler_version;

    const char *platform;
    const char *architecture;

    uint32_t pointer_bits;
    vitte_endianness_t endianness;
    uint32_t c_standard;

    const char *git_commit;
    const char *git_branch;
    bool git_dirty;

    const char *build_date;
    const char *build_time;
} vitte_version_info_t;

/* ========================================================================= */
/* Vitte semantic version                                                    */
/* ========================================================================= */

/*
 * Semantic-version components.
 */
uint32_t
vitte_version_major(void);

uint32_t
vitte_version_minor(void);

uint32_t
vitte_version_patch(void);

/*
 * Optional SemVer prerelease component without the leading '-'.
 *
 * Returns an empty string when absent.
 *
 * Example:
 *
 *     "alpha.1"
 */
const char *
vitte_version_prerelease(void);

/*
 * Optional SemVer build metadata without the leading '+'.
 *
 * Returns an empty string when absent.
 *
 * Example:
 *
 *     "linux.x86_64"
 */
const char *
vitte_version_build_metadata(void);

/*
 * Return the complete semantic version:
 *
 *     MAJOR.MINOR.PATCH
 *     MAJOR.MINOR.PATCH-prerelease
 *     MAJOR.MINOR.PATCH+metadata
 *     MAJOR.MINOR.PATCH-prerelease+metadata
 *
 * The returned pointer refers to immutable per-thread internal storage and
 * must not be modified or freed.  A caller must copy it before using it from
 * another thread.
 */
const char *
vitte_version_string(void);

/* ========================================================================= */
/* Public API version                                                        */
/* ========================================================================= */

uint32_t
vitte_version_api_major(void);

uint32_t
vitte_version_api_minor(void);

uint32_t
vitte_version_api_patch(void);

/*
 * Return API version as:
 *
 *     MAJOR.MINOR.PATCH
 */
const char *
vitte_version_api_string(void);

/*
 * Return true when this runtime API can satisfy a client requiring:
 *
 *     major.minor
 *
 * Current compatibility policy:
 *
 *     runtime.major == requested.major
 *     runtime.minor >= requested.minor
 *
 * Patch versions are assumed compatible within a major/minor API line.
 */
bool
vitte_version_api_compatible(
    uint32_t major,
    uint32_t minor
);

/* ========================================================================= */
/* ABI version                                                               */
/* ========================================================================= */

uint32_t
vitte_version_abi_major(void);

uint32_t
vitte_version_abi_minor(void);

uint32_t
vitte_version_abi_patch(void);

/*
 * Return ABI version as:
 *
 *     MAJOR.MINOR.PATCH
 */
const char *
vitte_version_abi_string(void);

/*
 * Return true when this runtime ABI can satisfy a client requiring:
 *
 *     major.minor
 *
 * Current policy:
 *
 *     runtime.major == requested.major
 *     runtime.minor >= requested.minor
 */
bool
vitte_version_abi_compatible(
    uint32_t major,
    uint32_t minor
);

/* ========================================================================= */
/* Identity                                                                  */
/* ========================================================================= */

/*
 * Human-facing project name.
 *
 * Default:
 *
 *     "Vitte"
 */
const char *
vitte_version_project_name(void);

/*
 * Command/tool identity.
 *
 * Default:
 *
 *     "vitte"
 */
const char *
vitte_version_compiler_name(void);

/* ========================================================================= */
/* Build mode                                                                */
/* ========================================================================= */

/*
 * Return the compiler build mode.
 *
 * Normally:
 *
 *     debug
 *     release
 *
 * A build may override the value through VITTE_BUILD_MODE.
 */
const char *
vitte_version_build_mode(void);

/* ========================================================================= */
/* Host C compiler                                                           */
/* ========================================================================= */

/*
 * Return the compiler family used to build this Vitte executable.
 *
 * Stable values may include:
 *
 *     clang
 *     gcc
 *     msvc
 *     intel
 *     intel-llvm
 *     unknown
 */
const char *
vitte_version_host_compiler(void);

/*
 * Return the host C compiler version string.
 */
const char *
vitte_version_host_compiler_version(void);

/* ========================================================================= */
/* Host platform                                                             */
/* ========================================================================= */

/*
 * Return the operating-system/platform family of this executable.
 *
 * Stable values may include:
 *
 *     darwin
 *     linux
 *     windows
 *     freebsd
 *     openbsd
 *     netbsd
 *     dragonfly
 *     android
 *     solaris
 *     haiku
 *     aix
 *     emscripten
 *     wasi
 *     unknown
 */
const char *
vitte_version_platform(void);

/*
 * Return the host architecture.
 *
 * Stable values may include:
 *
 *     aarch64
 *     arm64ec
 *     x86_64
 *     x86
 *     arm
 *     riscv64
 *     riscv32
 *     powerpc64
 *     powerpc
 *     s390x
 *     s390
 *     mips64
 *     mips
 *     loongarch64
 *     sparc64
 *     sparc
 *     wasm64
 *     wasm32
 *     unknown
 */
const char *
vitte_version_architecture(void);

/*
 * Return host pointer width in bits.
 */
uint32_t
vitte_version_pointer_bits(void);

/*
 * Return host byte order.
 */
vitte_endianness_t
vitte_version_endianness(void);

/*
 * Return the detected C language standard as a calendar-style number:
 *
 *     1990
 *     1999
 *     2011
 *     2017
 *     2023
 *
 * This describes the C compilation environment used for this translation
 * unit. It is unrelated to the C standard selected for generated backend
 * output.
 */
uint32_t
vitte_version_c_standard(void);

/* ========================================================================= */
/* Git metadata                                                              */
/* ========================================================================= */

/*
 * Return build-provided Git commit identifier.
 *
 * Empty string means unavailable.
 */
const char *
vitte_version_git_commit(void);

/*
 * Return build-provided Git branch name.
 *
 * Empty string means unavailable.
 */
const char *
vitte_version_git_branch(void);

/*
 * Return true when the build system marked the source tree dirty.
 */
bool
vitte_version_git_dirty(void);

/* ========================================================================= */
/* Build timestamp                                                           */
/* ========================================================================= */

/*
 * Return build date/time metadata.
 *
 * Release/reproducible builds should define:
 *
 *     VITTE_VERSION_BUILD_DATE
 *     VITTE_VERSION_BUILD_TIME
 *
 * from deterministic build inputs.
 *
 * The default implementation may fall back to __DATE__ and __TIME__.
 */
const char *
vitte_version_build_date(void);

const char *
vitte_version_build_time(void);

/* ========================================================================= */
/* Aggregate snapshot                                                        */
/* ========================================================================= */

/*
 * Return a complete immutable snapshot.
 *
 * The structure is returned by value.
 *
 * All contained strings remain owned by the version subsystem/build image.
 */
vitte_version_info_t
vitte_version_info(void);

/* ========================================================================= */
/* Human-readable representation                                             */
/* ========================================================================= */

/*
 * Return a compact one-line build identity.
 *
 * Typical shape:
 *
 *     vitte 0.1.1 (linux/x86_64, release, clang 18.1.8) git:abcdef
 *
 * Git information is omitted when unavailable.
 *
 * The returned pointer refers to immutable per-thread internal storage.  A
 * caller must copy it before using it from another thread.
 */
const char *
vitte_version_full_string(void);

/*
 * Format detailed multi-line build information.
 *
 * The return value follows snprintf-style length semantics:
 *
 *     - number of bytes that would have been written, excluding NUL;
 *     - may be greater than or equal to capacity when truncated;
 *     - zero on formatting failure.
 *
 * Supported size query:
 *
 *     required = vitte_version_format(NULL, 0);
 *
 * When buffer != NULL and capacity > 0, output is NUL-terminated whenever
 * snprintf can produce output.
 */
size_t
vitte_version_format(
    char *buffer,
    size_t capacity
);

/* ========================================================================= */
/* Deterministic build identity                                              */
/* ========================================================================= */

/*
 * Return a deterministic 64-bit identity for the semantic/compiler build.
 *
 * The current identity includes:
 *
 *     - identity schema
 *     - project name
 *     - compiler/tool name
 *     - Vitte semantic version
 *     - API version
 *     - ABI version
 *     - host platform
 *     - host architecture
 *     - host pointer width
 *     - host byte order
 *     - detected C standard
 *     - build mode
 *     - Git commit
 *     - dirty state
 *     - compiled feature set
 *
 * Build date/time are intentionally excluded so reproducible builds can
 * produce the same identity from identical semantic inputs.
 *
 * This value is suitable for:
 *
 *     caches
 *     logs
 *     test fixtures
 *     compiler provenance
 *     artifact metadata
 *
 * It is NOT a cryptographic hash and must not be used as a security
 * signature.
 */
uint64_t
vitte_version_build_id(void);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate internal version/build metadata consistency.
 *
 * Checks include:
 *
 *     - non-empty mandatory identity strings;
 *     - semantic-version identifier syntax;
 *     - prerelease numeric leading-zero rules;
 *     - API/ABI string consistency;
 *     - pointer-width validity;
 *     - byte-order validity;
 *     - C-standard validity;
 *     - semantic-version buffer completeness;
 *     - diagnostic feature dependency consistency.
 *
 * This function performs no allocation and has no compiler-frontend
 * dependency.
 */
bool
vitte_version_validate(void);

/* ========================================================================= */
/* Convenience helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_version_is_prerelease(void)
{
    const char *value =
        vitte_version_prerelease();

    return
        value != NULL &&
        value[0] != '\0';
}

static inline bool
vitte_version_has_build_metadata(void)
{
    const char *value =
        vitte_version_build_metadata();

    return
        value != NULL &&
        value[0] != '\0';
}

static inline bool
vitte_version_has_git_commit(void)
{
    const char *value =
        vitte_version_git_commit();

    return
        value != NULL &&
        value[0] != '\0';
}

static inline bool
vitte_version_has_git_branch(void)
{
    const char *value =
        vitte_version_git_branch();

    return
        value != NULL &&
        value[0] != '\0';
}

static inline bool
vitte_version_is_debug_build(void)
{
    return
        strcmp(
            vitte_version_build_mode(),
            "debug") == 0;
}

static inline bool
vitte_version_is_release_build(void)
{
    return
        strcmp(
            vitte_version_build_mode(),
            "release") == 0;
}

/* ========================================================================= */
/* Version comparison helpers                                                */
/* ========================================================================= */

/*
 * Compare the running Vitte semantic version core against major.minor.patch.
 *
 * Return:
 *
 *     < 0  running version is older
 *       0  core versions are equal
 *     > 0  running version is newer
 *
 * Prerelease/build metadata are intentionally ignored by this helper.
 */
static inline int
vitte_version_compare_core(
    uint32_t major,
    uint32_t minor,
    uint32_t patch)
{
    if (vitte_version_major() < major) {
        return -1;
    }

    if (vitte_version_major() > major) {
        return 1;
    }

    if (vitte_version_minor() < minor) {
        return -1;
    }

    if (vitte_version_minor() > minor) {
        return 1;
    }

    if (vitte_version_patch() < patch) {
        return -1;
    }

    if (vitte_version_patch() > patch) {
        return 1;
    }

    return 0;
}

static inline bool
vitte_version_at_least(
    uint32_t major,
    uint32_t minor,
    uint32_t patch)
{
    return
        vitte_version_compare_core(
            major,
            minor,
            patch) >= 0;
}

/* ========================================================================= */
/* Feature helpers                                                           */
/* ========================================================================= */

static inline bool
vitte_version_has_diagnostics(void)
{
    return
        vitte_version_has_feature(
            VITTE_VERSION_FEATURE_DIAGNOSTICS);
}

static inline bool
vitte_version_has_c17_backend(void)
{
    return
        vitte_version_has_feature(
            VITTE_VERSION_FEATURE_C17_BACKEND);
}

static inline bool
vitte_version_has_contracts(void)
{
    return
        vitte_version_has_feature(
            VITTE_VERSION_FEATURE_CONTRACTS);
}

static inline bool
vitte_version_has_comptime(void)
{
    return
        vitte_version_has_feature(
            VITTE_VERSION_FEATURE_COMPTIME);
}

/* ========================================================================= */
/* Architecture contract                                                     */
/* ========================================================================= */

/*
 * This API reports the identity of the compiler executable.
 *
 * Never use:
 *
 *     vitte_version_platform()
 *     vitte_version_architecture()
 *     vitte_version_pointer_bits()
 *     vitte_version_endianness()
 *
 * as a substitute for target information when cross-compiling.
 *
 * Target properties belong to the driver/backend/target subsystem.
 *
 *
 * Keep this header independent from:
 *
 *     api/error.h
 *     AST
 *     source manager
 *     diagnostics
 *     lexer/parser
 *     HIR
 *     IR
 *     backend implementation
 *
 * so `vitte --version` and embedding applications can inspect compiler
 * identity without initializing the compilation pipeline.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_API_VERSION_H */
