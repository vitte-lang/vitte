/*
 * Vitte Compiler
 * src/config/config.c
 *
 * Canonical compiler configuration engine.
 *
 * Responsibilities:
 *   - compiler/project configuration lifecycle
 *   - defaults
 *   - configuration layers and provenance
 *   - typed configuration values
 *   - target/backend/profile/optimization policy
 *   - diagnostics policy
 *   - runtime safety policy
 *   - build policy
 *   - package/registry policy
 *   - path configuration
 *   - environment/config/CLI precedence
 *   - deterministic merging
 *   - validation
 *   - conflict detection
 *   - immutable/frozen configuration
 *   - deterministic fingerprints
 *   - statistics
 *
 * This layer deliberately does NOT:
 *   - parse argv
 *   - parse project manifests
 *   - access the filesystem
 *   - read environment variables directly
 *   - download packages
 *   - compile source files
 *   - generate C17
 *   - execute programs
 *
 * Other subsystems convert their input into configuration assignments and
 * apply them through this API.
 *
 * Intended precedence:
 *
 *     built-in defaults
 *            <
 *     system configuration
 *            <
 *     user configuration
 *            <
 *     project configuration
 *            <
 *     environment
 *            <
 *     command line
 *            <
 *     explicit programmatic override
 *
 * ISO C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_CONFIG_MAGIC \
    UINT64_C(0x564954434F4E4647)

#define VITTE_CONFIG_DEAD_MAGIC \
    UINT64_C(0x44454144434F4E46)

#define VITTE_CONFIG_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_CONFIG_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_CONFIG_DEFAULT_MAX_ENTRIES \
    ((size_t)4096u)

#define VITTE_CONFIG_DEFAULT_INITIAL_CAPACITY \
    ((size_t)32u)

#define VITTE_CONFIG_DEFAULT_MAX_ERRORS \
    ((size_t)100u)

#define VITTE_CONFIG_DEFAULT_JOBS \
    ((size_t)1u)

#define VITTE_CONFIG_DEFAULT_MAX_OUTPUT_BYTES \
    ((size_t)0u)

#define VITTE_CONFIG_DEFAULT_MAX_STRING_BYTES \
    ((size_t)(1024u * 1024u))

/* ========================================================================= */
/* Error                                                                     */
/* ========================================================================= */

typedef enum vitte_config_error {
    VITTE_CONFIG_ERROR_NONE = 0,

    VITTE_CONFIG_ERROR_INVALID_CONFIG,
    VITTE_CONFIG_ERROR_INVALID_ARGUMENT,
    VITTE_CONFIG_ERROR_INVALID_STATE,

    VITTE_CONFIG_ERROR_INVALID_KEY,
    VITTE_CONFIG_ERROR_INVALID_VALUE,
    VITTE_CONFIG_ERROR_INVALID_TYPE,
    VITTE_CONFIG_ERROR_INVALID_SOURCE,

    VITTE_CONFIG_ERROR_UNKNOWN_KEY,
    VITTE_CONFIG_ERROR_DUPLICATE_VALUE,
    VITTE_CONFIG_ERROR_CONFLICT,
    VITTE_CONFIG_ERROR_LOWER_PRECEDENCE,

    VITTE_CONFIG_ERROR_INVALID_PROFILE,
    VITTE_CONFIG_ERROR_INVALID_BACKEND,
    VITTE_CONFIG_ERROR_INVALID_OPTIMIZATION,
    VITTE_CONFIG_ERROR_INVALID_DIAGNOSTIC_FORMAT,
    VITTE_CONFIG_ERROR_INVALID_COLOR_MODE,
    VITTE_CONFIG_ERROR_INVALID_TARGET,

    VITTE_CONFIG_ERROR_TOO_MANY_ENTRIES,
    VITTE_CONFIG_ERROR_STRING_TOO_LONG,

    VITTE_CONFIG_ERROR_FROZEN,

    VITTE_CONFIG_ERROR_OVERFLOW,
    VITTE_CONFIG_ERROR_OUT_OF_MEMORY,

    VITTE_CONFIG_ERROR_CORRUPTION,

    VITTE_CONFIG_ERROR_COUNT
} vitte_config_error_t;

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

typedef enum vitte_config_state {
    VITTE_CONFIG_STATE_INVALID = 0,

    VITTE_CONFIG_STATE_READY,
    VITTE_CONFIG_STATE_MERGING,
    VITTE_CONFIG_STATE_VALIDATED,
    VITTE_CONFIG_STATE_FROZEN,
    VITTE_CONFIG_STATE_FAILED,
    VITTE_CONFIG_STATE_DESTROYED,

    VITTE_CONFIG_STATE_COUNT
} vitte_config_state_t;

/* ========================================================================= */
/* Source / provenance                                                       */
/* ========================================================================= */

typedef enum vitte_config_source {
    VITTE_CONFIG_SOURCE_DEFAULT = 0,

    VITTE_CONFIG_SOURCE_SYSTEM,
    VITTE_CONFIG_SOURCE_USER,
    VITTE_CONFIG_SOURCE_PROJECT,
    VITTE_CONFIG_SOURCE_ENVIRONMENT,
    VITTE_CONFIG_SOURCE_COMMAND_LINE,
    VITTE_CONFIG_SOURCE_OVERRIDE,

    VITTE_CONFIG_SOURCE_COUNT
} vitte_config_source_t;

/* ========================================================================= */
/* Value type                                                                */
/* ========================================================================= */

typedef enum vitte_config_value_type {
    VITTE_CONFIG_VALUE_INVALID = 0,

    VITTE_CONFIG_VALUE_BOOL,
    VITTE_CONFIG_VALUE_SIZE,
    VITTE_CONFIG_VALUE_U64,
    VITTE_CONFIG_VALUE_STRING,

    VITTE_CONFIG_VALUE_COUNT
} vitte_config_value_type_t;

/* ========================================================================= */
/* Profile                                                                   */
/* ========================================================================= */

typedef enum vitte_config_profile {
    VITTE_CONFIG_PROFILE_DEFAULT = 0,

    VITTE_CONFIG_PROFILE_DEBUG,
    VITTE_CONFIG_PROFILE_RELEASE,
    VITTE_CONFIG_PROFILE_SIZE,
    VITTE_CONFIG_PROFILE_SANITIZE,
    VITTE_CONFIG_PROFILE_FREESTANDING,

    VITTE_CONFIG_PROFILE_COUNT
} vitte_config_profile_t;

/* ========================================================================= */
/* Backend                                                                   */
/* ========================================================================= */

typedef enum vitte_config_backend {
    VITTE_CONFIG_BACKEND_DEFAULT = 0,

    VITTE_CONFIG_BACKEND_C17,
    VITTE_CONFIG_BACKEND_NATIVE,
    VITTE_CONFIG_BACKEND_BYTECODE,

    VITTE_CONFIG_BACKEND_COUNT
} vitte_config_backend_t;

/* ========================================================================= */
/* Optimization                                                              */
/* ========================================================================= */

typedef enum vitte_config_optimization {
    VITTE_CONFIG_OPTIMIZATION_DEFAULT = 0,

    VITTE_CONFIG_OPTIMIZATION_NONE,
    VITTE_CONFIG_OPTIMIZATION_DEBUG,
    VITTE_CONFIG_OPTIMIZATION_BALANCED,
    VITTE_CONFIG_OPTIMIZATION_SPEED,
    VITTE_CONFIG_OPTIMIZATION_SIZE,
    VITTE_CONFIG_OPTIMIZATION_AGGRESSIVE,

    VITTE_CONFIG_OPTIMIZATION_COUNT
} vitte_config_optimization_t;

/* ========================================================================= */
/* Diagnostics                                                               */
/* ========================================================================= */

typedef enum vitte_config_diagnostic_format {
    VITTE_CONFIG_DIAGNOSTIC_DEFAULT = 0,

    VITTE_CONFIG_DIAGNOSTIC_TERMINAL,
    VITTE_CONFIG_DIAGNOSTIC_SHORT,
    VITTE_CONFIG_DIAGNOSTIC_JSON,
    VITTE_CONFIG_DIAGNOSTIC_SARIF,
    VITTE_CONFIG_DIAGNOSTIC_LSP,

    VITTE_CONFIG_DIAGNOSTIC_COUNT
} vitte_config_diagnostic_format_t;

/* ========================================================================= */
/* Color                                                                     */
/* ========================================================================= */

typedef enum vitte_config_color {
    VITTE_CONFIG_COLOR_AUTO = 0,

    VITTE_CONFIG_COLOR_ALWAYS,
    VITTE_CONFIG_COLOR_NEVER,

    VITTE_CONFIG_COLOR_COUNT
} vitte_config_color_t;

/* ========================================================================= */
/* Known configuration keys                                                  */
/* ========================================================================= */

typedef enum vitte_config_key {
    VITTE_CONFIG_KEY_INVALID = 0,

    VITTE_CONFIG_KEY_PROFILE,
    VITTE_CONFIG_KEY_BACKEND,
    VITTE_CONFIG_KEY_OPTIMIZATION,

    VITTE_CONFIG_KEY_TARGET,
    VITTE_CONFIG_KEY_OUTPUT,
    VITTE_CONFIG_KEY_MANIFEST_PATH,
    VITTE_CONFIG_KEY_PROJECT_ROOT,
    VITTE_CONFIG_KEY_BUILD_DIR,
    VITTE_CONFIG_KEY_CACHE_DIR,
    VITTE_CONFIG_KEY_SYSROOT,
    VITTE_CONFIG_KEY_REGISTRY,

    VITTE_CONFIG_KEY_JOBS,
    VITTE_CONFIG_KEY_MAX_ERRORS,
    VITTE_CONFIG_KEY_MAX_OUTPUT_BYTES,

    VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT,
    VITTE_CONFIG_KEY_COLOR,

    VITTE_CONFIG_KEY_VERBOSE,
    VITTE_CONFIG_KEY_QUIET,
    VITTE_CONFIG_KEY_WARNINGS_AS_ERRORS,

    VITTE_CONFIG_KEY_EMIT_DEBUG_INFO,
    VITTE_CONFIG_KEY_EMIT_SOURCE_MAP,
    VITTE_CONFIG_KEY_EMIT_COMMENTS,

    VITTE_CONFIG_KEY_RUNTIME_CHECKS,
    VITTE_CONFIG_KEY_BOUNDS_CHECKS,
    VITTE_CONFIG_KEY_NULL_CHECKS,
    VITTE_CONFIG_KEY_OVERFLOW_CHECKS,

    VITTE_CONFIG_KEY_DETERMINISTIC,
    VITTE_CONFIG_KEY_INCREMENTAL,

    VITTE_CONFIG_KEY_OFFLINE,
    VITTE_CONFIG_KEY_LOCKED,
    VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES,

    VITTE_CONFIG_KEY_FORCE,

    VITTE_CONFIG_KEY_COUNT
} vitte_config_key_t;

/* ========================================================================= */
/* Value                                                                     */
/* ========================================================================= */

typedef struct vitte_config_value {
    vitte_config_value_type_t type;

    union {
        bool boolean;
        size_t size;
        uint64_t u64;
        const char *string;
    } as;
} vitte_config_value_t;

/* ========================================================================= */
/* Provenance                                                                */
/* ========================================================================= */

typedef struct vitte_config_origin {
    vitte_config_source_t source;

    /*
     * Optional borrowed description of the source.
     *
     * Examples:
     *
     *     "built-in"
     *     "/etc/vitte/config"
     *     "~/.config/vitte/config"
     *     "./vitte.toml"
     *     "VITTE_TARGET"
     *     "--target"
     */
    const char *name;

    /*
     * Optional source position.
     *
     * Zero means unknown/not applicable.
     */
    size_t line;
    size_t column;
} vitte_config_origin_t;

/* ========================================================================= */
/* Entry                                                                     */
/* ========================================================================= */

typedef struct vitte_config_entry {
    vitte_config_key_t key;
    vitte_config_value_t value;
    vitte_config_origin_t origin;

    uint64_t sequence;
    bool explicitly_set;
} vitte_config_entry_t;

/* ========================================================================= */
/* Resolved configuration                                                    */
/* ========================================================================= */

typedef struct vitte_config_resolved {
    vitte_config_profile_t profile;
    vitte_config_backend_t backend;
    vitte_config_optimization_t optimization;

    const char *target;
    const char *output;
    const char *manifest_path;
    const char *project_root;
    const char *build_dir;
    const char *cache_dir;
    const char *sysroot;
    const char *registry;

    size_t jobs;
    size_t max_errors;
    size_t max_output_bytes;

    vitte_config_diagnostic_format_t diagnostic_format;
    vitte_config_color_t color;

    bool verbose;
    bool quiet;
    bool warnings_as_errors;

    bool emit_debug_info;
    bool emit_source_map;
    bool emit_comments;

    bool runtime_checks;
    bool bounds_checks;
    bool null_checks;
    bool overflow_checks;

    bool deterministic;
    bool incremental;

    bool offline;
    bool locked;
    bool frozen_dependencies;

    bool force;
} vitte_config_resolved_t;

/* ========================================================================= */
/* Message                                                                   */
/* ========================================================================= */

typedef struct vitte_config_message {
    vitte_config_error_t error;
    vitte_config_key_t key;

    vitte_config_origin_t origin;

    const char *detail;
} vitte_config_message_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_config_stats {
    size_t entry_count;
    size_t explicit_entry_count;
    size_t overridden_entry_count;
    size_t ignored_lower_precedence_count;

    size_t string_entry_count;
    size_t boolean_entry_count;
    size_t numeric_entry_count;

    uint64_t fingerprint;

    bool validated;
    bool frozen;
} vitte_config_stats_t;

/* ========================================================================= */
/* Config object                                                             */
/* ========================================================================= */

typedef struct vitte_config {
    uint64_t magic;

    vitte_config_state_t state;
    vitte_config_error_t last_error;

    vitte_config_entry_t *entries;
    size_t entry_count;
    size_t entry_capacity;
    size_t max_entries;

    size_t max_string_bytes;

    vitte_config_resolved_t resolved;

    vitte_config_message_t message;
    vitte_config_stats_t stats;

    uint64_t next_sequence;
    uint64_t generation;
} vitte_config_t;

/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

static bool
vitte_config_size_add(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left > SIZE_MAX - right) {
        return false;
    }

    *result = left + right;

    return true;
}

static bool
vitte_config_size_mul(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left != 0u &&
        right > SIZE_MAX / left) {
        return false;
    }

    *result = left * right;

    return true;
}

static bool
vitte_config_string_equal(
    const char *left,
    const char *right)
{
    if (left == NULL ||
        right == NULL) {
        return false;
    }

    return strcmp(left, right) == 0;
}

static uint64_t
vitte_config_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    if (data == NULL &&
        length != 0u) {
        return hash;
    }

    bytes =
        (const unsigned char *)data;

    for (index = 0u;
         index < length;
         ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_CONFIG_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_config_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned shift;

    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        unsigned char byte;

        byte =
            (unsigned char)(
                (value >> shift) &
                UINT64_C(0xff));

        hash =
            vitte_config_hash_bytes(
                hash,
                &byte,
                1u);
    }

    return hash;
}

static uint64_t
vitte_config_hash_string(
    uint64_t hash,
    const char *text)
{
    if (text == NULL) {
        return
            vitte_config_hash_u64(
                hash,
                UINT64_MAX);
    }

    hash =
        vitte_config_hash_bytes(
            hash,
            text,
            strlen(text));

    return
        vitte_config_hash_u64(
            hash,
            UINT64_C(0));
}

/* ========================================================================= */
/* Public names                                                              */
/* ========================================================================= */

const char *
vitte_config_error_name(
    vitte_config_error_t error)
{
    switch (error) {
        case VITTE_CONFIG_ERROR_NONE:
            return "none";

        case VITTE_CONFIG_ERROR_INVALID_CONFIG:
            return "invalid-config";

        case VITTE_CONFIG_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_CONFIG_ERROR_INVALID_STATE:
            return "invalid-state";

        case VITTE_CONFIG_ERROR_INVALID_KEY:
            return "invalid-key";

        case VITTE_CONFIG_ERROR_INVALID_VALUE:
            return "invalid-value";

        case VITTE_CONFIG_ERROR_INVALID_TYPE:
            return "invalid-type";

        case VITTE_CONFIG_ERROR_INVALID_SOURCE:
            return "invalid-source";

        case VITTE_CONFIG_ERROR_UNKNOWN_KEY:
            return "unknown-key";

        case VITTE_CONFIG_ERROR_DUPLICATE_VALUE:
            return "duplicate-value";

        case VITTE_CONFIG_ERROR_CONFLICT:
            return "conflict";

        case VITTE_CONFIG_ERROR_LOWER_PRECEDENCE:
            return "lower-precedence";

        case VITTE_CONFIG_ERROR_INVALID_PROFILE:
            return "invalid-profile";

        case VITTE_CONFIG_ERROR_INVALID_BACKEND:
            return "invalid-backend";

        case VITTE_CONFIG_ERROR_INVALID_OPTIMIZATION:
            return "invalid-optimization";

        case VITTE_CONFIG_ERROR_INVALID_DIAGNOSTIC_FORMAT:
            return "invalid-diagnostic-format";

        case VITTE_CONFIG_ERROR_INVALID_COLOR_MODE:
            return "invalid-color-mode";

        case VITTE_CONFIG_ERROR_INVALID_TARGET:
            return "invalid-target";

        case VITTE_CONFIG_ERROR_TOO_MANY_ENTRIES:
            return "too-many-entries";

        case VITTE_CONFIG_ERROR_STRING_TOO_LONG:
            return "string-too-long";

        case VITTE_CONFIG_ERROR_FROZEN:
            return "frozen";

        case VITTE_CONFIG_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_CONFIG_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";

        case VITTE_CONFIG_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_CONFIG_ERROR_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_config_state_name(
    vitte_config_state_t state)
{
    switch (state) {
        case VITTE_CONFIG_STATE_INVALID:
            return "invalid";

        case VITTE_CONFIG_STATE_READY:
            return "ready";

        case VITTE_CONFIG_STATE_MERGING:
            return "merging";

        case VITTE_CONFIG_STATE_VALIDATED:
            return "validated";

        case VITTE_CONFIG_STATE_FROZEN:
            return "frozen";

        case VITTE_CONFIG_STATE_FAILED:
            return "failed";

        case VITTE_CONFIG_STATE_DESTROYED:
            return "destroyed";

        case VITTE_CONFIG_STATE_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_config_source_name(
    vitte_config_source_t source)
{
    switch (source) {
        case VITTE_CONFIG_SOURCE_DEFAULT:
            return "default";

        case VITTE_CONFIG_SOURCE_SYSTEM:
            return "system";

        case VITTE_CONFIG_SOURCE_USER:
            return "user";

        case VITTE_CONFIG_SOURCE_PROJECT:
            return "project";

        case VITTE_CONFIG_SOURCE_ENVIRONMENT:
            return "environment";

        case VITTE_CONFIG_SOURCE_COMMAND_LINE:
            return "command-line";

        case VITTE_CONFIG_SOURCE_OVERRIDE:
            return "override";

        case VITTE_CONFIG_SOURCE_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_config_value_type_name(
    vitte_config_value_type_t type)
{
    switch (type) {
        case VITTE_CONFIG_VALUE_INVALID:
            return "invalid";

        case VITTE_CONFIG_VALUE_BOOL:
            return "bool";

        case VITTE_CONFIG_VALUE_SIZE:
            return "size";

        case VITTE_CONFIG_VALUE_U64:
            return "u64";

        case VITTE_CONFIG_VALUE_STRING:
            return "string";

        case VITTE_CONFIG_VALUE_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_config_profile_name(
    vitte_config_profile_t profile)
{
    switch (profile) {
        case VITTE_CONFIG_PROFILE_DEFAULT:
            return "default";

        case VITTE_CONFIG_PROFILE_DEBUG:
            return "debug";

        case VITTE_CONFIG_PROFILE_RELEASE:
            return "release";

        case VITTE_CONFIG_PROFILE_SIZE:
            return "size";

        case VITTE_CONFIG_PROFILE_SANITIZE:
            return "sanitize";

        case VITTE_CONFIG_PROFILE_FREESTANDING:
            return "freestanding";

        case VITTE_CONFIG_PROFILE_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_config_backend_name(
    vitte_config_backend_t backend)
{
    switch (backend) {
        case VITTE_CONFIG_BACKEND_DEFAULT:
            return "default";

        case VITTE_CONFIG_BACKEND_C17:
            return "c17";

        case VITTE_CONFIG_BACKEND_NATIVE:
            return "native";

        case VITTE_CONFIG_BACKEND_BYTECODE:
            return "bytecode";

        case VITTE_CONFIG_BACKEND_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_config_optimization_name(
    vitte_config_optimization_t optimization)
{
    switch (optimization) {
        case VITTE_CONFIG_OPTIMIZATION_DEFAULT:
            return "default";

        case VITTE_CONFIG_OPTIMIZATION_NONE:
            return "none";

        case VITTE_CONFIG_OPTIMIZATION_DEBUG:
            return "debug";

        case VITTE_CONFIG_OPTIMIZATION_BALANCED:
            return "balanced";

        case VITTE_CONFIG_OPTIMIZATION_SPEED:
            return "speed";

        case VITTE_CONFIG_OPTIMIZATION_SIZE:
            return "size";

        case VITTE_CONFIG_OPTIMIZATION_AGGRESSIVE:
            return "aggressive";

        case VITTE_CONFIG_OPTIMIZATION_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_config_diagnostic_format_name(
    vitte_config_diagnostic_format_t format)
{
    switch (format) {
        case VITTE_CONFIG_DIAGNOSTIC_DEFAULT:
            return "default";

        case VITTE_CONFIG_DIAGNOSTIC_TERMINAL:
            return "terminal";

        case VITTE_CONFIG_DIAGNOSTIC_SHORT:
            return "short";

        case VITTE_CONFIG_DIAGNOSTIC_JSON:
            return "json";

        case VITTE_CONFIG_DIAGNOSTIC_SARIF:
            return "sarif";

        case VITTE_CONFIG_DIAGNOSTIC_LSP:
            return "lsp";

        case VITTE_CONFIG_DIAGNOSTIC_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_config_color_name(
    vitte_config_color_t color)
{
    switch (color) {
        case VITTE_CONFIG_COLOR_AUTO:
            return "auto";

        case VITTE_CONFIG_COLOR_ALWAYS:
            return "always";

        case VITTE_CONFIG_COLOR_NEVER:
            return "never";

        case VITTE_CONFIG_COLOR_COUNT:
            return "count";
    }

    return "invalid";
}

const char *
vitte_config_key_name(
    vitte_config_key_t key)
{
    switch (key) {
        case VITTE_CONFIG_KEY_INVALID:
            return "invalid";

        case VITTE_CONFIG_KEY_PROFILE:
            return "profile";

        case VITTE_CONFIG_KEY_BACKEND:
            return "backend";

        case VITTE_CONFIG_KEY_OPTIMIZATION:
            return "optimization";

        case VITTE_CONFIG_KEY_TARGET:
            return "target";

        case VITTE_CONFIG_KEY_OUTPUT:
            return "output";

        case VITTE_CONFIG_KEY_MANIFEST_PATH:
            return "manifest-path";

        case VITTE_CONFIG_KEY_PROJECT_ROOT:
            return "project-root";

        case VITTE_CONFIG_KEY_BUILD_DIR:
            return "build-dir";

        case VITTE_CONFIG_KEY_CACHE_DIR:
            return "cache-dir";

        case VITTE_CONFIG_KEY_SYSROOT:
            return "sysroot";

        case VITTE_CONFIG_KEY_REGISTRY:
            return "registry";

        case VITTE_CONFIG_KEY_JOBS:
            return "jobs";

        case VITTE_CONFIG_KEY_MAX_ERRORS:
            return "max-errors";

        case VITTE_CONFIG_KEY_MAX_OUTPUT_BYTES:
            return "max-output-bytes";

        case VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT:
            return "diagnostic-format";

        case VITTE_CONFIG_KEY_COLOR:
            return "color";

        case VITTE_CONFIG_KEY_VERBOSE:
            return "verbose";

        case VITTE_CONFIG_KEY_QUIET:
            return "quiet";

        case VITTE_CONFIG_KEY_WARNINGS_AS_ERRORS:
            return "warnings-as-errors";

        case VITTE_CONFIG_KEY_EMIT_DEBUG_INFO:
            return "emit-debug-info";

        case VITTE_CONFIG_KEY_EMIT_SOURCE_MAP:
            return "emit-source-map";

        case VITTE_CONFIG_KEY_EMIT_COMMENTS:
            return "emit-comments";

        case VITTE_CONFIG_KEY_RUNTIME_CHECKS:
            return "runtime-checks";

        case VITTE_CONFIG_KEY_BOUNDS_CHECKS:
            return "bounds-checks";

        case VITTE_CONFIG_KEY_NULL_CHECKS:
            return "null-checks";

        case VITTE_CONFIG_KEY_OVERFLOW_CHECKS:
            return "overflow-checks";

        case VITTE_CONFIG_KEY_DETERMINISTIC:
            return "deterministic";

        case VITTE_CONFIG_KEY_INCREMENTAL:
            return "incremental";

        case VITTE_CONFIG_KEY_OFFLINE:
            return "offline";

        case VITTE_CONFIG_KEY_LOCKED:
            return "locked";

        case VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES:
            return "frozen-dependencies";

        case VITTE_CONFIG_KEY_FORCE:
            return "force";

        case VITTE_CONFIG_KEY_COUNT:
            return "count";
    }

    return "invalid";
}

/* ========================================================================= */
/* Key metadata                                                              */
/* ========================================================================= */

static vitte_config_value_type_t
vitte_config_key_expected_type(
    vitte_config_key_t key)
{
    switch (key) {
        case VITTE_CONFIG_KEY_PROFILE:
        case VITTE_CONFIG_KEY_BACKEND:
        case VITTE_CONFIG_KEY_OPTIMIZATION:
        case VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT:
        case VITTE_CONFIG_KEY_COLOR:
            return VITTE_CONFIG_VALUE_U64;

        case VITTE_CONFIG_KEY_TARGET:
        case VITTE_CONFIG_KEY_OUTPUT:
        case VITTE_CONFIG_KEY_MANIFEST_PATH:
        case VITTE_CONFIG_KEY_PROJECT_ROOT:
        case VITTE_CONFIG_KEY_BUILD_DIR:
        case VITTE_CONFIG_KEY_CACHE_DIR:
        case VITTE_CONFIG_KEY_SYSROOT:
        case VITTE_CONFIG_KEY_REGISTRY:
            return VITTE_CONFIG_VALUE_STRING;

        case VITTE_CONFIG_KEY_JOBS:
        case VITTE_CONFIG_KEY_MAX_ERRORS:
        case VITTE_CONFIG_KEY_MAX_OUTPUT_BYTES:
            return VITTE_CONFIG_VALUE_SIZE;

        case VITTE_CONFIG_KEY_VERBOSE:
        case VITTE_CONFIG_KEY_QUIET:
        case VITTE_CONFIG_KEY_WARNINGS_AS_ERRORS:
        case VITTE_CONFIG_KEY_EMIT_DEBUG_INFO:
        case VITTE_CONFIG_KEY_EMIT_SOURCE_MAP:
        case VITTE_CONFIG_KEY_EMIT_COMMENTS:
        case VITTE_CONFIG_KEY_RUNTIME_CHECKS:
        case VITTE_CONFIG_KEY_BOUNDS_CHECKS:
        case VITTE_CONFIG_KEY_NULL_CHECKS:
        case VITTE_CONFIG_KEY_OVERFLOW_CHECKS:
        case VITTE_CONFIG_KEY_DETERMINISTIC:
        case VITTE_CONFIG_KEY_INCREMENTAL:
        case VITTE_CONFIG_KEY_OFFLINE:
        case VITTE_CONFIG_KEY_LOCKED:
        case VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES:
        case VITTE_CONFIG_KEY_FORCE:
            return VITTE_CONFIG_VALUE_BOOL;

        case VITTE_CONFIG_KEY_INVALID:
        case VITTE_CONFIG_KEY_COUNT:
            return VITTE_CONFIG_VALUE_INVALID;
    }

    return VITTE_CONFIG_VALUE_INVALID;
}

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

vitte_config_resolved_t
vitte_config_resolved_default(void)
{
    vitte_config_resolved_t resolved;

    memset(
        &resolved,
        0,
        sizeof(resolved));

    resolved.profile =
        VITTE_CONFIG_PROFILE_DEFAULT;

    resolved.backend =
        VITTE_CONFIG_BACKEND_DEFAULT;

    resolved.optimization =
        VITTE_CONFIG_OPTIMIZATION_DEFAULT;

    resolved.jobs =
        VITTE_CONFIG_DEFAULT_JOBS;

    resolved.max_errors =
        VITTE_CONFIG_DEFAULT_MAX_ERRORS;

    resolved.max_output_bytes =
        VITTE_CONFIG_DEFAULT_MAX_OUTPUT_BYTES;

    resolved.diagnostic_format =
        VITTE_CONFIG_DIAGNOSTIC_DEFAULT;

    resolved.color =
        VITTE_CONFIG_COLOR_AUTO;

    resolved.runtime_checks = true;
    resolved.bounds_checks = true;
    resolved.null_checks = true;
    resolved.overflow_checks = true;

    resolved.deterministic = true;
    resolved.incremental = true;

    return resolved;
}

/* ========================================================================= */
/* Validation helpers                                                        */
/* ========================================================================= */

static bool
vitte_config_source_is_valid(
    vitte_config_source_t source)
{
    return
        source >= VITTE_CONFIG_SOURCE_DEFAULT &&
        source < VITTE_CONFIG_SOURCE_COUNT;
}

static bool
vitte_config_key_is_valid(
    vitte_config_key_t key)
{
    return
        key > VITTE_CONFIG_KEY_INVALID &&
        key < VITTE_CONFIG_KEY_COUNT;
}

static bool
vitte_config_value_type_is_valid(
    vitte_config_value_type_t type)
{
    return
        type > VITTE_CONFIG_VALUE_INVALID &&
        type < VITTE_CONFIG_VALUE_COUNT;
}

static bool
vitte_config_profile_is_valid(
    vitte_config_profile_t profile)
{
    return
        profile >= VITTE_CONFIG_PROFILE_DEFAULT &&
        profile < VITTE_CONFIG_PROFILE_COUNT;
}

static bool
vitte_config_backend_is_valid(
    vitte_config_backend_t backend)
{
    return
        backend >= VITTE_CONFIG_BACKEND_DEFAULT &&
        backend < VITTE_CONFIG_BACKEND_COUNT;
}

static bool
vitte_config_optimization_is_valid(
    vitte_config_optimization_t optimization)
{
    return
        optimization >=
            VITTE_CONFIG_OPTIMIZATION_DEFAULT &&
        optimization <
            VITTE_CONFIG_OPTIMIZATION_COUNT;
}

static bool
vitte_config_diagnostic_format_is_valid(
    vitte_config_diagnostic_format_t format)
{
    return
        format >= VITTE_CONFIG_DIAGNOSTIC_DEFAULT &&
        format < VITTE_CONFIG_DIAGNOSTIC_COUNT;
}

static bool
vitte_config_color_is_valid(
    vitte_config_color_t color)
{
    return
        color >= VITTE_CONFIG_COLOR_AUTO &&
        color < VITTE_CONFIG_COLOR_COUNT;
}

/* ========================================================================= */
/* Failure                                                                   */
/* ========================================================================= */

static bool
vitte_config_fail(
    vitte_config_t *config,
    vitte_config_error_t error,
    vitte_config_key_t key,
    vitte_config_origin_t origin,
    const char *detail)
{
    if (config == NULL) {
        return false;
    }

    config->last_error = error;
    config->state = VITTE_CONFIG_STATE_FAILED;

    config->message.error = error;
    config->message.key = key;
    config->message.origin = origin;
    config->message.detail = detail;

    return false;
}

/* ========================================================================= */
/* Structural validation                                                     */
/* ========================================================================= */

bool
vitte_config_is_valid(
    const vitte_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->magic !=
        VITTE_CONFIG_MAGIC) {
        return false;
    }

    if (config->state <=
            VITTE_CONFIG_STATE_INVALID ||
        config->state >=
            VITTE_CONFIG_STATE_DESTROYED) {
        return false;
    }

    if (config->entry_count >
        config->entry_capacity) {
        return false;
    }

    if (config->entry_count != 0u &&
        config->entries == NULL) {
        return false;
    }

    if (config->max_entries == 0u ||
        config->max_string_bytes == 0u) {
        return false;
    }

    if (config->entry_count >
        config->max_entries) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

static bool
vitte_config_reserve_entries(
    vitte_config_t *config,
    size_t required)
{
    vitte_config_entry_t *entries;
    size_t capacity;
    size_t bytes;

    if (required <=
        config->entry_capacity) {
        return true;
    }

    if (required >
        config->max_entries) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_TOO_MANY_ENTRIES,
                VITTE_CONFIG_KEY_INVALID,
                (vitte_config_origin_t){0},
                "configuration entry limit exceeded");
    }

    capacity =
        config->entry_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_CONFIG_DEFAULT_INITIAL_CAPACITY;
    }

    while (capacity < required) {
        size_t doubled;

        if (!vitte_config_size_add(
                capacity,
                capacity,
                &doubled)) {
            return
                vitte_config_fail(
                    config,
                    VITTE_CONFIG_ERROR_OVERFLOW,
                    VITTE_CONFIG_KEY_INVALID,
                    (vitte_config_origin_t){0},
                    "configuration capacity overflow");
        }

        capacity = doubled;

        if (capacity >
            config->max_entries) {
            capacity =
                config->max_entries;
            break;
        }
    }

    if (capacity < required) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_TOO_MANY_ENTRIES,
                VITTE_CONFIG_KEY_INVALID,
                (vitte_config_origin_t){0},
                "configuration entry limit exceeded");
    }

    if (!vitte_config_size_mul(
            capacity,
            sizeof(*entries),
            &bytes)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_OVERFLOW,
                VITTE_CONFIG_KEY_INVALID,
                (vitte_config_origin_t){0},
                "configuration allocation size overflow");
    }

    entries =
        (vitte_config_entry_t *)realloc(
            config->entries,
            bytes);

    if (entries == NULL) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_OUT_OF_MEMORY,
                VITTE_CONFIG_KEY_INVALID,
                (vitte_config_origin_t){0},
                "unable to grow configuration storage");
    }

    config->entries = entries;
    config->entry_capacity = capacity;

    return true;
}

/* ========================================================================= */
/* Entry lookup                                                              */
/* ========================================================================= */

static vitte_config_entry_t *
vitte_config_find_entry_mutable(
    vitte_config_t *config,
    vitte_config_key_t key)
{
    size_t index;

    for (index = 0u;
         index < config->entry_count;
         ++index) {
        if (config->entries[index].key ==
            key) {
            return &config->entries[index];
        }
    }

    return NULL;
}

const vitte_config_entry_t *
vitte_config_find_entry(
    const vitte_config_t *config,
    vitte_config_key_t key)
{
    size_t index;

    if (!vitte_config_is_valid(config) ||
        !vitte_config_key_is_valid(key)) {
        return NULL;
    }

    for (index = 0u;
         index < config->entry_count;
         ++index) {
        if (config->entries[index].key ==
            key) {
            return &config->entries[index];
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Value comparison                                                          */
/* ========================================================================= */

static bool
vitte_config_value_equal(
    const vitte_config_value_t *left,
    const vitte_config_value_t *right)
{
    if (left == NULL ||
        right == NULL ||
        left->type != right->type) {
        return false;
    }

    switch (left->type) {
        case VITTE_CONFIG_VALUE_BOOL:
            return
                left->as.boolean ==
                right->as.boolean;

        case VITTE_CONFIG_VALUE_SIZE:
            return
                left->as.size ==
                right->as.size;

        case VITTE_CONFIG_VALUE_U64:
            return
                left->as.u64 ==
                right->as.u64;

        case VITTE_CONFIG_VALUE_STRING:
            if (left->as.string == NULL ||
                right->as.string == NULL) {
                return
                    left->as.string ==
                    right->as.string;
            }

            return
                vitte_config_string_equal(
                    left->as.string,
                    right->as.string);

        case VITTE_CONFIG_VALUE_INVALID:
        case VITTE_CONFIG_VALUE_COUNT:
            return false;
    }

    return false;
}

/* ========================================================================= */
/* Source precedence                                                         */
/* ========================================================================= */

static unsigned
vitte_config_source_precedence(
    vitte_config_source_t source)
{
    switch (source) {
        case VITTE_CONFIG_SOURCE_DEFAULT:
            return 0u;

        case VITTE_CONFIG_SOURCE_SYSTEM:
            return 10u;

        case VITTE_CONFIG_SOURCE_USER:
            return 20u;

        case VITTE_CONFIG_SOURCE_PROJECT:
            return 30u;

        case VITTE_CONFIG_SOURCE_ENVIRONMENT:
            return 40u;

        case VITTE_CONFIG_SOURCE_COMMAND_LINE:
            return 50u;

        case VITTE_CONFIG_SOURCE_OVERRIDE:
            return 60u;

        case VITTE_CONFIG_SOURCE_COUNT:
            return 0u;
    }

    return 0u;
}

/* ========================================================================= */
/* Value validation                                                          */
/* ========================================================================= */

static bool
vitte_config_validate_value(
    const vitte_config_t *config,
    vitte_config_key_t key,
    const vitte_config_value_t *value)
{
    vitte_config_value_type_t expected;

    if (config == NULL ||
        value == NULL ||
        !vitte_config_key_is_valid(key) ||
        !vitte_config_value_type_is_valid(
            value->type)) {
        return false;
    }

    expected =
        vitte_config_key_expected_type(
            key);

    if (expected != value->type) {
        return false;
    }

    if (value->type ==
        VITTE_CONFIG_VALUE_STRING) {
        size_t length;

        if (value->as.string == NULL) {
            return true;
        }

        length =
            strlen(value->as.string);

        if (length >
            config->max_string_bytes) {
            return false;
        }
    }

    switch (key) {
        case VITTE_CONFIG_KEY_PROFILE:
            return
                value->as.u64 <
                    (uint64_t)
                        VITTE_CONFIG_PROFILE_COUNT;

        case VITTE_CONFIG_KEY_BACKEND:
            return
                value->as.u64 <
                    (uint64_t)
                        VITTE_CONFIG_BACKEND_COUNT;

        case VITTE_CONFIG_KEY_OPTIMIZATION:
            return
                value->as.u64 <
                    (uint64_t)
                        VITTE_CONFIG_OPTIMIZATION_COUNT;

        case VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT:
            return
                value->as.u64 <
                    (uint64_t)
                        VITTE_CONFIG_DIAGNOSTIC_COUNT;

        case VITTE_CONFIG_KEY_COLOR:
            return
                value->as.u64 <
                    (uint64_t)
                        VITTE_CONFIG_COLOR_COUNT;

        case VITTE_CONFIG_KEY_JOBS:
        case VITTE_CONFIG_KEY_MAX_ERRORS:
            return value->as.size != 0u;

        case VITTE_CONFIG_KEY_TARGET:
            return
                value->as.string != NULL &&
                value->as.string[0] != '\0';

        case VITTE_CONFIG_KEY_OUTPUT:
        case VITTE_CONFIG_KEY_MANIFEST_PATH:
        case VITTE_CONFIG_KEY_PROJECT_ROOT:
        case VITTE_CONFIG_KEY_BUILD_DIR:
        case VITTE_CONFIG_KEY_CACHE_DIR:
        case VITTE_CONFIG_KEY_SYSROOT:
        case VITTE_CONFIG_KEY_REGISTRY:
        case VITTE_CONFIG_KEY_MAX_OUTPUT_BYTES:
        case VITTE_CONFIG_KEY_VERBOSE:
        case VITTE_CONFIG_KEY_QUIET:
        case VITTE_CONFIG_KEY_WARNINGS_AS_ERRORS:
        case VITTE_CONFIG_KEY_EMIT_DEBUG_INFO:
        case VITTE_CONFIG_KEY_EMIT_SOURCE_MAP:
        case VITTE_CONFIG_KEY_EMIT_COMMENTS:
        case VITTE_CONFIG_KEY_RUNTIME_CHECKS:
        case VITTE_CONFIG_KEY_BOUNDS_CHECKS:
        case VITTE_CONFIG_KEY_NULL_CHECKS:
        case VITTE_CONFIG_KEY_OVERFLOW_CHECKS:
        case VITTE_CONFIG_KEY_DETERMINISTIC:
        case VITTE_CONFIG_KEY_INCREMENTAL:
        case VITTE_CONFIG_KEY_OFFLINE:
        case VITTE_CONFIG_KEY_LOCKED:
        case VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES:
        case VITTE_CONFIG_KEY_FORCE:
            return true;

        case VITTE_CONFIG_KEY_INVALID:
        case VITTE_CONFIG_KEY_COUNT:
            return false;
    }

    return false;
}

/* ========================================================================= */
/* Resolved assignment                                                       */
/* ========================================================================= */

static bool
vitte_config_apply_to_resolved(
    vitte_config_resolved_t *resolved,
    vitte_config_key_t key,
    const vitte_config_value_t *value)
{
    if (resolved == NULL ||
        value == NULL) {
        return false;
    }

    switch (key) {
        case VITTE_CONFIG_KEY_PROFILE:
            resolved->profile =
                (vitte_config_profile_t)
                    value->as.u64;
            return true;

        case VITTE_CONFIG_KEY_BACKEND:
            resolved->backend =
                (vitte_config_backend_t)
                    value->as.u64;
            return true;

        case VITTE_CONFIG_KEY_OPTIMIZATION:
            resolved->optimization =
                (vitte_config_optimization_t)
                    value->as.u64;
            return true;

        case VITTE_CONFIG_KEY_TARGET:
            resolved->target =
                value->as.string;
            return true;

        case VITTE_CONFIG_KEY_OUTPUT:
            resolved->output =
                value->as.string;
            return true;

        case VITTE_CONFIG_KEY_MANIFEST_PATH:
            resolved->manifest_path =
                value->as.string;
            return true;

        case VITTE_CONFIG_KEY_PROJECT_ROOT:
            resolved->project_root =
                value->as.string;
            return true;

        case VITTE_CONFIG_KEY_BUILD_DIR:
            resolved->build_dir =
                value->as.string;
            return true;

        case VITTE_CONFIG_KEY_CACHE_DIR:
            resolved->cache_dir =
                value->as.string;
            return true;

        case VITTE_CONFIG_KEY_SYSROOT:
            resolved->sysroot =
                value->as.string;
            return true;

        case VITTE_CONFIG_KEY_REGISTRY:
            resolved->registry =
                value->as.string;
            return true;

        case VITTE_CONFIG_KEY_JOBS:
            resolved->jobs =
                value->as.size;
            return true;

        case VITTE_CONFIG_KEY_MAX_ERRORS:
            resolved->max_errors =
                value->as.size;
            return true;

        case VITTE_CONFIG_KEY_MAX_OUTPUT_BYTES:
            resolved->max_output_bytes =
                value->as.size;
            return true;

        case VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT:
            resolved->diagnostic_format =
                (vitte_config_diagnostic_format_t)
                    value->as.u64;
            return true;

        case VITTE_CONFIG_KEY_COLOR:
            resolved->color =
                (vitte_config_color_t)
                    value->as.u64;
            return true;

        case VITTE_CONFIG_KEY_VERBOSE:
            resolved->verbose =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_QUIET:
            resolved->quiet =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_WARNINGS_AS_ERRORS:
            resolved->warnings_as_errors =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_EMIT_DEBUG_INFO:
            resolved->emit_debug_info =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_EMIT_SOURCE_MAP:
            resolved->emit_source_map =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_EMIT_COMMENTS:
            resolved->emit_comments =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_RUNTIME_CHECKS:
            resolved->runtime_checks =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_BOUNDS_CHECKS:
            resolved->bounds_checks =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_NULL_CHECKS:
            resolved->null_checks =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_OVERFLOW_CHECKS:
            resolved->overflow_checks =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_DETERMINISTIC:
            resolved->deterministic =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_INCREMENTAL:
            resolved->incremental =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_OFFLINE:
            resolved->offline =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_LOCKED:
            resolved->locked =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES:
            resolved->frozen_dependencies =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_FORCE:
            resolved->force =
                value->as.boolean;
            return true;

        case VITTE_CONFIG_KEY_INVALID:
        case VITTE_CONFIG_KEY_COUNT:
            return false;
    }

    return false;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_config_init(
    vitte_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    memset(
        config,
        0,
        sizeof(*config));

    config->magic =
        VITTE_CONFIG_MAGIC;

    config->state =
        VITTE_CONFIG_STATE_READY;

    config->last_error =
        VITTE_CONFIG_ERROR_NONE;

    config->max_entries =
        VITTE_CONFIG_DEFAULT_MAX_ENTRIES;

    config->max_string_bytes =
        VITTE_CONFIG_DEFAULT_MAX_STRING_BYTES;

    config->resolved =
        vitte_config_resolved_default();

    config->next_sequence =
        UINT64_C(1);

    config->generation =
        UINT64_C(1);

    return true;
}

void
vitte_config_destroy(
    vitte_config_t *config)
{
    if (config == NULL ||
        config->magic !=
            VITTE_CONFIG_MAGIC) {
        return;
    }

    free(config->entries);

    config->entries = NULL;
    config->entry_count = 0u;
    config->entry_capacity = 0u;

    memset(
        &config->resolved,
        0,
        sizeof(config->resolved));

    memset(
        &config->message,
        0,
        sizeof(config->message));

    memset(
        &config->stats,
        0,
        sizeof(config->stats));

    config->last_error =
        VITTE_CONFIG_ERROR_NONE;

    config->state =
        VITTE_CONFIG_STATE_DESTROYED;

    config->magic =
        VITTE_CONFIG_DEAD_MAGIC;
}

bool
vitte_config_reset(
    vitte_config_t *config)
{
    if (config == NULL ||
        config->magic !=
            VITTE_CONFIG_MAGIC) {
        return false;
    }

    if (config->state ==
        VITTE_CONFIG_STATE_MERGING) {
        return false;
    }

    config->entry_count = 0u;

    config->resolved =
        vitte_config_resolved_default();

    memset(
        &config->message,
        0,
        sizeof(config->message));

    memset(
        &config->stats,
        0,
        sizeof(config->stats));

    config->last_error =
        VITTE_CONFIG_ERROR_NONE;

    config->next_sequence =
        UINT64_C(1);

    if (config->generation !=
        UINT64_MAX) {
        ++config->generation;
    }

    config->state =
        VITTE_CONFIG_STATE_READY;

    return true;
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_config_set_limits(
    vitte_config_t *config,
    size_t max_entries,
    size_t max_string_bytes)
{
    if (!vitte_config_is_valid(config) ||
        config->state !=
            VITTE_CONFIG_STATE_READY ||
        max_entries == 0u ||
        max_string_bytes == 0u ||
        max_entries <
            config->entry_count) {
        return false;
    }

    config->max_entries =
        max_entries;

    config->max_string_bytes =
        max_string_bytes;

    return true;
}

/* ========================================================================= */
/* Generic assignment                                                        */
/* ========================================================================= */

bool
vitte_config_set(
    vitte_config_t *config,
    vitte_config_key_t key,
    vitte_config_value_t value,
    vitte_config_origin_t origin)
{
    vitte_config_entry_t *existing;
    unsigned old_precedence;
    unsigned new_precedence;
    size_t required;

    if (!vitte_config_is_valid(config)) {
        return false;
    }

    if (config->state ==
        VITTE_CONFIG_STATE_FROZEN) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_FROZEN,
                key,
                origin,
                "configuration is frozen");
    }

    if (config->state !=
            VITTE_CONFIG_STATE_READY &&
        config->state !=
            VITTE_CONFIG_STATE_MERGING &&
        config->state !=
            VITTE_CONFIG_STATE_VALIDATED) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_STATE,
                key,
                origin,
                "configuration cannot be modified in current state");
    }

    if (!vitte_config_key_is_valid(key)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_KEY,
                key,
                origin,
                "invalid configuration key");
    }

    if (!vitte_config_source_is_valid(
            origin.source)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_SOURCE,
                key,
                origin,
                "invalid configuration source");
    }

    if (!vitte_config_validate_value(
            config,
            key,
            &value)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_VALUE,
                key,
                origin,
                "invalid configuration value");
    }

    config->state =
        VITTE_CONFIG_STATE_MERGING;

    existing =
        vitte_config_find_entry_mutable(
            config,
            key);

    if (existing != NULL) {
        old_precedence =
            vitte_config_source_precedence(
                existing->origin.source);

        new_precedence =
            vitte_config_source_precedence(
                origin.source);

        if (new_precedence <
            old_precedence) {
            ++config->stats
                .ignored_lower_precedence_count;

            config->state =
                VITTE_CONFIG_STATE_READY;

            config->last_error =
                VITTE_CONFIG_ERROR_NONE;

            return true;
        }

        if (new_precedence ==
                old_precedence &&
            existing->origin.source !=
                VITTE_CONFIG_SOURCE_DEFAULT &&
            !vitte_config_value_equal(
                &existing->value,
                &value)) {
            return
                vitte_config_fail(
                    config,
                    VITTE_CONFIG_ERROR_CONFLICT,
                    key,
                    origin,
                    "conflicting values from equal-precedence configuration sources");
        }

        if (vitte_config_value_equal(
                &existing->value,
                &value)) {
            /*
             * Identical value: keep the highest/newest provenance when the
             * incoming source is at least as strong.
             */
            if (new_precedence >=
                old_precedence) {
                existing->origin = origin;
                existing->sequence =
                    config->next_sequence;

                if (config->next_sequence !=
                    UINT64_MAX) {
                    ++config->next_sequence;
                }
            }

            config->state =
                VITTE_CONFIG_STATE_READY;

            config->last_error =
                VITTE_CONFIG_ERROR_NONE;

            return true;
        }

        existing->value = value;
        existing->origin = origin;
        existing->sequence =
            config->next_sequence;
        existing->explicitly_set =
            origin.source !=
                VITTE_CONFIG_SOURCE_DEFAULT;

        if (config->next_sequence !=
            UINT64_MAX) {
            ++config->next_sequence;
        }

        ++config->stats
            .overridden_entry_count;

        if (!vitte_config_apply_to_resolved(
                &config->resolved,
                key,
                &value)) {
            return
                vitte_config_fail(
                    config,
                    VITTE_CONFIG_ERROR_CORRUPTION,
                    key,
                    origin,
                    "failed to update resolved configuration");
        }

        config->state =
            VITTE_CONFIG_STATE_READY;

        config->last_error =
            VITTE_CONFIG_ERROR_NONE;

        return true;
    }

    if (!vitte_config_size_add(
            config->entry_count,
            1u,
            &required)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_OVERFLOW,
                key,
                origin,
                "configuration entry count overflow");
    }

    if (!vitte_config_reserve_entries(
            config,
            required)) {
        return false;
    }

    config->entries[
        config->entry_count].key =
            key;

    config->entries[
        config->entry_count].value =
            value;

    config->entries[
        config->entry_count].origin =
            origin;

    config->entries[
        config->entry_count].sequence =
            config->next_sequence;

    config->entries[
        config->entry_count].explicitly_set =
            origin.source !=
                VITTE_CONFIG_SOURCE_DEFAULT;

    if (config->next_sequence !=
        UINT64_MAX) {
        ++config->next_sequence;
    }

    ++config->entry_count;

    if (!vitte_config_apply_to_resolved(
            &config->resolved,
            key,
            &value)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_CORRUPTION,
                key,
                origin,
                "failed to update resolved configuration");
    }

    config->state =
        VITTE_CONFIG_STATE_READY;

    config->last_error =
        VITTE_CONFIG_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Typed value constructors                                                  */
/* ========================================================================= */

vitte_config_value_t
vitte_config_value_bool(
    bool value)
{
    vitte_config_value_t result;

    memset(
        &result,
        0,
        sizeof(result));

    result.type =
        VITTE_CONFIG_VALUE_BOOL;

    result.as.boolean =
        value;

    return result;
}

vitte_config_value_t
vitte_config_value_size(
    size_t value)
{
    vitte_config_value_t result;

    memset(
        &result,
        0,
        sizeof(result));

    result.type =
        VITTE_CONFIG_VALUE_SIZE;

    result.as.size =
        value;

    return result;
}

vitte_config_value_t
vitte_config_value_u64(
    uint64_t value)
{
    vitte_config_value_t result;

    memset(
        &result,
        0,
        sizeof(result));

    result.type =
        VITTE_CONFIG_VALUE_U64;

    result.as.u64 =
        value;

    return result;
}

vitte_config_value_t
vitte_config_value_string(
    const char *value)
{
    vitte_config_value_t result;

    memset(
        &result,
        0,
        sizeof(result));

    result.type =
        VITTE_CONFIG_VALUE_STRING;

    result.as.string =
        value;

    return result;
}

/* ========================================================================= */
/* Typed setters                                                             */
/* ========================================================================= */

bool
vitte_config_set_bool(
    vitte_config_t *config,
    vitte_config_key_t key,
    bool value,
    vitte_config_origin_t origin)
{
    return
        vitte_config_set(
            config,
            key,
            vitte_config_value_bool(value),
            origin);
}

bool
vitte_config_set_size(
    vitte_config_t *config,
    vitte_config_key_t key,
    size_t value,
    vitte_config_origin_t origin)
{
    return
        vitte_config_set(
            config,
            key,
            vitte_config_value_size(value),
            origin);
}

bool
vitte_config_set_u64(
    vitte_config_t *config,
    vitte_config_key_t key,
    uint64_t value,
    vitte_config_origin_t origin)
{
    return
        vitte_config_set(
            config,
            key,
            vitte_config_value_u64(value),
            origin);
}

bool
vitte_config_set_string(
    vitte_config_t *config,
    vitte_config_key_t key,
    const char *value,
    vitte_config_origin_t origin)
{
    return
        vitte_config_set(
            config,
            key,
            vitte_config_value_string(value),
            origin);
}

/* ========================================================================= */
/* Specialized setters                                                       */
/* ========================================================================= */

bool
vitte_config_set_profile(
    vitte_config_t *config,
    vitte_config_profile_t profile,
    vitte_config_origin_t origin)
{
    if (!vitte_config_profile_is_valid(
            profile)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_PROFILE,
                VITTE_CONFIG_KEY_PROFILE,
                origin,
                "invalid build profile");
    }

    return
        vitte_config_set_u64(
            config,
            VITTE_CONFIG_KEY_PROFILE,
            (uint64_t)profile,
            origin);
}

bool
vitte_config_set_backend(
    vitte_config_t *config,
    vitte_config_backend_t backend,
    vitte_config_origin_t origin)
{
    if (!vitte_config_backend_is_valid(
            backend)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_BACKEND,
                VITTE_CONFIG_KEY_BACKEND,
                origin,
                "invalid code-generation backend");
    }

    return
        vitte_config_set_u64(
            config,
            VITTE_CONFIG_KEY_BACKEND,
            (uint64_t)backend,
            origin);
}

bool
vitte_config_set_optimization(
    vitte_config_t *config,
    vitte_config_optimization_t optimization,
    vitte_config_origin_t origin)
{
    if (!vitte_config_optimization_is_valid(
            optimization)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_OPTIMIZATION,
                VITTE_CONFIG_KEY_OPTIMIZATION,
                origin,
                "invalid optimization policy");
    }

    return
        vitte_config_set_u64(
            config,
            VITTE_CONFIG_KEY_OPTIMIZATION,
            (uint64_t)optimization,
            origin);
}

bool
vitte_config_set_diagnostic_format(
    vitte_config_t *config,
    vitte_config_diagnostic_format_t format,
    vitte_config_origin_t origin)
{
    if (!vitte_config_diagnostic_format_is_valid(
            format)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_DIAGNOSTIC_FORMAT,
                VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT,
                origin,
                "invalid diagnostic format");
    }

    return
        vitte_config_set_u64(
            config,
            VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT,
            (uint64_t)format,
            origin);
}

bool
vitte_config_set_color(
    vitte_config_t *config,
    vitte_config_color_t color,
    vitte_config_origin_t origin)
{
    if (!vitte_config_color_is_valid(
            color)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_COLOR_MODE,
                VITTE_CONFIG_KEY_COLOR,
                origin,
                "invalid color mode");
    }

    return
        vitte_config_set_u64(
            config,
            VITTE_CONFIG_KEY_COLOR,
            (uint64_t)color,
            origin);
}

/* ========================================================================= */
/* Profile application                                                       */
/* ========================================================================= */

bool
vitte_config_apply_profile(
    vitte_config_t *config,
    vitte_config_profile_t profile,
    vitte_config_origin_t origin)
{
    if (!vitte_config_profile_is_valid(
            profile)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_PROFILE,
                VITTE_CONFIG_KEY_PROFILE,
                origin,
                "invalid build profile");
    }

    if (!vitte_config_set_profile(
            config,
            profile,
            origin)) {
        return false;
    }

    switch (profile) {
        case VITTE_CONFIG_PROFILE_DEFAULT:
            return true;

        case VITTE_CONFIG_PROFILE_DEBUG:
            if (!vitte_config_set_optimization(
                    config,
                    VITTE_CONFIG_OPTIMIZATION_DEBUG,
                    origin)) {
                return false;
            }

            if (!vitte_config_set_bool(
                    config,
                    VITTE_CONFIG_KEY_EMIT_DEBUG_INFO,
                    true,
                    origin)) {
                return false;
            }

            return true;

        case VITTE_CONFIG_PROFILE_RELEASE:
            if (!vitte_config_set_optimization(
                    config,
                    VITTE_CONFIG_OPTIMIZATION_SPEED,
                    origin)) {
                return false;
            }

            /*
             * Release does not disable language safety checks.
             */
            return true;

        case VITTE_CONFIG_PROFILE_SIZE:
            return
                vitte_config_set_optimization(
                    config,
                    VITTE_CONFIG_OPTIMIZATION_SIZE,
                    origin);

        case VITTE_CONFIG_PROFILE_SANITIZE:
            if (!vitte_config_set_optimization(
                    config,
                    VITTE_CONFIG_OPTIMIZATION_DEBUG,
                    origin)) {
                return false;
            }

            if (!vitte_config_set_bool(
                    config,
                    VITTE_CONFIG_KEY_EMIT_DEBUG_INFO,
                    true,
                    origin)) {
                return false;
            }

            if (!vitte_config_set_bool(
                    config,
                    VITTE_CONFIG_KEY_RUNTIME_CHECKS,
                    true,
                    origin)) {
                return false;
            }

            if (!vitte_config_set_bool(
                    config,
                    VITTE_CONFIG_KEY_BOUNDS_CHECKS,
                    true,
                    origin)) {
                return false;
            }

            if (!vitte_config_set_bool(
                    config,
                    VITTE_CONFIG_KEY_NULL_CHECKS,
                    true,
                    origin)) {
                return false;
            }

            return
                vitte_config_set_bool(
                    config,
                    VITTE_CONFIG_KEY_OVERFLOW_CHECKS,
                    true,
                    origin);

        case VITTE_CONFIG_PROFILE_FREESTANDING:
            /*
             * The target subsystem determines the concrete freestanding ABI.
             * The generic config engine only adjusts broad build policy.
             */
            return
                vitte_config_set_bool(
                    config,
                    VITTE_CONFIG_KEY_INCREMENTAL,
                    false,
                    origin);

        case VITTE_CONFIG_PROFILE_COUNT:
            return false;
    }

    return false;
}

/* ========================================================================= */
/* Resolved semantic validation                                              */
/* ========================================================================= */

bool
vitte_config_validate(
    vitte_config_t *config)
{
    vitte_config_origin_t origin;

    if (!vitte_config_is_valid(config)) {
        return false;
    }

    if (config->state ==
        VITTE_CONFIG_STATE_FROZEN) {
        return true;
    }

    if (config->state !=
            VITTE_CONFIG_STATE_READY &&
        config->state !=
            VITTE_CONFIG_STATE_VALIDATED) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_STATE,
                VITTE_CONFIG_KEY_INVALID,
                (vitte_config_origin_t){0},
                "configuration cannot be validated in current state");
    }

    memset(
        &origin,
        0,
        sizeof(origin));

    origin.source =
        VITTE_CONFIG_SOURCE_OVERRIDE;

    origin.name =
        "semantic-validation";

    if (!vitte_config_profile_is_valid(
            config->resolved.profile)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_PROFILE,
                VITTE_CONFIG_KEY_PROFILE,
                origin,
                "resolved profile is invalid");
    }

    if (!vitte_config_backend_is_valid(
            config->resolved.backend)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_BACKEND,
                VITTE_CONFIG_KEY_BACKEND,
                origin,
                "resolved backend is invalid");
    }

    if (!vitte_config_optimization_is_valid(
            config->resolved.optimization)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_OPTIMIZATION,
                VITTE_CONFIG_KEY_OPTIMIZATION,
                origin,
                "resolved optimization is invalid");
    }

    if (!vitte_config_diagnostic_format_is_valid(
            config->resolved.diagnostic_format)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_DIAGNOSTIC_FORMAT,
                VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT,
                origin,
                "resolved diagnostic format is invalid");
    }

    if (!vitte_config_color_is_valid(
            config->resolved.color)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_COLOR_MODE,
                VITTE_CONFIG_KEY_COLOR,
                origin,
                "resolved color mode is invalid");
    }

    if (config->resolved.jobs == 0u) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_VALUE,
                VITTE_CONFIG_KEY_JOBS,
                origin,
                "jobs must be greater than zero");
    }

    if (config->resolved.max_errors == 0u) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_INVALID_VALUE,
                VITTE_CONFIG_KEY_MAX_ERRORS,
                origin,
                "max-errors must be greater than zero");
    }

    if (config->resolved.verbose &&
        config->resolved.quiet) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_CONFLICT,
                VITTE_CONFIG_KEY_VERBOSE,
                origin,
                "verbose and quiet modes cannot both be enabled");
    }

    if (config->resolved.frozen_dependencies &&
        (!config->resolved.locked ||
         !config->resolved.offline)) {
        return
            vitte_config_fail(
                config,
                VITTE_CONFIG_ERROR_CONFLICT,
                VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES,
                origin,
                "frozen dependency mode requires locked and offline modes");
    }

    config->state =
        VITTE_CONFIG_STATE_VALIDATED;

    config->last_error =
        VITTE_CONFIG_ERROR_NONE;

    config->stats.validated = true;

    return true;
}

/* ========================================================================= */
/* Freeze                                                                    */
/* ========================================================================= */

bool
vitte_config_freeze(
    vitte_config_t *config)
{
    if (!vitte_config_is_valid(config)) {
        return false;
    }

    if (config->state ==
        VITTE_CONFIG_STATE_FROZEN) {
        return true;
    }

    if (config->state !=
        VITTE_CONFIG_STATE_VALIDATED) {
        if (!vitte_config_validate(
                config)) {
            return false;
        }
    }

    config->state =
        VITTE_CONFIG_STATE_FROZEN;

    config->stats.frozen = true;

    return true;
}

/* ========================================================================= */
/* Frozen dependency convenience                                             */
/* ========================================================================= */

bool
vitte_config_enable_frozen_dependencies(
    vitte_config_t *config,
    vitte_config_origin_t origin)
{
    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES,
            true,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_LOCKED,
            true,
            origin)) {
        return false;
    }

    return
        vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_OFFLINE,
            true,
            origin);
}

/* ========================================================================= */
/* Getters                                                                   */
/* ========================================================================= */

const vitte_config_resolved_t *
vitte_config_resolved(
    const vitte_config_t *config)
{
    if (!vitte_config_is_valid(config)) {
        return NULL;
    }

    return &config->resolved;
}

vitte_config_state_t
vitte_config_state(
    const vitte_config_t *config)
{
    if (config == NULL ||
        config->magic !=
            VITTE_CONFIG_MAGIC) {
        return
            VITTE_CONFIG_STATE_INVALID;
    }

    return config->state;
}

vitte_config_error_t
vitte_config_last_error(
    const vitte_config_t *config)
{
    if (config == NULL ||
        config->magic !=
            VITTE_CONFIG_MAGIC) {
        return
            VITTE_CONFIG_ERROR_INVALID_CONFIG;
    }

    return config->last_error;
}

const vitte_config_message_t *
vitte_config_message(
    const vitte_config_t *config)
{
    if (config == NULL ||
        config->magic !=
            VITTE_CONFIG_MAGIC) {
        return NULL;
    }

    return &config->message;
}

size_t
vitte_config_entry_count(
    const vitte_config_t *config)
{
    if (!vitte_config_is_valid(config)) {
        return 0u;
    }

    return config->entry_count;
}

const vitte_config_entry_t *
vitte_config_entry_at(
    const vitte_config_t *config,
    size_t index)
{
    if (!vitte_config_is_valid(config) ||
        index >= config->entry_count) {
        return NULL;
    }

    return &config->entries[index];
}

/* ========================================================================= */
/* Presence                                                                  */
/* ========================================================================= */

bool
vitte_config_has(
    const vitte_config_t *config,
    vitte_config_key_t key)
{
    return
        vitte_config_find_entry(
            config,
            key) != NULL;
}

bool
vitte_config_is_explicit(
    const vitte_config_t *config,
    vitte_config_key_t key)
{
    const vitte_config_entry_t *entry;

    entry =
        vitte_config_find_entry(
            config,
            key);

    return
        entry != NULL &&
        entry->explicitly_set;
}

/* ========================================================================= */
/* Provenance                                                                */
/* ========================================================================= */

vitte_config_source_t
vitte_config_source_of(
    const vitte_config_t *config,
    vitte_config_key_t key)
{
    const vitte_config_entry_t *entry;

    entry =
        vitte_config_find_entry(
            config,
            key);

    if (entry == NULL) {
        return
            VITTE_CONFIG_SOURCE_DEFAULT;
    }

    return entry->origin.source;
}

const vitte_config_origin_t *
vitte_config_origin_of(
    const vitte_config_t *config,
    vitte_config_key_t key)
{
    const vitte_config_entry_t *entry;

    entry =
        vitte_config_find_entry(
            config,
            key);

    if (entry == NULL) {
        return NULL;
    }

    return &entry->origin;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

static uint64_t
vitte_config_hash_value(
    uint64_t hash,
    const vitte_config_value_t *value)
{
    hash =
        vitte_config_hash_u64(
            hash,
            (uint64_t)value->type);

    switch (value->type) {
        case VITTE_CONFIG_VALUE_BOOL:
            return
                vitte_config_hash_u64(
                    hash,
                    value->as.boolean
                        ? UINT64_C(1)
                        : UINT64_C(0));

        case VITTE_CONFIG_VALUE_SIZE:
            return
                vitte_config_hash_u64(
                    hash,
                    (uint64_t)value->as.size);

        case VITTE_CONFIG_VALUE_U64:
            return
                vitte_config_hash_u64(
                    hash,
                    value->as.u64);

        case VITTE_CONFIG_VALUE_STRING:
            return
                vitte_config_hash_string(
                    hash,
                    value->as.string);

        case VITTE_CONFIG_VALUE_INVALID:
        case VITTE_CONFIG_VALUE_COUNT:
            return hash;
    }

    return hash;
}

uint64_t
vitte_config_fingerprint(
    const vitte_config_t *config)
{
    uint64_t hash;
    vitte_config_key_t key;

    if (!vitte_config_is_valid(config)) {
        return 0u;
    }

    hash =
        VITTE_CONFIG_FNV_OFFSET;

    /*
     * Hash in canonical key order rather than insertion order.
     *
     * This means equivalent resolved configuration produces the same
     * fingerprint even when configuration layers were applied in a different
     * sequence.
     */
    for (key =
            (vitte_config_key_t)
                (VITTE_CONFIG_KEY_INVALID + 1);
         key < VITTE_CONFIG_KEY_COUNT;
         key =
            (vitte_config_key_t)
                (key + 1)) {
        const vitte_config_entry_t *entry;

        entry =
            vitte_config_find_entry(
                config,
                key);

        if (entry == NULL) {
            continue;
        }

        hash =
            vitte_config_hash_u64(
                hash,
                (uint64_t)key);

        hash =
            vitte_config_hash_value(
                hash,
                &entry->value);
    }

    /*
     * Include defaults from the resolved representation for fields that may
     * not have explicit entries.
     */
    hash =
        vitte_config_hash_u64(
            hash,
            (uint64_t)
                config->resolved.profile);

    hash =
        vitte_config_hash_u64(
            hash,
            (uint64_t)
                config->resolved.backend);

    hash =
        vitte_config_hash_u64(
            hash,
            (uint64_t)
                config->resolved.optimization);

    hash =
        vitte_config_hash_string(
            hash,
            config->resolved.target);

    hash =
        vitte_config_hash_string(
            hash,
            config->resolved.output);

    hash =
        vitte_config_hash_string(
            hash,
            config->resolved.manifest_path);

    hash =
        vitte_config_hash_string(
            hash,
            config->resolved.project_root);

    hash =
        vitte_config_hash_string(
            hash,
            config->resolved.build_dir);

    hash =
        vitte_config_hash_string(
            hash,
            config->resolved.cache_dir);

    hash =
        vitte_config_hash_string(
            hash,
            config->resolved.sysroot);

    hash =
        vitte_config_hash_string(
            hash,
            config->resolved.registry);

    hash =
        vitte_config_hash_u64(
            hash,
            (uint64_t)
                config->resolved.jobs);

    hash =
        vitte_config_hash_u64(
            hash,
            (uint64_t)
                config->resolved.max_errors);

    hash =
        vitte_config_hash_u64(
            hash,
            (uint64_t)
                config->resolved.max_output_bytes);

    hash =
        vitte_config_hash_u64(
            hash,
            (uint64_t)
                config->resolved.diagnostic_format);

    hash =
        vitte_config_hash_u64(
            hash,
            (uint64_t)
                config->resolved.color);

#define VITTE_CONFIG_HASH_RESOLVED_BOOL(field)                            \
    do {                                                                  \
        hash = vitte_config_hash_u64(                                     \
            hash,                                                         \
            config->resolved.field                                        \
                ? UINT64_C(1)                                             \
                : UINT64_C(0));                                           \
    } while (0)

    VITTE_CONFIG_HASH_RESOLVED_BOOL(verbose);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(quiet);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(warnings_as_errors);

    VITTE_CONFIG_HASH_RESOLVED_BOOL(emit_debug_info);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(emit_source_map);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(emit_comments);

    VITTE_CONFIG_HASH_RESOLVED_BOOL(runtime_checks);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(bounds_checks);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(null_checks);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(overflow_checks);

    VITTE_CONFIG_HASH_RESOLVED_BOOL(deterministic);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(incremental);

    VITTE_CONFIG_HASH_RESOLVED_BOOL(offline);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(locked);
    VITTE_CONFIG_HASH_RESOLVED_BOOL(frozen_dependencies);

    VITTE_CONFIG_HASH_RESOLVED_BOOL(force);

#undef VITTE_CONFIG_HASH_RESOLVED_BOOL

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_config_stats_t
vitte_config_stats(
    const vitte_config_t *config)
{
    vitte_config_stats_t stats;
    size_t index;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (!vitte_config_is_valid(config)) {
        return stats;
    }

    stats =
        config->stats;

    stats.entry_count =
        config->entry_count;

    stats.explicit_entry_count = 0u;
    stats.string_entry_count = 0u;
    stats.boolean_entry_count = 0u;
    stats.numeric_entry_count = 0u;

    for (index = 0u;
         index < config->entry_count;
         ++index) {
        const vitte_config_entry_t *entry;

        entry =
            &config->entries[index];

        if (entry->explicitly_set) {
            ++stats.explicit_entry_count;
        }

        switch (entry->value.type) {
            case VITTE_CONFIG_VALUE_STRING:
                ++stats.string_entry_count;
                break;

            case VITTE_CONFIG_VALUE_BOOL:
                ++stats.boolean_entry_count;
                break;

            case VITTE_CONFIG_VALUE_SIZE:
            case VITTE_CONFIG_VALUE_U64:
                ++stats.numeric_entry_count;
                break;

            case VITTE_CONFIG_VALUE_INVALID:
            case VITTE_CONFIG_VALUE_COUNT:
                break;
        }
    }

    stats.fingerprint =
        vitte_config_fingerprint(
            config);

    stats.validated =
        config->state ==
            VITTE_CONFIG_STATE_VALIDATED ||
        config->state ==
            VITTE_CONFIG_STATE_FROZEN;

    stats.frozen =
        config->state ==
            VITTE_CONFIG_STATE_FROZEN;

    return stats;
}

/* ========================================================================= */
/* String parsers                                                            */
/* ========================================================================= */

bool
vitte_config_parse_bool(
    const char *text,
    bool *value)
{
    if (text == NULL ||
        value == NULL) {
        return false;
    }

    if (vitte_config_string_equal(
            text,
            "true") ||
        vitte_config_string_equal(
            text,
            "yes") ||
        vitte_config_string_equal(
            text,
            "on") ||
        vitte_config_string_equal(
            text,
            "1")) {
        *value = true;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "false") ||
        vitte_config_string_equal(
            text,
            "no") ||
        vitte_config_string_equal(
            text,
            "off") ||
        vitte_config_string_equal(
            text,
            "0")) {
        *value = false;
        return true;
    }

    return false;
}

bool
vitte_config_parse_size(
    const char *text,
    size_t *value)
{
    size_t result;
    size_t index;

    if (text == NULL ||
        value == NULL ||
        text[0] == '\0') {
        return false;
    }

    result = 0u;

    for (index = 0u;
         text[index] != '\0';
         ++index) {
        unsigned digit;

        if (text[index] < '0' ||
            text[index] > '9') {
            return false;
        }

        digit =
            (unsigned)(
                text[index] - '0');

        if (result >
            (SIZE_MAX - (size_t)digit) /
                10u) {
            return false;
        }

        result =
            result * 10u +
            (size_t)digit;
    }

    *value = result;

    return true;
}

bool
vitte_config_parse_profile(
    const char *text,
    vitte_config_profile_t *profile)
{
    if (text == NULL ||
        profile == NULL) {
        return false;
    }

    if (vitte_config_string_equal(
            text,
            "default")) {
        *profile =
            VITTE_CONFIG_PROFILE_DEFAULT;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "debug")) {
        *profile =
            VITTE_CONFIG_PROFILE_DEBUG;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "release")) {
        *profile =
            VITTE_CONFIG_PROFILE_RELEASE;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "size")) {
        *profile =
            VITTE_CONFIG_PROFILE_SIZE;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "sanitize")) {
        *profile =
            VITTE_CONFIG_PROFILE_SANITIZE;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "freestanding")) {
        *profile =
            VITTE_CONFIG_PROFILE_FREESTANDING;
        return true;
    }

    return false;
}

bool
vitte_config_parse_backend(
    const char *text,
    vitte_config_backend_t *backend)
{
    if (text == NULL ||
        backend == NULL) {
        return false;
    }

    if (vitte_config_string_equal(
            text,
            "default")) {
        *backend =
            VITTE_CONFIG_BACKEND_DEFAULT;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "c17") ||
        vitte_config_string_equal(
            text,
            "c")) {
        *backend =
            VITTE_CONFIG_BACKEND_C17;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "native")) {
        *backend =
            VITTE_CONFIG_BACKEND_NATIVE;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "bytecode")) {
        *backend =
            VITTE_CONFIG_BACKEND_BYTECODE;
        return true;
    }

    return false;
}

bool
vitte_config_parse_optimization(
    const char *text,
    vitte_config_optimization_t *optimization)
{
    if (text == NULL ||
        optimization == NULL) {
        return false;
    }

    if (vitte_config_string_equal(
            text,
            "default")) {
        *optimization =
            VITTE_CONFIG_OPTIMIZATION_DEFAULT;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "none") ||
        vitte_config_string_equal(
            text,
            "0")) {
        *optimization =
            VITTE_CONFIG_OPTIMIZATION_NONE;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "debug")) {
        *optimization =
            VITTE_CONFIG_OPTIMIZATION_DEBUG;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "balanced") ||
        vitte_config_string_equal(
            text,
            "1")) {
        *optimization =
            VITTE_CONFIG_OPTIMIZATION_BALANCED;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "speed") ||
        vitte_config_string_equal(
            text,
            "2")) {
        *optimization =
            VITTE_CONFIG_OPTIMIZATION_SPEED;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "size") ||
        vitte_config_string_equal(
            text,
            "s")) {
        *optimization =
            VITTE_CONFIG_OPTIMIZATION_SIZE;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "aggressive") ||
        vitte_config_string_equal(
            text,
            "3")) {
        *optimization =
            VITTE_CONFIG_OPTIMIZATION_AGGRESSIVE;
        return true;
    }

    return false;
}

bool
vitte_config_parse_diagnostic_format(
    const char *text,
    vitte_config_diagnostic_format_t *format)
{
    if (text == NULL ||
        format == NULL) {
        return false;
    }

    if (vitte_config_string_equal(
            text,
            "default")) {
        *format =
            VITTE_CONFIG_DIAGNOSTIC_DEFAULT;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "terminal")) {
        *format =
            VITTE_CONFIG_DIAGNOSTIC_TERMINAL;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "short")) {
        *format =
            VITTE_CONFIG_DIAGNOSTIC_SHORT;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "json")) {
        *format =
            VITTE_CONFIG_DIAGNOSTIC_JSON;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "sarif")) {
        *format =
            VITTE_CONFIG_DIAGNOSTIC_SARIF;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "lsp")) {
        *format =
            VITTE_CONFIG_DIAGNOSTIC_LSP;
        return true;
    }

    return false;
}

bool
vitte_config_parse_color(
    const char *text,
    vitte_config_color_t *color)
{
    if (text == NULL ||
        color == NULL) {
        return false;
    }

    if (vitte_config_string_equal(
            text,
            "auto")) {
        *color =
            VITTE_CONFIG_COLOR_AUTO;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "always")) {
        *color =
            VITTE_CONFIG_COLOR_ALWAYS;
        return true;
    }

    if (vitte_config_string_equal(
            text,
            "never")) {
        *color =
            VITTE_CONFIG_COLOR_NEVER;
        return true;
    }

    return false;
}

/* ========================================================================= */
/* Key parsing                                                               */
/* ========================================================================= */

bool
vitte_config_parse_key(
    const char *text,
    vitte_config_key_t *key)
{
    vitte_config_key_t current;

    if (text == NULL ||
        key == NULL) {
        return false;
    }

    for (current =
            (vitte_config_key_t)
                (VITTE_CONFIG_KEY_INVALID + 1);
         current < VITTE_CONFIG_KEY_COUNT;
         current =
            (vitte_config_key_t)
                (current + 1)) {
        if (vitte_config_string_equal(
                text,
                vitte_config_key_name(
                    current))) {
            *key = current;
            return true;
        }
    }

    return false;
}

/* ========================================================================= */
/* Generic textual assignment                                                */
/* ========================================================================= */

bool
vitte_config_set_text(
    vitte_config_t *config,
    vitte_config_key_t key,
    const char *text,
    vitte_config_origin_t origin)
{
    vitte_config_value_type_t type;

    if (config == NULL ||
        text == NULL) {
        return false;
    }

    type =
        vitte_config_key_expected_type(
            key);

    switch (type) {
        case VITTE_CONFIG_VALUE_BOOL:
        {
            bool value;

            if (!vitte_config_parse_bool(
                    text,
                    &value)) {
                return
                    vitte_config_fail(
                        config,
                        VITTE_CONFIG_ERROR_INVALID_VALUE,
                        key,
                        origin,
                        "invalid boolean configuration value");
            }

            return
                vitte_config_set_bool(
                    config,
                    key,
                    value,
                    origin);
        }

        case VITTE_CONFIG_VALUE_SIZE:
        {
            size_t value;

            if (!vitte_config_parse_size(
                    text,
                    &value)) {
                return
                    vitte_config_fail(
                        config,
                        VITTE_CONFIG_ERROR_INVALID_VALUE,
                        key,
                        origin,
                        "invalid numeric configuration value");
            }

            return
                vitte_config_set_size(
                    config,
                    key,
                    value,
                    origin);
        }

        case VITTE_CONFIG_VALUE_STRING:
            return
                vitte_config_set_string(
                    config,
                    key,
                    text,
                    origin);

        case VITTE_CONFIG_VALUE_U64:
            switch (key) {
                case VITTE_CONFIG_KEY_PROFILE:
                {
                    vitte_config_profile_t profile;

                    if (!vitte_config_parse_profile(
                            text,
                            &profile)) {
                        return
                            vitte_config_fail(
                                config,
                                VITTE_CONFIG_ERROR_INVALID_PROFILE,
                                key,
                                origin,
                                "invalid profile value");
                    }

                    return
                        vitte_config_set_profile(
                            config,
                            profile,
                            origin);
                }

                case VITTE_CONFIG_KEY_BACKEND:
                {
                    vitte_config_backend_t backend;

                    if (!vitte_config_parse_backend(
                            text,
                            &backend)) {
                        return
                            vitte_config_fail(
                                config,
                                VITTE_CONFIG_ERROR_INVALID_BACKEND,
                                key,
                                origin,
                                "invalid backend value");
                    }

                    return
                        vitte_config_set_backend(
                            config,
                            backend,
                            origin);
                }

                case VITTE_CONFIG_KEY_OPTIMIZATION:
                {
                    vitte_config_optimization_t optimization;

                    if (!vitte_config_parse_optimization(
                            text,
                            &optimization)) {
                        return
                            vitte_config_fail(
                                config,
                                VITTE_CONFIG_ERROR_INVALID_OPTIMIZATION,
                                key,
                                origin,
                                "invalid optimization value");
                    }

                    return
                        vitte_config_set_optimization(
                            config,
                            optimization,
                            origin);
                }

                case VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT:
                {
                    vitte_config_diagnostic_format_t format;

                    if (!vitte_config_parse_diagnostic_format(
                            text,
                            &format)) {
                        return
                            vitte_config_fail(
                                config,
                                VITTE_CONFIG_ERROR_INVALID_DIAGNOSTIC_FORMAT,
                                key,
                                origin,
                                "invalid diagnostic format");
                    }

                    return
                        vitte_config_set_diagnostic_format(
                            config,
                            format,
                            origin);
                }

                case VITTE_CONFIG_KEY_COLOR:
                {
                    vitte_config_color_t color;

                    if (!vitte_config_parse_color(
                            text,
                            &color)) {
                        return
                            vitte_config_fail(
                                config,
                                VITTE_CONFIG_ERROR_INVALID_COLOR_MODE,
                                key,
                                origin,
                                "invalid color mode");
                    }

                    return
                        vitte_config_set_color(
                            config,
                            color,
                            origin);
                }

                case VITTE_CONFIG_KEY_INVALID:
                case VITTE_CONFIG_KEY_TARGET:
                case VITTE_CONFIG_KEY_OUTPUT:
                case VITTE_CONFIG_KEY_MANIFEST_PATH:
                case VITTE_CONFIG_KEY_PROJECT_ROOT:
                case VITTE_CONFIG_KEY_BUILD_DIR:
                case VITTE_CONFIG_KEY_CACHE_DIR:
                case VITTE_CONFIG_KEY_SYSROOT:
                case VITTE_CONFIG_KEY_REGISTRY:
                case VITTE_CONFIG_KEY_JOBS:
                case VITTE_CONFIG_KEY_MAX_ERRORS:
                case VITTE_CONFIG_KEY_MAX_OUTPUT_BYTES:
                case VITTE_CONFIG_KEY_VERBOSE:
                case VITTE_CONFIG_KEY_QUIET:
                case VITTE_CONFIG_KEY_WARNINGS_AS_ERRORS:
                case VITTE_CONFIG_KEY_EMIT_DEBUG_INFO:
                case VITTE_CONFIG_KEY_EMIT_SOURCE_MAP:
                case VITTE_CONFIG_KEY_EMIT_COMMENTS:
                case VITTE_CONFIG_KEY_RUNTIME_CHECKS:
                case VITTE_CONFIG_KEY_BOUNDS_CHECKS:
                case VITTE_CONFIG_KEY_NULL_CHECKS:
                case VITTE_CONFIG_KEY_OVERFLOW_CHECKS:
                case VITTE_CONFIG_KEY_DETERMINISTIC:
                case VITTE_CONFIG_KEY_INCREMENTAL:
                case VITTE_CONFIG_KEY_OFFLINE:
                case VITTE_CONFIG_KEY_LOCKED:
                case VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES:
                case VITTE_CONFIG_KEY_FORCE:
                case VITTE_CONFIG_KEY_COUNT:
                    return
                        vitte_config_fail(
                            config,
                            VITTE_CONFIG_ERROR_INVALID_TYPE,
                            key,
                            origin,
                            "configuration key has invalid enum storage mapping");
            }

            return false;

        case VITTE_CONFIG_VALUE_INVALID:
        case VITTE_CONFIG_VALUE_COUNT:
            return
                vitte_config_fail(
                    config,
                    VITTE_CONFIG_ERROR_INVALID_KEY,
                    key,
                    origin,
                    "configuration key has no valid value type");
    }

    return false;
}

/* ========================================================================= */
/* Merge                                                                     */
/* ========================================================================= */

bool
vitte_config_merge(
    vitte_config_t *destination,
    const vitte_config_t *source)
{
    size_t index;

    if (!vitte_config_is_valid(destination) ||
        !vitte_config_is_valid(source)) {
        return false;
    }

    if (destination ==
        source) {
        return true;
    }

    if (destination->state ==
        VITTE_CONFIG_STATE_FROZEN) {
        return
            vitte_config_fail(
                destination,
                VITTE_CONFIG_ERROR_FROZEN,
                VITTE_CONFIG_KEY_INVALID,
                (vitte_config_origin_t){0},
                "cannot merge into frozen configuration");
    }

    for (index = 0u;
         index < source->entry_count;
         ++index) {
        const vitte_config_entry_t *entry;

        entry =
            &source->entries[index];

        if (!vitte_config_set(
                destination,
                entry->key,
                entry->value,
                entry->origin)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Clone                                                                     */
/* ========================================================================= */

bool
vitte_config_clone(
    vitte_config_t *destination,
    const vitte_config_t *source)
{
    size_t bytes;

    if (destination == NULL ||
        !vitte_config_is_valid(source) ||
        destination == source) {
        return false;
    }

    if (!vitte_config_init(
            destination)) {
        return false;
    }

    destination->max_entries =
        source->max_entries;

    destination->max_string_bytes =
        source->max_string_bytes;

    if (source->entry_count != 0u) {
        if (!vitte_config_size_mul(
                source->entry_count,
                sizeof(*source->entries),
                &bytes)) {
            vitte_config_destroy(
                destination);

            return false;
        }

        destination->entries =
            (vitte_config_entry_t *)malloc(
                bytes);

        if (destination->entries == NULL) {
            vitte_config_destroy(
                destination);

            return false;
        }

        memcpy(
            destination->entries,
            source->entries,
            bytes);

        destination->entry_count =
            source->entry_count;

        destination->entry_capacity =
            source->entry_count;
    }

    destination->resolved =
        source->resolved;

    destination->message =
        source->message;

    destination->stats =
        source->stats;

    destination->last_error =
        source->last_error;

    destination->next_sequence =
        source->next_sequence;

    destination->generation =
        source->generation;

    /*
     * A clone of a frozen configuration remains frozen.
     * A failed source remains failed so that cloning does not silently erase
     * its semantic state.
     */
    destination->state =
        source->state;

    return true;
}

/* ========================================================================= */
/* Equality                                                                  */
/* ========================================================================= */

bool
vitte_config_equal(
    const vitte_config_t *left,
    const vitte_config_t *right)
{
    vitte_config_key_t key;

    if (!vitte_config_is_valid(left) ||
        !vitte_config_is_valid(right)) {
        return false;
    }

    for (key =
            (vitte_config_key_t)
                (VITTE_CONFIG_KEY_INVALID + 1);
         key < VITTE_CONFIG_KEY_COUNT;
         key =
            (vitte_config_key_t)
                (key + 1)) {
        const vitte_config_entry_t *left_entry;
        const vitte_config_entry_t *right_entry;

        left_entry =
            vitte_config_find_entry(
                left,
                key);

        right_entry =
            vitte_config_find_entry(
                right,
                key);

        if (left_entry == NULL &&
            right_entry == NULL) {
            continue;
        }

        if (left_entry == NULL ||
            right_entry == NULL) {
            /*
             * One side may simply be using a default without an explicit
             * entry. The resolved fingerprint comparison below handles
             * semantic equality.
             */
            continue;
        }

        if (!vitte_config_value_equal(
                &left_entry->value,
                &right_entry->value)) {
            return false;
        }
    }

    return
        vitte_config_fingerprint(left) ==
        vitte_config_fingerprint(right);
}

/* ========================================================================= */
/* Configuration origin constructors                                        */
/* ========================================================================= */

vitte_config_origin_t
vitte_config_origin_default(void)
{
    vitte_config_origin_t origin;

    memset(
        &origin,
        0,
        sizeof(origin));

    origin.source =
        VITTE_CONFIG_SOURCE_DEFAULT;

    origin.name =
        "built-in";

    return origin;
}

vitte_config_origin_t
vitte_config_origin_make(
    vitte_config_source_t source,
    const char *name,
    size_t line,
    size_t column)
{
    vitte_config_origin_t origin;

    memset(
        &origin,
        0,
        sizeof(origin));

    origin.source = source;
    origin.name = name;
    origin.line = line;
    origin.column = column;

    return origin;
}

/* ========================================================================= */
/* Apply canonical defaults                                                  */
/* ========================================================================= */

bool
vitte_config_apply_defaults(
    vitte_config_t *config)
{
    vitte_config_origin_t origin;

    if (!vitte_config_is_valid(config)) {
        return false;
    }

    origin =
        vitte_config_origin_default();

    if (!vitte_config_set_profile(
            config,
            VITTE_CONFIG_PROFILE_DEFAULT,
            origin)) {
        return false;
    }

    if (!vitte_config_set_backend(
            config,
            VITTE_CONFIG_BACKEND_DEFAULT,
            origin)) {
        return false;
    }

    if (!vitte_config_set_optimization(
            config,
            VITTE_CONFIG_OPTIMIZATION_DEFAULT,
            origin)) {
        return false;
    }

    if (!vitte_config_set_size(
            config,
            VITTE_CONFIG_KEY_JOBS,
            VITTE_CONFIG_DEFAULT_JOBS,
            origin)) {
        return false;
    }

    if (!vitte_config_set_size(
            config,
            VITTE_CONFIG_KEY_MAX_ERRORS,
            VITTE_CONFIG_DEFAULT_MAX_ERRORS,
            origin)) {
        return false;
    }

    if (!vitte_config_set_size(
            config,
            VITTE_CONFIG_KEY_MAX_OUTPUT_BYTES,
            VITTE_CONFIG_DEFAULT_MAX_OUTPUT_BYTES,
            origin)) {
        return false;
    }

    if (!vitte_config_set_diagnostic_format(
            config,
            VITTE_CONFIG_DIAGNOSTIC_DEFAULT,
            origin)) {
        return false;
    }

    if (!vitte_config_set_color(
            config,
            VITTE_CONFIG_COLOR_AUTO,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_VERBOSE,
            false,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_QUIET,
            false,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_WARNINGS_AS_ERRORS,
            false,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_EMIT_DEBUG_INFO,
            false,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_EMIT_SOURCE_MAP,
            false,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_EMIT_COMMENTS,
            false,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_RUNTIME_CHECKS,
            true,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_BOUNDS_CHECKS,
            true,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_NULL_CHECKS,
            true,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_OVERFLOW_CHECKS,
            true,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_DETERMINISTIC,
            true,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_INCREMENTAL,
            true,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_OFFLINE,
            false,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_LOCKED,
            false,
            origin)) {
        return false;
    }

    if (!vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES,
            false,
            origin)) {
        return false;
    }

    return
        vitte_config_set_bool(
            config,
            VITTE_CONFIG_KEY_FORCE,
            false,
            origin);
}

/* ========================================================================= */
/* Resolved convenience queries                                              */
/* ========================================================================= */

bool
vitte_config_all_runtime_checks_enabled(
    const vitte_config_t *config)
{
    if (!vitte_config_is_valid(config)) {
        return false;
    }

    return
        config->resolved.runtime_checks &&
        config->resolved.bounds_checks &&
        config->resolved.null_checks &&
        config->resolved.overflow_checks;
}

bool
vitte_config_is_reproducible(
    const vitte_config_t *config)
{
    if (!vitte_config_is_valid(config)) {
        return false;
    }

    return
        config->resolved.deterministic &&
        config->resolved.locked;
}

bool
vitte_config_is_dependency_frozen(
    const vitte_config_t *config)
{
    if (!vitte_config_is_valid(config)) {
        return false;
    }

    return
        config->resolved.frozen_dependencies &&
        config->resolved.locked &&
        config->resolved.offline;
}

/* ========================================================================= */
/* Generation                                                                */
/* ========================================================================= */

uint64_t
vitte_config_generation(
    const vitte_config_t *config)
{
    if (!vitte_config_is_valid(config)) {
        return 0u;
    }

    return config->generation;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
