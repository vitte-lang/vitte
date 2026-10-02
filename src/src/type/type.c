/*
 * Vitte Compiler
 * src/type/type.c
 *
 * Canonical type-system implementation.
 *
 * Public contract: type.h
 *
 * ISO C17.
 *
 * Responsibilities:
 *   - canonical type context lifecycle;
 *   - builtin primitive types;
 *   - structural type interning;
 *   - pointer/reference/array/slice/range/function types;
 *   - tuple and named types;
 *   - generic parameters and dyn types;
 *   - mutability metadata;
 *   - type classification;
 *   - equality and compatibility;
 *   - assignment compatibility;
 *   - numeric promotion;
 *   - deterministic structural hashing;
 *   - deterministic fingerprints;
 *   - cycle-safe validation;
 *   - bounded allocation;
 *   - statistics.
 *
 * type.h is the single public contract.
 */

#include "type.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

static bool
vitte_type_context_structurally_valid(
    const vitte_type_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    if (context->magic != VITTE_TYPE_MAGIC) {
        return false;
    }

    if (context->state <= VITTE_TYPE_STATE_INVALID ||
        context->state >= VITTE_TYPE_STATE_DESTROYED) {
        return false;
    }

    if (context->type_count > context->type_capacity) {
        return false;
    }

    if (context->type_capacity != 0u &&
        context->types == NULL) {
        return false;
    }

    if (context->parameter_count >
        context->parameter_capacity) {
        return false;
    }

    if (context->parameter_capacity != 0u &&
        context->parameters == NULL) {
        return false;
    }

    if (context->max_types == 0u ||
        context->max_parameters == 0u) {
        return false;
    }

    return true;
}

static bool
vitte_type_fail(
    vitte_type_context_t *context,
    vitte_type_error_t error)
{
    if (context != NULL &&
        context->magic == VITTE_TYPE_MAGIC) {
        context->last_error = error;

        if (error != VITTE_TYPE_ERROR_NONE) {
            context->state = VITTE_TYPE_STATE_FAILED;
            context->stats.failures++;
        }
    }

    return false;
}

static bool
vitte_type_size_add(
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
vitte_type_size_mul(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left != 0u &&
        right > SIZE_MAX / left) {
        return false;
    }

    *result = left * right;
    return true;
}

static uint64_t
vitte_type_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned int shift;

    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        const unsigned char byte =
            (unsigned char)(
                (value >> shift) &
                UINT64_C(0xff));

        hash ^= (uint64_t)byte;
        hash *= VITTE_TYPE_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_type_hash_size(
    uint64_t hash,
    size_t value)
{
    return vitte_type_hash_u64(
        hash,
        (uint64_t)value);
}

static uint64_t
vitte_type_hash_bool(
    uint64_t hash,
    bool value)
{
    return vitte_type_hash_u64(
        hash,
        value ? UINT64_C(1) : UINT64_C(0));
}

static uint64_t
vitte_type_hash_bytes(
    uint64_t hash,
    const char *data,
    size_t length)
{
    size_t index;

    if (data == NULL && length != 0u) {
        return hash;
    }

    for (index = 0u;
         index < length;
         ++index) {
        hash ^= (uint64_t)
            (unsigned char)data[index];

        hash *= VITTE_TYPE_FNV_PRIME;
    }

    return hash;
}

static char *
vitte_type_duplicate_string(
    vitte_type_context_t *context,
    const char *data,
    size_t length)
{
    char *result;
    size_t allocation_size;

    if (data == NULL && length != 0u) {
        return NULL;
    }

    if (!vitte_type_size_add(
            length,
            1u,
            &allocation_size)) {
        if (context != NULL) {
            context->last_error =
                VITTE_TYPE_ERROR_OVERFLOW;
        }

        return NULL;
    }

    result = (char *)malloc(allocation_size);

    if (result == NULL) {
        if (context != NULL) {
            context->stats.allocation_failures++;
            context->last_error =
                VITTE_TYPE_ERROR_OUT_OF_MEMORY;
        }

        return NULL;
    }

    if (context != NULL) {
        context->stats.allocations++;
    }

    if (length != 0u) {
        memcpy(result, data, length);
    }

    result[length] = '\0';

    return result;
}

static vitte_type_id_t *
vitte_type_duplicate_ids(
    vitte_type_context_t *context,
    const vitte_type_id_t *ids,
    size_t count)
{
    vitte_type_id_t *result;
    size_t allocation_size;

    if (count == 0u) {
        return NULL;
    }

    if (ids == NULL) {
        return NULL;
    }

    if (!vitte_type_size_mul(
            count,
            sizeof(*result),
            &allocation_size)) {
        if (context != NULL) {
            context->last_error =
                VITTE_TYPE_ERROR_OVERFLOW;
        }

        return NULL;
    }

    result = (vitte_type_id_t *)malloc(
        allocation_size);

    if (result == NULL) {
        if (context != NULL) {
            context->stats.allocation_failures++;
            context->last_error =
                VITTE_TYPE_ERROR_OUT_OF_MEMORY;
        }

        return NULL;
    }

    if (context != NULL) {
        context->stats.allocations++;
    }

    memcpy(result, ids, allocation_size);

    return result;
}

static void
vitte_type_entry_clear(
    vitte_type_t *type)
{
    if (type == NULL) {
        return;
    }

    free(type->name);
    free(type->parameters);

    memset(type, 0, sizeof(*type));
}

static bool
vitte_type_reserve_types(
    vitte_type_context_t *context,
    size_t required)
{
    vitte_type_t *types;
    size_t capacity;
    size_t allocation_size;

    if (context == NULL) {
        return false;
    }

    if (required <= context->type_capacity) {
        return true;
    }

    if (required > context->max_types) {
        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_TYPE_LIMIT);
    }

    capacity = context->type_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_TYPE_DEFAULT_INITIAL_CAPACITY;
    }

    while (capacity < required) {
        size_t doubled;

        if (!vitte_type_size_mul(
                capacity,
                2u,
                &doubled)) {
            capacity = required;
            break;
        }

        capacity = doubled;

        if (capacity > context->max_types) {
            capacity = context->max_types;
            break;
        }
    }

    if (capacity < required) {
        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_TYPE_LIMIT);
    }

    if (!vitte_type_size_mul(
            capacity,
            sizeof(*types),
            &allocation_size)) {
        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_OVERFLOW);
    }

    types = (vitte_type_t *)realloc(
        context->types,
        allocation_size);

    if (types == NULL) {
        context->stats.allocation_failures++;

        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_OUT_OF_MEMORY);
    }

    if (context->types == NULL) {
        context->stats.allocations++;
    } else {
        context->stats.reallocations++;
    }

    if (capacity > context->type_capacity) {
        const size_t old_capacity =
            context->type_capacity;

        memset(
            types + old_capacity,
            0,
            (capacity - old_capacity) *
                sizeof(*types));
    }

    context->types = types;
    context->type_capacity = capacity;

    return true;
}

static bool
vitte_type_reserve_parameters(
    vitte_type_context_t *context,
    size_t required)
{
    vitte_type_id_t *parameters;
    size_t capacity;
    size_t allocation_size;

    if (context == NULL) {
        return false;
    }

    if (required <= context->parameter_capacity) {
        return true;
    }

    if (required > context->max_parameters) {
        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_PARAMETER_LIMIT);
    }

    capacity = context->parameter_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_TYPE_DEFAULT_INITIAL_PARAMETER_CAPACITY;
    }

    while (capacity < required) {
        size_t doubled;

        if (!vitte_type_size_mul(
                capacity,
                2u,
                &doubled)) {
            capacity = required;
            break;
        }

        capacity = doubled;

        if (capacity > context->max_parameters) {
            capacity = context->max_parameters;
            break;
        }
    }

    if (capacity < required) {
        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_PARAMETER_LIMIT);
    }

    if (!vitte_type_size_mul(
            capacity,
            sizeof(*parameters),
            &allocation_size)) {
        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_OVERFLOW);
    }

    parameters =
        (vitte_type_id_t *)realloc(
            context->parameters,
            allocation_size);

    if (parameters == NULL) {
        context->stats.allocation_failures++;

        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_OUT_OF_MEMORY);
    }

    if (context->parameters == NULL) {
        context->stats.allocations++;
    } else {
        context->stats.reallocations++;
    }

    context->parameters = parameters;
    context->parameter_capacity = capacity;

    return true;
}

static bool
vitte_type_id_in_context(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    uint64_t index;

    if (!vitte_type_context_structurally_valid(
            context)) {
        return false;
    }

    if (type_id == VITTE_TYPE_INVALID_ID) {
        return false;
    }

    index = type_id - UINT64_C(1);

    if (index >=
        (uint64_t)context->type_count) {
        return false;
    }

    return context->types[index].id == type_id;
}

static const vitte_type_t *
vitte_type_get_internal(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    if (!vitte_type_id_in_context(
            context,
            type_id)) {
        return NULL;
    }

    return &context->types[
        (size_t)(type_id - UINT64_C(1))];
}

static bool
vitte_type_parameters_equal(
    const vitte_type_t *type,
    const vitte_type_descriptor_t *descriptor)
{
    size_t index;

    if (type == NULL || descriptor == NULL) {
        return false;
    }

    if (type->parameter_count !=
        descriptor->parameter_count) {
        return false;
    }

    for (index = 0u;
         index < type->parameter_count;
         ++index) {
        if (type->parameters[index] !=
            descriptor->parameters[index]) {
            return false;
        }
    }

    return true;
}

static bool
vitte_type_name_equal(
    const vitte_type_t *type,
    const vitte_type_descriptor_t *descriptor)
{
    if (type == NULL || descriptor == NULL) {
        return false;
    }

    if (type->name_length !=
        descriptor->name_length) {
        return false;
    }

    if (type->name_length == 0u) {
        return true;
    }

    if (type->name == NULL ||
        descriptor->name == NULL) {
        return false;
    }

    return memcmp(
               type->name,
               descriptor->name,
               type->name_length) == 0;
}

static bool
vitte_type_descriptor_equal(
    const vitte_type_t *type,
    const vitte_type_descriptor_t *descriptor)
{
    if (type == NULL || descriptor == NULL) {
        return false;
    }

    if (type->kind != descriptor->kind ||
        type->flags != descriptor->flags ||
        type->element_type !=
            descriptor->element_type ||
        type->return_type !=
            descriptor->return_type ||
        type->array_length !=
            descriptor->array_length ||
        type->symbol_id !=
            descriptor->symbol_id ||
        type->generic_index !=
            descriptor->generic_index) {
        return false;
    }

    if (!vitte_type_name_equal(
            type,
            descriptor)) {
        return false;
    }

    return vitte_type_parameters_equal(
        type,
        descriptor);
}

static uint64_t
vitte_type_descriptor_hash(
    const vitte_type_descriptor_t *descriptor)
{
    uint64_t hash;
    size_t index;

    if (descriptor == NULL) {
        return UINT64_C(0);
    }

    hash = VITTE_TYPE_FNV_OFFSET;

    hash = vitte_type_hash_u64(
        hash,
        (uint64_t)descriptor->kind);

    hash = vitte_type_hash_u64(
        hash,
        descriptor->flags);

    hash = vitte_type_hash_u64(
        hash,
        descriptor->element_type);

    hash = vitte_type_hash_u64(
        hash,
        descriptor->return_type);

    hash = vitte_type_hash_size(
        hash,
        descriptor->array_length);

    hash = vitte_type_hash_u64(
        hash,
        descriptor->symbol_id);

    hash = vitte_type_hash_size(
        hash,
        descriptor->generic_index);

    hash = vitte_type_hash_size(
        hash,
        descriptor->name_length);

    hash = vitte_type_hash_bytes(
        hash,
        descriptor->name,
        descriptor->name_length);

    hash = vitte_type_hash_size(
        hash,
        descriptor->parameter_count);

    for (index = 0u;
         index < descriptor->parameter_count;
         ++index) {
        hash = vitte_type_hash_u64(
            hash,
            descriptor->parameters[index]);
    }

    return hash;
}

static bool
vitte_type_descriptor_validate(
    const vitte_type_context_t *context,
    const vitte_type_descriptor_t *descriptor)
{
    size_t index;

    if (context == NULL ||
        descriptor == NULL) {
        return false;
    }

    if (descriptor->kind <=
            VITTE_TYPE_KIND_INVALID ||
        descriptor->kind >=
            VITTE_TYPE_KIND_COUNT) {
        return false;
    }

    if (descriptor->name_length != 0u &&
        descriptor->name == NULL) {
        return false;
    }

    if (descriptor->parameter_count != 0u &&
        descriptor->parameters == NULL) {
        return false;
    }

    if (descriptor->element_type !=
            VITTE_TYPE_INVALID_ID &&
        !vitte_type_id_in_context(
            context,
            descriptor->element_type)) {
        return false;
    }

    if (descriptor->return_type !=
            VITTE_TYPE_INVALID_ID &&
        !vitte_type_id_in_context(
            context,
            descriptor->return_type)) {
        return false;
    }

    for (index = 0u;
         index < descriptor->parameter_count;
         ++index) {
        if (!vitte_type_id_in_context(
                context,
                descriptor->parameters[index])) {
            return false;
        }
    }

    switch (descriptor->kind) {
        case VITTE_TYPE_KIND_POINTER:
        case VITTE_TYPE_KIND_REFERENCE:
        case VITTE_TYPE_KIND_SLICE:
        case VITTE_TYPE_KIND_RANGE:
            if (descriptor->element_type ==
                VITTE_TYPE_INVALID_ID) {
                return false;
            }
            break;

        case VITTE_TYPE_KIND_ARRAY:
            if (descriptor->element_type ==
                VITTE_TYPE_INVALID_ID) {
                return false;
            }
            break;

        case VITTE_TYPE_KIND_FUNCTION:
            if (descriptor->return_type ==
                VITTE_TYPE_INVALID_ID) {
                return false;
            }
            break;

        case VITTE_TYPE_KIND_TUPLE:
            /*
             * Empty tuple is valid.
             */
            break;

        case VITTE_TYPE_KIND_NAMED:
        case VITTE_TYPE_KIND_DYN:
            if (descriptor->symbol_id ==
                    VITTE_TYPE_INVALID_SYMBOL_ID &&
                descriptor->name_length == 0u) {
                return false;
            }
            break;

        case VITTE_TYPE_KIND_GENERIC_PARAMETER:
            if (descriptor->name_length == 0u) {
                return false;
            }
            break;

        default:
            break;
    }

    return true;
}

static bool
vitte_type_append_parameter_block(
    vitte_type_context_t *context,
    const vitte_type_id_t *parameters,
    size_t parameter_count,
    size_t *offset)
{
    size_t required;

    if (context == NULL || offset == NULL) {
        return false;
    }

    *offset = context->parameter_count;

    if (parameter_count == 0u) {
        return true;
    }

    if (parameters == NULL) {
        return false;
    }

    if (!vitte_type_size_add(
            context->parameter_count,
            parameter_count,
            &required)) {
        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_OVERFLOW);
    }

    if (!vitte_type_reserve_parameters(
            context,
            required)) {
        return false;
    }

    memcpy(
        context->parameters +
            context->parameter_count,
        parameters,
        parameter_count *
            sizeof(*parameters));

    context->parameter_count = required;

    return true;
}

static bool
vitte_type_create_internal(
    vitte_type_context_t *context,
    const vitte_type_descriptor_t *descriptor,
    vitte_type_id_t *out_type_id)
{
    vitte_type_t candidate;
    size_t index;
    size_t parameter_offset;

    if (context == NULL ||
        descriptor == NULL ||
        out_type_id == NULL) {
        return false;
    }

    if (!vitte_type_descriptor_validate(
            context,
            descriptor)) {
        context->last_error =
            VITTE_TYPE_ERROR_INVALID_DESCRIPTOR;
        return false;
    }

    /*
     * Linear interning is deterministic and simple.
     *
     * This layer can later acquire a structural hash table without changing
     * the public type identity contract.
     */
    for (index = 0u;
         index < context->type_count;
         ++index) {
        if (context->types[index].structural_hash !=
            vitte_type_descriptor_hash(
                descriptor)) {
            continue;
        }

        if (vitte_type_descriptor_equal(
                &context->types[index],
                descriptor)) {
            context->stats.intern_hits++;
            *out_type_id =
                context->types[index].id;

            context->last_error =
                VITTE_TYPE_ERROR_NONE;

            return true;
        }
    }

    if (context->type_count >=
        context->max_types) {
        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_TYPE_LIMIT);
    }

    if (context->type_count >=
        (size_t)UINT64_MAX - 1u) {
        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_TYPE_LIMIT);
    }

    memset(&candidate, 0, sizeof(candidate));

    candidate.id =
        (vitte_type_id_t)(
            context->type_count + 1u);

    candidate.kind = descriptor->kind;
    candidate.flags = descriptor->flags;

    candidate.element_type =
        descriptor->element_type;

    candidate.return_type =
        descriptor->return_type;

    candidate.array_length =
        descriptor->array_length;

    candidate.symbol_id =
        descriptor->symbol_id;

    candidate.generic_index =
        descriptor->generic_index;

    candidate.structural_hash =
        vitte_type_descriptor_hash(
            descriptor);

    if (descriptor->name_length != 0u) {
        candidate.name =
            vitte_type_duplicate_string(
                context,
                descriptor->name,
                descriptor->name_length);

        if (candidate.name == NULL) {
            return vitte_type_fail(
                context,
                context->last_error !=
                    VITTE_TYPE_ERROR_NONE
                    ? context->last_error
                    : VITTE_TYPE_ERROR_OUT_OF_MEMORY);
        }

        candidate.name_length =
            descriptor->name_length;
    }

    candidate.parameters =
        vitte_type_duplicate_ids(
            context,
            descriptor->parameters,
            descriptor->parameter_count);

    if (descriptor->parameter_count != 0u &&
        candidate.parameters == NULL) {
        vitte_type_entry_clear(&candidate);

        return vitte_type_fail(
            context,
            context->last_error !=
                VITTE_TYPE_ERROR_NONE
                ? context->last_error
                : VITTE_TYPE_ERROR_OUT_OF_MEMORY);
    }

    candidate.parameter_count =
        descriptor->parameter_count;

    /*
     * Keep a canonical flat parameter arena as well. It is useful for
     * serialization, validation and deterministic compiler introspection.
     */
    if (!vitte_type_append_parameter_block(
            context,
            descriptor->parameters,
            descriptor->parameter_count,
            &parameter_offset)) {
        vitte_type_entry_clear(&candidate);
        return false;
    }

    candidate.parameter_offset =
        parameter_offset;

    if (!vitte_type_reserve_types(
            context,
            context->type_count + 1u)) {
        /*
         * Roll back flat parameter append.
         */
        context->parameter_count =
            parameter_offset;

        vitte_type_entry_clear(&candidate);
        return false;
    }

    context->types[context->type_count] =
        candidate;

    context->type_count++;
    context->stats.types_created++;

    *out_type_id = candidate.id;

    context->last_error =
        VITTE_TYPE_ERROR_NONE;

    return true;
}

static bool
vitte_type_make_builtin(
    vitte_type_context_t *context,
    vitte_type_kind_t kind,
    const char *name,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    if (context == NULL ||
        name == NULL ||
        out_type_id == NULL) {
        return false;
    }

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind = kind;
    descriptor.flags =
        VITTE_TYPE_FLAG_BUILTIN |
        VITTE_TYPE_FLAG_CANONICAL;

    descriptor.name = name;
    descriptor.name_length = strlen(name);

    return vitte_type_create_internal(
        context,
        &descriptor,
        out_type_id);
}

static bool
vitte_type_initialize_builtins(
    vitte_type_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_ERROR,
            "<error>",
            &context->type_error)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_VOID,
            "void",
            &context->type_void)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_NEVER,
            "never",
            &context->type_never)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_BOOL,
            "bool",
            &context->type_bool)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_I8,
            "i8",
            &context->type_i8)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_I16,
            "i16",
            &context->type_i16)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_I32,
            "i32",
            &context->type_i32)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_I64,
            "i64",
            &context->type_i64)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_ISIZE,
            "isize",
            &context->type_isize)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_U8,
            "u8",
            &context->type_u8)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_U16,
            "u16",
            &context->type_u16)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_U32,
            "u32",
            &context->type_u32)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_U64,
            "u64",
            &context->type_u64)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_USIZE,
            "usize",
            &context->type_usize)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_F32,
            "f32",
            &context->type_f32)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_F64,
            "f64",
            &context->type_f64)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_CHAR,
            "char",
            &context->type_char)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_STRING,
            "string",
            &context->type_string)) {
        return false;
    }

    if (!vitte_type_make_builtin(
            context,
            VITTE_TYPE_KIND_NULL,
            "null",
            &context->type_null)) {
        return false;
    }

    return true;
}

static unsigned int
vitte_type_integer_bits(
    vitte_type_kind_t kind)
{
    switch (kind) {
        case VITTE_TYPE_KIND_I8:
        case VITTE_TYPE_KIND_U8:
            return 8u;

        case VITTE_TYPE_KIND_I16:
        case VITTE_TYPE_KIND_U16:
            return 16u;

        case VITTE_TYPE_KIND_I32:
        case VITTE_TYPE_KIND_U32:
            return 32u;

        case VITTE_TYPE_KIND_I64:
        case VITTE_TYPE_KIND_U64:
            return 64u;

        case VITTE_TYPE_KIND_ISIZE:
        case VITTE_TYPE_KIND_USIZE:
            return (unsigned int)(
                sizeof(size_t) * 8u);

        default:
            return 0u;
    }
}

static bool
vitte_type_integer_signed_kind(
    vitte_type_kind_t kind)
{
    switch (kind) {
        case VITTE_TYPE_KIND_I8:
        case VITTE_TYPE_KIND_I16:
        case VITTE_TYPE_KIND_I32:
        case VITTE_TYPE_KIND_I64:
        case VITTE_TYPE_KIND_ISIZE:
            return true;

        default:
            return false;
    }
}

static vitte_type_id_t
vitte_type_integer_for_bits(
    const vitte_type_context_t *context,
    unsigned int bits,
    bool is_signed)
{
    if (context == NULL) {
        return VITTE_TYPE_INVALID_ID;
    }

    if (is_signed) {
        if (bits <= 8u) {
            return context->type_i8;
        }

        if (bits <= 16u) {
            return context->type_i16;
        }

        if (bits <= 32u) {
            return context->type_i32;
        }

        return context->type_i64;
    }

    if (bits <= 8u) {
        return context->type_u8;
    }

    if (bits <= 16u) {
        return context->type_u16;
    }

    if (bits <= 32u) {
        return context->type_u32;
    }

    return context->type_u64;
}

static bool
vitte_type_structural_equal_recursive(
    const vitte_type_context_t *context,
    vitte_type_id_t left_id,
    vitte_type_id_t right_id,
    size_t depth)
{
    const vitte_type_t *left;
    const vitte_type_t *right;
    size_t index;

    if (left_id == right_id) {
        return true;
    }

    if (depth >
        VITTE_TYPE_DEFAULT_MAX_COMPARISON_DEPTH) {
        return false;
    }

    left = vitte_type_get_internal(
        context,
        left_id);

    right = vitte_type_get_internal(
        context,
        right_id);

    if (left == NULL || right == NULL) {
        return false;
    }

    if (left->kind != right->kind ||
        left->flags != right->flags ||
        left->array_length !=
            right->array_length ||
        left->symbol_id !=
            right->symbol_id ||
        left->generic_index !=
            right->generic_index ||
        left->parameter_count !=
            right->parameter_count) {
        return false;
    }

    /*
     * Nominal declarations remain nominal.
     */
    if ((left->kind == VITTE_TYPE_KIND_NAMED ||
         left->kind == VITTE_TYPE_KIND_DYN ||
         left->kind ==
            VITTE_TYPE_KIND_GENERIC_PARAMETER) &&
        left->symbol_id != right->symbol_id) {
        return false;
    }

    if (left->name_length != right->name_length) {
        return false;
    }

    if (left->name_length != 0u &&
        memcmp(
            left->name,
            right->name,
            left->name_length) != 0) {
        return false;
    }

    if (left->element_type !=
            VITTE_TYPE_INVALID_ID ||
        right->element_type !=
            VITTE_TYPE_INVALID_ID) {
        if (!vitte_type_structural_equal_recursive(
                context,
                left->element_type,
                right->element_type,
                depth + 1u)) {
            return false;
        }
    }

    if (left->return_type !=
            VITTE_TYPE_INVALID_ID ||
        right->return_type !=
            VITTE_TYPE_INVALID_ID) {
        if (!vitte_type_structural_equal_recursive(
                context,
                left->return_type,
                right->return_type,
                depth + 1u)) {
            return false;
        }
    }

    for (index = 0u;
         index < left->parameter_count;
         ++index) {
        if (!vitte_type_structural_equal_recursive(
                context,
                left->parameters[index],
                right->parameters[index],
                depth + 1u)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_type_error_name(
    vitte_type_error_t error)
{
    switch (error) {
        case VITTE_TYPE_ERROR_NONE:
            return "none";

        case VITTE_TYPE_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case VITTE_TYPE_ERROR_INVALID_CONTEXT:
            return "invalid_context";

        case VITTE_TYPE_ERROR_INVALID_STATE:
            return "invalid_state";

        case VITTE_TYPE_ERROR_INVALID_TYPE:
            return "invalid_type";

        case VITTE_TYPE_ERROR_INVALID_TYPE_ID:
            return "invalid_type_id";

        case VITTE_TYPE_ERROR_INVALID_KIND:
            return "invalid_kind";

        case VITTE_TYPE_ERROR_INVALID_DESCRIPTOR:
            return "invalid_descriptor";

        case VITTE_TYPE_ERROR_TYPE_LIMIT:
            return "type_limit";

        case VITTE_TYPE_ERROR_PARAMETER_LIMIT:
            return "parameter_limit";

        case VITTE_TYPE_ERROR_COMPARISON_DEPTH:
            return "comparison_depth";

        case VITTE_TYPE_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_TYPE_ERROR_OUT_OF_MEMORY:
            return "out_of_memory";

        case VITTE_TYPE_ERROR_VALIDATION:
            return "validation";

        case VITTE_TYPE_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_TYPE_ERROR_INTERNAL:
            return "internal";

        case VITTE_TYPE_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_type_state_name(
    vitte_type_state_t state)
{
    switch (state) {
        case VITTE_TYPE_STATE_INVALID:
            return "invalid";

        case VITTE_TYPE_STATE_READY:
            return "ready";

        case VITTE_TYPE_STATE_BUILDING:
            return "building";

        case VITTE_TYPE_STATE_FROZEN:
            return "frozen";

        case VITTE_TYPE_STATE_FAILED:
            return "failed";

        case VITTE_TYPE_STATE_DESTROYED:
            return "destroyed";

        case VITTE_TYPE_STATE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_type_kind_name(
    vitte_type_kind_t kind)
{
    switch (kind) {
        case VITTE_TYPE_KIND_INVALID:
            return "invalid";

        case VITTE_TYPE_KIND_ERROR:
            return "error";

        case VITTE_TYPE_KIND_VOID:
            return "void";

        case VITTE_TYPE_KIND_NEVER:
            return "never";

        case VITTE_TYPE_KIND_BOOL:
            return "bool";

        case VITTE_TYPE_KIND_I8:
            return "i8";

        case VITTE_TYPE_KIND_I16:
            return "i16";

        case VITTE_TYPE_KIND_I32:
            return "i32";

        case VITTE_TYPE_KIND_I64:
            return "i64";

        case VITTE_TYPE_KIND_ISIZE:
            return "isize";

        case VITTE_TYPE_KIND_U8:
            return "u8";

        case VITTE_TYPE_KIND_U16:
            return "u16";

        case VITTE_TYPE_KIND_U32:
            return "u32";

        case VITTE_TYPE_KIND_U64:
            return "u64";

        case VITTE_TYPE_KIND_USIZE:
            return "usize";

        case VITTE_TYPE_KIND_F32:
            return "f32";

        case VITTE_TYPE_KIND_F64:
            return "f64";

        case VITTE_TYPE_KIND_CHAR:
            return "char";

        case VITTE_TYPE_KIND_STRING:
            return "string";

        case VITTE_TYPE_KIND_NULL:
            return "null";

        case VITTE_TYPE_KIND_POINTER:
            return "pointer";

        case VITTE_TYPE_KIND_REFERENCE:
            return "reference";

        case VITTE_TYPE_KIND_ARRAY:
            return "array";

        case VITTE_TYPE_KIND_SLICE:
            return "slice";

        case VITTE_TYPE_KIND_RANGE:
            return "range";

        case VITTE_TYPE_KIND_TUPLE:
            return "tuple";

        case VITTE_TYPE_KIND_FUNCTION:
            return "function";

        case VITTE_TYPE_KIND_NAMED:
            return "named";

        case VITTE_TYPE_KIND_GENERIC_PARAMETER:
            return "generic_parameter";

        case VITTE_TYPE_KIND_DYN:
            return "dyn";

        case VITTE_TYPE_KIND_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_type_init(
    vitte_type_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    memset(context, 0, sizeof(*context));

    context->magic = VITTE_TYPE_MAGIC;
    context->state = VITTE_TYPE_STATE_BUILDING;
    context->last_error =
        VITTE_TYPE_ERROR_NONE;

    context->max_types =
        VITTE_TYPE_DEFAULT_MAX_TYPES;

    context->max_parameters =
        VITTE_TYPE_DEFAULT_MAX_PARAMETERS;

    context->generation = UINT64_C(1);

    if (!vitte_type_initialize_builtins(
            context)) {
        return false;
    }

    context->state = VITTE_TYPE_STATE_READY;
    context->last_error =
        VITTE_TYPE_ERROR_NONE;

    return true;
}

bool
vitte_type_reset(
    vitte_type_context_t *context)
{
    size_t index;

    if (!vitte_type_context_structurally_valid(
            context)) {
        return false;
    }

    for (index = 0u;
         index < context->type_count;
         ++index) {
        vitte_type_entry_clear(
            &context->types[index]);
    }

    context->type_count = 0u;
    context->parameter_count = 0u;

    context->type_error =
        VITTE_TYPE_INVALID_ID;
    context->type_void =
        VITTE_TYPE_INVALID_ID;
    context->type_never =
        VITTE_TYPE_INVALID_ID;
    context->type_bool =
        VITTE_TYPE_INVALID_ID;

    context->type_i8 =
        VITTE_TYPE_INVALID_ID;
    context->type_i16 =
        VITTE_TYPE_INVALID_ID;
    context->type_i32 =
        VITTE_TYPE_INVALID_ID;
    context->type_i64 =
        VITTE_TYPE_INVALID_ID;
    context->type_isize =
        VITTE_TYPE_INVALID_ID;

    context->type_u8 =
        VITTE_TYPE_INVALID_ID;
    context->type_u16 =
        VITTE_TYPE_INVALID_ID;
    context->type_u32 =
        VITTE_TYPE_INVALID_ID;
    context->type_u64 =
        VITTE_TYPE_INVALID_ID;
    context->type_usize =
        VITTE_TYPE_INVALID_ID;

    context->type_f32 =
        VITTE_TYPE_INVALID_ID;
    context->type_f64 =
        VITTE_TYPE_INVALID_ID;

    context->type_char =
        VITTE_TYPE_INVALID_ID;
    context->type_string =
        VITTE_TYPE_INVALID_ID;
    context->type_null =
        VITTE_TYPE_INVALID_ID;

    context->state =
        VITTE_TYPE_STATE_BUILDING;

    context->last_error =
        VITTE_TYPE_ERROR_NONE;

    if (context->generation != UINT64_MAX) {
        context->generation++;
    }

    context->stats.resets++;

    if (!vitte_type_initialize_builtins(
            context)) {
        return false;
    }

    context->state = VITTE_TYPE_STATE_READY;

    return true;
}

void
vitte_type_destroy(
    vitte_type_context_t *context)
{
    size_t index;

    if (context == NULL) {
        return;
    }

    if (context->magic ==
            VITTE_TYPE_DEAD_MAGIC &&
        context->state ==
            VITTE_TYPE_STATE_DESTROYED) {
        return;
    }

    if (context->magic == VITTE_TYPE_MAGIC) {
        for (index = 0u;
             index < context->type_count;
             ++index) {
            vitte_type_entry_clear(
                &context->types[index]);
        }

        free(context->types);
        free(context->parameters);
    }

    memset(context, 0, sizeof(*context));

    context->magic =
        VITTE_TYPE_DEAD_MAGIC;

    context->state =
        VITTE_TYPE_STATE_DESTROYED;
}

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

bool
vitte_type_is_valid(
    const vitte_type_context_t *context)
{
    return vitte_type_context_structurally_valid(
        context);
}

vitte_type_error_t
vitte_type_last_error(
    const vitte_type_context_t *context)
{
    if (!vitte_type_context_structurally_valid(
            context)) {
        return VITTE_TYPE_ERROR_INVALID_CONTEXT;
    }

    return context->last_error;
}

uint64_t
vitte_type_generation(
    const vitte_type_context_t *context)
{
    if (!vitte_type_context_structurally_valid(
            context)) {
        return UINT64_C(0);
    }

    return context->generation;
}

bool
vitte_type_set_limits(
    vitte_type_context_t *context,
    size_t max_types,
    size_t max_parameters)
{
    if (!vitte_type_context_structurally_valid(
            context)) {
        return false;
    }

    if (context->state ==
        VITTE_TYPE_STATE_FROZEN) {
        context->last_error =
            VITTE_TYPE_ERROR_INVALID_STATE;
        return false;
    }

    if (max_types == 0u ||
        max_parameters == 0u ||
        max_types < context->type_count ||
        max_parameters <
            context->parameter_count) {
        context->last_error =
            VITTE_TYPE_ERROR_INVALID_ARGUMENT;
        return false;
    }

    context->max_types = max_types;
    context->max_parameters = max_parameters;

    context->last_error =
        VITTE_TYPE_ERROR_NONE;

    return true;
}

bool
vitte_type_freeze(
    vitte_type_context_t *context)
{
    if (!vitte_type_context_structurally_valid(
            context)) {
        return false;
    }

    if (context->state !=
            VITTE_TYPE_STATE_READY &&
        context->state !=
            VITTE_TYPE_STATE_BUILDING) {
        context->last_error =
            VITTE_TYPE_ERROR_INVALID_STATE;
        return false;
    }

    context->state =
        VITTE_TYPE_STATE_FROZEN;

    context->stats.freezes++;

    return true;
}

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

const vitte_type_t *
vitte_type_get(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    return vitte_type_get_internal(
        context,
        type_id);
}

/* ========================================================================= */
/* Generic interning                                                         */
/* ========================================================================= */

bool
vitte_type_intern(
    vitte_type_context_t *context,
    const vitte_type_descriptor_t *descriptor,
    vitte_type_id_t *out_type_id)
{
    if (!vitte_type_context_structurally_valid(
            context)) {
        return false;
    }

    if (descriptor == NULL ||
        out_type_id == NULL) {
        context->last_error =
            VITTE_TYPE_ERROR_INVALID_ARGUMENT;
        return false;
    }

    if (context->state ==
        VITTE_TYPE_STATE_FROZEN) {
        context->last_error =
            VITTE_TYPE_ERROR_INVALID_STATE;
        return false;
    }

    if (context->state !=
            VITTE_TYPE_STATE_READY &&
        context->state !=
            VITTE_TYPE_STATE_BUILDING) {
        context->last_error =
            VITTE_TYPE_ERROR_INVALID_STATE;
        return false;
    }

    context->state =
        VITTE_TYPE_STATE_BUILDING;

    if (!vitte_type_create_internal(
            context,
            descriptor,
            out_type_id)) {
        return false;
    }

    context->state =
        VITTE_TYPE_STATE_READY;

    return true;
}

/* ========================================================================= */
/* Constructed types                                                         */
/* ========================================================================= */

bool
vitte_type_pointer(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    bool is_mutable,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_POINTER;

    descriptor.element_type =
        element_type;

    if (is_mutable) {
        descriptor.flags |=
            VITTE_TYPE_FLAG_MUTABLE;
    }

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

bool
vitte_type_reference(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    bool is_mutable,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_REFERENCE;

    descriptor.element_type =
        element_type;

    if (is_mutable) {
        descriptor.flags |=
            VITTE_TYPE_FLAG_MUTABLE;
    }

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

bool
vitte_type_array(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    size_t array_length,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_ARRAY;

    descriptor.element_type =
        element_type;

    descriptor.array_length =
        array_length;

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

bool
vitte_type_slice(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    bool is_mutable,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_SLICE;

    descriptor.element_type =
        element_type;

    if (is_mutable) {
        descriptor.flags |=
            VITTE_TYPE_FLAG_MUTABLE;
    }

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

bool
vitte_type_range(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    bool inclusive,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_RANGE;

    descriptor.element_type =
        element_type;

    if (inclusive) {
        descriptor.flags |=
            VITTE_TYPE_FLAG_INCLUSIVE;
    }

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

bool
vitte_type_tuple(
    vitte_type_context_t *context,
    const vitte_type_id_t *elements,
    size_t element_count,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_TUPLE;

    descriptor.parameters = elements;
    descriptor.parameter_count =
        element_count;

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

bool
vitte_type_function(
    vitte_type_context_t *context,
    const vitte_type_id_t *parameters,
    size_t parameter_count,
    vitte_type_id_t return_type,
    bool variadic,
    bool is_async,
    bool is_unsafe,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_FUNCTION;

    descriptor.parameters = parameters;
    descriptor.parameter_count =
        parameter_count;

    descriptor.return_type =
        return_type;

    if (variadic) {
        descriptor.flags |=
            VITTE_TYPE_FLAG_VARIADIC;
    }

    if (is_async) {
        descriptor.flags |=
            VITTE_TYPE_FLAG_ASYNC;
    }

    if (is_unsafe) {
        descriptor.flags |=
            VITTE_TYPE_FLAG_UNSAFE;
    }

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

bool
vitte_type_named(
    vitte_type_context_t *context,
    uint64_t symbol_id,
    const char *name,
    size_t name_length,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_NAMED;

    descriptor.symbol_id = symbol_id;
    descriptor.name = name;
    descriptor.name_length = name_length;

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

bool
vitte_type_generic_parameter(
    vitte_type_context_t *context,
    uint64_t symbol_id,
    const char *name,
    size_t name_length,
    size_t generic_index,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_GENERIC_PARAMETER;

    descriptor.symbol_id = symbol_id;
    descriptor.name = name;
    descriptor.name_length = name_length;
    descriptor.generic_index =
        generic_index;

    descriptor.flags |=
        VITTE_TYPE_FLAG_GENERIC;

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

bool
vitte_type_dyn(
    vitte_type_context_t *context,
    uint64_t trait_symbol_id,
    const char *name,
    size_t name_length,
    vitte_type_id_t *out_type_id)
{
    vitte_type_descriptor_t descriptor;

    memset(&descriptor, 0, sizeof(descriptor));

    descriptor.kind =
        VITTE_TYPE_KIND_DYN;

    descriptor.symbol_id =
        trait_symbol_id;

    descriptor.name = name;
    descriptor.name_length = name_length;

    return vitte_type_intern(
        context,
        &descriptor,
        out_type_id);
}

/* ========================================================================= */
/* Classification                                                            */
/* ========================================================================= */

bool
vitte_type_is_error(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    return context != NULL &&
           type_id == context->type_error;
}

bool
vitte_type_is_void(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    return context != NULL &&
           type_id == context->type_void;
}

bool
vitte_type_is_never(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    return context != NULL &&
           type_id == context->type_never;
}

bool
vitte_type_is_bool(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    return context != NULL &&
           type_id == context->type_bool;
}

bool
vitte_type_is_integer(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    if (type == NULL) {
        return false;
    }

    switch (type->kind) {
        case VITTE_TYPE_KIND_I8:
        case VITTE_TYPE_KIND_I16:
        case VITTE_TYPE_KIND_I32:
        case VITTE_TYPE_KIND_I64:
        case VITTE_TYPE_KIND_ISIZE:

        case VITTE_TYPE_KIND_U8:
        case VITTE_TYPE_KIND_U16:
        case VITTE_TYPE_KIND_U32:
        case VITTE_TYPE_KIND_U64:
        case VITTE_TYPE_KIND_USIZE:
            return true;

        default:
            return false;
    }
}

bool
vitte_type_is_signed_integer(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    return type != NULL &&
           vitte_type_integer_signed_kind(
               type->kind);
}

bool
vitte_type_is_unsigned_integer(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    return vitte_type_is_integer(
               context,
               type_id) &&
           !vitte_type_is_signed_integer(
               context,
               type_id);
}

bool
vitte_type_is_float(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    return type != NULL &&
           (type->kind ==
                VITTE_TYPE_KIND_F32 ||
            type->kind ==
                VITTE_TYPE_KIND_F64);
}

bool
vitte_type_is_numeric(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    return vitte_type_is_integer(
               context,
               type_id) ||
           vitte_type_is_float(
               context,
               type_id);
}

bool
vitte_type_is_scalar(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    if (type == NULL) {
        return false;
    }

    if (vitte_type_is_numeric(
            context,
            type_id)) {
        return true;
    }

    switch (type->kind) {
        case VITTE_TYPE_KIND_BOOL:
        case VITTE_TYPE_KIND_CHAR:
        case VITTE_TYPE_KIND_POINTER:
        case VITTE_TYPE_KIND_REFERENCE:
        case VITTE_TYPE_KIND_NULL:
            return true;

        default:
            return false;
    }
}

bool
vitte_type_is_pointer_like(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    if (type == NULL) {
        return false;
    }

    return type->kind ==
               VITTE_TYPE_KIND_POINTER ||
           type->kind ==
               VITTE_TYPE_KIND_REFERENCE;
}

bool
vitte_type_is_sequence(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    if (type == NULL) {
        return false;
    }

    return type->kind ==
               VITTE_TYPE_KIND_ARRAY ||
           type->kind ==
               VITTE_TYPE_KIND_SLICE ||
           type->kind ==
               VITTE_TYPE_KIND_STRING;
}

bool
vitte_type_is_callable(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    return type != NULL &&
           type->kind ==
               VITTE_TYPE_KIND_FUNCTION;
}

bool
vitte_type_is_aggregate(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    if (type == NULL) {
        return false;
    }

    switch (type->kind) {
        case VITTE_TYPE_KIND_ARRAY:
        case VITTE_TYPE_KIND_SLICE:
        case VITTE_TYPE_KIND_TUPLE:
        case VITTE_TYPE_KIND_NAMED:
        case VITTE_TYPE_KIND_DYN:
        case VITTE_TYPE_KIND_STRING:
            return true;

        default:
            return false;
    }
}

/* ========================================================================= */
/* Equality                                                                  */
/* ========================================================================= */

bool
vitte_type_equal(
    const vitte_type_context_t *context,
    vitte_type_id_t left,
    vitte_type_id_t right)
{
    if (!vitte_type_context_structurally_valid(
            context)) {
        return false;
    }

    if (!vitte_type_id_in_context(
            context,
            left) ||
        !vitte_type_id_in_context(
            context,
            right)) {
        return false;
    }

    return left == right;
}

bool
vitte_type_structural_equal(
    const vitte_type_context_t *context,
    vitte_type_id_t left,
    vitte_type_id_t right)
{
    if (!vitte_type_context_structurally_valid(
            context)) {
        return false;
    }

    if (!vitte_type_id_in_context(
            context,
            left) ||
        !vitte_type_id_in_context(
            context,
            right)) {
        return false;
    }

    return vitte_type_structural_equal_recursive(
        context,
        left,
        right,
        0u);
}

/* ========================================================================= */
/* Compatibility                                                             */
/* ========================================================================= */

bool
vitte_type_assignable(
    const vitte_type_context_t *context,
    vitte_type_id_t destination_id,
    vitte_type_id_t source_id)
{
    const vitte_type_t *destination;
    const vitte_type_t *source;

    if (!vitte_type_context_structurally_valid(
            context)) {
        return false;
    }

    destination =
        vitte_type_get_internal(
            context,
            destination_id);

    source =
        vitte_type_get_internal(
            context,
            source_id);

    if (destination == NULL ||
        source == NULL) {
        return false;
    }

    /*
     * Error recovery prevents cascaded diagnostics.
     */
    if (destination->kind ==
            VITTE_TYPE_KIND_ERROR ||
        source->kind ==
            VITTE_TYPE_KIND_ERROR) {
        return true;
    }

    if (destination_id == source_id) {
        return true;
    }

    /*
     * never coerces to every destination.
     */
    if (source->kind ==
        VITTE_TYPE_KIND_NEVER) {
        return true;
    }

    /*
     * null -> pointer/reference/dyn.
     */
    if (source->kind ==
        VITTE_TYPE_KIND_NULL) {
        return destination->kind ==
                   VITTE_TYPE_KIND_POINTER ||
               destination->kind ==
                   VITTE_TYPE_KIND_REFERENCE ||
               destination->kind ==
                   VITTE_TYPE_KIND_DYN;
    }

    /*
     * Mutable reference/pointer can weaken to immutable.
     */
    if (destination->kind ==
            source->kind &&
        (destination->kind ==
             VITTE_TYPE_KIND_REFERENCE ||
         destination->kind ==
             VITTE_TYPE_KIND_POINTER ||
         destination->kind ==
             VITTE_TYPE_KIND_SLICE)) {
        const bool destination_mutable =
            (destination->flags &
             VITTE_TYPE_FLAG_MUTABLE) != 0u;

        const bool source_mutable =
            (source->flags &
             VITTE_TYPE_FLAG_MUTABLE) != 0u;

        if (destination_mutable &&
            !source_mutable) {
            return false;
        }

        return destination->element_type ==
               source->element_type;
    }

    /*
     * Integer widening with matching signedness.
     */
    if (vitte_type_is_integer(
            context,
            destination_id) &&
        vitte_type_is_integer(
            context,
            source_id)) {
        const unsigned int destination_bits =
            vitte_type_integer_bits(
                destination->kind);

        const unsigned int source_bits =
            vitte_type_integer_bits(
                source->kind);

        const bool destination_signed =
            vitte_type_integer_signed_kind(
                destination->kind);

        const bool source_signed =
            vitte_type_integer_signed_kind(
                source->kind);

        if (destination_signed ==
                source_signed &&
            destination_bits >= source_bits) {
            return true;
        }

        /*
         * Unsigned source may widen into a strictly wider signed type.
         */
        if (destination_signed &&
            !source_signed &&
            destination_bits > source_bits) {
            return true;
        }

        return false;
    }

    /*
     * Float widening.
     */
    if (destination->kind ==
            VITTE_TYPE_KIND_F64 &&
        source->kind ==
            VITTE_TYPE_KIND_F32) {
        return true;
    }

    /*
     * Integer -> floating conversion.
     */
    if (vitte_type_is_float(
            context,
            destination_id) &&
        vitte_type_is_integer(
            context,
            source_id)) {
        return true;
    }

    /*
     * Fixed array -> immutable/mutable slice with same element type.
     */
    if (destination->kind ==
            VITTE_TYPE_KIND_SLICE &&
        source->kind ==
            VITTE_TYPE_KIND_ARRAY &&
        destination->element_type ==
            source->element_type) {
        return true;
    }

    return false;
}

bool
vitte_type_compatible(
    const vitte_type_context_t *context,
    vitte_type_id_t left,
    vitte_type_id_t right)
{
    return vitte_type_assignable(
               context,
               left,
               right) ||
           vitte_type_assignable(
               context,
               right,
               left);
}

/* ========================================================================= */
/* Numeric promotion                                                         */
/* ========================================================================= */

vitte_type_id_t
vitte_type_common_numeric(
    const vitte_type_context_t *context,
    vitte_type_id_t left_id,
    vitte_type_id_t right_id)
{
    const vitte_type_t *left;
    const vitte_type_t *right;

    if (!vitte_type_context_structurally_valid(
            context)) {
        return VITTE_TYPE_INVALID_ID;
    }

    left = vitte_type_get_internal(
        context,
        left_id);

    right = vitte_type_get_internal(
        context,
        right_id);

    if (left == NULL || right == NULL) {
        return VITTE_TYPE_INVALID_ID;
    }

    if (!vitte_type_is_numeric(
            context,
            left_id) ||
        !vitte_type_is_numeric(
            context,
            right_id)) {
        return VITTE_TYPE_INVALID_ID;
    }

    if (left_id == right_id) {
        return left_id;
    }

    if (left->kind ==
            VITTE_TYPE_KIND_F64 ||
        right->kind ==
            VITTE_TYPE_KIND_F64) {
        return context->type_f64;
    }

    if (left->kind ==
            VITTE_TYPE_KIND_F32 ||
        right->kind ==
            VITTE_TYPE_KIND_F32) {
        /*
         * Integer + f32 is promoted to f32. Backend/lowering can choose
         * stricter diagnostics for precision-sensitive contexts.
         */
        return context->type_f32;
    }

    {
        const unsigned int left_bits =
            vitte_type_integer_bits(
                left->kind);

        const unsigned int right_bits =
            vitte_type_integer_bits(
                right->kind);

        const bool left_signed =
            vitte_type_integer_signed_kind(
                left->kind);

        const bool right_signed =
            vitte_type_integer_signed_kind(
                right->kind);

        unsigned int result_bits =
            left_bits > right_bits
                ? left_bits
                : right_bits;

        bool result_signed;

        if (left_signed == right_signed) {
            result_signed = left_signed;
        } else {
            /*
             * Mixed signedness:
             *
             * If the signed operand is strictly wider, retain signedness.
             * Otherwise choose unsigned at the common width.
             */
            if (left_signed &&
                left_bits > right_bits) {
                result_signed = true;
            } else if (right_signed &&
                       right_bits > left_bits) {
                result_signed = true;
            } else {
                result_signed = false;
            }
        }

        return vitte_type_integer_for_bits(
            context,
            result_bits,
            result_signed);
    }
}

/* ========================================================================= */
/* Type properties                                                           */
/* ========================================================================= */

vitte_type_id_t
vitte_type_element(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    if (type == NULL) {
        return VITTE_TYPE_INVALID_ID;
    }

    return type->element_type;
}

vitte_type_id_t
vitte_type_return(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    if (type == NULL) {
        return VITTE_TYPE_INVALID_ID;
    }

    return type->return_type;
}

size_t
vitte_type_parameter_count(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    if (type == NULL) {
        return 0u;
    }

    return type->parameter_count;
}

vitte_type_id_t
vitte_type_parameter(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id,
    size_t index)
{
    const vitte_type_t *type =
        vitte_type_get_internal(
            context,
            type_id);

    if (type == NULL ||
        index >= type->parameter_count) {
        return VITTE_TYPE_INVALID_ID;
    }

    return type->parameters[index];
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_type_validate(
    vitte_type_context_t *context)
{
    size_t index;

    if (!vitte_type_context_structurally_valid(
            context)) {
        return false;
    }

    context->stats.validation_runs++;

    if (context->type_count >
            context->max_types ||
        context->parameter_count >
            context->max_parameters) {
        context->stats.validation_failures++;

        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_VALIDATION);
    }

    for (index = 0u;
         index < context->type_count;
         ++index) {
        const vitte_type_t *type =
            &context->types[index];

        size_t parameter_index;

        if (type->id !=
            (vitte_type_id_t)(index + 1u)) {
            context->stats.validation_failures++;

            return vitte_type_fail(
                context,
                VITTE_TYPE_ERROR_VALIDATION);
        }

        if (type->kind <=
                VITTE_TYPE_KIND_INVALID ||
            type->kind >=
                VITTE_TYPE_KIND_COUNT) {
            context->stats.validation_failures++;

            return vitte_type_fail(
                context,
                VITTE_TYPE_ERROR_VALIDATION);
        }

        if (type->name_length != 0u &&
            type->name == NULL) {
            context->stats.validation_failures++;

            return vitte_type_fail(
                context,
                VITTE_TYPE_ERROR_VALIDATION);
        }

        if (type->name != NULL &&
            type->name[type->name_length] !=
                '\0') {
            context->stats.validation_failures++;

            return vitte_type_fail(
                context,
                VITTE_TYPE_ERROR_VALIDATION);
        }

        if (type->parameter_count != 0u &&
            type->parameters == NULL) {
            context->stats.validation_failures++;

            return vitte_type_fail(
                context,
                VITTE_TYPE_ERROR_VALIDATION);
        }

        if (type->parameter_offset >
            context->parameter_count) {
            context->stats.validation_failures++;

            return vitte_type_fail(
                context,
                VITTE_TYPE_ERROR_VALIDATION);
        }

        if (type->parameter_count >
            context->parameter_count -
                type->parameter_offset) {
            context->stats.validation_failures++;

            return vitte_type_fail(
                context,
                VITTE_TYPE_ERROR_VALIDATION);
        }

        if (type->element_type !=
                VITTE_TYPE_INVALID_ID &&
            !vitte_type_id_in_context(
                context,
                type->element_type)) {
            context->stats.validation_failures++;

            return vitte_type_fail(
                context,
                VITTE_TYPE_ERROR_VALIDATION);
        }

        if (type->return_type !=
                VITTE_TYPE_INVALID_ID &&
            !vitte_type_id_in_context(
                context,
                type->return_type)) {
            context->stats.validation_failures++;

            return vitte_type_fail(
                context,
                VITTE_TYPE_ERROR_VALIDATION);
        }

        for (parameter_index = 0u;
             parameter_index <
                 type->parameter_count;
             ++parameter_index) {
            if (!vitte_type_id_in_context(
                    context,
                    type->parameters[
                        parameter_index])) {
                context->stats.validation_failures++;

                return vitte_type_fail(
                    context,
                    VITTE_TYPE_ERROR_VALIDATION);
            }

            if (context->parameters[
                    type->parameter_offset +
                    parameter_index] !=
                type->parameters[
                    parameter_index]) {
                context->stats.validation_failures++;

                return vitte_type_fail(
                    context,
                    VITTE_TYPE_ERROR_VALIDATION);
            }
        }

        {
            vitte_type_descriptor_t descriptor;
            uint64_t expected_hash;

            memset(
                &descriptor,
                0,
                sizeof(descriptor));

            descriptor.kind = type->kind;
            descriptor.flags = type->flags;
            descriptor.element_type =
                type->element_type;
            descriptor.return_type =
                type->return_type;
            descriptor.array_length =
                type->array_length;
            descriptor.symbol_id =
                type->symbol_id;
            descriptor.generic_index =
                type->generic_index;
            descriptor.name = type->name;
            descriptor.name_length =
                type->name_length;
            descriptor.parameters =
                type->parameters;
            descriptor.parameter_count =
                type->parameter_count;

            expected_hash =
                vitte_type_descriptor_hash(
                    &descriptor);

            if (expected_hash !=
                type->structural_hash) {
                context->stats.validation_failures++;

                return vitte_type_fail(
                    context,
                    VITTE_TYPE_ERROR_VALIDATION);
            }
        }
    }

    if (!vitte_type_id_in_context(
            context,
            context->type_error) ||
        !vitte_type_id_in_context(
            context,
            context->type_void) ||
        !vitte_type_id_in_context(
            context,
            context->type_never) ||
        !vitte_type_id_in_context(
            context,
            context->type_bool) ||
        !vitte_type_id_in_context(
            context,
            context->type_i64) ||
        !vitte_type_id_in_context(
            context,
            context->type_u64) ||
        !vitte_type_id_in_context(
            context,
            context->type_f64) ||
        !vitte_type_id_in_context(
            context,
            context->type_char) ||
        !vitte_type_id_in_context(
            context,
            context->type_string) ||
        !vitte_type_id_in_context(
            context,
            context->type_null)) {
        context->stats.validation_failures++;

        return vitte_type_fail(
            context,
            VITTE_TYPE_ERROR_VALIDATION);
    }

    context->last_error =
        VITTE_TYPE_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_type_fingerprint(
    vitte_type_context_t *context)
{
    uint64_t hash;
    size_t index;

    if (!vitte_type_context_structurally_valid(
            context)) {
        return UINT64_C(0);
    }

    context->stats.hash_runs++;

    hash = VITTE_TYPE_FNV_OFFSET;

    hash = vitte_type_hash_size(
        hash,
        context->type_count);

    for (index = 0u;
         index < context->type_count;
         ++index) {
        const vitte_type_t *type =
            &context->types[index];

        size_t parameter_index;

        hash = vitte_type_hash_u64(
            hash,
            type->id);

        hash = vitte_type_hash_u64(
            hash,
            (uint64_t)type->kind);

        hash = vitte_type_hash_u64(
            hash,
            type->flags);

        hash = vitte_type_hash_u64(
            hash,
            type->element_type);

        hash = vitte_type_hash_u64(
            hash,
            type->return_type);

        hash = vitte_type_hash_size(
            hash,
            type->array_length);

        hash = vitte_type_hash_u64(
            hash,
            type->symbol_id);

        hash = vitte_type_hash_size(
            hash,
            type->generic_index);

        hash = vitte_type_hash_size(
            hash,
            type->name_length);

        hash = vitte_type_hash_bytes(
            hash,
            type->name,
            type->name_length);

        hash = vitte_type_hash_size(
            hash,
            type->parameter_count);

        for (parameter_index = 0u;
             parameter_index <
                 type->parameter_count;
             ++parameter_index) {
            hash = vitte_type_hash_u64(
                hash,
                type->parameters[
                    parameter_index]);
        }

        hash = vitte_type_hash_bool(
            hash,
            (type->flags &
             VITTE_TYPE_FLAG_MUTABLE) != 0u);
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_type_stats_t
vitte_type_stats(
    const vitte_type_context_t *context)
{
    vitte_type_stats_t empty;

    memset(&empty, 0, sizeof(empty));

    if (!vitte_type_context_structurally_valid(
            context)) {
        return empty;
    }

    return context->stats;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_type_translation_unit_anchor(void)
{
    /*
     * Stable link-time anchor for this translation unit.
     */
}
