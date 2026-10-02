/*
 * Vitte Compiler
 * src/backend/c17/translation_unit.c
 *
 * Final ISO C17 translation-unit assembly layer.
 *
 * Responsibilities:
 *
 *   - translation-unit lifecycle
 *   - program attachment
 *   - backend attachment
 *   - whole-program finalization
 *   - deterministic module ordering
 *   - include aggregation and deduplication
 *   - feature aggregation and deduplication
 *   - runtime-requirement aggregation
 *   - section-oriented fragment aggregation
 *   - entry-point coordination
 *   - source-map propagation
 *   - module-boundary emission
 *   - final translation-unit emission
 *   - statistics
 *   - validation
 *   - stable fingerprinting
 *
 * Architectural position:
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
 * This file intentionally does not perform expression/type/statement
 * lowering. Those responsibilities belong to their specialized backend
 * layers.
 */

#include "translation_unit.h"

#include "backend.h"
#include "module.h"
#include "program.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
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
/* Arithmetic                                                                */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_add_overflow(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (left > SIZE_MAX - right) {
        return true;
    }

    *result = left + right;
    return false;
}

static bool
vitte_c17_translation_unit_mul_overflow(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (left != 0u &&
        right > SIZE_MAX / left) {
        return true;
    }

    *result = left * right;
    return false;
}

static size_t
vitte_c17_translation_unit_saturating_add(
    size_t left,
    size_t right)
{
    size_t result;

    if (vitte_c17_translation_unit_add_overflow(
            left,
            right,
            &result)) {
        return SIZE_MAX;
    }

    return result;
}

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

static uint64_t
vitte_c17_translation_unit_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    if (data == NULL) {
        return hash;
    }

    bytes =
        (const unsigned char *)data;

    for (index = 0u;
         index < length;
         ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_C17_TRANSLATION_UNIT_HASH_PRIME;
    }

    return hash;
}

static uint64_t
vitte_c17_translation_unit_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned char bytes[8];
    size_t index;

    for (index = 0u;
         index < sizeof(bytes);
         ++index) {
        bytes[index] =
            (unsigned char)(
                (value >> (index * 8u)) &
                UINT64_C(0xff));
    }

    return
        vitte_c17_translation_unit_hash_bytes(
            hash,
            bytes,
            sizeof(bytes));
}

static uint64_t
vitte_c17_translation_unit_hash_size(
    uint64_t hash,
    size_t value)
{
    return
        vitte_c17_translation_unit_hash_u64(
            hash,
            (uint64_t)value);
}

static uint64_t
vitte_c17_translation_unit_hash_bool(
    uint64_t hash,
    bool value)
{
    const unsigned char byte =
        value
            ? (unsigned char)1u
            : (unsigned char)0u;

    return
        vitte_c17_translation_unit_hash_bytes(
            hash,
            &byte,
            1u);
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_translation_unit_error_name(
    vitte_c17_translation_unit_error_t error)
{
    switch (error) {
        case VITTE_C17_TRANSLATION_UNIT_ERROR_NONE:
            return "none";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_UNIT:
            return "invalid-unit";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_STATE:
            return "invalid-state";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_PROGRAM:
            return "invalid-program";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_MODULE:
            return "invalid-module";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_BACKEND:
            return "invalid-backend";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_DUPLICATE:
            return "duplicate";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_LIMIT_EXCEEDED:
            return "limit-exceeded";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_PROGRAM_FINALIZATION:
            return "program-finalization";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_MODULE_EMISSION:
            return "module-emission";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_BACKEND:
            return "backend";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_OUTPUT:
            return "output";

        case VITTE_C17_TRANSLATION_UNIT_ERROR_CORRUPTION:
            return "corruption";

        default:
            return "unknown";
    }
}

const char *
vitte_c17_translation_unit_state_name(
    vitte_c17_translation_unit_state_t state)
{
    switch (state) {
        case VITTE_C17_TRANSLATION_UNIT_STATE_UNINITIALIZED:
            return "uninitialized";

        case VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING:
            return "building";

        case VITTE_C17_TRANSLATION_UNIT_STATE_PREPARED:
            return "prepared";

        case VITTE_C17_TRANSLATION_UNIT_STATE_EMITTING:
            return "emitting";

        case VITTE_C17_TRANSLATION_UNIT_STATE_EMITTED:
            return "emitted";

        case VITTE_C17_TRANSLATION_UNIT_STATE_FAILED:
            return "failed";

        case VITTE_C17_TRANSLATION_UNIT_STATE_DESTROYED:
            return "destroyed";

        default:
            return "invalid";
    }
}

/* ========================================================================= */
/* Failure helper                                                            */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_fail(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_translation_unit_error_t error)
{
    if (unit != NULL &&
        unit->magic ==
            VITTE_C17_TRANSLATION_UNIT_MAGIC) {
        unit->last_error = error;

        if (unit->state !=
            VITTE_C17_TRANSLATION_UNIT_STATE_DESTROYED) {
            unit->state =
                VITTE_C17_TRANSLATION_UNIT_STATE_FAILED;
        }
    }

    return false;
}

/* ========================================================================= */
/* Validity                                                                  */
/* ========================================================================= */

bool
vitte_c17_translation_unit_is_valid(
    const vitte_c17_translation_unit_t *unit)
{
    if (unit == NULL) {
        return false;
    }

    if (unit->magic !=
        VITTE_C17_TRANSLATION_UNIT_MAGIC) {
        return false;
    }

    if (unit->state <=
            VITTE_C17_TRANSLATION_UNIT_STATE_UNINITIALIZED ||
        unit->state >=
            VITTE_C17_TRANSLATION_UNIT_STATE_DESTROYED) {
        return false;
    }

    if (unit->include_count >
        unit->include_capacity) {
        return false;
    }

    if (unit->feature_count >
        unit->feature_capacity) {
        return false;
    }

    if (unit->fragment_count >
        unit->fragment_capacity) {
        return false;
    }

    if (unit->include_count != 0u &&
        unit->includes == NULL) {
        return false;
    }

    if (unit->feature_count != 0u &&
        unit->features == NULL) {
        return false;
    }

    if (unit->fragment_count != 0u &&
        unit->fragments == NULL) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_c17_translation_unit_config_t
vitte_c17_translation_unit_config_default(void)
{
    vitte_c17_translation_unit_config_t config;

    memset(
        &config,
        0,
        sizeof(config));

    config.deterministic = true;

    config.finalize_program = true;
    config.validate_program = true;
    config.validate_modules = true;

    config.aggregate_includes = true;
    config.aggregate_features = true;
    config.aggregate_fragments = true;
    config.aggregate_runtime_requirements = true;

    config.deduplicate_includes = true;
    config.deduplicate_features = true;
    config.deduplicate_fragments = false;

    config.emit_module_boundaries = true;
    config.emit_empty_sections = false;
    config.emit_entry_last = true;

    config.max_includes =
        VITTE_C17_TRANSLATION_UNIT_MAX_INCLUDES;

    config.max_features =
        VITTE_C17_TRANSLATION_UNIT_MAX_FEATURES;

    config.max_fragments =
        VITTE_C17_TRANSLATION_UNIT_MAX_FRAGMENTS;

    return config;
}

static bool
vitte_c17_translation_unit_config_valid(
    const vitte_c17_translation_unit_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->max_includes == 0u ||
        config->max_includes >
            VITTE_C17_TRANSLATION_UNIT_MAX_INCLUDES) {
        return false;
    }

    if (config->max_features == 0u ||
        config->max_features >
            VITTE_C17_TRANSLATION_UNIT_MAX_FEATURES) {
        return false;
    }

    if (config->max_fragments == 0u ||
        config->max_fragments >
            VITTE_C17_TRANSLATION_UNIT_MAX_FRAGMENTS) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_c17_translation_unit_init(
    vitte_c17_translation_unit_t *unit,
    const vitte_c17_translation_unit_config_t *config)
{
    vitte_c17_translation_unit_config_t effective;

    if (unit == NULL) {
        return false;
    }

    effective =
        config != NULL
            ? *config
            : vitte_c17_translation_unit_config_default();

    if (!vitte_c17_translation_unit_config_valid(
            &effective)) {
        return false;
    }

    memset(
        unit,
        0,
        sizeof(*unit));

    unit->magic =
        VITTE_C17_TRANSLATION_UNIT_MAGIC;

    unit->state =
        VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING;

    unit->last_error =
        VITTE_C17_TRANSLATION_UNIT_ERROR_NONE;

    unit->config = effective;

    unit->generation =
        UINT64_C(1);

    return true;
}

/* ========================================================================= */
/* Entry cleanup                                                             */
/* ========================================================================= */

static void
vitte_c17_translation_unit_destroy_include(
    vitte_c17_translation_unit_include_t *include)
{
    if (include == NULL) {
        return;
    }

    free(include->path);

    memset(
        include,
        0,
        sizeof(*include));
}

static void
vitte_c17_translation_unit_destroy_feature(
    vitte_c17_translation_unit_feature_t *feature)
{
    if (feature == NULL) {
        return;
    }

    free(feature->name);

    memset(
        feature,
        0,
        sizeof(*feature));
}

static void
vitte_c17_translation_unit_destroy_fragment(
    vitte_c17_translation_unit_fragment_t *fragment)
{
    if (fragment == NULL) {
        return;
    }

    /*
     * Fragment text is borrowed from module.c.
     *
     * translation_unit.c owns only the aggregate descriptor.
     */
    memset(
        fragment,
        0,
        sizeof(*fragment));
}

/* ========================================================================= */
/* Clear aggregates                                                          */
/* ========================================================================= */

static void
vitte_c17_translation_unit_clear_aggregates(
    vitte_c17_translation_unit_t *unit)
{
    size_t index;

    for (index = 0u;
         index < unit->include_count;
         ++index) {
        vitte_c17_translation_unit_destroy_include(
            &unit->includes[index]);
    }

    for (index = 0u;
         index < unit->feature_count;
         ++index) {
        vitte_c17_translation_unit_destroy_feature(
            &unit->features[index]);
    }

    for (index = 0u;
         index < unit->fragment_count;
         ++index) {
        vitte_c17_translation_unit_destroy_fragment(
            &unit->fragments[index]);
    }

    unit->include_count = 0u;
    unit->feature_count = 0u;
    unit->fragment_count = 0u;

    unit->fragment_bytes = 0u;
    unit->runtime_requirements = UINT64_C(0);

    memset(
        &unit->stats,
        0,
        sizeof(unit->stats));
}

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

void
vitte_c17_translation_unit_reset(
    vitte_c17_translation_unit_t *unit)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return;
    }

    vitte_c17_translation_unit_clear_aggregates(
        unit);

    unit->program = NULL;
    unit->backend = NULL;

    unit->state =
        VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING;

    unit->last_error =
        VITTE_C17_TRANSLATION_UNIT_ERROR_NONE;

    if (unit->generation != UINT64_MAX) {
        ++unit->generation;
    }
}

/* ========================================================================= */
/* Destroy                                                                   */
/* ========================================================================= */

void
vitte_c17_translation_unit_destroy(
    vitte_c17_translation_unit_t *unit)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return;
    }

    vitte_c17_translation_unit_clear_aggregates(
        unit);

    free(unit->includes);
    free(unit->features);
    free(unit->fragments);

    memset(
        unit,
        0,
        sizeof(*unit));

    unit->magic =
        VITTE_C17_TRANSLATION_UNIT_DEAD_MAGIC;

    unit->state =
        VITTE_C17_TRANSLATION_UNIT_STATE_DESTROYED;
}

/* ========================================================================= */
/* Attach program/backend                                                    */
/* ========================================================================= */

bool
vitte_c17_translation_unit_set_program(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_program_t *program)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return false;
    }

    if (unit->state !=
        VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_STATE);
    }

    if (program == NULL ||
        !vitte_c17_program_is_valid(program)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_PROGRAM);
    }

    unit->program = program;

    unit->last_error =
        VITTE_C17_TRANSLATION_UNIT_ERROR_NONE;

    return true;
}

bool
vitte_c17_translation_unit_set_backend(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_backend_t *backend)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return false;
    }

    if (unit->state !=
        VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_STATE);
    }

    if (backend == NULL ||
        !vitte_c17_backend_is_valid(backend)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_BACKEND);
    }

    unit->backend = backend;

    unit->last_error =
        VITTE_C17_TRANSLATION_UNIT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Generic reserve                                                           */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_reserve_raw(
    vitte_c17_translation_unit_t *unit,
    void **storage,
    size_t *capacity,
    size_t minimum,
    size_t initial,
    size_t maximum,
    size_t element_size)
{
    size_t new_capacity;
    size_t bytes;
    void *replacement;

    if (storage == NULL ||
        capacity == NULL ||
        element_size == 0u) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_ARGUMENT);
    }

    if (minimum <= *capacity) {
        return true;
    }

    if (minimum > maximum) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_LIMIT_EXCEEDED);
    }

    new_capacity =
        *capacity != 0u
            ? *capacity
            : initial;

    if (new_capacity == 0u) {
        new_capacity = 1u;
    }

    while (new_capacity < minimum) {
        if (new_capacity > maximum / 2u) {
            new_capacity = maximum;
            break;
        }

        new_capacity *= 2u;
    }

    if (new_capacity < minimum) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_LIMIT_EXCEEDED);
    }

    if (vitte_c17_translation_unit_mul_overflow(
            new_capacity,
            element_size,
            &bytes)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_OVERFLOW);
    }

    replacement =
        realloc(
            *storage,
            bytes);

    if (replacement == NULL) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_OUT_OF_MEMORY);
    }

    *storage = replacement;
    *capacity = new_capacity;

    return true;
}

/* ========================================================================= */
/* Reserve                                                                   */
/* ========================================================================= */

bool
vitte_c17_translation_unit_reserve_includes(
    vitte_c17_translation_unit_t *unit,
    size_t capacity)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return false;
    }

    return
        vitte_c17_translation_unit_reserve_raw(
            unit,
            (void **)&unit->includes,
            &unit->include_capacity,
            capacity,
            VITTE_C17_TRANSLATION_UNIT_INITIAL_INCLUDE_CAPACITY,
            unit->config.max_includes,
            sizeof(unit->includes[0]));
}

bool
vitte_c17_translation_unit_reserve_features(
    vitte_c17_translation_unit_t *unit,
    size_t capacity)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return false;
    }

    return
        vitte_c17_translation_unit_reserve_raw(
            unit,
            (void **)&unit->features,
            &unit->feature_capacity,
            capacity,
            VITTE_C17_TRANSLATION_UNIT_INITIAL_FEATURE_CAPACITY,
            unit->config.max_features,
            sizeof(unit->features[0]));
}

bool
vitte_c17_translation_unit_reserve_fragments(
    vitte_c17_translation_unit_t *unit,
    size_t capacity)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return false;
    }

    return
        vitte_c17_translation_unit_reserve_raw(
            unit,
            (void **)&unit->fragments,
            &unit->fragment_capacity,
            capacity,
            VITTE_C17_TRANSLATION_UNIT_INITIAL_FRAGMENT_CAPACITY,
            unit->config.max_fragments,
            sizeof(unit->fragments[0]));
}

/* ========================================================================= */
/* String copy                                                               */
/* ========================================================================= */

static char *
vitte_c17_translation_unit_copy_string(
    const char *text,
    size_t length)
{
    char *copy;
    size_t bytes;

    if (text == NULL) {
        return NULL;
    }

    if (vitte_c17_translation_unit_add_overflow(
            length,
            1u,
            &bytes)) {
        return NULL;
    }

    copy =
        (char *)malloc(bytes);

    if (copy == NULL) {
        return NULL;
    }

    if (length != 0u) {
        memcpy(
            copy,
            text,
            length);
    }

    copy[length] = '\0';

    return copy;
}

/* ========================================================================= */
/* Include lookup                                                            */
/* ========================================================================= */

static ptrdiff_t
vitte_c17_translation_unit_find_include(
    const vitte_c17_translation_unit_t *unit,
    vitte_c17_include_kind_t kind,
    const char *path,
    size_t length)
{
    size_t index;

    for (index = 0u;
         index < unit->include_count;
         ++index) {
        const vitte_c17_translation_unit_include_t *include;

        include =
            &unit->includes[index];

        if (include->kind != kind ||
            include->path_length != length) {
            continue;
        }

        if (length == 0u ||
            memcmp(
                include->path,
                path,
                length) == 0) {
            return (ptrdiff_t)index;
        }
    }

    return -1;
}

/* ========================================================================= */
/* Add include                                                               */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_add_include(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_include_kind_t kind,
    const char *path,
    size_t length,
    uint64_t module_id)
{
    vitte_c17_translation_unit_include_t *include;
    char *copy;

    if (path == NULL ||
        length == 0u) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_ARGUMENT);
    }

    if (unit->config.deduplicate_includes &&
        vitte_c17_translation_unit_find_include(
            unit,
            kind,
            path,
            length) >= 0) {
        return true;
    }

    if (unit->include_count >=
        unit->config.max_includes) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_LIMIT_EXCEEDED);
    }

    if (!vitte_c17_translation_unit_reserve_includes(
            unit,
            unit->include_count + 1u)) {
        return false;
    }

    copy =
        vitte_c17_translation_unit_copy_string(
            path,
            length);

    if (copy == NULL) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_OUT_OF_MEMORY);
    }

    include =
        &unit->includes[
            unit->include_count];

    memset(
        include,
        0,
        sizeof(*include));

    include->kind = kind;
    include->path = copy;
    include->path_length = length;
    include->module_id = module_id;

    ++unit->include_count;

    return true;
}

/* ========================================================================= */
/* Feature lookup                                                            */
/* ========================================================================= */

static ptrdiff_t
vitte_c17_translation_unit_find_feature(
    const vitte_c17_translation_unit_t *unit,
    const char *name,
    size_t length)
{
    size_t index;

    for (index = 0u;
         index < unit->feature_count;
         ++index) {
        const vitte_c17_translation_unit_feature_t *feature;

        feature =
            &unit->features[index];

        if (feature->name_length != length) {
            continue;
        }

        if (length == 0u ||
            memcmp(
                feature->name,
                name,
                length) == 0) {
            return (ptrdiff_t)index;
        }
    }

    return -1;
}

/* ========================================================================= */
/* Add feature                                                               */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_add_feature(
    vitte_c17_translation_unit_t *unit,
    const char *name,
    size_t length,
    uint64_t module_id)
{
    vitte_c17_translation_unit_feature_t *feature;
    char *copy;

    if (name == NULL ||
        length == 0u) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_ARGUMENT);
    }

    if (unit->config.deduplicate_features &&
        vitte_c17_translation_unit_find_feature(
            unit,
            name,
            length) >= 0) {
        return true;
    }

    if (unit->feature_count >=
        unit->config.max_features) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_LIMIT_EXCEEDED);
    }

    if (!vitte_c17_translation_unit_reserve_features(
            unit,
            unit->feature_count + 1u)) {
        return false;
    }

    copy =
        vitte_c17_translation_unit_copy_string(
            name,
            length);

    if (copy == NULL) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_OUT_OF_MEMORY);
    }

    feature =
        &unit->features[
            unit->feature_count];

    memset(
        feature,
        0,
        sizeof(*feature));

    feature->name = copy;
    feature->name_length = length;
    feature->module_id = module_id;

    ++unit->feature_count;

    return true;
}

/* ========================================================================= */
/* Fragment equality                                                         */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_fragment_equal(
    const vitte_c17_translation_unit_fragment_t *left,
    vitte_c17_section_t section,
    const char *text,
    size_t length)
{
    if (left->section != section ||
        left->length != length) {
        return false;
    }

    if (length == 0u) {
        return true;
    }

    if (left->text == NULL ||
        text == NULL) {
        return false;
    }

    return
        memcmp(
            left->text,
            text,
            length) == 0;
}

/* ========================================================================= */
/* Add fragment                                                              */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_add_fragment(
    vitte_c17_translation_unit_t *unit,
    uint64_t module_id,
    const vitte_c17_module_fragment_t *source)
{
    vitte_c17_translation_unit_fragment_t *fragment;
    size_t index;

    if (source == NULL ||
        source->section <= VITTE_C17_SECTION_NONE ||
        source->section >= VITTE_C17_SECTION_COUNT ||
        source->text == NULL) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_ARGUMENT);
    }

    if (unit->config.deduplicate_fragments) {
        for (index = 0u;
             index < unit->fragment_count;
             ++index) {
            if (vitte_c17_translation_unit_fragment_equal(
                    &unit->fragments[index],
                    source->section,
                    source->text,
                    source->length)) {
                return true;
            }
        }
    }

    if (unit->fragment_count >=
        unit->config.max_fragments) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_LIMIT_EXCEEDED);
    }

    if (!vitte_c17_translation_unit_reserve_fragments(
            unit,
            unit->fragment_count + 1u)) {
        return false;
    }

    fragment =
        &unit->fragments[
            unit->fragment_count];

    memset(
        fragment,
        0,
        sizeof(*fragment));

    fragment->module_id = module_id;

    fragment->section =
        source->section;

    /*
     * Borrow module-owned immutable generated text.
     */
    fragment->text =
        source->text;

    fragment->length =
        source->length;

    fragment->source_id =
        source->source_id;

    fragment->module_sequence =
        source->sequence;

    fragment->global_sequence =
        (uint64_t)unit->fragment_count;

    unit->fragment_bytes =
        vitte_c17_translation_unit_saturating_add(
            unit->fragment_bytes,
            source->length);

    ++unit->fragment_count;

    return true;
}

/* ========================================================================= */
/* Aggregate module                                                          */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_aggregate_module(
    vitte_c17_translation_unit_t *unit,
    const vitte_c17_program_module_t *program_module)
{
    const vitte_c17_module_t *module;
    size_t index;

    if (program_module == NULL ||
        program_module->module == NULL) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_MODULE);
    }

    module =
        program_module->module;

    if (unit->config.validate_modules &&
        !vitte_c17_module_validate(module)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_MODULE);
    }

    if (unit->config.aggregate_includes) {
        for (index = 0u;
             index < module->include_count;
             ++index) {
            const vitte_c17_module_include_t *include;

            include =
                &module->includes[index];

            if (!vitte_c17_translation_unit_add_include(
                    unit,
                    include->kind,
                    include->path,
                    include->path_length,
                    program_module->id)) {
                return false;
            }
        }
    }

    if (unit->config.aggregate_features) {
        for (index = 0u;
             index < module->feature_count;
             ++index) {
            const vitte_c17_module_feature_t *feature;

            feature =
                &module->features[index];

            if (!vitte_c17_translation_unit_add_feature(
                    unit,
                    feature->name,
                    feature->name_length,
                    program_module->id)) {
                return false;
            }
        }
    }

    if (unit->config.aggregate_runtime_requirements) {
        unit->runtime_requirements |=
            module->runtime_requirements;
    }

    if (unit->config.aggregate_fragments) {
        for (index = 0u;
             index < module->fragment_count;
             ++index) {
            if (!vitte_c17_translation_unit_add_fragment(
                    unit,
                    program_module->id,
                    &module->fragments[index])) {
                return false;
            }
        }
    }

    return true;
}

/* ========================================================================= */
/* Aggregate program                                                         */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_aggregate_program(
    vitte_c17_translation_unit_t *unit)
{
    size_t index;

    vitte_c17_translation_unit_clear_aggregates(
        unit);

    if (unit->program == NULL) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_PROGRAM);
    }

    /*
     * Prefer finalized dependency-safe order.
     */
    if (unit->program->ordered_module_count != 0u) {
        for (index = 0u;
             index <
                unit->program->ordered_module_count;
             ++index) {
            const vitte_c17_program_module_t *module;

            module =
                vitte_c17_program_ordered_module_at(
                    unit->program,
                    index);

            if (module == NULL) {
                return
                    vitte_c17_translation_unit_fail(
                        unit,
                        VITTE_C17_TRANSLATION_UNIT_ERROR_CORRUPTION);
            }

            if (!vitte_c17_translation_unit_aggregate_module(
                    unit,
                    module)) {
                return false;
            }
        }
    } else {
        for (index = 0u;
             index < unit->program->module_count;
             ++index) {
            const vitte_c17_program_module_t *module;

            module =
                &unit->program->modules[index];

            if (!unit->program->config.include_unreachable_modules &&
                unit->program->has_entry &&
                !module->reachable) {
                continue;
            }

            if (!vitte_c17_translation_unit_aggregate_module(
                    unit,
                    module)) {
                return false;
            }
        }
    }

    return true;
}

/* ========================================================================= */
/* Ordering                                                                  */
/* ========================================================================= */

static int
vitte_c17_translation_unit_compare_include(
    const void *left_pointer,
    const void *right_pointer)
{
    const vitte_c17_translation_unit_include_t *left;
    const vitte_c17_translation_unit_include_t *right;
    size_t common;
    int comparison;

    left =
        (const vitte_c17_translation_unit_include_t *)
            left_pointer;

    right =
        (const vitte_c17_translation_unit_include_t *)
            right_pointer;

    if (left->kind < right->kind) {
        return -1;
    }

    if (left->kind > right->kind) {
        return 1;
    }

    common =
        left->path_length <
            right->path_length
            ? left->path_length
            : right->path_length;

    comparison =
        common != 0u
            ? memcmp(
                left->path,
                right->path,
                common)
            : 0;

    if (comparison != 0) {
        return comparison;
    }

    if (left->path_length <
        right->path_length) {
        return -1;
    }

    if (left->path_length >
        right->path_length) {
        return 1;
    }

    if (left->module_id <
        right->module_id) {
        return -1;
    }

    if (left->module_id >
        right->module_id) {
        return 1;
    }

    return 0;
}

static int
vitte_c17_translation_unit_compare_feature(
    const void *left_pointer,
    const void *right_pointer)
{
    const vitte_c17_translation_unit_feature_t *left;
    const vitte_c17_translation_unit_feature_t *right;
    size_t common;
    int comparison;

    left =
        (const vitte_c17_translation_unit_feature_t *)
            left_pointer;

    right =
        (const vitte_c17_translation_unit_feature_t *)
            right_pointer;

    common =
        left->name_length <
            right->name_length
            ? left->name_length
            : right->name_length;

    comparison =
        common != 0u
            ? memcmp(
                left->name,
                right->name,
                common)
            : 0;

    if (comparison != 0) {
        return comparison;
    }

    if (left->name_length <
        right->name_length) {
        return -1;
    }

    if (left->name_length >
        right->name_length) {
        return 1;
    }

    if (left->module_id <
        right->module_id) {
        return -1;
    }

    if (left->module_id >
        right->module_id) {
        return 1;
    }

    return 0;
}

static int
vitte_c17_translation_unit_compare_fragment(
    const void *left_pointer,
    const void *right_pointer)
{
    const vitte_c17_translation_unit_fragment_t *left;
    const vitte_c17_translation_unit_fragment_t *right;

    left =
        (const vitte_c17_translation_unit_fragment_t *)
            left_pointer;

    right =
        (const vitte_c17_translation_unit_fragment_t *)
            right_pointer;

    if (left->section < right->section) {
        return -1;
    }

    if (left->section > right->section) {
        return 1;
    }

    /*
     * Preserve dependency-safe module aggregation order.
     *
     * global_sequence is assigned after program.c has produced its module
     * order, so sorting by it is stable and does not incorrectly sort module
     * IDs numerically.
     */
    if (left->global_sequence <
        right->global_sequence) {
        return -1;
    }

    if (left->global_sequence >
        right->global_sequence) {
        return 1;
    }

    return 0;
}

static void
vitte_c17_translation_unit_sort(
    vitte_c17_translation_unit_t *unit)
{
    if (!unit->config.deterministic) {
        return;
    }

    if (unit->include_count > 1u) {
        qsort(
            unit->includes,
            unit->include_count,
            sizeof(unit->includes[0]),
            vitte_c17_translation_unit_compare_include);
    }

    if (unit->feature_count > 1u) {
        qsort(
            unit->features,
            unit->feature_count,
            sizeof(unit->features[0]),
            vitte_c17_translation_unit_compare_feature);
    }

    if (unit->fragment_count > 1u) {
        qsort(
            unit->fragments,
            unit->fragment_count,
            sizeof(unit->fragments[0]),
            vitte_c17_translation_unit_compare_fragment);
    }
}

/* ========================================================================= */
/* Statistics refresh                                                        */
/* ========================================================================= */

static void
vitte_c17_translation_unit_refresh_stats(
    vitte_c17_translation_unit_t *unit)
{
    size_t index;

    memset(
        &unit->stats,
        0,
        sizeof(unit->stats));

    unit->stats.valid = true;

    if (unit->program != NULL) {
        unit->stats.module_count =
            unit->program->ordered_module_count != 0u
                ? unit->program->ordered_module_count
                : unit->program->module_count;

        unit->stats.program_hash =
            vitte_c17_program_hash(
                unit->program);
    }

    unit->stats.include_count =
        unit->include_count;

    unit->stats.feature_count =
        unit->feature_count;

    unit->stats.fragment_count =
        unit->fragment_count;

    unit->stats.fragment_bytes =
        unit->fragment_bytes;

    unit->stats.runtime_requirements =
        unit->runtime_requirements;

    unit->stats.generation =
        unit->generation;

    for (index = 0u;
         index < unit->fragment_count;
         ++index) {
        const vitte_c17_translation_unit_fragment_t *fragment;

        fragment =
            &unit->fragments[index];

        if (fragment->section >
                VITTE_C17_SECTION_NONE &&
            fragment->section <
                VITTE_C17_SECTION_COUNT) {
            ++unit->stats.fragments_by_section[
                (size_t)fragment->section];

            unit->stats.bytes_by_section[
                (size_t)fragment->section] =
                vitte_c17_translation_unit_saturating_add(
                    unit->stats.bytes_by_section[
                        (size_t)fragment->section],
                    fragment->length);
        }
    }
}

/* ========================================================================= */
/* Prepare                                                                   */
/* ========================================================================= */

bool
vitte_c17_translation_unit_prepare(
    vitte_c17_translation_unit_t *unit)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return false;
    }

    if (unit->state !=
            VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING &&
        unit->state !=
            VITTE_C17_TRANSLATION_UNIT_STATE_PREPARED) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_STATE);
    }

    if (unit->program == NULL ||
        !vitte_c17_program_is_valid(
            unit->program)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_PROGRAM);
    }

    if (unit->config.finalize_program) {
        if (!vitte_c17_program_finalize(
                unit->program)) {
            return
                vitte_c17_translation_unit_fail(
                    unit,
                    VITTE_C17_TRANSLATION_UNIT_ERROR_PROGRAM_FINALIZATION);
        }
    } else if (unit->program->ordered_module_count == 0u &&
               unit->program->module_count != 0u) {
        if (!vitte_c17_program_compute_order(
                unit->program)) {
            return
                vitte_c17_translation_unit_fail(
                    unit,
                    VITTE_C17_TRANSLATION_UNIT_ERROR_PROGRAM_FINALIZATION);
        }
    }

    if (unit->config.validate_program &&
        !vitte_c17_program_validate(
            unit->program)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_PROGRAM);
    }

    if (!vitte_c17_translation_unit_aggregate_program(
            unit)) {
        return false;
    }

    vitte_c17_translation_unit_sort(
        unit);

    vitte_c17_translation_unit_refresh_stats(
        unit);

    unit->state =
        VITTE_C17_TRANSLATION_UNIT_STATE_PREPARED;

    unit->last_error =
        VITTE_C17_TRANSLATION_UNIT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Feature macro emission                                                    */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_emit_feature_name(
    vitte_c17_backend_t *backend,
    const char *name,
    size_t length)
{
    size_t index;

    if (!vitte_c17_backend_write(
            backend,
            "#define VITTE_FEATURE_")) {
        return false;
    }

    for (index = 0u;
         index < length;
         ++index) {
        unsigned char byte;
        char output;

        byte =
            (unsigned char)name[index];

        if ((byte >= (unsigned char)'a' &&
             byte <= (unsigned char)'z')) {
            output =
                (char)(
                    byte -
                    (unsigned char)'a' +
                    (unsigned char)'A');
        } else if ((byte >= (unsigned char)'A' &&
                    byte <= (unsigned char)'Z') ||
                   (byte >= (unsigned char)'0' &&
                    byte <= (unsigned char)'9') ||
                   byte == (unsigned char)'_') {
            output = (char)byte;
        } else {
            output = '_';
        }

        if (!vitte_c17_backend_write_char(
                backend,
                output)) {
            return false;
        }
    }

    return
        vitte_c17_backend_write(
            backend,
            " 1");
}

/* ========================================================================= */
/* Emit feature macros                                                       */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_emit_features(
    vitte_c17_translation_unit_t *unit)
{
    size_t index;

    if (unit->feature_count == 0u &&
        !unit->config.emit_empty_sections) {
        return true;
    }

    if (!vitte_c17_backend_begin_section(
            unit->backend,
            VITTE_C17_SECTION_FEATURE_MACROS)) {
        return false;
    }

    for (index = 0u;
         index < unit->feature_count;
         ++index) {
        if (!vitte_c17_translation_unit_emit_feature_name(
                unit->backend,
                unit->features[index].name,
                unit->features[index].name_length) ||
            !vitte_c17_backend_newline(
                unit->backend)) {
            return false;
        }
    }

    return
        vitte_c17_backend_end_section(
            unit->backend);
}

/* ========================================================================= */
/* Emit includes                                                             */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_emit_includes(
    vitte_c17_translation_unit_t *unit)
{
    size_t index;

    if (unit->include_count == 0u &&
        !unit->config.emit_empty_sections) {
        return true;
    }

    if (!vitte_c17_backend_begin_section(
            unit->backend,
            VITTE_C17_SECTION_INCLUDES)) {
        return false;
    }

    for (index = 0u;
         index < unit->include_count;
         ++index) {
        const vitte_c17_translation_unit_include_t *include;

        include =
            &unit->includes[index];

        if (!vitte_c17_backend_write(
                unit->backend,
                "#include ")) {
            return false;
        }

        switch (include->kind) {
            case VITTE_C17_INCLUDE_SYSTEM:
                if (!vitte_c17_backend_write_char(
                        unit->backend,
                        '<') ||
                    !vitte_c17_backend_write_n(
                        unit->backend,
                        include->path,
                        include->path_length) ||
                    !vitte_c17_backend_write_char(
                        unit->backend,
                        '>')) {
                    return false;
                }
                break;

            case VITTE_C17_INCLUDE_LOCAL:
                if (!vitte_c17_backend_write_char(
                        unit->backend,
                        '"') ||
                    !vitte_c17_backend_write_n(
                        unit->backend,
                        include->path,
                        include->path_length) ||
                    !vitte_c17_backend_write_char(
                        unit->backend,
                        '"')) {
                    return false;
                }
                break;

            default:
                return false;
        }

        if (!vitte_c17_backend_newline(
                unit->backend)) {
            return false;
        }
    }

    return
        vitte_c17_backend_end_section(
            unit->backend);
}

/* ========================================================================= */
/* Module boundary comments                                                  */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_emit_module_boundary(
    vitte_c17_translation_unit_t *unit,
    uint64_t module_id)
{
    const vitte_c17_program_module_t *module;

    if (!unit->config.emit_module_boundaries ||
        module_id == UINT64_C(0)) {
        return true;
    }

    module =
        vitte_c17_program_find_module_by_id(
            unit->program,
            module_id);

    if (module == NULL) {
        return false;
    }

    if (!vitte_c17_backend_write(
            unit->backend,
            "/* module: ") ||
        !vitte_c17_backend_write_n(
            unit->backend,
            module->name,
            module->name_length) ||
        !vitte_c17_backend_write(
            unit->backend,
            " */") ||
        !vitte_c17_backend_newline(
            unit->backend)) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Emit fragments for section                                                */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_emit_fragment_section(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_section_t section)
{
    size_t index;
    size_t count;
    uint64_t previous_module;

    count = 0u;

    for (index = 0u;
         index < unit->fragment_count;
         ++index) {
        if (unit->fragments[index].section ==
            section) {
            ++count;
        }
    }

    if (count == 0u &&
        !unit->config.emit_empty_sections) {
        return true;
    }

    if (!vitte_c17_backend_begin_section(
            unit->backend,
            section)) {
        return false;
    }

    previous_module =
        UINT64_C(0);

    for (index = 0u;
         index < unit->fragment_count;
         ++index) {
        const vitte_c17_translation_unit_fragment_t *fragment;

        fragment =
            &unit->fragments[index];

        if (fragment->section != section) {
            continue;
        }

        if (fragment->module_id !=
            previous_module) {
            if (previous_module !=
                    UINT64_C(0) &&
                !vitte_c17_backend_newline(
                    unit->backend)) {
                return false;
            }

            if (!vitte_c17_translation_unit_emit_module_boundary(
                    unit,
                    fragment->module_id)) {
                return false;
            }

            previous_module =
                fragment->module_id;
        }

        /*
         * Source-map anchor.
         *
         * module fragment currently exposes source_id rather than a complete
         * source span. Full C<->Vitte remapping can replace this hook when
         * module fragments carry file/begin/end coordinates.
         */
        if (fragment->text != NULL &&
            fragment->length != 0u) {
            if (!vitte_c17_backend_write_n(
                    unit->backend,
                    fragment->text,
                    fragment->length)) {
                return false;
            }
        }

        /*
         * Ensure fragments cannot accidentally concatenate onto one C token.
         */
        if (fragment->length == 0u ||
            fragment->text[
                fragment->length - 1u] != '\n') {
            if (!vitte_c17_backend_newline(
                    unit->backend)) {
                return false;
            }
        }

        ++unit->stats.emitted_fragment_count;

        unit->stats.emitted_fragment_bytes =
            vitte_c17_translation_unit_saturating_add(
                unit->stats.emitted_fragment_bytes,
                fragment->length);
    }

    return
        vitte_c17_backend_end_section(
            unit->backend);
}

/* ========================================================================= */
/* Runtime requirement comment                                               */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_emit_runtime_requirements(
    vitte_c17_translation_unit_t *unit)
{
    if (unit->runtime_requirements ==
        UINT64_C(0)) {
        return true;
    }

    /*
     * runtime.c will eventually consume this mask directly. Until that layer
     * owns concrete runtime emission, keep the requirement visible in
     * generated debug output without inventing runtime definitions here.
     */
    if (!unit->backend->config.emit_comments) {
        return true;
    }

    return
        vitte_c17_backend_comment(
            unit->backend,
            "Vitte runtime requirements aggregated for this translation unit");
}

/* ========================================================================= */
/* Canonical section emission                                                */
/* ========================================================================= */

static bool
vitte_c17_translation_unit_emit_sections(
    vitte_c17_translation_unit_t *unit)
{
    static const vitte_c17_section_t sections[] = {
        VITTE_C17_SECTION_FORWARD_DECLARATIONS,
        VITTE_C17_SECTION_TYPE_DECLARATIONS,
        VITTE_C17_SECTION_GLOBAL_DECLARATIONS,
        VITTE_C17_SECTION_RUNTIME_DECLARATIONS,
        VITTE_C17_SECTION_FUNCTION_DECLARATIONS,
        VITTE_C17_SECTION_RUNTIME_DEFINITIONS,
        VITTE_C17_SECTION_GLOBAL_DEFINITIONS,
        VITTE_C17_SECTION_FUNCTION_DEFINITIONS,
        VITTE_C17_SECTION_ENTRY,
        VITTE_C17_SECTION_EPILOGUE
    };

    size_t index;

    for (index = 0u;
         index < sizeof(sections) / sizeof(sections[0]);
         ++index) {
        if (!vitte_c17_translation_unit_emit_fragment_section(
                unit,
                sections[index])) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Emit                                                                      */
/* ========================================================================= */

bool
vitte_c17_translation_unit_emit(
    vitte_c17_translation_unit_t *unit)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return false;
    }

    if (unit->state ==
        VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING) {
        if (!vitte_c17_translation_unit_prepare(
                unit)) {
            return false;
        }
    }

    if (unit->state !=
        VITTE_C17_TRANSLATION_UNIT_STATE_PREPARED) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_STATE);
    }

    if (unit->backend == NULL ||
        !vitte_c17_backend_is_valid(
            unit->backend)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_BACKEND);
    }

    unit->state =
        VITTE_C17_TRANSLATION_UNIT_STATE_EMITTING;

    unit->stats.emitted_fragment_count = 0u;
    unit->stats.emitted_fragment_bytes = 0u;

    /*
     * backend_begin() is intentionally not called here.
     *
     * This function emits into an already-generating backend context.
     * vitte_c17_translation_unit_generate() below owns the complete
     * backend_generate()/begin/finish lifecycle.
     */

    if (!vitte_c17_translation_unit_emit_features(
            unit)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_BACKEND);
    }

    if (!vitte_c17_translation_unit_emit_includes(
            unit)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_BACKEND);
    }

    if (!vitte_c17_translation_unit_emit_runtime_requirements(
            unit)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_BACKEND);
    }

    if (!vitte_c17_translation_unit_emit_sections(
            unit)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_MODULE_EMISSION);
    }

    unit->state =
        VITTE_C17_TRANSLATION_UNIT_STATE_EMITTED;

    unit->last_error =
        VITTE_C17_TRANSLATION_UNIT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Backend adapter                                                           */
/* ========================================================================= */

bool
vitte_c17_translation_unit_emit_adapter(
    vitte_c17_backend_t *backend,
    const void *translation_unit,
    void *user_data)
{
    vitte_c17_translation_unit_t *unit;

    (void)user_data;

    if (backend == NULL ||
        translation_unit == NULL) {
        return false;
    }

    unit =
        (vitte_c17_translation_unit_t *)
            translation_unit;

    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return false;
    }

    /*
     * backend_generate() supplies the backend used for this emission.
     */
    unit->backend = backend;

    return
        vitte_c17_translation_unit_emit(
            unit);
}

/* ========================================================================= */
/* Generate                                                                  */
/* ========================================================================= */

bool
vitte_c17_translation_unit_generate(
    vitte_c17_translation_unit_t *unit,
    vitte_c17_backend_t *backend)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit) ||
        backend == NULL ||
        !vitte_c17_backend_is_valid(
            backend)) {
        return false;
    }

    if (unit->state ==
        VITTE_C17_TRANSLATION_UNIT_STATE_BUILDING) {
        if (!vitte_c17_translation_unit_prepare(
                unit)) {
            return false;
        }
    }

    if (unit->state !=
        VITTE_C17_TRANSLATION_UNIT_STATE_PREPARED) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_STATE);
    }

    unit->backend = backend;

    if (!vitte_c17_backend_generate(
            backend,
            unit,
            vitte_c17_translation_unit_emit_adapter,
            NULL)) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_BACKEND);
    }

    /*
     * emit_adapter() transitions to EMITTED.
     */
    if (unit->state !=
        VITTE_C17_TRANSLATION_UNIT_STATE_EMITTED) {
        return
            vitte_c17_translation_unit_fail(
                unit,
                VITTE_C17_TRANSLATION_UNIT_ERROR_CORRUPTION);
    }

    unit->last_error =
        VITTE_C17_TRANSLATION_UNIT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Stable hash                                                               */
/* ========================================================================= */

uint64_t
vitte_c17_translation_unit_hash(
    const vitte_c17_translation_unit_t *unit)
{
    uint64_t hash;
    size_t index;

    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return UINT64_C(0);
    }

    hash =
        VITTE_C17_TRANSLATION_UNIT_HASH_OFFSET;

    if (unit->program != NULL) {
        hash =
            vitte_c17_translation_unit_hash_u64(
                hash,
                vitte_c17_program_hash(
                    unit->program));
    }

    hash =
        vitte_c17_translation_unit_hash_size(
            hash,
            unit->include_count);

    hash =
        vitte_c17_translation_unit_hash_size(
            hash,
            unit->feature_count);

    hash =
        vitte_c17_translation_unit_hash_size(
            hash,
            unit->fragment_count);

    hash =
        vitte_c17_translation_unit_hash_u64(
            hash,
            unit->runtime_requirements);

    hash =
        vitte_c17_translation_unit_hash_bool(
            hash,
            unit->config.deterministic);

    for (index = 0u;
         index < unit->include_count;
         ++index) {
        hash =
            vitte_c17_translation_unit_hash_u64(
                hash,
                (uint64_t)unit->includes[index].kind);

        hash =
            vitte_c17_translation_unit_hash_size(
                hash,
                unit->includes[index].path_length);

        hash =
            vitte_c17_translation_unit_hash_bytes(
                hash,
                unit->includes[index].path,
                unit->includes[index].path_length);
    }

    for (index = 0u;
         index < unit->feature_count;
         ++index) {
        hash =
            vitte_c17_translation_unit_hash_size(
                hash,
                unit->features[index].name_length);

        hash =
            vitte_c17_translation_unit_hash_bytes(
                hash,
                unit->features[index].name,
                unit->features[index].name_length);
    }

    for (index = 0u;
         index < unit->fragment_count;
         ++index) {
        hash =
            vitte_c17_translation_unit_hash_u64(
                hash,
                (uint64_t)unit->fragments[index].section);

        hash =
            vitte_c17_translation_unit_hash_u64(
                hash,
                unit->fragments[index].module_id);

        hash =
            vitte_c17_translation_unit_hash_size(
                hash,
                unit->fragments[index].length);

        hash =
            vitte_c17_translation_unit_hash_bytes(
                hash,
                unit->fragments[index].text,
                unit->fragments[index].length);
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_translation_unit_stats_t
vitte_c17_translation_unit_stats(
    const vitte_c17_translation_unit_t *unit)
{
    vitte_c17_translation_unit_stats_t stats;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return stats;
    }

    stats = unit->stats;

    stats.valid = true;

    stats.hash =
        vitte_c17_translation_unit_hash(
            unit);

    return stats;
}

/* ========================================================================= */
/* Deep validation                                                          */
/* ========================================================================= */

bool
vitte_c17_translation_unit_validate(
    const vitte_c17_translation_unit_t *unit)
{
    size_t index;
    size_t other;
    size_t computed_fragment_bytes;

    if (!vitte_c17_translation_unit_is_valid(
            unit)) {
        return false;
    }

    if (!vitte_c17_translation_unit_config_valid(
            &unit->config)) {
        return false;
    }

    if (unit->program != NULL &&
        !vitte_c17_program_is_valid(
            unit->program)) {
        return false;
    }

    if (unit->backend != NULL &&
        !vitte_c17_backend_is_valid(
            unit->backend)) {
        return false;
    }

    if ((unit->state ==
             VITTE_C17_TRANSLATION_UNIT_STATE_PREPARED ||
         unit->state ==
             VITTE_C17_TRANSLATION_UNIT_STATE_EMITTING ||
         unit->state ==
             VITTE_C17_TRANSLATION_UNIT_STATE_EMITTED) &&
        unit->program == NULL) {
        return false;
    }

    /* Includes */
    for (index = 0u;
         index < unit->include_count;
         ++index) {
        const vitte_c17_translation_unit_include_t *include;

        include =
            &unit->includes[index];

        if (include->kind <=
                VITTE_C17_INCLUDE_INVALID ||
            include->kind >=
                VITTE_C17_INCLUDE_COUNT ||
            include->path == NULL ||
            include->path_length == 0u) {
            return false;
        }

        if (unit->config.deduplicate_includes) {
            for (other = index + 1u;
                 other < unit->include_count;
                 ++other) {
                if (include->kind ==
                        unit->includes[other].kind &&
                    include->path_length ==
                        unit->includes[other].path_length &&
                    memcmp(
                        include->path,
                        unit->includes[other].path,
                        include->path_length) == 0) {
                    return false;
                }
            }
        }
    }

    /* Features */
    for (index = 0u;
         index < unit->feature_count;
         ++index) {
        const vitte_c17_translation_unit_feature_t *feature;

        feature =
            &unit->features[index];

        if (feature->name == NULL ||
            feature->name_length == 0u) {
            return false;
        }

        if (unit->config.deduplicate_features) {
            for (other = index + 1u;
                 other < unit->feature_count;
                 ++other) {
                if (feature->name_length ==
                        unit->features[other].name_length &&
                    memcmp(
                        feature->name,
                        unit->features[other].name,
                        feature->name_length) == 0) {
                    return false;
                }
            }
        }
    }

    /* Fragments */
    computed_fragment_bytes = 0u;

    for (index = 0u;
         index < unit->fragment_count;
         ++index) {
        const vitte_c17_translation_unit_fragment_t *fragment;

        fragment =
            &unit->fragments[index];

        if (fragment->section <=
                VITTE_C17_SECTION_NONE ||
            fragment->section >=
                VITTE_C17_SECTION_COUNT ||
            fragment->text == NULL) {
            return false;
        }

        computed_fragment_bytes =
            vitte_c17_translation_unit_saturating_add(
                computed_fragment_bytes,
                fragment->length);

        if (unit->program != NULL &&
            fragment->module_id != UINT64_C(0) &&
            vitte_c17_program_find_module_by_id(
                unit->program,
                fragment->module_id) == NULL) {
            return false;
        }

        if (unit->config.deduplicate_fragments) {
            for (other = index + 1u;
                 other < unit->fragment_count;
                 ++other) {
                if (vitte_c17_translation_unit_fragment_equal(
                        fragment,
                        unit->fragments[other].section,
                        unit->fragments[other].text,
                        unit->fragments[other].length)) {
                    return false;
                }
            }
        }
    }

    if (computed_fragment_bytes !=
        unit->fragment_bytes) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Accessors                                                                 */
/* ========================================================================= */

vitte_c17_translation_unit_state_t
vitte_c17_translation_unit_state(
    const vitte_c17_translation_unit_t *unit)
{
    if (unit == NULL ||
        unit->magic !=
            VITTE_C17_TRANSLATION_UNIT_MAGIC) {
        return
            VITTE_C17_TRANSLATION_UNIT_STATE_UNINITIALIZED;
    }

    return unit->state;
}

vitte_c17_translation_unit_error_t
vitte_c17_translation_unit_last_error(
    const vitte_c17_translation_unit_t *unit)
{
    if (unit == NULL ||
        unit->magic !=
            VITTE_C17_TRANSLATION_UNIT_MAGIC) {
        return
            VITTE_C17_TRANSLATION_UNIT_ERROR_INVALID_UNIT;
    }

    return unit->last_error;
}

void
vitte_c17_translation_unit_clear_error(
    vitte_c17_translation_unit_t *unit)
{
    if (unit == NULL ||
        unit->magic !=
            VITTE_C17_TRANSLATION_UNIT_MAGIC) {
        return;
    }

    unit->last_error =
        VITTE_C17_TRANSLATION_UNIT_ERROR_NONE;

    /*
     * Do not silently recover FAILED -> BUILDING/PREPARED here.
     *
     * A failed translation unit must be reset explicitly.
     */
}

size_t
vitte_c17_translation_unit_include_count(
    const vitte_c17_translation_unit_t *unit)
{
    return
        vitte_c17_translation_unit_is_valid(unit)
            ? unit->include_count
            : 0u;
}

size_t
vitte_c17_translation_unit_feature_count(
    const vitte_c17_translation_unit_t *unit)
{
    return
        vitte_c17_translation_unit_is_valid(unit)
            ? unit->feature_count
            : 0u;
}

size_t
vitte_c17_translation_unit_fragment_count(
    const vitte_c17_translation_unit_t *unit)
{
    return
        vitte_c17_translation_unit_is_valid(unit)
            ? unit->fragment_count
            : 0u;
}

uint64_t
vitte_c17_translation_unit_runtime_requirements(
    const vitte_c17_translation_unit_t *unit)
{
    return
        vitte_c17_translation_unit_is_valid(unit)
            ? unit->runtime_requirements
            : UINT64_C(0);
}

const vitte_c17_translation_unit_include_t *
vitte_c17_translation_unit_include_at(
    const vitte_c17_translation_unit_t *unit,
    size_t index)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit) ||
        index >= unit->include_count) {
        return NULL;
    }

    return &unit->includes[index];
}

const vitte_c17_translation_unit_feature_t *
vitte_c17_translation_unit_feature_at(
    const vitte_c17_translation_unit_t *unit,
    size_t index)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit) ||
        index >= unit->feature_count) {
        return NULL;
    }

    return &unit->features[index];
}

const vitte_c17_translation_unit_fragment_t *
vitte_c17_translation_unit_fragment_at(
    const vitte_c17_translation_unit_t *unit,
    size_t index)
{
    if (!vitte_c17_translation_unit_is_valid(
            unit) ||
        index >= unit->fragment_count) {
        return NULL;
    }

    return &unit->fragments[index];
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
