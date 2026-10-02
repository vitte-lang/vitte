#ifndef VITTE_SYMBOL_SYMBOL_H
#define VITTE_SYMBOL_SYMBOL_H

/*
 * Vitte Compiler
 * src/symbol/symbol.h
 *
 * Canonical public symbol-table contract.
 *
 * Responsibilities:
 *   - stable symbol IDs;
 *   - declaration identity;
 *   - namespaces;
 *   - visibility;
 *   - symbol kinds;
 *   - lexical ownership;
 *   - source locations;
 *   - type association;
 *   - mutability/storage metadata;
 *   - procedure/type/trait metadata;
 *   - deterministic lookup;
 *   - duplicate detection;
 *   - symbol-table lifecycle;
 *   - validation;
 *   - deterministic fingerprints;
 *   - statistics;
 *   - bounded resource usage.
 *
 * ISO C17 / C++ compatible public header.
 */

#include "../parser/parser.h"
#include "../source/source.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

#define VITTE_SYMBOL_API_VERSION_MAJOR 1u
#define VITTE_SYMBOL_API_VERSION_MINOR 0u
#define VITTE_SYMBOL_API_VERSION_PATCH 0u

#define VITTE_SYMBOL_API_VERSION \
    ((VITTE_SYMBOL_API_VERSION_MAJOR * 10000u) + \
     (VITTE_SYMBOL_API_VERSION_MINOR * 100u) + \
     VITTE_SYMBOL_API_VERSION_PATCH)

/* ========================================================================= */
/* Magic                                                                     */
/* ========================================================================= */

#define VITTE_SYMBOL_MAGIC \
    UINT64_C(0x5649545453594d42)

#define VITTE_SYMBOL_DEAD_MAGIC \
    UINT64_C(0x4445414453594d42)

/* ========================================================================= */
/* Hashing                                                                   */
/* ========================================================================= */

#define VITTE_SYMBOL_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_SYMBOL_FNV_PRIME \
    UINT64_C(1099511628211)

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

#define VITTE_SYMBOL_DEFAULT_INITIAL_CAPACITY \
    ((size_t)128u)

#define VITTE_SYMBOL_DEFAULT_INITIAL_BUCKET_CAPACITY \
    ((size_t)256u)

#define VITTE_SYMBOL_DEFAULT_INITIAL_STRING_CAPACITY \
    ((size_t)4096u)

#define VITTE_SYMBOL_DEFAULT_MAX_SYMBOLS \
    ((size_t)16777216u)

#define VITTE_SYMBOL_DEFAULT_MAX_STRING_BYTES \
    ((size_t)(1024u * 1024u * 1024u))

/* ========================================================================= */
/* IDs                                                                       */
/* ========================================================================= */

typedef uint64_t vitte_symbol_id_t;
typedef uint64_t vitte_symbol_scope_id_t;
typedef uint64_t vitte_symbol_type_id_t;

#define VITTE_SYMBOL_INVALID_ID \
    ((vitte_symbol_id_t)UINT64_C(0))

#define VITTE_SYMBOL_INVALID_SCOPE_ID \
    ((vitte_symbol_scope_id_t)UINT64_C(0))

#define VITTE_SYMBOL_INVALID_TYPE_ID \
    ((vitte_symbol_type_id_t)UINT64_C(0))

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_symbol_error {
    VITTE_SYMBOL_ERROR_NONE = 0,

    VITTE_SYMBOL_ERROR_INVALID_ARGUMENT,
    VITTE_SYMBOL_ERROR_INVALID_CONTEXT,
    VITTE_SYMBOL_ERROR_INVALID_STATE,

    VITTE_SYMBOL_ERROR_INVALID_SYMBOL,
    VITTE_SYMBOL_ERROR_INVALID_SYMBOL_ID,
    VITTE_SYMBOL_ERROR_INVALID_SCOPE,
    VITTE_SYMBOL_ERROR_INVALID_NAME,
    VITTE_SYMBOL_ERROR_INVALID_NAMESPACE,
    VITTE_SYMBOL_ERROR_INVALID_KIND,

    VITTE_SYMBOL_ERROR_DUPLICATE_SYMBOL,
    VITTE_SYMBOL_ERROR_NOT_FOUND,

    VITTE_SYMBOL_ERROR_SYMBOL_LIMIT,
    VITTE_SYMBOL_ERROR_STRING_LIMIT,

    VITTE_SYMBOL_ERROR_OVERFLOW,
    VITTE_SYMBOL_ERROR_OUT_OF_MEMORY,

    VITTE_SYMBOL_ERROR_VALIDATION,
    VITTE_SYMBOL_ERROR_CORRUPTION,

    VITTE_SYMBOL_ERROR_INTERNAL,

    VITTE_SYMBOL_ERROR_COUNT
} vitte_symbol_error_t;

/* ========================================================================= */
/* Context state                                                             */
/* ========================================================================= */

typedef enum vitte_symbol_state {
    VITTE_SYMBOL_STATE_INVALID = 0,

    VITTE_SYMBOL_STATE_READY,
    VITTE_SYMBOL_STATE_BUILDING,
    VITTE_SYMBOL_STATE_FROZEN,
    VITTE_SYMBOL_STATE_FAILED,
    VITTE_SYMBOL_STATE_DESTROYED,

    VITTE_SYMBOL_STATE_COUNT
} vitte_symbol_state_t;

/* ========================================================================= */
/* Symbol namespaces                                                         */
/* ========================================================================= */

typedef enum vitte_symbol_namespace {
    VITTE_SYMBOL_NAMESPACE_INVALID = 0,

    /*
     * Ordinary value namespace:
     * variables, constants, statics, procedures, parameters.
     */
    VITTE_SYMBOL_NAMESPACE_VALUE,

    /*
     * Type namespace:
     * type, opaque, form, pick.
     */
    VITTE_SYMBOL_NAMESPACE_TYPE,

    /*
     * Trait namespace.
     */
    VITTE_SYMBOL_NAMESPACE_TRAIT,

    /*
     * Macro namespace.
     */
    VITTE_SYMBOL_NAMESPACE_MACRO,

    /*
     * Module/space namespace.
     */
    VITTE_SYMBOL_NAMESPACE_SPACE,

    VITTE_SYMBOL_NAMESPACE_COUNT
} vitte_symbol_namespace_t;

/* ========================================================================= */
/* Symbol kinds                                                              */
/* ========================================================================= */

typedef enum vitte_symbol_kind {
    VITTE_SYMBOL_KIND_INVALID = 0,

    /*
     * Module/namespace level.
     */
    VITTE_SYMBOL_KIND_SPACE,
    VITTE_SYMBOL_KIND_IMPORT,

    /*
     * Values.
     */
    VITTE_SYMBOL_KIND_CONST,
    VITTE_SYMBOL_KIND_STATIC,
    VITTE_SYMBOL_KIND_LOCAL,
    VITTE_SYMBOL_KIND_PARAMETER,

    /*
     * Procedures.
     */
    VITTE_SYMBOL_KIND_PROC,
    VITTE_SYMBOL_KIND_EXTERN_PROC,

    /*
     * Types.
     */
    VITTE_SYMBOL_KIND_TYPE,
    VITTE_SYMBOL_KIND_OPAQUE,
    VITTE_SYMBOL_KIND_FORM,
    VITTE_SYMBOL_KIND_FIELD,
    VITTE_SYMBOL_KIND_PICK,
    VITTE_SYMBOL_KIND_VARIANT,

    /*
     * Traits / implementations.
     */
    VITTE_SYMBOL_KIND_TRAIT,
    VITTE_SYMBOL_KIND_TRAIT_ITEM,
    VITTE_SYMBOL_KIND_IMPL,

    /*
     * Compile-time.
     */
    VITTE_SYMBOL_KIND_MACRO,

    /*
     * Generic declarations.
     */
    VITTE_SYMBOL_KIND_GENERIC_PARAMETER,

    /*
     * Test declarations.
     */
    VITTE_SYMBOL_KIND_TEST,

    /*
     * Internal alias/import binding.
     */
    VITTE_SYMBOL_KIND_ALIAS,

    VITTE_SYMBOL_KIND_COUNT
} vitte_symbol_kind_t;

/* ========================================================================= */
/* Visibility                                                                */
/* ========================================================================= */

typedef enum vitte_symbol_visibility {
    VITTE_SYMBOL_VISIBILITY_INVALID = 0,

    VITTE_SYMBOL_VISIBILITY_PRIVATE,
    VITTE_SYMBOL_VISIBILITY_PUBLIC,

    VITTE_SYMBOL_VISIBILITY_COUNT
} vitte_symbol_visibility_t;

/* ========================================================================= */
/* Storage                                                                   */
/* ========================================================================= */

typedef enum vitte_symbol_storage {
    VITTE_SYMBOL_STORAGE_INVALID = 0,

    VITTE_SYMBOL_STORAGE_NONE,

    VITTE_SYMBOL_STORAGE_LOCAL,
    VITTE_SYMBOL_STORAGE_PARAMETER,
    VITTE_SYMBOL_STORAGE_STATIC,
    VITTE_SYMBOL_STORAGE_GLOBAL,
    VITTE_SYMBOL_STORAGE_EXTERN,
    VITTE_SYMBOL_STORAGE_COMPTIME,

    VITTE_SYMBOL_STORAGE_COUNT
} vitte_symbol_storage_t;

/* ========================================================================= */
/* Symbol flags                                                              */
/* ========================================================================= */

typedef uint64_t vitte_symbol_flags_t;

#define VITTE_SYMBOL_FLAG_NONE \
    UINT64_C(0)

#define VITTE_SYMBOL_FLAG_PUBLIC \
    (UINT64_C(1) << 0)

#define VITTE_SYMBOL_FLAG_MUTABLE \
    (UINT64_C(1) << 1)

#define VITTE_SYMBOL_FLAG_EXTERN \
    (UINT64_C(1) << 2)

#define VITTE_SYMBOL_FLAG_ASYNC \
    (UINT64_C(1) << 3)

#define VITTE_SYMBOL_FLAG_UNSAFE \
    (UINT64_C(1) << 4)

#define VITTE_SYMBOL_FLAG_COMPTIME \
    (UINT64_C(1) << 5)

#define VITTE_SYMBOL_FLAG_DECLARED \
    (UINT64_C(1) << 6)

#define VITTE_SYMBOL_FLAG_DEFINED \
    (UINT64_C(1) << 7)

#define VITTE_SYMBOL_FLAG_REFERENCED \
    (UINT64_C(1) << 8)

#define VITTE_SYMBOL_FLAG_USED \
    (UINT64_C(1) << 9)

#define VITTE_SYMBOL_FLAG_IMPORTED \
    (UINT64_C(1) << 10)

#define VITTE_SYMBOL_FLAG_BUILTIN \
    (UINT64_C(1) << 11)

#define VITTE_SYMBOL_FLAG_GENERIC \
    (UINT64_C(1) << 12)

#define VITTE_SYMBOL_FLAG_DEPRECATED \
    (UINT64_C(1) << 13)

#define VITTE_SYMBOL_FLAG_SYNTHETIC \
    (UINT64_C(1) << 14)

#define VITTE_SYMBOL_FLAG_SHADOWS \
    (UINT64_C(1) << 15)

#define VITTE_SYMBOL_FLAG_ADDRESS_TAKEN \
    (UINT64_C(1) << 16)

#define VITTE_SYMBOL_FLAG_CAPTURED \
    (UINT64_C(1) << 17)

#define VITTE_SYMBOL_FLAG_READ \
    (UINT64_C(1) << 18)

#define VITTE_SYMBOL_FLAG_WRITTEN \
    (UINT64_C(1) << 19)

/* ========================================================================= */
/* Source location                                                           */
/* ========================================================================= */

typedef struct vitte_symbol_location {
    vitte_source_id_t source_id;

    size_t begin;
    size_t end;

    size_t line;
    size_t column;

    bool valid;
} vitte_symbol_location_t;

/* ========================================================================= */
/* Name                                                                      */
/* ========================================================================= */

typedef struct vitte_symbol_name {
    const char *data;
    size_t length;

    /*
     * Deterministic FNV-1a name hash.
     */
    uint64_t hash;
} vitte_symbol_name_t;

/* ========================================================================= */
/* Symbol                                                                    */
/* ========================================================================= */

typedef struct vitte_symbol {
    vitte_symbol_id_t id;

    vitte_symbol_kind_t kind;
    vitte_symbol_namespace_t name_space;
    vitte_symbol_visibility_t visibility;
    vitte_symbol_storage_t storage;

    vitte_symbol_flags_t flags;

    /*
     * Interned name.
     */
    const char *name;
    size_t name_length;
    uint64_t name_hash;

    /*
     * Lexical owner.
     */
    vitte_symbol_scope_id_t scope_id;

    /*
     * Parent semantic declaration.
     *
     * Examples:
     *   field   -> form
     *   variant -> pick
     *   method  -> trait/impl
     */
    vitte_symbol_id_t parent_symbol_id;

    /*
     * Symbol shadowed by this declaration, if any.
     */
    vitte_symbol_id_t shadowed_symbol_id;

    /*
     * Associated semantic type.
     *
     * The symbol layer intentionally stores an opaque type ID instead of
     * depending on sema.h. This prevents symbol <-> sema include cycles.
     */
    vitte_symbol_type_id_t type_id;

    /*
     * AST declaration node.
     */
    vitte_ast_node_id_t declaration_node_id;

    /*
     * Source declaration.
     */
    vitte_symbol_location_t location;

    /*
     * Hash-table collision chain.
     *
     * Stored as symbol ID, never a raw pointer, so reallocating the symbol
     * array does not invalidate the table.
     */
    vitte_symbol_id_t next_in_bucket;

    /*
     * Number of resolved references to this symbol.
     */
    uint64_t reference_count;
} vitte_symbol_t;

/* ========================================================================= */
/* Symbol declaration                                                        */
/* ========================================================================= */

typedef struct vitte_symbol_declaration {
    const char *name;
    size_t name_length;

    vitte_symbol_kind_t kind;
    vitte_symbol_namespace_t name_space;
    vitte_symbol_visibility_t visibility;
    vitte_symbol_storage_t storage;

    vitte_symbol_flags_t flags;

    vitte_symbol_scope_id_t scope_id;
    vitte_symbol_id_t parent_symbol_id;
    vitte_symbol_id_t shadowed_symbol_id;

    vitte_symbol_type_id_t type_id;

    vitte_ast_node_id_t declaration_node_id;

    vitte_symbol_location_t location;
} vitte_symbol_declaration_t;

/* ========================================================================= */
/* Lookup key                                                                */
/* ========================================================================= */

typedef struct vitte_symbol_lookup_key {
    const char *name;
    size_t name_length;

    vitte_symbol_namespace_t name_space;
    vitte_symbol_scope_id_t scope_id;
} vitte_symbol_lookup_key_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_symbol_stats {
    uint64_t symbols_created;
    uint64_t symbols_removed;

    uint64_t lookups;
    uint64_t lookup_hits;
    uint64_t lookup_misses;

    uint64_t duplicate_rejections;

    uint64_t names_interned;
    uint64_t intern_hits;
    uint64_t string_bytes;

    uint64_t references_recorded;

    uint64_t table_rebuilds;
    uint64_t collisions;
    uint64_t maximum_chain_length;

    uint64_t validation_runs;
    uint64_t validation_failures;

    uint64_t hash_runs;

    uint64_t resets;
    uint64_t freezes;
    uint64_t failures;

    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;
} vitte_symbol_stats_t;

/* ========================================================================= */
/* Symbol context                                                            */
/* ========================================================================= */

typedef struct vitte_symbol_context {
    uint64_t magic;

    vitte_symbol_state_t state;
    vitte_symbol_error_t last_error;

    /*
     * Stable symbol storage.
     *
     * IDs are one-based:
     *
     *   symbols[id - 1]
     */
    vitte_symbol_t *symbols;
    size_t symbol_count;
    size_t symbol_capacity;

    /*
     * Hash buckets.
     *
     * Each bucket stores a symbol ID.
     */
    vitte_symbol_id_t *buckets;
    size_t bucket_count;

    /*
     * Interned string arena.
     */
    char *strings;
    size_t string_size;
    size_t string_capacity;

    /*
     * Resource limits.
     */
    size_t max_symbols;
    size_t max_string_bytes;

    vitte_symbol_stats_t stats;

    uint64_t generation;
} vitte_symbol_context_t;

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_symbol_error_name(
    vitte_symbol_error_t error);

const char *
vitte_symbol_state_name(
    vitte_symbol_state_t state);

const char *
vitte_symbol_kind_name(
    vitte_symbol_kind_t kind);

const char *
vitte_symbol_namespace_name(
    vitte_symbol_namespace_t name_space);

const char *
vitte_symbol_visibility_name(
    vitte_symbol_visibility_t visibility);

const char *
vitte_symbol_storage_name(
    vitte_symbol_storage_t storage);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_symbol_init(
    vitte_symbol_context_t *context);

bool
vitte_symbol_reset(
    vitte_symbol_context_t *context);

void
vitte_symbol_destroy(
    vitte_symbol_context_t *context);

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

bool
vitte_symbol_is_valid(
    const vitte_symbol_context_t *context);

vitte_symbol_error_t
vitte_symbol_last_error(
    const vitte_symbol_context_t *context);

uint64_t
vitte_symbol_generation(
    const vitte_symbol_context_t *context);

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_symbol_set_limits(
    vitte_symbol_context_t *context,
    size_t max_symbols,
    size_t max_string_bytes);

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

bool
vitte_symbol_begin_build(
    vitte_symbol_context_t *context);

bool
vitte_symbol_freeze(
    vitte_symbol_context_t *context);

/* ========================================================================= */
/* Symbol declaration                                                        */
/* ========================================================================= */

bool
vitte_symbol_declare(
    vitte_symbol_context_t *context,
    const vitte_symbol_declaration_t *declaration,
    vitte_symbol_id_t *out_symbol_id);

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

const vitte_symbol_t *
vitte_symbol_get(
    const vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id);

vitte_symbol_t *
vitte_symbol_get_mut(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id);

const vitte_symbol_t *
vitte_symbol_lookup(
    const vitte_symbol_context_t *context,
    const char *name,
    size_t name_length,
    vitte_symbol_namespace_t name_space,
    vitte_symbol_scope_id_t scope_id);

const vitte_symbol_t *
vitte_symbol_lookup_cstr(
    const vitte_symbol_context_t *context,
    const char *name,
    vitte_symbol_namespace_t name_space,
    vitte_symbol_scope_id_t scope_id);

bool
vitte_symbol_contains(
    const vitte_symbol_context_t *context,
    const char *name,
    size_t name_length,
    vitte_symbol_namespace_t name_space,
    vitte_symbol_scope_id_t scope_id);

/* ========================================================================= */
/* Symbol mutation                                                           */
/* ========================================================================= */

bool
vitte_symbol_set_type(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_symbol_type_id_t type_id);

bool
vitte_symbol_add_flags(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_symbol_flags_t flags);

bool
vitte_symbol_remove_flags(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_symbol_flags_t flags);

bool
vitte_symbol_record_reference(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id);

bool
vitte_symbol_record_read(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id);

bool
vitte_symbol_record_write(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id);

bool
vitte_symbol_record_address_taken(
    vitte_symbol_context_t *context,
    vitte_symbol_id_t symbol_id);

/* ========================================================================= */
/* Hashing                                                                   */
/* ========================================================================= */

uint64_t
vitte_symbol_hash_name(
    const char *name,
    size_t length);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_symbol_validate(
    vitte_symbol_context_t *context);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_symbol_fingerprint(
    vitte_symbol_context_t *context);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_symbol_stats_t
vitte_symbol_stats(
    const vitte_symbol_context_t *context);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_symbol_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline API version                                                        */
/* ========================================================================= */

static inline unsigned int
vitte_symbol_api_version_major(void)
{
    return VITTE_SYMBOL_API_VERSION_MAJOR;
}

static inline unsigned int
vitte_symbol_api_version_minor(void)
{
    return VITTE_SYMBOL_API_VERSION_MINOR;
}

static inline unsigned int
vitte_symbol_api_version_patch(void)
{
    return VITTE_SYMBOL_API_VERSION_PATCH;
}

static inline unsigned int
vitte_symbol_api_version(void)
{
    return VITTE_SYMBOL_API_VERSION;
}

/* ========================================================================= */
/* Inline ID helpers                                                         */
/* ========================================================================= */

static inline bool
vitte_symbol_id_is_valid(
    vitte_symbol_id_t symbol_id)
{
    return symbol_id != VITTE_SYMBOL_INVALID_ID;
}

static inline bool
vitte_symbol_scope_id_is_valid(
    vitte_symbol_scope_id_t scope_id)
{
    return scope_id !=
           VITTE_SYMBOL_INVALID_SCOPE_ID;
}

static inline bool
vitte_symbol_type_id_is_valid(
    vitte_symbol_type_id_t type_id)
{
    return type_id !=
           VITTE_SYMBOL_INVALID_TYPE_ID;
}

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_symbol_error_is_valid(
    vitte_symbol_error_t error)
{
    return error >= VITTE_SYMBOL_ERROR_NONE &&
           error < VITTE_SYMBOL_ERROR_COUNT;
}

static inline bool
vitte_symbol_state_is_valid(
    vitte_symbol_state_t state)
{
    return state > VITTE_SYMBOL_STATE_INVALID &&
           state < VITTE_SYMBOL_STATE_COUNT;
}

static inline bool
vitte_symbol_kind_is_valid(
    vitte_symbol_kind_t kind)
{
    return kind > VITTE_SYMBOL_KIND_INVALID &&
           kind < VITTE_SYMBOL_KIND_COUNT;
}

static inline bool
vitte_symbol_namespace_is_valid(
    vitte_symbol_namespace_t name_space)
{
    return name_space >
               VITTE_SYMBOL_NAMESPACE_INVALID &&
           name_space <
               VITTE_SYMBOL_NAMESPACE_COUNT;
}

static inline bool
vitte_symbol_visibility_is_valid(
    vitte_symbol_visibility_t visibility)
{
    return visibility >
               VITTE_SYMBOL_VISIBILITY_INVALID &&
           visibility <
               VITTE_SYMBOL_VISIBILITY_COUNT;
}

static inline bool
vitte_symbol_storage_is_valid(
    vitte_symbol_storage_t storage)
{
    return storage >
               VITTE_SYMBOL_STORAGE_INVALID &&
           storage <
               VITTE_SYMBOL_STORAGE_COUNT;
}

/* ========================================================================= */
/* Inline context helpers                                                    */
/* ========================================================================= */

static inline bool
vitte_symbol_is_ready(
    const vitte_symbol_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_SYMBOL_MAGIC &&
           context->state == VITTE_SYMBOL_STATE_READY;
}

static inline bool
vitte_symbol_is_building(
    const vitte_symbol_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_SYMBOL_MAGIC &&
           context->state ==
               VITTE_SYMBOL_STATE_BUILDING;
}

static inline bool
vitte_symbol_is_frozen(
    const vitte_symbol_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_SYMBOL_MAGIC &&
           context->state ==
               VITTE_SYMBOL_STATE_FROZEN;
}

static inline bool
vitte_symbol_has_failed(
    const vitte_symbol_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_SYMBOL_MAGIC &&
           context->state ==
               VITTE_SYMBOL_STATE_FAILED;
}

static inline bool
vitte_symbol_is_destroyed(
    const vitte_symbol_context_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_SYMBOL_DEAD_MAGIC &&
           context->state ==
               VITTE_SYMBOL_STATE_DESTROYED;
}

/* ========================================================================= */
/* Inline context counts                                                     */
/* ========================================================================= */

static inline size_t
vitte_symbol_count(
    const vitte_symbol_context_t *context)
{
    return context != NULL
        ? context->symbol_count
        : 0u;
}

static inline bool
vitte_symbol_empty(
    const vitte_symbol_context_t *context)
{
    return context == NULL ||
           context->symbol_count == 0u;
}

/* ========================================================================= */
/* Inline location helpers                                                   */
/* ========================================================================= */

static inline bool
vitte_symbol_location_is_valid(
    vitte_symbol_location_t location)
{
    return location.valid &&
           location.source_id !=
               VITTE_SOURCE_INVALID_ID &&
           location.begin <= location.end;
}

static inline size_t
vitte_symbol_location_length(
    vitte_symbol_location_t location)
{
    if (!location.valid ||
        location.end < location.begin) {
        return 0u;
    }

    return location.end - location.begin;
}

static inline bool
vitte_symbol_location_is_empty(
    vitte_symbol_location_t location)
{
    return vitte_symbol_location_length(
               location) == 0u;
}

/* ========================================================================= */
/* Inline symbol helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_symbol_entry_is_valid(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->id !=
               VITTE_SYMBOL_INVALID_ID &&
           vitte_symbol_kind_is_valid(
               symbol->kind) &&
           vitte_symbol_namespace_is_valid(
               symbol->name_space) &&
           symbol->name != NULL;
}

static inline bool
vitte_symbol_has_flag(
    const vitte_symbol_t *symbol,
    vitte_symbol_flags_t flag)
{
    return symbol != NULL &&
           (symbol->flags & flag) !=
               VITTE_SYMBOL_FLAG_NONE;
}

static inline bool
vitte_symbol_is_public(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           (symbol->visibility ==
                VITTE_SYMBOL_VISIBILITY_PUBLIC ||
            vitte_symbol_has_flag(
                symbol,
                VITTE_SYMBOL_FLAG_PUBLIC));
}

static inline bool
vitte_symbol_is_mutable(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_MUTABLE);
}

static inline bool
vitte_symbol_is_extern(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_EXTERN);
}

static inline bool
vitte_symbol_is_async(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_ASYNC);
}

static inline bool
vitte_symbol_is_unsafe(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_UNSAFE);
}

static inline bool
vitte_symbol_is_comptime(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_COMPTIME);
}

static inline bool
vitte_symbol_is_defined(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_DEFINED);
}

static inline bool
vitte_symbol_is_referenced(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           (symbol->reference_count != UINT64_C(0) ||
            vitte_symbol_has_flag(
                symbol,
                VITTE_SYMBOL_FLAG_REFERENCED));
}

static inline bool
vitte_symbol_is_builtin(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_BUILTIN);
}

static inline bool
vitte_symbol_is_imported(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_IMPORTED);
}

static inline bool
vitte_symbol_is_generic(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_GENERIC);
}

static inline bool
vitte_symbol_is_deprecated(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_DEPRECATED);
}

static inline bool
vitte_symbol_is_synthetic(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_SYNTHETIC);
}

static inline bool
vitte_symbol_is_value(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->name_space ==
               VITTE_SYMBOL_NAMESPACE_VALUE;
}

static inline bool
vitte_symbol_is_type(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->name_space ==
               VITTE_SYMBOL_NAMESPACE_TYPE;
}

static inline bool
vitte_symbol_is_trait(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->name_space ==
               VITTE_SYMBOL_NAMESPACE_TRAIT;
}

static inline bool
vitte_symbol_is_macro(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->name_space ==
               VITTE_SYMBOL_NAMESPACE_MACRO;
}

static inline bool
vitte_symbol_is_space(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->name_space ==
               VITTE_SYMBOL_NAMESPACE_SPACE;
}

static inline bool
vitte_symbol_is_callable(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           (symbol->kind ==
                VITTE_SYMBOL_KIND_PROC ||
            symbol->kind ==
                VITTE_SYMBOL_KIND_EXTERN_PROC);
}

static inline bool
vitte_symbol_is_variable(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           (symbol->kind ==
                VITTE_SYMBOL_KIND_LOCAL ||
            symbol->kind ==
                VITTE_SYMBOL_KIND_PARAMETER ||
            symbol->kind ==
                VITTE_SYMBOL_KIND_STATIC);
}

static inline bool
vitte_symbol_is_constant(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->kind ==
               VITTE_SYMBOL_KIND_CONST;
}

static inline bool
vitte_symbol_is_type_declaration(
    const vitte_symbol_t *symbol)
{
    if (symbol == NULL) {
        return false;
    }

    switch (symbol->kind) {
        case VITTE_SYMBOL_KIND_TYPE:
        case VITTE_SYMBOL_KIND_OPAQUE:
        case VITTE_SYMBOL_KIND_FORM:
        case VITTE_SYMBOL_KIND_PICK:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_symbol_has_type(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->type_id !=
               VITTE_SYMBOL_INVALID_TYPE_ID;
}

static inline bool
vitte_symbol_has_parent(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->parent_symbol_id !=
               VITTE_SYMBOL_INVALID_ID;
}

static inline bool
vitte_symbol_shadows_other(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->shadowed_symbol_id !=
               VITTE_SYMBOL_INVALID_ID;
}

/* ========================================================================= */
/* Inline name helpers                                                       */
/* ========================================================================= */

static inline vitte_symbol_name_t
vitte_symbol_make_name(
    const char *data,
    size_t length)
{
    vitte_symbol_name_t result;

    result.data = data;
    result.length = length;
    result.hash =
        data != NULL
            ? vitte_symbol_hash_name(
                  data,
                  length)
            : UINT64_C(0);

    return result;
}

static inline bool
vitte_symbol_name_is_valid(
    vitte_symbol_name_t name)
{
    return name.data != NULL &&
           name.length != 0u;
}

static inline bool
vitte_symbol_name_equal(
    vitte_symbol_name_t left,
    vitte_symbol_name_t right)
{
    size_t index;

    if (left.length != right.length) {
        return false;
    }

    if (left.length == 0u) {
        return true;
    }

    if (left.data == NULL ||
        right.data == NULL) {
        return false;
    }

    if (left.hash != UINT64_C(0) &&
        right.hash != UINT64_C(0) &&
        left.hash != right.hash) {
        return false;
    }

    for (index = 0u;
         index < left.length;
         ++index) {
        if (left.data[index] !=
            right.data[index]) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__cplusplus)

static_assert(
    sizeof(vitte_symbol_id_t) ==
        sizeof(uint64_t),
    "vitte_symbol_id_t must be 64-bit");

static_assert(
    sizeof(vitte_symbol_scope_id_t) ==
        sizeof(uint64_t),
    "vitte_symbol_scope_id_t must be 64-bit");

static_assert(
    sizeof(vitte_symbol_type_id_t) ==
        sizeof(uint64_t),
    "vitte_symbol_type_id_t must be 64-bit");

static_assert(
    VITTE_SYMBOL_INVALID_ID ==
        UINT64_C(0),
    "invalid symbol ID must be zero");

static_assert(
    VITTE_SYMBOL_KIND_INVALID == 0,
    "invalid symbol kind must be zero");

static_assert(
    VITTE_SYMBOL_NAMESPACE_INVALID == 0,
    "invalid symbol namespace must be zero");

static_assert(
    VITTE_SYMBOL_STATE_INVALID == 0,
    "invalid symbol state must be zero");

#else

_Static_assert(
    sizeof(vitte_symbol_id_t) ==
        sizeof(uint64_t),
    "vitte_symbol_id_t must be 64-bit");

_Static_assert(
    sizeof(vitte_symbol_scope_id_t) ==
        sizeof(uint64_t),
    "vitte_symbol_scope_id_t must be 64-bit");

_Static_assert(
    sizeof(vitte_symbol_type_id_t) ==
        sizeof(uint64_t),
    "vitte_symbol_type_id_t must be 64-bit");

_Static_assert(
    VITTE_SYMBOL_INVALID_ID ==
        UINT64_C(0),
    "invalid symbol ID must be zero");

_Static_assert(
    VITTE_SYMBOL_KIND_INVALID == 0,
    "invalid symbol kind must be zero");

_Static_assert(
    VITTE_SYMBOL_NAMESPACE_INVALID == 0,
    "invalid symbol namespace must be zero");

_Static_assert(
    VITTE_SYMBOL_STATE_INVALID == 0,
    "invalid symbol state must be zero");

#endif

#ifdef __cplusplus
}
#endif

#endif /* VITTE_SYMBOL_SYMBOL_H */
