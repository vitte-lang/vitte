#ifndef VITTE_SRC_BACKEND_C17_OPTIONS_H
#define VITTE_SRC_BACKEND_C17_OPTIONS_H

/*
 * Vitte Compiler
 * src/backend/c17/options.h
 *
 * Canonical configuration/policy layer for the ISO C17 backend.
 *
 * This module sits above backend_config:
 *
 *   command line / project configuration
 *                 |
 *                 v
 *             options.h
 *                 |
 *                 +-- profile
 *                 +-- dialect
 *                 +-- optimization
 *                 +-- runtime policy
 *                 +-- safety policy
 *                 +-- diagnostics
 *                 +-- source maps
 *                 +-- deterministic output
 *                 +-- native compiler policy
 *                 +-- sanitizers
 *                 +-- resource limits
 *                 |
 *                 v
 *       vitte_c17_backend_config_t
 *
 * options.c owns policy.
 * backend.c owns C17 generation.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#ifndef VITTE_C17_OPTIONS_MAGIC
#define VITTE_C17_OPTIONS_MAGIC \
    UINT64_C(0x56495454454F5054)
#endif

#ifndef VITTE_C17_OPTIONS_DEAD_MAGIC
#define VITTE_C17_OPTIONS_DEAD_MAGIC \
    UINT64_C(0x444541444F505421)
#endif

#ifndef VITTE_C17_OPTIONS_DEFAULT_INDENT_WIDTH
#define VITTE_C17_OPTIONS_DEFAULT_INDENT_WIDTH \
    ((size_t)4u)
#endif

#ifndef VITTE_C17_OPTIONS_DEFAULT_MAX_INDENT_DEPTH
#define VITTE_C17_OPTIONS_DEFAULT_MAX_INDENT_DEPTH \
    ((size_t)4096u)
#endif

#ifndef VITTE_C17_OPTIONS_DEFAULT_MAX_IDENTIFIER_BYTES
#define VITTE_C17_OPTIONS_DEFAULT_MAX_IDENTIFIER_BYTES \
    ((size_t)4096u)
#endif

#ifndef VITTE_C17_OPTIONS_DEFAULT_MAX_ERRORS
#define VITTE_C17_OPTIONS_DEFAULT_MAX_ERRORS \
    ((size_t)100u)
#endif

#ifndef VITTE_C17_OPTIONS_DEFAULT_MAX_OUTPUT_BYTES
#define VITTE_C17_OPTIONS_DEFAULT_MAX_OUTPUT_BYTES \
    SIZE_MAX
#endif

#ifndef VITTE_C17_OPTIONS_MAX_INDENT_WIDTH
#define VITTE_C17_OPTIONS_MAX_INDENT_WIDTH \
    ((size_t)32u)
#endif

#ifndef VITTE_C17_OPTIONS_MAX_INDENT_DEPTH
#define VITTE_C17_OPTIONS_MAX_INDENT_DEPTH \
    ((size_t)(1024u * 1024u))
#endif

#ifndef VITTE_C17_OPTIONS_MAX_IDENTIFIER_BYTES
#define VITTE_C17_OPTIONS_MAX_IDENTIFIER_BYTES \
    ((size_t)(1024u * 1024u))
#endif

#ifndef VITTE_C17_OPTIONS_HASH_OFFSET
#define VITTE_C17_OPTIONS_HASH_OFFSET \
    UINT64_C(14695981039346656037)
#endif

#ifndef VITTE_C17_OPTIONS_HASH_PRIME
#define VITTE_C17_OPTIONS_HASH_PRIME \
    UINT64_C(1099511628211)
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_c17_options
    vitte_c17_options_t;

typedef struct vitte_c17_options_stats
    vitte_c17_options_stats_t;

/*
 * backend.h owns the complete definition.
 *
 * Forward declaration keeps options.h independent from backend.h and avoids
 * unnecessary include coupling.
 */
typedef struct vitte_c17_backend_config
    vitte_c17_backend_config_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_c17_options_error {
    VITTE_C17_OPTIONS_ERROR_NONE = 0,

    VITTE_C17_OPTIONS_ERROR_INVALID_OPTIONS,

    VITTE_C17_OPTIONS_ERROR_INVALID_ARGUMENT,

    VITTE_C17_OPTIONS_ERROR_INVALID_PROFILE,

    VITTE_C17_OPTIONS_ERROR_INVALID_DIALECT,

    VITTE_C17_OPTIONS_ERROR_INVALID_OPTIMIZATION,

    VITTE_C17_OPTIONS_ERROR_INVALID_RUNTIME,

    VITTE_C17_OPTIONS_ERROR_INVALID_LIMIT,

    VITTE_C17_OPTIONS_ERROR_CONFLICT,

    VITTE_C17_OPTIONS_ERROR_UNSUPPORTED,

    VITTE_C17_OPTIONS_ERROR_CORRUPTION,

    VITTE_C17_OPTIONS_ERROR_COUNT
} vitte_c17_options_error_t;

/* ========================================================================= */
/* Profiles                                                                  */
/* ========================================================================= */

/*
 * High-level compilation profiles.
 *
 * DEFAULT
 *     Safe canonical baseline.
 *
 * DEBUG
 *     Debug information, source mapping, assertions, checks and readable C.
 *
 * RELEASE
 *     Speed-oriented output while preserving memory/runtime safety checks.
 *
 * SIZE
 *     Size-oriented native compilation and compact generated output.
 *
 * SANITIZE
 *     Debug-oriented generation with native sanitizers.
 *
 * FREESTANDING
 *     Runtime-independent/freestanding C environment.
 */
typedef enum vitte_c17_profile {
    VITTE_C17_PROFILE_DEFAULT = 0,

    VITTE_C17_PROFILE_DEBUG,

    VITTE_C17_PROFILE_RELEASE,

    VITTE_C17_PROFILE_SIZE,

    VITTE_C17_PROFILE_SANITIZE,

    VITTE_C17_PROFILE_FREESTANDING,

    VITTE_C17_PROFILE_COUNT
} vitte_c17_profile_t;

/* ========================================================================= */
/* C dialect                                                                 */
/* ========================================================================= */

typedef enum vitte_c17_dialect {
    VITTE_C17_DIALECT_INVALID = 0,

    /*
     * Strict ISO C17.
     */
    VITTE_C17_DIALECT_C17,

    /*
     * GNU C17 extensions.
     */
    VITTE_C17_DIALECT_GNU17,

    VITTE_C17_DIALECT_COUNT
} vitte_c17_dialect_t;

/* ========================================================================= */
/* Optimization                                                              */
/* ========================================================================= */

typedef enum vitte_c17_optimization {
    VITTE_C17_OPTIMIZATION_INVALID = 0,

    /*
     * Native compiler equivalent:
     *
     *     -O0
     */
    VITTE_C17_OPTIMIZATION_NONE,

    /*
     * Debug-friendly optimization:
     *
     *     -Og
     */
    VITTE_C17_OPTIMIZATION_DEBUG,

    /*
     * Balanced:
     *
     *     -O2
     */
    VITTE_C17_OPTIMIZATION_BALANCED,

    /*
     * Speed:
     *
     *     -O3
     */
    VITTE_C17_OPTIMIZATION_SPEED,

    /*
     * Size:
     *
     *     -Os
     */
    VITTE_C17_OPTIMIZATION_SIZE,

    /*
     * Aggressive backend optimization policy.
     *
     * Currently maps to -O3 at the native compiler layer. More aggressive
     * flags should remain toolchain-specific rather than being hard-coded
     * into the generic option model.
     */
    VITTE_C17_OPTIMIZATION_AGGRESSIVE,

    VITTE_C17_OPTIMIZATION_COUNT
} vitte_c17_optimization_t;

/* ========================================================================= */
/* Runtime mode                                                              */
/* ========================================================================= */

typedef enum vitte_c17_runtime_mode {
    VITTE_C17_RUNTIME_MODE_INVALID = 0,

    /*
     * Backend selects the appropriate runtime strategy.
     */
    VITTE_C17_RUNTIME_MODE_AUTO,

    /*
     * Emit required runtime implementation into generated output.
     */
    VITTE_C17_RUNTIME_MODE_EMBEDDED,

    /*
     * Generated code references an externally linked Vitte runtime.
     */
    VITTE_C17_RUNTIME_MODE_EXTERNAL,

    /*
     * No Vitte runtime.
     */
    VITTE_C17_RUNTIME_MODE_NONE,

    VITTE_C17_RUNTIME_MODE_COUNT
} vitte_c17_runtime_mode_t;

/* ========================================================================= */
/* Options                                                                   */
/* ========================================================================= */

struct vitte_c17_options {
    /*
     * Object integrity marker.
     */
    uint64_t magic;

    /*
     * High-level preset from which the current options originated.
     *
     * Individual fields may subsequently be overridden.
     */
    vitte_c17_profile_t profile;

    /*
     * C language mode.
     */
    vitte_c17_dialect_t dialect;

    /*
     * Native optimization policy.
     */
    vitte_c17_optimization_t optimization;

    /*
     * Vitte runtime integration.
     */
    vitte_c17_runtime_mode_t runtime_mode;

    /*
     * Last option-layer error.
     */
    vitte_c17_options_error_t last_error;

    /* --------------------------------------------------------------------- */
    /* Formatting                                                            */
    /* --------------------------------------------------------------------- */

    size_t indent_width;

    bool pretty;

    bool emit_comments;

    bool emit_debug_comments;

    bool emit_banner;

    /* --------------------------------------------------------------------- */
    /* C language policy                                                     */
    /* --------------------------------------------------------------------- */

    /*
     * Require strict ISO C17 output.
     */
    bool strict_c17;

    /*
     * Permit compiler-specific C extensions.
     *
     * Canonical GNU17 normalization:
     *
     *     strict_c17      = false
     *     allow_extensions = true
     */
    bool allow_extensions;

    bool emit_static_assertions;

    /* --------------------------------------------------------------------- */
    /* Runtime                                                               */
    /* --------------------------------------------------------------------- */

    /*
     * Emit runtime implementation into the generated translation unit.
     */
    bool emit_runtime;

    /*
     * Master runtime-check policy.
     */
    bool runtime_checks;

    /*
     * Individual safety checks.
     */
    bool bounds_checks;

    bool null_checks;

    bool overflow_checks;

    bool assertions;

    /* --------------------------------------------------------------------- */
    /* Debugging / source mapping                                            */
    /* --------------------------------------------------------------------- */

    bool debug_info;

    bool emit_line_directives;

    bool emit_source_map;

    /* --------------------------------------------------------------------- */
    /* Diagnostics                                                           */
    /* --------------------------------------------------------------------- */

    bool fail_fast;

    bool warnings_as_errors;

    bool verbose_diagnostics;

    /* --------------------------------------------------------------------- */
    /* Reproducibility                                                       */
    /* --------------------------------------------------------------------- */

    /*
     * Master deterministic-generation policy.
     */
    bool deterministic;

    /*
     * Stable generated identifiers.
     */
    bool stable_names;

    /*
     * Stable declaration/definition ordering.
     */
    bool stable_order;

    /*
     * Stable fingerprints/hashes.
     */
    bool stable_hashes;

    /* --------------------------------------------------------------------- */
    /* Generated C string policy                                             */
    /* --------------------------------------------------------------------- */

    bool trigraph_safe_strings;

    bool escape_non_ascii;

    /* --------------------------------------------------------------------- */
    /* Native compiler policy                                                */
    /* --------------------------------------------------------------------- */

    bool native_debug_symbols;

    /*
     * Preserve frame pointers.
     *
     * Particularly useful for debugging, profiling and sanitizers.
     */
    bool native_frame_pointer;

    bool native_lto;

    bool native_pic;

    bool native_pie;

    /* --------------------------------------------------------------------- */
    /* Native sanitizers                                                     */
    /* --------------------------------------------------------------------- */

    bool sanitize_address;

    bool sanitize_undefined;

    bool sanitize_leak;

    bool sanitize_thread;

    /* --------------------------------------------------------------------- */
    /* Execution environment                                                 */
    /* --------------------------------------------------------------------- */

    /*
     * ISO C freestanding environment.
     */
    bool freestanding;

    /*
     * ISO C hosted environment.
     *
     * Exactly one of hosted/freestanding must be true after normalization.
     */
    bool hosted;

    /* --------------------------------------------------------------------- */
    /* Limits                                                                */
    /* --------------------------------------------------------------------- */

    /*
     * Maximum generated output size.
     *
     * SIZE_MAX means effectively unlimited by this layer.
     */
    size_t max_output_bytes;

    /*
     * Maximum accepted/generated identifier size used by backend policy.
     */
    size_t max_identifier_bytes;

    /*
     * Maximum formatting indentation nesting.
     */
    size_t max_indent_depth;

    /*
     * Maximum number of backend diagnostics classified as errors.
     */
    size_t max_errors;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_c17_options_stats {
    /*
     * Structural/policy validation result.
     */
    bool valid;

    /*
     * Number of boolean policy fields represented by this version of the
     * option model.
     */
    size_t boolean_option_count;

    /*
     * Number currently enabled.
     */
    size_t enabled_boolean_count;

    /*
     * Derived properties.
     */
    bool has_sanitizer;

    bool has_runtime_checks;

    bool reproducible;

    /*
     * Stable fingerprint of the effective option set.
     */
    uint64_t hash;
};

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_options_error_name(
    vitte_c17_options_error_t error);

const char *
vitte_c17_profile_name(
    vitte_c17_profile_t profile);

const char *
vitte_c17_dialect_name(
    vitte_c17_dialect_t dialect);

const char *
vitte_c17_optimization_name(
    vitte_c17_optimization_t optimization);

const char *
vitte_c17_runtime_mode_name(
    vitte_c17_runtime_mode_t mode);

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

/*
 * Return the canonical safe baseline configuration.
 */
vitte_c17_options_t
vitte_c17_options_default(void);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize options using a complete profile.
 */
bool
vitte_c17_options_init(
    vitte_c17_options_t *options,
    vitte_c17_profile_t profile);

/*
 * Mark an option object dead.
 *
 * No dynamic storage is currently owned by vitte_c17_options_t, but keeping
 * an explicit lifecycle allows future extension without changing callers.
 */
void
vitte_c17_options_destroy(
    vitte_c17_options_t *options);

/* ========================================================================= */
/* Profiles                                                                  */
/* ========================================================================= */

/*
 * Replace policy fields with the selected complete profile.
 *
 * This intentionally resets previous profile-specific flags before applying
 * the new profile.
 */
bool
vitte_c17_options_apply_profile(
    vitte_c17_options_t *options,
    vitte_c17_profile_t profile);

/* ========================================================================= */
/* Validity / validation                                                     */
/* ========================================================================= */

/*
 * Lightweight structural validity.
 */
bool
vitte_c17_options_is_valid(
    const vitte_c17_options_t *options);

/*
 * Deep policy validation.
 *
 * Returns VITTE_C17_OPTIONS_ERROR_NONE when valid.
 *
 * This checks constraints including:
 *
 *   - dialect consistency
 *   - strict/extension consistency
 *   - hosted/freestanding exclusivity
 *   - runtime consistency
 *   - sanitizer compatibility
 *   - LTO/sanitizer policy
 *   - identifier limits
 *   - indentation limits
 *   - diagnostic limits
 */
vitte_c17_options_error_t
vitte_c17_options_validate(
    const vitte_c17_options_t *options);

/*
 * Normalize related policy fields and then validate.
 *
 * Examples:
 *
 *   C17:
 *       strict_c17       = true
 *       allow_extensions = false
 *
 *   GNU17:
 *       strict_c17       = false
 *       allow_extensions = true
 *
 *   sanitizers:
 *       frame pointers   = true
 *       debug symbols    = true
 *       LTO              = false
 */
bool
vitte_c17_options_normalize(
    vitte_c17_options_t *options);

/* ========================================================================= */
/* Backend conversion                                                        */
/* ========================================================================= */

/*
 * Convert high-level options to the lower-level backend generation config.
 *
 * Native compiler-only options remain in vitte_c17_options_t and are not
 * copied into backend_config.
 */
bool
vitte_c17_options_to_backend_config(
    const vitte_c17_options_t *options,
    vitte_c17_backend_config_t *config);

/* ========================================================================= */
/* Stable fingerprint                                                        */
/* ========================================================================= */

/*
 * Return a deterministic FNV-1a-derived fingerprint of all effective option
 * fields.
 *
 * The serialization is explicitly host-endian independent.
 *
 * This hash is suitable for:
 *
 *   - build cache keys
 *   - reproducibility checks
 *   - backend configuration fingerprints
 *
 * It is not cryptographic.
 */
uint64_t
vitte_c17_options_hash(
    const vitte_c17_options_t *options);

/* ========================================================================= */
/* Equality                                                                  */
/* ========================================================================= */

/*
 * Semantic fieldwise equality.
 *
 * Deliberately does not use memcmp(), avoiding padding-byte dependence.
 */
bool
vitte_c17_options_equal(
    const vitte_c17_options_t *left,
    const vitte_c17_options_t *right);

/* ========================================================================= */
/* Derived feature predicates                                                */
/* ========================================================================= */

bool
vitte_c17_options_has_sanitizer(
    const vitte_c17_options_t *options);

bool
vitte_c17_options_has_runtime_checks(
    const vitte_c17_options_t *options);

bool
vitte_c17_options_is_reproducible(
    const vitte_c17_options_t *options);

bool
vitte_c17_options_optimizes_for_speed(
    const vitte_c17_options_t *options);

bool
vitte_c17_options_optimizes_for_size(
    const vitte_c17_options_t *options);

/* ========================================================================= */
/* Native compiler hints                                                     */
/* ========================================================================= */

/*
 * Generic Clang/GCC-style optimization flag:
 *
 *     NONE        -> -O0
 *     DEBUG       -> -Og
 *     BALANCED    -> -O2
 *     SPEED       -> -O3
 *     SIZE        -> -Os
 *     AGGRESSIVE  -> -O3
 *
 * A future compiler-driver layer should own toolchain-specific translation
 * rather than assuming every native compiler accepts these flags.
 */
const char *
vitte_c17_options_native_optimization_flag(
    const vitte_c17_options_t *options);

/*
 * Generic Clang/GCC-style language flag:
 *
 *     C17    -> -std=c17
 *     GNU17  -> -std=gnu17
 */
const char *
vitte_c17_options_native_dialect_flag(
    const vitte_c17_options_t *options);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_options_stats_t
vitte_c17_options_stats(
    const vitte_c17_options_t *options);

/* ========================================================================= */
/* Error access                                                              */
/* ========================================================================= */

vitte_c17_options_error_t
vitte_c17_options_last_error(
    const vitte_c17_options_t *options);

void
vitte_c17_options_clear_error(
    vitte_c17_options_t *options);

/* ========================================================================= */
/* Inline helpers                                                            */
/* ========================================================================= */

static inline bool
vitte_c17_options_has_error(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->last_error !=
            VITTE_C17_OPTIONS_ERROR_NONE;
}

static inline bool
vitte_c17_options_is_debug(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->profile ==
            VITTE_C17_PROFILE_DEBUG;
}

static inline bool
vitte_c17_options_is_release(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->profile ==
            VITTE_C17_PROFILE_RELEASE;
}

static inline bool
vitte_c17_options_is_freestanding(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->freestanding;
}

static inline bool
vitte_c17_options_is_hosted(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->hosted;
}

static inline bool
vitte_c17_options_uses_external_runtime(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->runtime_mode ==
            VITTE_C17_RUNTIME_MODE_EXTERNAL;
}

static inline bool
vitte_c17_options_embeds_runtime(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->runtime_mode ==
            VITTE_C17_RUNTIME_MODE_EMBEDDED;
}

static inline bool
vitte_c17_options_has_runtime(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->runtime_mode !=
            VITTE_C17_RUNTIME_MODE_NONE;
}

static inline bool
vitte_c17_options_uses_strict_c17(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->dialect ==
            VITTE_C17_DIALECT_C17 &&
        options->strict_c17 &&
        !options->allow_extensions;
}

static inline bool
vitte_c17_options_uses_gnu17(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->dialect ==
            VITTE_C17_DIALECT_GNU17;
}

/* ========================================================================= */
/* Sanitizer helpers                                                         */
/* ========================================================================= */

static inline bool
vitte_c17_options_has_address_sanitizer(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->sanitize_address;
}

static inline bool
vitte_c17_options_has_undefined_sanitizer(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->sanitize_undefined;
}

static inline bool
vitte_c17_options_has_leak_sanitizer(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->sanitize_leak;
}

static inline bool
vitte_c17_options_has_thread_sanitizer(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->sanitize_thread;
}

/* ========================================================================= */
/* Safety helpers                                                            */
/* ========================================================================= */

static inline bool
vitte_c17_options_checks_bounds(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->bounds_checks;
}

static inline bool
vitte_c17_options_checks_null(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->null_checks;
}

static inline bool
vitte_c17_options_checks_overflow(
    const vitte_c17_options_t *options)
{
    return
        options != NULL &&
        options->magic ==
            VITTE_C17_OPTIONS_MAGIC &&
        options->overflow_checks;
}

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte C17 options require 64-bit uint64_t");

_Static_assert(
    VITTE_C17_OPTIONS_ERROR_NONE == 0,
    "options no-error value must remain zero");

_Static_assert(
    VITTE_C17_PROFILE_DEFAULT == 0,
    "default profile must remain zero");

_Static_assert(
    VITTE_C17_DIALECT_INVALID == 0,
    "invalid dialect must remain zero");

_Static_assert(
    VITTE_C17_OPTIMIZATION_INVALID == 0,
    "invalid optimization must remain zero");

_Static_assert(
    VITTE_C17_RUNTIME_MODE_INVALID == 0,
    "invalid runtime mode must remain zero");

_Static_assert(
    VITTE_C17_OPTIONS_DEFAULT_INDENT_WIDTH <=
        VITTE_C17_OPTIONS_MAX_INDENT_WIDTH,
    "default indentation exceeds maximum");

_Static_assert(
    VITTE_C17_OPTIONS_DEFAULT_MAX_INDENT_DEPTH <=
        VITTE_C17_OPTIONS_MAX_INDENT_DEPTH,
    "default indentation depth exceeds maximum");

_Static_assert(
    VITTE_C17_OPTIONS_DEFAULT_MAX_IDENTIFIER_BYTES <=
        VITTE_C17_OPTIONS_MAX_IDENTIFIER_BYTES,
    "default identifier limit exceeds maximum");

_Static_assert(
    VITTE_C17_OPTIONS_DEFAULT_MAX_ERRORS != 0u,
    "default maximum error count must not be zero");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_BACKEND_C17_OPTIONS_H */
