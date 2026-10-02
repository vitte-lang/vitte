#ifndef VITTE_SRC_BACKEND_C17_TRANSLATION_UNIT_H
#define VITTE_SRC_BACKEND_C17_TRANSLATION_UNIT_H

/*
 * Vitte Compiler
 * src/backend/c17/translation_unit.h
 *
 * Final ISO C17 translation-unit assembly layer.
 *
 * This layer sits between the whole-program model and the low-level C17
 * backend writer.
 *
 * Responsibilities:
 *
 *   - attach a whole-program model
 *   - finalize/validate the program
 *   - aggregate module includes
 *   - aggregate feature requirements
 *   - aggregate runtime requirements
 *   - aggregate generated C fragments
 *   - preserve dependency-safe module ordering
 *   - organize fragments by C17 output section
 *   - drive final backend emission
 *   - expose translation-unit statistics
 *   - expose stable fingerprints
 *   - validate aggregate integrity
 *
 * Architecture:
 *
 *     AST / HIR / IR
 *          |
 *          v
 *       module.c
 *          |
 *          v
 *      program.c
 *          |
 *          v
 *  translation_unit.c
 *          |
 *          v
 *      backend.c
 *          |
 *          v
 *    generated ISO C17
 *
 * Ownership:
 *
 *   vitte_c17_translation_unit_t owns:
 *
 *     - aggregate include descriptors
 *     - copied include paths
 *     - aggregate feature descriptors
 *     - copied feature names
 *     - aggregate fragment descriptors
 *
 *   vitte_c17_translation_unit_t does NOT own:
 *
 *     - vitte_c17_program_t
 *     - vitte_c17_backend_t
 *     - vitte_c17_module_t
 *     - fragment text
 *
 * Fragment text is borrowed from module.c and must remain alive until the
 * translation unit is reset/destroyed or no longer emitted.
 */

#include "backend.h"
#include "module.h"
#include "program.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration constants                                                   */
/* ========================================================================= */

#ifndef VITTE_C17_TRANSLATION_UNIT_MAGIC
#define VITTE_C17_TRANSLATION_UNIT_MAGIC \
    UINT64_C(0x564954544554554E)
#endif

#ifndef VITTE_C17_TRANSLATION_UNIT_DEAD_MAGIC
#define VITTE_C17_TRANSLATION_UNIT_DEAD_MAGIC \
    UINT64_C(0x4445414454554E21)
#endif

#ifndef VITTE_C17_TRANSLATION_UNIT_INITIAL_INCLUDE_CAPACITY
#define VITTE_C17_TRANSLATION_UNIT_INITIAL_INCLUDE_CAPACITY \
    ((size_t)16u)
#endif

#ifndef VITTE_C17_TRANSLATION_UNIT_INITIAL_FEATURE_CAPACITY
#define VITTE_C17_TRANSLATION_UNIT_INITIAL_FEATURE_CAPACITY \
    ((size_t)16u)
#endif

#ifndef VITTE_C17_TRANSLATION_UNIT_INITIAL_FRAGMENT_CAPACITY
#define VITTE_C17_TRANSLATION_UNIT_INITIAL_FRAGMENT_CAPACITY \
    ((size_t)64u)
#endif

#ifndef VITTE_C17_TRANSLATION_UNIT_MAX_INCLUDES
#define VITTE_C17_TRANSLATION_UNIT_MAX_INCLUDES \
    ((size_t)(1024u * 1024u))
#endif

#ifndef VITTE_C17_TRANSLATION_UNIT_MAX_FEATURES
#define VITTE_C17_TRANSLATION_UNIT_MAX_FEATURES \
    ((size_t)(1024u * 1024u))
#endif

#ifndef VITTE_C17_TRANSLATION_UNIT_MAX_FRAGMENTS
#define VITTE_C17_TRANSLATION_UNIT_MAX_FRAGMENTS \
    ((size_t)(16u * 1024u * 1024u))
#endif

#ifndef VITTE_C17_TRANSLATION_UNIT_HASH_OFFSET
#define VITTE_C17_TRANSLATION_UNIT_HASH_OFFSET \
    UINT64_C(14695981039346656037)
#endif

#ifndef VITTE_C17_TRANSLATION_UNIT_HASH_PRIME
#define VITTE_C17_TRANSLATION_UNIT_HASH_PRIME \
    UINT64_C(1099511628211)
#endif

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

typedef struct vitte_c17_translation_unit
    vitte_c17_translation_unit_t;

typedef struct vitte_c17_translation_unit_config
    vitte_c17_translation_unit_config_t;

typedef struct vitte_c17_translation_unit_include
    vitte_c17_translation_unit_include_t;

typedef struct vitte_c17_translation_unit_feature
    vitte_c17_translation_unit_feature_t;

typedef struct vitte_c17_translation_unit_fragment
    vitte_c17_translation_unit_fragment_t;

typedef struct vitte_c17_translation_unit_stats
    vitte_c17_translation_unit_stats_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_c17_translation_unit_error {
    VITTE_C17_TRANSLATION_UNIT_ERROR_NONE = 0,

    VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_UNIT,

    VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_ARGUMENT,

    VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_STATE,

    VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_PROGRAM,

    VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_MODULE,

    VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_BACKEND,

    VITTE_C17_TRANSLATION_UNIT_ERROR_DUPLICATE,

    VITTE_C17_TRANSLATION_UNIT_ERROR_LIMIT_EXCEEDED,

    VITTE_C17_TRANSLATION_UNIT_ERROR_OVERFLOW,

    VITTE_C17_TRANSLATION_UNIT_ERROR_OUT_OF_MEMORY,

    VITTE_C17_TRANSLATION_UNIT_ERROR_PROGRAM_FINALIZATION,

    VITTE_C17_TRANSLATION_UNIT_ERROR_MODULE_EMISSION,

    VITTE_C17_TRANSLATION_UNIT_ERROR_BACKEND,

    VITTE_C17_TRANSLATION_UNIT_ERROR_OUTPUT,

    VITTE_C17_TRANSLATION_UNIT_ERROR_CORRUPTION,

    VITTE_C17_TRANSLATION_UNIT_ERROR_COUNT
} vitte_c17_translation_unit_error_t;

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

typedef enum vitte_c17_translation_unit_state {
    VITTE_C17_TRANSLATION_UNIT_STATE_UNINITIALIZED = 0,

    /*
     * Program/backend may be attached.
     * Aggregates have not necessarily been prepared.
     */
    VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING,

    /*
     * Program finalized and aggregate data prepared.
     */
    VITTE_C17_TRANSLATION_UNIT_STATE_PREPARED,

    /*
     * Currently emitting through backend.c.
     */
    VITTE_C17_TRANSLATION_UNIT_STATE_EMITTING,

    /*
     * Successfully emitted.
     */
    VITTE_C17_TRANSLATION_UNIT_STATE_EMITTED,

    /*
     * An unrecovered preparation/emission error occurred.
     *
     * Explicit reset is required before reuse.
     */
    VITTE_C17_TRANSLATION_UNIT_STATE_FAILED,

    VITTE_C17_TRANSLATION_UNIT_STATE_DESTROYED,

    VITTE_C17_TRANSLATION_UNIT_STATE_COUNT
} vitte_c17_translation_unit_state_t;

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

struct vitte_c17_translation_unit_config {
    /*
     * Produce stable aggregate ordering.
     */
    bool deterministic;

    /*
     * Invoke vitte_c17_program_finalize() during prepare.
     */
    bool finalize_program;

    /*
     * Deep-validate program after finalization.
     */
    bool validate_program;

    /*
     * Validate every module while aggregating it.
     */
    bool validate_modules;

    /*
     * Aggregate module include requirements into one TU include registry.
     */
    bool aggregate_includes;

    /*
     * Aggregate module feature requirements.
     */
    bool aggregate_features;

    /*
     * Aggregate generated C fragments.
     */
    bool aggregate_fragments;

    /*
     * OR module runtime requirement masks into one TU mask.
     */
    bool aggregate_runtime_requirements;

    /*
     * Remove duplicate include requirements.
     */
    bool deduplicate_includes;

    /*
     * Remove duplicate feature requirements.
     */
    bool deduplicate_features;

    /*
     * Remove text-identical fragments in the same C17 section.
     *
     * Disabled by default because two identical textual fragments may still
     * represent intentionally distinct declarations or definitions.
     */
    bool deduplicate_fragments;

    /*
     * Emit comments marking transitions between Vitte modules.
     */
    bool emit_module_boundaries;

    /*
     * Open otherwise-empty backend sections.
     */
    bool emit_empty_sections;

    /*
     * Policy marker for final entry emission.
     *
     * ENTRY already appears after function definitions in canonical C17
     * section order. This flag is retained explicitly so translation-unit
     * policy can evolve without changing the public structure.
     */
    bool emit_entry_last;

    /*
     * Aggregate resource limits.
     */
    size_t max_includes;

    size_t max_features;

    size_t max_fragments;
};

/* ========================================================================= */
/* Aggregated include                                                        */
/* ========================================================================= */

struct vitte_c17_translation_unit_include {
    vitte_c17_include_kind_t kind;

    /*
     * Translation-unit-owned copy.
     */
    char *path;

    size_t path_length;

    /*
     * First module responsible for the requirement.
     *
     * When deduplication is enabled, later equivalent requirements do not
     * replace this value.
     */
    uint64_t module_id;
};

/* ========================================================================= */
/* Aggregated feature                                                        */
/* ========================================================================= */

struct vitte_c17_translation_unit_feature {
    /*
     * Translation-unit-owned copy.
     */
    char *name;

    size_t name_length;

    /*
     * First module responsible for the feature.
     */
    uint64_t module_id;
};

/* ========================================================================= */
/* Aggregated fragment                                                       */
/* ========================================================================= */

struct vitte_c17_translation_unit_fragment {
    /*
     * Module that owns the original fragment.
     */
    uint64_t module_id;

    /*
     * Final C17 output section.
     */
    vitte_c17_section_t section;

    /*
     * Borrowed immutable module-owned generated C text.
     */
    const char *text;

    size_t length;

    /*
     * Optional AST/HIR/IR/source identity propagated from module.c.
     *
     * Zero means unavailable.
     */
    uint64_t source_id;

    /*
     * Original fragment sequence inside its module.
     */
    uint64_t module_sequence;

    /*
     * Sequence assigned while aggregating modules in dependency-safe program
     * order.
     *
     * This is deliberately separate from module_id. Sorting numerically by
     * module_id could destroy program.c's topological ordering.
     */
    uint64_t global_sequence;
};

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

struct vitte_c17_translation_unit_stats {
    bool valid;

    /*
     * Number of modules represented by the prepared translation unit.
     */
    size_t module_count;

    /*
     * Aggregated requirements.
     */
    size_t include_count;

    size_t feature_count;

    size_t fragment_count;

    size_t fragment_bytes;

    uint64_t runtime_requirements;

    /*
     * Per-section aggregate information.
     *
     * Index is vitte_c17_section_t.
     */
    size_t fragments_by_section[
        VITTE_C17_SECTION_COUNT];

    size_t bytes_by_section[
        VITTE_C17_SECTION_COUNT];

    /*
     * Actual emission counters.
     */
    size_t emitted_fragment_count;

    size_t emitted_fragment_bytes;

    /*
     * Program fingerprint captured during preparation.
     */
    uint64_t program_hash;

    /*
     * Translation-unit fingerprint.
     *
     * Filled by vitte_c17_translation_unit_stats().
     */
    uint64_t hash;

    uint64_t generation;
};

/* ========================================================================= */
/* Translation unit                                                          */
/* ========================================================================= */

struct vitte_c17_translation_unit {
    uint64_t magic;

    vitte_c17_translation_unit_state_t state;

    vitte_c17_translation_unit_error_t last_error;

    vitte_c17_translation_unit_config_t config;

    /*
     * Non-owning whole-program model.
     */
    vitte_c17_program_t *program;

    /*
     * Non-owning active backend.
     *
     * May be NULL before emission.
     */
    vitte_c17_backend_t *backend;

    /* --------------------------------------------------------------------- */
    /* Includes                                                              */
    /* --------------------------------------------------------------------- */

    vitte_c17_translation_unit_include_t *includes;

    size_t include_count;

    size_t include_capacity;

    /* --------------------------------------------------------------------- */
    /* Features                                                              */
    /* --------------------------------------------------------------------- */

    vitte_c17_translation_unit_feature_t *features;

    size_t feature_count;

    size_t feature_capacity;

    /* --------------------------------------------------------------------- */
    /* Runtime requirements                                                  */
    /* --------------------------------------------------------------------- */

    uint64_t runtime_requirements;

    /* --------------------------------------------------------------------- */
    /* Fragments                                                             */
    /* --------------------------------------------------------------------- */

    vitte_c17_translation_unit_fragment_t *fragments;

    size_t fragment_count;

    size_t fragment_capacity;

    size_t fragment_bytes;

    /* --------------------------------------------------------------------- */
    /* Statistics                                                            */
    /* --------------------------------------------------------------------- */

    vitte_c17_translation_unit_stats_t stats;

    /* --------------------------------------------------------------------- */
    /* Lifecycle                                                             */
    /* --------------------------------------------------------------------- */

    uint64_t generation;
};

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_translation_unit_error_name(
    vitte_c17_translation_unit_error_t error);

const char *
vitte_c17_translation_unit_state_name(
    vitte_c17_translation_unit_state_t state);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_c17_translation_unit_config_t
vitte_c17_translation_unit_config_default(void);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_c17_translation_unit_init(
    vitte_c17_translation_unit_t *unit,
    const vitte_c17_translation_unit_config_t *config);

void
vitte_c17_translation_unit_reset(
    vitte_c17_translation_unit_t *unit);

void
vitte_c17_translation_unit_destroy(
    vitte_c17_translation_unit_t *unit);

/* ========================================================================= */
/* Structural validity                                                       */
/* ========================================================================= */

bool
vitte_c17_translation_unit_is_valid(
    const vitte_c17_translation_unit_t *unit);

/* ========================================================================= */
/* Attachments                                                               */
/* ========================================================================= */

bool
vitte_c17_translation_unit_set_program(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_program_t *program);

bool
vitte_c17_translation_unit_set_backend(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

bool
vitte_c17_translation_unit_reserve_includes(
    vitte_c17_translation_unit_t *unit,
    size_t capacity);

bool
vitte_c17_translation_unit_reserve_features(
    vitte_c17_translation_unit_t *unit,
    size_t capacity);

bool
vitte_c17_translation_unit_reserve_fragments(
    vitte_c17_translation_unit_t *unit,
    size_t capacity);

/* ========================================================================= */
/* Preparation                                                               */
/* ========================================================================= */

/*
 * Prepare the final translation unit.
 *
 * Depending on configuration:
 *
 *   1. finalize program
 *   2. validate program
 *   3. traverse dependency-safe module order
 *   4. validate modules
 *   5. aggregate includes
 *   6. aggregate features
 *   7. aggregate runtime requirements
 *   8. aggregate C fragments
 *   9. perform deterministic aggregate ordering
 *  10. compute aggregate statistics
 *
 * On success:
 *
 *     BUILDING -> PREPARED
 */
bool
vitte_c17_translation_unit_prepare(
    vitte_c17_translation_unit_t *unit);

/* ========================================================================= */
/* Emission                                                                  */
/* ========================================================================= */

/*
 * Emit into unit->backend.
 *
 * The backend must already be inside the generation lifecycle.
 *
 * This function does not call vitte_c17_backend_begin() or
 * vitte_c17_backend_finish().
 */
bool
vitte_c17_translation_unit_emit(
    vitte_c17_translation_unit_t *unit);

/*
 * Adapter matching vitte_c17_emit_translation_unit_fn.
 *
 * Intended for vitte_c17_backend_generate().
 */
bool
vitte_c17_translation_unit_emit_adapter(
    vitte_c17_backend_t *backend,
    const void *translation_unit,
    void *user_data);

/*
 * Complete generation convenience function.
 *
 * This uses backend_generate(), which owns the backend begin/finish
 * lifecycle.
 */
bool
vitte_c17_translation_unit_generate(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_backend_t *backend);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Stable non-cryptographic fingerprint of the prepared translation unit.
 *
 * Includes:
 *
 *   - program fingerprint
 *   - include requirements
 *   - feature requirements
 *   - runtime requirement mask
 *   - C17 fragment sections
 *   - module identities
 *   - generated fragment bytes
 *
 * Intended for:
 *
 *   - reproducibility checks
 *   - backend cache keys
 *   - incremental build metadata
 *   - diagnostics
 *
 * Not suitable for cryptographic use.
 */
uint64_t
vitte_c17_translation_unit_hash(
    const vitte_c17_translation_unit_t *unit);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_translation_unit_stats_t
vitte_c17_translation_unit_stats(
    const vitte_c17_translation_unit_t *unit);

/* ========================================================================= */
/* Deep validation                                                          */
/* ========================================================================= */

bool
vitte_c17_translation_unit_validate(
    const vitte_c17_translation_unit_t *unit);

/* ========================================================================= */
/* Accessors                                                                 */
/* ========================================================================= */

vitte_c17_translation_unit_state_t
vitte_c17_translation_unit_state(
    const vitte_c17_translation_unit_t *unit);

vitte_c17_translation_unit_error_t
vitte_c17_translation_unit_last_error(
    const vitte_c17_translation_unit_t *unit);

void
vitte_c17_translation_unit_clear_error(
    vitte_c17_translation_unit_t *unit);

size_t
vitte_c17_translation_unit_include_count(
    const vitte_c17_translation_unit_t *unit);

size_t
vitte_c17_translation_unit_feature_count(
    const vitte_c17_translation_unit_t *unit);

size_t
vitte_c17_translation_unit_fragment_count(
    const vitte_c17_translation_unit_t *unit);

uint64_t
vitte_c17_translation_unit_runtime_requirements(
    const vitte_c17_translation_unit_t *unit);

const vitte_c17_translation_unit_include_t *
vitte_c17_translation_unit_include_at(
    const vitte_c17_translation_unit_t *unit,
    size_t index);

const vitte_c17_translation_unit_feature_t *
vitte_c17_translation_unit_feature_at(
    const vitte_c17_translation_unit_t *unit,
    size_t index);

const vitte_c17_translation_unit_fragment_t *
vitte_c17_translation_unit_fragment_at(
    const vitte_c17_translation_unit_t *unit,
    size_t index);

/* ========================================================================= */
/* Inline state helpers                                                      */
/* ========================================================================= */

static inline bool
vitte_c17_translation_unit_is_building(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->state ==
            VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING;
}

static inline bool
vitte_c17_translation_unit_is_prepared(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->state ==
            VITTE_C17_TRANSLATION_UNIT_STATE_PREPARED;
}

static inline bool
vitte_c17_translation_unit_is_emitting(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->state ==
            VITTE_C17_TRANSLATION_UNIT_STATE_EMITTING;
}

static inline bool
vitte_c17_translation_unit_is_emitted(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->state ==
            VITTE_C17_TRANSLATION_UNIT_STATE_EMITTED;
}

static inline bool
vitte_c17_translation_unit_has_failed(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->state ==
            VITTE_C17_TRANSLATION_UNIT_STATE_FAILED;
}

static inline bool
vitte_c17_translation_unit_has_error(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->last_error !=
            VITTE_C17_TRANSLATION_UNIT_ERROR_NONE;
}

/* ========================================================================= */
/* Inline attachment helpers                                                 */
/* ========================================================================= */

static inline bool
vitte_c17_translation_unit_has_program(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->program != NULL;
}

static inline bool
vitte_c17_translation_unit_has_backend(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->backend != NULL;
}

static inline vitte_c17_program_t *
vitte_c17_translation_unit_program(
    vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC
            ? unit->program
            : NULL;
}

static inline const vitte_c17_program_t *
vitte_c17_translation_unit_program_const(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC
            ? unit->program
            : NULL;
}

static inline vitte_c17_backend_t *
vitte_c17_translation_unit_backend(
    vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC
            ? unit->backend
            : NULL;
}

/* ========================================================================= */
/* Inline aggregate helpers                                                  */
/* ========================================================================= */

static inline bool
vitte_c17_translation_unit_has_includes(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->include_count != 0u;
}

static inline bool
vitte_c17_translation_unit_has_features(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->feature_count != 0u;
}

static inline bool
vitte_c17_translation_unit_has_fragments(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->fragment_count != 0u;
}

static inline bool
vitte_c17_translation_unit_requires_runtime(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC &&
        unit->runtime_requirements != UINT64_C(0);
}

static inline bool
vitte_c17_translation_unit_requires_runtime_feature(
    const vitte_c17_translation_unit_t *unit,
    vitte_c17_runtime_requirement_t requirement)
{
    unsigned value;

    if (unit == NULL ||
        unit->magic !=
            VITTE_C17_TRANSLATION_UNIT_MAGIC) {
        return false;
    }

    value = (unsigned)requirement;

    if (value == 0u ||
        value >= 64u) {
        return false;
    }

    return
        (unit->runtime_requirements &
         (UINT64_C(1) << value)) != UINT64_C(0);
}

/* ========================================================================= */
/* Inline counters                                                           */
/* ========================================================================= */

static inline size_t
vitte_c17_translation_unit_fragment_bytes(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC
            ? unit->fragment_bytes
            : 0u;
}

static inline uint64_t
vitte_c17_translation_unit_generation(
    const vitte_c17_translation_unit_t *unit)
{
    return
        unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC
            ? unit->generation
            : UINT64_C(0);
}

/* ========================================================================= */
/* Section helpers                                                           */
/* ========================================================================= */

static inline size_t
vitte_c17_translation_unit_section_fragment_count(
    const vitte_c17_translation_unit_t *unit,
    vitte_c17_section_t section)
{
    if (unit == NULL ||
        unit->magic !=
            VITTE_C17_TRANSLATION_UNIT_MAGIC ||
        section <= VITTE_C17_SECTION_NONE ||
        section >= VITTE_C17_SECTION_COUNT) {
        return 0u;
    }

    return
        unit->stats.fragments_by_section[
            (size_t)section];
}

static inline size_t
vitte_c17_translation_unit_section_bytes(
    const vitte_c17_translation_unit_t *unit,
    vitte_c17_section_t section)
{
    if (unit == NULL ||
        unit->magic !=
            VITTE_C17_TRANSLATION_UNIT_MAGIC ||
        section <= VITTE_C17_SECTION_NONE ||
        section >= VITTE_C17_SECTION_COUNT) {
        return 0u;
    }

    return
        unit->stats.bytes_by_section[
            (size_t)section];
}

/* ========================================================================= */
/* Iteration macros                                                          */
/* ========================================================================= */

#define VITTE_C17_TRANSLATION_UNIT_FOR_EACH_INCLUDE(unit_ptr, index_name) \
    for ((index_name) = 0u; \
         (unit_ptr) != NULL && \
         (index_name) < (unit_ptr)->include_count; \
         ++(index_name))

#define VITTE_C17_TRANSLATION_UNIT_FOR_EACH_FEATURE(unit_ptr, index_name) \
    for ((index_name) = 0u; \
         (unit_ptr) != NULL && \
         (index_name) < (unit_ptr)->feature_count; \
         ++(index_name))

#define VITTE_C17_TRANSLATION_UNIT_FOR_EACH_FRAGMENT(unit_ptr, index_name) \
    for ((index_name) = 0u; \
         (unit_ptr) != NULL && \
         (index_name) < (unit_ptr)->fragment_count; \
         ++(index_name))

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

#if defined(__STDC_VERSION__) && \
    __STDC_VERSION__ >= 201112L

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte C17 translation unit requires 64-bit uint64_t");

_Static_assert(
    VITTE_C17_TRANSLATION_UNIT_ERROR_NONE == 0,
    "translation-unit no-error value must remain zero");

_Static_assert(
    VITTE_C17_TRANSLATION_UNIT_STATE_UNINITIALIZED == 0,
    "translation-unit uninitialized state must remain zero");

_Static_assert(
    VITTE_C17_TRANSLATION_UNIT_INITIAL_INCLUDE_CAPACITY != 0u,
    "initial include capacity must not be zero");

_Static_assert(
    VITTE_C17_TRANSLATION_UNIT_INITIAL_FEATURE_CAPACITY != 0u,
    "initial feature capacity must not be zero");

_Static_assert(
    VITTE_C17_TRANSLATION_UNIT_INITIAL_FRAGMENT_CAPACITY != 0u,
    "initial fragment capacity must not be zero");

_Static_assert(
    VITTE_C17_TRANSLATION_UNIT_INITIAL_INCLUDE_CAPACITY <=
        VITTE_C17_TRANSLATION_UNIT_MAX_INCLUDES,
    "initial include capacity exceeds maximum");

_Static_assert(
    VITTE_C17_TRANSLATION_UNIT_INITIAL_FEATURE_CAPACITY <=
        VITTE_C17_TRANSLATION_UNIT_MAX_FEATURES,
    "initial feature capacity exceeds maximum");

_Static_assert(
    VITTE_C17_TRANSLATION_UNIT_INITIAL_FRAGMENT_CAPACITY <=
        VITTE_C17_TRANSLATION_UNIT_MAX_FRAGMENTS,
    "initial fragment capacity exceeds maximum");

_Static_assert(
    VITTE_C17_SECTION_COUNT > 1,
    "C17 backend must expose output sections");

_Static_assert(
    VITTE_C17_RUNTIME_COUNT <= 64,
    "runtime requirement mask supports at most 63 non-zero requirements");

#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_SRC_BACKEND_C17_TRANSLATION_UNIT_H */
