#ifndef VITTE_MODULE_MODULE_H
#define VITTE_MODULE_MODULE_H

/*
 * Vitte Compiler
 * src/module/module.h
 *
 * Canonical module / namespace subsystem contract.
 *
 * Responsibilities
 * ----------------
 *
 *   - canonical Vitte module identities;
 *   - `space` hierarchy;
 *   - parent/child module relationships;
 *   - source-unit association;
 *   - source/library/package/builtin/external modules;
 *   - declaration/symbol registration;
 *   - visibility metadata;
 *   - canonical string interning;
 *   - deterministic lookup;
 *   - structural validation;
 *   - deterministic fingerprints;
 *   - resource limits;
 *   - statistics.
 *
 * Non-responsibilities
 * --------------------
 *
 * This subsystem does not:
 *
 *   - read source files;
 *   - parse Vitte;
 *   - resolve `use` declarations;
 *   - perform semantic/type analysis;
 *   - own AST/HIR/IR nodes;
 *   - perform package downloads;
 *   - invoke the filesystem.
 *
 * Those responsibilities belong to their respective compiler stages.
 *
 * IDs
 * ---
 *
 * Public IDs are 64-bit unsigned integers.
 *
 * ID zero is always invalid.
 *
 * Valid object IDs are stable for the lifetime of one context generation.
 *
 * Context lifecycle
 * -----------------
 *
 *     init()
 *       |
 *       v
 *     BUILDING
 *       |
 *       +---- add modules / symbols / strings
 *       |
 *       v
 *     seal()
 *       |
 *       v
 *     VALIDATING
 *       |
 *       +---- success ---> SEALED
 *       |
 *       +---- failure ---> FAILED
 *
 * reset() starts a new BUILDING generation.
 *
 * destroy() invalidates the context permanently until init() is called
 * again.
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
/* API version                                                               */
/* ========================================================================= */

#define VITTE_MODULE_API_VERSION_MAJOR 1u
#define VITTE_MODULE_API_VERSION_MINOR 0u
#define VITTE_MODULE_API_VERSION_PATCH 0u

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_MODULE_MAGIC \
    UINT64_C(0x564954544d4f4421)

#define VITTE_MODULE_DEAD_MAGIC \
    UINT64_C(0x444541444d4f4421)

#define VITTE_MODULE_INVALID_ID \
    UINT64_C(0)

#define VITTE_MODULE_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_MODULE_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_MODULE_DEFAULT_INITIAL_CAPACITY \
    ((size_t)64u)

#define VITTE_MODULE_DEFAULT_MAX_MODULES \
    ((size_t)1048576u)

#define VITTE_MODULE_DEFAULT_MAX_SYMBOLS \
    ((size_t)16777216u)

#define VITTE_MODULE_DEFAULT_MAX_STRING_BYTES \
    ((size_t)(256u * 1024u * 1024u))

/* ========================================================================= */
/* IDs                                                                       */
/* ========================================================================= */

typedef uint64_t vitte_module_id_t;
typedef uint64_t vitte_module_string_id_t;
typedef uint64_t vitte_module_symbol_id_t;
typedef uint64_t vitte_module_source_id_t;
typedef uint64_t vitte_module_payload_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_module_error {
    VITTE_MODULE_ERROR_NONE = 0,

    VITTE_MODULE_ERROR_INVALID_ARGUMENT,
    VITTE_MODULE_ERROR_INVALID_CONTEXT,
    VITTE_MODULE_ERROR_INVALID_STATE,

    VITTE_MODULE_ERROR_INVALID_NAME,
    VITTE_MODULE_ERROR_INVALID_PATH,

    VITTE_MODULE_ERROR_INVALID_MODULE,
    VITTE_MODULE_ERROR_INVALID_PARENT,
    VITTE_MODULE_ERROR_INVALID_SYMBOL,

    VITTE_MODULE_ERROR_DUPLICATE_MODULE,
    VITTE_MODULE_ERROR_DUPLICATE_PATH,
    VITTE_MODULE_ERROR_DUPLICATE_SYMBOL,

    VITTE_MODULE_ERROR_MODULE_LIMIT,
    VITTE_MODULE_ERROR_SYMBOL_LIMIT,
    VITTE_MODULE_ERROR_STRING_LIMIT,

    VITTE_MODULE_ERROR_OVERFLOW,
    VITTE_MODULE_ERROR_OUT_OF_MEMORY,

    VITTE_MODULE_ERROR_VALIDATION,
    VITTE_MODULE_ERROR_CORRUPTION,

    VITTE_MODULE_ERROR_UNSUPPORTED,
    VITTE_MODULE_ERROR_INTERNAL,

    VITTE_MODULE_ERROR_COUNT
} vitte_module_error_t;

/* ========================================================================= */
/* Context state                                                             */
/* ========================================================================= */

typedef enum vitte_module_state {
    VITTE_MODULE_STATE_INVALID = 0,

    VITTE_MODULE_STATE_BUILDING,
    VITTE_MODULE_STATE_VALIDATING,
    VITTE_MODULE_STATE_SEALED,
    VITTE_MODULE_STATE_FAILED,
    VITTE_MODULE_STATE_DESTROYED,

    VITTE_MODULE_STATE_COUNT
} vitte_module_state_t;

/* ========================================================================= */
/* Module kind                                                               */
/* ========================================================================= */

typedef enum vitte_module_kind {
    VITTE_MODULE_KIND_INVALID = 0,

    /*
     * Ordinary module originating from Vitte source.
     */
    VITTE_MODULE_KIND_SOURCE,

    /*
     * Library module.
     */
    VITTE_MODULE_KIND_LIBRARY,

    /*
     * Package-level module.
     */
    VITTE_MODULE_KIND_PACKAGE,

    /*
     * Compiler/runtime-provided module.
     */
    VITTE_MODULE_KIND_BUILTIN,

    /*
     * Module whose implementation is external to the current compilation.
     */
    VITTE_MODULE_KIND_EXTERNAL,

    VITTE_MODULE_KIND_COUNT
} vitte_module_kind_t;

/* ========================================================================= */
/* Visibility                                                                */
/* ========================================================================= */

typedef enum vitte_module_visibility {
    VITTE_MODULE_VISIBILITY_PRIVATE = 0,
    VITTE_MODULE_VISIBILITY_PUBLIC,

    VITTE_MODULE_VISIBILITY_COUNT
} vitte_module_visibility_t;

/* ========================================================================= */
/* Symbol kinds                                                              */
/* ========================================================================= */

typedef enum vitte_module_symbol_kind {
    VITTE_MODULE_SYMBOL_INVALID = 0,

    VITTE_MODULE_SYMBOL_CONST,
    VITTE_MODULE_SYMBOL_STATIC,

    VITTE_MODULE_SYMBOL_TYPE,
    VITTE_MODULE_SYMBOL_FORM,
    VITTE_MODULE_SYMBOL_PICK,
    VITTE_MODULE_SYMBOL_TRAIT,

    VITTE_MODULE_SYMBOL_PROC,
    VITTE_MODULE_SYMBOL_MACRO,

    VITTE_MODULE_SYMBOL_MODULE,

    VITTE_MODULE_SYMBOL_TEST,

    VITTE_MODULE_SYMBOL_COUNT
} vitte_module_symbol_kind_t;

/* ========================================================================= */
/* Source span                                                               */
/* ========================================================================= */

/*
 * Generic source span.
 *
 * Half-open byte interval:
 *
 *     [begin, end)
 *
 * file_id/source identity is intentionally represented as a numeric source
 * ID so the module layer remains independent from lexer/parser source
 * managers.
 */
typedef struct vitte_module_span {
    vitte_module_source_id_t source_id;

    size_t begin;
    size_t end;

    bool valid;
} vitte_module_span_t;

/* ========================================================================= */
/* Interned string                                                           */
/* ========================================================================= */

typedef struct vitte_module_string {
    vitte_module_string_id_t id;

    /*
     * Owned, NUL-terminated storage.
     *
     * length excludes the trailing NUL byte.
     */
    char *data;

    size_t length;

    /*
     * FNV-1a hash of exactly length bytes.
     */
    uint64_t hash;
} vitte_module_string_t;

/* ========================================================================= */
/* Module descriptor                                                         */
/* ========================================================================= */

typedef struct vitte_module_desc {
    /*
     * Canonical module name.
     *
     * Examples:
     *
     *     core
     *     core::memory
     *     std::io
     */
    vitte_module_string_id_t name;

    /*
     * Optional canonical source path.
     *
     * Zero means "no source path".
     */
    vitte_module_string_id_t path;

    /*
     * Parent module.
     *
     * Zero means root/no parent.
     */
    vitte_module_id_t parent;

    /*
     * Source manager identifier associated with this module.
     *
     * Zero may mean no source.
     */
    vitte_module_source_id_t source_id;

    vitte_module_visibility_t visibility;
    vitte_module_kind_t kind;

    vitte_module_span_t span;

    bool is_root;
    bool is_external;
} vitte_module_desc_t;

/* ========================================================================= */
/* Module                                                                    */
/* ========================================================================= */

typedef struct vitte_module {
    vitte_module_id_t id;

    vitte_module_string_id_t name;
    vitte_module_string_id_t path;

    vitte_module_id_t parent;

    vitte_module_source_id_t source_id;

    vitte_module_visibility_t visibility;
    vitte_module_kind_t kind;

    vitte_module_span_t span;

    bool is_root;
    bool is_external;

    /*
     * Direct children only.
     */
    vitte_module_id_t *children;

    size_t child_count;
    size_t child_capacity;

    /*
     * Symbols declared directly in this module.
     */
    vitte_module_symbol_id_t *symbols;

    size_t symbol_count;
    size_t symbol_capacity;
} vitte_module_t;

/* ========================================================================= */
/* Symbol descriptor                                                         */
/* ========================================================================= */

typedef struct vitte_module_symbol_desc {
    vitte_module_id_t module;

    vitte_module_string_id_t name;

    vitte_module_symbol_kind_t kind;
    vitte_module_visibility_t visibility;

    vitte_module_span_t span;

    /*
     * Opaque compiler-stage payload.
     *
     * Typical users may store an AST/HIR/declaration ID here.
     *
     * The module subsystem never dereferences or interprets it.
     */
    vitte_module_payload_t payload;
} vitte_module_symbol_desc_t;

/* ========================================================================= */
/* Symbol                                                                    */
/* ========================================================================= */

typedef struct vitte_module_symbol {
    vitte_module_symbol_id_t id;

    vitte_module_id_t module;

    vitte_module_string_id_t name;

    vitte_module_symbol_kind_t kind;
    vitte_module_visibility_t visibility;

    vitte_module_span_t span;

    vitte_module_payload_t payload;
} vitte_module_symbol_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_module_stats {
    uint64_t modules_registered;
    uint64_t symbols_registered;

    uint64_t strings_interned;
    uint64_t string_bytes;

    uint64_t external_modules;

    uint64_t duplicate_modules;
    uint64_t duplicate_symbols;

    uint64_t validation_runs;
    uint64_t validation_failures;

    uint64_t hash_runs;

    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;
} vitte_module_stats_t;

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

/*
 * API-v1 transparent context.
 *
 * This is intentionally visible because the compiler may stack-allocate
 * contexts and because low-level bootstrap/debug tooling benefits from
 * direct inspection.
 *
 * Consumers should nevertheless mutate it only through the API.
 */
typedef struct vitte_module_context {
    uint64_t magic;

    vitte_module_state_t state;
    vitte_module_error_t last_error;

    /* --------------------------------------------------------------------- */
    /* Interned strings                                                      */
    /* --------------------------------------------------------------------- */

    vitte_module_string_t *strings;

    size_t string_count;
    size_t string_capacity;

    /*
     * Total owned bytes including each trailing NUL byte.
     */
    size_t string_bytes;

    /* --------------------------------------------------------------------- */
    /* Modules                                                               */
    /* --------------------------------------------------------------------- */

    vitte_module_t *modules;

    size_t module_count;
    size_t module_capacity;

    /* --------------------------------------------------------------------- */
    /* Symbols                                                               */
    /* --------------------------------------------------------------------- */

    vitte_module_symbol_t *symbols;

    size_t symbol_count;
    size_t symbol_capacity;

    /* --------------------------------------------------------------------- */
    /* Limits                                                                */
    /* --------------------------------------------------------------------- */

    size_t max_modules;
    size_t max_symbols;
    size_t max_string_bytes;

    /* --------------------------------------------------------------------- */
    /* Statistics                                                            */
    /* --------------------------------------------------------------------- */

    vitte_module_stats_t stats;

    /*
     * Increased by reset().
     *
     * IDs from an earlier generation must not be mixed with IDs from a new
     * generation.
     */
    uint64_t generation;
} vitte_module_context_t;

/* ========================================================================= */
/* Enum names                                                                */
/* ========================================================================= */

const char *
vitte_module_error_name(
    vitte_module_error_t error);

const char *
vitte_module_state_name(
    vitte_module_state_t state);

const char *
vitte_module_kind_name(
    vitte_module_kind_t kind);

const char *
vitte_module_symbol_kind_name(
    vitte_module_symbol_kind_t kind);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_module_init(
    vitte_module_context_t *context);

bool
vitte_module_reset(
    vitte_module_context_t *context);

void
vitte_module_destroy(
    vitte_module_context_t *context);

bool
vitte_module_is_valid(
    const vitte_module_context_t *context);

/* ========================================================================= */
/* String interning                                                          */
/* ========================================================================= */

vitte_module_string_id_t
vitte_module_intern_string(
    vitte_module_context_t *context,
    const char *data,
    size_t length);

const vitte_module_string_t *
vitte_module_get_string(
    const vitte_module_context_t *context,
    vitte_module_string_id_t id);

/* ========================================================================= */
/* Modules                                                                   */
/* ========================================================================= */

vitte_module_id_t
vitte_module_add(
    vitte_module_context_t *context,
    const vitte_module_desc_t *description);

const vitte_module_t *
vitte_module_get(
    const vitte_module_context_t *context,
    vitte_module_id_t id);

vitte_module_t *
vitte_module_get_mut(
    vitte_module_context_t *context,
    vitte_module_id_t id);

/* ========================================================================= */
/* Module lookup                                                             */
/* ========================================================================= */

vitte_module_id_t
vitte_module_find_by_name(
    const vitte_module_context_t *context,
    vitte_module_string_id_t name);

vitte_module_id_t
vitte_module_find_by_name_bytes(
    const vitte_module_context_t *context,
    const char *name,
    size_t length);

vitte_module_id_t
vitte_module_find_by_path(
    const vitte_module_context_t *context,
    vitte_module_string_id_t path);

/* ========================================================================= */
/* Module hierarchy                                                          */
/* ========================================================================= */

vitte_module_id_t
vitte_module_parent(
    const vitte_module_context_t *context,
    vitte_module_id_t module);

size_t
vitte_module_child_count(
    const vitte_module_context_t *context,
    vitte_module_id_t module);

vitte_module_id_t
vitte_module_child_at(
    const vitte_module_context_t *context,
    vitte_module_id_t module,
    size_t index);

/* ========================================================================= */
/* Symbols                                                                   */
/* ========================================================================= */

vitte_module_symbol_id_t
vitte_module_add_symbol(
    vitte_module_context_t *context,
    const vitte_module_symbol_desc_t *description);

const vitte_module_symbol_t *
vitte_module_get_symbol(
    const vitte_module_context_t *context,
    vitte_module_symbol_id_t id);

vitte_module_symbol_t *
vitte_module_get_symbol_mut(
    vitte_module_context_t *context,
    vitte_module_symbol_id_t id);

vitte_module_symbol_id_t
vitte_module_find_symbol(
    const vitte_module_context_t *context,
    vitte_module_id_t module,
    vitte_module_string_id_t name);

size_t
vitte_module_symbol_count(
    const vitte_module_context_t *context,
    vitte_module_id_t module);

vitte_module_symbol_id_t
vitte_module_symbol_at(
    const vitte_module_context_t *context,
    vitte_module_id_t module,
    size_t index);

/* ========================================================================= */
/* Validation / sealing                                                      */
/* ========================================================================= */

/*
 * Perform complete structural validation.
 *
 * Checks include:
 *
 *   - context envelope;
 *   - resource limits;
 *   - contiguous/stable IDs;
 *   - interned-string integrity;
 *   - string hashes;
 *   - module names;
 *   - module paths;
 *   - parent existence;
 *   - absence of parent cycles;
 *   - reciprocal parent/child relationships;
 *   - duplicate child IDs;
 *   - symbol ownership;
 *   - duplicate symbol IDs;
 *   - duplicate module names;
 *   - duplicate source paths;
 *   - symbol-name uniqueness inside each module;
 *   - enum validity.
 */
bool
vitte_module_validate(
    vitte_module_context_t *context);

/*
 * Validate and make the context immutable.
 *
 * BUILDING -> VALIDATING -> SEALED
 *
 * or:
 *
 * BUILDING -> VALIDATING -> FAILED
 */
bool
vitte_module_seal(
    vitte_module_context_t *context);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Deterministic FNV-1a-based structural fingerprint.
 *
 * Integer fields are serialized explicitly rather than hashed using their
 * in-memory representation. This avoids host-endianness dependencies.
 *
 * The fingerprint includes:
 *
 *   - API version;
 *   - string contents;
 *   - module identities;
 *   - module hierarchy;
 *   - module metadata;
 *   - child ordering;
 *   - symbol ordering;
 *   - symbol identities;
 *   - symbol kinds;
 *   - visibility;
 *   - opaque payload IDs.
 *
 * It is intended for:
 *
 *   - regression tests;
 *   - incremental compilation bookkeeping;
 *   - reproducibility checks;
 *   - cache keys combined with stronger contextual hashes.
 *
 * It is not cryptographic.
 */
uint64_t
vitte_module_fingerprint(
    vitte_module_context_t *context);

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

/*
 * Limits may only be changed while BUILDING.
 *
 * New limits may not be smaller than already allocated logical contents.
 */
bool
vitte_module_set_limits(
    vitte_module_context_t *context,
    size_t max_modules,
    size_t max_symbols,
    size_t max_string_bytes);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_module_stats_t
vitte_module_stats(
    const vitte_module_context_t *context);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_module_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline ID helpers                                                         */
/* ========================================================================= */

static inline bool
vitte_module_id_is_valid(
    vitte_module_id_t id)
{
    return id != VITTE_MODULE_INVALID_ID;
}

static inline bool
vitte_module_string_id_is_valid(
    vitte_module_string_id_t id)
{
    return id != VITTE_MODULE_INVALID_ID;
}

static inline bool
vitte_module_symbol_id_is_valid(
    vitte_module_symbol_id_t id)
{
    return id != VITTE_MODULE_INVALID_ID;
}

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_module_error_is_valid(
    vitte_module_error_t error)
{
    return error >= VITTE_MODULE_ERROR_NONE &&
           error < VITTE_MODULE_ERROR_COUNT;
}

static inline bool
vitte_module_state_is_valid(
    vitte_module_state_t state)
{
    return state > VITTE_MODULE_STATE_INVALID &&
           state < VITTE_MODULE_STATE_COUNT;
}

static inline bool
vitte_module_kind_is_valid(
    vitte_module_kind_t kind)
{
    return kind > VITTE_MODULE_KIND_INVALID &&
           kind < VITTE_MODULE_KIND_COUNT;
}

static inline bool
vitte_module_visibility_is_valid(
    vitte_module_visibility_t visibility)
{
    return visibility >= VITTE_MODULE_VISIBILITY_PRIVATE &&
           visibility < VITTE_MODULE_VISIBILITY_COUNT;
}

static inline bool
vitte_module_symbol_kind_is_valid(
    vitte_module_symbol_kind_t kind)
{
    return kind > VITTE_MODULE_SYMBOL_INVALID &&
           kind < VITTE_MODULE_SYMBOL_COUNT;
}

/* ========================================================================= */
/* Inline span helpers                                                       */
/* ========================================================================= */

static inline vitte_module_span_t
vitte_module_span_invalid(void)
{
    vitte_module_span_t span;

    span.source_id = VITTE_MODULE_INVALID_ID;
    span.begin = 0u;
    span.end = 0u;
    span.valid = false;

    return span;
}

static inline vitte_module_span_t
vitte_module_span_make(
    vitte_module_source_id_t source_id,
    size_t begin,
    size_t end)
{
    vitte_module_span_t span;

    span.source_id = source_id;
    span.begin = begin;
    span.end = end;
    span.valid = begin <= end;

    return span;
}

static inline size_t
vitte_module_span_length(
    const vitte_module_span_t *span)
{
    if (span == NULL ||
        !span->valid ||
        span->end < span->begin) {
        return 0u;
    }

    return span->end - span->begin;
}

static inline bool
vitte_module_span_contains(
    const vitte_module_span_t *span,
    size_t offset)
{
    return span != NULL &&
           span->valid &&
           span->begin <= offset &&
           offset < span->end;
}

/* ========================================================================= */
/* Inline context queries                                                    */
/* ========================================================================= */

static inline vitte_module_state_t
vitte_module_state(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return VITTE_MODULE_STATE_INVALID;
    }

    return context->state;
}

static inline vitte_module_error_t
vitte_module_last_error(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return VITTE_MODULE_ERROR_INVALID_CONTEXT;
    }

    return context->last_error;
}

static inline bool
vitte_module_is_building(
    const vitte_module_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_MODULE_MAGIC &&
           context->state == VITTE_MODULE_STATE_BUILDING;
}

static inline bool
vitte_module_is_sealed(
    const vitte_module_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_MODULE_MAGIC &&
           context->state == VITTE_MODULE_STATE_SEALED;
}

static inline bool
vitte_module_has_failed(
    const vitte_module_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_MODULE_MAGIC &&
           context->state == VITTE_MODULE_STATE_FAILED;
}

static inline uint64_t
vitte_module_generation(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return UINT64_C(0);
    }

    return context->generation;
}

static inline size_t
vitte_module_count(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return 0u;
    }

    return context->module_count;
}

static inline size_t
vitte_module_total_symbol_count(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return 0u;
    }

    return context->symbol_count;
}

static inline size_t
vitte_module_string_count(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return 0u;
    }

    return context->string_count;
}

static inline size_t
vitte_module_string_bytes(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return 0u;
    }

    return context->string_bytes;
}

/* ========================================================================= */
/* Inline module queries                                                     */
/* ========================================================================= */

static inline bool
vitte_module_object_is_root(
    const vitte_module_t *module)
{
    return module != NULL &&
           module->is_root;
}

static inline bool
vitte_module_object_is_external(
    const vitte_module_t *module)
{
    return module != NULL &&
           module->is_external;
}

static inline bool
vitte_module_object_is_public(
    const vitte_module_t *module)
{
    return module != NULL &&
           module->visibility ==
               VITTE_MODULE_VISIBILITY_PUBLIC;
}

static inline bool
vitte_module_object_has_parent(
    const vitte_module_t *module)
{
    return module != NULL &&
           module->parent != VITTE_MODULE_INVALID_ID;
}

static inline bool
vitte_module_object_has_source(
    const vitte_module_t *module)
{
    return module != NULL &&
           module->source_id != VITTE_MODULE_INVALID_ID;
}

static inline bool
vitte_module_object_has_path(
    const vitte_module_t *module)
{
    return module != NULL &&
           module->path != VITTE_MODULE_INVALID_ID;
}

/* ========================================================================= */
/* Inline symbol queries                                                     */
/* ========================================================================= */

static inline bool
vitte_module_symbol_is_public(
    const vitte_module_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->visibility ==
               VITTE_MODULE_VISIBILITY_PUBLIC;
}

static inline bool
vitte_module_symbol_has_payload(
    const vitte_module_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->payload != UINT64_C(0);
}

/* ========================================================================= */
/* Inline string queries                                                     */
/* ========================================================================= */

static inline const char *
vitte_module_string_data(
    const vitte_module_string_t *string)
{
    return string != NULL
        ? string->data
        : NULL;
}

static inline size_t
vitte_module_string_length(
    const vitte_module_string_t *string)
{
    return string != NULL
        ? string->length
        : 0u;
}

static inline bool
vitte_module_string_is_empty(
    const vitte_module_string_t *string)
{
    return string != NULL &&
           string->length == 0u;
}

/* ========================================================================= */
/* Indexed module access                                                     */
/* ========================================================================= */

static inline const vitte_module_t *
vitte_module_at(
    const vitte_module_context_t *context,
    size_t index)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC ||
        index >= context->module_count) {
        return NULL;
    }

    return &context->modules[index];
}

static inline const vitte_module_symbol_t *
vitte_module_global_symbol_at(
    const vitte_module_context_t *context,
    size_t index)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC ||
        index >= context->symbol_count) {
        return NULL;
    }

    return &context->symbols[index];
}

static inline const vitte_module_string_t *
vitte_module_string_at(
    const vitte_module_context_t *context,
    size_t index)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC ||
        index >= context->string_count) {
        return NULL;
    }

    return &context->strings[index];
}

/* ========================================================================= */
/* Resource-limit queries                                                    */
/* ========================================================================= */

static inline size_t
vitte_module_max_modules(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return 0u;
    }

    return context->max_modules;
}

static inline size_t
vitte_module_max_symbols(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return 0u;
    }

    return context->max_symbols;
}

static inline size_t
vitte_module_max_string_bytes(
    const vitte_module_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_MODULE_MAGIC) {
        return 0u;
    }

    return context->max_string_bytes;
}

/* ========================================================================= */
/* Descriptor constructors                                                   */
/* ========================================================================= */

static inline vitte_module_desc_t
vitte_module_desc_make(
    vitte_module_string_id_t name,
    vitte_module_kind_t kind)
{
    vitte_module_desc_t description;

    description.name = name;
    description.path = VITTE_MODULE_INVALID_ID;
    description.parent = VITTE_MODULE_INVALID_ID;
    description.source_id = VITTE_MODULE_INVALID_ID;

    description.visibility =
        VITTE_MODULE_VISIBILITY_PRIVATE;

    description.kind = kind;

    description.span =
        vitte_module_span_invalid();

    description.is_root = false;
    description.is_external =
        kind == VITTE_MODULE_KIND_EXTERNAL;

    return description;
}

static inline vitte_module_symbol_desc_t
vitte_module_symbol_desc_make(
    vitte_module_id_t module,
    vitte_module_string_id_t name,
    vitte_module_symbol_kind_t kind)
{
    vitte_module_symbol_desc_t description;

    description.module = module;
    description.name = name;
    description.kind = kind;

    description.visibility =
        VITTE_MODULE_VISIBILITY_PRIVATE;

    description.span =
        vitte_module_span_invalid();

    description.payload = UINT64_C(0);

    return description;
}

/* ========================================================================= */
/* Compile-time contract                                                     */
/* ========================================================================= */

#if defined(__cplusplus)

#define VITTE_MODULE_STATIC_ASSERT(condition, message) \
    static_assert((condition), message)

#else

#define VITTE_MODULE_STATIC_ASSERT(condition, message) \
    _Static_assert((condition), message)

#endif

VITTE_MODULE_STATIC_ASSERT(
    sizeof(uint64_t) == 8u,
    "module subsystem requires 64-bit uint64_t");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_INVALID_ID == UINT64_C(0),
    "module zero ID must remain invalid");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_DEFAULT_INITIAL_CAPACITY > 0u,
    "module initial capacity must be non-zero");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_DEFAULT_MAX_MODULES > 0u,
    "module limit must be non-zero");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_DEFAULT_MAX_SYMBOLS > 0u,
    "module symbol limit must be non-zero");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_DEFAULT_MAX_STRING_BYTES > 0u,
    "module string-byte limit must be non-zero");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_ERROR_COUNT >
        VITTE_MODULE_ERROR_INTERNAL,
    "module error enum invariant");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_STATE_COUNT >
        VITTE_MODULE_STATE_DESTROYED,
    "module state enum invariant");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_KIND_COUNT >
        VITTE_MODULE_KIND_EXTERNAL,
    "module kind enum invariant");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_SYMBOL_COUNT >
        VITTE_MODULE_SYMBOL_TEST,
    "module symbol enum invariant");

VITTE_MODULE_STATIC_ASSERT(
    VITTE_MODULE_VISIBILITY_COUNT >
        VITTE_MODULE_VISIBILITY_PUBLIC,
    "module visibility enum invariant");

VITTE_MODULE_STATIC_ASSERT(
    sizeof(vitte_module_id_t) == sizeof(uint64_t),
    "module ID ABI invariant");

VITTE_MODULE_STATIC_ASSERT(
    sizeof(vitte_module_string_id_t) == sizeof(uint64_t),
    "module string ID ABI invariant");

VITTE_MODULE_STATIC_ASSERT(
    sizeof(vitte_module_symbol_id_t) == sizeof(uint64_t),
    "module symbol ID ABI invariant");

#undef VITTE_MODULE_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif /* VITTE_MODULE_MODULE_H */
