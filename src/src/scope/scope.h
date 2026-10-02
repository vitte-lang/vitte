#ifndef VITTE_SCOPE_SCOPE_H
#define VITTE_SCOPE_SCOPE_H

/*
 * Vitte Compiler
 * src/scope/scope.h
 *
 * Public lexical scope, symbol-table and name-resolution contract.
 *
 * Synchronized with scope.c.
 *
 * Responsibilities:
 *   - lexical scope hierarchy;
 *   - stable scope IDs;
 *   - stable symbol IDs;
 *   - deterministic declaration order;
 *   - local and recursive name lookup;
 *   - public/private visibility;
 *   - lexical shadowing;
 *   - source spans;
 *   - opaque compiler payload association;
 *   - resource limits;
 *   - validation;
 *   - deterministic fingerprints;
 *   - statistics;
 *   - explicit lifecycle.
 *
 * Design:
 *   - ISO C17;
 *   - C/C++ compatible public API;
 *   - scope.h is the single public contract;
 *   - no parser/HIR/IR dependency;
 *   - no global mutable state;
 *   - names are owned by the scope context;
 *   - payload pointers remain owned by the caller;
 *   - scope/symbol IDs start at 1;
 *   - ID zero is always invalid;
 *   - half-open source spans [begin, end);
 *   - bounded scope depth and resource consumption.
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

#define VITTE_SCOPE_API_VERSION_MAJOR 1u
#define VITTE_SCOPE_API_VERSION_MINOR 0u
#define VITTE_SCOPE_API_VERSION_PATCH 0u

#define VITTE_SCOPE_API_VERSION \
    ((VITTE_SCOPE_API_VERSION_MAJOR * 10000u) + \
     (VITTE_SCOPE_API_VERSION_MINOR * 100u) + \
     VITTE_SCOPE_API_VERSION_PATCH)

/* ========================================================================= */
/* Context integrity                                                         */
/* ========================================================================= */

#define VITTE_SCOPE_MAGIC \
    UINT64_C(0x5649545453434f50)

#define VITTE_SCOPE_DEAD_MAGIC \
    UINT64_C(0x4445414453434f50)

/* ========================================================================= */
/* Fingerprinting                                                            */
/* ========================================================================= */

#define VITTE_SCOPE_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_SCOPE_FNV_PRIME \
    UINT64_C(1099511628211)

/* ========================================================================= */
/* Default capacities / resource limits                                      */
/* ========================================================================= */

#define VITTE_SCOPE_DEFAULT_INITIAL_SCOPE_CAPACITY \
    ((size_t)64u)

#define VITTE_SCOPE_DEFAULT_INITIAL_SYMBOL_CAPACITY \
    ((size_t)128u)

/*
 * Hard runtime defaults. Applications can replace them before substantial
 * construction with vitte_scope_set_limits().
 */
#define VITTE_SCOPE_DEFAULT_MAX_SCOPES \
    ((size_t)1048576u)

#define VITTE_SCOPE_DEFAULT_MAX_SYMBOLS \
    ((size_t)16777216u)

#define VITTE_SCOPE_DEFAULT_MAX_NAME_BYTES \
    ((size_t)1048576u)

#define VITTE_SCOPE_DEFAULT_MAX_DEPTH \
    ((size_t)4096u)

/* ========================================================================= */
/* Stable IDs                                                                */
/* ========================================================================= */

typedef uint64_t vitte_scope_id_t;
typedef uint64_t vitte_symbol_id_t;

#define VITTE_SCOPE_INVALID_SCOPE_ID \
    ((vitte_scope_id_t)UINT64_C(0))

#define VITTE_SCOPE_INVALID_SYMBOL_ID \
    ((vitte_symbol_id_t)UINT64_C(0))

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_scope_error {
    VITTE_SCOPE_ERROR_NONE = 0,

    VITTE_SCOPE_ERROR_INVALID_ARGUMENT,
    VITTE_SCOPE_ERROR_INVALID_CONTEXT,
    VITTE_SCOPE_ERROR_INVALID_STATE,

    VITTE_SCOPE_ERROR_INVALID_SCOPE,
    VITTE_SCOPE_ERROR_INVALID_SYMBOL,
    VITTE_SCOPE_ERROR_INVALID_NAME,

    VITTE_SCOPE_ERROR_DUPLICATE_SYMBOL,

    VITTE_SCOPE_ERROR_SCOPE_LIMIT,
    VITTE_SCOPE_ERROR_SYMBOL_LIMIT,
    VITTE_SCOPE_ERROR_NAME_LIMIT,
    VITTE_SCOPE_ERROR_DEPTH_LIMIT,

    VITTE_SCOPE_ERROR_OVERFLOW,
    VITTE_SCOPE_ERROR_OUT_OF_MEMORY,

    VITTE_SCOPE_ERROR_VALIDATION,
    VITTE_SCOPE_ERROR_CORRUPTION,
    VITTE_SCOPE_ERROR_INTERNAL,

    VITTE_SCOPE_ERROR_COUNT
} vitte_scope_error_t;

/* ========================================================================= */
/* Context lifecycle                                                         */
/* ========================================================================= */

typedef enum vitte_scope_state {
    VITTE_SCOPE_STATE_INVALID = 0,

    /*
     * Scope hierarchy and symbols can be mutated.
     */
    VITTE_SCOPE_STATE_BUILDING,

    /*
     * Context is immutable and ready for stable lookup/analysis.
     */
    VITTE_SCOPE_STATE_SEALED,

    /*
     * Fatal subsystem failure.
     */
    VITTE_SCOPE_STATE_FAILED,

    /*
     * Context was explicitly destroyed.
     */
    VITTE_SCOPE_STATE_DESTROYED,

    VITTE_SCOPE_STATE_COUNT
} vitte_scope_state_t;

/* ========================================================================= */
/* Scope kinds                                                               */
/* ========================================================================= */

typedef enum vitte_scope_kind {
    VITTE_SCOPE_KIND_INVALID = 0,

    /*
     * Unique context root.
     */
    VITTE_SCOPE_KIND_ROOT,

    /*
     * `space`.
     */
    VITTE_SCOPE_KIND_SPACE,

    /*
     * Aggregate type body.
     */
    VITTE_SCOPE_KIND_FORM,

    /*
     * Variant type body.
     */
    VITTE_SCOPE_KIND_PICK,

    /*
     * Trait body.
     */
    VITTE_SCOPE_KIND_TRAIT,

    /*
     * Trait/type implementation body.
     */
    VITTE_SCOPE_KIND_IMPL,

    /*
     * Procedure body / parameters.
     */
    VITTE_SCOPE_KIND_PROC,

    /*
     * Generic lexical block.
     */
    VITTE_SCOPE_KIND_BLOCK,

    /*
     * loop/while/for body.
     */
    VITTE_SCOPE_KIND_LOOP,

    /*
     * match expression/statement scope.
     */
    VITTE_SCOPE_KIND_MATCH,

    /*
     * Individual match arm.
     */
    VITTE_SCOPE_KIND_MATCH_ARM,

    /*
     * Macro definition/expansion lexical environment.
     */
    VITTE_SCOPE_KIND_MACRO,

    /*
     * Test declaration.
     */
    VITTE_SCOPE_KIND_TEST,

    /*
     * Compile-time evaluation scope.
     */
    VITTE_SCOPE_KIND_COMPTIME,

    /*
     * Unsafe lexical block.
     */
    VITTE_SCOPE_KIND_UNSAFE,

    VITTE_SCOPE_KIND_COUNT
} vitte_scope_kind_t;

/* ========================================================================= */
/* Symbol kinds                                                              */
/* ========================================================================= */

typedef enum vitte_symbol_kind {
    VITTE_SYMBOL_KIND_INVALID = 0,

    VITTE_SYMBOL_KIND_SPACE,

    VITTE_SYMBOL_KIND_CONST,
    VITTE_SYMBOL_KIND_STATIC,

    VITTE_SYMBOL_KIND_TYPE,
    VITTE_SYMBOL_KIND_OPAQUE,

    VITTE_SYMBOL_KIND_FORM,
    VITTE_SYMBOL_KIND_FIELD,

    VITTE_SYMBOL_KIND_PICK,
    VITTE_SYMBOL_KIND_VARIANT,

    VITTE_SYMBOL_KIND_TRAIT,
    VITTE_SYMBOL_KIND_IMPL,

    VITTE_SYMBOL_KIND_PROC,
    VITTE_SYMBOL_KIND_PARAMETER,
    VITTE_SYMBOL_KIND_GENERIC_PARAMETER,

    VITTE_SYMBOL_KIND_LOCAL,

    VITTE_SYMBOL_KIND_MACRO,
    VITTE_SYMBOL_KIND_TEST,

    /*
     * Imported binding or module alias.
     */
    VITTE_SYMBOL_KIND_IMPORT,

    VITTE_SYMBOL_KIND_COUNT
} vitte_symbol_kind_t;

/* ========================================================================= */
/* Visibility                                                               */
/* ========================================================================= */

typedef enum vitte_symbol_visibility {
    VITTE_SYMBOL_VISIBILITY_PRIVATE = 0,
    VITTE_SYMBOL_VISIBILITY_PUBLIC,

    VITTE_SYMBOL_VISIBILITY_COUNT
} vitte_symbol_visibility_t;

/* ========================================================================= */
/* Symbol flags                                                              */
/* ========================================================================= */

typedef uint32_t vitte_symbol_flags_t;

#define VITTE_SYMBOL_FLAG_NONE \
    UINT32_C(0)

#define VITTE_SYMBOL_FLAG_MUTABLE \
    (UINT32_C(1) << 0)

#define VITTE_SYMBOL_FLAG_EXTERN \
    (UINT32_C(1) << 1)

#define VITTE_SYMBOL_FLAG_ASYNC \
    (UINT32_C(1) << 2)

#define VITTE_SYMBOL_FLAG_UNSAFE \
    (UINT32_C(1) << 3)

#define VITTE_SYMBOL_FLAG_COMPTIME \
    (UINT32_C(1) << 4)

#define VITTE_SYMBOL_FLAG_PARAMETER \
    (UINT32_C(1) << 5)

#define VITTE_SYMBOL_FLAG_GENERIC \
    (UINT32_C(1) << 6)

#define VITTE_SYMBOL_FLAG_IMPORTED \
    (UINT32_C(1) << 7)

#define VITTE_SYMBOL_FLAG_SYNTHETIC \
    (UINT32_C(1) << 8)

#define VITTE_SYMBOL_FLAG_USED \
    (UINT32_C(1) << 9)

#define VITTE_SYMBOL_FLAG_REFERENCED \
    (UINT32_C(1) << 10)

#define VITTE_SYMBOL_FLAG_CAPTURED \
    (UINT32_C(1) << 11)

#define VITTE_SYMBOL_FLAG_RESERVED_12 \
    (UINT32_C(1) << 12)

#define VITTE_SYMBOL_FLAG_RESERVED_13 \
    (UINT32_C(1) << 13)

#define VITTE_SYMBOL_FLAG_RESERVED_14 \
    (UINT32_C(1) << 14)

#define VITTE_SYMBOL_FLAG_RESERVED_15 \
    (UINT32_C(1) << 15)

/* ========================================================================= */
/* Source spans                                                              */
/* ========================================================================= */

/*
 * Source ranges are half-open:
 *
 *     [begin, end)
 *
 * file_id is intentionally independent of parser/lexer types so the scope
 * subsystem can remain a low-level standalone compiler component.
 */
typedef struct vitte_scope_span {
    uint32_t file_id;

    size_t begin;
    size_t end;

    size_t line;
    size_t column;

    bool valid;
} vitte_scope_span_t;

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_scope_entry vitte_scope_entry_t;
typedef struct vitte_symbol vitte_symbol_t;
typedef struct vitte_scope vitte_scope_t;

/* ========================================================================= */
/* Symbol                                                                    */
/* ========================================================================= */

struct vitte_symbol {
    /*
     * Stable global symbol ID.
     */
    vitte_symbol_id_t id;

    /*
     * Scope containing this declaration.
     */
    vitte_scope_id_t scope_id;

    /*
     * Context-owned NUL-terminated copy.
     *
     * name_length excludes the trailing NUL.
     */
    char *name;
    size_t name_length;

    /*
     * Cached FNV-1a hash of name bytes.
     */
    uint64_t name_hash;

    vitte_symbol_kind_t kind;
    vitte_symbol_visibility_t visibility;

    /*
     * VITTE_SYMBOL_FLAG_*.
     *
     * Kept as uint32_t in the public declaration API for simple ABI
     * interoperability with frontend/compiler passes.
     */
    uint32_t flags;

    vitte_scope_span_t span;

    /*
     * Opaque caller-owned semantic object.
     *
     * Examples:
     *   AST declaration,
     *   HIR declaration,
     *   type descriptor,
     *   import descriptor.
     *
     * scope.c never frees this pointer.
     */
    void *payload;

    /*
     * Nearest lexically shadowed declaration with the same name.
     *
     * Zero when this declaration shadows nothing.
     */
    vitte_symbol_id_t shadowed_symbol;
};

/* ========================================================================= */
/* Scope entry                                                               */
/* ========================================================================= */

struct vitte_scope_entry {
    /*
     * Stable scope ID.
     */
    vitte_scope_id_t id;

    /*
     * Zero only for the unique root scope.
     */
    vitte_scope_id_t parent_id;

    vitte_scope_kind_t kind;

    /*
     * Root depth is zero.
     */
    size_t depth;

    vitte_scope_span_t span;

    /*
     * Opaque caller-owned owner/declaration object.
     */
    void *payload;

    /*
     * Direct child scopes in deterministic creation order.
     */
    vitte_scope_id_t *children;
    size_t child_count;
    size_t child_capacity;

    /*
     * Directly declared symbols in deterministic declaration order.
     */
    vitte_symbol_id_t *symbols;
    size_t symbol_count;
    size_t symbol_capacity;

    /*
     * Private implementation table used by scope.c.
     *
     * Entries encode global symbol index + 1.
     * Zero means empty.
     *
     * Exposed structurally because vitte_scope_t is intentionally a
     * stack-allocatable public context, but callers must treat these fields
     * as read-only implementation state.
     */
    size_t *hash_slots;
    size_t hash_capacity;

    /*
     * Prevent additional declarations/children from being semantically
     * appended to this scope.
     */
    bool sealed;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_scope_stats {
    uint64_t scopes_created;
    uint64_t symbols_declared;

    uint64_t local_lookups;
    uint64_t recursive_lookups;

    uint64_t lookup_hits;
    uint64_t lookup_misses;

    uint64_t duplicate_declarations;
    uint64_t shadowing_declarations;
    uint64_t visibility_rejections;

    uint64_t validation_runs;
    uint64_t validation_failures;

    uint64_t hash_runs;

    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;

    uint64_t failures;

    /*
     * Maximum scope depth observed.
     */
    uint64_t max_depth;
} vitte_scope_stats_t;

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

struct vitte_scope {
    uint64_t magic;

    vitte_scope_state_t state;
    vitte_scope_error_t last_error;

    /*
     * Global stable scope table.
     */
    vitte_scope_entry_t *scopes;
    size_t scope_count;
    size_t scope_capacity;

    /*
     * Global stable symbol table.
     */
    vitte_symbol_t *symbols;
    size_t symbol_count;
    size_t symbol_capacity;

    /*
     * Unique lexical root.
     */
    vitte_scope_id_t root_scope;

    /*
     * Configurable resource bounds.
     */
    size_t max_scopes;
    size_t max_symbols;
    size_t max_name_bytes;
    size_t max_depth;

    vitte_scope_stats_t stats;

    /*
     * Reset generation.
     *
     * IDs must not be cached across reset unless the owner separately tracks
     * this generation.
     */
    uint64_t generation;
};

typedef vitte_scope_t vitte_scope_context_t;

/* ========================================================================= */
/* Human-readable names                                                      */
/* ========================================================================= */

const char *
vitte_scope_error_name(
    vitte_scope_error_t error);

const char *
vitte_scope_state_name(
    vitte_scope_state_t state);

const char *
vitte_scope_kind_name(
    vitte_scope_kind_t kind);

const char *
vitte_symbol_kind_name(
    vitte_symbol_kind_t kind);

const char *
vitte_symbol_visibility_name(
    vitte_symbol_visibility_t visibility);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize an empty scope context and create its unique root scope.
 */
bool
vitte_scope_init(
    vitte_scope_t *context);

/*
 * Remove all scopes/symbols while retaining the global scope/symbol arrays.
 *
 * Per-scope child/symbol/hash allocations and owned symbol names are released.
 * A fresh root scope is created.
 *
 * Existing IDs become invalid.
 */
bool
vitte_scope_reset(
    vitte_scope_t *context);

void
vitte_scope_destroy(
    vitte_scope_t *context);

/* ========================================================================= */
/* Resource limits                                                           */
/* ========================================================================= */

bool
vitte_scope_set_limits(
    vitte_scope_t *context,
    size_t max_scopes,
    size_t max_symbols,
    size_t max_name_bytes,
    size_t max_depth);

/* ========================================================================= */
/* Scope creation                                                            */
/* ========================================================================= */

/*
 * Create a child lexical scope.
 *
 * parent_id must identify an existing scope, except for the internal root
 * creation performed during initialization/reset.
 *
 * payload remains caller-owned.
 */
vitte_scope_id_t
vitte_scope_create(
    vitte_scope_t *context,
    vitte_scope_id_t parent_id,
    vitte_scope_kind_t kind,
    vitte_scope_span_t span,
    void *payload);

/* ========================================================================= */
/* Sealing                                                                   */
/* ========================================================================= */

bool
vitte_scope_seal_scope(
    vitte_scope_t *context,
    vitte_scope_id_t scope_id);

/*
 * Validate and freeze the complete context.
 */
bool
vitte_scope_seal(
    vitte_scope_t *context);

/* ========================================================================= */
/* Symbol declaration                                                        */
/* ========================================================================= */

/*
 * Declare one symbol in a lexical scope.
 *
 * Duplicate names in the same scope are rejected.
 * A declaration with the same name in an ancestor scope is accepted and
 * recorded as lexical shadowing.
 *
 * name is copied and owned by the scope context.
 * payload remains caller-owned.
 */
vitte_symbol_id_t
vitte_scope_declare(
    vitte_scope_t *context,
    vitte_scope_id_t scope_id,
    const char *name,
    size_t name_length,
    vitte_symbol_kind_t kind,
    vitte_symbol_visibility_t visibility,
    uint32_t flags,
    vitte_scope_span_t span,
    void *payload);

/* ========================================================================= */
/* Local lookup                                                              */
/* ========================================================================= */

vitte_symbol_id_t
vitte_scope_lookup_local_n(
    vitte_scope_t *context,
    vitte_scope_id_t scope_id,
    const char *name,
    size_t name_length);

vitte_symbol_id_t
vitte_scope_lookup_local(
    vitte_scope_t *context,
    vitte_scope_id_t scope_id,
    const char *name);

/* ========================================================================= */
/* Lexical lookup                                                            */
/* ========================================================================= */

/*
 * Search start_scope first, then each lexical parent until the root.
 */
vitte_symbol_id_t
vitte_scope_lookup_n(
    vitte_scope_t *context,
    vitte_scope_id_t start_scope,
    const char *name,
    size_t name_length);

vitte_symbol_id_t
vitte_scope_lookup(
    vitte_scope_t *context,
    vitte_scope_id_t start_scope,
    const char *name);

/* ========================================================================= */
/* Visibility-aware lookup                                                   */
/* ========================================================================= */

/*
 * Perform lexical lookup from start_scope and then apply declaration
 * visibility relative to requester_scope.
 *
 * Public symbols are globally visible once found through the requested
 * lexical chain.
 *
 * Private symbols are visible from their declaration scope and descendants.
 */
vitte_symbol_id_t
vitte_scope_lookup_visible_n(
    vitte_scope_t *context,
    vitte_scope_id_t start_scope,
    vitte_scope_id_t requester_scope,
    const char *name,
    size_t name_length);

vitte_symbol_id_t
vitte_scope_lookup_visible(
    vitte_scope_t *context,
    vitte_scope_id_t start_scope,
    vitte_scope_id_t requester_scope,
    const char *name);

/* ========================================================================= */
/* Object access                                                             */
/* ========================================================================= */

const vitte_scope_entry_t *
vitte_scope_get_scope(
    const vitte_scope_t *context,
    vitte_scope_id_t id);

/*
 * Mutable access is available only while BUILDING.
 *
 * Direct mutation can violate invariants; prefer dedicated API functions.
 */
vitte_scope_entry_t *
vitte_scope_get_scope_mut(
    vitte_scope_t *context,
    vitte_scope_id_t id);

const vitte_symbol_t *
vitte_scope_get_symbol(
    const vitte_scope_t *context,
    vitte_symbol_id_t id);

vitte_symbol_t *
vitte_scope_get_symbol_mut(
    vitte_scope_t *context,
    vitte_symbol_id_t id);

/* ========================================================================= */
/* Hierarchy queries                                                         */
/* ========================================================================= */

/*
 * A scope is considered its own ancestor.
 */
bool
vitte_scope_is_ancestor(
    const vitte_scope_t *context,
    vitte_scope_id_t ancestor,
    vitte_scope_id_t descendant);

/*
 * Return the nearest common lexical ancestor.
 */
vitte_scope_id_t
vitte_scope_common_ancestor(
    const vitte_scope_t *context,
    vitte_scope_id_t left,
    vitte_scope_id_t right);

/* ========================================================================= */
/* Symbol queries                                                            */
/* ========================================================================= */

bool
vitte_scope_symbol_shadows(
    const vitte_scope_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_symbol_id_t *shadowed_symbol);

bool
vitte_scope_symbol_is_visible_from(
    const vitte_scope_t *context,
    vitte_symbol_id_t symbol_id,
    vitte_scope_id_t requester_scope);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate:
 *   - global context invariants;
 *   - unique root;
 *   - stable IDs;
 *   - parent/child consistency;
 *   - scope depths;
 *   - hierarchy acyclicity;
 *   - symbol ownership;
 *   - local symbol uniqueness/indexing;
 *   - name hashes;
 *   - shadow links;
 *   - resource bounds.
 */
bool
vitte_scope_validate(
    vitte_scope_t *context);

/* ========================================================================= */
/* Fingerprints                                                              */
/* ========================================================================= */

/*
 * Deterministic structural fingerprint.
 *
 * Includes:
 *   - stable IDs;
 *   - hierarchy;
 *   - scope kinds;
 *   - declaration order;
 *   - symbol names;
 *   - symbol kinds;
 *   - visibility;
 *   - flags;
 *   - shadow links.
 *
 * Excludes:
 *   - pointer addresses;
 *   - allocation capacities;
 *   - runtime statistics.
 */
uint64_t
vitte_scope_fingerprint(
    vitte_scope_t *context);

/*
 * Semantic fingerprint.
 *
 * Excludes:
 *   - source locations;
 *   - payload addresses;
 *   - capacities;
 *   - statistics;
 *   - generation.
 */
uint64_t
vitte_scope_semantic_fingerprint(
    vitte_scope_t *context);

/* ========================================================================= */
/* Root / counts                                                             */
/* ========================================================================= */

vitte_scope_id_t
vitte_scope_root(
    const vitte_scope_t *context);

size_t
vitte_scope_count(
    const vitte_scope_t *context);

size_t
vitte_scope_symbol_count(
    const vitte_scope_t *context);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_scope_stats_t
vitte_scope_stats(
    const vitte_scope_t *context);

/* ========================================================================= */
/* Context queries                                                           */
/* ========================================================================= */

bool
vitte_scope_is_valid(
    const vitte_scope_t *context);

vitte_scope_error_t
vitte_scope_last_error(
    const vitte_scope_t *context);

uint64_t
vitte_scope_generation(
    const vitte_scope_t *context);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_scope_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_scope_error_is_valid(
    vitte_scope_error_t error)
{
    return error >=
               VITTE_SCOPE_ERROR_NONE &&
           error <
               VITTE_SCOPE_ERROR_COUNT;
}

static inline bool
vitte_scope_state_is_valid(
    vitte_scope_state_t state)
{
    return state >
               VITTE_SCOPE_STATE_INVALID &&
           state <
               VITTE_SCOPE_STATE_COUNT;
}

static inline bool
vitte_scope_kind_is_valid(
    vitte_scope_kind_t kind)
{
    return kind >
               VITTE_SCOPE_KIND_INVALID &&
           kind <
               VITTE_SCOPE_KIND_COUNT;
}

static inline bool
vitte_symbol_kind_is_valid(
    vitte_symbol_kind_t kind)
{
    return kind >
               VITTE_SYMBOL_KIND_INVALID &&
           kind <
               VITTE_SYMBOL_KIND_COUNT;
}

static inline bool
vitte_symbol_visibility_is_valid(
    vitte_symbol_visibility_t visibility)
{
    return visibility >=
               VITTE_SYMBOL_VISIBILITY_PRIVATE &&
           visibility <
               VITTE_SYMBOL_VISIBILITY_COUNT;
}

/* ========================================================================= */
/* Inline ID helpers                                                         */
/* ========================================================================= */

static inline bool
vitte_scope_id_is_valid(
    vitte_scope_id_t id)
{
    return id !=
           VITTE_SCOPE_INVALID_SCOPE_ID;
}

static inline bool
vitte_symbol_id_is_valid(
    vitte_symbol_id_t id)
{
    return id !=
           VITTE_SCOPE_INVALID_SYMBOL_ID;
}

/* ========================================================================= */
/* Inline span helpers                                                       */
/* ========================================================================= */

static inline vitte_scope_span_t
vitte_scope_span_invalid(void)
{
    vitte_scope_span_t span;

    span.file_id = UINT32_C(0);
    span.begin = 0u;
    span.end = 0u;
    span.line = 0u;
    span.column = 0u;
    span.valid = false;

    return span;
}

static inline bool
vitte_scope_span_is_valid(
    vitte_scope_span_t span)
{
    return span.valid &&
           span.begin <= span.end;
}

static inline size_t
vitte_scope_span_length(
    vitte_scope_span_t span)
{
    if (!vitte_scope_span_is_valid(span)) {
        return 0u;
    }

    return span.end - span.begin;
}

static inline bool
vitte_scope_span_is_empty(
    vitte_scope_span_t span)
{
    return !vitte_scope_span_is_valid(span) ||
           span.begin == span.end;
}

static inline bool
vitte_scope_span_contains_offset(
    vitte_scope_span_t span,
    size_t offset)
{
    return vitte_scope_span_is_valid(span) &&
           offset >= span.begin &&
           offset < span.end;
}

static inline bool
vitte_scope_span_contains_span(
    vitte_scope_span_t outer,
    vitte_scope_span_t inner)
{
    return vitte_scope_span_is_valid(outer) &&
           vitte_scope_span_is_valid(inner) &&
           outer.file_id == inner.file_id &&
           inner.begin >= outer.begin &&
           inner.end <= outer.end;
}

static inline bool
vitte_scope_spans_overlap(
    vitte_scope_span_t left,
    vitte_scope_span_t right)
{
    return vitte_scope_span_is_valid(left) &&
           vitte_scope_span_is_valid(right) &&
           left.file_id == right.file_id &&
           left.begin < right.end &&
           right.begin < left.end;
}

/* ========================================================================= */
/* Inline context-state helpers                                              */
/* ========================================================================= */

static inline bool
vitte_scope_is_building(
    const vitte_scope_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_SCOPE_MAGIC &&
           context->state ==
               VITTE_SCOPE_STATE_BUILDING;
}

static inline bool
vitte_scope_is_sealed(
    const vitte_scope_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_SCOPE_MAGIC &&
           context->state ==
               VITTE_SCOPE_STATE_SEALED;
}

static inline bool
vitte_scope_has_failed(
    const vitte_scope_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_SCOPE_MAGIC &&
           context->state ==
               VITTE_SCOPE_STATE_FAILED;
}

static inline bool
vitte_scope_is_destroyed(
    const vitte_scope_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_SCOPE_DEAD_MAGIC &&
           context->state ==
               VITTE_SCOPE_STATE_DESTROYED;
}

/* ========================================================================= */
/* Inline scope-entry helpers                                                */
/* ========================================================================= */

static inline bool
vitte_scope_entry_is_root(
    const vitte_scope_entry_t *scope)
{
    return scope != NULL &&
           scope->parent_id ==
               VITTE_SCOPE_INVALID_SCOPE_ID &&
           scope->kind ==
               VITTE_SCOPE_KIND_ROOT;
}

static inline bool
vitte_scope_entry_is_sealed(
    const vitte_scope_entry_t *scope)
{
    return scope != NULL &&
           scope->sealed;
}

static inline size_t
vitte_scope_entry_depth(
    const vitte_scope_entry_t *scope)
{
    return scope != NULL
               ? scope->depth
               : 0u;
}

static inline size_t
vitte_scope_entry_child_count(
    const vitte_scope_entry_t *scope)
{
    return scope != NULL
               ? scope->child_count
               : 0u;
}

static inline size_t
vitte_scope_entry_symbol_count(
    const vitte_scope_entry_t *scope)
{
    return scope != NULL
               ? scope->symbol_count
               : 0u;
}

static inline vitte_scope_id_t
vitte_scope_entry_child_at(
    const vitte_scope_entry_t *scope,
    size_t index)
{
    if (scope == NULL ||
        index >= scope->child_count ||
        scope->children == NULL) {
        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    return scope->children[index];
}

static inline vitte_symbol_id_t
vitte_scope_entry_symbol_at(
    const vitte_scope_entry_t *scope,
    size_t index)
{
    if (scope == NULL ||
        index >= scope->symbol_count ||
        scope->symbols == NULL) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    return scope->symbols[index];
}

/* ========================================================================= */
/* Inline symbol helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_symbol_is_public(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->visibility ==
               VITTE_SYMBOL_VISIBILITY_PUBLIC;
}

static inline bool
vitte_symbol_is_private(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->visibility ==
               VITTE_SYMBOL_VISIBILITY_PRIVATE;
}

static inline bool
vitte_symbol_has_flag(
    const vitte_symbol_t *symbol,
    uint32_t flag)
{
    return symbol != NULL &&
           (symbol->flags & flag) !=
               UINT32_C(0);
}

static inline void
vitte_symbol_add_flag(
    vitte_symbol_t *symbol,
    uint32_t flag)
{
    if (symbol != NULL) {
        symbol->flags |= flag;
    }
}

static inline void
vitte_symbol_remove_flag(
    vitte_symbol_t *symbol,
    uint32_t flag)
{
    if (symbol != NULL) {
        symbol->flags &= ~flag;
    }
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
vitte_symbol_is_imported(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           (symbol->kind ==
                VITTE_SYMBOL_KIND_IMPORT ||
            vitte_symbol_has_flag(
                symbol,
                VITTE_SYMBOL_FLAG_IMPORTED));
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
vitte_symbol_is_used(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_USED);
}

static inline bool
vitte_symbol_is_captured(
    const vitte_symbol_t *symbol)
{
    return vitte_symbol_has_flag(
        symbol,
        VITTE_SYMBOL_FLAG_CAPTURED);
}

static inline bool
vitte_symbol_has_shadowed_declaration(
    const vitte_symbol_t *symbol)
{
    return symbol != NULL &&
           symbol->shadowed_symbol !=
               VITTE_SCOPE_INVALID_SYMBOL_ID;
}

/* ========================================================================= */
/* Inline symbol-category helpers                                            */
/* ========================================================================= */

static inline bool
vitte_symbol_kind_is_type(
    vitte_symbol_kind_t kind)
{
    switch (kind) {
        case VITTE_SYMBOL_KIND_TYPE:
        case VITTE_SYMBOL_KIND_OPAQUE:
        case VITTE_SYMBOL_KIND_FORM:
        case VITTE_SYMBOL_KIND_PICK:
        case VITTE_SYMBOL_KIND_TRAIT:
            return true;

        case VITTE_SYMBOL_KIND_INVALID:
        case VITTE_SYMBOL_KIND_SPACE:
        case VITTE_SYMBOL_KIND_CONST:
        case VITTE_SYMBOL_KIND_STATIC:
        case VITTE_SYMBOL_KIND_FIELD:
        case VITTE_SYMBOL_KIND_VARIANT:
        case VITTE_SYMBOL_KIND_IMPL:
        case VITTE_SYMBOL_KIND_PROC:
        case VITTE_SYMBOL_KIND_PARAMETER:
        case VITTE_SYMBOL_KIND_GENERIC_PARAMETER:
        case VITTE_SYMBOL_KIND_LOCAL:
        case VITTE_SYMBOL_KIND_MACRO:
        case VITTE_SYMBOL_KIND_TEST:
        case VITTE_SYMBOL_KIND_IMPORT:
        case VITTE_SYMBOL_KIND_COUNT:
            return false;
    }

    return false;
}

static inline bool
vitte_symbol_kind_is_value(
    vitte_symbol_kind_t kind)
{
    switch (kind) {
        case VITTE_SYMBOL_KIND_CONST:
        case VITTE_SYMBOL_KIND_STATIC:
        case VITTE_SYMBOL_KIND_FIELD:
        case VITTE_SYMBOL_KIND_VARIANT:
        case VITTE_SYMBOL_KIND_PROC:
        case VITTE_SYMBOL_KIND_PARAMETER:
        case VITTE_SYMBOL_KIND_LOCAL:
            return true;

        case VITTE_SYMBOL_KIND_INVALID:
        case VITTE_SYMBOL_KIND_SPACE:
        case VITTE_SYMBOL_KIND_TYPE:
        case VITTE_SYMBOL_KIND_OPAQUE:
        case VITTE_SYMBOL_KIND_FORM:
        case VITTE_SYMBOL_KIND_PICK:
        case VITTE_SYMBOL_KIND_TRAIT:
        case VITTE_SYMBOL_KIND_IMPL:
        case VITTE_SYMBOL_KIND_GENERIC_PARAMETER:
        case VITTE_SYMBOL_KIND_MACRO:
        case VITTE_SYMBOL_KIND_TEST:
        case VITTE_SYMBOL_KIND_IMPORT:
        case VITTE_SYMBOL_KIND_COUNT:
            return false;
    }

    return false;
}

static inline bool
vitte_symbol_kind_is_callable(
    vitte_symbol_kind_t kind)
{
    return kind ==
               VITTE_SYMBOL_KIND_PROC ||
           kind ==
               VITTE_SYMBOL_KIND_MACRO;
}

static inline bool
vitte_symbol_kind_is_local_binding(
    vitte_symbol_kind_t kind)
{
    return kind ==
               VITTE_SYMBOL_KIND_PARAMETER ||
           kind ==
               VITTE_SYMBOL_KIND_LOCAL ||
           kind ==
               VITTE_SYMBOL_KIND_GENERIC_PARAMETER;
}

/* ========================================================================= */
/* Inline version helpers                                                    */
/* ========================================================================= */

static inline unsigned int
vitte_scope_api_version_major(void)
{
    return VITTE_SCOPE_API_VERSION_MAJOR;
}

static inline unsigned int
vitte_scope_api_version_minor(void)
{
    return VITTE_SCOPE_API_VERSION_MINOR;
}

static inline unsigned int
vitte_scope_api_version_patch(void)
{
    return VITTE_SCOPE_API_VERSION_PATCH;
}

/* ========================================================================= */
/* Compile-time contract checks                                              */
/* ========================================================================= */

#if defined(__cplusplus)

static_assert(
    sizeof(uint64_t) == 8u,
    "scope requires 64-bit uint64_t");

static_assert(
    sizeof(uint32_t) == 4u,
    "scope requires 32-bit uint32_t");

static_assert(
    sizeof(vitte_scope_id_t) == 8u,
    "scope IDs must remain 64-bit");

static_assert(
    sizeof(vitte_symbol_id_t) == 8u,
    "symbol IDs must remain 64-bit");

static_assert(
    VITTE_SCOPE_INVALID_SCOPE_ID == UINT64_C(0),
    "scope ID zero must remain invalid");

static_assert(
    VITTE_SCOPE_INVALID_SYMBOL_ID == UINT64_C(0),
    "symbol ID zero must remain invalid");

static_assert(
    VITTE_SCOPE_DEFAULT_INITIAL_SCOPE_CAPACITY > 0u,
    "initial scope capacity must be non-zero");

static_assert(
    VITTE_SCOPE_DEFAULT_INITIAL_SYMBOL_CAPACITY > 0u,
    "initial symbol capacity must be non-zero");

static_assert(
    VITTE_SCOPE_DEFAULT_MAX_SCOPES > 0u,
    "maximum scope count must be non-zero");

static_assert(
    VITTE_SCOPE_DEFAULT_MAX_SYMBOLS > 0u,
    "maximum symbol count must be non-zero");

static_assert(
    VITTE_SCOPE_DEFAULT_MAX_DEPTH > 0u,
    "maximum scope depth must be non-zero");

#else

_Static_assert(
    sizeof(uint64_t) == 8u,
    "scope requires 64-bit uint64_t");

_Static_assert(
    sizeof(uint32_t) == 4u,
    "scope requires 32-bit uint32_t");

_Static_assert(
    sizeof(vitte_scope_id_t) == 8u,
    "scope IDs must remain 64-bit");

_Static_assert(
    sizeof(vitte_symbol_id_t) == 8u,
    "symbol IDs must remain 64-bit");

_Static_assert(
    VITTE_SCOPE_INVALID_SCOPE_ID == UINT64_C(0),
    "scope ID zero must remain invalid");

_Static_assert(
    VITTE_SCOPE_INVALID_SYMBOL_ID == UINT64_C(0),
    "symbol ID zero must remain invalid");

_Static_assert(
    VITTE_SCOPE_DEFAULT_INITIAL_SCOPE_CAPACITY > 0u,
    "initial scope capacity must be non-zero");

_Static_assert(
    VITTE_SCOPE_DEFAULT_INITIAL_SYMBOL_CAPACITY > 0u,
    "initial symbol capacity must be non-zero");

_Static_assert(
    VITTE_SCOPE_DEFAULT_MAX_SCOPES > 0u,
    "maximum scope count must be non-zero");

_Static_assert(
    VITTE_SCOPE_DEFAULT_MAX_SYMBOLS > 0u,
    "maximum symbol count must be non-zero");

_Static_assert(
    VITTE_SCOPE_DEFAULT_MAX_DEPTH > 0u,
    "maximum scope depth must be non-zero");

#endif

#ifdef __cplusplus
}
#endif

#endif /* VITTE_SCOPE_SCOPE_H */
