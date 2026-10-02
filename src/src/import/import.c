/*
 * Vitte Compiler
 * src/import/import.c
 *
 * Import and module dependency subsystem.
 *
 * Responsibilities:
 *
 *   - import context lifecycle;
 *   - module registration;
 *   - module lookup;
 *   - import registration;
 *   - dependency graph construction;
 *   - duplicate dependency suppression;
 *   - module state tracking;
 *   - deterministic dependency traversal;
 *   - cycle detection;
 *   - topological ordering;
 *   - import resolution;
 *   - source/module association;
 *   - cache-friendly fingerprints;
 *   - resource limits;
 *   - statistics;
 *   - defensive validation.
 *
 * This implementation intentionally does not perform filesystem access.
 * Module discovery and source loading belong to the filesystem/source/package
 * layers. This subsystem receives canonical module identities and coordinates
 * their dependency relationships.
 *
 * ISO C17.
 */

#include "import.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(
    VITTE_IMPORT_INVALID_ID == UINT64_C(0),
    "import zero ID must remain invalid");

_Static_assert(
    VITTE_IMPORT_DEFAULT_INITIAL_CAPACITY > 0u,
    "import initial capacity must be non-zero");

_Static_assert(
    VITTE_IMPORT_DEFAULT_MAX_MODULES > 0u,
    "import module limit must be non-zero");

_Static_assert(
    VITTE_IMPORT_DEFAULT_MAX_IMPORTS > 0u,
    "import dependency limit must be non-zero");

_Static_assert(
    VITTE_IMPORT_DEFAULT_MAX_STRING_BYTES > 0u,
    "import string limit must be non-zero");

/* ========================================================================= */
/* Private helpers                                                           */
/* ========================================================================= */

static bool
vitte_import_size_add(
    size_t left,
    size_t right,
    size_t *result);

static bool
vitte_import_size_mul(
    size_t left,
    size_t right,
    size_t *result);

static uint64_t
vitte_import_u64_add_sat(
    uint64_t left,
    uint64_t right);

static uint64_t
vitte_import_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t length);

static uint64_t
vitte_import_hash_u64(
    uint64_t hash,
    uint64_t value);

static uint64_t
vitte_import_hash_size(
    uint64_t hash,
    size_t value);

static uint64_t
vitte_import_hash_bool(
    uint64_t hash,
    bool value);

static bool
vitte_import_context_envelope_valid(
    const vitte_import_context_t *context);

static bool
vitte_import_is_mutable(
    const vitte_import_context_t *context);

static bool
vitte_import_fail(
    vitte_import_context_t *context,
    vitte_import_error_t error);

static void *
vitte_import_realloc(
    vitte_import_context_t *context,
    void *pointer,
    size_t bytes);

static bool
vitte_import_reserve_raw(
    vitte_import_context_t *context,
    void **storage,
    size_t *capacity,
    size_t required,
    size_t element_size);

static bool
vitte_import_reserve_modules(
    vitte_import_context_t *context,
    size_t required);

static bool
vitte_import_reserve_edges(
    vitte_import_context_t *context,
    size_t required);

static bool
vitte_import_reserve_strings(
    vitte_import_context_t *context,
    size_t required);

static bool
vitte_import_reserve_id_list(
    vitte_import_context_t *context,
    vitte_import_module_id_t **items,
    size_t *capacity,
    size_t required);

static void
vitte_import_module_release(
    vitte_import_module_t *module);

static void
vitte_import_edge_release(
    vitte_import_edge_t *edge);

static void
vitte_import_string_release(
    vitte_import_string_t *string);

static void
vitte_import_release_storage(
    vitte_import_context_t *context);

static bool
vitte_import_string_id_exists(
    const vitte_import_context_t *context,
    vitte_import_string_id_t id);

static bool
vitte_import_module_id_exists(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id);

static bool
vitte_import_edge_id_exists(
    const vitte_import_context_t *context,
    vitte_import_edge_id_t id);

static vitte_import_string_id_t
vitte_import_find_string(
    const vitte_import_context_t *context,
    const char *data,
    size_t length,
    uint64_t hash);

static vitte_import_module_id_t
vitte_import_find_module_by_name_internal(
    const vitte_import_context_t *context,
    vitte_import_string_id_t name);

static vitte_import_module_id_t
vitte_import_find_module_by_path_internal(
    const vitte_import_context_t *context,
    vitte_import_string_id_t path);

static bool
vitte_import_module_has_dependency(
    const vitte_import_module_t *module,
    vitte_import_module_id_t dependency);

static bool
vitte_import_module_add_dependency(
    vitte_import_context_t *context,
    vitte_import_module_t *module,
    vitte_import_module_id_t dependency);

static bool
vitte_import_module_add_dependent(
    vitte_import_context_t *context,
    vitte_import_module_t *module,
    vitte_import_module_id_t dependent);

static bool
vitte_import_module_add_edge(
    vitte_import_context_t *context,
    vitte_import_module_t *module,
    vitte_import_edge_id_t edge);

static bool
vitte_import_validate_string(
    const vitte_import_context_t *context,
    const vitte_import_string_t *string);

static bool
vitte_import_validate_module(
    const vitte_import_context_t *context,
    const vitte_import_module_t *module);

static bool
vitte_import_validate_edge(
    const vitte_import_context_t *context,
    const vitte_import_edge_t *edge);

static bool
vitte_import_dfs_visit(
    vitte_import_context_t *context,
    vitte_import_module_id_t module_id,
    uint8_t *marks,
    vitte_import_module_id_t *stack,
    size_t *stack_count,
    vitte_import_module_id_t *order,
    size_t *order_count);

static bool
vitte_import_build_topological_order(
    vitte_import_context_t *context);

static bool
vitte_import_resolve_edge(
    vitte_import_context_t *context,
    vitte_import_edge_t *edge);

static void
vitte_import_clear_resolution_state(
    vitte_import_context_t *context);

static bool
vitte_import_visibility_valid(
    vitte_import_visibility_t visibility);

static bool
vitte_import_module_state_valid(
    vitte_import_module_state_t state);

static bool
vitte_import_edge_state_valid(
    vitte_import_edge_state_t state);

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_import_error_name(
    vitte_import_error_t error)
{
    switch (error) {
        case VITTE_IMPORT_ERROR_NONE:
            return "none";

        case VITTE_IMPORT_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case VITTE_IMPORT_ERROR_INVALID_CONTEXT:
            return "invalid_context";

        case VITTE_IMPORT_ERROR_INVALID_STATE:
            return "invalid_state";

        case VITTE_IMPORT_ERROR_INVALID_STRING:
            return "invalid_string";

        case VITTE_IMPORT_ERROR_INVALID_MODULE:
            return "invalid_module";

        case VITTE_IMPORT_ERROR_INVALID_EDGE:
            return "invalid_edge";

        case VITTE_IMPORT_ERROR_DUPLICATE_MODULE:
            return "duplicate_module";

        case VITTE_IMPORT_ERROR_DUPLICATE_IMPORT:
            return "duplicate_import";

        case VITTE_IMPORT_ERROR_MODULE_NOT_FOUND:
            return "module_not_found";

        case VITTE_IMPORT_ERROR_IMPORT_NOT_FOUND:
            return "import_not_found";

        case VITTE_IMPORT_ERROR_SELF_IMPORT:
            return "self_import";

        case VITTE_IMPORT_ERROR_CYCLE:
            return "cycle";

        case VITTE_IMPORT_ERROR_TOO_MANY_MODULES:
            return "too_many_modules";

        case VITTE_IMPORT_ERROR_TOO_MANY_IMPORTS:
            return "too_many_imports";

        case VITTE_IMPORT_ERROR_STRING_LIMIT:
            return "string_limit";

        case VITTE_IMPORT_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_IMPORT_ERROR_OUT_OF_MEMORY:
            return "out_of_memory";

        case VITTE_IMPORT_ERROR_VALIDATION:
            return "validation";

        case VITTE_IMPORT_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_IMPORT_ERROR_UNSUPPORTED:
            return "unsupported";

        case VITTE_IMPORT_ERROR_INTERNAL:
            return "internal";

        case VITTE_IMPORT_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_import_state_name(
    vitte_import_state_t state)
{
    switch (state) {
        case VITTE_IMPORT_STATE_INVALID:
            return "invalid";

        case VITTE_IMPORT_STATE_BUILDING:
            return "building";

        case VITTE_IMPORT_STATE_RESOLVING:
            return "resolving";

        case VITTE_IMPORT_STATE_RESOLVED:
            return "resolved";

        case VITTE_IMPORT_STATE_FAILED:
            return "failed";

        case VITTE_IMPORT_STATE_DESTROYED:
            return "destroyed";

        case VITTE_IMPORT_STATE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_import_module_state_name(
    vitte_import_module_state_t state)
{
    switch (state) {
        case VITTE_IMPORT_MODULE_INVALID:
            return "invalid";

        case VITTE_IMPORT_MODULE_REGISTERED:
            return "registered";

        case VITTE_IMPORT_MODULE_RESOLVING:
            return "resolving";

        case VITTE_IMPORT_MODULE_RESOLVED:
            return "resolved";

        case VITTE_IMPORT_MODULE_FAILED:
            return "failed";

        case VITTE_IMPORT_MODULE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_import_edge_state_name(
    vitte_import_edge_state_t state)
{
    switch (state) {
        case VITTE_IMPORT_EDGE_INVALID:
            return "invalid";

        case VITTE_IMPORT_EDGE_UNRESOLVED:
            return "unresolved";

        case VITTE_IMPORT_EDGE_RESOLVED:
            return "resolved";

        case VITTE_IMPORT_EDGE_FAILED:
            return "failed";

        case VITTE_IMPORT_EDGE_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Checked arithmetic                                                        */
/* ========================================================================= */

static bool
vitte_import_size_add(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left > SIZE_MAX - right) {
        return false;
    }

    *result = left + right;
    return true;
}

static bool
vitte_import_size_mul(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left != 0u && right > SIZE_MAX / left) {
        return false;
    }

    *result = left * right;
    return true;
}

static uint64_t
vitte_import_u64_add_sat(
    uint64_t left,
    uint64_t right)
{
    if (UINT64_MAX - left < right) {
        return UINT64_MAX;
    }

    return left + right;
}

/* ========================================================================= */
/* Hashing                                                                   */
/* ========================================================================= */

static uint64_t
vitte_import_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    if (data == NULL && length != 0u) {
        return hash;
    }

    bytes = (const unsigned char *)data;

    for (index = 0u; index < length; ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_IMPORT_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_import_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned int shift;

    for (shift = 0u; shift < 64u; shift += 8u) {
        const unsigned char byte =
            (unsigned char)((value >> shift) & UINT64_C(0xff));

        hash ^= (uint64_t)byte;
        hash *= VITTE_IMPORT_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_import_hash_size(
    uint64_t hash,
    size_t value)
{
    const unsigned char *bytes;
    size_t index;

    bytes = (const unsigned char *)&value;

    for (index = 0u; index < sizeof(value); ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_IMPORT_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_import_hash_bool(
    uint64_t hash,
    bool value)
{
    const unsigned char byte =
        value ? (unsigned char)1u : (unsigned char)0u;

    hash ^= (uint64_t)byte;
    hash *= VITTE_IMPORT_FNV_PRIME;

    return hash;
}

/* ========================================================================= */
/* Enum validation                                                           */
/* ========================================================================= */

static bool
vitte_import_visibility_valid(
    vitte_import_visibility_t visibility)
{
    return
        visibility >= VITTE_IMPORT_VISIBILITY_PRIVATE &&
        visibility < VITTE_IMPORT_VISIBILITY_COUNT;
}

static bool
vitte_import_module_state_valid(
    vitte_import_module_state_t state)
{
    return
        state > VITTE_IMPORT_MODULE_INVALID &&
        state < VITTE_IMPORT_MODULE_COUNT;
}

static bool
vitte_import_edge_state_valid(
    vitte_import_edge_state_t state)
{
    return
        state > VITTE_IMPORT_EDGE_INVALID &&
        state < VITTE_IMPORT_EDGE_COUNT;
}

/* ========================================================================= */
/* Context validation                                                        */
/* ========================================================================= */

static bool
vitte_import_context_envelope_valid(
    const vitte_import_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    if (context->magic != VITTE_IMPORT_MAGIC) {
        return false;
    }

    if (context->state <= VITTE_IMPORT_STATE_INVALID ||
        context->state >= VITTE_IMPORT_STATE_DESTROYED) {
        return false;
    }

    if (context->module_count > context->module_capacity) {
        return false;
    }

    if (context->edge_count > context->edge_capacity) {
        return false;
    }

    if (context->string_count > context->string_capacity) {
        return false;
    }

    if (context->module_count != 0u &&
        context->modules == NULL) {
        return false;
    }

    if (context->edge_count != 0u &&
        context->edges == NULL) {
        return false;
    }

    if (context->string_count != 0u &&
        context->strings == NULL) {
        return false;
    }

    if (context->topological_count >
        context->topological_capacity) {
        return false;
    }

    if (context->topological_count != 0u &&
        context->topological_order == NULL) {
        return false;
    }

    return true;
}

bool
vitte_import_is_valid(
    const vitte_import_context_t *context)
{
    return vitte_import_context_envelope_valid(context);
}

static bool
vitte_import_is_mutable(
    const vitte_import_context_t *context)
{
    return
        vitte_import_context_envelope_valid(context) &&
        context->state == VITTE_IMPORT_STATE_BUILDING;
}

/* ========================================================================= */
/* Failure                                                                   */
/* ========================================================================= */

static bool
vitte_import_fail(
    vitte_import_context_t *context,
    vitte_import_error_t error)
{
    if (context != NULL &&
        context->magic == VITTE_IMPORT_MAGIC) {
        context->last_error = error;
    }

    return false;
}

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

static void *
vitte_import_realloc(
    vitte_import_context_t *context,
    void *pointer,
    size_t bytes)
{
    void *result;

    if (bytes == 0u) {
        free(pointer);
        return NULL;
    }

    result = realloc(pointer, bytes);

    if (result == NULL) {
        if (context != NULL) {
            context->stats.allocation_failures =
                vitte_import_u64_add_sat(
                    context->stats.allocation_failures,
                    UINT64_C(1));

            context->last_error =
                VITTE_IMPORT_ERROR_OUT_OF_MEMORY;
        }

        return NULL;
    }

    if (context != NULL) {
        if (pointer == NULL) {
            context->stats.allocations =
                vitte_import_u64_add_sat(
                    context->stats.allocations,
                    UINT64_C(1));
        } else {
            context->stats.reallocations =
                vitte_import_u64_add_sat(
                    context->stats.reallocations,
                    UINT64_C(1));
        }
    }

    return result;
}

static bool
vitte_import_reserve_raw(
    vitte_import_context_t *context,
    void **storage,
    size_t *capacity,
    size_t required,
    size_t element_size)
{
    size_t new_capacity;
    size_t bytes;
    void *new_storage;

    if (context == NULL ||
        storage == NULL ||
        capacity == NULL ||
        element_size == 0u) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);
    }

    if (required <= *capacity) {
        return true;
    }

    new_capacity = *capacity;

    if (new_capacity == 0u) {
        new_capacity =
            VITTE_IMPORT_DEFAULT_INITIAL_CAPACITY;
    }

    while (new_capacity < required) {
        size_t doubled;

        if (!vitte_import_size_mul(
                new_capacity,
                (size_t)2u,
                &doubled)) {
            new_capacity = required;
            break;
        }

        if (doubled <= new_capacity) {
            new_capacity = required;
            break;
        }

        new_capacity = doubled;
    }

    if (!vitte_import_size_mul(
            new_capacity,
            element_size,
            &bytes)) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);
    }

    new_storage =
        vitte_import_realloc(
            context,
            *storage,
            bytes);

    if (new_storage == NULL) {
        return false;
    }

    *storage = new_storage;
    *capacity = new_capacity;

    return true;
}

static bool
vitte_import_reserve_modules(
    vitte_import_context_t *context,
    size_t required)
{
    return vitte_import_reserve_raw(
        context,
        (void **)&context->modules,
        &context->module_capacity,
        required,
        sizeof(*context->modules));
}

static bool
vitte_import_reserve_edges(
    vitte_import_context_t *context,
    size_t required)
{
    return vitte_import_reserve_raw(
        context,
        (void **)&context->edges,
        &context->edge_capacity,
        required,
        sizeof(*context->edges));
}

static bool
vitte_import_reserve_strings(
    vitte_import_context_t *context,
    size_t required)
{
    return vitte_import_reserve_raw(
        context,
        (void **)&context->strings,
        &context->string_capacity,
        required,
        sizeof(*context->strings));
}

static bool
vitte_import_reserve_id_list(
    vitte_import_context_t *context,
    vitte_import_module_id_t **items,
    size_t *capacity,
    size_t required)
{
    return vitte_import_reserve_raw(
        context,
        (void **)items,
        capacity,
        required,
        sizeof(**items));
}

/* ========================================================================= */
/* Storage release                                                           */
/* ========================================================================= */

static void
vitte_import_module_release(
    vitte_import_module_t *module)
{
    if (module == NULL) {
        return;
    }

    free(module->dependencies);
    free(module->dependents);
    free(module->imports);

    module->dependencies = NULL;
    module->dependency_count = 0u;
    module->dependency_capacity = 0u;

    module->dependents = NULL;
    module->dependent_count = 0u;
    module->dependent_capacity = 0u;

    module->imports = NULL;
    module->import_count = 0u;
    module->import_capacity = 0u;
}

static void
vitte_import_edge_release(
    vitte_import_edge_t *edge)
{
    if (edge == NULL) {
        return;
    }

    memset(edge, 0, sizeof(*edge));
}

static void
vitte_import_string_release(
    vitte_import_string_t *string)
{
    if (string == NULL) {
        return;
    }

    free(string->data);

    string->data = NULL;
    string->length = 0u;
    string->hash = UINT64_C(0);
    string->id = VITTE_IMPORT_INVALID_ID;
}

static void
vitte_import_release_storage(
    vitte_import_context_t *context)
{
    size_t index;

    if (context == NULL) {
        return;
    }

    if (context->modules != NULL) {
        for (index = 0u;
             index < context->module_count;
             ++index) {
            vitte_import_module_release(
                &context->modules[index]);
        }
    }

    if (context->edges != NULL) {
        for (index = 0u;
             index < context->edge_count;
             ++index) {
            vitte_import_edge_release(
                &context->edges[index]);
        }
    }

    if (context->strings != NULL) {
        for (index = 0u;
             index < context->string_count;
             ++index) {
            vitte_import_string_release(
                &context->strings[index]);
        }
    }

    free(context->modules);
    free(context->edges);
    free(context->strings);
    free(context->topological_order);

    context->modules = NULL;
    context->module_count = 0u;
    context->module_capacity = 0u;

    context->edges = NULL;
    context->edge_count = 0u;
    context->edge_capacity = 0u;

    context->strings = NULL;
    context->string_count = 0u;
    context->string_capacity = 0u;
    context->string_bytes = 0u;

    context->topological_order = NULL;
    context->topological_count = 0u;
    context->topological_capacity = 0u;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_import_init(
    vitte_import_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    memset(context, 0, sizeof(*context));

    context->magic = VITTE_IMPORT_MAGIC;
    context->state = VITTE_IMPORT_STATE_BUILDING;
    context->last_error = VITTE_IMPORT_ERROR_NONE;

    context->max_modules =
        VITTE_IMPORT_DEFAULT_MAX_MODULES;

    context->max_imports =
        VITTE_IMPORT_DEFAULT_MAX_IMPORTS;

    context->max_string_bytes =
        VITTE_IMPORT_DEFAULT_MAX_STRING_BYTES;

    context->generation = UINT64_C(1);

    return true;
}

void
vitte_import_destroy(
    vitte_import_context_t *context)
{
    if (context == NULL) {
        return;
    }

    if (context->magic != VITTE_IMPORT_MAGIC) {
        memset(context, 0, sizeof(*context));
        context->state = VITTE_IMPORT_STATE_DESTROYED;
        context->magic = VITTE_IMPORT_DEAD_MAGIC;
        return;
    }

    vitte_import_release_storage(context);

    memset(&context->stats, 0, sizeof(context->stats));

    context->last_error = VITTE_IMPORT_ERROR_NONE;
    context->state = VITTE_IMPORT_STATE_DESTROYED;
    context->magic = VITTE_IMPORT_DEAD_MAGIC;
}

bool
vitte_import_reset(
    vitte_import_context_t *context)
{
    uint64_t generation;
    size_t max_modules;
    size_t max_imports;
    size_t max_string_bytes;

    if (context == NULL ||
        context->magic != VITTE_IMPORT_MAGIC) {
        return false;
    }

    generation = context->generation;

    max_modules = context->max_modules;
    max_imports = context->max_imports;
    max_string_bytes = context->max_string_bytes;

    vitte_import_release_storage(context);

    memset(&context->stats, 0, sizeof(context->stats));

    context->state = VITTE_IMPORT_STATE_BUILDING;
    context->last_error = VITTE_IMPORT_ERROR_NONE;

    context->max_modules = max_modules;
    context->max_imports = max_imports;
    context->max_string_bytes = max_string_bytes;

    if (generation == UINT64_MAX) {
        context->generation = UINT64_MAX;
    } else {
        context->generation = generation + UINT64_C(1);
    }

    return true;
}

/* ========================================================================= */
/* ID validation                                                             */
/* ========================================================================= */

static bool
vitte_import_string_id_exists(
    const vitte_import_context_t *context,
    vitte_import_string_id_t id)
{
    if (!vitte_import_context_envelope_valid(context)) {
        return false;
    }

    if (id == VITTE_IMPORT_INVALID_ID) {
        return false;
    }

    return id <= (uint64_t)context->string_count;
}

static bool
vitte_import_module_id_exists(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id)
{
    if (!vitte_import_context_envelope_valid(context)) {
        return false;
    }

    if (id == VITTE_IMPORT_INVALID_ID) {
        return false;
    }

    return id <= (uint64_t)context->module_count;
}

static bool
vitte_import_edge_id_exists(
    const vitte_import_context_t *context,
    vitte_import_edge_id_t id)
{
    if (!vitte_import_context_envelope_valid(context)) {
        return false;
    }

    if (id == VITTE_IMPORT_INVALID_ID) {
        return false;
    }

    return id <= (uint64_t)context->edge_count;
}

/* ========================================================================= */
/* String interning                                                          */
/* ========================================================================= */

static vitte_import_string_id_t
vitte_import_find_string(
    const vitte_import_context_t *context,
    const char *data,
    size_t length,
    uint64_t hash)
{
    size_t index;

    if (!vitte_import_context_envelope_valid(context)) {
        return VITTE_IMPORT_INVALID_ID;
    }

    for (index = 0u;
         index < context->string_count;
         ++index) {
        const vitte_import_string_t *string =
            &context->strings[index];

        if (string->hash != hash ||
            string->length != length) {
            continue;
        }

        if (length == 0u ||
            memcmp(string->data, data, length) == 0) {
            return string->id;
        }
    }

    return VITTE_IMPORT_INVALID_ID;
}

vitte_import_string_id_t
vitte_import_intern_string(
    vitte_import_context_t *context,
    const char *data,
    size_t length)
{
    uint64_t hash;
    vitte_import_string_id_t existing;
    size_t required_count;
    size_t allocation_size;
    size_t new_total;
    char *copy;
    vitte_import_string_t *string;

    if (!vitte_import_is_mutable(context)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_STATE);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (data == NULL && length != 0u) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);

        return VITTE_IMPORT_INVALID_ID;
    }

    hash = vitte_import_hash_bytes(
        VITTE_IMPORT_FNV_OFFSET,
        data,
        length);

    existing =
        vitte_import_find_string(
            context,
            data,
            length,
            hash);

    if (existing != VITTE_IMPORT_INVALID_ID) {
        return existing;
    }

    if (!vitte_import_size_add(
            context->string_bytes,
            length,
            &new_total)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (new_total > context->max_string_bytes) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_STRING_LIMIT);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_size_add(
            context->string_count,
            (size_t)1u,
            &required_count)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_reserve_strings(
            context,
            required_count)) {
        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_size_add(
            length,
            (size_t)1u,
            &allocation_size)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);

        return VITTE_IMPORT_INVALID_ID;
    }

    copy = (char *)malloc(allocation_size);

    if (copy == NULL) {
        context->stats.allocation_failures =
            vitte_import_u64_add_sat(
                context->stats.allocation_failures,
                UINT64_C(1));

        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OUT_OF_MEMORY);

        return VITTE_IMPORT_INVALID_ID;
    }

    context->stats.allocations =
        vitte_import_u64_add_sat(
            context->stats.allocations,
            UINT64_C(1));

    if (length != 0u) {
        memcpy(copy, data, length);
    }

    copy[length] = '\0';

    string = &context->strings[context->string_count];

    memset(string, 0, sizeof(*string));

    string->id =
        (vitte_import_string_id_t)(
            context->string_count + (size_t)1u);

    string->data = copy;
    string->length = length;
    string->hash = hash;

    context->string_count = required_count;
    context->string_bytes = new_total;

    context->stats.strings_interned =
        vitte_import_u64_add_sat(
            context->stats.strings_interned,
            UINT64_C(1));

    context->stats.string_bytes =
        vitte_import_u64_add_sat(
            context->stats.string_bytes,
            (uint64_t)length);

    context->last_error = VITTE_IMPORT_ERROR_NONE;

    return string->id;
}

const vitte_import_string_t *
vitte_import_get_string(
    const vitte_import_context_t *context,
    vitte_import_string_id_t id)
{
    if (!vitte_import_string_id_exists(context, id)) {
        return NULL;
    }

    return &context->strings[(size_t)(id - UINT64_C(1))];
}

/* ========================================================================= */
/* Module lookup                                                             */
/* ========================================================================= */

static vitte_import_module_id_t
vitte_import_find_module_by_name_internal(
    const vitte_import_context_t *context,
    vitte_import_string_id_t name)
{
    size_t index;

    if (!vitte_import_string_id_exists(context, name)) {
        return VITTE_IMPORT_INVALID_ID;
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        if (context->modules[index].name == name) {
            return context->modules[index].id;
        }
    }

    return VITTE_IMPORT_INVALID_ID;
}

static vitte_import_module_id_t
vitte_import_find_module_by_path_internal(
    const vitte_import_context_t *context,
    vitte_import_string_id_t path)
{
    size_t index;

    if (!vitte_import_string_id_exists(context, path)) {
        return VITTE_IMPORT_INVALID_ID;
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        if (context->modules[index].path == path) {
            return context->modules[index].id;
        }
    }

    return VITTE_IMPORT_INVALID_ID;
}

vitte_import_module_id_t
vitte_import_find_module_by_name(
    const vitte_import_context_t *context,
    vitte_import_string_id_t name)
{
    return vitte_import_find_module_by_name_internal(
        context,
        name);
}

vitte_import_module_id_t
vitte_import_find_module_by_path(
    const vitte_import_context_t *context,
    vitte_import_string_id_t path)
{
    return vitte_import_find_module_by_path_internal(
        context,
        path);
}

/* ========================================================================= */
/* Module registration                                                       */
/* ========================================================================= */

vitte_import_module_id_t
vitte_import_add_module(
    vitte_import_context_t *context,
    const vitte_import_module_desc_t *description)
{
    size_t required;
    vitte_import_module_t *module;

    if (!vitte_import_is_mutable(context)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_STATE);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (description == NULL) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_string_id_exists(
            context,
            description->name)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_STRING);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (description->path != VITTE_IMPORT_INVALID_ID &&
        !vitte_import_string_id_exists(
            context,
            description->path)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_STRING);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_visibility_valid(
            description->visibility)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (vitte_import_find_module_by_name_internal(
            context,
            description->name) !=
        VITTE_IMPORT_INVALID_ID) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_DUPLICATE_MODULE);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (description->path != VITTE_IMPORT_INVALID_ID &&
        vitte_import_find_module_by_path_internal(
            context,
            description->path) !=
        VITTE_IMPORT_INVALID_ID) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_DUPLICATE_MODULE);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (context->module_count >= context->max_modules) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_TOO_MANY_MODULES);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_size_add(
            context->module_count,
            (size_t)1u,
            &required)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_reserve_modules(
            context,
            required)) {
        return VITTE_IMPORT_INVALID_ID;
    }

    module = &context->modules[context->module_count];

    memset(module, 0, sizeof(*module));

    module->id =
        (vitte_import_module_id_t)(
            context->module_count + (size_t)1u);

    module->name = description->name;
    module->path = description->path;
    module->source_id = description->source_id;

    module->visibility = description->visibility;

    module->state =
        VITTE_IMPORT_MODULE_REGISTERED;

    module->is_root = description->is_root;
    module->is_external = description->is_external;

    context->module_count = required;

    context->stats.modules_registered =
        vitte_import_u64_add_sat(
            context->stats.modules_registered,
            UINT64_C(1));

    context->last_error = VITTE_IMPORT_ERROR_NONE;

    return module->id;
}

const vitte_import_module_t *
vitte_import_get_module(
    const vitte_import_context_t *context,
    vitte_import_module_id_t id)
{
    if (!vitte_import_module_id_exists(context, id)) {
        return NULL;
    }

    return &context->modules[(size_t)(id - UINT64_C(1))];
}

vitte_import_module_t *
vitte_import_get_module_mut(
    vitte_import_context_t *context,
    vitte_import_module_id_t id)
{
    if (!vitte_import_is_mutable(context)) {
        return NULL;
    }

    if (!vitte_import_module_id_exists(context, id)) {
        return NULL;
    }

    return &context->modules[(size_t)(id - UINT64_C(1))];
}

/* ========================================================================= */
/* Module lists                                                              */
/* ========================================================================= */

static bool
vitte_import_module_has_dependency(
    const vitte_import_module_t *module,
    vitte_import_module_id_t dependency)
{
    size_t index;

    if (module == NULL) {
        return false;
    }

    for (index = 0u;
         index < module->dependency_count;
         ++index) {
        if (module->dependencies[index] == dependency) {
            return true;
        }
    }

    return false;
}

static bool
vitte_import_module_add_dependency(
    vitte_import_context_t *context,
    vitte_import_module_t *module,
    vitte_import_module_id_t dependency)
{
    size_t required;

    if (context == NULL || module == NULL) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);
    }

    if (vitte_import_module_has_dependency(
            module,
            dependency)) {
        return true;
    }

    if (!vitte_import_size_add(
            module->dependency_count,
            (size_t)1u,
            &required)) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);
    }

    if (!vitte_import_reserve_id_list(
            context,
            &module->dependencies,
            &module->dependency_capacity,
            required)) {
        return false;
    }

    module->dependencies[module->dependency_count] =
        dependency;

    module->dependency_count = required;

    return true;
}

static bool
vitte_import_module_add_dependent(
    vitte_import_context_t *context,
    vitte_import_module_t *module,
    vitte_import_module_id_t dependent)
{
    size_t index;
    size_t required;

    if (context == NULL || module == NULL) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);
    }

    for (index = 0u;
         index < module->dependent_count;
         ++index) {
        if (module->dependents[index] == dependent) {
            return true;
        }
    }

    if (!vitte_import_size_add(
            module->dependent_count,
            (size_t)1u,
            &required)) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);
    }

    if (!vitte_import_reserve_id_list(
            context,
            &module->dependents,
            &module->dependent_capacity,
            required)) {
        return false;
    }

    module->dependents[module->dependent_count] =
        dependent;

    module->dependent_count = required;

    return true;
}

static bool
vitte_import_module_add_edge(
    vitte_import_context_t *context,
    vitte_import_module_t *module,
    vitte_import_edge_id_t edge)
{
    size_t required;

    if (context == NULL || module == NULL) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);
    }

    if (!vitte_import_size_add(
            module->import_count,
            (size_t)1u,
            &required)) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);
    }

    if (!vitte_import_reserve_raw(
            context,
            (void **)&module->imports,
            &module->import_capacity,
            required,
            sizeof(*module->imports))) {
        return false;
    }

    module->imports[module->import_count] = edge;
    module->import_count = required;

    return true;
}

/* ========================================================================= */
/* Import edges                                                              */
/* ========================================================================= */

vitte_import_edge_id_t
vitte_import_add(
    vitte_import_context_t *context,
    const vitte_import_desc_t *description)
{
    vitte_import_module_t *source;
    vitte_import_module_t *target;
    size_t required;
    size_t index;
    vitte_import_edge_t *edge;

    if (!vitte_import_is_mutable(context)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_STATE);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (description == NULL) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_module_id_exists(
            context,
            description->source_module)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_MODULE);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_string_id_exists(
            context,
            description->module_name)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_STRING);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (description->alias != VITTE_IMPORT_INVALID_ID &&
        !vitte_import_string_id_exists(
            context,
            description->alias)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_STRING);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_visibility_valid(
            description->visibility)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);

        return VITTE_IMPORT_INVALID_ID;
    }

    source =
        &context->modules[
            (size_t)(
                description->source_module -
                UINT64_C(1))];

    for (index = 0u;
         index < source->import_count;
         ++index) {
        const vitte_import_edge_id_t edge_id =
            source->imports[index];

        if (!vitte_import_edge_id_exists(
                context,
                edge_id)) {
            vitte_import_fail(
                context,
                VITTE_IMPORT_ERROR_CORRUPTION);

            return VITTE_IMPORT_INVALID_ID;
        }

        edge =
            &context->edges[
                (size_t)(edge_id - UINT64_C(1))];

        if (edge->module_name ==
                description->module_name &&
            edge->alias == description->alias) {
            vitte_import_fail(
                context,
                VITTE_IMPORT_ERROR_DUPLICATE_IMPORT);

            return VITTE_IMPORT_INVALID_ID;
        }
    }

    if (context->edge_count >= context->max_imports) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_TOO_MANY_IMPORTS);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_size_add(
            context->edge_count,
            (size_t)1u,
            &required)) {
        vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);

        return VITTE_IMPORT_INVALID_ID;
    }

    if (!vitte_import_reserve_edges(
            context,
            required)) {
        return VITTE_IMPORT_INVALID_ID;
    }

    edge = &context->edges[context->edge_count];

    memset(edge, 0, sizeof(*edge));

    edge->id =
        (vitte_import_edge_id_t)(
            context->edge_count + (size_t)1u);

    edge->source_module = description->source_module;
    edge->module_name = description->module_name;
    edge->alias = description->alias;

    edge->visibility = description->visibility;
    edge->span = description->span;

    edge->state = VITTE_IMPORT_EDGE_UNRESOLVED;

    context->edge_count = required;

    if (!vitte_import_module_add_edge(
            context,
            source,
            edge->id)) {
        --context->edge_count;
        memset(edge, 0, sizeof(*edge));
        return VITTE_IMPORT_INVALID_ID;
    }

    /*
     * Resolve eagerly when the target module is already registered.
     * The full resolution pass will resolve forward imports.
     */
    edge->target_module =
        vitte_import_find_module_by_name_internal(
            context,
            edge->module_name);

    if (edge->target_module != VITTE_IMPORT_INVALID_ID) {
        if (edge->target_module == edge->source_module) {
            --source->import_count;
            --context->edge_count;

            memset(edge, 0, sizeof(*edge));

            vitte_import_fail(
                context,
                VITTE_IMPORT_ERROR_SELF_IMPORT);

            return VITTE_IMPORT_INVALID_ID;
        }

        target =
            &context->modules[
                (size_t)(
                    edge->target_module -
                    UINT64_C(1))];

        if (!vitte_import_module_add_dependency(
                context,
                source,
                target->id) ||
            !vitte_import_module_add_dependent(
                context,
                target,
                source->id)) {
            /*
             * The context remains valid but this operation failed.
             * reset() is the safest recovery path after OOM.
             */
            edge->state = VITTE_IMPORT_EDGE_FAILED;
            return VITTE_IMPORT_INVALID_ID;
        }

        edge->state = VITTE_IMPORT_EDGE_RESOLVED;
    }

    context->stats.imports_registered =
        vitte_import_u64_add_sat(
            context->stats.imports_registered,
            UINT64_C(1));

    context->last_error = VITTE_IMPORT_ERROR_NONE;

    return edge->id;
}

const vitte_import_edge_t *
vitte_import_get(
    const vitte_import_context_t *context,
    vitte_import_edge_id_t id)
{
    if (!vitte_import_edge_id_exists(context, id)) {
        return NULL;
    }

    return &context->edges[(size_t)(id - UINT64_C(1))];
}

/* ========================================================================= */
/* Resolution                                                                */
/* ========================================================================= */

static bool
vitte_import_resolve_edge(
    vitte_import_context_t *context,
    vitte_import_edge_t *edge)
{
    vitte_import_module_t *source;
    vitte_import_module_t *target;

    if (context == NULL || edge == NULL) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);
    }

    if (!vitte_import_module_id_exists(
            context,
            edge->source_module)) {
        edge->state = VITTE_IMPORT_EDGE_FAILED;

        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_MODULE);
    }

    source =
        &context->modules[
            (size_t)(
                edge->source_module -
                UINT64_C(1))];

    edge->target_module =
        vitte_import_find_module_by_name_internal(
            context,
            edge->module_name);

    if (edge->target_module == VITTE_IMPORT_INVALID_ID) {
        edge->state = VITTE_IMPORT_EDGE_FAILED;

        context->stats.unresolved_imports =
            vitte_import_u64_add_sat(
                context->stats.unresolved_imports,
                UINT64_C(1));

        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_IMPORT_NOT_FOUND);
    }

    if (edge->target_module == edge->source_module) {
        edge->state = VITTE_IMPORT_EDGE_FAILED;

        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_SELF_IMPORT);
    }

    target =
        &context->modules[
            (size_t)(
                edge->target_module -
                UINT64_C(1))];

    if (!vitte_import_module_add_dependency(
            context,
            source,
            target->id)) {
        edge->state = VITTE_IMPORT_EDGE_FAILED;
        return false;
    }

    if (!vitte_import_module_add_dependent(
            context,
            target,
            source->id)) {
        edge->state = VITTE_IMPORT_EDGE_FAILED;
        return false;
    }

    edge->state = VITTE_IMPORT_EDGE_RESOLVED;

    context->stats.imports_resolved =
        vitte_import_u64_add_sat(
            context->stats.imports_resolved,
            UINT64_C(1));

    return true;
}

static void
vitte_import_clear_resolution_state(
    vitte_import_context_t *context)
{
    size_t index;

    if (context == NULL) {
        return;
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        vitte_import_module_t *module =
            &context->modules[index];

        module->state =
            VITTE_IMPORT_MODULE_REGISTERED;

        module->dependency_count = 0u;
        module->dependent_count = 0u;
    }

    for (index = 0u;
         index < context->edge_count;
         ++index) {
        vitte_import_edge_t *edge =
            &context->edges[index];

        edge->target_module = VITTE_IMPORT_INVALID_ID;
        edge->state = VITTE_IMPORT_EDGE_UNRESOLVED;
    }

    context->topological_count = 0u;
}

bool
vitte_import_resolve(
    vitte_import_context_t *context)
{
    size_t index;

    if (!vitte_import_context_envelope_valid(context)) {
        return false;
    }

    if (context->state != VITTE_IMPORT_STATE_BUILDING &&
        context->state != VITTE_IMPORT_STATE_FAILED) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_STATE);
    }

    context->stats.resolution_runs =
        vitte_import_u64_add_sat(
            context->stats.resolution_runs,
            UINT64_C(1));

    context->state = VITTE_IMPORT_STATE_RESOLVING;
    context->last_error = VITTE_IMPORT_ERROR_NONE;

    vitte_import_clear_resolution_state(context);

    for (index = 0u;
         index < context->edge_count;
         ++index) {
        if (!vitte_import_resolve_edge(
                context,
                &context->edges[index])) {
            context->state = VITTE_IMPORT_STATE_FAILED;

            context->stats.resolution_failures =
                vitte_import_u64_add_sat(
                    context->stats.resolution_failures,
                    UINT64_C(1));

            return false;
        }
    }

    if (!vitte_import_build_topological_order(context)) {
        context->state = VITTE_IMPORT_STATE_FAILED;

        context->stats.resolution_failures =
            vitte_import_u64_add_sat(
                context->stats.resolution_failures,
                UINT64_C(1));

        return false;
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        context->modules[index].state =
            VITTE_IMPORT_MODULE_RESOLVED;
    }

    context->state = VITTE_IMPORT_STATE_RESOLVED;
    context->last_error = VITTE_IMPORT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Cycle detection / topological ordering                                    */
/* ========================================================================= */

static bool
vitte_import_dfs_visit(
    vitte_import_context_t *context,
    vitte_import_module_id_t module_id,
    uint8_t *marks,
    vitte_import_module_id_t *stack,
    size_t *stack_count,
    vitte_import_module_id_t *order,
    size_t *order_count)
{
    size_t module_index;
    vitte_import_module_t *module;
    size_t dependency_index;

    if (context == NULL ||
        marks == NULL ||
        stack == NULL ||
        stack_count == NULL ||
        order == NULL ||
        order_count == NULL) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);
    }

    if (!vitte_import_module_id_exists(
            context,
            module_id)) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_MODULE);
    }

    module_index =
        (size_t)(module_id - UINT64_C(1));

    if (marks[module_index] == (uint8_t)2u) {
        return true;
    }

    if (marks[module_index] == (uint8_t)1u) {
        context->stats.cycles_detected =
            vitte_import_u64_add_sat(
                context->stats.cycles_detected,
                UINT64_C(1));

        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_CYCLE);
    }

    marks[module_index] = (uint8_t)1u;

    stack[*stack_count] = module_id;
    *stack_count += (size_t)1u;

    module = &context->modules[module_index];

    module->state =
        VITTE_IMPORT_MODULE_RESOLVING;

    for (dependency_index = 0u;
         dependency_index < module->dependency_count;
         ++dependency_index) {
        const vitte_import_module_id_t dependency =
            module->dependencies[dependency_index];

        if (!vitte_import_dfs_visit(
                context,
                dependency,
                marks,
                stack,
                stack_count,
                order,
                order_count)) {
            module->state =
                VITTE_IMPORT_MODULE_FAILED;

            return false;
        }
    }

    if (*stack_count == 0u) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INTERNAL);
    }

    *stack_count -= (size_t)1u;

    marks[module_index] = (uint8_t)2u;

    module->state =
        VITTE_IMPORT_MODULE_RESOLVED;

    order[*order_count] = module_id;
    *order_count += (size_t)1u;

    return true;
}

static bool
vitte_import_build_topological_order(
    vitte_import_context_t *context)
{
    uint8_t *marks;
    vitte_import_module_id_t *stack;
    vitte_import_module_id_t *order;
    size_t order_count;
    size_t stack_count;
    size_t index;
    size_t bytes;

    if (context == NULL) {
        return false;
    }

    context->topological_count = 0u;

    if (context->module_count == 0u) {
        return true;
    }

    if (!vitte_import_size_mul(
            context->module_count,
            sizeof(*marks),
            &bytes)) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);
    }

    marks = (uint8_t *)calloc(
        context->module_count,
        sizeof(*marks));

    if (marks == NULL) {
        context->stats.allocation_failures =
            vitte_import_u64_add_sat(
                context->stats.allocation_failures,
                UINT64_C(1));

        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OUT_OF_MEMORY);
    }

    (void)bytes;

    context->stats.allocations =
        vitte_import_u64_add_sat(
            context->stats.allocations,
            UINT64_C(1));

    if (!vitte_import_size_mul(
            context->module_count,
            sizeof(*stack),
            &bytes)) {
        free(marks);

        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OVERFLOW);
    }

    stack = (vitte_import_module_id_t *)malloc(bytes);

    if (stack == NULL) {
        free(marks);

        context->stats.allocation_failures =
            vitte_import_u64_add_sat(
                context->stats.allocation_failures,
                UINT64_C(1));

        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_OUT_OF_MEMORY);
    }

    context->stats.allocations =
        vitte_import_u64_add_sat(
            context->stats.allocations,
            UINT64_C(1));

    order = NULL;

    if (!vitte_import_reserve_id_list(
            context,
            &order,
            &context->topological_capacity,
            context->module_count)) {
        free(stack);
        free(marks);
        return false;
    }

    /*
     * reserve_id_list() cannot directly update context->topological_order
     * when using a temporary pointer, therefore transfer it now.
     */
    free(context->topological_order);
    context->topological_order = order;

    order_count = 0u;
    stack_count = 0u;

    for (index = 0u;
         index < context->module_count;
         ++index) {
        const vitte_import_module_id_t module_id =
            (vitte_import_module_id_t)(
                index + (size_t)1u);

        if (marks[index] != (uint8_t)0u) {
            continue;
        }

        if (!vitte_import_dfs_visit(
                context,
                module_id,
                marks,
                stack,
                &stack_count,
                context->topological_order,
                &order_count)) {
            free(stack);
            free(marks);

            context->topological_count = 0u;
            return false;
        }
    }

    context->topological_count = order_count;

    free(stack);
    free(marks);

    return true;
}

/* ========================================================================= */
/* Topological order                                                         */
/* ========================================================================= */

size_t
vitte_import_topological_count(
    const vitte_import_context_t *context)
{
    if (!vitte_import_context_envelope_valid(context)) {
        return 0u;
    }

    return context->topological_count;
}

vitte_import_module_id_t
vitte_import_topological_at(
    const vitte_import_context_t *context,
    size_t index)
{
    if (!vitte_import_context_envelope_valid(context)) {
        return VITTE_IMPORT_INVALID_ID;
    }

    if (index >= context->topological_count ||
        context->topological_order == NULL) {
        return VITTE_IMPORT_INVALID_ID;
    }

    return context->topological_order[index];
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

static bool
vitte_import_validate_string(
    const vitte_import_context_t *context,
    const vitte_import_string_t *string)
{
    uint64_t hash;

    if (context == NULL || string == NULL) {
        return false;
    }

    if (string->id == VITTE_IMPORT_INVALID_ID) {
        return false;
    }

    if (string->data == NULL) {
        return false;
    }

    if (string->data[string->length] != '\0') {
        return false;
    }

    hash =
        vitte_import_hash_bytes(
            VITTE_IMPORT_FNV_OFFSET,
            string->data,
            string->length);

    return hash == string->hash;
}

static bool
vitte_import_validate_module(
    const vitte_import_context_t *context,
    const vitte_import_module_t *module)
{
    size_t index;

    if (context == NULL || module == NULL) {
        return false;
    }

    if (!vitte_import_module_id_exists(
            context,
            module->id)) {
        return false;
    }

    if (!vitte_import_string_id_exists(
            context,
            module->name)) {
        return false;
    }

    if (module->path != VITTE_IMPORT_INVALID_ID &&
        !vitte_import_string_id_exists(
            context,
            module->path)) {
        return false;
    }

    if (!vitte_import_visibility_valid(
            module->visibility)) {
        return false;
    }

    if (!vitte_import_module_state_valid(
            module->state)) {
        return false;
    }

    if (module->dependency_count >
        module->dependency_capacity) {
        return false;
    }

    if (module->dependent_count >
        module->dependent_capacity) {
        return false;
    }

    if (module->import_count >
        module->import_capacity) {
        return false;
    }

    if (module->dependency_count != 0u &&
        module->dependencies == NULL) {
        return false;
    }

    if (module->dependent_count != 0u &&
        module->dependents == NULL) {
        return false;
    }

    if (module->import_count != 0u &&
        module->imports == NULL) {
        return false;
    }

    for (index = 0u;
         index < module->dependency_count;
         ++index) {
        if (!vitte_import_module_id_exists(
                context,
                module->dependencies[index])) {
            return false;
        }

        if (module->dependencies[index] == module->id) {
            return false;
        }
    }

    for (index = 0u;
         index < module->dependent_count;
         ++index) {
        if (!vitte_import_module_id_exists(
                context,
                module->dependents[index])) {
            return false;
        }

        if (module->dependents[index] == module->id) {
            return false;
        }
    }

    for (index = 0u;
         index < module->import_count;
         ++index) {
        if (!vitte_import_edge_id_exists(
                context,
                module->imports[index])) {
            return false;
        }
    }

    return true;
}

static bool
vitte_import_validate_edge(
    const vitte_import_context_t *context,
    const vitte_import_edge_t *edge)
{
    if (context == NULL || edge == NULL) {
        return false;
    }

    if (!vitte_import_edge_id_exists(
            context,
            edge->id)) {
        return false;
    }

    if (!vitte_import_module_id_exists(
            context,
            edge->source_module)) {
        return false;
    }

    if (!vitte_import_string_id_exists(
            context,
            edge->module_name)) {
        return false;
    }

    if (edge->alias != VITTE_IMPORT_INVALID_ID &&
        !vitte_import_string_id_exists(
            context,
            edge->alias)) {
        return false;
    }

    if (!vitte_import_visibility_valid(
            edge->visibility)) {
        return false;
    }

    if (!vitte_import_edge_state_valid(
            edge->state)) {
        return false;
    }

    if (edge->state == VITTE_IMPORT_EDGE_RESOLVED &&
        !vitte_import_module_id_exists(
            context,
            edge->target_module)) {
        return false;
    }

    if (edge->target_module != VITTE_IMPORT_INVALID_ID &&
        edge->target_module == edge->source_module) {
        return false;
    }

    if (edge->span.valid &&
        edge->span.end < edge->span.begin) {
        return false;
    }

    return true;
}

bool
vitte_import_validate(
    vitte_import_context_t *context)
{
    size_t index;
    size_t total_string_bytes;

    if (!vitte_import_context_envelope_valid(context)) {
        return false;
    }

    context->stats.validation_runs =
        vitte_import_u64_add_sat(
            context->stats.validation_runs,
            UINT64_C(1));

    total_string_bytes = 0u;

    for (index = 0u;
         index < context->string_count;
         ++index) {
        const vitte_import_string_t *string =
            &context->strings[index];

        if (string->id !=
            (vitte_import_string_id_t)(
                index + (size_t)1u)) {
            goto validation_failure;
        }

        if (!vitte_import_validate_string(
                context,
                string)) {
            goto validation_failure;
        }

        if (!vitte_import_size_add(
                total_string_bytes,
                string->length,
                &total_string_bytes)) {
            context->last_error =
                VITTE_IMPORT_ERROR_OVERFLOW;

            goto validation_failure;
        }
    }

    if (total_string_bytes != context->string_bytes) {
        goto validation_failure;
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        const vitte_import_module_t *module =
            &context->modules[index];

        if (module->id !=
            (vitte_import_module_id_t)(
                index + (size_t)1u)) {
            goto validation_failure;
        }

        if (!vitte_import_validate_module(
                context,
                module)) {
            goto validation_failure;
        }
    }

    for (index = 0u;
         index < context->edge_count;
         ++index) {
        const vitte_import_edge_t *edge =
            &context->edges[index];

        if (edge->id !=
            (vitte_import_edge_id_t)(
                index + (size_t)1u)) {
            goto validation_failure;
        }

        if (!vitte_import_validate_edge(
                context,
                edge)) {
            goto validation_failure;
        }
    }

    if (context->state == VITTE_IMPORT_STATE_RESOLVED) {
        if (context->topological_count !=
            context->module_count) {
            goto validation_failure;
        }

        for (index = 0u;
             index < context->topological_count;
             ++index) {
            size_t previous;

            if (!vitte_import_module_id_exists(
                    context,
                    context->topological_order[index])) {
                goto validation_failure;
            }

            for (previous = 0u;
                 previous < index;
                 ++previous) {
                if (context->topological_order[previous] ==
                    context->topological_order[index]) {
                    goto validation_failure;
                }
            }
        }
    }

    context->last_error = VITTE_IMPORT_ERROR_NONE;
    return true;

validation_failure:

    context->stats.validation_failures =
        vitte_import_u64_add_sat(
            context->stats.validation_failures,
            UINT64_C(1));

    if (context->last_error ==
        VITTE_IMPORT_ERROR_NONE) {
        context->last_error =
            VITTE_IMPORT_ERROR_VALIDATION;
    }

    return false;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_import_fingerprint(
    vitte_import_context_t *context)
{
    uint64_t hash;
    size_t index;

    if (!vitte_import_context_envelope_valid(context)) {
        return UINT64_C(0);
    }

    context->stats.hash_runs =
        vitte_import_u64_add_sat(
            context->stats.hash_runs,
            UINT64_C(1));

    hash = VITTE_IMPORT_FNV_OFFSET;

    hash = vitte_import_hash_u64(
        hash,
        (uint64_t)VITTE_IMPORT_API_VERSION_MAJOR);

    hash = vitte_import_hash_u64(
        hash,
        (uint64_t)VITTE_IMPORT_API_VERSION_MINOR);

    hash = vitte_import_hash_u64(
        hash,
        (uint64_t)VITTE_IMPORT_API_VERSION_PATCH);

    hash = vitte_import_hash_size(
        hash,
        context->module_count);

    hash = vitte_import_hash_size(
        hash,
        context->edge_count);

    hash = vitte_import_hash_size(
        hash,
        context->string_count);

    /*
     * Important:
     * bounds are tested before array dereference.
     */
    for (index = 0u;
         index < context->string_count;
         ++index) {
        const vitte_import_string_t *string =
            &context->strings[index];

        hash = vitte_import_hash_u64(
            hash,
            string->id);

        hash = vitte_import_hash_size(
            hash,
            string->length);

        hash = vitte_import_hash_bytes(
            hash,
            string->data,
            string->length);
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        const vitte_import_module_t *module =
            &context->modules[index];

        size_t dependency_index;

        hash = vitte_import_hash_u64(
            hash,
            module->id);

        hash = vitte_import_hash_u64(
            hash,
            module->name);

        hash = vitte_import_hash_u64(
            hash,
            module->path);

        hash = vitte_import_hash_u64(
            hash,
            module->source_id);

        hash = vitte_import_hash_u64(
            hash,
            (uint64_t)module->visibility);

        hash = vitte_import_hash_bool(
            hash,
            module->is_root);

        hash = vitte_import_hash_bool(
            hash,
            module->is_external);

        hash = vitte_import_hash_size(
            hash,
            module->dependency_count);

        for (dependency_index = 0u;
             dependency_index < module->dependency_count;
             ++dependency_index) {
            hash = vitte_import_hash_u64(
                hash,
                module->dependencies[dependency_index]);
        }
    }

    for (index = 0u;
         index < context->edge_count;
         ++index) {
        const vitte_import_edge_t *edge =
            &context->edges[index];

        hash = vitte_import_hash_u64(
            hash,
            edge->id);

        hash = vitte_import_hash_u64(
            hash,
            edge->source_module);

        hash = vitte_import_hash_u64(
            hash,
            edge->target_module);

        hash = vitte_import_hash_u64(
            hash,
            edge->module_name);

        hash = vitte_import_hash_u64(
            hash,
            edge->alias);

        hash = vitte_import_hash_u64(
            hash,
            (uint64_t)edge->visibility);
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_import_stats_t
vitte_import_stats(
    const vitte_import_context_t *context)
{
    vitte_import_stats_t stats;

    memset(&stats, 0, sizeof(stats));

    if (!vitte_import_context_envelope_valid(context)) {
        return stats;
    }

    return context->stats;
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_import_set_limits(
    vitte_import_context_t *context,
    size_t max_modules,
    size_t max_imports,
    size_t max_string_bytes)
{
    if (!vitte_import_is_mutable(context)) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_STATE);
    }

    if (max_modules == 0u ||
        max_imports == 0u ||
        max_string_bytes == 0u) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);
    }

    if (max_modules < context->module_count ||
        max_imports < context->edge_count ||
        max_string_bytes < context->string_bytes) {
        return vitte_import_fail(
            context,
            VITTE_IMPORT_ERROR_INVALID_ARGUMENT);
    }

    context->max_modules = max_modules;
    context->max_imports = max_imports;
    context->max_string_bytes = max_string_bytes;

    context->last_error = VITTE_IMPORT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_import_translation_unit_anchor(void)
{
    /*
     * Deliberately empty.
     *
     * Useful to force this translation unit into static archives and for
     * build-system/linkage diagnostics.
     */
}
