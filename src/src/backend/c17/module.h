#ifndef VITTE_SRC_BACKEND_C17_MODULE_H
#define VITTE_SRC_BACKEND_C17_MODULE_H

/*
 * Vitte Compiler
 * src/backend/c17/module.h
 *
 * C17 backend module representation.
 *
 * A vitte_c17_module_t is an owned, deterministic intermediate container
 * between Vitte lowering and final C17 output.
 *
 * Responsibilities:
 *
 *   - module identity
 *   - source identity
 *   - dependency collection
 *   - C include collection
 *   - generated symbol registration
 *   - feature requirements
 *   - runtime requirements
 *   - section-oriented generated fragments
 *   - deterministic ordering
 *   - sealing
 *   - validation
 *   - emission through backend.c
 *   - statistics
 *
 * Ownership:
 *
 *   The module owns:
 *
 *       name
 *       source_path
 *       dependencies
 *       includes
 *       symbols
 *       features
 *       fragments
 *
 *   All caller strings passed to module mutation functions are copied.
 *
 * Lifecycle:
 *
 *       UNINITIALIZED
 *             |
 *             v
 *          BUILDING
 *             |
 *             v
 *           SEALED
 *             |
 *             v
 *          EMITTING
 *             |
 *             +----> EMITTED
 *             |
 *             +----> FAILED
 *
 * A sealed module is immutable.
 */

#include "backend.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_C17_MODULE_MAGIC
#define VITTE_C17_MODULE_MAGIC \
    UINT64_C(0x56495454454D4F44)
#endif

#ifndef VITTE_C17_MODULE_DEAD_MAGIC
#define VITTE_C17_MODULE_DEAD_MAGIC \
    UINT64_C(0x444541444D4F4421)
#endif

#ifndef VITTE_C17_MODULE_INITIAL_CAPACITY
#define VITTE_C17_MODULE_INITIAL_CAPACITY \
    ((size_t)8u)
#endif

#ifndef VITTE_C17_MODULE_INITIAL_FRAGMENT_CAPACITY
#define VITTE_C17_MODULE_INITIAL_FRAGMENT_CAPACITY \
    ((size_t)16u)
#endif

#ifndef VITTE_C17_MODULE_MAX_NAME_BYTES
#define VITTE_C17_MODULE_MAX_NAME_BYTES \
    ((size_t)4096u)
#endif

#ifndef VITTE_C17_MODULE_MAX_PATH_BYTES
#define VITTE_C17_MODULE_MAX_PATH_BYTES \
    ((size_t)(1024u * 1024u))
#endif

#ifndef VITTE_C17_MODULE_MAX_FRAGMENT_BYTES
#define VITTE_C17_MODULE_MAX_FRAGMENT_BYTES \
    SIZE_MAX
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_c17_module
    vitte_c17_module_t;

typedef struct vitte_c17_module_config
    vitte_c17_module_config_t;

typedef struct vitte_c17_module_dependency
    vitte_c17_module_dependency_t;

typedef struct vitte_c17_module_include
    vitte_c17_module_include_t;

typedef struct vitte_c17_module_symbol
    vitte_c17_module_symbol_t;

typedef struct vitte_c17_module_feature
    vitte_c17_module_feature_t;

typedef struct vitte_c17_module_fragment
    vitte_c17_module_fragment_t;

typedef struct vitte_c17_module_stats
    vitte_c17_module_stats_t;

/* ========================================================================= */
/* Module state                                                              */
/* ========================================================================= */

typedef enum vitte_c17_module_state {
    VITTE_C17_MODULE_STATE_UNINITIALIZED = 0,

    VITTE_C17_MODULE_STATE_BUILDING,

    VITTE_C17_MODULE_STATE_SEALED,

    VITTE_C17_MODULE_STATE_EMITTING,

    VITTE_C17_MODULE_STATE_EMITTED,

    VITTE_C17_MODULE_STATE_FAILED,

    VITTE_C17_MODULE_STATE_DESTROYED,

    VITTE_C17_MODULE_STATE_COUNT
} vitte_c17_module_state_t;

/* ========================================================================= */
/* Module errors                                                             */
/* ========================================================================= */

typedef enum vitte_c17_module_error {
    VITTE_C17_MODULE_ERROR_NONE = 0,

    VITTE_C17_MODULE_ERROR_INVALID_MODULE,

    VITTE_C17_MODULE_ERROR_INVALID_ARGUMENT,

    VITTE_C17_MODULE_ERROR_INVALID_STATE,

    VITTE_C17_MODULE_ERROR_OVERFLOW,

    VITTE_C17_MODULE_ERROR_OUT_OF_MEMORY,

    VITTE_C17_MODULE_ERROR_DUPLICATE,

    VITTE_C17_MODULE_ERROR_CONFLICT,

    VITTE_C17_MODULE_ERROR_INVALID_SECTION,

    VITTE_C17_MODULE_ERROR_OUTPUT,

    VITTE_C17_MODULE_ERROR_CORRUPTION,

    VITTE_C17_MODULE_ERROR_COUNT
} vitte_c17_module_error_t;

/* ========================================================================= */
/* Include kind                                                              */
/* ========================================================================= */

typedef enum vitte_c17_include_kind {
    VITTE_C17_INCLUDE_INVALID = 0,

    /*
     * #include <...>
     */
    VITTE_C17_INCLUDE_SYSTEM,

    /*
     * #include "..."
     */
    VITTE_C17_INCLUDE_LOCAL,

    VITTE_C17_INCLUDE_COUNT
} vitte_c17_include_kind_t;

/* ========================================================================= */
/* Symbol kind                                                               */
/* ========================================================================= */

typedef enum vitte_c17_symbol_kind {
    VITTE_C17_SYMBOL_INVALID = 0,

    VITTE_C17_SYMBOL_TYPE,

    VITTE_C17_SYMBOL_GLOBAL,

    VITTE_C17_SYMBOL_FUNCTION,

    VITTE_C17_SYMBOL_RUNTIME,

    VITTE_C17_SYMBOL_INTERNAL,

    VITTE_C17_SYMBOL_COUNT
} vitte_c17_symbol_kind_t;

/* ========================================================================= */
/* Runtime requirements                                                      */
/* ========================================================================= */

/*
 * Runtime requirements are stored as bits inside module->runtime_requirements.
 *
 * Keep VITTE_C17_RUNTIME_COUNT <= 64 unless the representation is changed.
 */
typedef enum vitte_c17_runtime_requirement {
    VITTE_C17_RUNTIME_NONE = 0,

    VITTE_C17_RUNTIME_ALLOC,

    VITTE_C17_RUNTIME_STRING,

    VITTE_C17_RUNTIME_ARRAY,

    VITTE_C17_RUNTIME_SLICE,

    VITTE_C17_RUNTIME_MAP,

    VITTE_C17_RUNTIME_PANIC,

    VITTE_C17_RUNTIME_ASSERT,

    VITTE_C17_RUNTIME_IO,

    VITTE_C17_RUNTIME_MATH,

    VITTE_C17_RUNTIME_ASYNC,

    VITTE_C17_RUNTIME_COUNT
} vitte_c17_runtime_requirement_t;

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

struct vitte_c17_module_config {
    /*
     * Sort module collections before sealing.
     */
    bool deterministic;

    /*
     * Adding an existing dependency succeeds and reuses the existing entry.
     */
    bool deduplicate_dependencies;

    /*
     * Adding an identical include succeeds without creating another entry.
     */
    bool deduplicate_includes;

    /*
     * Reject conflicting generated symbol names.
     */
    bool reject_duplicate_symbols;

    /*
     * Reject fragments with identical section + contents.
     *
     * Usually disabled because independently lowered constructs may
     * legitimately produce identical C text.
     */
    bool reject_duplicate_fragments;

    /*
     * Open sections even when they contain no generated content.
     */
    bool emit_empty_sections;
};

/* ========================================================================= */
/* Dependency                                                                */
/* ========================================================================= */

struct vitte_c17_module_dependency {
    /*
     * Owned NUL-terminated module name.
     */
    char *name;

    size_t name_length;

    /*
     * True when this dependency forms part of the public module interface.
     */
    bool public_dependency;
};

/* ========================================================================= */
/* Include                                                                   */
/* ========================================================================= */

struct vitte_c17_module_include {
    vitte_c17_include_kind_t kind;

    /*
     * Owned NUL-terminated path without surrounding <> or "".
     */
    char *path;

    size_t path_length;
};

/* ========================================================================= */
/* Symbol                                                                    */
/* ========================================================================= */

struct vitte_c17_module_symbol {
    vitte_c17_symbol_kind_t kind;

    /*
     * Owned symbol name.
     *
     * This may represent either a final C symbol or a canonical backend
     * symbol depending on the lowering stage that registers it.
     */
    char *name;

    size_t name_length;

    /*
     * Whether the symbol belongs to the externally visible module ABI.
     */
    bool exported;

    /*
     * Stable ID supplied by the producer.
     *
     * Typically AST/HIR/IR declaration ID.
     *
     * Zero is permitted for synthetic backend symbols.
     */
    uint64_t source_id;
};

/* ========================================================================= */
/* Feature                                                                   */
/* ========================================================================= */

struct vitte_c17_module_feature {
    /*
     * Owned feature name.
     *
     * During emission this becomes:
     *
     *     VITTE_FEATURE_<NORMALIZED_NAME>
     */
    char *name;

    size_t name_length;
};

/* ========================================================================= */
/* Fragment                                                                  */
/* ========================================================================= */

struct vitte_c17_module_fragment {
    /*
     * Canonical destination section.
     */
    vitte_c17_section_t section;

    /*
     * Owned generated C17 text.
     *
     * NUL terminated for convenience, but length remains authoritative.
     */
    char *text;

    size_t length;

    /*
     * Stable AST/HIR/IR source identity.
     *
     * Used for deterministic ordering and future source-map integration.
     */
    uint64_t source_id;

    /*
     * Original insertion sequence.
     *
     * Used as deterministic tie breaker.
     */
    uint64_t sequence;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_c17_module_stats {
    size_t dependency_count;

    size_t include_count;

    size_t symbol_count;

    size_t feature_count;

    size_t fragment_count;

    /*
     * Total stored fragment payload bytes.
     */
    size_t fragment_bytes;

    /*
     * Successfully emitted fragments.
     */
    size_t emitted_fragments;

    /*
     * Source fragment bytes successfully submitted to backend emission.
     *
     * This does not include formatting bytes inserted by backend.c.
     */
    size_t emitted_fragment_bytes;
};

/* ========================================================================= */
/* Module                                                                    */
/* ========================================================================= */

struct vitte_c17_module {
    uint64_t magic;

    vitte_c17_module_state_t state;

    vitte_c17_module_error_t last_error;

    vitte_c17_module_config_t config;

    /* --------------------------------------------------------------------- */
    /* Identity                                                              */
    /* --------------------------------------------------------------------- */

    char *name;

    size_t name_length;

    /* --------------------------------------------------------------------- */
    /* Source                                                                */
    /* --------------------------------------------------------------------- */

    uint32_t source_file_id;

    char *source_path;

    size_t source_path_length;

    /* --------------------------------------------------------------------- */
    /* Dependencies                                                          */
    /* --------------------------------------------------------------------- */

    vitte_c17_module_dependency_t *dependencies;

    size_t dependency_count;

    size_t dependency_capacity;

    /* --------------------------------------------------------------------- */
    /* Includes                                                              */
    /* --------------------------------------------------------------------- */

    vitte_c17_module_include_t *includes;

    size_t include_count;

    size_t include_capacity;

    /* --------------------------------------------------------------------- */
    /* Symbols                                                               */
    /* --------------------------------------------------------------------- */

    vitte_c17_module_symbol_t *symbols;

    size_t symbol_count;

    size_t symbol_capacity;

    /* --------------------------------------------------------------------- */
    /* Features                                                              */
    /* --------------------------------------------------------------------- */

    vitte_c17_module_feature_t *features;

    size_t feature_count;

    size_t feature_capacity;

    /* --------------------------------------------------------------------- */
    /* Runtime                                                               */
    /* --------------------------------------------------------------------- */

    uint64_t runtime_requirements;

    /* --------------------------------------------------------------------- */
    /* Generated fragments                                                   */
    /* --------------------------------------------------------------------- */

    vitte_c17_module_fragment_t *fragments;

    size_t fragment_count;

    size_t fragment_capacity;

    size_t fragment_bytes;

    uint64_t next_fragment_sequence;

    /* --------------------------------------------------------------------- */
    /* Statistics                                                            */
    /* --------------------------------------------------------------------- */

    vitte_c17_module_stats_t stats;
};

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_module_error_name(
    vitte_c17_module_error_t error);

const char *
vitte_c17_module_state_name(
    vitte_c17_module_state_t state);

const char *
vitte_c17_include_kind_name(
    vitte_c17_include_kind_t kind);

const char *
vitte_c17_symbol_kind_name(
    vitte_c17_symbol_kind_t kind);

const char *
vitte_c17_runtime_requirement_name(
    vitte_c17_runtime_requirement_t requirement);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_c17_module_config_t
vitte_c17_module_config_default(void);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_c17_module_init(
    vitte_c17_module_t *module,
    const char *name,
    size_t name_length,
    const vitte_c17_module_config_t *config);

void
vitte_c17_module_destroy(
    vitte_c17_module_t *module);

/* ========================================================================= */
/* Validity                                                                  */
/* ========================================================================= */

bool
vitte_c17_module_is_valid(
    const vitte_c17_module_t *module);

bool
vitte_c17_module_validate(
    const vitte_c17_module_t *module);

/* ========================================================================= */
/* Source identity                                                           */
/* ========================================================================= */

bool
vitte_c17_module_set_source(
    vitte_c17_module_t *module,
    uint32_t file_id,
    const char *path,
    size_t path_length);

/* ========================================================================= */
/* Dependencies                                                              */
/* ========================================================================= */

bool
vitte_c17_module_add_dependency(
    vitte_c17_module_t *module,
    const char *name,
    size_t name_length,
    bool public_dependency);

const vitte_c17_module_dependency_t *
vitte_c17_module_find_dependency(
    const vitte_c17_module_t *module,
    const char *name,
    size_t name_length);

/* ========================================================================= */
/* Includes                                                                  */
/* ========================================================================= */

bool
vitte_c17_module_add_include(
    vitte_c17_module_t *module,
    vitte_c17_include_kind_t kind,
    const char *path,
    size_t path_length);

/* ========================================================================= */
/* Symbols                                                                   */
/* ========================================================================= */

bool
vitte_c17_module_add_symbol(
    vitte_c17_module_t *module,
    vitte_c17_symbol_kind_t kind,
    const char *name,
    size_t name_length,
    bool exported,
    uint64_t source_id);

const vitte_c17_module_symbol_t *
vitte_c17_module_find_symbol(
    const vitte_c17_module_t *module,
    const char *name,
    size_t name_length);

/* ========================================================================= */
/* Features                                                                  */
/* ========================================================================= */

bool
vitte_c17_module_require_feature(
    vitte_c17_module_t *module,
    const char *name,
    size_t name_length);

bool
vitte_c17_module_has_feature(
    const vitte_c17_module_t *module,
    const char *name,
    size_t name_length);

/* ========================================================================= */
/* Runtime requirements                                                      */
/* ========================================================================= */

bool
vitte_c17_module_require_runtime(
    vitte_c17_module_t *module,
    vitte_c17_runtime_requirement_t requirement);

bool
vitte_c17_module_requires_runtime(
    const vitte_c17_module_t *module,
    vitte_c17_runtime_requirement_t requirement);

/* ========================================================================= */
/* Fragments                                                                 */
/* ========================================================================= */

/*
 * Add owned generated C text to a canonical backend section.
 *
 * text is copied.
 *
 * source_id should identify the AST/HIR/IR construct that produced the
 * fragment. Zero is allowed for synthetic backend output.
 */
bool
vitte_c17_module_add_fragment(
    vitte_c17_module_t *module,
    vitte_c17_section_t section,
    const char *text,
    size_t length,
    uint64_t source_id);

size_t
vitte_c17_module_section_fragment_count(
    const vitte_c17_module_t *module,
    vitte_c17_section_t section);

/* ========================================================================= */
/* Sealing                                                                   */
/* ========================================================================= */

/*
 * Finalize module construction.
 *
 * In deterministic mode this sorts:
 *
 *   dependencies
 *   includes
 *   symbols
 *   features
 *   fragments
 *
 * After sealing, mutation APIs reject changes.
 */
bool
vitte_c17_module_seal(
    vitte_c17_module_t *module);

/* ========================================================================= */
/* Emission                                                                  */
/* ========================================================================= */

/*
 * Emit an already-begun backend generation.
 *
 * backend must be in:
 *
 *     VITTE_C17_BACKEND_STATE_GENERATING
 *
 * BUILDING modules are automatically sealed.
 */
bool
vitte_c17_module_emit(
    vitte_c17_module_t *module,
    vitte_c17_backend_t *backend);

/*
 * Adapter for:
 *
 *     vitte_c17_backend_generate()
 */
bool
vitte_c17_module_emit_adapter(
    vitte_c17_backend_t *backend,
    const void *translation_unit,
    void *user_data);

/*
 * Complete convenience driver:
 *
 *     backend_begin
 *       -> module_emit
 *     backend_finish
 */
bool
vitte_c17_module_generate(
    vitte_c17_module_t *module,
    vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Accessors                                                                 */
/* ========================================================================= */

const char *
vitte_c17_module_name(
    const vitte_c17_module_t *module);

size_t
vitte_c17_module_name_length(
    const vitte_c17_module_t *module);

const char *
vitte_c17_module_source_path(
    const vitte_c17_module_t *module);

uint32_t
vitte_c17_module_source_file_id(
    const vitte_c17_module_t *module);

vitte_c17_module_state_t
vitte_c17_module_state(
    const vitte_c17_module_t *module);

vitte_c17_module_error_t
vitte_c17_module_last_error(
    const vitte_c17_module_t *module);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_module_stats_t
vitte_c17_module_stats(
    const vitte_c17_module_t *module);

/* ========================================================================= */
/* Inline collection access                                                  */
/* ========================================================================= */

static inline size_t
vitte_c17_module_dependency_count(
    const vitte_c17_module_t *module)
{
    return
        module != NULL
            ? module->dependency_count
            : 0u;
}

static inline size_t
vitte_c17_module_include_count(
    const vitte_c17_module_t *module)
{
    return
        module != NULL
            ? module->include_count
            : 0u;
}

static inline size_t
vitte_c17_module_symbol_count(
    const vitte_c17_module_t *module)
{
    return
        module != NULL
            ? module->symbol_count
            : 0u;
}

static inline size_t
vitte_c17_module_feature_count(
    const vitte_c17_module_t *module)
{
    return
        module != NULL
            ? module->feature_count
            : 0u;
}

static inline size_t
vitte_c17_module_fragment_count(
    const vitte_c17_module_t *module)
{
    return
        module != NULL
            ? module->fragment_count
            : 0u;
}

/* ========================================================================= */
/* Indexed access                                                            */
/* ========================================================================= */

static inline const vitte_c17_module_dependency_t *
vitte_c17_module_dependency_at(
    const vitte_c17_module_t *module,
    size_t index)
{
    if (module == NULL ||
        index >= module->dependency_count) {
        return NULL;
    }

    return &module->dependencies[index];
}

static inline const vitte_c17_module_include_t *
vitte_c17_module_include_at(
    const vitte_c17_module_t *module,
    size_t index)
{
    if (module == NULL ||
        index >= module->include_count) {
        return NULL;
    }

    return &module->includes[index];
}

static inline const vitte_c17_module_symbol_t *
vitte_c17_module_symbol_at(
    const vitte_c17_module_t *module,
    size_t index)
{
    if (module == NULL ||
        index >= module->symbol_count) {
        return NULL;
    }

    return &module->symbols[index];
}

static inline const vitte_c17_module_feature_t *
vitte_c17_module_feature_at(
    const vitte_c17_module_t *module,
    size_t index)
{
    if (module == NULL ||
        index >= module->feature_count) {
        return NULL;
    }

    return &module->features[index];
}

static inline const vitte_c17_module_fragment_t *
vitte_c17_module_fragment_at(
    const vitte_c17_module_t *module,
    size_t index)
{
    if (module == NULL ||
        index >= module->fragment_count) {
        return NULL;
    }

    return &module->fragments[index];
}

/* ========================================================================= */
/* Inline state helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_c17_module_is_building(
    const vitte_c17_module_t *module)
{
    return
        module != NULL &&
        module->state ==
            VITTE_C17_MODULE_STATE_BUILDING;
}

static inline bool
vitte_c17_module_is_sealed(
    const vitte_c17_module_t *module)
{
    return
        module != NULL &&
        module->state ==
            VITTE_C17_MODULE_STATE_SEALED;
}

static inline bool
vitte_c17_module_is_emitting(
    const vitte_c17_module_t *module)
{
    return
        module != NULL &&
        module->state ==
            VITTE_C17_MODULE_STATE_EMITTING;
}

static inline bool
vitte_c17_module_is_emitted(
    const vitte_c17_module_t *module)
{
    return
        module != NULL &&
        module->state ==
            VITTE_C17_MODULE_STATE_EMITTED;
}

static inline bool
vitte_c17_module_has_failed(
    const vitte_c17_module_t *module)
{
    return
        module != NULL &&
        module->state ==
            VITTE_C17_MODULE_STATE_FAILED;
}

static inline bool
vitte_c17_module_has_error(
    const vitte_c17_module_t *module)
{
    return
        module != NULL &&
        module->last_error !=
            VITTE_C17_MODULE_ERROR_NONE;
}

/* ========================================================================= */
/* Runtime bit helpers                                                       */
/* ========================================================================= */

static inline uint64_t
vitte_c17_module_runtime_mask(
    const vitte_c17_module_t *module)
{
    return
        module != NULL
            ? module->runtime_requirements
            : UINT64_C(0);
}

static inline bool
vitte_c17_runtime_requirement_fits_mask(
    vitte_c17_runtime_requirement_t requirement)
{
    return
        requirement > VITTE_C17_RUNTIME_NONE &&
        requirement < VITTE_C17_RUNTIME_COUNT &&
        (unsigned)requirement < 64u;
}

/* ========================================================================= */
/* Convenience macros                                                        */
/* ========================================================================= */

#define VITTE_C17_MODULE_ADD_SYSTEM_INCLUDE(module, literal) \
    vitte_c17_module_add_include( \
        (module), \
        VITTE_C17_INCLUDE_SYSTEM, \
        (literal), \
        sizeof(literal) - 1u)

#define VITTE_C17_MODULE_ADD_LOCAL_INCLUDE(module, literal) \
    vitte_c17_module_add_include( \
        (module), \
        VITTE_C17_INCLUDE_LOCAL, \
        (literal), \
        sizeof(literal) - 1u)

#define VITTE_C17_MODULE_REQUIRE_FEATURE(module, literal) \
    vitte_c17_module_require_feature( \
        (module), \
        (literal), \
        sizeof(literal) - 1u)

#define VITTE_C17_MODULE_ADD_FRAGMENT(module, section, literal, source_id) \
    vitte_c17_module_add_fragment( \
        (module), \
        (section), \
        (literal), \
        sizeof(literal) - 1u, \
        (source_id))

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte C17 module requires 64-bit uint64_t");

_Static_assert(
    sizeof(uint32_t) == 4u,
    "Vitte C17 module requires 32-bit uint32_t");

_Static_assert(
    VITTE_C17_MODULE_STATE_UNINITIALIZED == 0,
    "module uninitialized state must remain zero");

_Static_assert(
    VITTE_C17_MODULE_ERROR_NONE == 0,
    "module no-error value must remain zero");

_Static_assert(
    VITTE_C17_INCLUDE_INVALID == 0,
    "invalid include kind must remain zero");

_Static_assert(
    VITTE_C17_SYMBOL_INVALID == 0,
    "invalid symbol kind must remain zero");

_Static_assert(
    VITTE_C17_RUNTIME_NONE == 0,
    "runtime none requirement must remain zero");

_Static_assert(
    VITTE_C17_RUNTIME_COUNT <= 64,
    "runtime requirement bitset supports at most 63 non-zero requirements");

_Static_assert(
    VITTE_C17_MODULE_INITIAL_CAPACITY != 0u,
    "module initial capacity must not be zero");

_Static_assert(
    VITTE_C17_MODULE_INITIAL_FRAGMENT_CAPACITY != 0u,
    "module initial fragment capacity must not be zero");

_Static_assert(
    VITTE_C17_MODULE_MAX_NAME_BYTES != 0u,
    "module maximum name size must not be zero");

_Static_assert(
    VITTE_C17_MODULE_MAX_PATH_BYTES != 0u,
    "module maximum path size must not be zero");

#endif

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_BACKEND_C17_MODULE_H */
