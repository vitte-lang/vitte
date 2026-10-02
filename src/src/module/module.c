/*
 * Vitte Compiler
 * src/module/module.c
 *
 * Module / namespace subsystem implementation.
 *
 * Public contract: module.h
 *
 * Responsibilities:
 *
 *   - module lifecycle;
 *   - canonical module identity;
 *   - Vitte `space` hierarchy;
 *   - module registration;
 *   - parent/child relationships;
 *   - source-unit association;
 *   - path/name lookup;
 *   - public/private visibility;
 *   - external modules;
 *   - declaration/symbol registration;
 *   - duplicate detection;
 *   - deterministic ordering;
 *   - structural validation;
 *   - deterministic fingerprints;
 *   - resource limits;
 *   - statistics.
 *
 * This subsystem deliberately does NOT:
 *
 *   - read files;
 *   - resolve `use` declarations;
 *   - perform parsing;
 *   - perform type checking;
 *   - own AST/HIR/IR objects;
 *   - invoke package registries.
 *
 * Those responsibilities belong to filesystem/import/parser/HIR/etc.
 *
 * ISO C17.
 */

#include "module.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Private helpers                                                           */
/* ========================================================================= */

static bool
vitte_module_size_add(
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
vitte_module_size_mul(
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
vitte_module_u64_add_sat(
    uint64_t left,
    uint64_t right)
{
    if (UINT64_MAX - left < right) {
        return UINT64_MAX;
    }

    return left + right;
}

static uint64_t
vitte_module_hash_bytes(
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
        hash *= VITTE_MODULE_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_module_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned int shift;

    /*
     * Explicit little-endian serialization makes the fingerprint
     * independent from host byte order.
     */
    for (shift = 0u; shift < 64u; shift += 8u) {
        hash ^= (value >> shift) & UINT64_C(0xff);
        hash *= VITTE_MODULE_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_module_hash_bool(
    uint64_t hash,
    bool value)
{
    return vitte_module_hash_u64(
        hash,
        value ? UINT64_C(1) : UINT64_C(0));
}

static bool
vitte_module_context_valid(
    const vitte_module_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    if (context->magic != VITTE_MODULE_MAGIC) {
        return false;
    }

    if (context->state <= VITTE_MODULE_STATE_INVALID ||
        context->state >= VITTE_MODULE_STATE_DESTROYED) {
        return false;
    }

    if (context->module_count > context->module_capacity ||
        context->string_count > context->string_capacity ||
        context->symbol_count > context->symbol_capacity) {
        return false;
    }

    if (context->module_count != 0u &&
        context->modules == NULL) {
        return false;
    }

    if (context->string_count != 0u &&
        context->strings == NULL) {
        return false;
    }

    if (context->symbol_count != 0u &&
        context->symbols == NULL) {
        return false;
    }

    if (context->string_bytes > context->max_string_bytes) {
        return false;
    }

    return true;
}

static bool
vitte_module_mutable(
    const vitte_module_context_t *context)
{
    return vitte_module_context_valid(context) &&
           context->state == VITTE_MODULE_STATE_BUILDING;
}

static bool
vitte_module_fail(
    vitte_module_context_t *context,
    vitte_module_error_t error)
{
    if (context != NULL &&
        context->magic == VITTE_MODULE_MAGIC) {
        context->last_error = error;
    }

    return false;
}

static vitte_module_id_t
vitte_module_fail_id(
    vitte_module_context_t *context,
    vitte_module_error_t error)
{
    (void)vitte_module_fail(context, error);

    return VITTE_MODULE_INVALID_ID;
}

static vitte_module_string_id_t
vitte_module_fail_string_id(
    vitte_module_context_t *context,
    vitte_module_error_t error)
{
    (void)vitte_module_fail(context, error);

    return VITTE_MODULE_INVALID_ID;
}

static vitte_module_symbol_id_t
vitte_module_fail_symbol_id(
    vitte_module_context_t *context,
    vitte_module_error_t error)
{
    (void)vitte_module_fail(context, error);

    return VITTE_MODULE_INVALID_ID;
}

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

static bool
vitte_module_reserve_raw(
    vitte_module_context_t *context,
    void **storage,
    size_t *capacity,
    size_t required,
    size_t element_size)
{
    size_t next_capacity;
    size_t bytes;
    void *new_storage;

    if (context == NULL ||
        storage == NULL ||
        capacity == NULL ||
        element_size == 0u) {
        return false;
    }

    if (required <= *capacity) {
        return true;
    }

    next_capacity = *capacity;

    if (next_capacity == 0u) {
        next_capacity = VITTE_MODULE_DEFAULT_INITIAL_CAPACITY;
    }

    while (next_capacity < required) {
        size_t doubled;

        if (!vitte_module_size_mul(
                next_capacity,
                (size_t)2u,
                &doubled) ||
            doubled <= next_capacity) {
            next_capacity = required;
            break;
        }

        next_capacity = doubled;
    }

    if (!vitte_module_size_mul(
            next_capacity,
            element_size,
            &bytes)) {
        return vitte_module_fail(
            context,
            VITTE_MODULE_ERROR_OVERFLOW);
    }

    new_storage = realloc(*storage, bytes);

    if (new_storage == NULL) {
        context->stats.allocation_failures =
            vitte_module_u64_add_sat(
                context->stats.allocation_failures,
                UINT64_C(1));

        return vitte_module_fail(
            context,
            VITTE_MODULE_ERROR_OUT_OF_MEMORY);
    }

    if (*storage == NULL) {
        context->stats.allocations =
            vitte_module_u64_add_sat(
                context->stats.allocations,
                UINT64_C(1));
    } else {
        context->stats.reallocations =
            vitte_module_u64_add_sat(
                context->stats.reallocations,
                UINT64_C(1));
    }

    *storage = new_storage;
    *capacity = next_capacity;

    return true;
}

static bool
vitte_module_reserve_modules(
    vitte_module_context_t *context,
    size_t required)
{
    return vitte_module_reserve_raw(
        context,
        (void **)&context->modules,
        &context->module_capacity,
        required,
        sizeof(*context->modules));
}

static bool
vitte_module_reserve_strings(
    vitte_module_context_t *context,
    size_t required)
{
    return vitte_module_reserve_raw(
        context,
        (void **)&context->strings,
        &context->string_capacity,
        required,
        sizeof(*context->strings));
}

static bool
vitte_module_reserve_symbols(
    vitte_module_context_t *context,
    size_t required)
{
    return vitte_module_reserve_raw(
        context,
        (void **)&context->symbols,
        &context->symbol_capacity,
        required,
        sizeof(*context->symbols));
}

static bool
vitte_module_reserve_id_array(
    vitte_module_context_t *context,
    vitte_module_id_t **storage,
    size_t *capacity,
    size_t required)
{
    return vitte_module_reserve_raw(
        context,
        (void **)storage,
        capacity,
        required,
        sizeof(**storage));
}

static bool
vitte_module_reserve_symbol_id_array(
    vitte_module_context_t *context,
    vitte_module_symbol_id_t **storage,
    size_t *capacity,
    size_t required)
{
    return vitte_module_reserve_raw(
        context,
        (void **)storage,
        capacity,
        required,
        sizeof(**storage));
}

/* ========================================================================= */
/* String intern table                                                       */
/* ========================================================================= */

static const vitte_module_string_t *
vitte_module_string_by_id(
    const vitte_module_context_t *context,
    vitte_module_string_id_t id)
{
    size_t index;

    if (!vitte_module_context_valid(context) ||
        id == VITTE_MODULE_INVALID_ID) {
        return NULL;
    }

    if (id > (vitte_module_string_id_t)SIZE_MAX) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= context->string_count) {
        return NULL;
    }

    if (context->strings[index].id != id) {
        return NULL;
    }

    return &context->strings[index];
}

static bool
vitte_module_string_equal(
    const vitte_module_string_t *string,
    const char *data,
    size_t length,
    uint64_t hash)
{
    if (string == NULL ||
        (data == NULL && length != 0u)) {
        return false;
    }

    if (string->hash != hash ||
        string->length != length) {
        return false;
    }

    if (length == 0u) {
        return true;
    }

    return memcmp(string->data, data, length) == 0;
}

static uint64_t
vitte_module_string_hash(
    const char *data,
    size_t length)
{
    return vitte_module_hash_bytes(
        VITTE_MODULE_FNV_OFFSET,
        data,
        length);
}

vitte_module_string_id_t
vitte_module_intern_string(
    vitte_module_context_t *context,
    const char *data,
    size_t length)
{
    uint64_t hash;
    size_t index;
    size_t required_count;
    size_t required_bytes;
    char *copy;
    vitte_module_string_t *string;

    if (!vitte_module_mutable(context)) {
        return vitte_module_fail_string_id(
            context,
            VITTE_MODULE_ERROR_INVALID_STATE);
    }

    if (data == NULL && length != 0u) {
        return vitte_module_fail_string_id(
            context,
            VITTE_MODULE_ERROR_INVALID_ARGUMENT);
    }

    hash = vitte_module_string_hash(data, length);

    for (index = 0u;
         index < context->string_count;
         ++index) {
        if (vitte_module_string_equal(
                &context->strings[index],
                data,
                length,
                hash)) {
            return context->strings[index].id;
        }
    }

    if (!vitte_module_size_add(
            context->string_count,
            1u,
            &required_count)) {
        return vitte_module_fail_string_id(
            context,
            VITTE_MODULE_ERROR_OVERFLOW);
    }

    if (!vitte_module_size_add(
            context->string_bytes,
            length,
            &required_bytes) ||
        !vitte_module_size_add(
            required_bytes,
            1u,
            &required_bytes)) {
        return vitte_module_fail_string_id(
            context,
            VITTE_MODULE_ERROR_OVERFLOW);
    }

    if (required_bytes > context->max_string_bytes) {
        return vitte_module_fail_string_id(
            context,
            VITTE_MODULE_ERROR_STRING_LIMIT);
    }

    if (!vitte_module_reserve_strings(
            context,
            required_count)) {
        return VITTE_MODULE_INVALID_ID;
    }

    copy = (char *)malloc(length + 1u);

    if (copy == NULL) {
        context->stats.allocation_failures =
            vitte_module_u64_add_sat(
                context->stats.allocation_failures,
                UINT64_C(1));

        return vitte_module_fail_string_id(
            context,
            VITTE_MODULE_ERROR_OUT_OF_MEMORY);
    }

    context->stats.allocations =
        vitte_module_u64_add_sat(
            context->stats.allocations,
            UINT64_C(1));

    if (length != 0u) {
        memcpy(copy, data, length);
    }

    copy[length] = '\0';

    string = &context->strings[context->string_count];

    memset(string, 0, sizeof(*string));

    string->id =
        (vitte_module_string_id_t)required_count;

    string->data = copy;
    string->length = length;
    string->hash = hash;

    context->string_count = required_count;
    context->string_bytes = required_bytes;

    context->stats.strings_interned =
        vitte_module_u64_add_sat(
            context->stats.strings_interned,
            UINT64_C(1));

    context->stats.string_bytes =
        vitte_module_u64_add_sat(
            context->stats.string_bytes,
            (uint64_t)length);

    context->last_error = VITTE_MODULE_ERROR_NONE;

    return string->id;
}

const vitte_module_string_t *
vitte_module_get_string(
    const vitte_module_context_t *context,
    vitte_module_string_id_t id)
{
    return vitte_module_string_by_id(context, id);
}

/* ========================================================================= */
/* Name validation                                                          */
/* ========================================================================= */

static bool
vitte_module_ascii_identifier_start(unsigned char c)
{
    return c == (unsigned char)'_' ||
           (c >= (unsigned char)'A' &&
            c <= (unsigned char)'Z') ||
           (c >= (unsigned char)'a' &&
            c <= (unsigned char)'z');
}

static bool
vitte_module_ascii_identifier_continue(unsigned char c)
{
    return vitte_module_ascii_identifier_start(c) ||
           (c >= (unsigned char)'0' &&
            c <= (unsigned char)'9');
}

static bool
vitte_module_name_is_valid_bytes(
    const char *data,
    size_t length)
{
    size_t index;
    bool segment_start;

    if (data == NULL || length == 0u) {
        return false;
    }

    /*
     * Canonical module names use:
     *
     *     foo
     *     foo::bar
     *     foo::bar::baz
     *
     * Non-ASCII bytes are accepted here as identifier bytes; full Unicode
     * XID validation belongs to the lexer/name-resolution Unicode layer.
     */

    index = 0u;
    segment_start = true;

    while (index < length) {
        unsigned char c;

        c = (unsigned char)data[index];

        if (segment_start) {
            if (c >= 0x80u ||
                vitte_module_ascii_identifier_start(c)) {
                segment_start = false;
                ++index;
                continue;
            }

            return false;
        }

        if (c >= 0x80u ||
            vitte_module_ascii_identifier_continue(c)) {
            ++index;
            continue;
        }

        if (c == (unsigned char)':') {
            if (index + 1u >= length ||
                data[index + 1u] != ':') {
                return false;
            }

            index += 2u;

            if (index >= length) {
                return false;
            }

            segment_start = true;
            continue;
        }

        return false;
    }

    return !segment_start;
}

static bool
vitte_module_local_name_is_valid_bytes(
    const char *data,
    size_t length)
{
    size_t index;

    if (data == NULL || length == 0u) {
        return false;
    }

    if ((unsigned char)data[0] < 0x80u &&
        !vitte_module_ascii_identifier_start(
            (unsigned char)data[0])) {
        return false;
    }

    for (index = 1u; index < length; ++index) {
        unsigned char c;

        c = (unsigned char)data[index];

        if (c < 0x80u &&
            !vitte_module_ascii_identifier_continue(c)) {
            return false;
        }
    }

    return true;
}

static bool
vitte_module_string_is_valid_name(
    const vitte_module_context_t *context,
    vitte_module_string_id_t id)
{
    const vitte_module_string_t *string;

    string = vitte_module_string_by_id(context, id);

    if (string == NULL) {
        return false;
    }

    return vitte_module_name_is_valid_bytes(
        string->data,
        string->length);
}

static bool
vitte_module_string_is_valid_local_name(
    const vitte_module_context_t *context,
    vitte_module_string_id_t id)
{
    const vitte_module_string_t *string;

    string = vitte_module_string_by_id(context, id);

    if (string == NULL) {
        return false;
    }

    return vitte_module_local_name_is_valid_bytes(
        string->data,
        string->length);
}

/* ========================================================================= */
/* Object lookup                                                             */
/* ========================================================================= */

const vitte_module_t *
vitte_module_get(
    const vitte_module_context_t *context,
    vitte_module_id_t id)
{
    size_t index;

    if (!vitte_module_context_valid(context) ||
        id == VITTE_MODULE_INVALID_ID ||
        id > (vitte_module_id_t)SIZE_MAX) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= context->module_count) {
        return NULL;
    }

    if (context->modules[index].id != id) {
        return NULL;
    }

    return &context->modules[index];
}

vitte_module_t *
vitte_module_get_mut(
    vitte_module_context_t *context,
    vitte_module_id_t id)
{
    size_t index;

    if (!vitte_module_mutable(context) ||
        id == VITTE_MODULE_INVALID_ID ||
        id > (vitte_module_id_t)SIZE_MAX) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= context->module_count ||
        context->modules[index].id != id) {
        return NULL;
    }

    return &context->modules[index];
}

const vitte_module_symbol_t *
vitte_module_get_symbol(
    const vitte_module_context_t *context,
    vitte_module_symbol_id_t id)
{
    size_t index;

    if (!vitte_module_context_valid(context) ||
        id == VITTE_MODULE_INVALID_ID ||
        id > (vitte_module_symbol_id_t)SIZE_MAX) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= context->symbol_count) {
        return NULL;
    }

    if (context->symbols[index].id != id) {
        return NULL;
    }

    return &context->symbols[index];
}

vitte_module_symbol_t *
vitte_module_get_symbol_mut(
    vitte_module_context_t *context,
    vitte_module_symbol_id_t id)
{
    size_t index;

    if (!vitte_module_mutable(context) ||
        id == VITTE_MODULE_INVALID_ID ||
        id > (vitte_module_symbol_id_t)SIZE_MAX) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= context->symbol_count ||
        context->symbols[index].id != id) {
        return NULL;
    }

    return &context->symbols[index];
}

/* ========================================================================= */
/* Find module                                                               */
/* ========================================================================= */

vitte_module_id_t
vitte_module_find_by_name(
    const vitte_module_context_t *context,
    vitte_module_string_id_t name)
{
    size_t index;

    if (!vitte_module_context_valid(context) ||
        name == VITTE_MODULE_INVALID_ID) {
        return VITTE_MODULE_INVALID_ID;
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        if (context->modules[index].name == name) {
            return context->modules[index].id;
        }
    }

    return VITTE_MODULE_INVALID_ID;
}

vitte_module_id_t
vitte_module_find_by_name_bytes(
    const vitte_module_context_t *context,
    const char *name,
    size_t length)
{
    uint64_t hash;
    size_t index;

    if (!vitte_module_context_valid(context) ||
        name == NULL ||
        length == 0u) {
        return VITTE_MODULE_INVALID_ID;
    }

    hash = vitte_module_string_hash(name, length);

    for (index = 0u;
         index < context->module_count;
         ++index) {
        const vitte_module_string_t *string;

        string =
            vitte_module_string_by_id(
                context,
                context->modules[index].name);

        if (vitte_module_string_equal(
                string,
                name,
                length,
                hash)) {
            return context->modules[index].id;
        }
    }

    return VITTE_MODULE_INVALID_ID;
}

vitte_module_id_t
vitte_module_find_by_path(
    const vitte_module_context_t *context,
    vitte_module_string_id_t path)
{
    size_t index;

    if (!vitte_module_context_valid(context) ||
        path == VITTE_MODULE_INVALID_ID) {
        return VITTE_MODULE_INVALID_ID;
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        if (context->modules[index].path == path) {
            return context->modules[index].id;
        }
    }

    return VITTE_MODULE_INVALID_ID;
}

/* ========================================================================= */
/* Child/symbol list helpers                                                 */
/* ========================================================================= */

static bool
vitte_module_id_array_contains(
    const vitte_module_id_t *items,
    size_t count,
    vitte_module_id_t value)
{
    size_t index;

    if (items == NULL) {
        return false;
    }

    for (index = 0u; index < count; ++index) {
        if (items[index] == value) {
            return true;
        }
    }

    return false;
}

static bool
vitte_module_symbol_id_array_contains(
    const vitte_module_symbol_id_t *items,
    size_t count,
    vitte_module_symbol_id_t value)
{
    size_t index;

    if (items == NULL) {
        return false;
    }

    for (index = 0u; index < count; ++index) {
        if (items[index] == value) {
            return true;
        }
    }

    return false;
}

static bool
vitte_module_append_child(
    vitte_module_context_t *context,
    vitte_module_t *parent,
    vitte_module_id_t child)
{
    size_t required;

    if (context == NULL ||
        parent == NULL ||
        child == VITTE_MODULE_INVALID_ID) {
        return false;
    }

    if (vitte_module_id_array_contains(
            parent->children,
            parent->child_count,
            child)) {
        return true;
    }

    if (!vitte_module_size_add(
            parent->child_count,
            1u,
            &required)) {
        return vitte_module_fail(
            context,
            VITTE_MODULE_ERROR_OVERFLOW);
    }

    if (!vitte_module_reserve_id_array(
            context,
            &parent->children,
            &parent->child_capacity,
            required)) {
        return false;
    }

    parent->children[parent->child_count] = child;
    parent->child_count = required;

    return true;
}

static bool
vitte_module_append_symbol(
    vitte_module_context_t *context,
    vitte_module_t *module,
    vitte_module_symbol_id_t symbol)
{
    size_t required;

    if (context == NULL ||
        module == NULL ||
        symbol == VITTE_MODULE_INVALID_ID) {
        return false;
    }

    if (vitte_module_symbol_id_array_contains(
            module->symbols,
            module->symbol_count,
            symbol)) {
        return true;
    }

    if (!vitte_module_size_add(
            module->symbol_count,
            1u,
            &required)) {
        return vitte_module_fail(
            context,
            VITTE_MODULE_ERROR_OVERFLOW);
    }

    if (!vitte_module_reserve_symbol_id_array(
            context,
            &module->symbols,
            &module->symbol_capacity,
            required)) {
        return false;
    }

    module->symbols[module->symbol_count] = symbol;
    module->symbol_count = required;

    return true;
}

/* ========================================================================= */
/* Module registration                                                       */
/* ========================================================================= */

vitte_module_id_t
vitte_module_add(
    vitte_module_context_t *context,
    const vitte_module_desc_t *description)
{
    size_t required;
    vitte_module_t *module;
    vitte_module_t *parent;

    if (!vitte_module_mutable(context)) {
        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_INVALID_STATE);
    }

    if (description == NULL) {
        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_INVALID_ARGUMENT);
    }

    if (!vitte_module_string_is_valid_name(
            context,
            description->name)) {
        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_INVALID_NAME);
    }

    if (description->path != VITTE_MODULE_INVALID_ID &&
        vitte_module_string_by_id(
            context,
            description->path) == NULL) {
        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_INVALID_PATH);
    }

    if (description->visibility < VITTE_MODULE_VISIBILITY_PRIVATE ||
        description->visibility >= VITTE_MODULE_VISIBILITY_COUNT) {
        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_INVALID_ARGUMENT);
    }

    if (description->kind < VITTE_MODULE_KIND_SOURCE ||
        description->kind >= VITTE_MODULE_KIND_COUNT) {
        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_INVALID_ARGUMENT);
    }

    if (vitte_module_find_by_name(
            context,
            description->name) !=
        VITTE_MODULE_INVALID_ID) {
        context->stats.duplicate_modules =
            vitte_module_u64_add_sat(
                context->stats.duplicate_modules,
                UINT64_C(1));

        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_DUPLICATE_MODULE);
    }

    if (description->path != VITTE_MODULE_INVALID_ID &&
        vitte_module_find_by_path(
            context,
            description->path) !=
        VITTE_MODULE_INVALID_ID) {
        context->stats.duplicate_modules =
            vitte_module_u64_add_sat(
                context->stats.duplicate_modules,
                UINT64_C(1));

        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_DUPLICATE_PATH);
    }

    parent = NULL;

    if (description->parent != VITTE_MODULE_INVALID_ID) {
        parent =
            vitte_module_get_mut(
                context,
                description->parent);

        if (parent == NULL) {
            return vitte_module_fail_id(
                context,
                VITTE_MODULE_ERROR_INVALID_PARENT);
        }
    }

    if (context->module_count >= context->max_modules) {
        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_MODULE_LIMIT);
    }

    if (!vitte_module_size_add(
            context->module_count,
            1u,
            &required)) {
        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_OVERFLOW);
    }

    if (!vitte_module_reserve_modules(
            context,
            required)) {
        return VITTE_MODULE_INVALID_ID;
    }

    /*
     * reserve_modules() may move context->modules.
     * Reacquire parent after the reallocation.
     */
    if (description->parent != VITTE_MODULE_INVALID_ID) {
        parent =
            vitte_module_get_mut(
                context,
                description->parent);

        if (parent == NULL) {
            return vitte_module_fail_id(
                context,
                VITTE_MODULE_ERROR_CORRUPTION);
        }
    }

    module = &context->modules[context->module_count];

    memset(module, 0, sizeof(*module));

    module->id = (vitte_module_id_t)required;
    module->name = description->name;
    module->path = description->path;
    module->parent = description->parent;
    module->source_id = description->source_id;
    module->visibility = description->visibility;
    module->kind = description->kind;
    module->span = description->span;

    module->is_root = description->is_root;
    module->is_external = description->is_external;

    if (module->is_root &&
        module->parent != VITTE_MODULE_INVALID_ID) {
        return vitte_module_fail_id(
            context,
            VITTE_MODULE_ERROR_INVALID_PARENT);
    }

    /*
     * Append to the parent before publishing module_count.
     *
     * If allocation fails, the zero-initialized unpublished module slot
     * can safely be overwritten by a later registration.
     */
    if (parent != NULL &&
        !vitte_module_append_child(
            context,
            parent,
            module->id)) {
        return VITTE_MODULE_INVALID_ID;
    }

    context->module_count = required;

    context->stats.modules_registered =
        vitte_module_u64_add_sat(
            context->stats.modules_registered,
            UINT64_C(1));

    if (module->is_external) {
        context->stats.external_modules =
            vitte_module_u64_add_sat(
                context->stats.external_modules,
                UINT64_C(1));
    }

    context->last_error = VITTE_MODULE_ERROR_NONE;

    return module->id;
}

/* ========================================================================= */
/* Symbols                                                                   */
/* ========================================================================= */

static vitte_module_symbol_id_t
vitte_module_find_symbol_in_module(
    const vitte_module_context_t *context,
    vitte_module_id_t module_id,
    vitte_module_string_id_t name)
{
    const vitte_module_t *module;
    size_t index;

    module = vitte_module_get(context, module_id);

    if (module == NULL) {
        return VITTE_MODULE_INVALID_ID;
    }

    for (index = 0u;
         index < module->symbol_count;
         ++index) {
        const vitte_module_symbol_t *symbol;

        symbol =
            vitte_module_get_symbol(
                context,
                module->symbols[index]);

        if (symbol != NULL &&
            symbol->name == name) {
            return symbol->id;
        }
    }

    return VITTE_MODULE_INVALID_ID;
}

vitte_module_symbol_id_t
vitte_module_add_symbol(
    vitte_module_context_t *context,
    const vitte_module_symbol_desc_t *description)
{
    vitte_module_t *module;
    vitte_module_symbol_t *symbol;
    size_t required;

    if (!vitte_module_mutable(context)) {
        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_INVALID_STATE);
    }

    if (description == NULL) {
        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_INVALID_ARGUMENT);
    }

    module =
        vitte_module_get_mut(
            context,
            description->module);

    if (module == NULL) {
        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_INVALID_MODULE);
    }

    if (!vitte_module_string_is_valid_local_name(
            context,
            description->name)) {
        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_INVALID_NAME);
    }

    if (description->kind < VITTE_MODULE_SYMBOL_CONST ||
        description->kind >= VITTE_MODULE_SYMBOL_COUNT) {
        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_INVALID_SYMBOL);
    }

    if (description->visibility <
            VITTE_MODULE_VISIBILITY_PRIVATE ||
        description->visibility >=
            VITTE_MODULE_VISIBILITY_COUNT) {
        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_INVALID_ARGUMENT);
    }

    if (vitte_module_find_symbol_in_module(
            context,
            description->module,
            description->name) !=
        VITTE_MODULE_INVALID_ID) {
        context->stats.duplicate_symbols =
            vitte_module_u64_add_sat(
                context->stats.duplicate_symbols,
                UINT64_C(1));

        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_DUPLICATE_SYMBOL);
    }

    if (context->symbol_count >= context->max_symbols) {
        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_SYMBOL_LIMIT);
    }

    if (!vitte_module_size_add(
            context->symbol_count,
            1u,
            &required)) {
        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_OVERFLOW);
    }

    if (!vitte_module_reserve_symbols(
            context,
            required)) {
        return VITTE_MODULE_INVALID_ID;
    }

    /*
     * The symbols allocation does not move modules, but reacquiring keeps
     * this function robust if storage strategy changes later.
     */
    module =
        vitte_module_get_mut(
            context,
            description->module);

    if (module == NULL) {
        return vitte_module_fail_symbol_id(
            context,
            VITTE_MODULE_ERROR_CORRUPTION);
    }

    symbol = &context->symbols[context->symbol_count];

    memset(symbol, 0, sizeof(*symbol));

    symbol->id =
        (vitte_module_symbol_id_t)required;

    symbol->module = description->module;
    symbol->name = description->name;
    symbol->kind = description->kind;
    symbol->visibility = description->visibility;
    symbol->span = description->span;
    symbol->payload = description->payload;

    /*
     * Do not publish symbol_count before module list insertion succeeds.
     */
    if (!vitte_module_append_symbol(
            context,
            module,
            symbol->id)) {
        return VITTE_MODULE_INVALID_ID;
    }

    context->symbol_count = required;

    context->stats.symbols_registered =
        vitte_module_u64_add_sat(
            context->stats.symbols_registered,
            UINT64_C(1));

    context->last_error = VITTE_MODULE_ERROR_NONE;

    return symbol->id;
}

vitte_module_symbol_id_t
vitte_module_find_symbol(
    const vitte_module_context_t *context,
    vitte_module_id_t module,
    vitte_module_string_id_t name)
{
    if (!vitte_module_context_valid(context)) {
        return VITTE_MODULE_INVALID_ID;
    }

    return vitte_module_find_symbol_in_module(
        context,
        module,
        name);
}

/* ========================================================================= */
/* Parent/child queries                                                      */
/* ========================================================================= */

vitte_module_id_t
vitte_module_parent(
    const vitte_module_context_t *context,
    vitte_module_id_t module)
{
    const vitte_module_t *item;

    item = vitte_module_get(context, module);

    if (item == NULL) {
        return VITTE_MODULE_INVALID_ID;
    }

    return item->parent;
}

size_t
vitte_module_child_count(
    const vitte_module_context_t *context,
    vitte_module_id_t module)
{
    const vitte_module_t *item;

    item = vitte_module_get(context, module);

    return item != NULL
        ? item->child_count
        : 0u;
}

vitte_module_id_t
vitte_module_child_at(
    const vitte_module_context_t *context,
    vitte_module_id_t module,
    size_t index)
{
    const vitte_module_t *item;

    item = vitte_module_get(context, module);

    if (item == NULL ||
        index >= item->child_count ||
        item->children == NULL) {
        return VITTE_MODULE_INVALID_ID;
    }

    return item->children[index];
}

size_t
vitte_module_symbol_count(
    const vitte_module_context_t *context,
    vitte_module_id_t module)
{
    const vitte_module_t *item;

    item = vitte_module_get(context, module);

    return item != NULL
        ? item->symbol_count
        : 0u;
}

vitte_module_symbol_id_t
vitte_module_symbol_at(
    const vitte_module_context_t *context,
    vitte_module_id_t module,
    size_t index)
{
    const vitte_module_t *item;

    item = vitte_module_get(context, module);

    if (item == NULL ||
        index >= item->symbol_count ||
        item->symbols == NULL) {
        return VITTE_MODULE_INVALID_ID;
    }

    return item->symbols[index];
}

/* ========================================================================= */
/* Validation helpers                                                        */
/* ========================================================================= */

static bool
vitte_module_validate_strings(
    const vitte_module_context_t *context)
{
    size_t index;
    size_t total_bytes;

    total_bytes = 0u;

    for (index = 0u;
         index < context->string_count;
         ++index) {
        const vitte_module_string_t *string;
        size_t required;

        string = &context->strings[index];

        if (string->id !=
            (vitte_module_string_id_t)(index + 1u)) {
            return false;
        }

        if (string->data == NULL) {
            return false;
        }

        if (string->data[string->length] != '\0') {
            return false;
        }

        if (string->hash !=
            vitte_module_string_hash(
                string->data,
                string->length)) {
            return false;
        }

        if (!vitte_module_size_add(
                total_bytes,
                string->length,
                &required) ||
            !vitte_module_size_add(
                required,
                1u,
                &required)) {
            return false;
        }

        total_bytes = required;
    }

    return total_bytes == context->string_bytes;
}

static bool
vitte_module_validate_parent_chain(
    const vitte_module_context_t *context,
    vitte_module_id_t start)
{
    vitte_module_id_t slow;
    vitte_module_id_t fast;

    slow = start;
    fast = start;

    for (;;) {
        const vitte_module_t *slow_module;
        const vitte_module_t *fast_module;

        slow_module = vitte_module_get(context, slow);

        if (slow_module == NULL) {
            return false;
        }

        slow = slow_module->parent;

        if (slow == VITTE_MODULE_INVALID_ID) {
            return true;
        }

        fast_module = vitte_module_get(context, fast);

        if (fast_module == NULL) {
            return false;
        }

        fast = fast_module->parent;

        if (fast == VITTE_MODULE_INVALID_ID) {
            return true;
        }

        fast_module = vitte_module_get(context, fast);

        if (fast_module == NULL) {
            return false;
        }

        fast = fast_module->parent;

        if (fast == VITTE_MODULE_INVALID_ID) {
            return true;
        }

        if (slow == fast) {
            return false;
        }
    }
}

static bool
vitte_module_parent_contains_child(
    const vitte_module_context_t *context,
    vitte_module_id_t parent,
    vitte_module_id_t child)
{
    const vitte_module_t *parent_module;

    parent_module = vitte_module_get(context, parent);

    if (parent_module == NULL) {
        return false;
    }

    return vitte_module_id_array_contains(
        parent_module->children,
        parent_module->child_count,
        child);
}

static bool
vitte_module_module_contains_symbol(
    const vitte_module_context_t *context,
    vitte_module_id_t module,
    vitte_module_symbol_id_t symbol)
{
    const vitte_module_t *item;

    item = vitte_module_get(context, module);

    if (item == NULL) {
        return false;
    }

    return vitte_module_symbol_id_array_contains(
        item->symbols,
        item->symbol_count,
        symbol);
}

static bool
vitte_module_validate_modules(
    const vitte_module_context_t *context)
{
    size_t index;

    for (index = 0u;
         index < context->module_count;
         ++index) {
        const vitte_module_t *module;
        size_t child_index;
        size_t symbol_index;

        module = &context->modules[index];

        if (module->id !=
            (vitte_module_id_t)(index + 1u)) {
            return false;
        }

        if (!vitte_module_string_is_valid_name(
                context,
                module->name)) {
            return false;
        }

        if (module->path != VITTE_MODULE_INVALID_ID &&
            vitte_module_string_by_id(
                context,
                module->path) == NULL) {
            return false;
        }

        if (module->visibility <
                VITTE_MODULE_VISIBILITY_PRIVATE ||
            module->visibility >=
                VITTE_MODULE_VISIBILITY_COUNT) {
            return false;
        }

        if (module->kind < VITTE_MODULE_KIND_SOURCE ||
            module->kind >= VITTE_MODULE_KIND_COUNT) {
            return false;
        }

        if (module->is_root &&
            module->parent != VITTE_MODULE_INVALID_ID) {
            return false;
        }

        if (module->parent != VITTE_MODULE_INVALID_ID) {
            if (vitte_module_get(
                    context,
                    module->parent) == NULL) {
                return false;
            }

            if (!vitte_module_parent_contains_child(
                    context,
                    module->parent,
                    module->id)) {
                return false;
            }
        }

        if (!vitte_module_validate_parent_chain(
                context,
                module->id)) {
            return false;
        }

        if (module->child_count >
            module->child_capacity) {
            return false;
        }

        if (module->child_count != 0u &&
            module->children == NULL) {
            return false;
        }

        for (child_index = 0u;
             child_index < module->child_count;
             ++child_index) {
            const vitte_module_t *child;
            size_t other;

            child =
                vitte_module_get(
                    context,
                    module->children[child_index]);

            if (child == NULL ||
                child->parent != module->id) {
                return false;
            }

            for (other = child_index + 1u;
                 other < module->child_count;
                 ++other) {
                if (module->children[child_index] ==
                    module->children[other]) {
                    return false;
                }
            }
        }

        if (module->symbol_count >
            module->symbol_capacity) {
            return false;
        }

        if (module->symbol_count != 0u &&
            module->symbols == NULL) {
            return false;
        }

        for (symbol_index = 0u;
             symbol_index < module->symbol_count;
             ++symbol_index) {
            const vitte_module_symbol_t *symbol;
            size_t other;

            symbol =
                vitte_module_get_symbol(
                    context,
                    module->symbols[symbol_index]);

            if (symbol == NULL ||
                symbol->module != module->id) {
                return false;
            }

            for (other = symbol_index + 1u;
                 other < module->symbol_count;
                 ++other) {
                if (module->symbols[symbol_index] ==
                    module->symbols[other]) {
                    return false;
                }
            }
        }
    }

    /*
     * Canonical names and source paths must be globally unique.
     */
    for (index = 0u;
         index < context->module_count;
         ++index) {
        size_t other;

        for (other = index + 1u;
             other < context->module_count;
             ++other) {
            if (context->modules[index].name ==
                context->modules[other].name) {
                return false;
            }

            if (context->modules[index].path !=
                    VITTE_MODULE_INVALID_ID &&
                context->modules[index].path ==
                    context->modules[other].path) {
                return false;
            }
        }
    }

    return true;
}

static bool
vitte_module_validate_symbols(
    const vitte_module_context_t *context)
{
    size_t index;

    for (index = 0u;
         index < context->symbol_count;
         ++index) {
        const vitte_module_symbol_t *symbol;

        symbol = &context->symbols[index];

        if (symbol->id !=
            (vitte_module_symbol_id_t)(index + 1u)) {
            return false;
        }

        if (vitte_module_get(
                context,
                symbol->module) == NULL) {
            return false;
        }

        if (!vitte_module_string_is_valid_local_name(
                context,
                symbol->name)) {
            return false;
        }

        if (symbol->kind < VITTE_MODULE_SYMBOL_CONST ||
            symbol->kind >= VITTE_MODULE_SYMBOL_COUNT) {
            return false;
        }

        if (symbol->visibility <
                VITTE_MODULE_VISIBILITY_PRIVATE ||
            symbol->visibility >=
                VITTE_MODULE_VISIBILITY_COUNT) {
            return false;
        }

        if (!vitte_module_module_contains_symbol(
                context,
                symbol->module,
                symbol->id)) {
            return false;
        }
    }

    /*
     * Symbol names are unique inside each module.
     */
    for (index = 0u;
         index < context->symbol_count;
         ++index) {
        size_t other;

        for (other = index + 1u;
             other < context->symbol_count;
             ++other) {
            if (context->symbols[index].module ==
                    context->symbols[other].module &&
                context->symbols[index].name ==
                    context->symbols[other].name) {
                return false;
            }
        }
    }

    return true;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_module_validate(
    vitte_module_context_t *context)
{
    if (!vitte_module_context_valid(context)) {
        return false;
    }

    context->stats.validation_runs =
        vitte_module_u64_add_sat(
            context->stats.validation_runs,
            UINT64_C(1));

    if (context->module_count > context->max_modules ||
        context->symbol_count > context->max_symbols ||
        context->string_bytes > context->max_string_bytes) {
        goto failure;
    }

    if (!vitte_module_validate_strings(context) ||
        !vitte_module_validate_modules(context) ||
        !vitte_module_validate_symbols(context)) {
        goto failure;
    }

    context->last_error = VITTE_MODULE_ERROR_NONE;

    return true;

failure:

    context->stats.validation_failures =
        vitte_module_u64_add_sat(
            context->stats.validation_failures,
            UINT64_C(1));

    context->last_error =
        VITTE_MODULE_ERROR_VALIDATION;

    return false;
}

/* ========================================================================= */
/* Sealing                                                                   */
/* ========================================================================= */

bool
vitte_module_seal(
    vitte_module_context_t *context)
{
    if (!vitte_module_mutable(context)) {
        return vitte_module_fail(
            context,
            VITTE_MODULE_ERROR_INVALID_STATE);
    }

    context->state = VITTE_MODULE_STATE_VALIDATING;

    if (!vitte_module_validate(context)) {
        context->state = VITTE_MODULE_STATE_FAILED;
        return false;
    }

    context->state = VITTE_MODULE_STATE_SEALED;
    context->last_error = VITTE_MODULE_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_module_fingerprint(
    vitte_module_context_t *context)
{
    uint64_t hash;
    size_t index;

    if (!vitte_module_context_valid(context)) {
        return UINT64_C(0);
    }

    context->stats.hash_runs =
        vitte_module_u64_add_sat(
            context->stats.hash_runs,
            UINT64_C(1));

    hash = VITTE_MODULE_FNV_OFFSET;

    hash =
        vitte_module_hash_u64(
            hash,
            (uint64_t)VITTE_MODULE_API_VERSION_MAJOR);

    hash =
        vitte_module_hash_u64(
            hash,
            (uint64_t)VITTE_MODULE_API_VERSION_MINOR);

    hash =
        vitte_module_hash_u64(
            hash,
            (uint64_t)VITTE_MODULE_API_VERSION_PATCH);

    hash =
        vitte_module_hash_u64(
            hash,
            (uint64_t)context->module_count);

    hash =
        vitte_module_hash_u64(
            hash,
            (uint64_t)context->symbol_count);

    hash =
        vitte_module_hash_u64(
            hash,
            (uint64_t)context->string_count);

    for (index = 0u;
         index < context->string_count;
         ++index) {
        const vitte_module_string_t *string;

        string = &context->strings[index];

        hash =
            vitte_module_hash_u64(
                hash,
                string->id);

        hash =
            vitte_module_hash_u64(
                hash,
                (uint64_t)string->length);

        hash =
            vitte_module_hash_bytes(
                hash,
                string->data,
                string->length);
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        const vitte_module_t *module;
        size_t child_index;
        size_t symbol_index;

        module = &context->modules[index];

        hash =
            vitte_module_hash_u64(
                hash,
                module->id);

        hash =
            vitte_module_hash_u64(
                hash,
                module->name);

        hash =
            vitte_module_hash_u64(
                hash,
                module->path);

        hash =
            vitte_module_hash_u64(
                hash,
                module->parent);

        hash =
            vitte_module_hash_u64(
                hash,
                module->source_id);

        hash =
            vitte_module_hash_u64(
                hash,
                (uint64_t)module->visibility);

        hash =
            vitte_module_hash_u64(
                hash,
                (uint64_t)module->kind);

        hash =
            vitte_module_hash_bool(
                hash,
                module->is_root);

        hash =
            vitte_module_hash_bool(
                hash,
                module->is_external);

        hash =
            vitte_module_hash_u64(
                hash,
                (uint64_t)module->child_count);

        for (child_index = 0u;
             child_index < module->child_count;
             ++child_index) {
            hash =
                vitte_module_hash_u64(
                    hash,
                    module->children[child_index]);
        }

        hash =
            vitte_module_hash_u64(
                hash,
                (uint64_t)module->symbol_count);

        for (symbol_index = 0u;
             symbol_index < module->symbol_count;
             ++symbol_index) {
            hash =
                vitte_module_hash_u64(
                    hash,
                    module->symbols[symbol_index]);
        }
    }

    for (index = 0u;
         index < context->symbol_count;
         ++index) {
        const vitte_module_symbol_t *symbol;

        symbol = &context->symbols[index];

        hash =
            vitte_module_hash_u64(
                hash,
                symbol->id);

        hash =
            vitte_module_hash_u64(
                hash,
                symbol->module);

        hash =
            vitte_module_hash_u64(
                hash,
                symbol->name);

        hash =
            vitte_module_hash_u64(
                hash,
                (uint64_t)symbol->kind);

        hash =
            vitte_module_hash_u64(
                hash,
                (uint64_t)symbol->visibility);

        hash =
            vitte_module_hash_u64(
                hash,
                symbol->payload);
    }

    return hash;
}

/* ========================================================================= */
/* Enum names                                                                */
/* ========================================================================= */

const char *
vitte_module_error_name(
    vitte_module_error_t error)
{
    switch (error) {
        case VITTE_MODULE_ERROR_NONE:
            return "none";

        case VITTE_MODULE_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case VITTE_MODULE_ERROR_INVALID_CONTEXT:
            return "invalid_context";

        case VITTE_MODULE_ERROR_INVALID_STATE:
            return "invalid_state";

        case VITTE_MODULE_ERROR_INVALID_NAME:
            return "invalid_name";

        case VITTE_MODULE_ERROR_INVALID_PATH:
            return "invalid_path";

        case VITTE_MODULE_ERROR_INVALID_MODULE:
            return "invalid_module";

        case VITTE_MODULE_ERROR_INVALID_PARENT:
            return "invalid_parent";

        case VITTE_MODULE_ERROR_INVALID_SYMBOL:
            return "invalid_symbol";

        case VITTE_MODULE_ERROR_DUPLICATE_MODULE:
            return "duplicate_module";

        case VITTE_MODULE_ERROR_DUPLICATE_PATH:
            return "duplicate_path";

        case VITTE_MODULE_ERROR_DUPLICATE_SYMBOL:
            return "duplicate_symbol";

        case VITTE_MODULE_ERROR_MODULE_LIMIT:
            return "module_limit";

        case VITTE_MODULE_ERROR_SYMBOL_LIMIT:
            return "symbol_limit";

        case VITTE_MODULE_ERROR_STRING_LIMIT:
            return "string_limit";

        case VITTE_MODULE_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_MODULE_ERROR_OUT_OF_MEMORY:
            return "out_of_memory";

        case VITTE_MODULE_ERROR_VALIDATION:
            return "validation";

        case VITTE_MODULE_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_MODULE_ERROR_UNSUPPORTED:
            return "unsupported";

        case VITTE_MODULE_ERROR_INTERNAL:
            return "internal";

        case VITTE_MODULE_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_module_state_name(
    vitte_module_state_t state)
{
    switch (state) {
        case VITTE_MODULE_STATE_INVALID:
            return "invalid";

        case VITTE_MODULE_STATE_BUILDING:
            return "building";

        case VITTE_MODULE_STATE_VALIDATING:
            return "validating";

        case VITTE_MODULE_STATE_SEALED:
            return "sealed";

        case VITTE_MODULE_STATE_FAILED:
            return "failed";

        case VITTE_MODULE_STATE_DESTROYED:
            return "destroyed";

        case VITTE_MODULE_STATE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_module_kind_name(
    vitte_module_kind_t kind)
{
    switch (kind) {
        case VITTE_MODULE_KIND_INVALID:
            return "invalid";

        case VITTE_MODULE_KIND_SOURCE:
            return "source";

        case VITTE_MODULE_KIND_LIBRARY:
            return "library";

        case VITTE_MODULE_KIND_PACKAGE:
            return "package";

        case VITTE_MODULE_KIND_BUILTIN:
            return "builtin";

        case VITTE_MODULE_KIND_EXTERNAL:
            return "external";

        case VITTE_MODULE_KIND_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_module_symbol_kind_name(
    vitte_module_symbol_kind_t kind)
{
    switch (kind) {
        case VITTE_MODULE_SYMBOL_INVALID:
            return "invalid";

        case VITTE_MODULE_SYMBOL_CONST:
            return "const";

        case VITTE_MODULE_SYMBOL_STATIC:
            return "static";

        case VITTE_MODULE_SYMBOL_TYPE:
            return "type";

        case VITTE_MODULE_SYMBOL_FORM:
            return "form";

        case VITTE_MODULE_SYMBOL_PICK:
            return "pick";

        case VITTE_MODULE_SYMBOL_TRAIT:
            return "trait";

        case VITTE_MODULE_SYMBOL_PROC:
            return "proc";

        case VITTE_MODULE_SYMBOL_MACRO:
            return "macro";

        case VITTE_MODULE_SYMBOL_MODULE:
            return "module";

        case VITTE_MODULE_SYMBOL_TEST:
            return "test";

        case VITTE_MODULE_SYMBOL_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Destruction helpers                                                       */
/* ========================================================================= */

static void
vitte_module_release_contents(
    vitte_module_context_t *context)
{
    size_t index;

    if (context == NULL) {
        return;
    }

    for (index = 0u;
         index < context->string_count;
         ++index) {
        free(context->strings[index].data);

        context->strings[index].data = NULL;
        context->strings[index].length = 0u;
        context->strings[index].hash = UINT64_C(0);
        context->strings[index].id = VITTE_MODULE_INVALID_ID;
    }

    for (index = 0u;
         index < context->module_count;
         ++index) {
        free(context->modules[index].children);
        free(context->modules[index].symbols);

        context->modules[index].children = NULL;
        context->modules[index].child_count = 0u;
        context->modules[index].child_capacity = 0u;

        context->modules[index].symbols = NULL;
        context->modules[index].symbol_count = 0u;
        context->modules[index].symbol_capacity = 0u;
    }

    free(context->strings);
    free(context->modules);
    free(context->symbols);

    context->strings = NULL;
    context->string_count = 0u;
    context->string_capacity = 0u;
    context->string_bytes = 0u;

    context->modules = NULL;
    context->module_count = 0u;
    context->module_capacity = 0u;

    context->symbols = NULL;
    context->symbol_count = 0u;
    context->symbol_capacity = 0u;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_module_init(
    vitte_module_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    memset(context, 0, sizeof(*context));

    context->magic = VITTE_MODULE_MAGIC;

    context->state = VITTE_MODULE_STATE_BUILDING;
    context->last_error = VITTE_MODULE_ERROR_NONE;

    context->max_modules =
        VITTE_MODULE_DEFAULT_MAX_MODULES;

    context->max_symbols =
        VITTE_MODULE_DEFAULT_MAX_SYMBOLS;

    context->max_string_bytes =
        VITTE_MODULE_DEFAULT_MAX_STRING_BYTES;

    context->generation = UINT64_C(1);

    return true;
}

bool
vitte_module_reset(
    vitte_module_context_t *context)
{
    size_t max_modules;
    size_t max_symbols;
    size_t max_string_bytes;
    uint64_t generation;

    if (!vitte_module_context_valid(context)) {
        return false;
    }

    max_modules = context->max_modules;
    max_symbols = context->max_symbols;
    max_string_bytes = context->max_string_bytes;
    generation = context->generation;

    vitte_module_release_contents(context);

    memset(&context->stats, 0, sizeof(context->stats));

    context->state = VITTE_MODULE_STATE_BUILDING;
    context->last_error = VITTE_MODULE_ERROR_NONE;

    context->max_modules = max_modules;
    context->max_symbols = max_symbols;
    context->max_string_bytes = max_string_bytes;

    context->generation =
        generation == UINT64_MAX
            ? UINT64_MAX
            : generation + UINT64_C(1);

    return true;
}

void
vitte_module_destroy(
    vitte_module_context_t *context)
{
    if (context == NULL) {
        return;
    }

    if (context->magic == VITTE_MODULE_MAGIC) {
        vitte_module_release_contents(context);
    }

    memset(context, 0, sizeof(*context));

    context->magic = VITTE_MODULE_DEAD_MAGIC;
    context->state = VITTE_MODULE_STATE_DESTROYED;
}

bool
vitte_module_is_valid(
    const vitte_module_context_t *context)
{
    return vitte_module_context_valid(context);
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_module_set_limits(
    vitte_module_context_t *context,
    size_t max_modules,
    size_t max_symbols,
    size_t max_string_bytes)
{
    if (!vitte_module_mutable(context)) {
        return vitte_module_fail(
            context,
            VITTE_MODULE_ERROR_INVALID_STATE);
    }

    if (max_modules == 0u ||
        max_symbols == 0u ||
        max_string_bytes == 0u) {
        return vitte_module_fail(
            context,
            VITTE_MODULE_ERROR_INVALID_ARGUMENT);
    }

    if (max_modules < context->module_count ||
        max_symbols < context->symbol_count ||
        max_string_bytes < context->string_bytes) {
        return vitte_module_fail(
            context,
            VITTE_MODULE_ERROR_INVALID_ARGUMENT);
    }

    context->max_modules = max_modules;
    context->max_symbols = max_symbols;
    context->max_string_bytes = max_string_bytes;

    context->last_error = VITTE_MODULE_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_module_stats_t
vitte_module_stats(
    const vitte_module_context_t *context)
{
    vitte_module_stats_t result;

    memset(&result, 0, sizeof(result));

    if (!vitte_module_context_valid(context)) {
        return result;
    }

    return context->stats;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_module_translation_unit_anchor(void)
{
    /*
     * Intentionally empty.
     *
     * Stable linkage probe and static-archive anchor.
     */
}
