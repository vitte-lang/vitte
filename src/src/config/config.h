#ifndef VITTE_CONFIG_CONFIG_H
#define VITTE_CONFIG_CONFIG_H

/*
 * Vitte Compiler
 * src/config/config.h
 *
 * Canonical public configuration API.
 *
 * Responsibilities:
 *   - configuration lifecycle
 *   - typed configuration values
 *   - configuration provenance
 *   - deterministic precedence/merging
 *   - build profiles
 *   - backend selection
 *   - optimization policy
 *   - target selection
 *   - diagnostic policy
 *   - runtime safety policy
 *   - build/package policy
 *   - validation
 *   - freezing
 *   - deterministic fingerprints
 *   - statistics
 *
 * Precedence:
 *
 *   defaults
 *      <
 *   system
 *      <
 *   user
 *      <
 *   project
 *      <
 *   environment
 *      <
 *   command line
 *      <
 *   explicit override
 *
 * Strings stored in configuration values and origins are borrowed.
 * Their storage must remain valid for the lifetime of the configuration
 * object, unless a higher-level owner guarantees a shorter safe lifetime.
 *
 * ISO C17.
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
/* Errors                                                                    */
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
/* Build profile                                                             */
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

    /*
     * Architectural slots.
     *
     * Their presence in the configuration API does not imply that the
     * corresponding backend is currently implemented.
     */
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
/* Diagnostic output                                                         */
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
/* Canonical keys                                                            */
/* ========================================================================= */

typedef enum vitte_config_key {
    VITTE_CONFIG_KEY_INVALID = 0,

    /* Build/code generation. */
    VITTE_CONFIG_KEY_PROFILE,
    VITTE_CONFIG_KEY_BACKEND,
    VITTE_CONFIG_KEY_OPTIMIZATION,

    /* Target/output/path policy. */
    VITTE_CONFIG_KEY_TARGET,
    VITTE_CONFIG_KEY_OUTPUT,
    VITTE_CONFIG_KEY_MANIFEST_PATH,
    VITTE_CONFIG_KEY_PROJECT_ROOT,
    VITTE_CONFIG_KEY_BUILD_DIR,
    VITTE_CONFIG_KEY_CACHE_DIR,
    VITTE_CONFIG_KEY_SYSROOT,
    VITTE_CONFIG_KEY_REGISTRY,

    /* Resource limits. */
    VITTE_CONFIG_KEY_JOBS,
    VITTE_CONFIG_KEY_MAX_ERRORS,
    VITTE_CONFIG_KEY_MAX_OUTPUT_BYTES,

    /* Diagnostics. */
    VITTE_CONFIG_KEY_DIAGNOSTIC_FORMAT,
    VITTE_CONFIG_KEY_COLOR,
    VITTE_CONFIG_KEY_VERBOSE,
    VITTE_CONFIG_KEY_QUIET,
    VITTE_CONFIG_KEY_WARNINGS_AS_ERRORS,

    /* Generated output metadata. */
    VITTE_CONFIG_KEY_EMIT_DEBUG_INFO,
    VITTE_CONFIG_KEY_EMIT_SOURCE_MAP,
    VITTE_CONFIG_KEY_EMIT_COMMENTS,

    /* Runtime safety. */
    VITTE_CONFIG_KEY_RUNTIME_CHECKS,
    VITTE_CONFIG_KEY_BOUNDS_CHECKS,
    VITTE_CONFIG_KEY_NULL_CHECKS,
    VITTE_CONFIG_KEY_OVERFLOW_CHECKS,

    /* Build behavior. */
    VITTE_CONFIG_KEY_DETERMINISTIC,
    VITTE_CONFIG_KEY_INCREMENTAL,

    /* Package/dependency policy. */
    VITTE_CONFIG_KEY_OFFLINE,
    VITTE_CONFIG_KEY_LOCKED,
    VITTE_CONFIG_KEY_FROZEN_DEPENDENCIES,

    /* Explicit override behavior. */
    VITTE_CONFIG_KEY_FORCE,

    VITTE_CONFIG_KEY_COUNT
} vitte_config_key_t;

/* ========================================================================= */
/* Configuration value                                                       */
/* ========================================================================= */

typedef struct vitte_config_value {
    vitte_config_value_type_t type;

    union {
        bool boolean;
        size_t size;
        uint64_t u64;

        /*
         * Borrowed.
         *
         * The configuration object does not allocate, duplicate or free this
         * string.
         */
        const char *string;
    } as;
} vitte_config_value_t;

/* ========================================================================= */
/* Provenance                                                                */
/* ========================================================================= */

typedef struct vitte_config_origin {
    vitte_config_source_t source;

    /*
     * Optional borrowed source description.
     *
     * Examples:
     *
     *   built-in
     *   /etc/vitte/config
     *   ~/.config/vitte/config
     *   ./vitte.toml
     *   VITTE_TARGET
     *   --target
     */
    const char *name;

    /*
     * Optional source location.
     *
     * 0 means unknown/not applicable.
     */
    size_t line;
    size_t column;
} vitte_config_origin_t;

/* ========================================================================= */
/* Configuration entry                                                       */
/* ========================================================================= */

typedef struct vitte_config_entry {
    vitte_config_key_t key;
    vitte_config_value_t value;
    vitte_config_origin_t origin;

    /*
     * Monotonic assignment sequence.
     *
     * This is metadata and is deliberately excluded from the semantic
     * configuration fingerprint.
     */
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

    /* Borrowed strings. */
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
/* Configuration message                                                     */
/* ========================================================================= */

typedef struct vitte_config_message {
    vitte_config_error_t error;
    vitte_config_key_t key;

    vitte_config_origin_t origin;

    /*
     * Static or externally owned diagnostic detail.
     *
     * Borrowed.
     */
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
/* Main configuration object                                                 */
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
/* Name helpers                                                              */
/* ========================================================================= */

const char *
vitte_config_error_name(
    vitte_config_error_t error);

const char *
vitte_config_state_name(
    vitte_config_state_t state);

const char *
vitte_config_source_name(
    vitte_config_source_t source);

const char *
vitte_config_value_type_name(
    vitte_config_value_type_t type);

const char *
vitte_config_profile_name(
    vitte_config_profile_t profile);

const char *
vitte_config_backend_name(
    vitte_config_backend_t backend);

const char *
vitte_config_optimization_name(
    vitte_config_optimization_t optimization);

const char *
vitte_config_diagnostic_format_name(
    vitte_config_diagnostic_format_t format);

const char *
vitte_config_color_name(
    vitte_config_color_t color);

const char *
vitte_config_key_name(
    vitte_config_key_t key);

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

vitte_config_resolved_t
vitte_config_resolved_default(void);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_config_init(
    vitte_config_t *config);

void
vitte_config_destroy(
    vitte_config_t *config);

bool
vitte_config_reset(
    vitte_config_t *config);

bool
vitte_config_is_valid(
    const vitte_config_t *config);

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_config_set_limits(
    vitte_config_t *config,
    size_t max_entries,
    size_t max_string_bytes);

/* ========================================================================= */
/* Value constructors                                                        */
/* ========================================================================= */

vitte_config_value_t
vitte_config_value_bool(
    bool value);

vitte_config_value_t
vitte_config_value_size(
    size_t value);

vitte_config_value_t
vitte_config_value_u64(
    uint64_t value);

vitte_config_value_t
vitte_config_value_string(
    const char *value);

/* ========================================================================= */
/* Generic assignment                                                        */
/* ========================================================================= */

bool
vitte_config_set(
    vitte_config_t *config,
    vitte_config_key_t key,
    vitte_config_value_t value,
    vitte_config_origin_t origin);

/* ========================================================================= */
/* Typed assignment                                                          */
/* ========================================================================= */

bool
vitte_config_set_bool(
    vitte_config_t *config,
    vitte_config_key_t key,
    bool value,
    vitte_config_origin_t origin);

bool
vitte_config_set_size(
    vitte_config_t *config,
    vitte_config_key_t key,
    size_t value,
    vitte_config_origin_t origin);

bool
vitte_config_set_u64(
    vitte_config_t *config,
    vitte_config_key_t key,
    uint64_t value,
    vitte_config_origin_t origin);

bool
vitte_config_set_string(
    vitte_config_t *config,
    vitte_config_key_t key,
    const char *value,
    vitte_config_origin_t origin);

/* ========================================================================= */
/* Specialized assignment                                                    */
/* ========================================================================= */

bool
vitte_config_set_profile(
    vitte_config_t *config,
    vitte_config_profile_t profile,
    vitte_config_origin_t origin);

bool
vitte_config_set_backend(
    vitte_config_t *config,
    vitte_config_backend_t backend,
    vitte_config_origin_t origin);

bool
vitte_config_set_optimization(
    vitte_config_t *config,
    vitte_config_optimization_t optimization,
    vitte_config_origin_t origin);

bool
vitte_config_set_diagnostic_format(
    vitte_config_t *config,
    vitte_config_diagnostic_format_t format,
    vitte_config_origin_t origin);

bool
vitte_config_set_color(
    vitte_config_t *config,
    vitte_config_color_t color,
    vitte_config_origin_t origin);

/* ========================================================================= */
/* Text assignment                                                           */
/* ========================================================================= */

bool
vitte_config_set_text(
    vitte_config_t *config,
    vitte_config_key_t key,
    const char *text,
    vitte_config_origin_t origin);

/* ========================================================================= */
/* Profiles                                                                  */
/* ========================================================================= */

bool
vitte_config_apply_profile(
    vitte_config_t *config,
    vitte_config_profile_t profile,
    vitte_config_origin_t origin);

/* ========================================================================= */
/* Canonical defaults                                                        */
/* ========================================================================= */

bool
vitte_config_apply_defaults(
    vitte_config_t *config);

/* ========================================================================= */
/* Validation / freezing                                                     */
/* ========================================================================= */

bool
vitte_config_validate(
    vitte_config_t *config);

bool
vitte_config_freeze(
    vitte_config_t *config);

bool
vitte_config_enable_frozen_dependencies(
    vitte_config_t *config,
    vitte_config_origin_t origin);

/* ========================================================================= */
/* Merge / clone                                                             */
/* ========================================================================= */

bool
vitte_config_merge(
    vitte_config_t *destination,
    const vitte_config_t *source);

bool
vitte_config_clone(
    vitte_config_t *destination,
    const vitte_config_t *source);

bool
vitte_config_equal(
    const vitte_config_t *left,
    const vitte_config_t *right);

/* ========================================================================= */
/* Entry access                                                              */
/* ========================================================================= */

size_t
vitte_config_entry_count(
    const vitte_config_t *config);

const vitte_config_entry_t *
vitte_config_entry_at(
    const vitte_config_t *config,
    size_t index);

const vitte_config_entry_t *
vitte_config_find_entry(
    const vitte_config_t *config,
    vitte_config_key_t key);

/* ========================================================================= */
/* Presence                                                                  */
/* ========================================================================= */

bool
vitte_config_has(
    const vitte_config_t *config,
    vitte_config_key_t key);

bool
vitte_config_is_explicit(
    const vitte_config_t *config,
    vitte_config_key_t key);

/* ========================================================================= */
/* Provenance                                                                */
/* ========================================================================= */

vitte_config_source_t
vitte_config_source_of(
    const vitte_config_t *config,
    vitte_config_key_t key);

const vitte_config_origin_t *
vitte_config_origin_of(
    const vitte_config_t *config,
    vitte_config_key_t key);

vitte_config_origin_t
vitte_config_origin_default(void);

vitte_config_origin_t
vitte_config_origin_make(
    vitte_config_source_t source,
    const char *name,
    size_t line,
    size_t column);

/* ========================================================================= */
/* Resolved configuration                                                    */
/* ========================================================================= */

const vitte_config_resolved_t *
vitte_config_resolved(
    const vitte_config_t *config);

/* ========================================================================= */
/* State/error                                                               */
/* ========================================================================= */

vitte_config_state_t
vitte_config_state(
    const vitte_config_t *config);

vitte_config_error_t
vitte_config_last_error(
    const vitte_config_t *config);

const vitte_config_message_t *
vitte_config_message(
    const vitte_config_t *config);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Returns a deterministic semantic configuration fingerprint.
 *
 * Provenance, assignment sequence, pointer addresses and insertion order are
 * intentionally excluded.
 */
uint64_t
vitte_config_fingerprint(
    const vitte_config_t *config);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_config_stats_t
vitte_config_stats(
    const vitte_config_t *config);

/* ========================================================================= */
/* Parsers                                                                   */
/* ========================================================================= */

bool
vitte_config_parse_bool(
    const char *text,
    bool *value);

bool
vitte_config_parse_size(
    const char *text,
    size_t *value);

bool
vitte_config_parse_profile(
    const char *text,
    vitte_config_profile_t *profile);

bool
vitte_config_parse_backend(
    const char *text,
    vitte_config_backend_t *backend);

bool
vitte_config_parse_optimization(
    const char *text,
    vitte_config_optimization_t *optimization);

bool
vitte_config_parse_diagnostic_format(
    const char *text,
    vitte_config_diagnostic_format_t *format);

bool
vitte_config_parse_color(
    const char *text,
    vitte_config_color_t *color);

bool
vitte_config_parse_key(
    const char *text,
    vitte_config_key_t *key);

/* ========================================================================= */
/* Convenience queries                                                       */
/* ========================================================================= */

bool
vitte_config_all_runtime_checks_enabled(
    const vitte_config_t *config);

bool
vitte_config_is_reproducible(
    const vitte_config_t *config);

bool
vitte_config_is_dependency_frozen(
    const vitte_config_t *config);

uint64_t
vitte_config_generation(
    const vitte_config_t *config);

/* ========================================================================= */
/* Inline state helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_config_is_ready(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        config->state == VITTE_CONFIG_STATE_READY;
}

static inline bool
vitte_config_is_validated(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        (config->state ==
             VITTE_CONFIG_STATE_VALIDATED ||
         config->state ==
             VITTE_CONFIG_STATE_FROZEN);
}

static inline bool
vitte_config_is_frozen(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        config->state ==
            VITTE_CONFIG_STATE_FROZEN;
}

static inline bool
vitte_config_has_failed(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        config->state ==
            VITTE_CONFIG_STATE_FAILED;
}

static inline bool
vitte_config_has_error(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        config->last_error !=
            VITTE_CONFIG_ERROR_NONE;
}

static inline bool
vitte_config_is_verbose(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        config->resolved.verbose;
}

static inline bool
vitte_config_is_quiet(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        config->resolved.quiet;
}

static inline bool
vitte_config_is_offline(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        config->resolved.offline;
}

static inline bool
vitte_config_is_locked(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        config->resolved.locked;
}

static inline bool
vitte_config_is_deterministic(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        config->resolved.deterministic;
}

static inline bool
vitte_config_uses_c17_backend(
    const vitte_config_t *config)
{
    return
        config != NULL &&
        config->magic == VITTE_CONFIG_MAGIC &&
        (config->resolved.backend ==
             VITTE_CONFIG_BACKEND_DEFAULT ||
         config->resolved.backend ==
             VITTE_CONFIG_BACKEND_C17);
}

/* ========================================================================= */
/* C++                                                                      */
/* ========================================================================= */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_CONFIG_CONFIG_H */
