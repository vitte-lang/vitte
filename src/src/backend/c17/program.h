#ifndef VITTE_SRC_BACKEND_C17_PROGRAM_H
#define VITTE_SRC_BACKEND_C17_PROGRAM_H

/*
 * Vitte Compiler
 * src/backend/c17/program.h
 *
 * Whole-program model for the ISO C17 backend.
 *
 * This layer represents the complete backend program after per-module
 * construction and before final C17 translation-unit emission.
 *
 * Responsibilities:
 *
 *   - module registry
 *   - stable module IDs
 *   - dependency graph
 *   - entry point
 *   - symbol registry
 *   - generated-name uniqueness
 *   - reachability
 *   - deterministic topological ordering
 *   - whole-program validation
 *   - statistics
 *   - stable fingerprints
 *
 * Architecture:
 *
 *      AST / HIR / IR
 *            |
 *            v
 *         module.c
 *            |
 *            v
 *        program.c
 *            |
 *      +-----+------+----------------+
 *      |            |                |
 *      v            v                v
 *   naming.c    runtime.c        options.c
 *      |            |                |
 *      +------------+----------------+
 *                   |
 *                   v
 *          translation_unit.c
 *                   |
 *                   v
 *               backend.c
 *                   |
 *                   v
 *           generated ISO C17
 *
 * Ownership:
 *
 *   - vitte_c17_program_t owns:
 *       * module registry storage
 *       * copied module names
 *       * dependency storage
 *       * symbol registry storage
 *       * copied symbol names
 *       * ordering caches
 *       * reachability caches
 *
 *   - vitte_c17_program_t does NOT own vitte_c17_module_t objects.
 *
 * Modules must therefore outlive the program or remain alive until they are
 * removed/reset/destroyed.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration constants                                                   */
/* ========================================================================= */

#ifndef VITTE_C17_PROGRAM_MAGIC
#define VITTE_C17_PROGRAM_MAGIC \
    UINT64_C(0x5649545445505247)
#endif

#ifndef VITTE_C17_PROGRAM_DEAD_MAGIC
#define VITTE_C17_PROGRAM_DEAD_MAGIC \
    UINT64_C(0x4445414450524F47)
#endif

#ifndef VITTE_C17_PROGRAM_INITIAL_MODULE_CAPACITY
#define VITTE_C17_PROGRAM_INITIAL_MODULE_CAPACITY \
    ((size_t)16u)
#endif

#ifndef VITTE_C17_PROGRAM_INITIAL_DEPENDENCY_CAPACITY
#define VITTE_C17_PROGRAM_INITIAL_DEPENDENCY_CAPACITY \
    ((size_t)32u)
#endif

#ifndef VITTE_C17_PROGRAM_INITIAL_SYMBOL_CAPACITY
#define VITTE_C17_PROGRAM_INITIAL_SYMBOL_CAPACITY \
    ((size_t)64u)
#endif

#ifndef VITTE_C17_PROGRAM_MAX_MODULES
#define VITTE_C17_PROGRAM_MAX_MODULES \
    ((size_t)(1024u * 1024u))
#endif

#ifndef VITTE_C17_PROGRAM_MAX_DEPENDENCIES
#define VITTE_C17_PROGRAM_MAX_DEPENDENCIES \
    ((size_t)(16u * 1024u * 1024u))
#endif

#ifndef VITTE_C17_PROGRAM_MAX_SYMBOLS
#define VITTE_C17_PROGRAM_MAX_SYMBOLS \
    ((size_t)(16u * 1024u * 1024u))
#endif

#ifndef VITTE_C17_PROGRAM_MAX_NAME_BYTES
#define VITTE_C17_PROGRAM_MAX_NAME_BYTES \
    ((size_t)(64u * 1024u))
#endif

#ifndef VITTE_C17_PROGRAM_HASH_OFFSET
#define VITTE_C17_PROGRAM_HASH_OFFSET \
    UINT64_C(14695981039346656037)
#endif

#ifndef VITTE_C17_PROGRAM_HASH_PRIME
#define VITTE_C17_PROGRAM_HASH_PRIME \
    UINT64_C(1099511628211)
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_c17_module
    vitte_c17_module_t;

typedef struct vitte_c17_program
    vitte_c17_program_t;

typedef struct vitte_c17_program_config
    vitte_c17_program_config_t;

typedef struct vitte_c17_program_module
    vitte_c17_program_module_t;

typedef struct vitte_c17_program_dependency
    vitte_c17_program_dependency_t;

typedef struct vitte_c17_program_symbol
    vitte_c17_program_symbol_t;

typedef struct vitte_c17_program_stats
    vitte_c17_program_stats_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_c17_program_error {
    VITTE_C17_PROGRAM_ERROR_NONE = 0,

    VITTE_C17_PROGRAM_ERROR_INVALID_PROGRAM,

    VITTE_C17_PROGRAM_ERROR_INVALID_ARGUMENT,

    VITTE_C17_PROGRAM_ERROR_INVALID_MODULE,

    VITTE_C17_PROGRAM_ERROR_INVALID_SYMBOL,

    VITTE_C17_PROGRAM_ERROR_DUPLICATE_MODULE,

    VITTE_C17_PROGRAM_ERROR_DUPLICATE_SYMBOL,

    VITTE_C17_PROGRAM_ERROR_UNKNOWN_MODULE,

    VITTE_C17_PROGRAM_ERROR_UNKNOWN_SYMBOL,

    VITTE_C17_PROGRAM_ERROR_SELF_DEPENDENCY,

    VITTE_C17_PROGRAM_ERROR_DUPLICATE_DEPENDENCY,

    VITTE_C17_PROGRAM_ERROR_DEPENDENCY_CYCLE,

    VITTE_C17_PROGRAM_ERROR_ENTRY_NOT_FOUND,

    VITTE_C17_PROGRAM_ERROR_MULTIPLE_ENTRY,

    VITTE_C17_PROGRAM_ERROR_LIMIT_EXCEEDED,

    VITTE_C17_PROGRAM_ERROR_OVERFLOW,

    VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY,

    VITTE_C17_PROGRAM_ERROR_CORRUPTION,

    VITTE_C17_PROGRAM_ERROR_COUNT
} vitte_c17_program_error_t;

/* ========================================================================= */
/* Symbol kinds                                                              */
/* ========================================================================= */

typedef enum vitte_c17_program_symbol_kind {
    VITTE_C17_PROGRAM_SYMBOL_INVALID = 0,

    /*
     * Generated C function.
     */
    VITTE_C17_PROGRAM_SYMBOL_FUNCTION,

    /*
     * Global/static storage object.
     */
    VITTE_C17_PROGRAM_SYMBOL_GLOBAL,

    /*
     * C type-level entity:
     *
     *   typedef
     *   struct
     *   union-like lowering
     *   enum-like lowering
     */
    VITTE_C17_PROGRAM_SYMBOL_TYPE,

    /*
     * Compile-time or generated constant.
     */
    VITTE_C17_PROGRAM_SYMBOL_CONSTANT,

    /*
     * Symbol owned by or required from the Vitte runtime.
     */
    VITTE_C17_PROGRAM_SYMBOL_RUNTIME,

    /*
     * Backend-generated implementation symbol.
     *
     * Examples:
     *
     *   temporaries promoted to file scope
     *   helper functions
     *   generated tables
     *   initialization helpers
     */
    VITTE_C17_PROGRAM_SYMBOL_INTERNAL,

    VITTE_C17_PROGRAM_SYMBOL_COUNT
} vitte_c17_program_symbol_kind_t;

/* ========================================================================= */
/* Symbol visibility                                                         */
/* ========================================================================= */

typedef enum vitte_c17_program_symbol_visibility {
    VITTE_C17_PROGRAM_SYMBOL_VISIBILITY_INVALID = 0,

    /*
     * Internal/module-local symbol.
     *
     * Usually emitted with static linkage when appropriate.
     */
    VITTE_C17_PROGRAM_SYMBOL_PRIVATE,

    /*
     * Program-visible/exported symbol.
     */
    VITTE_C17_PROGRAM_SYMBOL_PUBLIC,

    /*
     * Defined outside this generated program.
     */
    VITTE_C17_PROGRAM_SYMBOL_EXTERNAL,

    VITTE_C17_PROGRAM_SYMBOL_VISIBILITY_COUNT
} vitte_c17_program_symbol_visibility_t;

/* ========================================================================= */
/* Program configuration                                                     */
/* ========================================================================= */

struct vitte_c17_program_config {
    /*
     * Hard resource limits.
     */
    size_t max_modules;

    size_t max_dependencies;

    size_t max_symbols;

    size_t max_name_bytes;

    /*
     * Use stable lexical module ordering whenever several nodes are ready
     * during topological sorting.
     *
     * When false, registration order is used as the tie-breaker.
     */
    bool deterministic;

    /*
     * Require an executable entry point.
     *
     * Set false for library compilation.
     */
    bool require_entry;

    /*
     * Reject cyclic module dependency graphs.
     */
    bool reject_cycles;

    /*
     * Reject duplicate source/generated symbols.
     */
    bool reject_duplicate_symbols;

    /*
     * Reject repeated A -> B dependency edges.
     */
    bool reject_duplicate_dependencies;

    /*
     * Reserved for integration with module.c deep validation.
     *
     * program.c currently validates registry integrity itself. The module
     * subsystem may use this policy when program/module integration is
     * tightened.
     */
    bool validate_modules;

    /*
     * Include modules unreachable from the executable entry point in the
     * final topological order.
     *
     * Useful for:
     *
     *   - whole-library emission
     *   - tooling
     *   - debugging
     *   - keeping intentionally referenced modules
     */
    bool include_unreachable_modules;
};

/* ========================================================================= */
/* Registered module                                                         */
/* ========================================================================= */

struct vitte_c17_program_module {
    /*
     * Stable program-local module identity.
     *
     * Zero is reserved as invalid.
     */
    uint64_t id;

    /*
     * Non-owning module object.
     */
    vitte_c17_module_t *module;

    /*
     * Program-owned canonical module name.
     */
    char *name;

    size_t name_length;

    /*
     * Original registration position.
     *
     * Used as deterministic fallback when lexical deterministic ordering is
     * disabled.
     */
    size_t registration_index;

    /*
     * True for the selected executable entry module.
     */
    bool is_entry;

    /*
     * Reachability cache.
     *
     * Computed from entry_module_id through dependency edges.
     */
    bool reachable;
};

/* ========================================================================= */
/* Dependency                                                                */
/* ========================================================================= */

/*
 * Dependency orientation:
 *
 *     from_module_id -> to_module_id
 *
 * means:
 *
 *     "from_module_id depends on to_module_id"
 *
 * Example:
 *
 *     application -> core
 *
 * means core must be emitted/initialized before application.
 *
 * The topological sorter therefore internally interprets this as:
 *
 *     core -> application
 *
 * for ordering purposes.
 */
struct vitte_c17_program_dependency {
    uint64_t from_module_id;

    uint64_t to_module_id;

    /*
     * Stable original edge order.
     */
    size_t registration_index;
};

/* ========================================================================= */
/* Symbol                                                                    */
/* ========================================================================= */

struct vitte_c17_program_symbol {
    /*
     * Stable program-local symbol identity.
     *
     * Zero is invalid.
     */
    uint64_t id;

    /*
     * Owning module.
     *
     * Zero is permitted for whole-program/runtime/internal symbols that are
     * intentionally not attached to a Vitte module.
     */
    uint64_t module_id;

    /*
     * Optional upstream AST/HIR/IR symbol identity.
     *
     * Zero means unavailable/not applicable.
     */
    uint64_t source_id;

    vitte_c17_program_symbol_kind_t kind;

    vitte_c17_program_symbol_visibility_t visibility;

    /*
     * Original Vitte/backend source-level name.
     *
     * Owned by program.
     */
    char *source_name;

    size_t source_name_length;

    /*
     * Final generated C identifier.
     *
     * Owned by program.
     *
     * program.c treats this as a whole-program namespace and therefore
     * requires uniqueness.
     */
    char *generated_name;

    size_t generated_name_length;

    /*
     * true:
     *     this registry item owns/emits a definition.
     *
     * false:
     *     declaration/reference/import only.
     */
    bool definition;

    size_t registration_index;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_c17_program_stats {
    bool valid;

    /* Modules */
    size_t module_count;

    size_t module_capacity;

    size_t reachable_module_count;

    size_t ordered_module_count;

    size_t module_name_bytes;

    size_t reachable_module_name_bytes;

    /* Dependencies */
    size_t dependency_count;

    size_t dependency_capacity;

    /* Symbols */
    size_t symbol_count;

    size_t symbol_capacity;

    size_t definition_count;

    size_t declaration_count;

    size_t private_symbol_count;

    size_t public_symbol_count;

    size_t external_symbol_count;

    /*
     * Indexed by vitte_c17_program_symbol_kind_t.
     *
     * Element zero corresponds to INVALID and should normally remain zero.
     */
    size_t symbols_by_kind[
        VITTE_C17_PROGRAM_SYMBOL_COUNT];

    size_t source_symbol_name_bytes;

    size_t generated_symbol_name_bytes;

    /* Program */
    bool has_entry;

    uint64_t generation;

    /*
     * Stable non-cryptographic fingerprint.
     */
    uint64_t hash;
};

/* ========================================================================= */
/* Program                                                                   */
/* ========================================================================= */

struct vitte_c17_program {
    /*
     * Integrity marker.
     */
    uint64_t magic;

    vitte_c17_program_config_t config;

    vitte_c17_program_error_t last_error;

    /* --------------------------------------------------------------------- */
    /* Modules                                                               */
    /* --------------------------------------------------------------------- */

    vitte_c17_program_module_t *modules;

    size_t module_count;

    size_t module_capacity;

    uint64_t next_module_id;

    /* --------------------------------------------------------------------- */
    /* Dependency graph                                                      */
    /* --------------------------------------------------------------------- */

    vitte_c17_program_dependency_t *dependencies;

    size_t dependency_count;

    size_t dependency_capacity;

    /* --------------------------------------------------------------------- */
    /* Symbols                                                               */
    /* --------------------------------------------------------------------- */

    vitte_c17_program_symbol_t *symbols;

    size_t symbol_count;

    size_t symbol_capacity;

    uint64_t next_symbol_id;

    /* --------------------------------------------------------------------- */
    /* Entry                                                                 */
    /* --------------------------------------------------------------------- */

    uint64_t entry_module_id;

    uint64_t entry_symbol_id;

    bool has_entry;

    /* --------------------------------------------------------------------- */
    /* Topological ordering cache                                            */
    /* --------------------------------------------------------------------- */

    /*
     * Module IDs in dependency-safe emission order.
     */
    uint64_t *ordered_modules;

    size_t ordered_module_count;

    /* --------------------------------------------------------------------- */
    /* Reachability cache                                                    */
    /* --------------------------------------------------------------------- */

    uint64_t *reachable_modules;

    size_t reachable_module_count;

    /* --------------------------------------------------------------------- */
    /* Generation                                                            */
    /* --------------------------------------------------------------------- */

    /*
     * Logical lifecycle generation.
     *
     * Incremented on reset when possible.
     */
    uint64_t generation;
};

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_program_error_name(
    vitte_c17_program_error_t error);

const char *
vitte_c17_program_symbol_kind_name(
    vitte_c17_program_symbol_kind_t kind);

const char *
vitte_c17_program_symbol_visibility_name(
    vitte_c17_program_symbol_visibility_t visibility);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_c17_program_config_t
vitte_c17_program_config_default(void);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_c17_program_init(
    vitte_c17_program_t *program,
    const vitte_c17_program_config_t *config);

/*
 * Remove all registered program data while retaining the allocated backing
 * arrays for future reuse.
 *
 * Module objects themselves are never destroyed by this function.
 */
void
vitte_c17_program_reset(
    vitte_c17_program_t *program);

/*
 * Destroy all program-owned storage.
 *
 * Module objects themselves remain caller-owned.
 */
void
vitte_c17_program_destroy(
    vitte_c17_program_t *program);

/* ========================================================================= */
/* Structural validity                                                       */
/* ========================================================================= */

/*
 * Cheap structural validity check.
 *
 * This is intentionally distinct from vitte_c17_program_validate(), which
 * performs deep whole-program validation.
 */
bool
vitte_c17_program_is_valid(
    const vitte_c17_program_t *program);

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

bool
vitte_c17_program_reserve_modules(
    vitte_c17_program_t *program,
    size_t capacity);

bool
vitte_c17_program_reserve_dependencies(
    vitte_c17_program_t *program,
    size_t capacity);

bool
vitte_c17_program_reserve_symbols(
    vitte_c17_program_t *program,
    size_t capacity);

/* ========================================================================= */
/* Module registration                                                       */
/* ========================================================================= */

/*
 * Register a non-owning module pointer and copy its canonical name.
 *
 * Returns:
 *
 *     non-zero stable module ID on success
 *     zero on failure
 *
 * Duplicate module pointers and duplicate canonical module names are
 * rejected.
 */
uint64_t
vitte_c17_program_add_module(
    vitte_c17_program_t *program,
    vitte_c17_module_t *module,
    const char *name,
    size_t name_length);

/* ========================================================================= */
/* Module lookup                                                             */
/* ========================================================================= */

const vitte_c17_program_module_t *
vitte_c17_program_find_module_by_id(
    const vitte_c17_program_t *program,
    uint64_t module_id);

const vitte_c17_program_module_t *
vitte_c17_program_find_module_by_name(
    const vitte_c17_program_t *program,
    const char *name,
    size_t length);

const vitte_c17_program_module_t *
vitte_c17_program_find_module_by_pointer(
    const vitte_c17_program_t *program,
    const vitte_c17_module_t *module);

/* ========================================================================= */
/* Dependencies                                                              */
/* ========================================================================= */

/*
 * Register:
 *
 *     from_module_id depends on to_module_id
 */
bool
vitte_c17_program_add_dependency(
    vitte_c17_program_t *program,
    uint64_t from_module_id,
    uint64_t to_module_id);

bool
vitte_c17_program_has_dependency(
    const vitte_c17_program_t *program,
    uint64_t from_module_id,
    uint64_t to_module_id);

/* ========================================================================= */
/* Entry module                                                              */
/* ========================================================================= */

bool
vitte_c17_program_set_entry_module(
    vitte_c17_program_t *program,
    uint64_t module_id);

bool
vitte_c17_program_clear_entry(
    vitte_c17_program_t *program);

/* ========================================================================= */
/* Symbol registry                                                           */
/* ========================================================================= */

/*
 * Register a whole-program C symbol.
 *
 * module_id:
 *
 *     non-zero -> symbol belongs to a registered module
 *     zero     -> runtime/program/internal symbol
 *
 * source_id:
 *
 *     optional AST/HIR/IR identity; zero means unavailable.
 *
 * Returns zero on failure.
 */
uint64_t
vitte_c17_program_add_symbol(
    vitte_c17_program_t *program,
    uint64_t module_id,
    vitte_c17_program_symbol_kind_t kind,
    vitte_c17_program_symbol_visibility_t visibility,
    const char *source_name,
    size_t source_name_length,
    const char *generated_name,
    size_t generated_name_length,
    uint64_t source_id,
    bool definition);

const vitte_c17_program_symbol_t *
vitte_c17_program_find_symbol_by_id(
    const vitte_c17_program_t *program,
    uint64_t symbol_id);

const vitte_c17_program_symbol_t *
vitte_c17_program_find_generated_symbol(
    const vitte_c17_program_t *program,
    const char *name,
    size_t length);

const vitte_c17_program_symbol_t *
vitte_c17_program_find_source_symbol(
    const vitte_c17_program_t *program,
    uint64_t module_id,
    vitte_c17_program_symbol_kind_t kind,
    const char *name,
    size_t length);

/* ========================================================================= */
/* Entry symbol                                                              */
/* ========================================================================= */

/*
 * Select a registered function definition as the program entry symbol.
 *
 * If the symbol belongs to a module, that module automatically becomes the
 * entry module.
 */
bool
vitte_c17_program_set_entry_symbol(
    vitte_c17_program_t *program,
    uint64_t symbol_id);

/* ========================================================================= */
/* Reachability                                                              */
/* ========================================================================= */

/*
 * Compute modules reachable from the entry module.
 *
 * If no entry exists (library mode), all registered modules are considered
 * reachable.
 */
bool
vitte_c17_program_compute_reachability(
    vitte_c17_program_t *program);

/* ========================================================================= */
/* Ordering                                                                  */
/* ========================================================================= */

/*
 * Compute dependency-safe module order.
 *
 * For:
 *
 *     A -> B
 *
 * where A depends on B, B is placed before A.
 *
 * When several modules are simultaneously ready:
 *
 *     deterministic=true
 *         canonical module name is the primary ordering key.
 *
 *     deterministic=false
 *         registration order is used.
 *
 * Returns false when the included dependency graph contains a cycle.
 */
bool
vitte_c17_program_compute_order(
    vitte_c17_program_t *program);

size_t
vitte_c17_program_ordered_module_count(
    const vitte_c17_program_t *program);

const vitte_c17_program_module_t *
vitte_c17_program_ordered_module_at(
    const vitte_c17_program_t *program,
    size_t index);

/* ========================================================================= */
/* Root modules                                                              */
/* ========================================================================= */

/*
 * Return true when no other registered module directly depends on module_id.
 *
 * Note:
 *
 * "root" here refers to dependency-graph incoming edges according to the
 * stored A -> B means A depends on B orientation. It is not synonymous with
 * "entry module".
 */
bool
vitte_c17_program_module_is_root(
    const vitte_c17_program_t *program,
    uint64_t module_id);

/* ========================================================================= */
/* Indexed access                                                            */
/* ========================================================================= */

size_t
vitte_c17_program_module_count(
    const vitte_c17_program_t *program);

size_t
vitte_c17_program_dependency_count(
    const vitte_c17_program_t *program);

size_t
vitte_c17_program_symbol_count(
    const vitte_c17_program_t *program);

const vitte_c17_program_module_t *
vitte_c17_program_module_at(
    const vitte_c17_program_t *program,
    size_t index);

const vitte_c17_program_dependency_t *
vitte_c17_program_dependency_at(
    const vitte_c17_program_t *program,
    size_t index);

const vitte_c17_program_symbol_t *
vitte_c17_program_symbol_at(
    const vitte_c17_program_t *program,
    size_t index);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Return a stable non-cryptographic whole-program fingerprint.
 *
 * Current implementation uses FNV-1a-style hashing with explicit integer
 * serialization.
 *
 * Intended uses:
 *
 *   - backend cache keys
 *   - reproducibility checks
 *   - incremental compilation metadata
 *   - diagnostics/debugging
 *
 * Do not use as a cryptographic hash.
 */
uint64_t
vitte_c17_program_hash(
    const vitte_c17_program_t *program);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_program_stats_t
vitte_c17_program_stats(
    const vitte_c17_program_t *program);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Deep whole-program validation.
 *
 * Checks include:
 *
 *   - configuration invariants
 *   - capacities/counts
 *   - module identities
 *   - duplicate module names
 *   - duplicate module pointers
 *   - entry consistency
 *   - dependency endpoints
 *   - self-dependencies
 *   - duplicate dependency edges
 *   - symbol identities
 *   - symbol kinds
 *   - symbol visibility
 *   - generated-name uniqueness
 *   - source-name uniqueness per module/kind
 *   - entry-symbol validity
 *   - ordering-cache integrity
 *   - reachability-cache integrity
 */
bool
vitte_c17_program_validate(
    const vitte_c17_program_t *program);

/* ========================================================================= */
/* Finalization                                                              */
/* ========================================================================= */

/*
 * Prepare the program for final C17 emission.
 *
 * Performs:
 *
 *   1. entry validation
 *   2. reachability
 *   3. topological ordering
 *   4. cycle detection
 *   5. deep whole-program validation
 *
 * On success the ordered module cache is ready for translation_unit.c.
 */
bool
vitte_c17_program_finalize(
    vitte_c17_program_t *program);

/* ========================================================================= */
/* Error state                                                               */
/* ========================================================================= */

vitte_c17_program_error_t
vitte_c17_program_last_error(
    const vitte_c17_program_t *program);

void
vitte_c17_program_clear_error(
    vitte_c17_program_t *program);

/* ========================================================================= */
/* Inline module helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_c17_program_has_modules(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC &&
        program->module_count != 0u;
}

static inline bool
vitte_c17_program_has_dependencies(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC &&
        program->dependency_count != 0u;
}

static inline bool
vitte_c17_program_has_symbols(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC &&
        program->symbol_count != 0u;
}

static inline bool
vitte_c17_program_has_entry(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC &&
        program->has_entry;
}

static inline uint64_t
vitte_c17_program_entry_module_id(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC
            ? program->entry_module_id
            : UINT64_C(0);
}

static inline uint64_t
vitte_c17_program_entry_symbol_id(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC
            ? program->entry_symbol_id
            : UINT64_C(0);
}

static inline uint64_t
vitte_c17_program_generation(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC
            ? program->generation
            : UINT64_C(0);
}

/* ========================================================================= */
/* Inline ordering helpers                                                   */
/* ========================================================================= */

static inline bool
vitte_c17_program_has_order(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC &&
        program->ordered_module_count != 0u;
}

static inline bool
vitte_c17_program_has_reachability(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC &&
        program->reachable_module_count != 0u;
}

/* ========================================================================= */
/* Inline symbol helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_c17_program_symbol_is_definition(
    const vitte_c17_program_symbol_t *symbol)
{
    return
        symbol != NULL &&
        symbol->definition;
}

static inline bool
vitte_c17_program_symbol_is_external(
    const vitte_c17_program_symbol_t *symbol)
{
    return
        symbol != NULL &&
        symbol->visibility ==
            VITTE_C17_PROGRAM_SYMBOL_EXTERNAL;
}

static inline bool
vitte_c17_program_symbol_is_public(
    const vitte_c17_program_symbol_t *symbol)
{
    return
        symbol != NULL &&
        symbol->visibility ==
            VITTE_C17_PROGRAM_SYMBOL_PUBLIC;
}

static inline bool
vitte_c17_program_symbol_is_private(
    const vitte_c17_program_symbol_t *symbol)
{
    return
        symbol != NULL &&
        symbol->visibility ==
            VITTE_C17_PROGRAM_SYMBOL_PRIVATE;
}

static inline bool
vitte_c17_program_symbol_is_function(
    const vitte_c17_program_symbol_t *symbol)
{
    return
        symbol != NULL &&
        symbol->kind ==
            VITTE_C17_PROGRAM_SYMBOL_FUNCTION;
}

/* ========================================================================= */
/* Error helpers                                                             */
/* ========================================================================= */

static inline bool
vitte_c17_program_has_error(
    const vitte_c17_program_t *program)
{
    return
        program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC &&
        program->last_error !=
            VITTE_C17_PROGRAM_ERROR_NONE;
}

/* ========================================================================= */
/* Convenience macros                                                        */
/* ========================================================================= */

#define VITTE_C17_PROGRAM_FOR_EACH_MODULE(program_ptr, index_name) \
    for ((index_name) = 0u; \
         (program_ptr) != NULL && \
         (index_name) < (program_ptr)->module_count; \
         ++(index_name))

#define VITTE_C17_PROGRAM_FOR_EACH_DEPENDENCY(program_ptr, index_name) \
    for ((index_name) = 0u; \
         (program_ptr) != NULL && \
         (index_name) < (program_ptr)->dependency_count; \
         ++(index_name))

#define VITTE_C17_PROGRAM_FOR_EACH_SYMBOL(program_ptr, index_name) \
    for ((index_name) = 0u; \
         (program_ptr) != NULL && \
         (index_name) < (program_ptr)->symbol_count; \
         ++(index_name))

#define VITTE_C17_PROGRAM_FOR_EACH_ORDERED_MODULE(program_ptr, index_name) \
    for ((index_name) = 0u; \
         (program_ptr) != NULL && \
         (index_name) < (program_ptr)->ordered_module_count; \
         ++(index_name))

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte C17 program requires 64-bit uint64_t");

_Static_assert(
    VITTE_C17_PROGRAM_ERROR_NONE == 0,
    "program no-error value must remain zero");

_Static_assert(
    VITTE_C17_PROGRAM_SYMBOL_INVALID == 0,
    "invalid program symbol kind must remain zero");

_Static_assert(
    VITTE_C17_PROGRAM_SYMBOL_VISIBILITY_INVALID == 0,
    "invalid symbol visibility must remain zero");

_Static_assert(
    VITTE_C17_PROGRAM_INITIAL_MODULE_CAPACITY != 0u,
    "initial module capacity must not be zero");

_Static_assert(
    VITTE_C17_PROGRAM_INITIAL_DEPENDENCY_CAPACITY != 0u,
    "initial dependency capacity must not be zero");

_Static_assert(
    VITTE_C17_PROGRAM_INITIAL_SYMBOL_CAPACITY != 0u,
    "initial symbol capacity must not be zero");

_Static_assert(
    VITTE_C17_PROGRAM_INITIAL_MODULE_CAPACITY <=
        VITTE_C17_PROGRAM_MAX_MODULES,
    "initial module capacity exceeds maximum");

_Static_assert(
    VITTE_C17_PROGRAM_INITIAL_DEPENDENCY_CAPACITY <=
        VITTE_C17_PROGRAM_MAX_DEPENDENCIES,
    "initial dependency capacity exceeds maximum");

_Static_assert(
    VITTE_C17_PROGRAM_INITIAL_SYMBOL_CAPACITY <=
        VITTE_C17_PROGRAM_MAX_SYMBOLS,
    "initial symbol capacity exceeds maximum");

_Static_assert(
    VITTE_C17_PROGRAM_MAX_NAME_BYTES != 0u,
    "maximum program name length must not be zero");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_BACKEND_C17_PROGRAM_H */
