#ifndef VITTE_SRC_BACKEND_C17_NAMING_H
#define VITTE_SRC_BACKEND_C17_NAMING_H

/*
 * Vitte Compiler
 * src/backend/c17/naming.h
 *
 * Canonical naming and symbol mangling for the ISO C17 backend.
 *
 * Goals:
 *
 *   - deterministic generated C identifiers
 *   - collision-resistant and byte-safe mangling
 *   - C17 keyword protection
 *   - implementation-reserved identifier protection
 *   - stable module qualification
 *   - distinct semantic name kinds
 *   - generated temporaries
 *   - generated labels
 *   - generated internal names
 *   - runtime symbol names
 *   - stable hashing
 *   - global generated-name collision registry
 *   - validation and statistics
 *
 * Canonical prefix:
 *
 *     vitte_
 *
 * Canonical byte encoding:
 *
 *     A-Z / a-z / 0-9 -> unchanged
 *     _               -> _u
 *     other byte      -> _xHH
 *
 * Escaping '_' makes the byte encoding unambiguous:
 *
 *     "-"     -> _x2D
 *     "_x2D"  -> _ux2D
 *
 * The implementation intentionally treats source identifiers as byte
 * sequences. UTF-8 therefore remains deterministic and locale-independent.
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

#ifndef VITTE_C17_NAMING_MAGIC
#define VITTE_C17_NAMING_MAGIC \
    UINT64_C(0x56495454454E414D)
#endif

#ifndef VITTE_C17_NAMING_DEAD_MAGIC
#define VITTE_C17_NAMING_DEAD_MAGIC \
    UINT64_C(0x444541444E414D45)
#endif

#ifndef VITTE_C17_NAMING_PREFIX
#define VITTE_C17_NAMING_PREFIX \
    "vitte_"
#endif

#ifndef VITTE_C17_NAMING_PREFIX_LENGTH
#define VITTE_C17_NAMING_PREFIX_LENGTH \
    ((size_t)6u)
#endif

#ifndef VITTE_C17_NAMING_INITIAL_CAPACITY
#define VITTE_C17_NAMING_INITIAL_CAPACITY \
    ((size_t)64u)
#endif

#ifndef VITTE_C17_NAMING_MAX_IDENTIFIER_BYTES
#define VITTE_C17_NAMING_MAX_IDENTIFIER_BYTES \
    ((size_t)4096u)
#endif

#ifndef VITTE_C17_NAMING_MAX_GENERATED_BYTES
#define VITTE_C17_NAMING_MAX_GENERATED_BYTES \
    ((size_t)(64u * 1024u))
#endif

#ifndef VITTE_C17_NAMING_FNV_OFFSET
#define VITTE_C17_NAMING_FNV_OFFSET \
    UINT64_C(14695981039346656037)
#endif

#ifndef VITTE_C17_NAMING_FNV_PRIME
#define VITTE_C17_NAMING_FNV_PRIME \
    UINT64_C(1099511628211)
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_c17_naming
    vitte_c17_naming_t;

typedef struct vitte_c17_naming_config
    vitte_c17_naming_config_t;

typedef struct vitte_c17_name_entry
    vitte_c17_name_entry_t;

typedef struct vitte_c17_naming_stats
    vitte_c17_naming_stats_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_c17_naming_error {
    VITTE_C17_NAMING_ERROR_NONE = 0,

    VITTE_C17_NAMING_ERROR_INVALID_NAMING,

    VITTE_C17_NAMING_ERROR_INVALID_ARGUMENT,

    VITTE_C17_NAMING_ERROR_INVALID_KIND,

    VITTE_C17_NAMING_ERROR_INVALID_IDENTIFIER,

    VITTE_C17_NAMING_ERROR_TOO_LONG,

    VITTE_C17_NAMING_ERROR_OVERFLOW,

    VITTE_C17_NAMING_ERROR_OUT_OF_MEMORY,

    VITTE_C17_NAMING_ERROR_COLLISION,

    VITTE_C17_NAMING_ERROR_CORRUPTION,

    VITTE_C17_NAMING_ERROR_COUNT
} vitte_c17_naming_error_t;

/* ========================================================================= */
/* Name kind                                                                 */
/* ========================================================================= */

/*
 * Kind tags form part of the generated identifier by default.
 *
 * This keeps independent backend semantic namespaces distinct even though
 * ISO C ultimately places many generated names into overlapping namespaces.
 */
typedef enum vitte_c17_name_kind {
    VITTE_C17_NAME_INVALID = 0,

    /*
     * Logical module identity.
     */
    VITTE_C17_NAME_MODULE,

    /*
     * Generated typedef / struct / union / enum-related identity.
     */
    VITTE_C17_NAME_TYPE,

    /*
     * File/global storage.
     */
    VITTE_C17_NAME_GLOBAL,

    /*
     * Function/procedure.
     */
    VITTE_C17_NAME_FUNCTION,

    /*
     * Function parameter.
     */
    VITTE_C17_NAME_PARAMETER,

    /*
     * Local variable.
     */
    VITTE_C17_NAME_LOCAL,

    /*
     * Backend-generated temporary.
     */
    VITTE_C17_NAME_TEMPORARY,

    /*
     * C goto label.
     */
    VITTE_C17_NAME_LABEL,

    /*
     * Aggregate field/member.
     */
    VITTE_C17_NAME_FIELD,

    /*
     * Enum-like generated constant.
     */
    VITTE_C17_NAME_ENUMERATOR,

    /*
     * Runtime support symbol.
     */
    VITTE_C17_NAME_RUNTIME,

    /*
     * Generic backend-internal synthetic symbol.
     */
    VITTE_C17_NAME_INTERNAL,

    VITTE_C17_NAME_COUNT
} vitte_c17_name_kind_t;

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

struct vitte_c17_naming_config {
    /*
     * Maximum accepted source identifier byte length.
     */
    size_t max_identifier_bytes;

    /*
     * Maximum generated C identifier byte length.
     */
    size_t max_generated_bytes;

    /*
     * Include a semantic kind tag:
     *
     *     vitte_f_
     *     vitte_t_
     *     vitte_g_
     */
    bool include_kind_tag;

    /*
     * Include module identity:
     *
     *     m42_
     */
    bool include_module_id;

    /*
     * Include stable source/IR identity:
     *
     *     _s123
     */
    bool include_source_id;

    /*
     * Append a stable 64-bit hexadecimal hash.
     */
    bool append_hash;

    /*
     * Reject generated-name collisions.
     *
     * Note:
     *
     * The implementation never permits two semantically distinct registry
     * entries to alias the same generated C identifier, even when this flag
     * is false. This flag controls whether collision handling is treated as
     * an immediate hard registry failure.
     */
    bool reject_collisions;
};

/* ========================================================================= */
/* Registry entry                                                            */
/* ========================================================================= */

struct vitte_c17_name_entry {
    /*
     * Semantic category.
     */
    vitte_c17_name_kind_t kind;

    /*
     * Stable logical module identity.
     *
     * Zero is allowed for global runtime names.
     */
    uint64_t module_id;

    /*
     * Stable AST/HIR/IR/synthetic identity.
     */
    uint64_t source_id;

    /*
     * Owned original source/backend name.
     */
    char *source;

    size_t source_length;

    /*
     * Owned final C identifier.
     */
    char *generated;

    size_t generated_length;

    /*
     * Stable hash of generated.
     */
    uint64_t hash;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_c17_naming_stats {
    /*
     * Current registry.
     */
    size_t entry_count;

    size_t entry_capacity;

    /*
     * Number of unique names registered since init/reset.
     */
    size_t generated_count;

    /*
     * Number of generated-name collisions observed.
     */
    size_t collision_count;

    /*
     * Aggregate byte counts.
     */
    size_t source_bytes;

    size_t generated_bytes;

    /*
     * Longest generated C identifier.
     */
    size_t maximum_generated_length;

    /*
     * Per-kind registry counts.
     *
     * Index with vitte_c17_name_kind_t.
     */
    size_t by_kind[VITTE_C17_NAME_COUNT];
};

/* ========================================================================= */
/* Naming context                                                            */
/* ========================================================================= */

struct vitte_c17_naming {
    uint64_t magic;

    vitte_c17_naming_config_t config;

    vitte_c17_naming_error_t last_error;

    /*
     * Canonical symbol registry.
     */
    vitte_c17_name_entry_t *entries;

    size_t entry_count;

    size_t entry_capacity;

    /*
     * Synthetic identities.
     *
     * IDs start at 1.
     */
    uint64_t next_temporary_id;

    uint64_t next_label_id;

    uint64_t next_internal_id;

    /*
     * Statistics/collision accounting.
     */
    size_t collision_count;

    size_t generated_count;
};

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_naming_error_name(
    vitte_c17_naming_error_t error);

const char *
vitte_c17_name_kind_name(
    vitte_c17_name_kind_t kind);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_c17_naming_config_t
vitte_c17_naming_config_default(void);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize a naming registry.
 *
 * config == NULL selects canonical defaults.
 */
bool
vitte_c17_naming_init(
    vitte_c17_naming_t *naming,
    const vitte_c17_naming_config_t *config);

/*
 * Remove every registered name and reset synthetic counters while retaining
 * the allocated registry capacity.
 */
void
vitte_c17_naming_reset(
    vitte_c17_naming_t *naming);

/*
 * Destroy all owned names and registry storage.
 */
void
vitte_c17_naming_destroy(
    vitte_c17_naming_t *naming);

/* ========================================================================= */
/* Validity                                                                  */
/* ========================================================================= */

bool
vitte_c17_naming_is_valid(
    const vitte_c17_naming_t *naming);

/*
 * Deep structural validation.
 *
 * Checks:
 *
 *   - context invariants
 *   - registry bounds
 *   - valid kinds
 *   - owned source names
 *   - generated C identifiers
 *   - generated-name hashes
 *   - duplicate semantic keys
 *   - generated-name collisions
 */
bool
vitte_c17_naming_validate(
    const vitte_c17_naming_t *naming);

/* ========================================================================= */
/* C identifier classification                                               */
/* ========================================================================= */

/*
 * True for an ASCII C lexical identifier.
 *
 * This function does not reject keywords or reserved implementation names.
 */
bool
vitte_c17_naming_is_c_identifier(
    const char *identifier,
    size_t length);

/*
 * True if identifier is one of the ISO C17 language keywords recognized by
 * the backend.
 */
bool
vitte_c17_naming_is_keyword(
    const char *identifier,
    size_t length);

/*
 * Conservative implementation-reserved identifier check.
 *
 * Generated backend symbols deliberately avoid the complete leading '_'
 * namespace.
 */
bool
vitte_c17_naming_is_reserved_identifier(
    const char *identifier,
    size_t length);

/*
 * Full generated-name safety check:
 *
 *     lexical C identifier
 *     AND not keyword
 *     AND not implementation-reserved
 */
bool
vitte_c17_naming_is_safe_c_identifier(
    const char *identifier,
    size_t length);

/* ========================================================================= */
/* Stable hashing                                                            */
/* ========================================================================= */

/*
 * Stable FNV-1a 64-bit byte hash.
 *
 * This is intended for deterministic compiler naming, not cryptography.
 */
uint64_t
vitte_c17_naming_hash_bytes(
    const void *data,
    size_t length);

uint64_t
vitte_c17_naming_hash_string(
    const char *text,
    size_t length);

/* ========================================================================= */
/* Generic mangling                                                          */
/* ========================================================================= */

/*
 * Return the canonical registry entry for:
 *
 *     kind
 *     module_id
 *     source_id
 *     source bytes
 *
 * Repeating the exact semantic key returns the already registered entry.
 *
 * The returned pointer belongs to naming and remains valid until naming is
 * reset/destroyed or until a later registry realloc moves the entries array.
 *
 * IMPORTANT:
 *
 * Do not retain the vitte_c17_name_entry_t pointer across arbitrary calls
 * that may register new names. If long-lived access is required, retain the
 * generated string pointer or perform a lookup again.
 */
const vitte_c17_name_entry_t *
vitte_c17_naming_mangle(
    vitte_c17_naming_t *naming,
    vitte_c17_name_kind_t kind,
    uint64_t module_id,
    uint64_t source_id,
    const char *source,
    size_t source_length);

/* ========================================================================= */
/* Typed mangling                                                            */
/* ========================================================================= */

const vitte_c17_name_entry_t *
vitte_c17_naming_module(
    vitte_c17_naming_t *naming,
    uint64_t module_id,
    const char *name,
    size_t length);

const vitte_c17_name_entry_t *
vitte_c17_naming_type(
    vitte_c17_naming_t *naming,
    uint64_t module_id,
    uint64_t source_id,
    const char *name,
    size_t length);

const vitte_c17_name_entry_t *
vitte_c17_naming_global(
    vitte_c17_naming_t *naming,
    uint64_t module_id,
    uint64_t source_id,
    const char *name,
    size_t length);

const vitte_c17_name_entry_t *
vitte_c17_naming_function(
    vitte_c17_naming_t *naming,
    uint64_t module_id,
    uint64_t source_id,
    const char *name,
    size_t length);

const vitte_c17_name_entry_t *
vitte_c17_naming_parameter(
    vitte_c17_naming_t *naming,
    uint64_t module_id,
    uint64_t source_id,
    const char *name,
    size_t length);

const vitte_c17_name_entry_t *
vitte_c17_naming_local(
    vitte_c17_naming_t *naming,
    uint64_t module_id,
    uint64_t source_id,
    const char *name,
    size_t length);

const vitte_c17_name_entry_t *
vitte_c17_naming_field(
    vitte_c17_naming_t *naming,
    uint64_t module_id,
    uint64_t source_id,
    const char *name,
    size_t length);

const vitte_c17_name_entry_t *
vitte_c17_naming_enumerator(
    vitte_c17_naming_t *naming,
    uint64_t module_id,
    uint64_t source_id,
    const char *name,
    size_t length);

/*
 * Runtime names use:
 *
 *     module_id = 0
 *     source_id = 0
 */
const vitte_c17_name_entry_t *
vitte_c17_naming_runtime(
    vitte_c17_naming_t *naming,
    const char *name,
    size_t length);

/* ========================================================================= */
/* Synthetic names                                                           */
/* ========================================================================= */

/*
 * Allocate a new deterministic temporary ID and generate a temporary name.
 *
 * Example conceptual form:
 *
 *     vitte_tmp_m12_tmp_s1_h...
 */
const vitte_c17_name_entry_t *
vitte_c17_naming_temporary(
    vitte_c17_naming_t *naming,
    uint64_t module_id);

/*
 * Allocate a new deterministic C label.
 */
const vitte_c17_name_entry_t *
vitte_c17_naming_label(
    vitte_c17_naming_t *naming,
    uint64_t module_id);

/*
 * Allocate a new backend-internal symbol.
 *
 * Empty purpose names are normalized to "internal".
 */
const vitte_c17_name_entry_t *
vitte_c17_naming_internal(
    vitte_c17_naming_t *naming,
    uint64_t module_id,
    const char *purpose,
    size_t purpose_length);

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

/*
 * Find an entry by its final generated C identifier.
 */
const vitte_c17_name_entry_t *
vitte_c17_naming_find_generated(
    const vitte_c17_naming_t *naming,
    const char *generated,
    size_t generated_length);

/*
 * Find an entry by its complete semantic source key.
 */
const vitte_c17_name_entry_t *
vitte_c17_naming_find_source(
    const vitte_c17_naming_t *naming,
    vitte_c17_name_kind_t kind,
    uint64_t module_id,
    uint64_t source_id,
    const char *source,
    size_t source_length);

/* ========================================================================= */
/* Registry access                                                           */
/* ========================================================================= */

size_t
vitte_c17_naming_count(
    const vitte_c17_naming_t *naming);

const vitte_c17_name_entry_t *
vitte_c17_naming_at(
    const vitte_c17_naming_t *naming,
    size_t index);

/* ========================================================================= */
/* Error access                                                              */
/* ========================================================================= */

vitte_c17_naming_error_t
vitte_c17_naming_last_error(
    const vitte_c17_naming_t *naming);

void
vitte_c17_naming_clear_error(
    vitte_c17_naming_t *naming);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_naming_stats_t
vitte_c17_naming_stats(
    const vitte_c17_naming_t *naming);

/* ========================================================================= */
/* Inline entry helpers                                                      */
/* ========================================================================= */

static inline const char *
vitte_c17_name_entry_source(
    const vitte_c17_name_entry_t *entry)
{
    return
        entry != NULL
            ? entry->source
            : NULL;
}

static inline size_t
vitte_c17_name_entry_source_length(
    const vitte_c17_name_entry_t *entry)
{
    return
        entry != NULL
            ? entry->source_length
            : 0u;
}

static inline const char *
vitte_c17_name_entry_generated(
    const vitte_c17_name_entry_t *entry)
{
    return
        entry != NULL
            ? entry->generated
            : NULL;
}

static inline size_t
vitte_c17_name_entry_generated_length(
    const vitte_c17_name_entry_t *entry)
{
    return
        entry != NULL
            ? entry->generated_length
            : 0u;
}

static inline uint64_t
vitte_c17_name_entry_hash(
    const vitte_c17_name_entry_t *entry)
{
    return
        entry != NULL
            ? entry->hash
            : UINT64_C(0);
}

/* ========================================================================= */
/* Inline context helpers                                                    */
/* ========================================================================= */

static inline bool
vitte_c17_naming_has_error(
    const vitte_c17_naming_t *naming)
{
    return
        naming != NULL &&
        naming->last_error !=
            VITTE_C17_NAMING_ERROR_NONE;
}

static inline bool
vitte_c17_naming_is_empty(
    const vitte_c17_naming_t *naming)
{
    return
        naming == NULL ||
        naming->entry_count == 0u;
}

static inline size_t
vitte_c17_naming_collision_count(
    const vitte_c17_naming_t *naming)
{
    return
        naming != NULL
            ? naming->collision_count
            : 0u;
}

/* ========================================================================= */
/* Convenience literal macros                                                */
/* ========================================================================= */

#define VITTE_C17_NAMING_TYPE_LITERAL( \
    naming, module_id, source_id, literal) \
    vitte_c17_naming_type( \
        (naming), \
        (module_id), \
        (source_id), \
        (literal), \
        sizeof(literal) - 1u)

#define VITTE_C17_NAMING_GLOBAL_LITERAL( \
    naming, module_id, source_id, literal) \
    vitte_c17_naming_global( \
        (naming), \
        (module_id), \
        (source_id), \
        (literal), \
        sizeof(literal) - 1u)

#define VITTE_C17_NAMING_FUNCTION_LITERAL( \
    naming, module_id, source_id, literal) \
    vitte_c17_naming_function( \
        (naming), \
        (module_id), \
        (source_id), \
        (literal), \
        sizeof(literal) - 1u)

#define VITTE_C17_NAMING_LOCAL_LITERAL( \
    naming, module_id, source_id, literal) \
    vitte_c17_naming_local( \
        (naming), \
        (module_id), \
        (source_id), \
        (literal), \
        sizeof(literal) - 1u)

#define VITTE_C17_NAMING_RUNTIME_LITERAL( \
    naming, literal) \
    vitte_c17_naming_runtime( \
        (naming), \
        (literal), \
        sizeof(literal) - 1u)

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte C17 naming requires 64-bit uint64_t");

_Static_assert(
    VITTE_C17_NAMING_ERROR_NONE == 0,
    "naming no-error value must remain zero");

_Static_assert(
    VITTE_C17_NAME_INVALID == 0,
    "invalid name kind must remain zero");

_Static_assert(
    VITTE_C17_NAMING_PREFIX_LENGTH ==
        sizeof(VITTE_C17_NAMING_PREFIX) - 1u,
    "naming prefix length does not match naming prefix");

_Static_assert(
    VITTE_C17_NAMING_INITIAL_CAPACITY != 0u,
    "naming initial capacity must not be zero");

_Static_assert(
    VITTE_C17_NAMING_MAX_IDENTIFIER_BYTES != 0u,
    "maximum source identifier size must not be zero");

_Static_assert(
    VITTE_C17_NAMING_MAX_GENERATED_BYTES != 0u,
    "maximum generated identifier size must not be zero");

_Static_assert(
    VITTE_C17_NAMING_MAX_GENERATED_BYTES >=
        VITTE_C17_NAMING_PREFIX_LENGTH,
    "generated identifier limit is smaller than naming prefix");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_BACKEND_C17_NAMING_H */
