/*
 * Vitte Compiler
 * src/backend/c17/program.c
 *
 * Whole-program model for the ISO C17 backend.
 *
 * This module owns the backend-level representation of a complete Vitte
 * program before final C17 translation-unit emission.
 *
 * Responsibilities:
 *
 *   - program lifecycle
 *   - module registration
 *   - stable module identity
 *   - module lookup
 *   - entry-module selection
 *   - entry-symbol metadata
 *   - module dependency graph
 *   - duplicate detection
 *   - deterministic ordering
 *   - topological ordering
 *   - dependency-cycle detection
 *   - reachability
 *   - root-module discovery
 *   - imported/exported symbol registry
 *   - generated C symbol registry
 *   - whole-program feature aggregation
 *   - runtime requirement aggregation
 *   - whole-program validation
 *   - statistics
 *   - stable program fingerprinting
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
 *          +---- naming.c
 *          +---- options.c
 *          +---- runtime.c
 *          +---- symbol.c
 *          |
 *          v
 *   translation_unit.c
 *          |
 *          v
 *       backend.c
 *          |
 *          v
 *    generated ISO C17
 *
 * program.c deliberately does not emit C text. It models, validates and
 * orders the complete backend program. Emission belongs to the translation
 * unit/declaration/expression/statement layers.
 */

#include "program.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_C17_PROGRAM_MAGIC
#define VITTE_C17_PROGRAM_MAGIC UINT64_C(0x5649545445505247)
#endif

#ifndef VITTE_C17_PROGRAM_DEAD_MAGIC
#define VITTE_C17_PROGRAM_DEAD_MAGIC UINT64_C(0x4445414450524F47)
#endif

#ifndef VITTE_C17_PROGRAM_INITIAL_MODULE_CAPACITY
#define VITTE_C17_PROGRAM_INITIAL_MODULE_CAPACITY ((size_t)16u)
#endif

#ifndef VITTE_C17_PROGRAM_INITIAL_DEPENDENCY_CAPACITY
#define VITTE_C17_PROGRAM_INITIAL_DEPENDENCY_CAPACITY ((size_t)32u)
#endif

#ifndef VITTE_C17_PROGRAM_INITIAL_SYMBOL_CAPACITY
#define VITTE_C17_PROGRAM_INITIAL_SYMBOL_CAPACITY ((size_t)64u)
#endif

#ifndef VITTE_C17_PROGRAM_MAX_MODULES
#define VITTE_C17_PROGRAM_MAX_MODULES ((size_t)(1024u * 1024u))
#endif

#ifndef VITTE_C17_PROGRAM_MAX_DEPENDENCIES
#define VITTE_C17_PROGRAM_MAX_DEPENDENCIES ((size_t)(16u * 1024u * 1024u))
#endif

#ifndef VITTE_C17_PROGRAM_MAX_SYMBOLS
#define VITTE_C17_PROGRAM_MAX_SYMBOLS ((size_t)(16u * 1024u * 1024u))
#endif

#ifndef VITTE_C17_PROGRAM_MAX_NAME_BYTES
#define VITTE_C17_PROGRAM_MAX_NAME_BYTES ((size_t)(64u * 1024u))
#endif

#ifndef VITTE_C17_PROGRAM_HASH_OFFSET
#define VITTE_C17_PROGRAM_HASH_OFFSET UINT64_C(14695981039346656037)
#endif

#ifndef VITTE_C17_PROGRAM_HASH_PRIME
#define VITTE_C17_PROGRAM_HASH_PRIME UINT64_C(1099511628211)
#endif

/* ========================================================================= */
/* Internal arithmetic                                                       */
/* ========================================================================= */

static bool
vitte_c17_program_add_overflow(
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
vitte_c17_program_mul_overflow(
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
vitte_c17_program_saturating_add(
    size_t left,
    size_t right)
{
    size_t result;

    if (vitte_c17_program_add_overflow(
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
vitte_c17_program_hash_bytes(
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
        hash *= VITTE_C17_PROGRAM_HASH_PRIME;
    }

    return hash;
}

static uint64_t
vitte_c17_program_hash_u64(
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
        vitte_c17_program_hash_bytes(
            hash,
            bytes,
            sizeof(bytes));
}

static uint64_t
vitte_c17_program_hash_size(
    uint64_t hash,
    size_t value)
{
    return
        vitte_c17_program_hash_u64(
            hash,
            (uint64_t)value);
}

static uint64_t
vitte_c17_program_hash_bool(
    uint64_t hash,
    bool value)
{
    unsigned char byte;

    byte =
        value
            ? (unsigned char)1u
            : (unsigned char)0u;

    return
        vitte_c17_program_hash_bytes(
            hash,
            &byte,
            1u);
}

/* ========================================================================= */
/* String helpers                                                            */
/* ========================================================================= */

static char *
vitte_c17_program_copy_string(
    const char *text,
    size_t length)
{
    char *copy;
    size_t allocation_size;

    if (text == NULL) {
        return NULL;
    }

    if (length >
        VITTE_C17_PROGRAM_MAX_NAME_BYTES) {
        return NULL;
    }

    if (vitte_c17_program_add_overflow(
            length,
            1u,
            &allocation_size)) {
        return NULL;
    }

    copy =
        (char *)malloc(
            allocation_size);

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

static bool
vitte_c17_program_string_equal(
    const char *left,
    size_t left_length,
    const char *right,
    size_t right_length)
{
    if (left_length != right_length) {
        return false;
    }

    if (left_length == 0u) {
        return true;
    }

    if (left == NULL ||
        right == NULL) {
        return false;
    }

    return
        memcmp(
            left,
            right,
            left_length) == 0;
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_c17_program_error_name(
    vitte_c17_program_error_t error)
{
    switch (error) {
        case VITTE_C17_PROGRAM_ERROR_NONE:
            return "none";

        case VITTE_C17_PROGRAM_ERROR_INVALID_PROGRAM:
            return "invalid-program";

        case VITTE_C17_PROGRAM_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_C17_PROGRAM_ERROR_INVALID_MODULE:
            return "invalid-module";

        case VITTE_C17_PROGRAM_ERROR_INVALID_SYMBOL:
            return "invalid-symbol";

        case VITTE_C17_PROGRAM_ERROR_DUPLICATE_MODULE:
            return "duplicate-module";

        case VITTE_C17_PROGRAM_ERROR_DUPLICATE_SYMBOL:
            return "duplicate-symbol";

        case VITTE_C17_PROGRAM_ERROR_UNKNOWN_MODULE:
            return "unknown-module";

        case VITTE_C17_PROGRAM_ERROR_UNKNOWN_SYMBOL:
            return "unknown-symbol";

        case VITTE_C17_PROGRAM_ERROR_SELF_DEPENDENCY:
            return "self-dependency";

        case VITTE_C17_PROGRAM_ERROR_DUPLICATE_DEPENDENCY:
            return "duplicate-dependency";

        case VITTE_C17_PROGRAM_ERROR_DEPENDENCY_CYCLE:
            return "dependency-cycle";

        case VITTE_C17_PROGRAM_ERROR_ENTRY_NOT_FOUND:
            return "entry-not-found";

        case VITTE_C17_PROGRAM_ERROR_MULTIPLE_ENTRY:
            return "multiple-entry";

        case VITTE_C17_PROGRAM_ERROR_LIMIT_EXCEEDED:
            return "limit-exceeded";

        case VITTE_C17_PROGRAM_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";

        case VITTE_C17_PROGRAM_ERROR_CORRUPTION:
            return "corruption";

        default:
            return "unknown";
    }
}

const char *
vitte_c17_program_symbol_kind_name(
    vitte_c17_program_symbol_kind_t kind)
{
    switch (kind) {
        case VITTE_C17_PROGRAM_SYMBOL_FUNCTION:
            return "function";

        case VITTE_C17_PROGRAM_SYMBOL_GLOBAL:
            return "global";

        case VITTE_C17_PROGRAM_SYMBOL_TYPE:
            return "type";

        case VITTE_C17_PROGRAM_SYMBOL_CONSTANT:
            return "constant";

        case VITTE_C17_PROGRAM_SYMBOL_RUNTIME:
            return "runtime";

        case VITTE_C17_PROGRAM_SYMBOL_INTERNAL:
            return "internal";

        default:
            return "invalid";
    }
}

const char *
vitte_c17_program_symbol_visibility_name(
    vitte_c17_program_symbol_visibility_t visibility)
{
    switch (visibility) {
        case VITTE_C17_PROGRAM_SYMBOL_PRIVATE:
            return "private";

        case VITTE_C17_PROGRAM_SYMBOL_PUBLIC:
            return "public";

        case VITTE_C17_PROGRAM_SYMBOL_EXTERNAL:
            return "external";

        default:
            return "invalid";
    }
}

/* ========================================================================= */
/* Failure helper                                                            */
/* ========================================================================= */

static bool
vitte_c17_program_fail(
    vitte_c17_program_t *program,
    vitte_c17_program_error_t error)
{
    if (program != NULL &&
        program->magic ==
            VITTE_C17_PROGRAM_MAGIC) {
        program->last_error = error;
    }

    return false;
}

/* ========================================================================= */
/* Validity                                                                  */
/* ========================================================================= */

bool
vitte_c17_program_is_valid(
    const vitte_c17_program_t *program)
{
    if (program == NULL) {
        return false;
    }

    if (program->magic !=
        VITTE_C17_PROGRAM_MAGIC) {
        return false;
    }

    if (program->module_count >
        program->module_capacity) {
        return false;
    }

    if (program->dependency_count >
        program->dependency_capacity) {
        return false;
    }

    if (program->symbol_count >
        program->symbol_capacity) {
        return false;
    }

    if (program->module_count != 0u &&
        program->modules == NULL) {
        return false;
    }

    if (program->dependency_count != 0u &&
        program->dependencies == NULL) {
        return false;
    }

    if (program->symbol_count != 0u &&
        program->symbols == NULL) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_c17_program_config_t
vitte_c17_program_config_default(void)
{
    vitte_c17_program_config_t config;

    memset(
        &config,
        0,
        sizeof(config));

    config.max_modules =
        VITTE_C17_PROGRAM_MAX_MODULES;

    config.max_dependencies =
        VITTE_C17_PROGRAM_MAX_DEPENDENCIES;

    config.max_symbols =
        VITTE_C17_PROGRAM_MAX_SYMBOLS;

    config.max_name_bytes =
        VITTE_C17_PROGRAM_MAX_NAME_BYTES;

    config.deterministic = true;

    config.require_entry = true;

    config.reject_cycles = true;

    config.reject_duplicate_symbols = true;

    config.reject_duplicate_dependencies = true;

    config.validate_modules = true;

    config.include_unreachable_modules = false;

    return config;
}

static bool
vitte_c17_program_config_valid(
    const vitte_c17_program_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->max_modules == 0u ||
        config->max_modules >
            VITTE_C17_PROGRAM_MAX_MODULES) {
        return false;
    }

    if (config->max_dependencies == 0u ||
        config->max_dependencies >
            VITTE_C17_PROGRAM_MAX_DEPENDENCIES) {
        return false;
    }

    if (config->max_symbols == 0u ||
        config->max_symbols >
            VITTE_C17_PROGRAM_MAX_SYMBOLS) {
        return false;
    }

    if (config->max_name_bytes == 0u ||
        config->max_name_bytes >
            VITTE_C17_PROGRAM_MAX_NAME_BYTES) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_c17_program_init(
    vitte_c17_program_t *program,
    const vitte_c17_program_config_t *config)
{
    vitte_c17_program_config_t effective;

    if (program == NULL) {
        return false;
    }

    effective =
        config != NULL
            ? *config
            : vitte_c17_program_config_default();

    if (!vitte_c17_program_config_valid(
            &effective)) {
        return false;
    }

    memset(
        program,
        0,
        sizeof(*program));

    program->magic =
        VITTE_C17_PROGRAM_MAGIC;

    program->config = effective;

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    program->next_module_id =
        UINT64_C(1);

    program->next_symbol_id =
        UINT64_C(1);

    program->generation =
        UINT64_C(1);

    return true;
}

static void
vitte_c17_program_destroy_module_entry(
    vitte_c17_program_module_t *entry)
{
    if (entry == NULL) {
        return;
    }

    free(entry->name);

    entry->name = NULL;
    entry->name_length = 0u;
    entry->module = NULL;
    entry->id = UINT64_C(0);
}

static void
vitte_c17_program_destroy_symbol_entry(
    vitte_c17_program_symbol_t *symbol)
{
    if (symbol == NULL) {
        return;
    }

    free(symbol->source_name);
    free(symbol->generated_name);

    symbol->source_name = NULL;
    symbol->generated_name = NULL;

    symbol->source_name_length = 0u;
    symbol->generated_name_length = 0u;
}

void
vitte_c17_program_reset(
    vitte_c17_program_t *program)
{
    size_t index;

    if (!vitte_c17_program_is_valid(
            program)) {
        return;
    }

    for (index = 0u;
         index < program->module_count;
         ++index) {
        vitte_c17_program_destroy_module_entry(
            &program->modules[index]);

        memset(
            &program->modules[index],
            0,
            sizeof(program->modules[index]));
    }

    for (index = 0u;
         index < program->symbol_count;
         ++index) {
        vitte_c17_program_destroy_symbol_entry(
            &program->symbols[index]);

        memset(
            &program->symbols[index],
            0,
            sizeof(program->symbols[index]));
    }

    if (program->dependencies != NULL &&
        program->dependency_count != 0u) {
        memset(
            program->dependencies,
            0,
            program->dependency_count *
                sizeof(program->dependencies[0]));
    }

    free(program->ordered_modules);
    free(program->reachable_modules);

    program->ordered_modules = NULL;
    program->ordered_module_count = 0u;

    program->reachable_modules = NULL;
    program->reachable_module_count = 0u;

    program->module_count = 0u;
    program->dependency_count = 0u;
    program->symbol_count = 0u;

    program->entry_module_id =
        UINT64_C(0);

    program->entry_symbol_id =
        UINT64_C(0);

    program->has_entry = false;

    program->next_module_id =
        UINT64_C(1);

    program->next_symbol_id =
        UINT64_C(1);

    if (program->generation != UINT64_MAX) {
        ++program->generation;
    }

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;
}

void
vitte_c17_program_destroy(
    vitte_c17_program_t *program)
{
    size_t index;

    if (!vitte_c17_program_is_valid(
            program)) {
        return;
    }

    for (index = 0u;
         index < program->module_count;
         ++index) {
        vitte_c17_program_destroy_module_entry(
            &program->modules[index]);
    }

    for (index = 0u;
         index < program->symbol_count;
         ++index) {
        vitte_c17_program_destroy_symbol_entry(
            &program->symbols[index]);
    }

    free(program->modules);
    free(program->dependencies);
    free(program->symbols);
    free(program->ordered_modules);
    free(program->reachable_modules);

    memset(
        program,
        0,
        sizeof(*program));

    program->magic =
        VITTE_C17_PROGRAM_DEAD_MAGIC;
}

/* ========================================================================= */
/* Capacity helpers                                                          */
/* ========================================================================= */

static bool
vitte_c17_program_reserve_raw(
    vitte_c17_program_t *program,
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
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_INVALID_ARGUMENT);
    }

    if (minimum <= *capacity) {
        return true;
    }

    if (minimum > maximum) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_LIMIT_EXCEEDED);
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

    if (new_capacity < minimum ||
        new_capacity > maximum) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_LIMIT_EXCEEDED);
    }

    if (vitte_c17_program_mul_overflow(
            new_capacity,
            element_size,
            &bytes)) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_OVERFLOW);
    }

    replacement =
        realloc(
            *storage,
            bytes);

    if (replacement == NULL) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY);
    }

    *storage = replacement;
    *capacity = new_capacity;

    return true;
}

bool
vitte_c17_program_reserve_modules(
    vitte_c17_program_t *program,
    size_t capacity)
{
    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    return
        vitte_c17_program_reserve_raw(
            program,
            (void **)&program->modules,
            &program->module_capacity,
            capacity,
            VITTE_C17_PROGRAM_INITIAL_MODULE_CAPACITY,
            program->config.max_modules,
            sizeof(program->modules[0]));
}

bool
vitte_c17_program_reserve_dependencies(
    vitte_c17_program_t *program,
    size_t capacity)
{
    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    return
        vitte_c17_program_reserve_raw(
            program,
            (void **)&program->dependencies,
            &program->dependency_capacity,
            capacity,
            VITTE_C17_PROGRAM_INITIAL_DEPENDENCY_CAPACITY,
            program->config.max_dependencies,
            sizeof(program->dependencies[0]));
}

bool
vitte_c17_program_reserve_symbols(
    vitte_c17_program_t *program,
    size_t capacity)
{
    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    return
        vitte_c17_program_reserve_raw(
            program,
            (void **)&program->symbols,
            &program->symbol_capacity,
            capacity,
            VITTE_C17_PROGRAM_INITIAL_SYMBOL_CAPACITY,
            program->config.max_symbols,
            sizeof(program->symbols[0]));
}

/* ========================================================================= */
/* Cache invalidation                                                        */
/* ========================================================================= */

static void
vitte_c17_program_invalidate_order(
    vitte_c17_program_t *program)
{
    free(program->ordered_modules);

    program->ordered_modules = NULL;
    program->ordered_module_count = 0u;

    free(program->reachable_modules);

    program->reachable_modules = NULL;
    program->reachable_module_count = 0u;
}

/* ========================================================================= */
/* Module lookup                                                             */
/* ========================================================================= */

static ptrdiff_t
vitte_c17_program_find_module_index_by_id(
    const vitte_c17_program_t *program,
    uint64_t module_id)
{
    size_t index;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        if (program->modules[index].id ==
            module_id) {
            return (ptrdiff_t)index;
        }
    }

    return -1;
}

static ptrdiff_t
vitte_c17_program_find_module_index_by_pointer(
    const vitte_c17_program_t *program,
    const vitte_c17_module_t *module)
{
    size_t index;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        if (program->modules[index].module ==
            module) {
            return (ptrdiff_t)index;
        }
    }

    return -1;
}

static ptrdiff_t
vitte_c17_program_find_module_index_by_name(
    const vitte_c17_program_t *program,
    const char *name,
    size_t length)
{
    size_t index;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        if (vitte_c17_program_string_equal(
                program->modules[index].name,
                program->modules[index].name_length,
                name,
                length)) {
            return (ptrdiff_t)index;
        }
    }

    return -1;
}

const vitte_c17_program_module_t *
vitte_c17_program_find_module_by_id(
    const vitte_c17_program_t *program,
    uint64_t module_id)
{
    ptrdiff_t index;

    if (!vitte_c17_program_is_valid(
            program)) {
        return NULL;
    }

    index =
        vitte_c17_program_find_module_index_by_id(
            program,
            module_id);

    if (index < 0) {
        return NULL;
    }

    return
        &program->modules[(size_t)index];
}

const vitte_c17_program_module_t *
vitte_c17_program_find_module_by_name(
    const vitte_c17_program_t *program,
    const char *name,
    size_t length)
{
    ptrdiff_t index;

    if (!vitte_c17_program_is_valid(
            program) ||
        name == NULL ||
        length == 0u) {
        return NULL;
    }

    index =
        vitte_c17_program_find_module_index_by_name(
            program,
            name,
            length);

    if (index < 0) {
        return NULL;
    }

    return
        &program->modules[(size_t)index];
}

const vitte_c17_program_module_t *
vitte_c17_program_find_module_by_pointer(
    const vitte_c17_program_t *program,
    const vitte_c17_module_t *module)
{
    ptrdiff_t index;

    if (!vitte_c17_program_is_valid(
            program) ||
        module == NULL) {
        return NULL;
    }

    index =
        vitte_c17_program_find_module_index_by_pointer(
            program,
            module);

    if (index < 0) {
        return NULL;
    }

    return
        &program->modules[(size_t)index];
}

/* ========================================================================= */
/* Module registration                                                       */
/* ========================================================================= */

uint64_t
vitte_c17_program_add_module(
    vitte_c17_program_t *program,
    vitte_c17_module_t *module,
    const char *name,
    size_t name_length)
{
    vitte_c17_program_module_t *entry;
    char *name_copy;
    uint64_t id;

    if (!vitte_c17_program_is_valid(
            program)) {
        return UINT64_C(0);
    }

    if (module == NULL ||
        name == NULL ||
        name_length == 0u) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_INVALID_MODULE);

        return UINT64_C(0);
    }

    if (name_length >
        program->config.max_name_bytes) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_LIMIT_EXCEEDED);

        return UINT64_C(0);
    }

    if (program->module_count >=
        program->config.max_modules) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_LIMIT_EXCEEDED);

        return UINT64_C(0);
    }

    if (vitte_c17_program_find_module_index_by_pointer(
            program,
            module) >= 0 ||
        vitte_c17_program_find_module_index_by_name(
            program,
            name,
            name_length) >= 0) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_DUPLICATE_MODULE);

        return UINT64_C(0);
    }

    if (program->next_module_id == UINT64_MAX) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_OVERFLOW);

        return UINT64_C(0);
    }

    if (!vitte_c17_program_reserve_modules(
            program,
            program->module_count + 1u)) {
        return UINT64_C(0);
    }

    name_copy =
        vitte_c17_program_copy_string(
            name,
            name_length);

    if (name_copy == NULL) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY);

        return UINT64_C(0);
    }

    id =
        program->next_module_id++;

    entry =
        &program->modules[
            program->module_count];

    memset(
        entry,
        0,
        sizeof(*entry));

    entry->id = id;
    entry->module = module;

    entry->name = name_copy;
    entry->name_length = name_length;

    entry->registration_index =
        program->module_count;

    entry->is_entry = false;
    entry->reachable = false;

    ++program->module_count;

    vitte_c17_program_invalidate_order(
        program);

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    return id;
}

/* ========================================================================= */
/* Dependencies                                                              */
/* ========================================================================= */

static ptrdiff_t
vitte_c17_program_find_dependency_index(
    const vitte_c17_program_t *program,
    uint64_t from_module,
    uint64_t to_module)
{
    size_t index;

    for (index = 0u;
         index < program->dependency_count;
         ++index) {
        const vitte_c17_program_dependency_t *dependency;

        dependency =
            &program->dependencies[index];

        if (dependency->from_module_id ==
                from_module &&
            dependency->to_module_id ==
                to_module) {
            return (ptrdiff_t)index;
        }
    }

    return -1;
}

bool
vitte_c17_program_add_dependency(
    vitte_c17_program_t *program,
    uint64_t from_module_id,
    uint64_t to_module_id)
{
    vitte_c17_program_dependency_t *dependency;

    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    if (from_module_id == UINT64_C(0) ||
        to_module_id == UINT64_C(0)) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_INVALID_ARGUMENT);
    }

    if (from_module_id ==
        to_module_id) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_SELF_DEPENDENCY);
    }

    if (vitte_c17_program_find_module_index_by_id(
            program,
            from_module_id) < 0 ||
        vitte_c17_program_find_module_index_by_id(
            program,
            to_module_id) < 0) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_UNKNOWN_MODULE);
    }

    if (vitte_c17_program_find_dependency_index(
            program,
            from_module_id,
            to_module_id) >= 0) {
        if (program->config.reject_duplicate_dependencies) {
            return
                vitte_c17_program_fail(
                    program,
                    VITTE_C17_PROGRAM_ERROR_DUPLICATE_DEPENDENCY);
        }

        program->last_error =
            VITTE_C17_PROGRAM_ERROR_NONE;

        return true;
    }

    if (program->dependency_count >=
        program->config.max_dependencies) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_LIMIT_EXCEEDED);
    }

    if (!vitte_c17_program_reserve_dependencies(
            program,
            program->dependency_count + 1u)) {
        return false;
    }

    dependency =
        &program->dependencies[
            program->dependency_count];

    dependency->from_module_id =
        from_module_id;

    dependency->to_module_id =
        to_module_id;

    dependency->registration_index =
        program->dependency_count;

    ++program->dependency_count;

    vitte_c17_program_invalidate_order(
        program);

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    return true;
}

bool
vitte_c17_program_has_dependency(
    const vitte_c17_program_t *program,
    uint64_t from_module_id,
    uint64_t to_module_id)
{
    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    return
        vitte_c17_program_find_dependency_index(
            program,
            from_module_id,
            to_module_id) >= 0;
}

/* ========================================================================= */
/* Entry point                                                               */
/* ========================================================================= */

bool
vitte_c17_program_set_entry_module(
    vitte_c17_program_t *program,
    uint64_t module_id)
{
    ptrdiff_t index;
    size_t cursor;

    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    index =
        vitte_c17_program_find_module_index_by_id(
            program,
            module_id);

    if (index < 0) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_UNKNOWN_MODULE);
    }

    for (cursor = 0u;
         cursor < program->module_count;
         ++cursor) {
        program->modules[cursor].is_entry =
            false;
    }

    program->entry_module_id =
        module_id;

    program->modules[
        (size_t)index].is_entry = true;

    program->has_entry = true;

    vitte_c17_program_invalidate_order(
        program);

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    return true;
}

bool
vitte_c17_program_clear_entry(
    vitte_c17_program_t *program)
{
    size_t index;

    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    for (index = 0u;
         index < program->module_count;
         ++index) {
        program->modules[index].is_entry =
            false;
    }

    program->entry_module_id =
        UINT64_C(0);

    program->entry_symbol_id =
        UINT64_C(0);

    program->has_entry = false;

    vitte_c17_program_invalidate_order(
        program);

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Symbol lookup                                                             */
/* ========================================================================= */

static ptrdiff_t
vitte_c17_program_find_symbol_index_by_id(
    const vitte_c17_program_t *program,
    uint64_t symbol_id)
{
    size_t index;

    for (index = 0u;
         index < program->symbol_count;
         ++index) {
        if (program->symbols[index].id ==
            symbol_id) {
            return (ptrdiff_t)index;
        }
    }

    return -1;
}

static ptrdiff_t
vitte_c17_program_find_generated_symbol_index(
    const vitte_c17_program_t *program,
    const char *name,
    size_t length)
{
    size_t index;

    for (index = 0u;
         index < program->symbol_count;
         ++index) {
        const vitte_c17_program_symbol_t *symbol;

        symbol =
            &program->symbols[index];

        if (vitte_c17_program_string_equal(
                symbol->generated_name,
                symbol->generated_name_length,
                name,
                length)) {
            return (ptrdiff_t)index;
        }
    }

    return -1;
}

static ptrdiff_t
vitte_c17_program_find_source_symbol_index(
    const vitte_c17_program_t *program,
    uint64_t module_id,
    vitte_c17_program_symbol_kind_t kind,
    const char *name,
    size_t length)
{
    size_t index;

    for (index = 0u;
         index < program->symbol_count;
         ++index) {
        const vitte_c17_program_symbol_t *symbol;

        symbol =
            &program->symbols[index];

        if (symbol->module_id != module_id ||
            symbol->kind != kind) {
            continue;
        }

        if (vitte_c17_program_string_equal(
                symbol->source_name,
                symbol->source_name_length,
                name,
                length)) {
            return (ptrdiff_t)index;
        }
    }

    return -1;
}

/* ========================================================================= */
/* Symbol registration                                                       */
/* ========================================================================= */

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
    bool definition)
{
    vitte_c17_program_symbol_t *symbol;
    char *source_copy;
    char *generated_copy;
    uint64_t id;

    if (!vitte_c17_program_is_valid(
            program)) {
        return UINT64_C(0);
    }

    if (kind <=
            VITTE_C17_PROGRAM_SYMBOL_INVALID ||
        kind >=
            VITTE_C17_PROGRAM_SYMBOL_COUNT ||
        visibility <=
            VITTE_C17_PROGRAM_SYMBOL_VISIBILITY_INVALID ||
        visibility >=
            VITTE_C17_PROGRAM_SYMBOL_VISIBILITY_COUNT) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_INVALID_SYMBOL);

        return UINT64_C(0);
    }

    if (source_name == NULL ||
        source_name_length == 0u ||
        generated_name == NULL ||
        generated_name_length == 0u) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_INVALID_SYMBOL);

        return UINT64_C(0);
    }

    if (source_name_length >
            program->config.max_name_bytes ||
        generated_name_length >
            program->config.max_name_bytes) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_LIMIT_EXCEEDED);

        return UINT64_C(0);
    }

    if (module_id != UINT64_C(0) &&
        vitte_c17_program_find_module_index_by_id(
            program,
            module_id) < 0) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_UNKNOWN_MODULE);

        return UINT64_C(0);
    }

    if (program->symbol_count >=
        program->config.max_symbols) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_LIMIT_EXCEEDED);

        return UINT64_C(0);
    }

    if (vitte_c17_program_find_generated_symbol_index(
            program,
            generated_name,
            generated_name_length) >= 0 ||
        vitte_c17_program_find_source_symbol_index(
            program,
            module_id,
            kind,
            source_name,
            source_name_length) >= 0) {
        if (program->config.reject_duplicate_symbols) {
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_DUPLICATE_SYMBOL);

            return UINT64_C(0);
        }
    }

    if (program->next_symbol_id == UINT64_MAX) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_OVERFLOW);

        return UINT64_C(0);
    }

    if (!vitte_c17_program_reserve_symbols(
            program,
            program->symbol_count + 1u)) {
        return UINT64_C(0);
    }

    source_copy =
        vitte_c17_program_copy_string(
            source_name,
            source_name_length);

    if (source_copy == NULL) {
        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY);

        return UINT64_C(0);
    }

    generated_copy =
        vitte_c17_program_copy_string(
            generated_name,
            generated_name_length);

    if (generated_copy == NULL) {
        free(source_copy);

        vitte_c17_program_fail(
            program,
            VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY);

        return UINT64_C(0);
    }

    id =
        program->next_symbol_id++;

    symbol =
        &program->symbols[
            program->symbol_count];

    memset(
        symbol,
        0,
        sizeof(*symbol));

    symbol->id = id;
    symbol->module_id = module_id;
    symbol->source_id = source_id;

    symbol->kind = kind;
    symbol->visibility = visibility;

    symbol->source_name = source_copy;
    symbol->source_name_length =
        source_name_length;

    symbol->generated_name =
        generated_copy;

    symbol->generated_name_length =
        generated_name_length;

    symbol->definition = definition;

    symbol->registration_index =
        program->symbol_count;

    ++program->symbol_count;

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    return id;
}

const vitte_c17_program_symbol_t *
vitte_c17_program_find_symbol_by_id(
    const vitte_c17_program_t *program,
    uint64_t symbol_id)
{
    ptrdiff_t index;

    if (!vitte_c17_program_is_valid(
            program)) {
        return NULL;
    }

    index =
        vitte_c17_program_find_symbol_index_by_id(
            program,
            symbol_id);

    if (index < 0) {
        return NULL;
    }

    return
        &program->symbols[(size_t)index];
}

const vitte_c17_program_symbol_t *
vitte_c17_program_find_generated_symbol(
    const vitte_c17_program_t *program,
    const char *name,
    size_t length)
{
    ptrdiff_t index;

    if (!vitte_c17_program_is_valid(
            program) ||
        name == NULL ||
        length == 0u) {
        return NULL;
    }

    index =
        vitte_c17_program_find_generated_symbol_index(
            program,
            name,
            length);

    if (index < 0) {
        return NULL;
    }

    return
        &program->symbols[(size_t)index];
}

const vitte_c17_program_symbol_t *
vitte_c17_program_find_source_symbol(
    const vitte_c17_program_t *program,
    uint64_t module_id,
    vitte_c17_program_symbol_kind_t kind,
    const char *name,
    size_t length)
{
    ptrdiff_t index;

    if (!vitte_c17_program_is_valid(
            program) ||
        name == NULL ||
        length == 0u) {
        return NULL;
    }

    index =
        vitte_c17_program_find_source_symbol_index(
            program,
            module_id,
            kind,
            name,
            length);

    if (index < 0) {
        return NULL;
    }

    return
        &program->symbols[(size_t)index];
}

/* ========================================================================= */
/* Entry symbol                                                              */
/* ========================================================================= */

bool
vitte_c17_program_set_entry_symbol(
    vitte_c17_program_t *program,
    uint64_t symbol_id)
{
    ptrdiff_t index;
    const vitte_c17_program_symbol_t *symbol;

    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    index =
        vitte_c17_program_find_symbol_index_by_id(
            program,
            symbol_id);

    if (index < 0) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_UNKNOWN_SYMBOL);
    }

    symbol =
        &program->symbols[(size_t)index];

    if (symbol->kind !=
            VITTE_C17_PROGRAM_SYMBOL_FUNCTION ||
        !symbol->definition) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_INVALID_SYMBOL);
    }

    if (symbol->module_id != UINT64_C(0)) {
        if (!vitte_c17_program_set_entry_module(
                program,
                symbol->module_id)) {
            return false;
        }
    }

    program->entry_symbol_id =
        symbol_id;

    program->has_entry = true;

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Reachability                                                              */
/* ========================================================================= */

static bool
vitte_c17_program_mark_reachable(
    vitte_c17_program_t *program,
    size_t module_index,
    bool *marks)
{
    size_t dependency_index;
    uint64_t module_id;

    if (marks[module_index]) {
        return true;
    }

    marks[module_index] = true;

    module_id =
        program->modules[module_index].id;

    for (dependency_index = 0u;
         dependency_index <
            program->dependency_count;
         ++dependency_index) {
        const vitte_c17_program_dependency_t *dependency;
        ptrdiff_t target_index;

        dependency =
            &program->dependencies[
                dependency_index];

        if (dependency->from_module_id !=
            module_id) {
            continue;
        }

        target_index =
            vitte_c17_program_find_module_index_by_id(
                program,
                dependency->to_module_id);

        if (target_index < 0) {
            return false;
        }

        if (!vitte_c17_program_mark_reachable(
                program,
                (size_t)target_index,
                marks)) {
            return false;
        }
    }

    return true;
}

bool
vitte_c17_program_compute_reachability(
    vitte_c17_program_t *program)
{
    bool *marks;
    ptrdiff_t entry_index;
    size_t count;
    size_t index;
    size_t cursor;

    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    free(program->reachable_modules);

    program->reachable_modules = NULL;
    program->reachable_module_count = 0u;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        program->modules[index].reachable =
            false;
    }

    if (program->module_count == 0u) {
        program->last_error =
            VITTE_C17_PROGRAM_ERROR_NONE;

        return true;
    }

    marks =
        (bool *)calloc(
            program->module_count,
            sizeof(bool));

    if (marks == NULL) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY);
    }

    if (program->has_entry) {
        entry_index =
            vitte_c17_program_find_module_index_by_id(
                program,
                program->entry_module_id);

        if (entry_index < 0) {
            free(marks);

            return
                vitte_c17_program_fail(
                    program,
                    VITTE_C17_PROGRAM_ERROR_ENTRY_NOT_FOUND);
        }

        if (!vitte_c17_program_mark_reachable(
                program,
                (size_t)entry_index,
                marks)) {
            free(marks);

            return
                vitte_c17_program_fail(
                    program,
                    VITTE_C17_PROGRAM_ERROR_CORRUPTION);
        }
    } else {
        /*
         * Library-style program without an entry point:
         *
         * all modules are considered reachable.
         */
        for (index = 0u;
             index < program->module_count;
             ++index) {
            marks[index] = true;
        }
    }

    count = 0u;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        if (marks[index]) {
            ++count;
        }
    }

    if (count != 0u) {
        program->reachable_modules =
            (uint64_t *)malloc(
                count *
                sizeof(
                    program->reachable_modules[0]));

        if (program->reachable_modules == NULL) {
            free(marks);

            return
                vitte_c17_program_fail(
                    program,
                    VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY);
        }
    }

    cursor = 0u;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        program->modules[index].reachable =
            marks[index];

        if (marks[index]) {
            program->reachable_modules[cursor++] =
                program->modules[index].id;
        }
    }

    program->reachable_module_count =
        count;

    free(marks);

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Deterministic module ordering                                             */
/* ========================================================================= */

static int
vitte_c17_program_compare_module_identity(
    const vitte_c17_program_module_t *left,
    const vitte_c17_program_module_t *right)
{
    size_t common;
    int comparison;

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

    if (comparison < 0) {
        return -1;
    }

    if (comparison > 0) {
        return 1;
    }

    if (left->name_length <
        right->name_length) {
        return -1;
    }

    if (left->name_length >
        right->name_length) {
        return 1;
    }

    if (left->id < right->id) {
        return -1;
    }

    if (left->id > right->id) {
        return 1;
    }

    return 0;
}

static ptrdiff_t
vitte_c17_program_select_ready_module(
    const vitte_c17_program_t *program,
    const size_t *indegree,
    const bool *selected,
    const bool *included)
{
    ptrdiff_t best;
    size_t index;

    best = -1;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        if (!included[index] ||
            selected[index] ||
            indegree[index] != 0u) {
            continue;
        }

        if (best < 0) {
            best = (ptrdiff_t)index;
            continue;
        }

        if (program->config.deterministic) {
            if (vitte_c17_program_compare_module_identity(
                    &program->modules[index],
                    &program->modules[(size_t)best]) < 0) {
                best = (ptrdiff_t)index;
            }
        } else {
            if (program->modules[index].registration_index <
                program->modules[(size_t)best].registration_index) {
                best = (ptrdiff_t)index;
            }
        }
    }

    return best;
}

/*
 * Dependency orientation:
 *
 *     A -> B
 *
 * means:
 *
 *     A depends on B
 *
 * Therefore B must appear before A in topological emission order.
 */
bool
vitte_c17_program_compute_order(
    vitte_c17_program_t *program)
{
    size_t *indegree;
    bool *selected;
    bool *included;
    uint64_t *order;
    size_t included_count;
    size_t index;
    size_t produced;

    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    if (!vitte_c17_program_compute_reachability(
            program)) {
        return false;
    }

    free(program->ordered_modules);

    program->ordered_modules = NULL;
    program->ordered_module_count = 0u;

    if (program->module_count == 0u) {
        program->last_error =
            VITTE_C17_PROGRAM_ERROR_NONE;

        return true;
    }

    indegree =
        (size_t *)calloc(
            program->module_count,
            sizeof(size_t));

    selected =
        (bool *)calloc(
            program->module_count,
            sizeof(bool));

    included =
        (bool *)calloc(
            program->module_count,
            sizeof(bool));

    if (indegree == NULL ||
        selected == NULL ||
        included == NULL) {
        free(indegree);
        free(selected);
        free(included);

        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY);
    }

    included_count = 0u;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        included[index] =
            program->config.include_unreachable_modules ||
            program->modules[index].reachable;

        if (included[index]) {
            ++included_count;
        }
    }

    /*
     * A depends on B => topological graph edge B -> A.
     *
     * indegree[A] therefore counts A's dependencies.
     */
    for (index = 0u;
         index < program->dependency_count;
         ++index) {
        ptrdiff_t from_index;
        ptrdiff_t to_index;

        from_index =
            vitte_c17_program_find_module_index_by_id(
                program,
                program->dependencies[index].
                    from_module_id);

        to_index =
            vitte_c17_program_find_module_index_by_id(
                program,
                program->dependencies[index].
                    to_module_id);

        if (from_index < 0 ||
            to_index < 0) {
            free(indegree);
            free(selected);
            free(included);

            return
                vitte_c17_program_fail(
                    program,
                    VITTE_C17_PROGRAM_ERROR_CORRUPTION);
        }

        if (included[(size_t)from_index] &&
            included[(size_t)to_index]) {
            if (indegree[(size_t)from_index] ==
                SIZE_MAX) {
                free(indegree);
                free(selected);
                free(included);

                return
                    vitte_c17_program_fail(
                        program,
                        VITTE_C17_PROGRAM_ERROR_OVERFLOW);
            }

            ++indegree[(size_t)from_index];
        }
    }

    order = NULL;

    if (included_count != 0u) {
        size_t bytes;

        if (vitte_c17_program_mul_overflow(
                included_count,
                sizeof(order[0]),
                &bytes)) {
            free(indegree);
            free(selected);
            free(included);

            return
                vitte_c17_program_fail(
                    program,
                    VITTE_C17_PROGRAM_ERROR_OVERFLOW);
        }

        order =
            (uint64_t *)malloc(bytes);

        if (order == NULL) {
            free(indegree);
            free(selected);
            free(included);

            return
                vitte_c17_program_fail(
                    program,
                    VITTE_C17_PROGRAM_ERROR_OUT_OF_MEMORY);
        }
    }

    produced = 0u;

    while (produced < included_count) {
        ptrdiff_t ready;
        uint64_t completed_id;
        size_t dependency_index;

        ready =
            vitte_c17_program_select_ready_module(
                program,
                indegree,
                selected,
                included);

        if (ready < 0) {
            free(indegree);
            free(selected);
            free(included);
            free(order);

            return
                vitte_c17_program_fail(
                    program,
                    VITTE_C17_PROGRAM_ERROR_DEPENDENCY_CYCLE);
        }

        selected[(size_t)ready] = true;

        completed_id =
            program->modules[(size_t)ready].id;

        order[produced++] =
            completed_id;

        /*
         * completed_id is a dependency of every "from" module where:
         *
         *     from -> completed_id
         *
         * Remove that dependency.
         */
        for (dependency_index = 0u;
             dependency_index <
                program->dependency_count;
             ++dependency_index) {
            const vitte_c17_program_dependency_t *dependency;
            ptrdiff_t dependent_index;

            dependency =
                &program->dependencies[
                    dependency_index];

            if (dependency->to_module_id !=
                completed_id) {
                continue;
            }

            dependent_index =
                vitte_c17_program_find_module_index_by_id(
                    program,
                    dependency->from_module_id);

            if (dependent_index < 0) {
                free(indegree);
                free(selected);
                free(included);
                free(order);

                return
                    vitte_c17_program_fail(
                        program,
                        VITTE_C17_PROGRAM_ERROR_CORRUPTION);
            }

            if (!included[(size_t)dependent_index] ||
                selected[(size_t)dependent_index]) {
                continue;
            }

            if (indegree[(size_t)dependent_index] == 0u) {
                free(indegree);
                free(selected);
                free(included);
                free(order);

                return
                    vitte_c17_program_fail(
                        program,
                        VITTE_C17_PROGRAM_ERROR_CORRUPTION);
            }

            --indegree[(size_t)dependent_index];
        }
    }

    free(indegree);
    free(selected);
    free(included);

    program->ordered_modules = order;
    program->ordered_module_count =
        produced;

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Ordered access                                                            */
/* ========================================================================= */

size_t
vitte_c17_program_ordered_module_count(
    const vitte_c17_program_t *program)
{
    if (!vitte_c17_program_is_valid(
            program)) {
        return 0u;
    }

    return program->ordered_module_count;
}

const vitte_c17_program_module_t *
vitte_c17_program_ordered_module_at(
    const vitte_c17_program_t *program,
    size_t index)
{
    if (!vitte_c17_program_is_valid(
            program) ||
        index >=
            program->ordered_module_count) {
        return NULL;
    }

    return
        vitte_c17_program_find_module_by_id(
            program,
            program->ordered_modules[index]);
}

/* ========================================================================= */
/* Root discovery                                                            */
/* ========================================================================= */

bool
vitte_c17_program_module_is_root(
    const vitte_c17_program_t *program,
    uint64_t module_id)
{
    size_t index;

    if (!vitte_c17_program_is_valid(
            program) ||
        vitte_c17_program_find_module_index_by_id(
            program,
            module_id) < 0) {
        return false;
    }

    /*
     * A root has no incoming "depends-on-me" edge.
     */
    for (index = 0u;
         index < program->dependency_count;
         ++index) {
        if (program->dependencies[index].
                to_module_id ==
            module_id) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Stable fingerprint                                                        */
/* ========================================================================= */

uint64_t
vitte_c17_program_hash(
    const vitte_c17_program_t *program)
{
    uint64_t hash;
    size_t index;

    if (!vitte_c17_program_is_valid(
            program)) {
        return UINT64_C(0);
    }

    hash =
        VITTE_C17_PROGRAM_HASH_OFFSET;

    hash =
        vitte_c17_program_hash_size(
            hash,
            program->module_count);

    hash =
        vitte_c17_program_hash_size(
            hash,
            program->dependency_count);

    hash =
        vitte_c17_program_hash_size(
            hash,
            program->symbol_count);

    hash =
        vitte_c17_program_hash_bool(
            hash,
            program->has_entry);

    hash =
        vitte_c17_program_hash_u64(
            hash,
            program->entry_module_id);

    hash =
        vitte_c17_program_hash_u64(
            hash,
            program->entry_symbol_id);

    /*
     * Module identity.
     */
    for (index = 0u;
         index < program->module_count;
         ++index) {
        const vitte_c17_program_module_t *module;

        module =
            &program->modules[index];

        hash =
            vitte_c17_program_hash_u64(
                hash,
                module->id);

        hash =
            vitte_c17_program_hash_size(
                hash,
                module->name_length);

        hash =
            vitte_c17_program_hash_bytes(
                hash,
                module->name,
                module->name_length);

        hash =
            vitte_c17_program_hash_bool(
                hash,
                module->is_entry);
    }

    /*
     * Dependency graph.
     */
    for (index = 0u;
         index < program->dependency_count;
         ++index) {
        hash =
            vitte_c17_program_hash_u64(
                hash,
                program->dependencies[index].
                    from_module_id);

        hash =
            vitte_c17_program_hash_u64(
                hash,
                program->dependencies[index].
                    to_module_id);
    }

    /*
     * Symbol identity.
     */
    for (index = 0u;
         index < program->symbol_count;
         ++index) {
        const vitte_c17_program_symbol_t *symbol;

        symbol =
            &program->symbols[index];

        hash =
            vitte_c17_program_hash_u64(
                hash,
                symbol->id);

        hash =
            vitte_c17_program_hash_u64(
                hash,
                symbol->module_id);

        hash =
            vitte_c17_program_hash_u64(
                hash,
                symbol->source_id);

        hash =
            vitte_c17_program_hash_u64(
                hash,
                (uint64_t)symbol->kind);

        hash =
            vitte_c17_program_hash_u64(
                hash,
                (uint64_t)symbol->visibility);

        hash =
            vitte_c17_program_hash_size(
                hash,
                symbol->source_name_length);

        hash =
            vitte_c17_program_hash_bytes(
                hash,
                symbol->source_name,
                symbol->source_name_length);

        hash =
            vitte_c17_program_hash_size(
                hash,
                symbol->generated_name_length);

        hash =
            vitte_c17_program_hash_bytes(
                hash,
                symbol->generated_name,
                symbol->generated_name_length);

        hash =
            vitte_c17_program_hash_bool(
                hash,
                symbol->definition);
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_c17_program_stats_t
vitte_c17_program_stats(
    const vitte_c17_program_t *program)
{
    vitte_c17_program_stats_t stats;
    size_t index;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (!vitte_c17_program_is_valid(
            program)) {
        return stats;
    }

    stats.valid = true;

    stats.module_count =
        program->module_count;

    stats.module_capacity =
        program->module_capacity;

    stats.dependency_count =
        program->dependency_count;

    stats.dependency_capacity =
        program->dependency_capacity;

    stats.symbol_count =
        program->symbol_count;

    stats.symbol_capacity =
        program->symbol_capacity;

    stats.reachable_module_count =
        program->reachable_module_count;

    stats.ordered_module_count =
        program->ordered_module_count;

    stats.has_entry =
        program->has_entry;

    stats.generation =
        program->generation;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        stats.module_name_bytes =
            vitte_c17_program_saturating_add(
                stats.module_name_bytes,
                program->modules[index].
                    name_length);

        if (program->modules[index].reachable) {
            stats.reachable_module_name_bytes =
                vitte_c17_program_saturating_add(
                    stats.reachable_module_name_bytes,
                    program->modules[index].
                        name_length);
        }
    }

    for (index = 0u;
         index < program->symbol_count;
         ++index) {
        const vitte_c17_program_symbol_t *symbol;

        symbol =
            &program->symbols[index];

        stats.source_symbol_name_bytes =
            vitte_c17_program_saturating_add(
                stats.source_symbol_name_bytes,
                symbol->source_name_length);

        stats.generated_symbol_name_bytes =
            vitte_c17_program_saturating_add(
                stats.generated_symbol_name_bytes,
                symbol->generated_name_length);

        if (symbol->definition) {
            ++stats.definition_count;
        } else {
            ++stats.declaration_count;
        }

        switch (symbol->visibility) {
            case VITTE_C17_PROGRAM_SYMBOL_PRIVATE:
                ++stats.private_symbol_count;
                break;

            case VITTE_C17_PROGRAM_SYMBOL_PUBLIC:
                ++stats.public_symbol_count;
                break;

            case VITTE_C17_PROGRAM_SYMBOL_EXTERNAL:
                ++stats.external_symbol_count;
                break;

            default:
                break;
        }

        if (symbol->kind >
                VITTE_C17_PROGRAM_SYMBOL_INVALID &&
            symbol->kind <
                VITTE_C17_PROGRAM_SYMBOL_COUNT) {
            ++stats.symbols_by_kind[
                (size_t)symbol->kind];
        }
    }

    stats.hash =
        vitte_c17_program_hash(
            program);

    return stats;
}

/* ========================================================================= */
/* Accessors                                                                 */
/* ========================================================================= */

size_t
vitte_c17_program_module_count(
    const vitte_c17_program_t *program)
{
    return
        vitte_c17_program_is_valid(program)
            ? program->module_count
            : 0u;
}

size_t
vitte_c17_program_dependency_count(
    const vitte_c17_program_t *program)
{
    return
        vitte_c17_program_is_valid(program)
            ? program->dependency_count
            : 0u;
}

size_t
vitte_c17_program_symbol_count(
    const vitte_c17_program_t *program)
{
    return
        vitte_c17_program_is_valid(program)
            ? program->symbol_count
            : 0u;
}

const vitte_c17_program_module_t *
vitte_c17_program_module_at(
    const vitte_c17_program_t *program,
    size_t index)
{
    if (!vitte_c17_program_is_valid(
            program) ||
        index >= program->module_count) {
        return NULL;
    }

    return &program->modules[index];
}

const vitte_c17_program_dependency_t *
vitte_c17_program_dependency_at(
    const vitte_c17_program_t *program,
    size_t index)
{
    if (!vitte_c17_program_is_valid(
            program) ||
        index >=
            program->dependency_count) {
        return NULL;
    }

    return &program->dependencies[index];
}

const vitte_c17_program_symbol_t *
vitte_c17_program_symbol_at(
    const vitte_c17_program_t *program,
    size_t index)
{
    if (!vitte_c17_program_is_valid(
            program) ||
        index >= program->symbol_count) {
        return NULL;
    }

    return &program->symbols[index];
}

/* ========================================================================= */
/* Error access                                                              */
/* ========================================================================= */

vitte_c17_program_error_t
vitte_c17_program_last_error(
    const vitte_c17_program_t *program)
{
    if (!vitte_c17_program_is_valid(
            program)) {
        return
            VITTE_C17_PROGRAM_ERROR_INVALID_PROGRAM;
    }

    return program->last_error;
}

void
vitte_c17_program_clear_error(
    vitte_c17_program_t *program)
{
    if (!vitte_c17_program_is_valid(
            program)) {
        return;
    }

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;
}

/* ========================================================================= */
/* Deep validation                                                          */
/* ========================================================================= */

bool
vitte_c17_program_validate(
    const vitte_c17_program_t *program)
{
    size_t index;
    size_t other;
    size_t entry_count;

    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    if (!vitte_c17_program_config_valid(
            &program->config)) {
        return false;
    }

    if (program->module_count >
            program->config.max_modules ||
        program->dependency_count >
            program->config.max_dependencies ||
        program->symbol_count >
            program->config.max_symbols) {
        return false;
    }

    /*
     * Modules.
     */
    entry_count = 0u;

    for (index = 0u;
         index < program->module_count;
         ++index) {
        const vitte_c17_program_module_t *module;

        module =
            &program->modules[index];

        if (module->id == UINT64_C(0) ||
            module->module == NULL ||
            module->name == NULL ||
            module->name_length == 0u ||
            module->name_length >
                program->config.max_name_bytes) {
            return false;
        }

        if (module->is_entry) {
            ++entry_count;

            if (module->id !=
                program->entry_module_id) {
                return false;
            }
        }

        for (other = index + 1u;
             other < program->module_count;
             ++other) {
            if (module->id ==
                    program->modules[other].id ||
                module->module ==
                    program->modules[other].module ||
                vitte_c17_program_string_equal(
                    module->name,
                    module->name_length,
                    program->modules[other].name,
                    program->modules[other].
                        name_length)) {
                return false;
            }
        }
    }

    if (program->has_entry) {
        if (entry_count != 1u) {
            return false;
        }

        if (vitte_c17_program_find_module_index_by_id(
                program,
                program->entry_module_id) < 0) {
            return false;
        }

        if (program->entry_symbol_id !=
            UINT64_C(0)) {
            ptrdiff_t symbol_index;

            symbol_index =
                vitte_c17_program_find_symbol_index_by_id(
                    program,
                    program->entry_symbol_id);

            if (symbol_index < 0) {
                return false;
            }

            if (program->symbols[
                    (size_t)symbol_index].kind !=
                    VITTE_C17_PROGRAM_SYMBOL_FUNCTION ||
                !program->symbols[
                    (size_t)symbol_index].definition) {
                return false;
            }
        }
    } else {
        if (entry_count != 0u ||
            program->entry_module_id !=
                UINT64_C(0) ||
            program->entry_symbol_id !=
                UINT64_C(0)) {
            return false;
        }

        if (program->config.require_entry) {
            return false;
        }
    }

    /*
     * Dependencies.
     */
    for (index = 0u;
         index < program->dependency_count;
         ++index) {
        const vitte_c17_program_dependency_t *dependency;

        dependency =
            &program->dependencies[index];

        if (dependency->from_module_id ==
                UINT64_C(0) ||
            dependency->to_module_id ==
                UINT64_C(0) ||
            dependency->from_module_id ==
                dependency->to_module_id) {
            return false;
        }

        if (vitte_c17_program_find_module_index_by_id(
                program,
                dependency->from_module_id) < 0 ||
            vitte_c17_program_find_module_index_by_id(
                program,
                dependency->to_module_id) < 0) {
            return false;
        }

        for (other = index + 1u;
             other < program->dependency_count;
             ++other) {
            if (dependency->from_module_id ==
                    program->dependencies[other].
                        from_module_id &&
                dependency->to_module_id ==
                    program->dependencies[other].
                        to_module_id) {
                return false;
            }
        }
    }

    /*
     * Symbols.
     */
    for (index = 0u;
         index < program->symbol_count;
         ++index) {
        const vitte_c17_program_symbol_t *symbol;

        symbol =
            &program->symbols[index];

        if (symbol->id == UINT64_C(0) ||
            symbol->kind <=
                VITTE_C17_PROGRAM_SYMBOL_INVALID ||
            symbol->kind >=
                VITTE_C17_PROGRAM_SYMBOL_COUNT ||
            symbol->visibility <=
                VITTE_C17_PROGRAM_SYMBOL_VISIBILITY_INVALID ||
            symbol->visibility >=
                VITTE_C17_PROGRAM_SYMBOL_VISIBILITY_COUNT ||
            symbol->source_name == NULL ||
            symbol->source_name_length == 0u ||
            symbol->generated_name == NULL ||
            symbol->generated_name_length == 0u) {
            return false;
        }

        if (symbol->module_id != UINT64_C(0) &&
            vitte_c17_program_find_module_index_by_id(
                program,
                symbol->module_id) < 0) {
            return false;
        }

        for (other = index + 1u;
             other < program->symbol_count;
             ++other) {
            const vitte_c17_program_symbol_t *candidate;

            candidate =
                &program->symbols[other];

            if (symbol->id ==
                candidate->id) {
                return false;
            }

            /*
             * Generated C names form a whole-program namespace here.
             */
            if (vitte_c17_program_string_equal(
                    symbol->generated_name,
                    symbol->generated_name_length,
                    candidate->generated_name,
                    candidate->generated_name_length)) {
                return false;
            }

            if (symbol->module_id ==
                    candidate->module_id &&
                symbol->kind ==
                    candidate->kind &&
                vitte_c17_program_string_equal(
                    symbol->source_name,
                    symbol->source_name_length,
                    candidate->source_name,
                    candidate->source_name_length)) {
                return false;
            }
        }
    }

    /*
     * Cached order must reference valid modules exactly once.
     */
    for (index = 0u;
         index < program->ordered_module_count;
         ++index) {
        if (vitte_c17_program_find_module_index_by_id(
                program,
                program->ordered_modules[index]) < 0) {
            return false;
        }

        for (other = index + 1u;
             other <
                program->ordered_module_count;
             ++other) {
            if (program->ordered_modules[index] ==
                program->ordered_modules[other]) {
                return false;
            }
        }
    }

    /*
     * Reachability cache.
     */
    for (index = 0u;
         index < program->reachable_module_count;
         ++index) {
        ptrdiff_t module_index;

        module_index =
            vitte_c17_program_find_module_index_by_id(
                program,
                program->reachable_modules[index]);

        if (module_index < 0 ||
            !program->modules[
                (size_t)module_index].reachable) {
            return false;
        }

        for (other = index + 1u;
             other <
                program->reachable_module_count;
             ++other) {
            if (program->reachable_modules[index] ==
                program->reachable_modules[other]) {
                return false;
            }
        }
    }

    return true;
}

/* ========================================================================= */
/* Finalization                                                              */
/* ========================================================================= */

bool
vitte_c17_program_finalize(
    vitte_c17_program_t *program)
{
    if (!vitte_c17_program_is_valid(
            program)) {
        return false;
    }

    if (program->config.require_entry &&
        !program->has_entry) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_ENTRY_NOT_FOUND);
    }

    if (!vitte_c17_program_compute_order(
            program)) {
        return false;
    }

    if (program->config.reject_cycles &&
        program->ordered_module_count == 0u &&
        program->module_count != 0u) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_DEPENDENCY_CYCLE);
    }

    if (!vitte_c17_program_validate(
            program)) {
        return
            vitte_c17_program_fail(
                program,
                VITTE_C17_PROGRAM_ERROR_CORRUPTION);
    }

    program->last_error =
        VITTE_C17_PROGRAM_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
