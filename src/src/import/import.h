#ifndef VITTE_IMPORT_IMPORT_H
#define VITTE_IMPORT_IMPORT_H

/*
 * Vitte Compiler
 * src/import/import.h
 *
 * Import and Module Dependency System
 * ===================================
 *
 * Public API for Vitte module/import resolution.
 *
 * Responsibilities:
 *
 *   - module registration;
 *   - import registration;
 *   - canonical module identity;
 *   - source/module association;
 *   - interned module names and paths;
 *   - dependency graph construction;
 *   - reverse dependency tracking;
 *   - forward import resolution;
 *   - duplicate detection;
 *   - self-import rejection;
 *   - cycle detection;
 *   - deterministic topological ordering;
 *   - module visibility;
 *   - import aliases;
 *   - source provenance;
 *   - validation;
 *   - deterministic fingerprints;
 *   - resource limits;
 *   - statistics.
 *
 * Architecture:
 *
 *     source files / packages
 *              |
 *              v
 *        filesystem/source
 *              |
 *              v
 *     +-------------------+
 *     |   import system   |
 *     +-------------------+
 *       |       |       |
 *       |       |       +---- dependency graph
 *       |       +------------ module identities
 *       +-------------------- resolved imports
 *              |
 *              v
 *       name resolution
 *              |
 *              v
 *         semantic/HIR
 *
 * This subsystem deliberately does not perform filesystem access.
 *
 * The filesystem/package/source layers discover physical files and provide
 * canonical logical module names. The import subsystem then constructs and
 * validates the logical dependency graph.
 *
 * -------------------------------------------------------------------------
 * IDs
 * -------------------------------------------------------------------------
 *
 * Every public ID namespace is 1-based.
 *
 *     0 == invalid / absent
 *
 * IDs remain stable until vitte_import_reset() or vitte_import_destroy().
 *
 * -------------------------------------------------------------------------
 * Ownership
 * -------------------------------------------------------------------------
 *
 * vitte_import_context_t owns:
 *
 *   - registered modules;
 *   - import edges;
 *   - interned strings;
 *   - dependency lists;
 *   - reverse dependency lists;
 *   - per-module import lists;
 *   - topological ordering.
 *
 * Interned strings are copied.
 *
 * -------------------------------------------------------------------------
 * State machine
 * -------------------------------------------------------------------------
 *
 *     init()
 *       |
 *       v
 *    BUILDING
 *       |
 *       | vitte_import_resolve()
 *       v
 *   RESOLVING
 *       |
 *       +--------------+
 *       |              |
 *       v              v
 *   RESOLVED         FAILED
 *
 * reset() returns a live context to BUILDING.
 *
 * destroy() moves the context to DESTROYED.
 *
 * -------------------------------------------------------------------------
 * Determinism
 * -------------------------------------------------------------------------
 *
 * Registration order is preserved.
 *
 * DFS traversal uses module insertion order and dependency insertion order.
 *
 * The resulting dependency-first topological order is deterministic for an
 * identical input graph.
 *
 * -------------------------------------------------------------------------
 * Threading
 * -------------------------------------------------------------------------
 *
 * A single context is not internally synchronized.
 *
 * Independent contexts may be used concurrently.
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

#define VITTE_IMPORT_API_VERSION_MAJOR 1u
#define VITTE_IMPORT_API_VERSION_MINOR 0u
#define VITTE_IMPORT_API_VERSION_PATCH 0u

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_IMPORT_MAGIC \
    UINT64_C(0x5649545445494d50)

#define VITTE_IMPORT_DEAD_MAGIC \
    UINT64_C(0x44454144494d5021)

#define VITTE_IMPORT_INVALID_ID \
    UINT64_C(0)

#define VITTE_IMPORT_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_IMPORT_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_IMPORT_DEFAULT_INITIAL_CAPACITY \
    ((size_t)16u)

#define VITTE_IMPORT_DEFAULT_MAX_MODULES \
    ((size_t)1048576u)

#define VITTE_IMPORT_DEFAULT_MAX_IMPORTS \
    ((size_t)8388608u)

#define VITTE_IMPORT_DEFAULT_MAX_STRING_BYTES \
    ((size_t)(256u * 1024u * 1024u))

/* ========================================================================= */
/* IDs                                                                       */
/* ========================================================================= */

typedef uint64_t vitte_import_id_t;

typedef uint64_t vitte_import_string_id_t;
typedef uint64_t vitte_import_module_id_t;
typedef uint64_t vitte_import_edge_id_t;

/*
 * Source IDs are intentionally opaque to this subsystem.
 *
 * They normally originate from Vitte's source manager.
 *
 * Zero may represent "no source".
 */
typedef uint64_t vitte_import_source_id_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_import_error {
    VITTE_IMPORT_ERROR_NONE = 0,

    VITTE_IMPORT_ERROR_INVALID_ARGUMENT,
    VITTE_IMPORT_ERROR_INVALID_CONTEXT,
    VITTE_IMPORT_ERROR_INVALID_STATE,

    VITTE_IMPORT_ERROR_INVALID_STRING,
    VITTE_IMPORT_ERROR_INVALID_MODULE,
    VITTE_IMPORT_ERROR_INVALID_EDGE,

    VITTE_IMPORT_ERROR_DUPLICATE_MODULE,
    VITTE_IMPORT_ERROR_DUPLICATE_IMPORT,

    VITTE_IMPORT_ERROR_MODULE_NOT_FOUND,
    VITTE_IMPORT_ERROR_IMPORT_NOT_FOUND,

    VITTE_IMPORT_ERROR_SELF_IMPORT,
    VITTE_IMPORT_ERROR_CYCLE,

    VITTE_IMPORT_ERROR_TOO_MANY_MODULES,
    VITTE_IMPORT_ERROR_TOO_MANY_IMPORTS,
    VITTE_IMPORT_ERROR_STRING_LIMIT,

    VITTE_IMPORT_ERROR_OVERFLOW,
    VITTE_IMPORT_ERROR_OUT_OF_MEMORY,

    VITTE_IMPORT_ERROR_VALIDATION,
    VITTE_IMPORT_ERROR_CORRUPTION,

    VITTE_IMPORT_ERROR_UNSUPPORTED,
    VITTE_IMPORT_ERROR_INTERNAL,

    VITTE_IMPORT_ERROR_COUNT
} vitte_import_error_t;

/* ========================================================================= */
/* Global context state                                                      */
/* ========================================================================= */

typedef enum vitte_import_state {
    VITTE_IMPORT_STATE_INVALID = 0,

    /*
     * Modules and imports may be registered.
     */
    VITTE_IMPORT_STATE_BUILDING,

    /*
     * Dependency resolution is currently running.
     */
    VITTE_IMPORT_STATE_RESOLVING,

    /*
     * All imports were resolved and a valid dependency ordering exists.
     */
    VITTE_IMPORT_STATE_RESOLVED,

    /*
     * The previous resolution operation failed.
     *
     * vitte_import_resolve() may be attempted again after the caller has
     * corrected externally-managed information where applicable.
     *
     * reset() always provides a clean recovery path.
     */
    VITTE_IMPORT_STATE_FAILED,

    /*
     * Owned storage has been released.
     */
    VITTE_IMPORT_STATE_DESTROYED,

    VITTE_IMPORT_STATE_COUNT
} vitte_import_state_t;

/* ========================================================================= */
/* Module state                                                              */
/* ========================================================================= */

typedef enum vitte_import_module_state {
    VITTE_IMPORT_MODULE_INVALID = 0,

    VITTE_IMPORT_MODULE_REGISTERED,

    /*
     * DFS traversal currently has this module on its active path.
     */
    VITTE_IMPORT_MODULE_RESOLVING,

    VITTE_IMPORT_MODULE_RESOLVED,

    VITTE_IMPORT_MODULE_FAILED,

    VITTE_IMPORT_MODULE_COUNT
} vitte_import_module_state_t;

/* ========================================================================= */
/* Import-edge state                                                         */
/* ========================================================================= */

typedef enum vitte_import_edge_state {
    VITTE_IMPORT_EDGE_INVALID = 0,

    /*
     * Import target has not yet been bound to a module ID.
     */
    VITTE_IMPORT_EDGE_UNRESOLVED,

    VITTE_IMPORT_EDGE_RESOLVED,

    VITTE_IMPORT_EDGE_FAILED,

    VITTE_IMPORT_EDGE_COUNT
} vitte_import_edge_state_t;

/* ========================================================================= */
/* Visibility                                                                */
/* ========================================================================= */

typedef enum vitte_import_visibility {
    VITTE_IMPORT_VISIBILITY_PRIVATE = 0,

    VITTE_IMPORT_VISIBILITY_PUBLIC,

    VITTE_IMPORT_VISIBILITY_COUNT
} vitte_import_visibility_t;

/* ========================================================================= */
/* Source span                                                               */
/* ========================================================================= */

/*
 * Half-open byte range:
 *
 *     [begin, end)
 *
 * file_id belongs to the compiler source manager.
 */
typedef struct vitte_import_span {
    uint32_t file_id;

    size_t begin;
    size_t end;

    bool valid;
} vitte_import_span_t;

/* ========================================================================= */
/* Interned string                                                           */
/* ========================================================================= */

typedef struct vitte_import_string {
    vitte_import_string_id_t id;

    /*
     * Owned NUL-terminated storage.
     *
     * length excludes the trailing terminator.
     *
     * The API is length-aware; embedded NUL bytes are representable.
     */
    char *data;

    size_t length;

    /*
     * FNV-1a hash of exactly length bytes.
     */
    uint64_t hash;
} vitte_import_string_t;

/* ========================================================================= */
/* Module descriptor                                                         */
/* ========================================================================= */

/*
 * Input structure for vitte_import_add_module().
 *
 * name:
 *     required interned logical module name.
 *
 * path:
 *     optional interned canonical physical/logical path.
 *
 * source_id:
 *     opaque source-manager identifier.
 *
 * is_root:
 *     true when this is a compilation root.
 *
 * is_external:
 *     true for external/package/stdlib modules if the frontend wants to
 *     preserve that distinction.
 */
typedef struct vitte_import_module_desc {
    vitte_import_string_id_t name;
    vitte_import_string_id_t path;

    vitte_import_source_id_t source_id;

    vitte_import_visibility_t visibility;

    bool is_root;
    bool is_external;
} vitte_import_module_desc_t;

/* ========================================================================= */
/* Import descriptor                                                         */
/* ========================================================================= */

/*
 * Input structure for vitte_import_add().
 *
 * source_module:
 *     module containing the import declaration.
 *
 * module_name:
 *     interned logical target module name.
 *
 * alias:
 *     optional interned alias.
 *
 * visibility:
 *     whether the import is private or publicly re-exported.
 *
 * span:
 *     source location of the import declaration.
 */
typedef struct vitte_import_desc {
    vitte_import_module_id_t source_module;

    vitte_import_string_id_t module_name;
    vitte_import_string_id_t alias;

    vitte_import_visibility_t visibility;

    vitte_import_span_t span;
} vitte_import_desc_t;

/* ========================================================================= */
/* Import edge                                                               */
/* ========================================================================= */

typedef struct vitte_import_edge {
    vitte_import_edge_id_t id;

    /*
     * Module containing the import.
     */
    vitte_import_module_id_t source_module;

    /*
     * Resolved target.
     *
     * Zero while unresolved.
     */
    vitte_import_module_id_t target_module;

    /*
     * Original logical target name.
     */
    vitte_import_string_id_t module_name;

    /*
     * Optional local alias.
     */
    vitte_import_string_id_t alias;

    vitte_import_visibility_t visibility;

    vitte_import_span_t span;

    vitte_import_edge_state_t state;
} vitte_import_edge_t;

/* ========================================================================= */
/* Module                                                                    */
/* ========================================================================= */

typedef struct vitte_import_module {
    vitte_import_module_id_t id;

    /*
     * Logical module identity.
     */
    vitte_import_string_id_t name;

    /*
     * Optional canonical path.
     */
    vitte_import_string_id_t path;

    /*
     * Opaque source-manager ID.
     */
    vitte_import_source_id_t source_id;

    vitte_import_visibility_t visibility;
    vitte_import_module_state_t state;

    bool is_root;
    bool is_external;

    /*
     * Direct dependency module IDs.
     *
     * Duplicate module IDs are suppressed.
     *
     * Ordering follows first dependency occurrence.
     */
    vitte_import_module_id_t *dependencies;

    size_t dependency_count;
    size_t dependency_capacity;

    /*
     * Reverse dependency graph.
     */
    vitte_import_module_id_t *dependents;

    size_t dependent_count;
    size_t dependent_capacity;

    /*
     * Import declarations originating in this module.
     */
    vitte_import_edge_id_t *imports;

    size_t import_count;
    size_t import_capacity;
} vitte_import_module_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_import_stats {
    uint64_t modules_registered;
    uint64_t imports_registered;

    uint64_t strings_interned;
    uint64_t string_bytes;

    uint64_t imports_resolved;
    uint64_t unresolved_imports;

    uint64_t cycles_detected;

    uint64_t resolution_runs;
    uint64_t resolution_failures;

    uint64_t validation_runs;
    uint64_t validation_failures;

    uint64_t hash_runs;

    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;
} vitte_import_stats_t;

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

/*
 * Public API-v1 representation.
 *
 * Compiler passes should normally use the accessor functions rather than
 * depending directly on these fields. Keeping the structure visible in API v1
 * simplifies stack allocation and low-level compiler debugging.
 */
typedef struct vitte_import_context {
    uint64_t magic;

    vitte_import_state_t state;
    vitte_import_error_t last_error;

    /* --------------------------------------------------------------------- */
    /* Modules                                                               */
    /* --------------------------------------------------------------------- */

    vitte_import_module_t *modules;

    size_t module_count;
    size_t module_capacity;

    /* --------------------------------------------------------------------- */
    /* Import edges                                                          */
    /* --------------------------------------------------------------------- */

    vitte_import_edge_t *edges;

    size_t edge_count;
    size_t edge_capacity;

    /* --------------------------------------------------------------------- */
    /* Interned strings                                                      */
    /* --------------------------------------------------------------------- */

    vitte_import_string_t *strings;

    size_t string_count;
    size_t string_capacity;

    size_t string_bytes;

    /* --------------------------------------------------------------------- */
    /* Dependency ordering                                                   */
    /* --------------------------------------------------------------------- */

    /*
     * Dependency-first topological ordering.
     *
     * If A imports B:
     *
     *     B appears before A.
     */
    vitte_import_module_id_t *topological_order;

    size_t topological_count;
    size_t topological_capacity;

    /* --------------------------------------------------------------------- */
    /* Limits                                                                */
    /* --------------------------------------------------------------------- */

    size_t max_modules;
    size_t max_imports;
    size_t max_string_bytes;

    /* --------------------------------------------------------------------- */
    /* Statistics                                                            */
    /* --------------------------------------------------------------------- */

    vitte_import_stats_t stats;

    /*
     * Incremented by reset().
     *
     * External caches can use this to detect stale IDs.
     */
    uint64_t generation;
} vitte_import_context_t;

/* ========================================================================= */
/* Name functions                                                            */
/* ========================================================================= */

/*
 * Returned strings have static storage duration.
 */

const char *
vitte_import_error_name(
    vitte_import_error_t error);

const char *
vitte_import_state_name(
    vitte_import_state_t state);

const char *
vitte_import_module_state_name(
    vitte_import_module_state_t state);

const char *
vitte_import_edge_state_name(
    vitte_import_edge_state_t state);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * Initialize an empty import context.
 *
 * Initial state:
 *
 *     VITTE_IMPORT_STATE_BUILDING
 *
 * The structure does not need to be zero-initialized beforehand.
 */
bool
vitte_import_init(
    vitte_import_context_t *context);

/*
 * Release all storage.
 *
 * Safe with NULL.
 *
 * After destruction the object must be initialized again before reuse.
 */
void
vitte_import_destroy(
    vitte_import_context_t *context);

/*
 * Clear all modules/imports/strings and begin a fresh generation.
 *
 * Existing IDs become stale.
 *
 * User-configured limits are preserved.
 */
bool
vitte_import_reset(
    vitte_import_context_t *context);

/*
 * Fast context-envelope validation.
 *
 * This checks the basic structural invariants of the context itself.
 *
 * Use vitte_import_validate() for complete validation.
 */
bool
vitte_import_is_valid(
    const vitte_import_context_t *context);

/* ========================================================================= */
/* String interning                                                          */
/* ========================================================================= */

/*
 * Intern exactly length bytes.
 *
 * data may be NULL only when length == 0.
 *
 * Equal byte sequences produce the same ID.
 *
 * The context copies the bytes and owns the resulting storage.
 *
 * Returns VITTE_IMPORT_INVALID_ID on failure.
 */
vitte_import_string_id_t
vitte_import_intern_string(
    vitte_import_context_t *context,
    const char *data,
    size_t length);

/*
 * Return an interned string or NULL for an invalid ID/context.
 */
const vitte_import_string_t *
vitte_import_get_string(
    const vitte_import_context_t *context,
    vitte_import_string_id_t id);

/* ========================================================================= */
/* Module registration                                                       */
/* ========================================================================= */

/*
 * Register a module.
 *
 * Requirements:
 *
 *   - context is BUILDING;
 *   - description != NULL;
 *   - name references an interned string;
 *   - optional path references an interned string;
 *   - module name is unique;
 *   - non-zero module path is unique.
 *
 * Returns zero on failure.
 */
vitte_import_module_id_t
vitte_import_add_module(
    vitte_import_context_t *context,
    const vitte_import_module_desc_t *description);

/*
 * Immutable module lookup.
 */
const vitte_import_module_t *
vitte_import_get_module(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id);

/*
 * Mutable module lookup.
 *
 * Only available while BUILDING.
 */
vitte_import_module_t *
vitte_import_get_module_mut(
    vitte_import_context_t *context,
    vitte_import_module_id_t id);

/* ========================================================================= */
/* Module lookup                                                             */
/* ========================================================================= */

/*
 * Lookup using an interned logical module name.
 *
 * Returns zero when not found.
 */
vitte_import_module_id_t
vitte_import_find_module_by_name(
    const vitte_import_context_t *context,
    vitte_import_string_id_t name);

/*
 * Lookup using an interned canonical module path.
 *
 * Returns zero when not found.
 */
vitte_import_module_id_t
vitte_import_find_module_by_path(
    const vitte_import_context_t *context,
    vitte_import_string_id_t path);

/* ========================================================================= */
/* Import registration                                                       */
/* ========================================================================= */

/*
 * Register an import declaration.
 *
 * Forward imports are supported:
 *
 * the target module does not have to exist yet when the import is added.
 *
 * If it already exists, the implementation may resolve the edge eagerly.
 *
 * Duplicate source/name/alias imports are rejected.
 *
 * Direct self-imports are rejected when immediately detectable and during
 * resolution otherwise.
 *
 * Returns zero on failure.
 */
vitte_import_edge_id_t
vitte_import_add(
    vitte_import_context_t *context,
    const vitte_import_desc_t *description);

/*
 * Retrieve an import edge.
 */
const vitte_import_edge_t *
vitte_import_get(
    const vitte_import_context_t *context,
    vitte_import_edge_id_t id);

/* ========================================================================= */
/* Resolution                                                                */
/* ========================================================================= */

/*
 * Resolve the complete import graph.
 *
 * The operation:
 *
 *   1. clears previous resolution state;
 *   2. resolves every import name to a module ID;
 *   3. constructs direct dependencies;
 *   4. constructs reverse dependencies;
 *   5. rejects missing modules;
 *   6. rejects self-imports;
 *   7. detects dependency cycles;
 *   8. constructs deterministic dependency-first topological order;
 *   9. marks modules RESOLVED;
 *  10. marks the context RESOLVED.
 *
 * Example:
 *
 *     app -> parser -> lexer
 *
 * topological order:
 *
 *     lexer
 *     parser
 *     app
 *
 * Returns false if resolution fails.
 */
bool
vitte_import_resolve(
    vitte_import_context_t *context);

/* ========================================================================= */
/* Topological ordering                                                      */
/* ========================================================================= */

/*
 * Number of modules in the resolved topological order.
 *
 * Normally zero before successful resolution.
 */
size_t
vitte_import_topological_count(
    const vitte_import_context_t *context);

/*
 * Return one module ID from the dependency-first order.
 *
 * Returns zero when index is out of range.
 */
vitte_import_module_id_t
vitte_import_topological_at(
    const vitte_import_context_t *context,
    size_t index);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Perform complete structural validation.
 *
 * Checks include:
 *
 *   - context envelope;
 *   - ID/index correspondence;
 *   - string integrity;
 *   - string hashes;
 *   - module names;
 *   - module paths;
 *   - enum ranges;
 *   - dependency IDs;
 *   - reverse dependency IDs;
 *   - import-edge IDs;
 *   - edge source/target IDs;
 *   - alias IDs;
 *   - source spans;
 *   - self dependencies;
 *   - topological-order IDs;
 *   - duplicate topological entries;
 *   - string-byte accounting.
 */
bool
vitte_import_validate(
    vitte_import_context_t *context);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Compute a deterministic structural fingerprint.
 *
 * The fingerprint includes:
 *
 *   - API version;
 *   - interned string contents;
 *   - module identities;
 *   - module paths;
 *   - source IDs;
 *   - visibility;
 *   - root/external flags;
 *   - dependency relationships;
 *   - import edges;
 *   - aliases.
 *
 * Intended uses:
 *
 *   - incremental compilation;
 *   - regression tests;
 *   - cache bookkeeping;
 *   - deterministic-build diagnostics.
 *
 * This is FNV-based and is NOT cryptographically secure.
 */
uint64_t
vitte_import_fingerprint(
    vitte_import_context_t *context);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_import_stats_t
vitte_import_stats(
    const vitte_import_context_t *context);

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

/*
 * Replace resource limits.
 *
 * All values must be non-zero.
 *
 * New limits cannot be lower than already consumed resources.
 *
 * Only valid while BUILDING.
 */
bool
vitte_import_set_limits(
    vitte_import_context_t *context,
    size_t max_modules,
    size_t max_imports,
    size_t max_string_bytes);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_import_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline generic ID helpers                                                 */
/* ========================================================================= */

static inline bool
vitte_import_id_is_valid(
    vitte_import_id_t id)
{
    return id != VITTE_IMPORT_INVALID_ID;
}

static inline bool
vitte_import_string_id_is_valid(
    vitte_import_string_id_t id)
{
    return id != VITTE_IMPORT_INVALID_ID;
}

static inline bool
vitte_import_module_id_is_valid(
    vitte_import_module_id_t id)
{
    return id != VITTE_IMPORT_INVALID_ID;
}

static inline bool
vitte_import_edge_id_is_valid(
    vitte_import_edge_id_t id)
{
    return id != VITTE_IMPORT_INVALID_ID;
}

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_import_error_is_valid(
    vitte_import_error_t error)
{
    return
        error >= VITTE_IMPORT_ERROR_NONE &&
        error < VITTE_IMPORT_ERROR_COUNT;
}

static inline bool
vitte_import_state_is_valid(
    vitte_import_state_t state)
{
    return
        state > VITTE_IMPORT_STATE_INVALID &&
        state < VITTE_IMPORT_STATE_COUNT;
}

static inline bool
vitte_import_module_state_is_valid(
    vitte_import_module_state_t state)
{
    return
        state > VITTE_IMPORT_MODULE_INVALID &&
        state < VITTE_IMPORT_MODULE_COUNT;
}

static inline bool
vitte_import_edge_state_is_valid(
    vitte_import_edge_state_t state)
{
    return
        state > VITTE_IMPORT_EDGE_INVALID &&
        state < VITTE_IMPORT_EDGE_COUNT;
}

static inline bool
vitte_import_visibility_is_valid(
    vitte_import_visibility_t visibility)
{
    return
        visibility >= VITTE_IMPORT_VISIBILITY_PRIVATE &&
        visibility < VITTE_IMPORT_VISIBILITY_COUNT;
}

/* ========================================================================= */
/* Inline state queries                                                      */
/* ========================================================================= */

static inline bool
vitte_import_is_building(
    const vitte_import_context_t *context)
{
    return
        context != NULL &&
        context->magic == VITTE_IMPORT_MAGIC &&
        context->state == VITTE_IMPORT_STATE_BUILDING;
}

static inline bool
vitte_import_is_resolving(
    const vitte_import_context_t *context)
{
    return
        context != NULL &&
        context->magic == VITTE_IMPORT_MAGIC &&
        context->state == VITTE_IMPORT_STATE_RESOLVING;
}

static inline bool
vitte_import_is_resolved(
    const vitte_import_context_t *context)
{
    return
        context != NULL &&
        context->magic == VITTE_IMPORT_MAGIC &&
        context->state == VITTE_IMPORT_STATE_RESOLVED;
}

static inline bool
vitte_import_has_failed(
    const vitte_import_context_t *context)
{
    return
        context != NULL &&
        context->magic == VITTE_IMPORT_MAGIC &&
        context->state == VITTE_IMPORT_STATE_FAILED;
}

/* ========================================================================= */
/* Inline source span helpers                                                */
/* ========================================================================= */

static inline vitte_import_span_t
vitte_import_span_invalid(void)
{
    vitte_import_span_t span;

    span.file_id = 0u;
    span.begin = 0u;
    span.end = 0u;
    span.valid = false;

    return span;
}

static inline vitte_import_span_t
vitte_import_span_make(
    uint32_t file_id,
    size_t begin,
    size_t end)
{
    vitte_import_span_t span;

    span.file_id = file_id;
    span.begin = begin;
    span.end = end;
    span.valid = begin <= end;

    return span;
}

static inline size_t
vitte_import_span_length(
    const vitte_import_span_t *span)
{
    if (span == NULL ||
        !span->valid ||
        span->end < span->begin) {
        return 0u;
    }

    return span->end - span->begin;
}

/* ========================================================================= */
/* Inline empty descriptors                                                  */
/* ========================================================================= */

static inline vitte_import_module_desc_t
vitte_import_module_desc_empty(void)
{
    vitte_import_module_desc_t description = {0};

    description.visibility =
        VITTE_IMPORT_VISIBILITY_PRIVATE;

    return description;
}

static inline vitte_import_desc_t
vitte_import_desc_empty(void)
{
    vitte_import_desc_t description = {0};

    description.visibility =
        VITTE_IMPORT_VISIBILITY_PRIVATE;

    description.span =
        vitte_import_span_invalid();

    return description;
}

/* ========================================================================= */
/* Inline context queries                                                    */
/* ========================================================================= */

static inline vitte_import_error_t
vitte_import_last_error(
    const vitte_import_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return VITTE_IMPORT_ERROR_INVALID_CONTEXT;
    }

    return context->last_error;
}

static inline vitte_import_state_t
vitte_import_state(
    const vitte_import_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return VITTE_IMPORT_STATE_INVALID;
    }

    return context->state;
}

static inline size_t
vitte_import_module_count(
    const vitte_import_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return 0u;
    }

    return context->module_count;
}

static inline size_t
vitte_import_edge_count(
    const vitte_import_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return 0u;
    }

    return context->edge_count;
}

static inline size_t
vitte_import_string_count(
    const vitte_import_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return 0u;
    }

    return context->string_count;
}

static inline size_t
vitte_import_string_bytes(
    const vitte_import_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return 0u;
    }

    return context->string_bytes;
}

static inline uint64_t
vitte_import_generation(
    const vitte_import_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return UINT64_C(0);
    }

    return context->generation;
}

/* ========================================================================= */
/* Inline string queries                                                     */
/* ========================================================================= */

static inline const char *
vitte_import_string_data(
    const vitte_import_context_t *context,
    vitte_import_string_id_t id)
{
    const vitte_import_string_t *string;

    string = vitte_import_get_string(context, id);

    if (string == NULL) {
        return NULL;
    }

    return string->data;
}

static inline size_t
vitte_import_string_length(
    const vitte_import_context_t *context,
    vitte_import_string_id_t id)
{
    const vitte_import_string_t *string;

    string = vitte_import_get_string(context, id);

    if (string == NULL) {
        return 0u;
    }

    return string->length;
}

/* ========================================================================= */
/* Inline module queries                                                     */
/* ========================================================================= */

static inline bool
vitte_import_module_is_root(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id)
{
    const vitte_import_module_t *module;

    module = vitte_import_get_module(context, id);

    return module != NULL && module->is_root;
}

static inline bool
vitte_import_module_is_external(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id)
{
    const vitte_import_module_t *module;

    module = vitte_import_get_module(context, id);

    return module != NULL && module->is_external;
}

static inline bool
vitte_import_module_is_resolved(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id)
{
    const vitte_import_module_t *module;

    module = vitte_import_get_module(context, id);

    return
        module != NULL &&
        module->state == VITTE_IMPORT_MODULE_RESOLVED;
}

static inline size_t
vitte_import_module_dependency_count(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id)
{
    const vitte_import_module_t *module;

    module = vitte_import_get_module(context, id);

    if (module == NULL) {
        return 0u;
    }

    return module->dependency_count;
}

static inline size_t
vitte_import_module_dependent_count(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id)
{
    const vitte_import_module_t *module;

    module = vitte_import_get_module(context, id);

    if (module == NULL) {
        return 0u;
    }

    return module->dependent_count;
}

static inline size_t
vitte_import_module_import_count(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id)
{
    const vitte_import_module_t *module;

    module = vitte_import_get_module(context, id);

    if (module == NULL) {
        return 0u;
    }

    return module->import_count;
}

static inline vitte_import_module_id_t
vitte_import_module_dependency_at(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id,
    size_t index)
{
    const vitte_import_module_t *module;

    module = vitte_import_get_module(context, id);

    if (module == NULL ||
        index >= module->dependency_count ||
        module->dependencies == NULL) {
        return VITTE_IMPORT_INVALID_ID;
    }

    return module->dependencies[index];
}

static inline vitte_import_module_id_t
vitte_import_module_dependent_at(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id,
    size_t index)
{
    const vitte_import_module_t *module;

    module = vitte_import_get_module(context, id);

    if (module == NULL ||
        index >= module->dependent_count ||
        module->dependents == NULL) {
        return VITTE_IMPORT_INVALID_ID;
    }

    return module->dependents[index];
}

static inline vitte_import_edge_id_t
vitte_import_module_import_at(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id,
    size_t index)
{
    const vitte_import_module_t *module;

    module = vitte_import_get_module(context, id);

    if (module == NULL ||
        index >= module->import_count ||
        module->imports == NULL) {
        return VITTE_IMPORT_INVALID_ID;
    }

    return module->imports[index];
}

/* ========================================================================= */
/* Inline edge queries                                                       */
/* ========================================================================= */

static inline bool
vitte_import_edge_is_resolved(
    const vitte_import_context_t *context,
    vitte_import_edge_id_t id)
{
    const vitte_import_edge_t *edge;

    edge = vitte_import_get(context, id);

    return
        edge != NULL &&
        edge->state == VITTE_IMPORT_EDGE_RESOLVED;
}

static inline bool
vitte_import_edge_is_public(
    const vitte_import_context_t *context,
    vitte_import_edge_id_t id)
{
    const vitte_import_edge_t *edge;

    edge = vitte_import_get(context, id);

    return
        edge != NULL &&
        edge->visibility == VITTE_IMPORT_VISIBILITY_PUBLIC;
}

static inline bool
vitte_import_edge_has_alias(
    const vitte_import_context_t *context,
    vitte_import_edge_id_t id)
{
    const vitte_import_edge_t *edge;

    edge = vitte_import_get(context, id);

    return
        edge != NULL &&
        edge->alias != VITTE_IMPORT_INVALID_ID;
}

/* ========================================================================= */
/* Inline statistics                                                        */
/* ========================================================================= */

static inline uint64_t
vitte_import_resolution_runs(
    const vitte_import_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return UINT64_C(0);
    }

    return context->stats.resolution_runs;
}

static inline uint64_t
vitte_import_cycle_count(
    const vitte_import_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return UINT64_C(0);
    }

    return context->stats.cycles_detected;
}

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__cplusplus)

#define VITTE_IMPORT_STATIC_ASSERT(condition, message) \
    static_assert((condition), message)

#else

#define VITTE_IMPORT_STATIC_ASSERT(condition, message) \
    _Static_assert((condition), message)

#endif

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_INVALID_ID == UINT64_C(0),
    "import zero ID must remain invalid");

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_DEFAULT_INITIAL_CAPACITY > 0u,
    "import initial capacity must be non-zero");

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_DEFAULT_MAX_MODULES > 0u,
    "import module limit must be non-zero");

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_DEFAULT_MAX_IMPORTS > 0u,
    "import edge limit must be non-zero");

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_DEFAULT_MAX_STRING_BYTES > 0u,
    "import string limit must be non-zero");

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_ERROR_COUNT >
        VITTE_IMPORT_ERROR_INTERNAL,
    "import error enum invariant");

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_STATE_COUNT >
        VITTE_IMPORT_STATE_DESTROYED,
    "import state enum invariant");

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_MODULE_COUNT >
        VITTE_IMPORT_MODULE_FAILED,
    "import module state enum invariant");

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_EDGE_COUNT >
        VITTE_IMPORT_EDGE_FAILED,
    "import edge state enum invariant");

VITTE_IMPORT_STATIC_ASSERT(
    VITTE_IMPORT_VISIBILITY_COUNT >
        VITTE_IMPORT_VISIBILITY_PUBLIC,
    "import visibility enum invariant");

#undef VITTE_IMPORT_STATIC_ASSERT

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_IMPORT_IMPORT_H */
