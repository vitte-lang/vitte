/*
 * Vitte Compiler
 * src/sema/sema.c
 *
 * Semantic analysis implementation.
 *
 * Public contract: sema.h
 *
 * Responsibilities:
 *   - semantic context lifecycle;
 *   - AST semantic traversal;
 *   - lexical scope construction;
 *   - declaration registration;
 *   - name resolution;
 *   - duplicate declaration diagnostics;
 *   - visibility checking;
 *   - lexical shadowing tracking;
 *   - type creation and interning;
 *   - primitive/builtin type management;
 *   - type compatibility;
 *   - implicit conversion classification;
 *   - unary/binary operator checking;
 *   - assignment checking;
 *   - call checking;
 *   - procedure return checking;
 *   - boolean-condition checking;
 *   - member/index checking;
 *   - control-flow context validation;
 *   - unreachable-code detection hooks;
 *   - AST-node semantic information;
 *   - constant-value metadata;
 *   - diagnostic collection;
 *   - validation;
 *   - deterministic fingerprints;
 *   - statistics;
 *   - bounded resource usage.
 *
 * Design:
 *   - ISO C17;
 *   - sema.h is the single public contract;
 *   - parser.h and scope.h are imported by sema.h;
 *   - no duplicate public declarations in this translation unit;
 *   - no global mutable semantic state;
 *   - deterministic analysis;
 *   - stable semantic type IDs;
 *   - stable semantic symbol references;
 *   - explicit ownership;
 *   - bounded recursion and allocations;
 *   - suitable for strict compiler builds.
 */

#include "sema.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Private constants                                                         */
/* ========================================================================= */

#define VITTE_SEMA_PRIVATE_NO_INDEX SIZE_MAX

static bool
vitte_sema_space_scope_private(
    const vitte_sema_t *context,
    const vitte_symbol_t *symbol,
    const vitte_ast_node_t *declaration,
    vitte_scope_id_t *scope_id);

/* ========================================================================= */
/* Private arithmetic                                                        */
/* ========================================================================= */

static bool
vitte_sema_size_add(
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
vitte_sema_size_mul(
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
vitte_sema_u64_add_sat(
    uint64_t left,
    uint64_t right)
{
    if (UINT64_MAX - left < right) {
        return UINT64_MAX;
    }

    return left + right;
}

/* ========================================================================= */
/* Private hashing                                                           */
/* ========================================================================= */

static uint64_t
vitte_sema_hash_byte(
    uint64_t hash,
    unsigned char value)
{
    hash ^= (uint64_t)value;
    hash *= VITTE_SEMA_FNV_PRIME;

    return hash;
}

static uint64_t
vitte_sema_hash_bytes(
    uint64_t hash,
    const unsigned char *bytes,
    size_t length)
{
    size_t index;

    if (bytes == NULL &&
        length != 0u) {
        return hash;
    }

    for (index = 0u;
         index < length;
         ++index) {
        hash =
            vitte_sema_hash_byte(
                hash,
                bytes[index]);
    }

    return hash;
}

static uint64_t
vitte_sema_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned int shift;

    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        hash =
            vitte_sema_hash_byte(
                hash,
                (unsigned char)(
                    (value >> shift) &
                    UINT64_C(0xff)));
    }

    return hash;
}

/* ========================================================================= */
/* Private allocation                                                        */
/* ========================================================================= */

static void *
vitte_sema_reallocate(
    vitte_sema_t *context,
    void *memory,
    size_t count,
    size_t element_size)
{
    size_t bytes;
    void *result;

    if (context == NULL) {
        return NULL;
    }

    if (!vitte_sema_size_mul(
            count,
            element_size,
            &bytes)) {
        context->last_error =
            VITTE_SEMA_ERROR_OVERFLOW;

        return NULL;
    }

    if (bytes == 0u) {
        bytes = 1u;
    }

    result = realloc(memory, bytes);

    if (result == NULL) {
        context->stats.allocation_failures =
            vitte_sema_u64_add_sat(
                context->stats.allocation_failures,
                UINT64_C(1));

        context->last_error =
            VITTE_SEMA_ERROR_OUT_OF_MEMORY;

        return NULL;
    }

    context->stats.reallocations =
        vitte_sema_u64_add_sat(
            context->stats.reallocations,
            UINT64_C(1));

    return result;
}

/* ========================================================================= */
/* Private context helpers                                                   */
/* ========================================================================= */

static bool
vitte_sema_context_valid_private(
    const vitte_sema_t *context)
{
    if (context == NULL) {
        return false;
    }

    if (context->magic !=
        VITTE_SEMA_MAGIC) {
        return false;
    }

    if (context->state <=
            VITTE_SEMA_STATE_INVALID ||
        context->state >=
            VITTE_SEMA_STATE_DESTROYED) {
        return false;
    }

    if (context->parser == NULL) {
        return false;
    }

    if (context->type_count >
        context->type_capacity) {
        return false;
    }

    if (context->node_info_count >
        context->node_info_capacity) {
        return false;
    }

    if (context->diagnostic_count >
        context->diagnostic_capacity) {
        return false;
    }

    if (context->type_count != 0u &&
        context->types == NULL) {
        return false;
    }

    if (context->node_info_count != 0u &&
        context->node_info == NULL) {
        return false;
    }

    if (context->diagnostic_count != 0u &&
        context->diagnostics == NULL) {
        return false;
    }

    if (context->max_types == 0u ||
        context->max_diagnostics == 0u ||
        context->max_recursion_depth == 0u) {
        return false;
    }

    if (context->type_count >
            context->max_types ||
        context->diagnostic_count >
            context->max_diagnostics) {
        return false;
    }

    return true;
}

static bool
vitte_sema_fail(
    vitte_sema_t *context,
    vitte_sema_error_t error)
{
    if (context != NULL &&
        context->magic ==
            VITTE_SEMA_MAGIC) {
        context->last_error = error;

        if (context->state !=
            VITTE_SEMA_STATE_DESTROYED) {
            context->state =
                VITTE_SEMA_STATE_FAILED;
        }

        context->stats.failures =
            vitte_sema_u64_add_sat(
                context->stats.failures,
                UINT64_C(1));
    }

    return false;
}

static void
vitte_sema_set_error(
    vitte_sema_t *context,
    vitte_sema_error_t error)
{
    if (context != NULL &&
        context->magic ==
            VITTE_SEMA_MAGIC) {
        context->last_error = error;
    }
}

/* ========================================================================= */
/* Private capacity helpers                                                  */
/* ========================================================================= */

static bool
vitte_sema_reserve_types(
    vitte_sema_t *context,
    size_t required)
{
    size_t capacity;
    vitte_sema_type_t *types;

    if (required <=
        context->type_capacity) {
        return true;
    }

    if (required >
        context->max_types) {
        context->last_error =
            VITTE_SEMA_ERROR_TYPE_LIMIT;

        return false;
    }

    capacity = context->type_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_SEMA_DEFAULT_INITIAL_TYPE_CAPACITY;
    }

    while (capacity < required) {
        size_t next;

        if (capacity >=
            context->max_types) {
            capacity =
                context->max_types;
            break;
        }

        if (capacity > SIZE_MAX / 2u) {
            next = context->max_types;
        } else {
            next = capacity * 2u;
        }

        if (next >
            context->max_types) {
            next =
                context->max_types;
        }

        if (next <= capacity) {
            context->last_error =
                VITTE_SEMA_ERROR_OVERFLOW;

            return false;
        }

        capacity = next;
    }

    types =
        (vitte_sema_type_t *)
        vitte_sema_reallocate(
            context,
            context->types,
            capacity,
            sizeof(*types));

    if (types == NULL) {
        return false;
    }

    if (capacity >
        context->type_capacity) {
        size_t old_bytes;
        size_t new_bytes;

        if (!vitte_sema_size_mul(
                context->type_capacity,
                sizeof(*types),
                &old_bytes) ||
            !vitte_sema_size_mul(
                capacity,
                sizeof(*types),
                &new_bytes)) {
            context->last_error =
                VITTE_SEMA_ERROR_OVERFLOW;

            return false;
        }

        memset(
            ((unsigned char *)types) +
                old_bytes,
            0,
            new_bytes - old_bytes);
    }

    context->types = types;
    context->type_capacity = capacity;

    return true;
}

static bool
vitte_sema_reserve_node_info(
    vitte_sema_t *context,
    size_t required)
{
    size_t capacity;
    vitte_sema_node_info_t *info;

    if (required <=
        context->node_info_capacity) {
        return true;
    }

    capacity =
        context->node_info_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_SEMA_DEFAULT_INITIAL_NODE_CAPACITY;
    }

    while (capacity < required) {
        if (capacity > SIZE_MAX / 2u) {
            context->last_error =
                VITTE_SEMA_ERROR_OVERFLOW;

            return false;
        }

        capacity *= 2u;
    }

    info =
        (vitte_sema_node_info_t *)
        vitte_sema_reallocate(
            context,
            context->node_info,
            capacity,
            sizeof(*info));

    if (info == NULL) {
        return false;
    }

    if (capacity >
        context->node_info_capacity) {
        size_t old_bytes;
        size_t new_bytes;

        if (!vitte_sema_size_mul(
                context->node_info_capacity,
                sizeof(*info),
                &old_bytes) ||
            !vitte_sema_size_mul(
                capacity,
                sizeof(*info),
                &new_bytes)) {
            context->last_error =
                VITTE_SEMA_ERROR_OVERFLOW;

            return false;
        }

        memset(
            ((unsigned char *)info) +
                old_bytes,
            0,
            new_bytes - old_bytes);
    }

    context->node_info = info;
    context->node_info_capacity = capacity;

    return true;
}

static bool
vitte_sema_reserve_diagnostics(
    vitte_sema_t *context,
    size_t required)
{
    size_t capacity;
    vitte_sema_diagnostic_t *diagnostics;

    if (required <=
        context->diagnostic_capacity) {
        return true;
    }

    if (required >
        context->max_diagnostics) {
        context->last_error =
            VITTE_SEMA_ERROR_DIAGNOSTIC_LIMIT;

        return false;
    }

    capacity =
        context->diagnostic_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_SEMA_DEFAULT_INITIAL_DIAGNOSTIC_CAPACITY;
    }

    while (capacity < required) {
        size_t next;

        if (capacity >=
            context->max_diagnostics) {
            capacity =
                context->max_diagnostics;
            break;
        }

        if (capacity > SIZE_MAX / 2u) {
            next =
                context->max_diagnostics;
        } else {
            next = capacity * 2u;
        }

        if (next >
            context->max_diagnostics) {
            next =
                context->max_diagnostics;
        }

        if (next <= capacity) {
            context->last_error =
                VITTE_SEMA_ERROR_OVERFLOW;

            return false;
        }

        capacity = next;
    }

    diagnostics =
        (vitte_sema_diagnostic_t *)
        vitte_sema_reallocate(
            context,
            context->diagnostics,
            capacity,
            sizeof(*diagnostics));

    if (diagnostics == NULL) {
        return false;
    }

    if (capacity >
        context->diagnostic_capacity) {
        size_t old_bytes;
        size_t new_bytes;

        if (!vitte_sema_size_mul(
                context->diagnostic_capacity,
                sizeof(*diagnostics),
                &old_bytes) ||
            !vitte_sema_size_mul(
                capacity,
                sizeof(*diagnostics),
                &new_bytes)) {
            context->last_error =
                VITTE_SEMA_ERROR_OVERFLOW;

            return false;
        }

        memset(
            ((unsigned char *)diagnostics) +
                old_bytes,
            0,
            new_bytes - old_bytes);
    }

    context->diagnostics = diagnostics;
    context->diagnostic_capacity = capacity;

    return true;
}

/* ========================================================================= */
/* Span conversion                                                           */
/* ========================================================================= */

static vitte_scope_span_t
vitte_sema_scope_span_from_parser(
    vitte_parser_span_t span)
{
    vitte_scope_span_t result;

    result.file_id = span.file_id;
    result.begin = span.begin;
    result.end = span.end;
    result.line = span.line;
    result.column = span.column;
    result.valid = span.valid;

    return result;
}

/* ========================================================================= */
/* AST helpers                                                               */
/* ========================================================================= */

static const vitte_ast_node_t *
vitte_sema_node(
    const vitte_sema_t *context,
    vitte_ast_node_id_t id)
{
    if (context == NULL ||
        context->parser == NULL ||
        id == VITTE_AST_INVALID_ID) {
        return NULL;
    }

    return vitte_parser_get_node(
        context->parser,
        id);
}

static const vitte_token_t *
vitte_sema_node_token(
    const vitte_sema_t *context,
    const vitte_ast_node_t *node)
{
    if (context == NULL ||
        context->parser == NULL ||
        node == NULL ||
        node->token_index ==
            VITTE_PARSER_NO_TOKEN ||
        node->token_index >=
            context->parser->token_count) {
        return NULL;
    }

    return &context->parser->tokens[
        node->token_index];
}

static bool
vitte_sema_token_name(
    const vitte_token_t *token,
    const char **name,
    size_t *length)
{
    if (token == NULL ||
        name == NULL ||
        length == NULL ||
        (token->kind != VITTE_TOKEN_IDENTIFIER &&
         token->kind != VITTE_TOKEN_KW_NULL) ||
        token->lexeme == NULL ||
        token->length == 0u) {
        return false;
    }

    *name = token->lexeme;
    *length = token->length;

    return true;
}

/* ========================================================================= */
/* Diagnostics                                                               */
/* ========================================================================= */

static bool
vitte_sema_add_diagnostic(
    vitte_sema_t *context,
    vitte_sema_diagnostic_kind_t kind,
    vitte_sema_diagnostic_code_t code,
    vitte_parser_span_t span,
    vitte_ast_node_id_t node_id,
    vitte_symbol_id_t symbol_id,
    vitte_sema_type_id_t expected_type,
    vitte_sema_type_id_t found_type)
{
    size_t required;
    vitte_sema_diagnostic_t *diagnostic;

    if (context == NULL) {
        return false;
    }

    if (context->diagnostic_count >=
        context->max_diagnostics) {
        context->last_error =
            VITTE_SEMA_ERROR_DIAGNOSTIC_LIMIT;

        return false;
    }

    if (!vitte_sema_size_add(
            context->diagnostic_count,
            1u,
            &required)) {
        context->last_error =
            VITTE_SEMA_ERROR_OVERFLOW;

        return false;
    }

    if (!vitte_sema_reserve_diagnostics(
            context,
            required)) {
        return false;
    }

    diagnostic =
        &context->diagnostics[
            context->diagnostic_count];

    memset(
        diagnostic,
        0,
        sizeof(*diagnostic));

    diagnostic->kind = kind;
    diagnostic->code = code;
    diagnostic->span = span;
    diagnostic->node_id = node_id;
    diagnostic->symbol_id = symbol_id;
    diagnostic->expected_type = expected_type;
    diagnostic->found_type = found_type;

    ++context->diagnostic_count;

    context->stats.diagnostics =
        vitte_sema_u64_add_sat(
            context->stats.diagnostics,
            UINT64_C(1));

    if (kind ==
        VITTE_SEMA_DIAGNOSTIC_ERROR) {
        context->stats.errors =
            vitte_sema_u64_add_sat(
                context->stats.errors,
                UINT64_C(1));
    } else if (kind ==
               VITTE_SEMA_DIAGNOSTIC_WARNING) {
        context->stats.warnings =
            vitte_sema_u64_add_sat(
                context->stats.warnings,
                UINT64_C(1));
    }

    return true;
}

/* ========================================================================= */
/* Type table                                                                */
/* ========================================================================= */

static const vitte_sema_type_t *
vitte_sema_type_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t id)
{
    uint64_t raw;
    size_t index;

    if (context == NULL ||
        id == VITTE_SEMA_INVALID_TYPE_ID) {
        return NULL;
    }

    raw = (uint64_t)id;

    if (raw == UINT64_C(0) ||
        raw > (uint64_t)context->type_count) {
        return NULL;
    }

    raw -= UINT64_C(1);

    if (raw > (uint64_t)SIZE_MAX) {
        return NULL;
    }

    index = (size_t)raw;

    return &context->types[index];
}

static vitte_sema_type_t *
vitte_sema_type_mut_private(
    vitte_sema_t *context,
    vitte_sema_type_id_t id)
{
    return (vitte_sema_type_t *)
        vitte_sema_type_private(
            context,
            id);
}

static bool
vitte_sema_type_equal_descriptor(
    const vitte_sema_type_t *type,
    vitte_sema_type_kind_t kind,
    vitte_sema_type_id_t element_type,
    size_t array_length,
    vitte_symbol_id_t symbol_id,
    bool is_mutable)
{
    if (type == NULL) {
        return false;
    }

    return type->kind == kind &&
           type->element_type == element_type &&
           type->array_length == array_length &&
           type->symbol_id == symbol_id &&
           type->is_mutable == is_mutable;
}

static vitte_sema_type_id_t
vitte_sema_find_type(
    const vitte_sema_t *context,
    vitte_sema_type_kind_t kind,
    vitte_sema_type_id_t element_type,
    size_t array_length,
    vitte_symbol_id_t symbol_id,
    bool is_mutable)
{
    size_t index;

    for (index = 0u;
         index < context->type_count;
         ++index) {
        if (vitte_sema_type_equal_descriptor(
                &context->types[index],
                kind,
                element_type,
                array_length,
                symbol_id,
                is_mutable)) {
            return context->types[index].id;
        }
    }

    return VITTE_SEMA_INVALID_TYPE_ID;
}

static vitte_sema_type_id_t
vitte_sema_create_type_private(
    vitte_sema_t *context,
    vitte_sema_type_kind_t kind,
    vitte_sema_type_id_t element_type,
    size_t array_length,
    vitte_symbol_id_t symbol_id,
    bool is_mutable)
{
    vitte_sema_type_id_t existing;
    size_t required;
    size_t index;
    vitte_sema_type_t *type;

    existing =
        vitte_sema_find_type(
            context,
            kind,
            element_type,
            array_length,
            symbol_id,
            is_mutable);

    if (existing !=
        VITTE_SEMA_INVALID_TYPE_ID) {
        context->stats.type_intern_hits =
            vitte_sema_u64_add_sat(
                context->stats.type_intern_hits,
                UINT64_C(1));

        return existing;
    }

    if (context->type_count >=
        context->max_types) {
        context->last_error =
            VITTE_SEMA_ERROR_TYPE_LIMIT;

        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    if (!vitte_sema_size_add(
            context->type_count,
            1u,
            &required)) {
        context->last_error =
            VITTE_SEMA_ERROR_OVERFLOW;

        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    if (!vitte_sema_reserve_types(
            context,
            required)) {
        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    index = context->type_count;

    if ((uint64_t)index >= UINT64_MAX) {
        context->last_error =
            VITTE_SEMA_ERROR_OVERFLOW;

        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    type = &context->types[index];

    memset(type, 0, sizeof(*type));

    type->id =
        (vitte_sema_type_id_t)(index + 1u);

    type->kind = kind;
    type->element_type = element_type;
    type->array_length = array_length;
    type->symbol_id = symbol_id;
    type->is_mutable = is_mutable;

    ++context->type_count;

    context->stats.types_created =
        vitte_sema_u64_add_sat(
            context->stats.types_created,
            UINT64_C(1));

    return type->id;
}

/* ========================================================================= */
/* Builtin types                                                             */
/* ========================================================================= */

static bool
vitte_sema_create_builtin_types(
    vitte_sema_t *context)
{
    context->type_error =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_ERROR,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    context->type_u8 =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_U8,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    context->type_void =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_VOID,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    context->type_bool =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_BOOL,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    context->type_i64 =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_I64,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    context->type_u64 =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_U64,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    context->type_f64 =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_F64,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    context->type_char =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_CHAR,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    context->type_string =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_STRING,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    context->type_null =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_NULL,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            false);

    if (context->type_error ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_void ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_bool ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_i64 ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_u64 ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_u8 ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_f64 ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_char ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_string ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_null ==
            VITTE_SEMA_INVALID_TYPE_ID) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Node information                                                          */
/* ========================================================================= */

static vitte_sema_node_info_t *
vitte_sema_node_info_mut_private(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id)
{
    uint64_t raw;
    size_t index;

    if (context == NULL ||
        node_id == VITTE_AST_INVALID_ID) {
        return NULL;
    }

    raw = (uint64_t)node_id;

    if (raw == UINT64_C(0)) {
        return NULL;
    }

    raw -= UINT64_C(1);

    if (raw > (uint64_t)SIZE_MAX) {
        return NULL;
    }

    index = (size_t)raw;

    if (index >=
        context->node_info_count) {
        return NULL;
    }

    return &context->node_info[index];
}

static const vitte_sema_node_info_t *
vitte_sema_node_info_private(
    const vitte_sema_t *context,
    vitte_ast_node_id_t node_id)
{
    return (const vitte_sema_node_info_t *)
        vitte_sema_node_info_mut_private(
            (vitte_sema_t *)context,
            node_id);
}

static bool
vitte_sema_prepare_node_info(
    vitte_sema_t *context)
{
    size_t count;
    size_t index;

    if (context == NULL ||
        context->parser == NULL) {
        return false;
    }

    count = context->parser->node_count;

    if (!vitte_sema_reserve_node_info(
            context,
            count)) {
        return false;
    }

    if (count != 0u) {
        memset(
            context->node_info,
            0,
            count *
                sizeof(*context->node_info));
    }

    context->node_info_count = count;

    for (index = 0u;
         index < count;
         ++index) {
        context->node_info[index].node_id =
            (vitte_ast_node_id_t)(index + 1u);

        context->node_info[index].type_id =
            VITTE_SEMA_INVALID_TYPE_ID;

        context->node_info[index].symbol_id =
            VITTE_SCOPE_INVALID_SYMBOL_ID;

        context->node_info[index].scope_id =
            VITTE_SCOPE_INVALID_SCOPE_ID;

        context->node_info[index].constant.kind =
            VITTE_SEMA_CONSTANT_NONE;
    }

    return true;
}

/* ========================================================================= */
/* Type predicates                                                           */
/* ========================================================================= */

static bool
vitte_sema_type_is_integer_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    const vitte_sema_type_t *type;

    type =
        vitte_sema_type_private(
            context,
            type_id);

    if (type == NULL) {
        return false;
    }

    return type->kind ==
               VITTE_SEMA_TYPE_I64 ||
           type->kind ==
               VITTE_SEMA_TYPE_U64 ||
           type->kind ==
               VITTE_SEMA_TYPE_U8 ||
           type->kind ==
               VITTE_SEMA_TYPE_CHAR;
}

static bool
vitte_sema_type_is_float_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    const vitte_sema_type_t *type;

    type =
        vitte_sema_type_private(
            context,
            type_id);

    return type != NULL &&
           type->kind ==
               VITTE_SEMA_TYPE_F64;
}

static bool
vitte_sema_type_is_numeric_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    return vitte_sema_type_is_integer_private(
               context,
               type_id) ||
           vitte_sema_type_is_float_private(
               context,
               type_id);
}

static bool
vitte_sema_type_is_bool_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    return type_id ==
           context->type_bool;
}

static bool
vitte_sema_type_is_error_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    return type_id ==
           context->type_error;
}

static bool
vitte_sema_type_is_pointer_like_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    const vitte_sema_type_t *type;

    type =
        vitte_sema_type_private(
            context,
            type_id);

    if (type == NULL) {
        return false;
    }

    return type->kind ==
               VITTE_SEMA_TYPE_POINTER ||
           type->kind ==
               VITTE_SEMA_TYPE_REFERENCE;
}

static vitte_sema_type_id_t
vitte_sema_value_type_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    const vitte_sema_type_t *type;

    type =
        vitte_sema_type_private(
            context,
            type_id);
    while (type != NULL &&
           type->kind ==
               VITTE_SEMA_TYPE_REFERENCE) {
        type_id = type->element_type;
        type =
            vitte_sema_type_private(
                context,
                type_id);
    }

    return type_id;
}

/* ========================================================================= */
/* Type compatibility                                                        */
/* ========================================================================= */

static bool
vitte_sema_same_declaration_private(
    const void *left_payload,
    const void *right_payload);

static bool
vitte_sema_type_same_nominal_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t left,
    vitte_sema_type_id_t right)
{
    const vitte_sema_type_t *left_type;
    const vitte_sema_type_t *right_type;
    const vitte_symbol_t *left_symbol;
    const vitte_symbol_t *right_symbol;

    if (left == right) {
        return true;
    }

    left_type =
        vitte_sema_type_private(
            context,
            left);
    right_type =
        vitte_sema_type_private(
            context,
            right);
    if (left_type == NULL ||
        right_type == NULL ||
        left_type->kind != VITTE_SEMA_TYPE_NAMED ||
        right_type->kind != VITTE_SEMA_TYPE_NAMED) {
        return false;
    }

    left_symbol =
        vitte_scope_get_symbol(
            &context->scopes,
            left_type->symbol_id);
    right_symbol =
        vitte_scope_get_symbol(
            &context->scopes,
            right_type->symbol_id);

    return left_symbol != NULL &&
           right_symbol != NULL &&
           left_symbol->payload != NULL &&
           vitte_sema_same_declaration_private(
               left_symbol->payload,
               right_symbol->payload);
}

static bool
vitte_sema_type_identical_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t left,
    vitte_sema_type_id_t right)
{
    const vitte_sema_type_t *left_type;
    const vitte_sema_type_t *right_type;

    if (left == right ||
        vitte_sema_type_same_nominal_private(
            context,
            left,
            right)) {
        return true;
    }

    left_type =
        vitte_sema_type_private(
            context,
            left);
    right_type =
        vitte_sema_type_private(
            context,
            right);
    if (left_type == NULL ||
        right_type == NULL ||
        left_type->kind != right_type->kind) {
        return false;
    }

    switch (left_type->kind) {
        case VITTE_SEMA_TYPE_REFERENCE:
        case VITTE_SEMA_TYPE_POINTER:
            return left_type->is_mutable ==
                       right_type->is_mutable &&
                   vitte_sema_type_identical_private(
                       context,
                       left_type->element_type,
                       right_type->element_type);

        case VITTE_SEMA_TYPE_ARRAY:
            return left_type->array_length ==
                       right_type->array_length &&
                   vitte_sema_type_identical_private(
                       context,
                       left_type->element_type,
                       right_type->element_type);

        default:
            return false;
    }
}

static bool
vitte_sema_type_assignable_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t destination,
    vitte_sema_type_id_t source)
{
    const vitte_sema_type_t *destination_type;
    const vitte_sema_type_t *source_type;

    if (destination ==
        source) {
        return true;
    }

    if (vitte_sema_type_is_error_private(
            context,
            destination) ||
        vitte_sema_type_is_error_private(
            context,
            source)) {
        return true;
    }

    destination_type =
        vitte_sema_type_private(
            context,
            destination);

    source_type =
        vitte_sema_type_private(
            context,
            source);

    if (destination_type == NULL ||
        source_type == NULL) {
        return false;
    }

    if (vitte_sema_type_identical_private(
            context,
            destination,
            source)) {
        return true;
    }

    if (destination_type->kind ==
            VITTE_SEMA_TYPE_ARRAY &&
        source_type->kind ==
            VITTE_SEMA_TYPE_ARRAY) {
        return (destination_type->array_length == 0u ||
                destination_type->array_length ==
                    source_type->array_length) &&
               vitte_sema_type_assignable_private(
                   context,
                   destination_type->element_type,
                   source_type->element_type);
    }

    /*
     * Numeric widening/conversion policy.
     */
    if (destination_type->kind ==
            VITTE_SEMA_TYPE_F64 &&
        (source_type->kind ==
             VITTE_SEMA_TYPE_I64 ||
         source_type->kind ==
             VITTE_SEMA_TYPE_U64 ||
         source_type->kind ==
             VITTE_SEMA_TYPE_U8 ||
         source_type->kind ==
             VITTE_SEMA_TYPE_CHAR)) {
        return true;
    }

    if ((destination_type->kind ==
             VITTE_SEMA_TYPE_I64 ||
         destination_type->kind ==
             VITTE_SEMA_TYPE_U64) &&
        (source_type->kind ==
             VITTE_SEMA_TYPE_U8 ||
         source_type->kind ==
             VITTE_SEMA_TYPE_CHAR)) {
        return true;
    }

    if (destination_type->kind ==
            VITTE_SEMA_TYPE_U8 &&
        source_type->kind ==
            VITTE_SEMA_TYPE_U8) {
        return true;
    }

    if ((destination_type->kind ==
             VITTE_SEMA_TYPE_REFERENCE ||
         destination_type->kind ==
             VITTE_SEMA_TYPE_POINTER) &&
        destination_type->kind ==
            source_type->kind &&
        (destination_type->element_type ==
            source_type->element_type ||
        vitte_sema_type_identical_private(
            context,
            destination_type->element_type,
            source_type->element_type)) &&
        (destination_type->is_mutable ==
            source_type->is_mutable ||
         (!destination_type->is_mutable &&
          source_type->is_mutable))) {
        return true;
    }

    /*
     * null -> pointer/reference.
     */
    if ((destination_type->kind ==
             VITTE_SEMA_TYPE_POINTER ||
         destination_type->kind ==
             VITTE_SEMA_TYPE_REFERENCE) &&
        source_type->kind ==
            VITTE_SEMA_TYPE_NULL) {
        return true;
    }

    if (destination_type->kind ==
            VITTE_SEMA_TYPE_POINTER &&
        destination_type->element_type ==
            context->type_u8 &&
        !destination_type->is_mutable &&
        source_type->kind ==
            VITTE_SEMA_TYPE_STRING) {
        return true;
    }

    return false;
}

static bool
vitte_sema_parse_unsigned_integer_literal(
    const vitte_sema_t *context,
    const vitte_ast_node_t *node,
    uint64_t *value)
{
    const vitte_token_t *token;
    size_t index;
    unsigned int base;
    uint64_t parsed;
    bool has_digit;

    if (context == NULL ||
        node == NULL ||
        node->kind !=
            VITTE_AST_NODE_INTEGER_LITERAL ||
        value == NULL) {
        return false;
    }

    token =
        vitte_sema_node_token(
            context,
            node);

    if (token == NULL ||
        token->lexeme == NULL ||
        token->length == 0u) {
        return false;
    }

    base = 10u;
    index = 0u;

    if (token->length >= 2u &&
        token->lexeme[0] == '0') {
        switch (token->lexeme[1]) {
            case 'x':
            case 'X':
                base = 16u;
                index = 2u;
                break;

            case 'b':
            case 'B':
                base = 2u;
                index = 2u;
                break;

            case 'o':
            case 'O':
                base = 8u;
                index = 2u;
                break;

            default:
                break;
        }
    }

    parsed = UINT64_C(0);
    has_digit = false;

    for (; index < token->length; ++index) {
        unsigned int digit;
        unsigned char character;

        character =
            (unsigned char)token->lexeme[index];

        if (character == (unsigned char)'_') {
            continue;
        }

        if (character >= (unsigned char)'0' &&
            character <= (unsigned char)'9') {
            digit =
                (unsigned int)(character -
                               (unsigned char)'0');
        } else if (character >= (unsigned char)'a' &&
                   character <= (unsigned char)'f') {
            digit =
                (unsigned int)(character -
                               (unsigned char)'a') +
                10u;
        } else if (character >= (unsigned char)'A' &&
                   character <= (unsigned char)'F') {
            digit =
                (unsigned int)(character -
                               (unsigned char)'A') +
                10u;
        } else {
            return false;
        }

        if (digit >= base ||
            parsed >
                (UINT64_MAX - (uint64_t)digit) /
                    (uint64_t)base) {
            return false;
        }

        parsed =
            parsed * (uint64_t)base +
            (uint64_t)digit;
        has_digit = true;
    }

    if (!has_digit) {
        return false;
    }

    *value = parsed;
    return true;
}

static bool
vitte_sema_contextualize_integer_literal(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id,
    vitte_sema_type_id_t expected_type)
{
    const vitte_ast_node_t *node;
    vitte_sema_node_info_t *info;
    uint64_t value;

    if (context == NULL ||
        expected_type != context->type_u64) {
        return false;
    }

    node =
        vitte_sema_node(
            context,
            node_id);

    if (node == NULL) {
        return false;
    }

    if (node->kind ==
            VITTE_AST_NODE_GROUP_EXPR &&
        node->child_count == 1u) {
        if (!vitte_sema_contextualize_integer_literal(
                context,
                node->children[0],
                expected_type)) {
            return false;
        }

        info =
            vitte_sema_node_info_mut_private(
                context,
                node_id);

        if (info != NULL) {
            info->type_id = expected_type;
        }

        return true;
    }

    if (!vitte_sema_parse_unsigned_integer_literal(
            context,
            node,
            &value)) {
        return false;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node_id);

    if (info == NULL) {
        return false;
    }

    info->type_id = expected_type;
    info->constant.kind =
        VITTE_SEMA_CONSTANT_UNSIGNED_INTEGER;
    info->constant.value.unsigned_integer_value =
        value;

    return true;
}

static vitte_sema_type_id_t
vitte_sema_common_numeric_type(
    const vitte_sema_t *context,
    vitte_sema_type_id_t left,
    vitte_sema_type_id_t right)
{
    const vitte_sema_type_t *left_type;
    const vitte_sema_type_t *right_type;

    if (!vitte_sema_type_is_numeric_private(
            context,
            left) ||
        !vitte_sema_type_is_numeric_private(
            context,
            right)) {
        return context->type_error;
    }

    left_type =
        vitte_sema_type_private(
            context,
            left);

    right_type =
        vitte_sema_type_private(
            context,
            right);

    if (left_type == NULL ||
        right_type == NULL) {
        return context->type_error;
    }

    if (left_type->kind ==
            VITTE_SEMA_TYPE_F64 ||
        right_type->kind ==
            VITTE_SEMA_TYPE_F64) {
        return context->type_f64;
    }

    if (left_type->kind ==
            VITTE_SEMA_TYPE_U64 &&
        right_type->kind ==
            VITTE_SEMA_TYPE_U64) {
        return context->type_u64;
    }

    if (left_type->kind ==
            VITTE_SEMA_TYPE_U8 &&
        right_type->kind ==
            VITTE_SEMA_TYPE_U8) {
        return context->type_u8;
    }

    if ((left_type->kind ==
             VITTE_SEMA_TYPE_U64 &&
         right_type->kind ==
             VITTE_SEMA_TYPE_U8) ||
        (right_type->kind ==
             VITTE_SEMA_TYPE_U64 &&
         left_type->kind ==
             VITTE_SEMA_TYPE_U8)) {
        return context->type_u64;
    }

    return context->type_i64;
}

/* ========================================================================= */
/* Symbol/type association                                                   */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_type_for_symbol(
    const vitte_sema_t *context,
    vitte_symbol_id_t symbol_id)
{
    size_t index;
    const vitte_symbol_t *symbol;

    if (symbol_id ==
        VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return context->type_error;
    }

    symbol =
        vitte_scope_get_symbol(
            &context->scopes,
            symbol_id);
    if (symbol != NULL &&
        (symbol->flags &
         VITTE_SYMBOL_FLAG_IMPORTED) != 0u &&
        symbol->payload != NULL) {
        const vitte_ast_node_t *declaration;
        const vitte_sema_node_info_t *declaration_info;

        declaration =
            (const vitte_ast_node_t *)symbol->payload;
        declaration_info =
            vitte_sema_node_info_private(
                context,
                declaration->id);
        if (declaration_info != NULL &&
            vitte_sema_node(
                context,
                declaration->id) == declaration &&
            declaration_info->type_id !=
                VITTE_SEMA_INVALID_TYPE_ID) {
            return declaration_info->type_id;
        }
    }

    for (index = 0u;
         index < context->node_info_count;
         ++index) {
        if (context->node_info[index].symbol_id ==
                symbol_id &&
            context->node_info[index].type_id !=
                VITTE_SEMA_INVALID_TYPE_ID) {
            return context->node_info[index].type_id;
        }
    }

    return context->type_error;
}

static bool
vitte_sema_same_declaration_private(
    const void *left_payload,
    const void *right_payload)
{
    const vitte_ast_node_t *left;
    const vitte_ast_node_t *right;

    if (left_payload == right_payload) {
        return left_payload != NULL;
    }
    if (left_payload == NULL ||
        right_payload == NULL) {
        return false;
    }

    left = (const vitte_ast_node_t *)left_payload;
    right = (const vitte_ast_node_t *)right_payload;
    if (left->kind != right->kind ||
        !left->span.valid ||
        !right->span.valid ||
        left->span.file_id != right->span.file_id ||
        left->span.begin != right->span.begin ||
        left->span.end != right->span.end) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Scope management during traversal                                         */
/* ========================================================================= */

static vitte_scope_kind_t
vitte_sema_scope_kind_for_node(
    vitte_ast_node_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_NODE_TRANSLATION_UNIT:
            return VITTE_SCOPE_KIND_ROOT;

        case VITTE_AST_NODE_SPACE_DECL:
            return VITTE_SCOPE_KIND_SPACE;

        case VITTE_AST_NODE_FORM_DECL:
            return VITTE_SCOPE_KIND_FORM;

        case VITTE_AST_NODE_PICK_DECL:
            return VITTE_SCOPE_KIND_PICK;

        case VITTE_AST_NODE_TRAIT_DECL:
            return VITTE_SCOPE_KIND_TRAIT;

        case VITTE_AST_NODE_IMPL_DECL:
            return VITTE_SCOPE_KIND_IMPL;

        case VITTE_AST_NODE_PROC_DECL:
        case VITTE_AST_NODE_EXTERN_PROC_DECL:
            return VITTE_SCOPE_KIND_PROC;

        case VITTE_AST_NODE_BLOCK:
            return VITTE_SCOPE_KIND_BLOCK;

        case VITTE_AST_NODE_WHILE_STMT:
        case VITTE_AST_NODE_LOOP_STMT:
        case VITTE_AST_NODE_FOR_STMT:
            return VITTE_SCOPE_KIND_LOOP;

        case VITTE_AST_NODE_MATCH_STMT:
            return VITTE_SCOPE_KIND_MATCH;

        case VITTE_AST_NODE_MATCH_ARM:
            return VITTE_SCOPE_KIND_MATCH_ARM;

        case VITTE_AST_NODE_MACRO_DECL:
            return VITTE_SCOPE_KIND_MACRO;

        case VITTE_AST_NODE_TEST_DECL:
            return VITTE_SCOPE_KIND_TEST;

        case VITTE_AST_NODE_UNSAFE_BLOCK:
            return VITTE_SCOPE_KIND_UNSAFE;

        case VITTE_AST_NODE_INVALID:
        case VITTE_AST_NODE_IDENTIFIER:
        case VITTE_AST_NODE_PATH:
        case VITTE_AST_NODE_USE_DECL:
        case VITTE_AST_NODE_CONST_DECL:
        case VITTE_AST_NODE_STATIC_DECL:
        case VITTE_AST_NODE_TYPE_DECL:
        case VITTE_AST_NODE_OPAQUE_DECL:
        case VITTE_AST_NODE_FIELD_DECL:
        case VITTE_AST_NODE_VARIANT_DECL:
        case VITTE_AST_NODE_PARAMETER:
        case VITTE_AST_NODE_GENERIC_PARAM:
        case VITTE_AST_NODE_WHERE_CLAUSE:
        case VITTE_AST_NODE_WHERE_PREDICATE:
        case VITTE_AST_NODE_LET_STMT:
        case VITTE_AST_NODE_RETURN_STMT:
        case VITTE_AST_NODE_DEFER_STMT:
        case VITTE_AST_NODE_IF_STMT:
        case VITTE_AST_NODE_IF_EXPR:
        case VITTE_AST_NODE_BLOCK_EXPR:
        case VITTE_AST_NODE_BREAK_STMT:
        case VITTE_AST_NODE_CONTINUE_STMT:
        case VITTE_AST_NODE_ASM_STMT:
        case VITTE_AST_NODE_ASSERT_STMT:
        case VITTE_AST_NODE_EXPR_STMT:
        case VITTE_AST_NODE_TYPE_EXPR:
        case VITTE_AST_NODE_POINTER_TYPE:
        case VITTE_AST_NODE_REFERENCE_TYPE:
        case VITTE_AST_NODE_ARRAY_TYPE:
        case VITTE_AST_NODE_INTEGER_LITERAL:
        case VITTE_AST_NODE_FLOAT_LITERAL:
        case VITTE_AST_NODE_STRING_LITERAL:
        case VITTE_AST_NODE_CHARACTER_LITERAL:
        case VITTE_AST_NODE_BOOL_LITERAL:
        case VITTE_AST_NODE_NULL_LITERAL:
        case VITTE_AST_NODE_SELF_EXPR:
        case VITTE_AST_NODE_GROUP_EXPR:
        case VITTE_AST_NODE_ARRAY_EXPR:
        case VITTE_AST_NODE_FORM_EXPR:
        case VITTE_AST_NODE_FIELD_INIT:
        case VITTE_AST_NODE_UNARY_EXPR:
        case VITTE_AST_NODE_BINARY_EXPR:
        case VITTE_AST_NODE_CAST_EXPR:
        case VITTE_AST_NODE_ASSIGN_EXPR:
        case VITTE_AST_NODE_CALL_EXPR:
        case VITTE_AST_NODE_INDEX_EXPR:
        case VITTE_AST_NODE_MEMBER_EXPR:
        case VITTE_AST_NODE_AWAIT_EXPR:
        case VITTE_AST_NODE_TRY_EXPR:
        case VITTE_AST_NODE_COUNT:
            return VITTE_SCOPE_KIND_INVALID;
    }

    return VITTE_SCOPE_KIND_INVALID;
}

/* ========================================================================= */
/* Declaration classification                                                */
/* ========================================================================= */

static vitte_symbol_kind_t
vitte_sema_symbol_kind_for_node(
    vitte_ast_node_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_NODE_SPACE_DECL:
            return VITTE_SYMBOL_KIND_SPACE;

        case VITTE_AST_NODE_CONST_DECL:
            return VITTE_SYMBOL_KIND_CONST;

        case VITTE_AST_NODE_STATIC_DECL:
            return VITTE_SYMBOL_KIND_STATIC;

        case VITTE_AST_NODE_TYPE_DECL:
            return VITTE_SYMBOL_KIND_TYPE;

        case VITTE_AST_NODE_OPAQUE_DECL:
            return VITTE_SYMBOL_KIND_OPAQUE;

        case VITTE_AST_NODE_FORM_DECL:
            return VITTE_SYMBOL_KIND_FORM;

        case VITTE_AST_NODE_FIELD_DECL:
            return VITTE_SYMBOL_KIND_FIELD;

        case VITTE_AST_NODE_PICK_DECL:
            return VITTE_SYMBOL_KIND_PICK;

        case VITTE_AST_NODE_VARIANT_DECL:
            return VITTE_SYMBOL_KIND_VARIANT;

        case VITTE_AST_NODE_TRAIT_DECL:
            return VITTE_SYMBOL_KIND_TRAIT;

        case VITTE_AST_NODE_IMPL_DECL:
            return VITTE_SYMBOL_KIND_IMPL;

        case VITTE_AST_NODE_PROC_DECL:
        case VITTE_AST_NODE_EXTERN_PROC_DECL:
            return VITTE_SYMBOL_KIND_PROC;

        case VITTE_AST_NODE_PARAMETER:
            return VITTE_SYMBOL_KIND_PARAMETER;

        case VITTE_AST_NODE_GENERIC_PARAM:
            return VITTE_SYMBOL_KIND_GENERIC_PARAMETER;

        case VITTE_AST_NODE_LET_STMT:
            return VITTE_SYMBOL_KIND_LOCAL;

        case VITTE_AST_NODE_MACRO_DECL:
            return VITTE_SYMBOL_KIND_MACRO;

        case VITTE_AST_NODE_TEST_DECL:
            return VITTE_SYMBOL_KIND_TEST;

        case VITTE_AST_NODE_USE_DECL:
            return VITTE_SYMBOL_KIND_IMPORT;

        case VITTE_AST_NODE_INVALID:
        case VITTE_AST_NODE_TRANSLATION_UNIT:
        case VITTE_AST_NODE_IDENTIFIER:
        case VITTE_AST_NODE_PATH:
        case VITTE_AST_NODE_WHERE_CLAUSE:
        case VITTE_AST_NODE_WHERE_PREDICATE:
        case VITTE_AST_NODE_BLOCK:
        case VITTE_AST_NODE_RETURN_STMT:
        case VITTE_AST_NODE_DEFER_STMT:
        case VITTE_AST_NODE_IF_STMT:
        case VITTE_AST_NODE_IF_EXPR:
        case VITTE_AST_NODE_BLOCK_EXPR:
        case VITTE_AST_NODE_WHILE_STMT:
        case VITTE_AST_NODE_LOOP_STMT:
        case VITTE_AST_NODE_FOR_STMT:
        case VITTE_AST_NODE_BREAK_STMT:
        case VITTE_AST_NODE_CONTINUE_STMT:
        case VITTE_AST_NODE_MATCH_STMT:
        case VITTE_AST_NODE_MATCH_ARM:
        case VITTE_AST_NODE_UNSAFE_BLOCK:
        case VITTE_AST_NODE_ASM_STMT:
        case VITTE_AST_NODE_ASSERT_STMT:
        case VITTE_AST_NODE_EXPR_STMT:
        case VITTE_AST_NODE_TYPE_EXPR:
        case VITTE_AST_NODE_POINTER_TYPE:
        case VITTE_AST_NODE_REFERENCE_TYPE:
        case VITTE_AST_NODE_ARRAY_TYPE:
        case VITTE_AST_NODE_INTEGER_LITERAL:
        case VITTE_AST_NODE_FLOAT_LITERAL:
        case VITTE_AST_NODE_STRING_LITERAL:
        case VITTE_AST_NODE_CHARACTER_LITERAL:
        case VITTE_AST_NODE_BOOL_LITERAL:
        case VITTE_AST_NODE_NULL_LITERAL:
        case VITTE_AST_NODE_SELF_EXPR:
        case VITTE_AST_NODE_GROUP_EXPR:
        case VITTE_AST_NODE_ARRAY_EXPR:
        case VITTE_AST_NODE_FORM_EXPR:
        case VITTE_AST_NODE_FIELD_INIT:
        case VITTE_AST_NODE_UNARY_EXPR:
        case VITTE_AST_NODE_BINARY_EXPR:
        case VITTE_AST_NODE_CAST_EXPR:
        case VITTE_AST_NODE_ASSIGN_EXPR:
        case VITTE_AST_NODE_CALL_EXPR:
        case VITTE_AST_NODE_INDEX_EXPR:
        case VITTE_AST_NODE_MEMBER_EXPR:
        case VITTE_AST_NODE_AWAIT_EXPR:
        case VITTE_AST_NODE_TRY_EXPR:
        case VITTE_AST_NODE_COUNT:
            return VITTE_SYMBOL_KIND_INVALID;
    }

    return VITTE_SYMBOL_KIND_INVALID;
}

static uint32_t
vitte_sema_symbol_flags_from_node(
    const vitte_ast_node_t *node)
{
    uint32_t flags;

    flags = VITTE_SYMBOL_FLAG_NONE;

    if (node == NULL) {
        return flags;
    }

    if ((node->flags &
         VITTE_AST_FLAG_MUTABLE) != 0u) {
        flags |=
            VITTE_SYMBOL_FLAG_MUTABLE;
    }

    if ((node->flags &
         VITTE_AST_FLAG_EXTERN) != 0u) {
        flags |=
            VITTE_SYMBOL_FLAG_EXTERN;
    }

    if ((node->flags &
         VITTE_AST_FLAG_ASYNC) != 0u) {
        flags |=
            VITTE_SYMBOL_FLAG_ASYNC;
    }

    if ((node->flags &
         VITTE_AST_FLAG_UNSAFE) != 0u) {
        flags |=
            VITTE_SYMBOL_FLAG_UNSAFE;
    }

    if ((node->flags &
         VITTE_AST_FLAG_COMPTIME) != 0u) {
        flags |=
            VITTE_SYMBOL_FLAG_COMPTIME;
    }

    if (node->kind ==
        VITTE_AST_NODE_PARAMETER) {
        flags |=
            VITTE_SYMBOL_FLAG_PARAMETER;
    }

    if (node->kind ==
        VITTE_AST_NODE_GENERIC_PARAM) {
        flags |=
            VITTE_SYMBOL_FLAG_GENERIC;
    }

    if (node->kind ==
        VITTE_AST_NODE_USE_DECL) {
        flags |=
            VITTE_SYMBOL_FLAG_IMPORTED;
    }

    return flags;
}

/* ========================================================================= */
/* Declaration name extraction                                               */
/* ========================================================================= */

static const vitte_ast_node_t *
vitte_sema_find_identifier_child(
    const vitte_sema_t *context,
    const vitte_ast_node_t *node)
{
    size_t index;

    if (context == NULL ||
        node == NULL) {
        return NULL;
    }

    for (index = 0u;
         index < node->child_count;
         ++index) {
        const vitte_ast_node_t *child;

        child =
            vitte_sema_node(
                context,
                node->children[index]);

        if (child != NULL &&
            child->kind ==
                VITTE_AST_NODE_IDENTIFIER) {
            return child;
        }
    }

    return NULL;
}

static bool
vitte_sema_declaration_name(
    const vitte_sema_t *context,
    const vitte_ast_node_t *node,
    const char **name,
    size_t *name_length)
{
    const vitte_ast_node_t *identifier;
    const vitte_token_t *token;

    if (context == NULL ||
        node == NULL ||
        name == NULL ||
        name_length == NULL) {
        return false;
    }

    /*
     * Some parser nodes carry their declaration token directly.
     */
    token =
        vitte_sema_node_token(
            context,
            node);

    if (vitte_sema_token_name(
            token,
            name,
            name_length)) {
        return true;
    }

    if (node->kind == VITTE_AST_NODE_SPACE_DECL &&
        node->child_count != 0u) {
        const vitte_ast_node_t *path;
        const vitte_ast_node_t *segment;

        path =
            vitte_sema_node(
                context,
                node->children[0]);
        segment =
            path != NULL &&
                    path->kind == VITTE_AST_NODE_PATH &&
                    path->child_count != 0u
                ? vitte_sema_node(
                      context,
                      path->children[path->child_count - 1u])
                : NULL;
        token =
            vitte_sema_node_token(
                context,
                segment);
        return vitte_sema_token_name(
            token,
            name,
            name_length);
    }

    /*
     * Otherwise use the first identifier child.
     */
    identifier =
        vitte_sema_find_identifier_child(
            context,
            node);

    if (identifier == NULL) {
        return false;
    }

    token =
        vitte_sema_node_token(
            context,
            identifier);

    return vitte_sema_token_name(
        token,
        name,
        name_length);
}

static bool
vitte_sema_is_explicitly_exported(
    const vitte_sema_t *context,
    const char *name,
    size_t name_length,
    vitte_scope_id_t scope_id)
{
    vitte_ast_node_id_t node_id;
    size_t node_count;

    if (context == NULL ||
        name == NULL ||
        name_length == 0u) {
        return false;
    }

    node_count =
        vitte_parser_node_count(
            context->parser);
    for (node_id = 1u;
         node_id <= (vitte_ast_node_id_t)node_count;
         ++node_id) {
        const vitte_ast_node_t *export_node;
        const vitte_sema_node_info_t *export_info;
        size_t item_index;

        export_node =
            vitte_sema_node(
                context,
                node_id);
        if (export_node == NULL ||
            export_node->kind !=
                VITTE_AST_NODE_USE_DECL ||
            (export_node->flags &
             VITTE_AST_FLAG_PUBLIC) == 0u) {
            continue;
        }

        export_info =
            vitte_sema_node_info_private(
                context,
                node_id);
        if (export_info == NULL ||
            export_info->scope_id != scope_id) {
            continue;
        }

        if (export_node->child_count == 0u) {
            return true;
        }

        for (item_index = 0u;
             item_index < export_node->child_count;
             ++item_index) {
            const vitte_ast_node_t *item;
            const vitte_ast_node_t *segment;
            const char *export_name;
            size_t export_name_length;

            item =
                vitte_sema_node(
                    context,
                    export_node->children[item_index]);
            if (item == NULL) {
                continue;
            }

            segment = item;
            if (item->kind == VITTE_AST_NODE_PATH) {
                if (item->child_count == 0u) {
                    continue;
                }
                segment =
                    vitte_sema_node(
                        context,
                        item->children[
                            item->child_count - 1u]);
            }

            if (segment == NULL ||
                !vitte_sema_token_name(
                    vitte_sema_node_token(
                        context,
                        segment),
                    &export_name,
                    &export_name_length)) {
                continue;
            }

            if (export_name_length == name_length &&
                memcmp(export_name, name, name_length) == 0) {
                return true;
            }
        }
    }

    return false;
}

/* ========================================================================= */
/* Forward declarations for recursive analysis                               */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_node(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id,
    vitte_scope_id_t scope_id);

static vitte_sema_type_id_t
vitte_sema_analyze_expression(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id,
    vitte_scope_id_t scope_id);

static vitte_sema_type_id_t
vitte_sema_infer_declaration_type(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id);

static bool
vitte_sema_resolve_qualified_import_type(
    vitte_sema_t *context,
    const vitte_ast_node_t *path,
    vitte_scope_id_t requester_scope,
    vitte_symbol_id_t *symbol_id);

static vitte_sema_type_id_t
vitte_sema_analyze_member(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id);

/* ========================================================================= */
/* Recursion guard                                                           */
/* ========================================================================= */

static bool
vitte_sema_enter(
    vitte_sema_t *context)
{
    if (context->recursion_depth >=
        context->max_recursion_depth) {
        context->last_error =
            VITTE_SEMA_ERROR_RECURSION_LIMIT;

        return false;
    }

    ++context->recursion_depth;

    if ((uint64_t)context->recursion_depth >
        context->stats.max_recursion_depth) {
        context->stats.max_recursion_depth =
            (uint64_t)context->recursion_depth;
    }

    return true;
}

static void
vitte_sema_leave(
    vitte_sema_t *context)
{
    if (context != NULL &&
        context->recursion_depth != 0u) {
        --context->recursion_depth;
    }
}

/* ========================================================================= */
/* Type syntax analysis                                                      */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_type_node(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id,
    vitte_scope_id_t scope_id)
{
    const vitte_ast_node_t *node;
    vitte_sema_node_info_t *info;

    node =
        vitte_sema_node(
            context,
            node_id);

    if (node == NULL) {
        return context->type_error;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node_id);

    switch (node->kind) {
        case VITTE_AST_NODE_POINTER_TYPE:
        {
            vitte_sema_type_id_t element;

            if (node->child_count == 0u) {
                return context->type_error;
            }

            element =
                vitte_sema_analyze_type_node(
                    context,
                    node->children[0],
                    scope_id);

            if (element ==
                context->type_error) {
                return element;
            }

            if (info != NULL) {
                info->type_id =
                    vitte_sema_create_type_private(
                        context,
                        VITTE_SEMA_TYPE_POINTER,
                        element,
                        0u,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        (node->flags &
                         VITTE_AST_FLAG_MUTABLE) != 0u);
            }

            return info != NULL
                ? info->type_id
                : context->type_error;
        }

        case VITTE_AST_NODE_REFERENCE_TYPE:
        {
            vitte_sema_type_id_t element;

            if (node->child_count == 0u) {
                return context->type_error;
            }

            element =
                vitte_sema_analyze_type_node(
                    context,
                    node->children[0],
                    scope_id);

            if (element ==
                context->type_error) {
                return element;
            }

            if (info != NULL) {
                info->type_id =
                    vitte_sema_create_type_private(
                        context,
                        VITTE_SEMA_TYPE_REFERENCE,
                        element,
                        0u,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        (node->flags &
                         VITTE_AST_FLAG_MUTABLE) != 0u);
            }

            return info != NULL
                ? info->type_id
                : context->type_error;
        }

        case VITTE_AST_NODE_ARRAY_TYPE:
        {
            vitte_sema_type_id_t element;
            size_t array_length;

            if (node->child_count == 0u) {
                return context->type_error;
            }

            array_length = 0u;
            if (node->child_count > 1u) {
                uint64_t parsed_length;
                const vitte_ast_node_t *length_node;

                length_node = vitte_sema_node(
                    context,
                    node->children[0]);
                if (vitte_sema_parse_unsigned_integer_literal(
                        context,
                        length_node,
                        &parsed_length) &&
                    parsed_length <= (uint64_t)SIZE_MAX) {
                    array_length = (size_t)parsed_length;
                }
            }

            element =
                vitte_sema_analyze_type_node(
                    context,
                    node->children[
                        node->child_count - 1u],
                    scope_id);

            if (element ==
                context->type_error) {
                return element;
            }

            if (info != NULL) {
                info->type_id =
                    vitte_sema_create_type_private(
                        context,
                        VITTE_SEMA_TYPE_ARRAY,
                        element,
                        array_length,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        false);
            }

            return info != NULL
                ? info->type_id
                : context->type_error;
        }

        case VITTE_AST_NODE_IDENTIFIER:
        case VITTE_AST_NODE_TYPE_EXPR:
        case VITTE_AST_NODE_PATH:
        {
            const vitte_token_t *token;
            const char *name;
            size_t name_length;
            vitte_symbol_id_t symbol_id;
            const vitte_symbol_t *symbol;
            vitte_sema_type_id_t type_id;

            token =
                vitte_sema_node_token(
                    context,
                    node);

            if (node->kind ==
                    VITTE_AST_NODE_PATH &&
                node->child_count > 1u &&
                vitte_sema_resolve_qualified_import_type(
                    context,
                    node,
                    scope_id,
                    &symbol_id)) {
                symbol =
                    vitte_scope_get_symbol(
                        &context->scopes,
                        symbol_id);

                if (symbol == NULL ||
                    !vitte_symbol_kind_is_type(
                        symbol->kind)) {
                    (void)vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE,
                        node->span,
                        node_id,
                        symbol_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID);
                    type_id = context->type_error;
                } else {
                    type_id =
                        vitte_sema_type_for_symbol(
                            context,
                            symbol_id);
                    if (type_id ==
                        context->type_error) {
                        type_id =
                            vitte_sema_create_type_private(
                                context,
                                VITTE_SEMA_TYPE_NAMED,
                                VITTE_SEMA_INVALID_TYPE_ID,
                                0u,
                                symbol_id,
                                false);
                    }
                }

                if (info != NULL) {
                    info->type_id = type_id;
                    info->symbol_id = symbol_id;
                    info->scope_id = scope_id;
                    info->flags |=
                        VITTE_SEMA_NODE_FLAG_RESOLVED;
                }

                return type_id;
            }

            if (!vitte_sema_token_name(
                    token,
                    &name,
                    &name_length)) {
                if ((node->kind ==
                         VITTE_AST_NODE_TYPE_EXPR ||
                     node->kind ==
                         VITTE_AST_NODE_PATH) &&
                    node->child_count == 1u) {
                    type_id =
                        vitte_sema_analyze_type_node(
                            context,
                            node->children[0],
                            scope_id);

                    if (info != NULL) {
                        info->type_id = type_id;
                    }

                    return type_id;
                }

                return context->type_error;
            }

            /*
             * Builtin primitive spellings.
             */
#define VITTE_SEMA_MATCH_BUILTIN(text_) \
    (name_length == sizeof(text_) - 1u && \
     memcmp(name, text_, sizeof(text_) - 1u) == 0)

            if (VITTE_SEMA_MATCH_BUILTIN("bool")) {
                type_id = context->type_bool;
            } else if (VITTE_SEMA_MATCH_BUILTIN("i64") ||
                       VITTE_SEMA_MATCH_BUILTIN("int")) {
                type_id = context->type_i64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("u64")) {
                type_id = context->type_u64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("usize")) {
                type_id = context->type_u64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("u8")) {
                type_id = context->type_u8;
            } else if (VITTE_SEMA_MATCH_BUILTIN("f64") ||
                       VITTE_SEMA_MATCH_BUILTIN("float")) {
                type_id = context->type_f64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("char")) {
                type_id = context->type_char;
            } else if (VITTE_SEMA_MATCH_BUILTIN("string")) {
                type_id = context->type_string;
            } else if (VITTE_SEMA_MATCH_BUILTIN("void")) {
                type_id = context->type_void;
            } else {
                symbol_id =
                    vitte_scope_lookup_n(
                        &context->scopes,
                        scope_id,
                        name,
                        name_length);

                if (symbol_id ==
                    VITTE_SCOPE_INVALID_SYMBOL_ID) {
                    (void)vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_UNKNOWN_TYPE,
                        node->span,
                        node_id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID);

                    type_id =
                        context->type_error;
                } else {
                    symbol =
                        vitte_scope_get_symbol(
                            &context->scopes,
                            symbol_id);

                    if (symbol == NULL ||
                        !vitte_symbol_kind_is_type(
                            symbol->kind)) {
                        (void)vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE,
                            node->span,
                            node_id,
                            symbol_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID);

                        type_id =
                            context->type_error;
                    } else {
                        type_id =
                            vitte_sema_type_for_symbol(
                                context,
                                symbol_id);
                        if (type_id ==
                            context->type_error) {
                            type_id =
                                vitte_sema_create_type_private(
                                    context,
                                    VITTE_SEMA_TYPE_NAMED,
                                    VITTE_SEMA_INVALID_TYPE_ID,
                                    0u,
                                    symbol_id,
                                    false);
                        }
                    }
                }
            }

#undef VITTE_SEMA_MATCH_BUILTIN

            if (info != NULL) {
                info->type_id = type_id;
            }

            return type_id;
        }

        default:
            break;
    }

    return vitte_sema_analyze_expression(
        context,
        node_id,
        scope_id);
}

/* ========================================================================= */
/* Identifier resolution                                                     */
/* ========================================================================= */

static bool
vitte_sema_is_private_glob_import_name(
    vitte_sema_t *context,
    vitte_scope_id_t requester_scope,
    const char *name,
    size_t name_length);

typedef enum vitte_sema_builtin_kind {
    VITTE_SEMA_BUILTIN_NONE = 0,
    VITTE_SEMA_BUILTIN_LEN,
    VITTE_SEMA_BUILTIN_TRIM,
    VITTE_SEMA_BUILTIN_STARTS_WITH,
    VITTE_SEMA_BUILTIN_ENDS_WITH
} vitte_sema_builtin_kind_t;

static vitte_sema_builtin_kind_t
vitte_sema_builtin_kind_from_name(
    const char *name,
    size_t name_length)
{
    if (name == NULL) {
        return VITTE_SEMA_BUILTIN_NONE;
    }
    if (name_length == 3u && memcmp(name, "len", 3u) == 0) {
        return VITTE_SEMA_BUILTIN_LEN;
    }
    if (name_length == 4u && memcmp(name, "trim", 4u) == 0) {
        return VITTE_SEMA_BUILTIN_TRIM;
    }
    if (name_length == 11u &&
        memcmp(name, "starts_with", 11u) == 0) {
        return VITTE_SEMA_BUILTIN_STARTS_WITH;
    }
    if (name_length == 9u &&
        memcmp(name, "ends_with", 9u) == 0) {
        return VITTE_SEMA_BUILTIN_ENDS_WITH;
    }
    return VITTE_SEMA_BUILTIN_NONE;
}

static vitte_sema_builtin_kind_t
vitte_sema_builtin_kind_from_node(
    const vitte_sema_t *context,
    const vitte_ast_node_t *node)
{
    const vitte_ast_node_t *name_node;
    const vitte_token_t *token;

    if (context == NULL || node == NULL) {
        return VITTE_SEMA_BUILTIN_NONE;
    }
    name_node = node;
    if ((node->kind == VITTE_AST_NODE_PATH ||
         node->kind == VITTE_AST_NODE_TYPE_EXPR) &&
        node->child_count != 0u) {
        name_node = vitte_sema_node(
            context,
            node->children[node->child_count - 1u]);
    }
    token = vitte_sema_node_token(context, name_node);
    if (token == NULL || token->kind != VITTE_TOKEN_IDENTIFIER) {
        return VITTE_SEMA_BUILTIN_NONE;
    }
    return vitte_sema_builtin_kind_from_name(
        token->lexeme,
        token->length);
}

static vitte_sema_type_id_t
vitte_sema_analyze_identifier(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    const vitte_token_t *token;
    const char *name;
    size_t name_length;
    vitte_symbol_id_t symbol_id;
    vitte_sema_node_info_t *info;
    vitte_sema_type_id_t type_id;

    token =
        vitte_sema_node_token(
            context,
            node);

    if (!vitte_sema_token_name(
            token,
            &name,
            &name_length)) {
        return context->type_error;
    }

    symbol_id =
        vitte_scope_lookup_n(
            &context->scopes,
            scope_id,
            name,
            name_length);

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (symbol_id ==
        VITTE_SCOPE_INVALID_SYMBOL_ID) {
        vitte_sema_diagnostic_code_t diagnostic_code;

        if (vitte_sema_builtin_kind_from_name(
                name,
                name_length) != VITTE_SEMA_BUILTIN_NONE) {
            vitte_sema_builtin_kind_t builtin_kind;
            vitte_sema_type_id_t function_type;
            vitte_sema_type_t *function;

            builtin_kind = vitte_sema_builtin_kind_from_name(
                name,
                name_length);

            function_type =
                vitte_sema_create_type_private(
                    context,
                    VITTE_SEMA_TYPE_FUNCTION,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    0u,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    false);
            function =
                vitte_sema_type_mut_private(
                    context,
                    function_type);
            if (function == NULL) {
                return context->type_error;
            }
            function->return_type =
                builtin_kind == VITTE_SEMA_BUILTIN_TRIM
                    ? context->type_string
                    : (builtin_kind == VITTE_SEMA_BUILTIN_STARTS_WITH ||
                       builtin_kind == VITTE_SEMA_BUILTIN_ENDS_WITH)
                        ? context->type_bool
                        : context->type_i64;

            if (info != NULL) {
                info->type_id = function_type;
                info->scope_id = scope_id;
                info->flags |=
                    VITTE_SEMA_NODE_FLAG_RESOLVED |
                    VITTE_SEMA_NODE_FLAG_BUILTIN;
            }
            return function_type;
        }

        diagnostic_code =
            vitte_sema_is_private_glob_import_name(
                context,
                scope_id,
                name,
                name_length)
                ? VITTE_SEMA_DIAGNOSTIC_PRIVATE_SYMBOL
                : VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME;

        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            diagnostic_code,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID);

        if (info != NULL) {
            info->type_id =
                context->type_error;
        }

        context->stats.unresolved_names =
            vitte_sema_u64_add_sat(
                context->stats.unresolved_names,
                UINT64_C(1));

        return context->type_error;
    }

    type_id =
        vitte_sema_type_for_symbol(
            context,
            symbol_id);

    if (info != NULL) {
        info->symbol_id = symbol_id;
        info->type_id = type_id;
        info->scope_id = scope_id;
        info->flags |=
            VITTE_SEMA_NODE_FLAG_RESOLVED;
    }

    context->stats.names_resolved =
        vitte_sema_u64_add_sat(
            context->stats.names_resolved,
            UINT64_C(1));

    return type_id;
}

/* ========================================================================= */
/* Literal analysis                                                          */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_literal(
    vitte_sema_t *context,
    const vitte_ast_node_t *node)
{
    vitte_sema_type_id_t type_id;
    vitte_sema_node_info_t *info;

    switch (node->kind) {
        case VITTE_AST_NODE_INTEGER_LITERAL:
            type_id = context->type_i64;
            break;

        case VITTE_AST_NODE_FLOAT_LITERAL:
            type_id = context->type_f64;
            break;

        case VITTE_AST_NODE_STRING_LITERAL:
            type_id = context->type_string;
            break;

        case VITTE_AST_NODE_CHARACTER_LITERAL:
            type_id = context->type_char;
            break;

        case VITTE_AST_NODE_BOOL_LITERAL:
            type_id = context->type_bool;
            break;

        case VITTE_AST_NODE_NULL_LITERAL:
            type_id = context->type_null;
            break;

        default:
            return context->type_error;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->type_id = type_id;
        info->flags |=
            VITTE_SEMA_NODE_FLAG_CONSTANT;

        switch (node->kind) {
            case VITTE_AST_NODE_INTEGER_LITERAL:
                info->constant.kind =
                    VITTE_SEMA_CONSTANT_INTEGER;
                break;

            case VITTE_AST_NODE_FLOAT_LITERAL:
                info->constant.kind =
                    VITTE_SEMA_CONSTANT_FLOAT;
                break;

            case VITTE_AST_NODE_STRING_LITERAL:
                info->constant.kind =
                    VITTE_SEMA_CONSTANT_STRING;
                break;

            case VITTE_AST_NODE_CHARACTER_LITERAL:
                info->constant.kind =
                    VITTE_SEMA_CONSTANT_CHARACTER;
                break;

            case VITTE_AST_NODE_BOOL_LITERAL:
                info->constant.kind =
                    VITTE_SEMA_CONSTANT_BOOLEAN;
                break;

            case VITTE_AST_NODE_NULL_LITERAL:
                info->constant.kind =
                    VITTE_SEMA_CONSTANT_NULL;
                break;

            default:
                break;
        }
    }

    context->stats.expressions_checked =
        vitte_sema_u64_add_sat(
            context->stats.expressions_checked,
            UINT64_C(1));

    return type_id;
}

/* ========================================================================= */
/* Unary expressions                                                         */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_unary(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    vitte_sema_type_id_t operand_type;
    vitte_sema_type_id_t result;
    vitte_sema_node_info_t *info;

    if (node->child_count == 0u) {
        return context->type_error;
    }

    operand_type =
        vitte_sema_analyze_expression(
            context,
            node->children[0],
            scope_id);

    result = context->type_error;

    switch (node->operator_kind) {
        case VITTE_TOKEN_PLUS:
        case VITTE_TOKEN_MINUS:
            operand_type =
                vitte_sema_value_type_private(
                    context,
                    operand_type);
            if (vitte_sema_type_is_numeric_private(
                    context,
                    operand_type)) {
                result = operand_type;
            }
            break;

        case VITTE_TOKEN_TILDE:
            operand_type =
                vitte_sema_value_type_private(
                    context,
                    operand_type);
            if (vitte_sema_type_is_integer_private(
                    context,
                    operand_type)) {
                result = operand_type;
            }
            break;

        case VITTE_TOKEN_BANG:
        case VITTE_TOKEN_KW_NOT:
            operand_type =
                vitte_sema_value_type_private(
                    context,
                    operand_type);
            if (vitte_sema_type_is_bool_private(
                    context,
                    operand_type)) {
                result = context->type_bool;
            }
            break;

        case VITTE_TOKEN_STAR:
        {
            const vitte_sema_type_t *operand;

            operand =
                vitte_sema_type_private(
                    context,
                    operand_type);

            if (operand != NULL &&
                (operand->kind ==
                     VITTE_SEMA_TYPE_POINTER ||
                 operand->kind ==
                     VITTE_SEMA_TYPE_REFERENCE)) {
                result =
                    operand->element_type;
            }

            break;
        }

        case VITTE_TOKEN_AMP:
        case VITTE_TOKEN_KW_REF:
        {
            const vitte_sema_node_info_t *operand_info;
            bool is_mutable;

            operand_info =
                vitte_sema_node_info_private(
                    context,
                    node->children[0]);
            is_mutable =
                operand_info != NULL &&
                (operand_info->flags &
                 VITTE_SEMA_NODE_FLAG_MUTABLE) != 0u;

            result =
                vitte_sema_create_type_private(
                    context,
                    VITTE_SEMA_TYPE_REFERENCE,
                    operand_type,
                    0u,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    is_mutable);

            break;
        }

        default:
            break;
    }

    if (result ==
        context->type_error) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_INVALID_UNARY_OPERAND,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            operand_type);
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->type_id = result;
    }

    return result;
}

/* ========================================================================= */
/* Binary expressions                                                        */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_binary(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    vitte_sema_type_id_t left;
    vitte_sema_type_id_t right;
    vitte_sema_type_id_t result;
    vitte_sema_node_info_t *info;

    if (node->child_count < 2u) {
        return context->type_error;
    }

    left =
        vitte_sema_analyze_expression(
            context,
            node->children[0],
            scope_id);

    right =
        vitte_sema_analyze_expression(
            context,
            node->children[1],
            scope_id);

    left =
        vitte_sema_value_type_private(
            context,
            left);
    right =
        vitte_sema_value_type_private(
            context,
            right);

    if (left == context->type_u64 &&
        right != context->type_u64 &&
        vitte_sema_contextualize_integer_literal(
            context,
            node->children[1],
            context->type_u64)) {
        right = context->type_u64;
    } else if (right == context->type_u64 &&
               left != context->type_u64 &&
               vitte_sema_contextualize_integer_literal(
                   context,
                   node->children[0],
                   context->type_u64)) {
        left = context->type_u64;
    }

    if (vitte_sema_type_is_error_private(
            context,
            left) ||
        vitte_sema_type_is_error_private(
            context,
            right)) {
        return context->type_error;
    }

    result = context->type_error;

    switch (node->operator_kind) {
        case VITTE_TOKEN_PLUS:
        case VITTE_TOKEN_MINUS:
        case VITTE_TOKEN_STAR:
        case VITTE_TOKEN_SLASH:
        case VITTE_TOKEN_PERCENT:
            if (vitte_sema_type_is_numeric_private(
                    context,
                    left) &&
                vitte_sema_type_is_numeric_private(
                    context,
                    right)) {
                result =
                    vitte_sema_common_numeric_type(
                        context,
                        left,
                        right);
            } else if (node->operator_kind ==
                           VITTE_TOKEN_PLUS &&
                       left ==
                           context->type_string &&
                       (right ==
                            context->type_string ||
                        right ==
                            context->type_char)) {
                result =
                    context->type_string;
            }
            break;

        case VITTE_TOKEN_AMP:
        case VITTE_TOKEN_PIPE:
        case VITTE_TOKEN_CARET:
        case VITTE_TOKEN_SHIFT_LEFT:
        case VITTE_TOKEN_SHIFT_RIGHT:
            if (vitte_sema_type_is_integer_private(
                    context,
                    left) &&
                vitte_sema_type_is_integer_private(
                    context,
                    right)) {
                result =
                    vitte_sema_common_numeric_type(
                        context,
                        left,
                        right);
            }
            break;

        case VITTE_TOKEN_LESS:
        case VITTE_TOKEN_LESS_EQUAL:
        case VITTE_TOKEN_GREATER:
        case VITTE_TOKEN_GREATER_EQUAL:
            if (vitte_sema_type_is_numeric_private(
                    context,
                    left) &&
                vitte_sema_type_is_numeric_private(
                    context,
                    right)) {
                result =
                    context->type_bool;
            }
            break;

        case VITTE_TOKEN_EQUAL_EQUAL:
        case VITTE_TOKEN_BANG_EQUAL:
            if (vitte_sema_type_assignable_private(
                    context,
                    left,
                    right) ||
                vitte_sema_type_assignable_private(
                    context,
                    right,
                    left)) {
                result =
                    context->type_bool;
            }
            break;

        case VITTE_TOKEN_KW_AND:
        case VITTE_TOKEN_KW_OR:
        case VITTE_TOKEN_AMP_AMP:
        case VITTE_TOKEN_PIPE_PIPE:
            if (left ==
                    context->type_bool &&
                right ==
                    context->type_bool) {
                result =
                    context->type_bool;
            }
            break;

        case VITTE_TOKEN_QUESTION_QUESTION:
            if (vitte_sema_type_assignable_private(
                    context,
                    left,
                    right)) {
                result = left;
            } else if (vitte_sema_type_assignable_private(
                           context,
                           right,
                           left)) {
                result = right;
            }
            break;

        case VITTE_TOKEN_DOT_DOT:
        case VITTE_TOKEN_DOT_DOT_EQUAL:
            if (vitte_sema_type_is_integer_private(
                    context,
                    left) &&
                vitte_sema_type_is_integer_private(
                    context,
                    right)) {
                result =
                    vitte_sema_create_type_private(
                        context,
                        VITTE_SEMA_TYPE_RANGE,
                        vitte_sema_common_numeric_type(
                            context,
                            left,
                            right),
                        0u,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        false);
            }
            break;

        default:
            break;
    }

    if (result ==
        context->type_error) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_INVALID_BINARY_OPERANDS,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            left,
            right);
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->type_id = result;
    }

    return result;
}

/* ========================================================================= */
/* Assignment                                                                */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_assignment(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    vitte_sema_type_id_t left;
    vitte_sema_type_id_t right;
    vitte_sema_node_info_t *left_info;
    vitte_sema_node_info_t *info;
    const vitte_symbol_t *left_symbol;

    if (node->child_count < 2u) {
        return context->type_error;
    }

    left =
        vitte_sema_analyze_expression(
            context,
            node->children[0],
            scope_id);

    right =
        vitte_sema_analyze_expression(
            context,
            node->children[1],
            scope_id);

    left_info =
        vitte_sema_node_info_mut_private(
            context,
            node->children[0]);

    left_symbol =
        left_info != NULL &&
                left_info->symbol_id !=
                    VITTE_SCOPE_INVALID_SYMBOL_ID
            ? vitte_scope_get_symbol(
                  &context->scopes,
                  left_info->symbol_id)
            : NULL;

    if (left_info == NULL ||
        (left_info->flags &
         VITTE_SEMA_NODE_FLAG_LVALUE) == 0u ||
        (left_symbol != NULL &&
         (left_symbol->kind ==
              VITTE_SYMBOL_KIND_LOCAL ||
          left_symbol->kind ==
              VITTE_SYMBOL_KIND_PARAMETER ||
          left_symbol->kind ==
              VITTE_SYMBOL_KIND_STATIC) &&
         (left_symbol->flags &
          VITTE_SYMBOL_FLAG_MUTABLE) == 0u)) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_NOT_ASSIGNABLE,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            left,
            right);

        return context->type_error;
    }

    if (!vitte_sema_type_assignable_private(
            context,
            left,
            right) &&
        !vitte_sema_contextualize_integer_literal(
            context,
            node->children[1],
            left)) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            left,
            right);

        return context->type_error;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->type_id = left;
    }

    return left;
}

/* ========================================================================= */
/* Calls                                                                     */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_call(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    vitte_sema_type_id_t callee_type;
    const vitte_sema_node_info_t *callee_info;
    const vitte_sema_type_t *type;
    size_t index;
    vitte_sema_type_id_t result;
    vitte_sema_node_info_t *info;

    if (node->child_count == 0u) {
        return context->type_error;
    }

    callee_type =
        vitte_sema_analyze_expression(
            context,
            node->children[0],
            scope_id);
    callee_info =
        vitte_sema_node_info_private(
            context,
            node->children[0]);

    for (index = 1u;
         index < node->child_count;
         ++index) {
        (void)vitte_sema_analyze_expression(
            context,
            node->children[index],
            scope_id);
    }

    type =
        vitte_sema_type_private(
            context,
            callee_type);

    if (callee_type == context->type_error) {
        result = context->type_error;
    } else if (type == NULL ||
        type->kind !=
            VITTE_SEMA_TYPE_FUNCTION) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_NOT_CALLABLE,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            callee_type);

        result =
            context->type_error;
    } else if (callee_info != NULL &&
               (callee_info->flags &
                VITTE_SEMA_NODE_FLAG_BUILTIN) != 0u) {
        vitte_sema_builtin_kind_t builtin_kind;
        size_t expected_arguments;

        builtin_kind = vitte_sema_builtin_kind_from_node(
            context,
            vitte_sema_node(context, node->children[0]));
        expected_arguments =
            builtin_kind == VITTE_SEMA_BUILTIN_STARTS_WITH ||
            builtin_kind == VITTE_SEMA_BUILTIN_ENDS_WITH
                ? 2u
                : 1u;
        result = type->return_type;
        if (node->child_count != expected_arguments + 1u) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_COUNT,
                node->span,
                node->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID);
            result = context->type_error;
        } else {
            for (index = 0u;
                 index < expected_arguments;
                 ++index) {
                const vitte_sema_node_info_t *argument_info;
                const vitte_ast_node_t *argument;

                argument_info =
                    vitte_sema_node_info_private(
                        context,
                        node->children[index + 1u]);
                if (argument_info == NULL ||
                    vitte_sema_type_assignable_private(
                        context,
                        context->type_string,
                        argument_info->type_id)) {
                    continue;
                }
                argument =
                    vitte_sema_node(
                        context,
                        node->children[index + 1u]);
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE,
                    argument != NULL
                        ? argument->span
                        : node->span,
                    argument != NULL
                        ? argument->id
                        : node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    context->type_string,
                    argument_info->type_id);
                result = context->type_error;
            }
        }
    } else {
        const vitte_symbol_t *function_symbol;
        const vitte_ast_node_t *function_declaration;
        size_t parameter_count;
        size_t parameter_index;
        size_t argument_index;

        result =
            type->return_type !=
                    VITTE_SEMA_INVALID_TYPE_ID
                ? type->return_type
                : context->type_void;

        function_symbol =
            vitte_scope_get_symbol(
                &context->scopes,
                type->symbol_id);
        function_declaration =
            function_symbol != NULL
                ? (const vitte_ast_node_t *)function_symbol->payload
                : NULL;
        parameter_count = 0u;

        if (function_declaration != NULL) {
            for (parameter_index = 0u;
                 parameter_index <
                     function_declaration->child_count;
                 ++parameter_index) {
                const vitte_ast_node_t *parameter;

                parameter =
                    vitte_sema_node(
                        context,
                        function_declaration->children[
                            parameter_index]);

                if (parameter != NULL &&
                    parameter->kind ==
                        VITTE_AST_NODE_PARAMETER) {
                    ++parameter_count;
                }
            }
        }

        if (parameter_count != node->child_count - 1u) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_COUNT,
                node->span,
                node->id,
                type->symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID);
            result = context->type_error;
        } else if (function_declaration != NULL) {
            parameter_index = 0u;
            argument_index = 1u;

            while (parameter_index <
                       function_declaration->child_count &&
                   argument_index < node->child_count) {
                const vitte_ast_node_t *parameter;
                const vitte_sema_node_info_t *parameter_info;
                const vitte_sema_node_info_t *argument_info;

                parameter =
                    vitte_sema_node(
                        context,
                        function_declaration->children[
                            parameter_index]);

                if (parameter == NULL ||
                    parameter->kind !=
                        VITTE_AST_NODE_PARAMETER) {
                    ++parameter_index;
                    continue;
                }

                parameter_info =
                    vitte_sema_node_info_private(
                        context,
                        parameter->id);
                argument_info =
                    vitte_sema_node_info_private(
                        context,
                        node->children[argument_index]);

                if (parameter_info != NULL &&
                    argument_info != NULL) {
                    vitte_sema_type_id_t argument_type;

                    argument_type =
                        parameter_info->type_id;
                    if (!vitte_sema_type_is_pointer_like_private(
                            context,
                            parameter_info->type_id)) {
                        argument_type =
                            vitte_sema_value_type_private(
                                context,
                                argument_info->type_id);
                    } else {
                        argument_type =
                            argument_info->type_id;
                    }

                    if (!vitte_sema_type_assignable_private(
                            context,
                            parameter_info->type_id,
                            argument_type) &&
                        !vitte_sema_contextualize_integer_literal(
                            context,
                            node->children[argument_index],
                            parameter_info->type_id)) {
                        const vitte_ast_node_t *argument;

                        argument =
                            vitte_sema_node(
                                context,
                                node->children[argument_index]);

                        (void)vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE,
                            argument != NULL
                                ? argument->span
                                : node->span,
                            argument != NULL
                                ? argument->id
                                : node->id,
                            type->symbol_id,
                            parameter_info->type_id,
                            argument_info->type_id);
                        result = context->type_error;
                    }
                }

                ++parameter_index;
                ++argument_index;
            }
        }
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->type_id = result;
    }

    context->stats.calls_checked =
        vitte_sema_u64_add_sat(
            context->stats.calls_checked,
            UINT64_C(1));

    return result;
}

/* ========================================================================= */
/* Index expressions                                                         */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_index(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    vitte_sema_type_id_t object_type;
    vitte_sema_type_id_t index_type;
    const vitte_sema_type_t *object;
    vitte_sema_type_id_t result;
    vitte_sema_node_info_t *info;

    if (node->child_count < 2u) {
        return context->type_error;
    }

    object_type =
        vitte_sema_analyze_expression(
            context,
            node->children[0],
            scope_id);

    index_type =
        vitte_sema_analyze_expression(
            context,
            node->children[1],
            scope_id);

    if (!vitte_sema_type_is_integer_private(
            context,
            index_type)) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_INVALID_INDEX_TYPE,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            context->type_i64,
            index_type);
    }

    object =
        vitte_sema_type_private(
            context,
            vitte_sema_value_type_private(
                context,
                object_type));

    result = context->type_error;

    if (object != NULL) {
        if (object->kind ==
                VITTE_SEMA_TYPE_ARRAY ||
            object->kind ==
                VITTE_SEMA_TYPE_POINTER ||
            object->kind ==
                VITTE_SEMA_TYPE_REFERENCE) {
            result =
                object->element_type;
        } else if (object->kind ==
                   VITTE_SEMA_TYPE_STRING) {
            result =
                context->type_char;
        }
    }

    if (result ==
        context->type_error) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_NOT_INDEXABLE,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            object_type);
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->type_id = result;
        info->flags |=
            VITTE_SEMA_NODE_FLAG_LVALUE;
    }

    return result;
}

/* ========================================================================= */
/* Expression analysis                                                       */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_block_expression(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t parent_scope)
{
    vitte_scope_id_t block_scope;
    size_t index;

    block_scope = vitte_scope_create(
        &context->scopes,
        parent_scope,
        VITTE_SCOPE_KIND_BLOCK,
        vitte_sema_scope_span_from_parser(node->span),
        (void *)node);
    if (block_scope == VITTE_SCOPE_INVALID_SCOPE_ID) {
        return context->type_error;
    }
    if (node->child_count == 0u) {
        return context->type_void;
    }

    for (index = 0u; index + 1u < node->child_count; ++index) {
        (void)vitte_sema_analyze_node(
            context,
            node->children[index],
            block_scope);
    }
    return vitte_sema_analyze_expression(
        context,
        node->children[node->child_count - 1u],
        block_scope);
}

static vitte_sema_type_id_t
vitte_sema_analyze_form_expression(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    const vitte_sema_type_t *form_type;
    const vitte_symbol_t *form_symbol;
    const vitte_ast_node_t *form_declaration;
    vitte_scope_id_t form_scope;
    vitte_sema_type_id_t type_id;
    size_t scope_index;
    size_t index;

    if (node->child_count == 0u) {
        return context->type_error;
    }
    type_id = vitte_sema_analyze_type_node(
        context,
        node->children[0],
        scope_id);
    form_type = vitte_sema_type_private(context, type_id);
    if (form_type == NULL ||
        form_type->kind != VITTE_SEMA_TYPE_NAMED) {
        return context->type_error;
    }
    form_symbol = vitte_scope_get_symbol(
        &context->scopes,
        form_type->symbol_id);
    form_declaration = form_symbol != NULL
        ? (const vitte_ast_node_t *)form_symbol->payload
        : NULL;
    if (form_declaration == NULL ||
        form_declaration->kind != VITTE_AST_NODE_FORM_DECL) {
        return context->type_error;
    }

    form_scope = VITTE_SCOPE_INVALID_SCOPE_ID;
    for (scope_index = 0u;
         scope_index < context->scopes.scope_count;
         ++scope_index) {
        const vitte_scope_entry_t *scope;
        scope = &context->scopes.scopes[scope_index];
        if (scope->kind == VITTE_SCOPE_KIND_FORM &&
            scope->payload == (void *)form_declaration) {
            form_scope = scope->id;
            break;
        }
    }
    if (form_scope == VITTE_SCOPE_INVALID_SCOPE_ID) {
        return context->type_error;
    }

    for (index = 1u; index < node->child_count; ++index) {
        const vitte_ast_node_t *initializer;
        const vitte_ast_node_t *name;
        const vitte_token_t *name_token;
        vitte_symbol_id_t field_symbol_id;
        const vitte_symbol_t *field_symbol;
        const vitte_ast_node_t *field_declaration;
        vitte_sema_type_id_t field_type;
        vitte_sema_type_id_t value_type;
        size_t previous;

        initializer = vitte_sema_node(
            context,
            node->children[index]);
        if (initializer == NULL ||
            initializer->kind != VITTE_AST_NODE_FIELD_INIT ||
            initializer->child_count != 2u) {
            return context->type_error;
        }
        name = vitte_sema_node(
            context,
            initializer->children[0]);
        name_token = vitte_sema_node_token(context, name);
        if (name_token == NULL || name_token->lexeme == NULL) {
            return context->type_error;
        }
        field_symbol_id = vitte_scope_lookup_local_n(
            &context->scopes,
            form_scope,
            name_token->lexeme,
            name_token->length);
        field_symbol = field_symbol_id !=
                VITTE_SCOPE_INVALID_SYMBOL_ID
            ? vitte_scope_get_symbol(
                  &context->scopes,
                  field_symbol_id)
            : NULL;
        field_declaration = field_symbol != NULL
            ? (const vitte_ast_node_t *)field_symbol->payload
            : NULL;
        if (field_declaration == NULL ||
            field_declaration->kind != VITTE_AST_NODE_FIELD_DECL) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_UNKNOWN_MEMBER,
                name->span,
                name->id,
                form_type->symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                type_id);
            continue;
        }
        for (previous = 1u; previous < index; ++previous) {
            const vitte_ast_node_t *prior;
            const vitte_token_t *prior_token;
            prior = vitte_sema_node(
                context,
                node->children[previous]);
            prior = prior != NULL && prior->child_count != 0u
                ? vitte_sema_node(context, prior->children[0])
                : NULL;
            prior_token = vitte_sema_node_token(context, prior);
            if (prior_token != NULL &&
                prior_token->length == name_token->length &&
                memcmp(
                    prior_token->lexeme,
                    name_token->lexeme,
                    name_token->length) == 0) {
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                    name->span,
                    name->id,
                    field_symbol_id,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID);
                break;
            }
        }
        field_type = vitte_sema_analyze_type_node(
            context,
            field_declaration->children[1],
            form_scope);
        value_type = vitte_sema_analyze_expression(
            context,
            initializer->children[1],
            scope_id);
        if (!vitte_sema_type_assignable_private(
                context,
                field_type,
                value_type) &&
            !vitte_sema_contextualize_integer_literal(
                context,
                initializer->children[1],
                field_type)) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                initializer->span,
                initializer->id,
                field_symbol_id,
                field_type,
                value_type);
        }
    }
    return type_id;
}

static vitte_sema_type_id_t
vitte_sema_analyze_expression(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id,
    vitte_scope_id_t scope_id)
{
    const vitte_ast_node_t *node;
    vitte_sema_type_id_t result;
    vitte_sema_node_info_t *info;

    node =
        vitte_sema_node(
            context,
            node_id);

    if (node == NULL) {
        return context->type_error;
    }

    if (!vitte_sema_enter(context)) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_RECURSION_LIMIT,
            node->span,
            node_id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID);

        return context->type_error;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node_id);

    if (info != NULL) {
        info->scope_id = scope_id;
    }

    switch (node->kind) {
        case VITTE_AST_NODE_PATH:
            if (node->child_count == 1u) {
                const vitte_sema_node_info_t *child_info;

                result =
                    vitte_sema_analyze_expression(
                        context,
                        node->children[0],
                        scope_id);

                child_info =
                    vitte_sema_get_node_info(
                        context,
                        node->children[0]);

                if (info != NULL &&
                    child_info != NULL) {
                    info->type_id = child_info->type_id;
                    info->symbol_id = child_info->symbol_id;
                    info->scope_id = child_info->scope_id;
                    info->flags |= child_info->flags;
                }
            } else {
                vitte_scope_id_t path_scope;
                vitte_symbol_id_t path_symbol;
                size_t segment_index;

                path_scope = scope_id;
                path_symbol =
                    VITTE_SCOPE_INVALID_SYMBOL_ID;
                result = context->type_error;

                for (segment_index = 0u;
                     segment_index < node->child_count;
                     ++segment_index) {
                    const vitte_ast_node_t *segment;
                    const vitte_token_t *segment_token;
                    const vitte_symbol_t *symbol;
                    const char *segment_name;
                    size_t segment_name_length;

                    segment =
                        vitte_sema_node(
                            context,
                            node->children[segment_index]);
                    segment_token =
                        vitte_sema_node_token(
                            context,
                            segment);

                    if (!vitte_sema_token_name(
                            segment_token,
                            &segment_name,
                            &segment_name_length)) {
                        break;
                    }

                    path_symbol =
                        vitte_scope_lookup_visible_n(
                            &context->scopes,
                            path_scope,
                            scope_id,
                            segment_name,
                            segment_name_length);
                    if (path_symbol ==
                        VITTE_SCOPE_INVALID_SYMBOL_ID) {
                        if (segment != NULL) {
                            vitte_symbol_id_t hidden_symbol_id;
                            const vitte_symbol_t *hidden_symbol;
                            vitte_sema_diagnostic_code_t diagnostic_code;

                            hidden_symbol_id =
                                vitte_scope_lookup_n(
                                    &context->scopes,
                                    path_scope,
                                    segment_name,
                                    segment_name_length);
                            hidden_symbol =
                                hidden_symbol_id !=
                                        VITTE_SCOPE_INVALID_SYMBOL_ID
                                    ? vitte_scope_get_symbol(
                                          &context->scopes,
                                          hidden_symbol_id)
                                    : NULL;
                            diagnostic_code =
                                vitte_symbol_is_private(hidden_symbol)
                                    ? VITTE_SEMA_DIAGNOSTIC_PRIVATE_SYMBOL
                                    : VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME;

                            (void)vitte_sema_add_diagnostic(
                                context,
                                VITTE_SEMA_DIAGNOSTIC_ERROR,
                                diagnostic_code,
                                segment->span,
                                segment->id,
                                VITTE_SCOPE_INVALID_SYMBOL_ID,
                                VITTE_SEMA_INVALID_TYPE_ID,
                                VITTE_SEMA_INVALID_TYPE_ID);
                        }
                        break;
                    }

                    symbol =
                        vitte_scope_get_symbol(
                            &context->scopes,
                            path_symbol);
                    if (symbol == NULL) {
                        path_symbol =
                            VITTE_SCOPE_INVALID_SYMBOL_ID;
                        break;
                    }

                    if (segment_index + 1u <
                        node->child_count) {
                        const vitte_ast_node_t *declaration;
                        const vitte_sema_node_info_t *decl_info;
                        vitte_scope_id_t child_scope;

                        if (symbol->kind !=
                            VITTE_SYMBOL_KIND_SPACE) {
                            path_symbol =
                                VITTE_SCOPE_INVALID_SYMBOL_ID;
                            break;
                        }

                        declaration =
                            (const vitte_ast_node_t *)
                                symbol->payload;
                        child_scope =
                            VITTE_SCOPE_INVALID_SCOPE_ID;
                        if (vitte_sema_space_scope_private(
                                context,
                                symbol,
                                declaration,
                                &child_scope)) {
                            path_scope = child_scope;
                            continue;
                        }
                        decl_info =
                            declaration != NULL
                                ? vitte_sema_node_info_private(
                                      context,
                                      declaration->id)
                                : NULL;
                        if (decl_info == NULL) {
                            path_symbol =
                                VITTE_SCOPE_INVALID_SYMBOL_ID;
                            break;
                        }
                        path_scope = decl_info->scope_id;
                    } else {
                        result =
                            vitte_sema_type_for_symbol(
                                context,
                                path_symbol);
                        if (info != NULL) {
                            info->symbol_id = path_symbol;
                            info->flags |=
                                VITTE_SEMA_NODE_FLAG_RESOLVED;
                        }
                    }
                }
            }
            break;

        case VITTE_AST_NODE_IDENTIFIER:
            result =
                vitte_sema_analyze_identifier(
                    context,
                    node,
                    scope_id);

            if (info != NULL &&
                info->symbol_id !=
                    VITTE_SCOPE_INVALID_SYMBOL_ID) {
                const vitte_symbol_t *symbol;

                symbol =
                    vitte_scope_get_symbol(
                        &context->scopes,
                        info->symbol_id);

                if (symbol != NULL &&
                    (symbol->kind ==
                         VITTE_SYMBOL_KIND_LOCAL ||
                     symbol->kind ==
                         VITTE_SYMBOL_KIND_PARAMETER ||
                     symbol->kind ==
                         VITTE_SYMBOL_KIND_STATIC ||
                     symbol->kind ==
                         VITTE_SYMBOL_KIND_FIELD)) {
                    info->flags |=
                        VITTE_SEMA_NODE_FLAG_LVALUE;

                    if ((symbol->flags &
                         VITTE_SYMBOL_FLAG_MUTABLE) != 0u) {
                        info->flags |=
                            VITTE_SEMA_NODE_FLAG_MUTABLE;
                    }
                }
            }
            break;

        case VITTE_AST_NODE_INTEGER_LITERAL:
        case VITTE_AST_NODE_FLOAT_LITERAL:
        case VITTE_AST_NODE_STRING_LITERAL:
        case VITTE_AST_NODE_CHARACTER_LITERAL:
        case VITTE_AST_NODE_BOOL_LITERAL:
        case VITTE_AST_NODE_NULL_LITERAL:
            result =
                vitte_sema_analyze_literal(
                    context,
                    node);
            break;

        case VITTE_AST_NODE_GROUP_EXPR:
            if (node->child_count != 0u) {
                result =
                    vitte_sema_analyze_expression(
                        context,
                        node->children[0],
                        scope_id);
            } else {
                result =
                    context->type_error;
            }
            break;

        case VITTE_AST_NODE_IF_EXPR:
        {
            vitte_sema_type_id_t condition_type;
            vitte_sema_type_id_t then_type;
            vitte_sema_type_id_t else_type;
            const vitte_ast_node_t *condition;

            if (node->child_count != 3u) {
                result = context->type_error;
                break;
            }
            condition_type = vitte_sema_analyze_expression(
                context,
                node->children[0],
                scope_id);
            condition = vitte_sema_node(
                context,
                node->children[0]);
            if (condition_type != context->type_bool &&
                condition_type != context->type_error &&
                condition != NULL) {
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_CONDITION_NOT_BOOL,
                    condition->span,
                    condition->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    context->type_bool,
                    condition_type);
            }
            then_type = vitte_sema_analyze_expression(
                context,
                node->children[1],
                scope_id);
            else_type = vitte_sema_analyze_expression(
                context,
                node->children[2],
                scope_id);
            if (vitte_sema_type_assignable_private(
                    context,
                    then_type,
                    else_type)) {
                result = then_type;
            } else if (vitte_sema_type_assignable_private(
                           context,
                           else_type,
                           then_type)) {
                result = else_type;
            } else {
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                    node->span,
                    node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    then_type,
                    else_type);
                result = context->type_error;
            }
            break;
        }

        case VITTE_AST_NODE_BLOCK_EXPR:
            result = vitte_sema_analyze_block_expression(
                context,
                node,
                scope_id);
            break;

        case VITTE_AST_NODE_ARRAY_EXPR:
        {
            size_t child_index;
            vitte_sema_type_id_t element_type;

            element_type =
                VITTE_SEMA_INVALID_TYPE_ID;
            for (child_index = 0u;
                 child_index < node->child_count;
                 ++child_index) {
                vitte_sema_type_id_t item_type;

                item_type = vitte_sema_analyze_expression(
                    context,
                    node->children[child_index],
                    scope_id);
                if (element_type ==
                    VITTE_SEMA_INVALID_TYPE_ID) {
                    element_type = item_type;
                } else if (!vitte_sema_type_assignable_private(
                               context,
                               element_type,
                               item_type)) {
                    const vitte_ast_node_t *item;

                    if (vitte_sema_type_assignable_private(
                            context,
                            item_type,
                            element_type)) {
                        element_type = item_type;
                    } else {
                        item = vitte_sema_node(
                            context,
                            node->children[child_index]);
                        if (item != NULL) {
                            (void)vitte_sema_add_diagnostic(
                                context,
                                VITTE_SEMA_DIAGNOSTIC_ERROR,
                                VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                                item->span,
                                item->id,
                                VITTE_SCOPE_INVALID_SYMBOL_ID,
                                element_type,
                                item_type);
                        }
                        element_type = context->type_error;
                    }
                }
            }
            result = element_type !=
                    VITTE_SEMA_INVALID_TYPE_ID &&
                    element_type != context->type_error
                ? vitte_sema_create_type_private(
                      context,
                      VITTE_SEMA_TYPE_ARRAY,
                      element_type,
                      node->child_count,
                      VITTE_SCOPE_INVALID_SYMBOL_ID,
                      false)
                : context->type_error;
            break;
        }

        case VITTE_AST_NODE_FORM_EXPR:
            result = vitte_sema_analyze_form_expression(
                context,
                node,
                scope_id);
            break;

        case VITTE_AST_NODE_UNARY_EXPR:
            result =
                vitte_sema_analyze_unary(
                    context,
                    node,
                    scope_id);
            break;

        case VITTE_AST_NODE_BINARY_EXPR:
            result =
                vitte_sema_analyze_binary(
                    context,
                    node,
                    scope_id);
            break;

        case VITTE_AST_NODE_CAST_EXPR:
            if (node->child_count == 2u) {
                vitte_sema_type_id_t source_type;
                vitte_sema_type_id_t target_type;

                source_type = vitte_sema_analyze_expression(
                    context,
                    node->children[0],
                    scope_id);
                target_type = vitte_sema_analyze_type_node(
                    context,
                    node->children[1],
                    scope_id);
                if (source_type == context->type_error ||
                    target_type == context->type_error) {
                    result = context->type_error;
                } else if (vitte_sema_type_is_numeric_private(
                               context,
                               source_type) &&
                           vitte_sema_type_is_numeric_private(
                               context,
                               target_type)) {
                    result = target_type;
                } else {
                    (void)vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                        node->span,
                        node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        target_type,
                        source_type);
                    result = context->type_error;
                }
            } else {
                result = context->type_error;
            }
            break;

        case VITTE_AST_NODE_ASSIGN_EXPR:
            result =
                vitte_sema_analyze_assignment(
                    context,
                    node,
                    scope_id);
            break;

        case VITTE_AST_NODE_CALL_EXPR:
            result =
                vitte_sema_analyze_call(
                    context,
                    node,
                    scope_id);
            break;

        case VITTE_AST_NODE_INDEX_EXPR:
            result =
                vitte_sema_analyze_index(
                    context,
                    node,
                    scope_id);
            break;

        case VITTE_AST_NODE_MEMBER_EXPR:
            result =
                vitte_sema_analyze_member(
                    context,
                    node,
                    scope_id);
            break;

        case VITTE_AST_NODE_AWAIT_EXPR:
            if (node->child_count != 0u) {
                result =
                    vitte_sema_analyze_expression(
                        context,
                        node->children[0],
                        scope_id);
            } else {
                result =
                    context->type_error;
            }
            break;

        case VITTE_AST_NODE_TRY_EXPR:
            if (node->child_count != 0u) {
                result =
                    vitte_sema_analyze_expression(
                        context,
                        node->children[0],
                        scope_id);
            } else {
                result =
                    context->type_error;
            }
            break;

        case VITTE_AST_NODE_SELF_EXPR:
            result =
                context->current_self_type !=
                        VITTE_SEMA_INVALID_TYPE_ID
                    ? context->current_self_type
                    : context->type_error;
            break;

        case VITTE_AST_NODE_TYPE_EXPR:
        case VITTE_AST_NODE_POINTER_TYPE:
        case VITTE_AST_NODE_REFERENCE_TYPE:
        case VITTE_AST_NODE_ARRAY_TYPE:
            result =
                vitte_sema_analyze_type_node(
                    context,
                    node_id,
                    scope_id);
            break;

        default:
            result =
                context->type_error;
            break;
    }

    if (info != NULL &&
        info->type_id ==
            VITTE_SEMA_INVALID_TYPE_ID) {
        info->type_id = result;
    }

    context->stats.expressions_checked =
        vitte_sema_u64_add_sat(
            context->stats.expressions_checked,
            UINT64_C(1));

    vitte_sema_leave(context);

    return result;
}

/* ========================================================================= */
/* Declaration registration                                                  */
/* ========================================================================= */

static bool
vitte_sema_scope_is_space_descendant(
    const vitte_sema_t *context,
    vitte_scope_id_t scope_id)
{
    while (context != NULL &&
           scope_id != VITTE_SCOPE_INVALID_SCOPE_ID) {
        const vitte_scope_entry_t *scope;

        scope =
            vitte_scope_get_scope(
                &context->scopes,
                scope_id);
        if (scope == NULL) {
            return false;
        }
        if (scope->kind == VITTE_SCOPE_KIND_SPACE) {
            return true;
        }
        if (scope->parent_id == scope_id) {
            break;
        }
        scope_id = scope->parent_id;
    }

    return false;
}

static vitte_symbol_id_t
vitte_sema_register_declaration(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    vitte_symbol_kind_t kind;
    vitte_symbol_visibility_t visibility;
    const char *name;
    size_t name_length;
    vitte_symbol_id_t symbol_id;
    vitte_sema_node_info_t *info;

    kind =
        vitte_sema_symbol_kind_for_node(
            node->kind);

    if (kind ==
        VITTE_SYMBOL_KIND_INVALID) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (!vitte_sema_declaration_name(
            context,
            node,
            &name,
            &name_length)) {
        /*
         * Some declarations (notably impl) may be anonymous from the
         * lexical namespace perspective.
         */
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    visibility =
        (node->flags &
         VITTE_AST_FLAG_PUBLIC) != 0u
            ? VITTE_SYMBOL_VISIBILITY_PUBLIC
            : VITTE_SYMBOL_VISIBILITY_PRIVATE;
    if (visibility ==
            VITTE_SYMBOL_VISIBILITY_PRIVATE &&
        vitte_sema_is_explicitly_exported(
            context,
            name,
            name_length,
            scope_id)) {
        visibility =
            VITTE_SYMBOL_VISIBILITY_PUBLIC;
    }

    symbol_id =
        vitte_scope_declare(
            &context->scopes,
            scope_id,
            name,
            name_length,
            kind,
            visibility,
            vitte_sema_symbol_flags_from_node(
                node),
            vitte_sema_scope_span_from_parser(
                node->span),
            (void *)node);

    if (symbol_id ==
        VITTE_SCOPE_INVALID_SYMBOL_ID) {
        vitte_scope_error_t scope_error;

        scope_error =
            vitte_scope_last_error(
                &context->scopes);

        if (scope_error ==
            VITTE_SCOPE_ERROR_DUPLICATE_SYMBOL) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                node->span,
                node->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID);

            context->stats.duplicate_declarations =
                vitte_sema_u64_add_sat(
                    context->stats.duplicate_declarations,
                    UINT64_C(1));
        }

        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->symbol_id = symbol_id;
        info->scope_id = scope_id;
        info->flags |=
            VITTE_SEMA_NODE_FLAG_DECLARATION;
    }

    /*
     * Local shadowing is a first-class lexical-scope feature in Vitte.  The
     * resolver already records the hidden symbol and DWARF keeps both live
     * bindings distinct, so warning for every intentional nested `let` only
     * adds noise to valid programs and the positive corpus.  Keep W0030 for
     * declarations with broader/module visibility where the collision is
     * materially harder to spot.
     */
    if (!vitte_sema_scope_is_space_descendant(
            context,
            scope_id) &&
        vitte_sema_symbol_kind_for_node(node->kind) !=
            VITTE_SYMBOL_KIND_LOCAL &&
        vitte_sema_symbol_kind_for_node(node->kind) !=
            VITTE_SYMBOL_KIND_FIELD &&
        vitte_sema_symbol_kind_for_node(node->kind) !=
            VITTE_SYMBOL_KIND_VARIANT &&
        vitte_scope_symbol_shadows(
            &context->scopes,
            symbol_id,
            NULL)) {
        context->stats.shadowing_declarations =
            vitte_sema_u64_add_sat(
                context->stats.shadowing_declarations,
                UINT64_C(1));

        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_WARNING,
            VITTE_SEMA_DIAGNOSTIC_SHADOWED_DECLARATION,
            node->span,
            node->id,
            symbol_id,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID);
    }

    context->stats.declarations =
        vitte_sema_u64_add_sat(
            context->stats.declarations,
            UINT64_C(1));

    return symbol_id;
}

static bool
vitte_sema_space_scope_private(
    const vitte_sema_t *context,
    const vitte_symbol_t *symbol,
    const vitte_ast_node_t *declaration,
    vitte_scope_id_t *scope_id)
{
    const vitte_scope_entry_t *parent;
    size_t index;

    if (context == NULL ||
        symbol == NULL ||
        declaration == NULL ||
        scope_id == NULL) {
        return false;
    }

    parent =
        vitte_scope_get_scope(
            &context->scopes,
            symbol->scope_id);
    if (parent == NULL) {
        return false;
    }

    for (index = 0u;
         index < parent->child_count;
         ++index) {
        const vitte_scope_entry_t *child;

        child =
            vitte_scope_get_scope(
                &context->scopes,
                parent->children[index]);
        if (child != NULL &&
            child->kind == VITTE_SCOPE_KIND_SPACE &&
            child->payload == declaration) {
            *scope_id = child->id;
            return true;
        }
    }

    return false;
}

static bool
vitte_sema_resolve_import_module(
    vitte_sema_t *context,
    const vitte_ast_node_t *path,
    vitte_scope_id_t requester_scope,
    vitte_scope_id_t *module_scope)
{
    vitte_scope_id_t current_scope;
    size_t segment_index;

    if (context == NULL ||
        path == NULL ||
        path->kind != VITTE_AST_NODE_PATH ||
        path->child_count == 0u ||
        module_scope == NULL) {
        return false;
    }

    current_scope = requester_scope;
    for (segment_index = 0u;
         segment_index < path->child_count;
         ++segment_index) {
        const vitte_ast_node_t *segment;
        const char *name;
        size_t name_length;
        const vitte_symbol_t *symbol;
        vitte_symbol_id_t symbol_id;
        const vitte_ast_node_t *declaration;
        const vitte_sema_node_info_t *declaration_info;
        vitte_scope_id_t child_scope;

        segment =
            vitte_sema_node(
                context,
                path->children[segment_index]);
        if (segment == NULL ||
            !vitte_sema_token_name(
                vitte_sema_node_token(
                    context,
                    segment),
                &name,
                &name_length)) {
            return false;
        }

        symbol_id =
            vitte_scope_lookup_visible_n(
                &context->scopes,
                current_scope,
                requester_scope,
                name,
                name_length);
        symbol =
            symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID
                ? vitte_scope_get_symbol(
                      &context->scopes,
                      symbol_id)
                : NULL;
        if (symbol == NULL ||
            symbol->kind != VITTE_SYMBOL_KIND_SPACE) {
            goto unresolved_path;
        }

        declaration =
            (const vitte_ast_node_t *)symbol->payload;
        child_scope =
            VITTE_SCOPE_INVALID_SCOPE_ID;
        if (vitte_sema_space_scope_private(
                context,
                symbol,
                declaration,
                &child_scope)) {
            current_scope = child_scope;
            continue;
        }

        declaration_info =
            declaration != NULL
                ? vitte_sema_node_info_private(
                      context,
                      declaration->id)
                : NULL;
        if (declaration_info == NULL ||
            vitte_sema_node(
                context,
                declaration->id) != declaration) {
            goto unresolved_path;
        }

        current_scope = declaration_info->scope_id;
    }

    *module_scope = current_scope;
    return true;

unresolved_path:
    {
        size_t node_index;

        for (node_index = 0u;
             node_index < context->parser->node_count;
             ++node_index) {
            const vitte_ast_node_t *declaration;
            const vitte_ast_node_t *declared_path;
            const vitte_sema_node_info_t *declaration_info;
            size_t index;
            bool matches;

            declaration =
                vitte_sema_node(
                    context,
                    (vitte_ast_node_id_t)(node_index + 1u));
            if (declaration == NULL ||
                declaration->kind !=
                    VITTE_AST_NODE_SPACE_DECL ||
                declaration->child_count == 0u) {
                continue;
            }

            declared_path =
                vitte_sema_node(
                    context,
                    declaration->children[0]);
            if (declared_path == NULL ||
                declared_path->kind !=
                    VITTE_AST_NODE_PATH ||
                declared_path->child_count !=
                    path->child_count) {
                continue;
            }

            matches = true;
            for (index = 0u;
                 index < path->child_count;
                 ++index) {
                const vitte_ast_node_t *requested_segment;
                const vitte_ast_node_t *declared_segment;
                const char *requested_name;
                const char *declared_name;
                size_t requested_length;
                size_t declared_length;

                requested_segment =
                    vitte_sema_node(
                        context,
                        path->children[index]);
                declared_segment =
                    vitte_sema_node(
                        context,
                        declared_path->children[index]);
                if (!vitte_sema_token_name(
                        vitte_sema_node_token(
                            context,
                            requested_segment),
                        &requested_name,
                        &requested_length) ||
                    !vitte_sema_token_name(
                        vitte_sema_node_token(
                            context,
                            declared_segment),
                        &declared_name,
                        &declared_length) ||
                    requested_length != declared_length ||
                    memcmp(
                        requested_name,
                        declared_name,
                        requested_length) != 0) {
                    matches = false;
                    break;
                }
            }

            if (!matches) {
                continue;
            }

            declaration_info =
                vitte_sema_node_info_private(
                    context,
                    declaration->id);
            if (declaration_info != NULL &&
                declaration_info->scope_id !=
                    VITTE_SCOPE_INVALID_SCOPE_ID) {
                *module_scope = declaration_info->scope_id;
                return true;
            }
        }
    }

    return false;
}

static bool
vitte_sema_is_private_glob_import_name(
    vitte_sema_t *context,
    vitte_scope_id_t requester_scope,
    const char *name,
    size_t name_length)
{
    size_t node_index;

    if (context == NULL ||
        context->parser == NULL ||
        name == NULL ||
        name_length == 0u) {
        return false;
    }

    for (node_index = 0u;
         node_index < context->parser->node_count;
         ++node_index) {
        const vitte_ast_node_t *import_node;
        const vitte_sema_node_info_t *import_info;
        const vitte_ast_node_t *module_path;
        vitte_scope_id_t module_scope;
        vitte_symbol_id_t symbol_id;
        const vitte_symbol_t *symbol;

        import_node =
            vitte_sema_node(
                context,
                (vitte_ast_node_id_t)(node_index + 1u));
        if (import_node == NULL ||
            import_node->kind != VITTE_AST_NODE_USE_DECL ||
            (import_node->flags &
             VITTE_AST_FLAG_GLOB_IMPORT) == 0u ||
            import_node->child_count == 0u) {
            continue;
        }

        import_info =
            vitte_sema_node_info_private(
                context,
                import_node->id);
        if (import_info == NULL ||
            !vitte_scope_is_ancestor(
                &context->scopes,
                import_info->scope_id,
                requester_scope)) {
            continue;
        }

        module_path =
            vitte_sema_node(
                context,
                import_node->children[0]);
        if (!vitte_sema_resolve_import_module(
                context,
                module_path,
                import_info->scope_id,
                &module_scope)) {
            continue;
        }

        symbol_id =
            vitte_scope_lookup_local_n(
                &context->scopes,
                module_scope,
                name,
                name_length);
        symbol =
            symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID
                ? vitte_scope_get_symbol(
                      &context->scopes,
                      symbol_id)
                : NULL;
        if (vitte_symbol_is_private(symbol)) {
            return true;
        }
    }

    return false;
}

static bool
vitte_sema_resolve_import_item(
    vitte_sema_t *context,
    const vitte_ast_node_t *item,
    vitte_scope_id_t module_scope,
    vitte_scope_id_t requester_scope,
    vitte_symbol_id_t *resolved_symbol)
{
    vitte_scope_id_t current_scope;
    size_t segment_index;

    if (context == NULL ||
        item == NULL ||
        resolved_symbol == NULL) {
        return false;
    }

    current_scope = module_scope;
    *resolved_symbol = VITTE_SCOPE_INVALID_SYMBOL_ID;

    if (item->kind == VITTE_AST_NODE_IDENTIFIER) {
        const char *name;
        size_t name_length;

        if (!vitte_sema_token_name(
                vitte_sema_node_token(
                    context,
                    item),
                &name,
                &name_length)) {
            return false;
        }
        *resolved_symbol =
            vitte_scope_lookup_visible_n(
                &context->scopes,
                current_scope,
                requester_scope,
                name,
                name_length);
        return *resolved_symbol !=
            VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    if (item->kind != VITTE_AST_NODE_PATH ||
        item->child_count == 0u) {
        return false;
    }

    for (segment_index = 0u;
         segment_index < item->child_count;
         ++segment_index) {
        const vitte_ast_node_t *segment;
        const char *name;
        size_t name_length;
        vitte_symbol_id_t symbol_id;
        const vitte_symbol_t *symbol;

        segment =
            vitte_sema_node(
                context,
                item->children[segment_index]);
        if (segment == NULL ||
            !vitte_sema_token_name(
                vitte_sema_node_token(
                    context,
                    segment),
                &name,
                &name_length)) {
            return false;
        }

        symbol_id =
            vitte_scope_lookup_visible_n(
                &context->scopes,
                current_scope,
                requester_scope,
                name,
                name_length);
        symbol =
            symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID
                ? vitte_scope_get_symbol(
                      &context->scopes,
                      symbol_id)
                : NULL;
        if (symbol == NULL) {
            return false;
        }

        if (segment_index + 1u == item->child_count) {
            *resolved_symbol = symbol_id;
            return true;
        }

        if (symbol->kind != VITTE_SYMBOL_KIND_SPACE) {
            return false;
        }

        {
            const vitte_ast_node_t *declaration;
            const vitte_sema_node_info_t *declaration_info;

            declaration =
                (const vitte_ast_node_t *)symbol->payload;
            declaration_info =
                declaration != NULL
                    ? vitte_sema_node_info_private(
                          context,
                          declaration->id)
                    : NULL;
            if (declaration_info == NULL) {
                return false;
            }
            current_scope = declaration_info->scope_id;
        }
    }

    return false;
}

static bool
vitte_sema_find_unique_public_type(
    const vitte_sema_t *context,
    const char *name,
    size_t name_length,
    vitte_symbol_id_t *resolved_symbol)
{
    vitte_symbol_id_t match;
    const void *match_payload;
    size_t scope_index;

    if (context == NULL ||
        name == NULL ||
        name_length == 0u ||
        resolved_symbol == NULL) {
        return false;
    }

    match = VITTE_SCOPE_INVALID_SYMBOL_ID;
    match_payload = NULL;

    for (scope_index = 0u;
         scope_index < context->scopes.scope_count;
         ++scope_index) {
        const vitte_scope_entry_t *scope;
        size_t symbol_index;

        scope = &context->scopes.scopes[scope_index];
        for (symbol_index = 0u;
             symbol_index <
                 vitte_scope_entry_symbol_count(scope);
             ++symbol_index) {
            vitte_symbol_id_t candidate_id;
            const vitte_symbol_t *candidate;

            candidate_id =
                vitte_scope_entry_symbol_at(
                    scope,
                    symbol_index);
            candidate =
                vitte_scope_get_symbol(
                    &context->scopes,
                    candidate_id);
            if (candidate == NULL ||
                !vitte_symbol_kind_is_type(
                    candidate->kind) ||
                !vitte_symbol_is_public(candidate) ||
                candidate->name_length != name_length ||
                memcmp(candidate->name, name, name_length) != 0) {
                continue;
            }

            if (match != VITTE_SCOPE_INVALID_SYMBOL_ID &&
                candidate->payload != match_payload) {
                return false;
            }

            match = candidate_id;
            match_payload = candidate->payload;
        }
    }

    if (match == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return false;
    }

    *resolved_symbol = match;
    return true;
}

static bool
vitte_sema_resolve_qualified_import_type(
    vitte_sema_t *context,
    const vitte_ast_node_t *path,
    vitte_scope_id_t requester_scope,
    vitte_symbol_id_t *resolved_symbol)
{
    const vitte_ast_node_t *alias_segment;
    const char *alias_name;
    size_t alias_name_length;
    size_t node_index;

    if (context == NULL ||
        path == NULL ||
        path->kind != VITTE_AST_NODE_PATH ||
        path->child_count < 2u ||
        resolved_symbol == NULL) {
        return false;
    }

    alias_segment =
        vitte_sema_node(
            context,
            path->children[0]);
    if (alias_segment == NULL ||
        !vitte_sema_token_name(
            vitte_sema_node_token(
                context,
                alias_segment),
            &alias_name,
            &alias_name_length)) {
        return false;
    }

    for (node_index = 0u;
         node_index < context->parser->node_count;
         ++node_index) {
        const vitte_ast_node_t *use_node;
        const vitte_sema_node_info_t *use_info;
        const vitte_ast_node_t *alias;
        const char *candidate_name;
        size_t candidate_name_length;
        const vitte_ast_node_t *module_path;
        vitte_scope_id_t module_scope;
        vitte_scope_id_t current_scope;
        size_t segment_index;

        use_node =
            vitte_sema_node(
                context,
                (vitte_ast_node_id_t)(node_index + 1u));
        if (use_node == NULL ||
            use_node->kind != VITTE_AST_NODE_USE_DECL ||
            (use_node->flags &
             VITTE_AST_FLAG_GLOB_IMPORT) != 0u ||
            use_node->child_count != 2u) {
            continue;
        }

        use_info =
            vitte_sema_node_info_private(
                context,
                use_node->id);
        if (use_info == NULL ||
            !vitte_scope_is_ancestor(
                &context->scopes,
                use_info->scope_id,
                requester_scope)) {
            continue;
        }

        alias =
            vitte_sema_node(
                context,
                use_node->children[1]);
        if (alias == NULL ||
            alias->kind != VITTE_AST_NODE_IDENTIFIER ||
            !vitte_sema_token_name(
                vitte_sema_node_token(
                    context,
                    alias),
                &candidate_name,
                &candidate_name_length) ||
            candidate_name_length != alias_name_length ||
            memcmp(
                candidate_name,
                alias_name,
                alias_name_length) != 0) {
            continue;
        }

        module_path =
            vitte_sema_node(
                context,
                use_node->children[0]);
        if (module_path == NULL ||
            !vitte_sema_resolve_import_module(
                context,
                module_path,
                use_info->scope_id,
                &module_scope)) {
            const vitte_ast_node_t *type_segment;
            const char *type_name;
            size_t type_name_length;

            type_segment =
                vitte_sema_node(
                    context,
                    path->children[path->child_count - 1u]);
            if (vitte_sema_token_name(
                    vitte_sema_node_token(
                        context,
                        type_segment),
                    &type_name,
                    &type_name_length) &&
                vitte_sema_find_unique_public_type(
                    context,
                    type_name,
                    type_name_length,
                    resolved_symbol)) {
                return true;
            }

            continue;
        }

        current_scope = module_scope;
        for (segment_index = 1u;
             segment_index < path->child_count;
             ++segment_index) {
            const vitte_ast_node_t *segment;
            const char *name;
            size_t name_length;
            vitte_symbol_id_t symbol_id;
            const vitte_symbol_t *symbol;

            segment =
                vitte_sema_node(
                    context,
                    path->children[segment_index]);
            if (segment == NULL ||
                !vitte_sema_token_name(
                    vitte_sema_node_token(
                        context,
                        segment),
                    &name,
                    &name_length)) {
                return false;
            }

            symbol_id =
                vitte_scope_lookup_visible_n(
                    &context->scopes,
                    current_scope,
                    requester_scope,
                    name,
                    name_length);
            symbol =
                symbol_id !=
                        VITTE_SCOPE_INVALID_SYMBOL_ID
                    ? vitte_scope_get_symbol(
                          &context->scopes,
                          symbol_id)
                    : NULL;
            if (symbol == NULL) {
                break;
            }

            if (segment_index + 1u ==
                path->child_count) {
                *resolved_symbol = symbol_id;
                return true;
            }

            if (symbol->kind !=
                VITTE_SYMBOL_KIND_SPACE) {
                break;
            }

            {
                const vitte_ast_node_t *declaration;
                const vitte_sema_node_info_t *declaration_info;

                declaration =
                    (const vitte_ast_node_t *)symbol->payload;
                declaration_info =
                    declaration != NULL
                        ? vitte_sema_node_info_private(
                              context,
                              declaration->id)
                        : NULL;
                if (declaration_info == NULL) {
                    break;
                }

                current_scope =
                    declaration_info->scope_id;
            }
        }
    }

    return false;
}

static vitte_sema_type_id_t
vitte_sema_analyze_import(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    const vitte_ast_node_t *module_path;
    const vitte_ast_node_t *module_alias;
    vitte_scope_id_t module_scope;
    size_t item_index;

    if (node == NULL ||
        node->child_count == 0u) {
        return context->type_void;
    }

    module_path =
        vitte_sema_node(
            context,
            node->children[0]);
    if (module_path == NULL) {
        return context->type_void;
    }

    module_alias = NULL;
    if (node->child_count == 2u) {
        const vitte_ast_node_t *possible_alias;

        possible_alias =
            vitte_sema_node(
                context,
                node->children[1]);
        if (possible_alias != NULL &&
            possible_alias->kind ==
                VITTE_AST_NODE_IDENTIFIER) {
            module_alias = possible_alias;
        }
    }

    /*
     * A bare module import exposes the module under its final path segment.
     * For example, `use tests::pkg::lib;` makes `lib::symbol` available.
     * Keep this equivalent to the explicit `as lib` form so qualified lookup
     * can apply the same visibility rules in both cases.
     */
    if (module_alias == NULL &&
        (node->flags & VITTE_AST_FLAG_GLOB_IMPORT) == 0u &&
        node->child_count == 1u &&
        module_path->kind == VITTE_AST_NODE_PATH &&
        module_path->child_count != 0u) {
        module_alias =
            vitte_sema_node(
                context,
                module_path->children[
                    module_path->child_count - 1u]);
    }

    if (!vitte_sema_resolve_import_module(
            context,
            module_path,
            scope_id,
            &module_scope)) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME,
            module_path->span,
            module_path->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID);
        return context->type_void;
    }

    if (module_alias != NULL) {
        const vitte_scope_entry_t *module_entry;
        const vitte_ast_node_t *module_declaration;
        const char *alias_name;
        size_t alias_length;
        vitte_symbol_id_t alias_id;

        module_entry =
            vitte_scope_get_scope(
                &context->scopes,
                module_scope);
        module_declaration =
            module_entry != NULL
                ? (const vitte_ast_node_t *)module_entry->payload
                : NULL;
        if (module_declaration == NULL ||
            module_declaration->kind !=
                VITTE_AST_NODE_SPACE_DECL ||
            !vitte_sema_token_name(
                vitte_sema_node_token(
                    context,
                    module_alias),
                &alias_name,
                &alias_length)) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME,
                module_alias->span,
                module_alias->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID);
            return context->type_void;
        }

        alias_id =
            vitte_scope_declare(
                &context->scopes,
                scope_id,
                alias_name,
                alias_length,
                VITTE_SYMBOL_KIND_SPACE,
                VITTE_SYMBOL_VISIBILITY_PUBLIC,
                VITTE_SYMBOL_FLAG_IMPORTED,
                vitte_sema_scope_span_from_parser(
                    module_alias->span),
                (void *)module_declaration);
        if (alias_id ==
            VITTE_SCOPE_INVALID_SYMBOL_ID) {
            vitte_symbol_id_t existing_id;
            const vitte_symbol_t *existing_symbol;

            existing_id =
                vitte_scope_lookup_local_n(
                    &context->scopes,
                    scope_id,
                    alias_name,
                    alias_length);
            existing_symbol =
                existing_id !=
                        VITTE_SCOPE_INVALID_SYMBOL_ID
                    ? vitte_scope_get_symbol(
                          &context->scopes,
                          existing_id)
                    : NULL;
            if (existing_symbol != NULL &&
                (existing_symbol->flags &
                 VITTE_SYMBOL_FLAG_IMPORTED) != 0u &&
                existing_symbol->kind ==
                    VITTE_SYMBOL_KIND_SPACE &&
                vitte_sema_same_declaration_private(
                    existing_symbol->payload,
                    module_declaration)) {
                return context->type_void;
            }

            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                module_alias->span,
                module_alias->id,
                existing_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID);
        } else {
            vitte_sema_node_info_t *alias_info;

            alias_info =
                vitte_sema_node_info_mut_private(
                    context,
                    module_alias->id);
            if (alias_info != NULL) {
                alias_info->symbol_id = alias_id;
                alias_info->type_id = context->type_void;
                alias_info->scope_id = scope_id;
                alias_info->flags |=
                    VITTE_SEMA_NODE_FLAG_RESOLVED |
                    VITTE_SEMA_NODE_FLAG_DECLARATION;
            }
        }
        return context->type_void;
    }

    if ((node->flags &
         VITTE_AST_FLAG_GLOB_IMPORT) != 0u) {
        const vitte_scope_entry_t *module_entry;
        size_t symbol_index;

        module_entry =
            vitte_scope_get_scope(
                &context->scopes,
                module_scope);
        for (symbol_index = 0u;
             symbol_index <
                 vitte_scope_entry_symbol_count(
                     module_entry);
             ++symbol_index) {
            vitte_symbol_id_t target_id;
            const vitte_symbol_t *target_symbol;
            const vitte_ast_node_t *target_declaration;

            target_id =
                vitte_scope_entry_symbol_at(
                    module_entry,
                    symbol_index);
            target_symbol =
                vitte_scope_get_symbol(
                    &context->scopes,
                    target_id);
            if (!vitte_symbol_is_public(target_symbol)) {
                continue;
            }

            target_declaration =
                (const vitte_ast_node_t *)target_symbol->payload;
            {
                vitte_symbol_id_t existing_id;
                const vitte_symbol_t *existing_symbol;

                existing_id =
                    vitte_scope_lookup_local_n(
                        &context->scopes,
                        scope_id,
                        target_symbol->name,
                        target_symbol->name_length);
                existing_symbol =
                    existing_id !=
                            VITTE_SCOPE_INVALID_SYMBOL_ID
                        ? vitte_scope_get_symbol(
                              &context->scopes,
                              existing_id)
                        : NULL;
                if (existing_symbol != NULL &&
                    (existing_symbol->flags &
                     VITTE_SYMBOL_FLAG_IMPORTED) != 0u &&
                    existing_symbol->kind ==
                        target_symbol->kind &&
                    vitte_sema_same_declaration_private(
                        existing_symbol->payload,
                        target_declaration)) {
                    continue;
                }
            }
            if (vitte_scope_declare(
                    &context->scopes,
                    scope_id,
                    target_symbol->name,
                    target_symbol->name_length,
                    target_symbol->kind,
                    VITTE_SYMBOL_VISIBILITY_PUBLIC,
                    VITTE_SYMBOL_FLAG_IMPORTED,
                    vitte_sema_scope_span_from_parser(
                        node->span),
                    (void *)target_declaration) ==
                VITTE_SCOPE_INVALID_SYMBOL_ID) {
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                    node->span,
                    node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID);
            }
        }
        return context->type_void;
    }

    for (item_index = 1u;
         item_index < node->child_count;
         ++item_index) {
        const vitte_ast_node_t *item;
        const vitte_ast_node_t *alias_node;
        const vitte_symbol_t *target_symbol;
        const vitte_ast_node_t *target_declaration;
        const char *alias;
        size_t alias_length;
        vitte_symbol_id_t target_id;
        vitte_symbol_id_t alias_id;

        item =
            vitte_sema_node(
                context,
                node->children[item_index]);
        if (item == NULL) {
            continue;
        }

        if (!vitte_sema_resolve_import_item(
                context,
                item,
                module_scope,
                scope_id,
                &target_id)) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME,
                item->span,
                item->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID);
            continue;
        }

        target_symbol =
            vitte_scope_get_symbol(
                &context->scopes,
                target_id);
        if (target_symbol == NULL) {
            continue;
        }

        alias_node = NULL;
        if (item_index + 1u < node->child_count) {
            const vitte_ast_node_t *candidate;

            candidate =
                vitte_sema_node(
                    context,
                    node->children[item_index + 1u]);
            if (candidate != NULL &&
                candidate->kind ==
                    VITTE_AST_NODE_IDENTIFIER) {
                alias_node = candidate;
                ++item_index;
            }
        }

        if (alias_node != NULL) {
            if (!vitte_sema_token_name(
                    vitte_sema_node_token(
                        context,
                        alias_node),
                    &alias,
                    &alias_length)) {
                continue;
            }
        } else {
            alias = target_symbol->name;
            alias_length = target_symbol->name_length;
        }

        target_declaration =
            (const vitte_ast_node_t *)target_symbol->payload;
        alias_id =
            vitte_scope_declare(
                &context->scopes,
                scope_id,
                alias,
                alias_length,
                target_symbol->kind,
                VITTE_SYMBOL_VISIBILITY_PUBLIC,
                VITTE_SYMBOL_FLAG_IMPORTED,
                vitte_sema_scope_span_from_parser(
                    item->span),
                (void *)target_declaration);
        if (alias_id ==
            VITTE_SCOPE_INVALID_SYMBOL_ID) {
            vitte_symbol_id_t existing_id;
            const vitte_symbol_t *existing_symbol;

            existing_id =
                vitte_scope_lookup_local_n(
                    &context->scopes,
                    scope_id,
                    alias,
                    alias_length);
            existing_symbol =
                existing_id !=
                        VITTE_SCOPE_INVALID_SYMBOL_ID
                    ? vitte_scope_get_symbol(
                          &context->scopes,
                          existing_id)
                    : NULL;
            if (existing_symbol != NULL &&
                (existing_symbol->flags &
                 VITTE_SYMBOL_FLAG_IMPORTED) != 0u &&
                existing_symbol->kind ==
                    target_symbol->kind &&
                vitte_sema_same_declaration_private(
                    existing_symbol->payload,
                    target_declaration)) {
                continue;
            }
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                item->span,
                item->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID);
        } else {
            vitte_sema_node_info_t *item_info;

            item_info =
                vitte_sema_node_info_mut_private(
                    context,
                    item->id);
            if (item_info != NULL) {
                item_info->symbol_id = alias_id;
                item_info->type_id =
                    vitte_sema_type_for_symbol(
                        context,
                        target_id);
                item_info->scope_id = scope_id;
                item_info->flags |=
                    VITTE_SEMA_NODE_FLAG_RESOLVED |
                    VITTE_SEMA_NODE_FLAG_DECLARATION;
            }
        }
    }

    return context->type_void;
}

static void
vitte_sema_register_export_aliases(
    vitte_sema_t *context,
    const vitte_ast_node_t *body,
    vitte_scope_id_t scope_id)
{
    size_t declaration_index;

    if (context == NULL ||
        body == NULL ||
        body->kind != VITTE_AST_NODE_BLOCK) {
        return;
    }

    for (declaration_index = 0u;
         declaration_index < body->child_count;
         ++declaration_index) {
        const vitte_ast_node_t *export_node;
        size_t item_index;

        export_node =
            vitte_sema_node(
                context,
                body->children[declaration_index]);
        if (export_node == NULL ||
            export_node->kind !=
                VITTE_AST_NODE_USE_DECL ||
            (export_node->flags &
             VITTE_AST_FLAG_PUBLIC) == 0u) {
            continue;
        }

        for (item_index = 0u;
             item_index + 1u < export_node->child_count;
             ++item_index) {
            const vitte_ast_node_t *item;
            const vitte_ast_node_t *alias_node;
            const vitte_ast_node_t *segment;
            const vitte_symbol_t *target_symbol;
            const char *source_name;
            const char *alias;
            size_t source_name_length;
            size_t alias_length;
            vitte_symbol_id_t target_id;

            item =
                vitte_sema_node(
                    context,
                    export_node->children[item_index]);
            alias_node =
                vitte_sema_node(
                    context,
                    export_node->children[item_index + 1u]);
            if (item == NULL ||
                item->kind != VITTE_AST_NODE_PATH ||
                alias_node == NULL ||
                alias_node->kind !=
                    VITTE_AST_NODE_IDENTIFIER ||
                item->child_count == 0u) {
                continue;
            }

            segment =
                vitte_sema_node(
                    context,
                    item->children[
                        item->child_count - 1u]);
            if (segment == NULL ||
                !vitte_sema_token_name(
                    vitte_sema_node_token(
                        context,
                        segment),
                    &source_name,
                    &source_name_length) ||
                !vitte_sema_token_name(
                    vitte_sema_node_token(
                        context,
                        alias_node),
                    &alias,
                    &alias_length)) {
                continue;
            }

            ++item_index;
            if (source_name_length == alias_length &&
                memcmp(source_name, alias, alias_length) == 0) {
                continue;
            }

            target_id =
                vitte_scope_lookup_local_n(
                    &context->scopes,
                    scope_id,
                    source_name,
                    source_name_length);
            target_symbol =
                target_id != VITTE_SCOPE_INVALID_SYMBOL_ID
                    ? vitte_scope_get_symbol(
                          &context->scopes,
                          target_id)
                    : NULL;
            if (target_symbol == NULL) {
                continue;
            }

            if (vitte_scope_declare(
                    &context->scopes,
                    scope_id,
                    alias,
                    alias_length,
                    target_symbol->kind,
                    VITTE_SYMBOL_VISIBILITY_PUBLIC,
                    VITTE_SYMBOL_FLAG_IMPORTED,
                    vitte_sema_scope_span_from_parser(
                        alias_node->span),
                    target_symbol->payload) ==
                VITTE_SCOPE_INVALID_SYMBOL_ID) {
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                    alias_node->span,
                    alias_node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID);
            }
        }
    }
}

/* ========================================================================= */
/* Declaration type inference                                                */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_infer_declaration_type(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    size_t index;
    vitte_sema_type_id_t explicit_type;
    vitte_sema_type_id_t initializer_type;
    vitte_ast_node_id_t initializer_node_id;

    explicit_type =
        VITTE_SEMA_INVALID_TYPE_ID;

    initializer_type =
        VITTE_SEMA_INVALID_TYPE_ID;
    initializer_node_id =
        VITTE_AST_INVALID_ID;

    for (index = 0u;
         index < node->child_count;
         ++index) {
        const vitte_ast_node_t *child;

        child =
            vitte_sema_node(
                context,
                node->children[index]);

        if (child == NULL) {
            continue;
        }

        if (child->kind ==
                VITTE_AST_NODE_TYPE_EXPR ||
            child->kind ==
                VITTE_AST_NODE_POINTER_TYPE ||
            child->kind ==
                VITTE_AST_NODE_REFERENCE_TYPE ||
            child->kind ==
                VITTE_AST_NODE_ARRAY_TYPE) {
            if (explicit_type ==
                VITTE_SEMA_INVALID_TYPE_ID) {
                explicit_type =
                    vitte_sema_analyze_type_node(
                        context,
                        child->id,
                        scope_id);
            }

            continue;
        }

        if (child->kind ==
            VITTE_AST_NODE_IDENTIFIER) {
            continue;
        }

        if (initializer_type ==
            VITTE_SEMA_INVALID_TYPE_ID) {
            initializer_type =
                vitte_sema_analyze_expression(
                    context,
                    child->id,
                    scope_id);
            initializer_node_id = child->id;
        }
    }

    if (explicit_type !=
        VITTE_SEMA_INVALID_TYPE_ID) {
        if (initializer_type !=
                VITTE_SEMA_INVALID_TYPE_ID &&
            !vitte_sema_type_assignable_private(
                context,
                explicit_type,
                initializer_type) &&
            vitte_sema_contextualize_integer_literal(
                context,
                initializer_node_id,
                explicit_type)) {
            initializer_type = explicit_type;
        }

        if (initializer_type !=
                VITTE_SEMA_INVALID_TYPE_ID &&
            !vitte_sema_type_assignable_private(
                context,
                explicit_type,
                initializer_type)) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                node->span,
                node->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                explicit_type,
                initializer_type);
        }

        return explicit_type;
    }

    if (initializer_type !=
        VITTE_SEMA_INVALID_TYPE_ID) {
        return initializer_type;
    }

    return context->type_error;
}

static vitte_sema_type_id_t
vitte_sema_analyze_member(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    const vitte_ast_node_t *member_node;
    const vitte_token_t *member_token;
    const vitte_sema_node_info_t *base_info;
    const vitte_sema_type_t *base_type;
    const vitte_symbol_t *type_symbol;
    const vitte_ast_node_t *type_declaration;
    vitte_sema_type_id_t base_type_id;
    vitte_symbol_id_t field_symbol_id;
    const vitte_symbol_t *field_symbol;
    const vitte_ast_node_t *field_declaration;
    vitte_sema_node_info_t *info;
    size_t scope_index;
    bool mutable_receiver;

    if (context == NULL ||
        node == NULL ||
        node->child_count < 2u) {
        return context != NULL
            ? context->type_error
            : VITTE_SEMA_INVALID_TYPE_ID;
    }

    base_type_id =
        vitte_sema_analyze_expression(
            context,
            node->children[0],
            scope_id);

    if (vitte_sema_type_is_error_private(
            context,
            base_type_id)) {
        return context->type_error;
    }

    base_info =
        vitte_sema_node_info_private(
            context,
            node->children[0]);
    base_type =
        vitte_sema_type_private(
            context,
            base_type_id);
    mutable_receiver =
        base_info != NULL &&
        (base_info->flags &
         VITTE_SEMA_NODE_FLAG_MUTABLE) != 0u;

    while (base_type != NULL &&
           (base_type->kind ==
                VITTE_SEMA_TYPE_REFERENCE ||
            base_type->kind ==
                VITTE_SEMA_TYPE_POINTER)) {
        if (base_type->is_mutable) {
            mutable_receiver = true;
        }

        base_type_id = base_type->element_type;
        base_type =
            vitte_sema_type_private(
                context,
                base_type_id);
    }

    if (base_type == NULL ||
        base_type->kind !=
            VITTE_SEMA_TYPE_NAMED) {
        return context->type_error;
    }

    type_symbol =
        vitte_scope_get_symbol(
            &context->scopes,
            base_type->symbol_id);
    type_declaration =
        type_symbol != NULL
            ? (const vitte_ast_node_t *)
                  type_symbol->payload
            : NULL;

    member_node =
        vitte_sema_node(
            context,
            node->children[1]);
    member_token =
        vitte_sema_node_token(
            context,
            member_node);

    if (type_declaration == NULL ||
        type_declaration->kind !=
            VITTE_AST_NODE_FORM_DECL ||
        member_token == NULL ||
        member_token->lexeme == NULL ||
        member_token->length == 0u) {
        return context->type_error;
    }

    field_symbol_id =
        VITTE_SCOPE_INVALID_SYMBOL_ID;

    for (scope_index = 0u;
         scope_index < context->scopes.scope_count;
         ++scope_index) {
        const vitte_scope_entry_t *scope;

        scope = &context->scopes.scopes[scope_index];

        if (scope->kind ==
                VITTE_SCOPE_KIND_FORM &&
            scope->payload ==
                (void *)type_declaration) {
            field_symbol_id =
                vitte_scope_lookup_local_n(
                    &context->scopes,
                    scope->id,
                    member_token->lexeme,
                    member_token->length);
            break;
        }
    }

    field_symbol =
        field_symbol_id !=
                VITTE_SCOPE_INVALID_SYMBOL_ID
            ? vitte_scope_get_symbol(
                  &context->scopes,
                  field_symbol_id)
            : NULL;
    field_declaration =
        field_symbol != NULL
            ? (const vitte_ast_node_t *)
                  field_symbol->payload
            : NULL;

    if (field_declaration == NULL ||
        field_declaration->kind !=
            VITTE_AST_NODE_FIELD_DECL) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_UNKNOWN_MEMBER,
            member_node != NULL
                ? member_node->span
                : node->span,
            member_node != NULL
                ? member_node->id
                : node->id,
            base_type->symbol_id,
            VITTE_SEMA_INVALID_TYPE_ID,
            base_type_id);

        return context->type_error;
    }

    {
        const vitte_sema_node_info_t *field_info;
        vitte_sema_type_id_t field_type_id;

        field_info =
            vitte_sema_node_info_private(
                context,
                field_declaration->id);
        field_type_id =
            field_info != NULL
                ? field_info->type_id
                : VITTE_SEMA_INVALID_TYPE_ID;

        if (field_type_id ==
            VITTE_SEMA_INVALID_TYPE_ID) {
            field_type_id =
                vitte_sema_infer_declaration_type(
                    context,
                    field_declaration,
                    scope_id);
        }

        info =
            vitte_sema_node_info_mut_private(
                context,
                node->id);

        if (info != NULL) {
            info->type_id = field_type_id;
            info->symbol_id = field_symbol_id;
            info->flags |=
                VITTE_SEMA_NODE_FLAG_LVALUE;

            if (mutable_receiver ||
                (field_symbol->flags &
                 VITTE_SYMBOL_FLAG_MUTABLE) != 0u) {
                info->flags |=
                    VITTE_SEMA_NODE_FLAG_MUTABLE;
            }
        }

        return field_type_id;
    }
}

/* ========================================================================= */
/* Procedure analysis                                                        */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_proc(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t parent_scope,
    vitte_symbol_id_t symbol_id)
{
    vitte_scope_id_t proc_scope;
    vitte_sema_type_id_t previous_return;
    vitte_scope_id_t previous_proc_scope;
    size_t index;
    vitte_sema_type_id_t return_type;
    vitte_sema_type_id_t function_type;
    vitte_sema_node_info_t *info;

    proc_scope =
        vitte_scope_create(
            &context->scopes,
            parent_scope,
            VITTE_SCOPE_KIND_PROC,
            vitte_sema_scope_span_from_parser(
                node->span),
            (void *)node);

    if (proc_scope ==
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        return context->type_error;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->scope_id = proc_scope;
    }

    previous_return =
        context->current_return_type;

    previous_proc_scope =
        context->current_proc_scope;

    context->current_proc_scope =
        proc_scope;

    return_type =
        context->type_void;

    /*
     * Register parameters/generic parameters before the body.
     */
    for (index = 0u;
         index < node->child_count;
         ++index) {
        const vitte_ast_node_t *child;

        child =
            vitte_sema_node(
                context,
                node->children[index]);

        if (child == NULL) {
            continue;
        }

        if (child->kind ==
                VITTE_AST_NODE_PARAMETER ||
            child->kind ==
                VITTE_AST_NODE_GENERIC_PARAM) {
            vitte_symbol_id_t parameter_symbol;
            vitte_sema_type_id_t parameter_type;
            vitte_sema_node_info_t *parameter_info;

            parameter_symbol =
                vitte_sema_register_declaration(
                    context,
                    child,
                    proc_scope);

            parameter_type =
                vitte_sema_infer_declaration_type(
                    context,
                    child,
                    proc_scope);

            parameter_info =
                vitte_sema_node_info_mut_private(
                    context,
                    child->id);

            if (parameter_info != NULL) {
                parameter_info->type_id =
                    parameter_type;
            }

            (void)parameter_symbol;
        }
    }

    /*
     * Detect an explicit return type among type children that are not part
     * of parameters. Parser representation is intentionally kept generic.
     */
    for (index = 0u;
         index < node->child_count;
         ++index) {
        const vitte_ast_node_t *child;

        child =
            vitte_sema_node(
                context,
                node->children[index]);

        if (child == NULL) {
            continue;
        }

        if (child->kind ==
                VITTE_AST_NODE_TYPE_EXPR ||
            child->kind ==
                VITTE_AST_NODE_POINTER_TYPE ||
            child->kind ==
                VITTE_AST_NODE_REFERENCE_TYPE ||
            child->kind ==
                VITTE_AST_NODE_ARRAY_TYPE) {
            return_type =
                vitte_sema_analyze_type_node(
                    context,
                    child->id,
                    proc_scope);
        }
    }

    context->current_return_type =
        return_type;

    function_type =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_FUNCTION,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            symbol_id,
            false);

    if (function_type !=
        VITTE_SEMA_INVALID_TYPE_ID) {
        vitte_sema_type_t *type;

        type =
            vitte_sema_type_mut_private(
                context,
                function_type);

        if (type != NULL) {
            type->return_type =
                return_type;
        }
    }

    if (info != NULL) {
        info->type_id =
            function_type;
    }

    for (index = 0u;
         index < node->child_count;
         ++index) {
        const vitte_ast_node_t *child;

        child =
            vitte_sema_node(
                context,
                node->children[index]);

        if (child == NULL) {
            continue;
        }

        if ((child->flags &
             (VITTE_AST_FLAG_CONTRACT_REQUIRES |
              VITTE_AST_FLAG_CONTRACT_ENSURES)) != 0u) {
            vitte_sema_type_id_t contract_type;

            contract_type =
                vitte_sema_analyze_expression(
                    context,
                    child->id,
                    proc_scope);
            if (contract_type != context->type_bool &&
                contract_type != context->type_error) {
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_CONDITION_NOT_BOOL,
                    child->span,
                    child->id,
                    symbol_id,
                    context->type_bool,
                    contract_type);
            }
            continue;
        }

        if (child->kind ==
                VITTE_AST_NODE_PARAMETER ||
            child->kind ==
                VITTE_AST_NODE_GENERIC_PARAM ||
            child->kind ==
                VITTE_AST_NODE_TYPE_EXPR ||
            child->kind ==
                VITTE_AST_NODE_POINTER_TYPE ||
            child->kind ==
                VITTE_AST_NODE_REFERENCE_TYPE ||
            child->kind ==
                VITTE_AST_NODE_ARRAY_TYPE ||
            child->kind ==
                VITTE_AST_NODE_IDENTIFIER) {
            continue;
        }

        (void)vitte_sema_analyze_node(
            context,
            child->id,
            proc_scope);
    }

    context->current_return_type =
        previous_return;

    context->current_proc_scope =
        previous_proc_scope;

    return function_type;
}

/* ========================================================================= */
/* Statement analysis                                                        */
/* ========================================================================= */

static void
vitte_sema_predeclare_types(
    vitte_sema_t *context,
    const vitte_ast_node_id_t *declarations,
    size_t declaration_count,
    vitte_scope_id_t scope_id);

static vitte_sema_type_id_t
vitte_sema_analyze_block(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t parent_scope)
{
    vitte_scope_id_t block_scope;
    size_t index;
    bool terminated;
    vitte_sema_node_info_t *info;

    block_scope =
        vitte_scope_create(
            &context->scopes,
            parent_scope,
            VITTE_SCOPE_KIND_BLOCK,
            vitte_sema_scope_span_from_parser(
                node->span),
            (void *)node);

    if (block_scope ==
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        return context->type_error;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->scope_id = block_scope;
        info->type_id = context->type_void;
    }

    vitte_sema_predeclare_types(
        context,
        node->children,
        node->child_count,
        block_scope);

    terminated = false;

    for (index = 0u;
         index < node->child_count;
         ++index) {
        const vitte_ast_node_t *child;

        child =
            vitte_sema_node(
                context,
                node->children[index]);

        if (child == NULL) {
            continue;
        }

        if (terminated) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_WARNING,
                VITTE_SEMA_DIAGNOSTIC_UNREACHABLE_CODE,
                child->span,
                child->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID);

            context->stats.unreachable_statements =
                vitte_sema_u64_add_sat(
                    context->stats.unreachable_statements,
                    UINT64_C(1));
        }

        (void)vitte_sema_analyze_node(
            context,
            child->id,
            block_scope);

        if (child->kind ==
                VITTE_AST_NODE_RETURN_STMT ||
            child->kind ==
                VITTE_AST_NODE_BREAK_STMT ||
            child->kind ==
                VITTE_AST_NODE_CONTINUE_STMT) {
            terminated = true;
        }
    }

    return context->type_void;
}

static vitte_sema_type_id_t
vitte_sema_analyze_return(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id)
{
    vitte_sema_type_id_t found;
    const vitte_sema_type_t *return_type;

    if (context->current_proc_scope ==
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_RETURN_OUTSIDE_PROC,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID);

        return context->type_error;
    }

    return_type =
        vitte_sema_type_private(
            context,
            context->current_return_type);

    if (node->child_count == 0u) {
        found =
            context->type_void;
    } else {
        found =
            vitte_sema_analyze_expression(
                context,
                node->children[0],
                scope_id);

        if (return_type == NULL ||
            return_type->kind !=
                VITTE_SEMA_TYPE_REFERENCE) {
            if (!vitte_sema_type_assignable_private(
                    context,
                    context->current_return_type,
                    vitte_sema_value_type_private(
                        context,
                        found)) &&
                vitte_sema_contextualize_integer_literal(
                    context,
                    node->children[0],
                    context->current_return_type)) {
                found = context->current_return_type;
            } else {
                found =
                    vitte_sema_value_type_private(
                        context,
                        found);
            }
        }
    }

    if (!vitte_sema_type_assignable_private(
            context,
            context->current_return_type,
            found)) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_RETURN_TYPE_MISMATCH,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            context->current_return_type,
            found);
    }

    return context->type_void;
}

static vitte_sema_type_id_t
vitte_sema_analyze_condition(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id,
    vitte_scope_id_t scope_id)
{
    vitte_sema_type_id_t type;
    const vitte_ast_node_t *node;

    type =
        vitte_sema_analyze_expression(
            context,
            node_id,
            scope_id);
    type =
        vitte_sema_value_type_private(
            context,
            type);

    if (type !=
            context->type_bool &&
        type !=
            context->type_error) {
        node =
            vitte_sema_node(
                context,
                node_id);

        if (node != NULL) {
            (void)vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_CONDITION_NOT_BOOL,
                node->span,
                node_id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                context->type_bool,
                type);
        }
    }

    return type;
}

static void
vitte_sema_predeclare_procedures(
    vitte_sema_t *context,
    const vitte_ast_node_id_t *declarations,
    size_t declaration_count,
    vitte_scope_id_t scope_id)
{
    size_t index;

    for (index = 0u;
         index < declaration_count;
         ++index) {
        const vitte_ast_node_t *declaration;
        vitte_sema_node_info_t *info;
        vitte_symbol_id_t symbol_id;
        vitte_sema_type_id_t return_type;
        vitte_sema_type_id_t function_type;
        size_t child_index;
        bool has_generic_parameters;

        declaration =
            vitte_sema_node(
                context,
                declarations[index]);
        if (declaration == NULL ||
            (declaration->kind !=
                 VITTE_AST_NODE_PROC_DECL &&
             declaration->kind !=
                 VITTE_AST_NODE_EXTERN_PROC_DECL)) {
            continue;
        }

        symbol_id =
            vitte_sema_register_declaration(
            context,
            declaration,
            scope_id);

        info =
            vitte_sema_node_info_mut_private(
                context,
                declaration->id);
        if (info != NULL) {
            info->flags |=
                VITTE_SEMA_NODE_FLAG_PREDECLARED;
        }

        return_type = context->type_void;
        has_generic_parameters = false;

        for (child_index = 0u;
             child_index < declaration->child_count;
             ++child_index) {
            const vitte_ast_node_t *child;

            child =
                vitte_sema_node(
                    context,
                    declaration->children[child_index]);
            if (child == NULL) {
                continue;
            }

            if (child->kind ==
                VITTE_AST_NODE_GENERIC_PARAM) {
                has_generic_parameters = true;
                continue;
            }

            if (child->kind ==
                VITTE_AST_NODE_PARAMETER) {
                vitte_sema_node_info_t *parameter_info;
                vitte_sema_type_id_t parameter_type;

                parameter_type =
                    vitte_sema_infer_declaration_type(
                    context,
                    child,
                    scope_id);
                parameter_info =
                    vitte_sema_node_info_mut_private(
                        context,
                        child->id);
                if (parameter_info != NULL) {
                    parameter_info->type_id = parameter_type;
                    parameter_info->flags |=
                        VITTE_SEMA_NODE_FLAG_PREDECLARED;
                }
                continue;
            }

            if (child->kind ==
                    VITTE_AST_NODE_TYPE_EXPR ||
                child->kind ==
                    VITTE_AST_NODE_POINTER_TYPE ||
                child->kind ==
                    VITTE_AST_NODE_REFERENCE_TYPE ||
                child->kind ==
                    VITTE_AST_NODE_ARRAY_TYPE) {
                return_type =
                    vitte_sema_analyze_type_node(
                        context,
                        child->id,
                        scope_id);
            }
        }

        if (has_generic_parameters ||
            symbol_id ==
                VITTE_SCOPE_INVALID_SYMBOL_ID) {
            continue;
        }

        function_type =
            vitte_sema_create_type_private(
                context,
                VITTE_SEMA_TYPE_FUNCTION,
                VITTE_SEMA_INVALID_TYPE_ID,
                0u,
                symbol_id,
                false);
        if (function_type !=
            VITTE_SEMA_INVALID_TYPE_ID) {
            vitte_sema_type_t *type;

            type =
                vitte_sema_type_mut_private(
                    context,
                    function_type);
            if (type != NULL) {
                type->return_type = return_type;
            }
            if (info != NULL) {
                info->type_id = function_type;
            }
        }
    }
}

static void
vitte_sema_predeclare_types(
    vitte_sema_t *context,
    const vitte_ast_node_id_t *declarations,
    size_t declaration_count,
    vitte_scope_id_t scope_id)
{
    size_t index;

    for (index = 0u;
         index < declaration_count;
         ++index) {
        const vitte_ast_node_t *declaration;
        vitte_symbol_id_t symbol_id;
        vitte_sema_type_id_t named_type;
        vitte_sema_node_info_t *info;

        declaration =
            vitte_sema_node(
                context,
                declarations[index]);
        if (declaration == NULL ||
            (declaration->kind != VITTE_AST_NODE_TYPE_DECL &&
             declaration->kind != VITTE_AST_NODE_OPAQUE_DECL &&
             declaration->kind != VITTE_AST_NODE_FORM_DECL &&
             declaration->kind != VITTE_AST_NODE_PICK_DECL &&
             declaration->kind != VITTE_AST_NODE_TRAIT_DECL)) {
            continue;
        }

        symbol_id =
            vitte_sema_register_declaration(
                context,
                declaration,
                scope_id);
        if (symbol_id ==
            VITTE_SCOPE_INVALID_SYMBOL_ID) {
            continue;
        }

        named_type =
            vitte_sema_create_type_private(
                context,
                VITTE_SEMA_TYPE_NAMED,
                VITTE_SEMA_INVALID_TYPE_ID,
                0u,
                symbol_id,
                false);

        info =
            vitte_sema_node_info_mut_private(
                context,
                declaration->id);
        if (info != NULL) {
            info->type_id = named_type;
            info->flags |=
                VITTE_SEMA_NODE_FLAG_PREDECLARED;
        }
    }
}

/* ========================================================================= */
/* General node analysis                                                     */
/* ========================================================================= */

static vitte_sema_type_id_t
vitte_sema_analyze_node(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id,
    vitte_scope_id_t scope_id)
{
    const vitte_ast_node_t *node;
    vitte_sema_node_info_t *info;
    vitte_sema_type_id_t result;
    size_t index;

    node =
        vitte_sema_node(
            context,
            node_id);

    if (node == NULL) {
        return context->type_error;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node_id);

    if (info != NULL) {
        info->scope_id = scope_id;
    }

    context->stats.nodes_visited =
        vitte_sema_u64_add_sat(
            context->stats.nodes_visited,
            UINT64_C(1));

    switch (node->kind) {
        case VITTE_AST_NODE_TRANSLATION_UNIT:
            vitte_sema_predeclare_types(
                context,
                node->children,
                node->child_count,
                scope_id);
            vitte_sema_predeclare_procedures(
                context,
                node->children,
                node->child_count,
                scope_id);

            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    scope_id);
            }

            return context->type_void;

        case VITTE_AST_NODE_CONST_DECL:
        case VITTE_AST_NODE_STATIC_DECL:
        case VITTE_AST_NODE_LET_STMT:
        case VITTE_AST_NODE_FIELD_DECL:
        case VITTE_AST_NODE_VARIANT_DECL:
        {
            vitte_symbol_id_t symbol_id;

            symbol_id =
                vitte_sema_register_declaration(
                    context,
                    node,
                    scope_id);

            result =
                vitte_sema_infer_declaration_type(
                    context,
                    node,
                    scope_id);

            if (info != NULL) {
                info->symbol_id = symbol_id;
                info->type_id = result;
            }

            return result;
        }

        case VITTE_AST_NODE_TYPE_DECL:
        case VITTE_AST_NODE_OPAQUE_DECL:
        case VITTE_AST_NODE_FORM_DECL:
        case VITTE_AST_NODE_PICK_DECL:
        case VITTE_AST_NODE_TRAIT_DECL:
        {
            vitte_symbol_id_t symbol_id;
            vitte_scope_kind_t child_kind;
            vitte_scope_id_t child_scope;
            vitte_sema_type_id_t named_type;

            if (info != NULL &&
                (info->flags &
                 VITTE_SEMA_NODE_FLAG_PREDECLARED) != 0u) {
                symbol_id = info->symbol_id;
                named_type = info->type_id;
            } else {
                symbol_id =
                    vitte_sema_register_declaration(
                        context,
                        node,
                        scope_id);

                named_type =
                    vitte_sema_create_type_private(
                        context,
                        VITTE_SEMA_TYPE_NAMED,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        0u,
                        symbol_id,
                        false);
            }

            if (info != NULL) {
                info->symbol_id = symbol_id;
                info->type_id = named_type;
            }

            child_kind =
                vitte_sema_scope_kind_for_node(
                    node->kind);

            child_scope =
                child_kind !=
                        VITTE_SCOPE_KIND_INVALID
                    ? vitte_scope_create(
                          &context->scopes,
                          scope_id,
                          child_kind,
                          vitte_sema_scope_span_from_parser(
                              node->span),
                          (void *)node)
                    : scope_id;

            if (child_scope ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
                child_scope = scope_id;
            }

            if (info != NULL) {
                info->scope_id = child_scope;
            }

            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *child;

                child =
                    vitte_sema_node(
                        context,
                        node->children[index]);

                if (child == NULL ||
                    child->kind ==
                        VITTE_AST_NODE_IDENTIFIER) {
                    continue;
                }

                (void)vitte_sema_analyze_node(
                    context,
                    child->id,
                    child_scope);
            }

            return named_type;
        }

        case VITTE_AST_NODE_PROC_DECL:
        case VITTE_AST_NODE_EXTERN_PROC_DECL:
        {
            vitte_symbol_id_t symbol_id;

            if (info != NULL &&
                (info->flags &
                 VITTE_SEMA_NODE_FLAG_PREDECLARED) != 0u) {
                symbol_id = info->symbol_id;
                if (symbol_id ==
                    VITTE_SCOPE_INVALID_SYMBOL_ID) {
                    return context->type_error;
                }
            } else {
                symbol_id =
                    vitte_sema_register_declaration(
                        context,
                        node,
                        scope_id);
            }

            return vitte_sema_analyze_proc(
                context,
                node,
                scope_id,
                symbol_id);
        }

        case VITTE_AST_NODE_SPACE_DECL:
        case VITTE_AST_NODE_MACRO_DECL:
        case VITTE_AST_NODE_TEST_DECL:
        {
            vitte_symbol_id_t symbol_id;
            vitte_scope_kind_t kind;
            vitte_scope_id_t child_scope;

            kind =
                vitte_sema_scope_kind_for_node(
                    node->kind);

            symbol_id =
                VITTE_SCOPE_INVALID_SYMBOL_ID;
            child_scope =
                VITTE_SCOPE_INVALID_SCOPE_ID;
            if (node->kind ==
                VITTE_AST_NODE_SPACE_DECL) {
                const char *name;
                size_t name_length;

                if (vitte_sema_declaration_name(
                        context,
                        node,
                        &name,
                        &name_length)) {
                    vitte_symbol_id_t existing_id;
                    const vitte_symbol_t *existing_symbol;

                    existing_id =
                        vitte_scope_lookup_local_n(
                            &context->scopes,
                            scope_id,
                            name,
                            name_length);
                    existing_symbol =
                        existing_id !=
                                VITTE_SCOPE_INVALID_SYMBOL_ID
                            ? vitte_scope_get_symbol(
                                  &context->scopes,
                                  existing_id)
                            : NULL;
                    if (existing_symbol != NULL &&
                        existing_symbol->kind ==
                            VITTE_SYMBOL_KIND_SPACE &&
                        vitte_sema_space_scope_private(
                            context,
                            existing_symbol,
                            (const vitte_ast_node_t *)
                                existing_symbol->payload,
                            &child_scope)) {
                        symbol_id = existing_id;
                    }
                }
            }

            if (symbol_id ==
                VITTE_SCOPE_INVALID_SYMBOL_ID) {
                symbol_id =
                    vitte_sema_register_declaration(
                        context,
                        node,
                        scope_id);

                child_scope =
                    vitte_scope_create(
                        &context->scopes,
                        scope_id,
                        kind,
                        vitte_sema_scope_span_from_parser(
                            node->span),
                        (void *)node);
            }

            if (child_scope ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
                child_scope = scope_id;
            }

            if (info != NULL) {
                info->symbol_id = symbol_id;
                info->scope_id = child_scope;
                info->type_id =
                    context->type_void;
            }

            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *child;

                child =
                    vitte_sema_node(
                        context,
                        node->children[index]);

                if (child == NULL ||
                    child->kind ==
                        VITTE_AST_NODE_IDENTIFIER ||
                    child->kind ==
                        VITTE_AST_NODE_USE_DECL ||
                    (node->kind ==
                         VITTE_AST_NODE_SPACE_DECL &&
                     child->kind ==
                         VITTE_AST_NODE_PATH)) {
                    continue;
                }

                if (node->kind ==
                        VITTE_AST_NODE_SPACE_DECL &&
                    child->kind ==
                        VITTE_AST_NODE_BLOCK) {
                    size_t declaration_index;

                    for (declaration_index = 0u;
                         declaration_index <
                             child->child_count;
                         ++declaration_index) {
                        const vitte_ast_node_t *declaration;

                        declaration =
                            vitte_sema_node(
                                context,
                                child->children[
                                    declaration_index]);
                        if (declaration != NULL &&
                            declaration->kind ==
                                VITTE_AST_NODE_USE_DECL) {
                            (void)vitte_sema_analyze_import(
                                context,
                                declaration,
                                child_scope);
                        }
                    }

                    vitte_sema_predeclare_types(
                        context,
                        child->children,
                        child->child_count,
                        child_scope);
                    vitte_sema_predeclare_procedures(
                        context,
                        child->children,
                        child->child_count,
                        child_scope);

                    for (declaration_index = 0u;
                         declaration_index <
                             child->child_count;
                         ++declaration_index) {
                        (void)vitte_sema_analyze_node(
                            context,
                            child->children[
                                declaration_index],
                            child_scope);
                    }
                    continue;
                }

                (void)vitte_sema_analyze_node(
                    context,
                    child->id,
                    child_scope);
            }

            if (node->kind ==
                VITTE_AST_NODE_SPACE_DECL) {
                for (index = 0u;
                     index < node->child_count;
                     ++index) {
                    const vitte_ast_node_t *child;

                    child =
                        vitte_sema_node(
                            context,
                            node->children[index]);
                    if (child != NULL &&
                        child->kind ==
                            VITTE_AST_NODE_BLOCK) {
                        vitte_sema_register_export_aliases(
                            context,
                            child,
                            child_scope);
                    }
                }
            }

            return context->type_void;
        }

        case VITTE_AST_NODE_USE_DECL:
            if ((node->flags &
                 VITTE_AST_FLAG_PUBLIC) != 0u) {
                return context->type_void;
            }
            return vitte_sema_analyze_import(
                context,
                node,
                scope_id);

        case VITTE_AST_NODE_IMPL_DECL:
        {
            vitte_scope_id_t impl_scope;

            impl_scope =
                vitte_scope_create(
                    &context->scopes,
                    scope_id,
                    VITTE_SCOPE_KIND_IMPL,
                    vitte_sema_scope_span_from_parser(
                        node->span),
                    (void *)node);

            if (impl_scope ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
                impl_scope = scope_id;
            }

            if (info != NULL) {
                info->scope_id = impl_scope;
                info->type_id =
                    context->type_void;
            }

            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    impl_scope);
            }

            return context->type_void;
        }

        case VITTE_AST_NODE_BLOCK:
            return vitte_sema_analyze_block(
                context,
                node,
                scope_id);

        case VITTE_AST_NODE_RETURN_STMT:
            return vitte_sema_analyze_return(
                context,
                node,
                scope_id);

        case VITTE_AST_NODE_IF_STMT:
            if (node->child_count != 0u) {
                (void)vitte_sema_analyze_condition(
                    context,
                    node->children[0],
                    scope_id);
            }

            for (index = 1u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    scope_id);
            }

            return context->type_void;

        case VITTE_AST_NODE_WHILE_STMT:
        case VITTE_AST_NODE_FOR_STMT:
        {
            vitte_scope_id_t loop_scope;
            size_t previous_loop_depth;

            loop_scope =
                vitte_scope_create(
                    &context->scopes,
                    scope_id,
                    VITTE_SCOPE_KIND_LOOP,
                    vitte_sema_scope_span_from_parser(
                        node->span),
                    (void *)node);

            if (loop_scope ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
                loop_scope = scope_id;
            }

            previous_loop_depth =
                context->loop_depth;

            ++context->loop_depth;

            if (node->child_count != 0u) {
                (void)vitte_sema_analyze_condition(
                    context,
                    node->children[0],
                    loop_scope);
            }

            for (index = 1u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    loop_scope);
            }

            context->loop_depth =
                previous_loop_depth;

            return context->type_void;
        }

        case VITTE_AST_NODE_LOOP_STMT:
        {
            vitte_scope_id_t loop_scope;
            size_t previous_loop_depth;

            loop_scope =
                vitte_scope_create(
                    &context->scopes,
                    scope_id,
                    VITTE_SCOPE_KIND_LOOP,
                    vitte_sema_scope_span_from_parser(
                        node->span),
                    (void *)node);

            if (loop_scope ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
                loop_scope = scope_id;
            }

            previous_loop_depth =
                context->loop_depth;

            ++context->loop_depth;

            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    loop_scope);
            }

            context->loop_depth =
                previous_loop_depth;

            return context->type_void;
        }

        case VITTE_AST_NODE_BREAK_STMT:
        case VITTE_AST_NODE_CONTINUE_STMT:
            if (context->loop_depth == 0u) {
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    node->kind ==
                            VITTE_AST_NODE_BREAK_STMT
                        ? VITTE_SEMA_DIAGNOSTIC_BREAK_OUTSIDE_LOOP
                        : VITTE_SEMA_DIAGNOSTIC_CONTINUE_OUTSIDE_LOOP,
                    node->span,
                    node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID);
            }

            return context->type_void;

        case VITTE_AST_NODE_MATCH_STMT:
        case VITTE_AST_NODE_MATCH_ARM:
        {
            vitte_scope_kind_t kind;
            vitte_scope_id_t match_scope;

            kind =
                node->kind ==
                        VITTE_AST_NODE_MATCH_STMT
                    ? VITTE_SCOPE_KIND_MATCH
                    : VITTE_SCOPE_KIND_MATCH_ARM;

            match_scope =
                vitte_scope_create(
                    &context->scopes,
                    scope_id,
                    kind,
                    vitte_sema_scope_span_from_parser(
                        node->span),
                    (void *)node);

            if (match_scope ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
                match_scope = scope_id;
            }

            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    match_scope);
            }

            return context->type_void;
        }

        case VITTE_AST_NODE_DEFER_STMT:
        case VITTE_AST_NODE_UNSAFE_BLOCK:
        case VITTE_AST_NODE_ASM_STMT:
            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    scope_id);
            }

            return context->type_void;

        case VITTE_AST_NODE_ASSERT_STMT:
            if (node->child_count != 0u) {
                (void)vitte_sema_analyze_condition(
                    context,
                    node->children[0],
                    scope_id);
            }

            return context->type_void;

        case VITTE_AST_NODE_EXPR_STMT:
            if (node->child_count != 0u) {
                return vitte_sema_analyze_expression(
                    context,
                    node->children[0],
                    scope_id);
            }

            return context->type_void;

        case VITTE_AST_NODE_IDENTIFIER:
        case VITTE_AST_NODE_TYPE_EXPR:
        case VITTE_AST_NODE_POINTER_TYPE:
        case VITTE_AST_NODE_REFERENCE_TYPE:
        case VITTE_AST_NODE_ARRAY_TYPE:
        case VITTE_AST_NODE_INTEGER_LITERAL:
        case VITTE_AST_NODE_FLOAT_LITERAL:
        case VITTE_AST_NODE_STRING_LITERAL:
        case VITTE_AST_NODE_CHARACTER_LITERAL:
        case VITTE_AST_NODE_BOOL_LITERAL:
        case VITTE_AST_NODE_NULL_LITERAL:
        case VITTE_AST_NODE_SELF_EXPR:
        case VITTE_AST_NODE_GROUP_EXPR:
        case VITTE_AST_NODE_IF_EXPR:
        case VITTE_AST_NODE_BLOCK_EXPR:
        case VITTE_AST_NODE_ARRAY_EXPR:
        case VITTE_AST_NODE_FORM_EXPR:
        case VITTE_AST_NODE_FIELD_INIT:
        case VITTE_AST_NODE_UNARY_EXPR:
        case VITTE_AST_NODE_BINARY_EXPR:
        case VITTE_AST_NODE_CAST_EXPR:
        case VITTE_AST_NODE_ASSIGN_EXPR:
        case VITTE_AST_NODE_CALL_EXPR:
        case VITTE_AST_NODE_INDEX_EXPR:
        case VITTE_AST_NODE_MEMBER_EXPR:
        case VITTE_AST_NODE_AWAIT_EXPR:
        case VITTE_AST_NODE_TRY_EXPR:
            return vitte_sema_analyze_expression(
                context,
                node_id,
                scope_id);

        case VITTE_AST_NODE_PATH:
        case VITTE_AST_NODE_PARAMETER:
        case VITTE_AST_NODE_GENERIC_PARAM:
        case VITTE_AST_NODE_WHERE_CLAUSE:
        case VITTE_AST_NODE_WHERE_PREDICATE:
            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    scope_id);
            }

            return context->type_void;

        case VITTE_AST_NODE_INVALID:
        case VITTE_AST_NODE_COUNT:
            break;
    }

    return context->type_error;
}

/* ========================================================================= */
/* Public names                                                              */
/* ========================================================================= */

const char *
vitte_sema_error_name(
    vitte_sema_error_t error)
{
    switch (error) {
        case VITTE_SEMA_ERROR_NONE:
            return "none";
        case VITTE_SEMA_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";
        case VITTE_SEMA_ERROR_INVALID_CONTEXT:
            return "invalid_context";
        case VITTE_SEMA_ERROR_INVALID_STATE:
            return "invalid_state";
        case VITTE_SEMA_ERROR_INVALID_AST:
            return "invalid_ast";
        case VITTE_SEMA_ERROR_INVALID_SCOPE:
            return "invalid_scope";
        case VITTE_SEMA_ERROR_INVALID_TYPE:
            return "invalid_type";
        case VITTE_SEMA_ERROR_TYPE_LIMIT:
            return "type_limit";
        case VITTE_SEMA_ERROR_DIAGNOSTIC_LIMIT:
            return "diagnostic_limit";
        case VITTE_SEMA_ERROR_RECURSION_LIMIT:
            return "recursion_limit";
        case VITTE_SEMA_ERROR_OVERFLOW:
            return "overflow";
        case VITTE_SEMA_ERROR_OUT_OF_MEMORY:
            return "out_of_memory";
        case VITTE_SEMA_ERROR_VALIDATION:
            return "validation";
        case VITTE_SEMA_ERROR_CORRUPTION:
            return "corruption";
        case VITTE_SEMA_ERROR_INTERNAL:
            return "internal";
        case VITTE_SEMA_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_sema_state_name(
    vitte_sema_state_t state)
{
    switch (state) {
        case VITTE_SEMA_STATE_INVALID:
            return "invalid";
        case VITTE_SEMA_STATE_READY:
            return "ready";
        case VITTE_SEMA_STATE_ANALYZING:
            return "analyzing";
        case VITTE_SEMA_STATE_DONE:
            return "done";
        case VITTE_SEMA_STATE_FAILED:
            return "failed";
        case VITTE_SEMA_STATE_DESTROYED:
            return "destroyed";
        case VITTE_SEMA_STATE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_sema_type_kind_name(
    vitte_sema_type_kind_t kind)
{
    switch (kind) {
        case VITTE_SEMA_TYPE_INVALID:
            return "invalid";
        case VITTE_SEMA_TYPE_ERROR:
            return "error";
        case VITTE_SEMA_TYPE_VOID:
            return "void";
        case VITTE_SEMA_TYPE_BOOL:
            return "bool";
        case VITTE_SEMA_TYPE_I64:
            return "i64";
        case VITTE_SEMA_TYPE_U64:
            return "u64";
        case VITTE_SEMA_TYPE_U8:
            return "u8";
        case VITTE_SEMA_TYPE_F64:
            return "f64";
        case VITTE_SEMA_TYPE_CHAR:
            return "char";
        case VITTE_SEMA_TYPE_STRING:
            return "string";
        case VITTE_SEMA_TYPE_NULL:
            return "null";
        case VITTE_SEMA_TYPE_POINTER:
            return "pointer";
        case VITTE_SEMA_TYPE_REFERENCE:
            return "reference";
        case VITTE_SEMA_TYPE_ARRAY:
            return "array";
        case VITTE_SEMA_TYPE_RANGE:
            return "range";
        case VITTE_SEMA_TYPE_FUNCTION:
            return "function";
        case VITTE_SEMA_TYPE_NAMED:
            return "named";
        case VITTE_SEMA_TYPE_DYN:
            return "dyn";
        case VITTE_SEMA_TYPE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_sema_diagnostic_kind_name(
    vitte_sema_diagnostic_kind_t kind)
{
    switch (kind) {
        case VITTE_SEMA_DIAGNOSTIC_NOTE:
            return "note";
        case VITTE_SEMA_DIAGNOSTIC_HELP:
            return "help";
        case VITTE_SEMA_DIAGNOSTIC_WARNING:
            return "warning";
        case VITTE_SEMA_DIAGNOSTIC_ERROR:
            return "error";
        case VITTE_SEMA_DIAGNOSTIC_KIND_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_sema_diagnostic_code_name(
    vitte_sema_diagnostic_code_t code)
{
    switch (code) {
        case VITTE_SEMA_DIAGNOSTIC_NONE:
            return "none";
        case VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME:
            return "unresolved_name";
        case VITTE_SEMA_DIAGNOSTIC_PRIVATE_SYMBOL:
            return "private_symbol";
        case VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION:
            return "duplicate_declaration";
        case VITTE_SEMA_DIAGNOSTIC_SHADOWED_DECLARATION:
            return "shadowed_declaration";
        case VITTE_SEMA_DIAGNOSTIC_UNKNOWN_TYPE:
            return "unknown_type";
        case VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE:
            return "not_a_type";
        case VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH:
            return "type_mismatch";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_UNARY_OPERAND:
            return "invalid_unary_operand";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_BINARY_OPERANDS:
            return "invalid_binary_operands";
        case VITTE_SEMA_DIAGNOSTIC_NOT_ASSIGNABLE:
            return "not_assignable";
        case VITTE_SEMA_DIAGNOSTIC_NOT_CALLABLE:
            return "not_callable";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_COUNT:
            return "invalid_argument_count";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE:
            return "invalid_argument_type";
        case VITTE_SEMA_DIAGNOSTIC_NOT_INDEXABLE:
            return "not_indexable";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_INDEX_TYPE:
            return "invalid_index_type";
        case VITTE_SEMA_DIAGNOSTIC_UNKNOWN_MEMBER:
            return "unknown_member";
        case VITTE_SEMA_DIAGNOSTIC_CONDITION_NOT_BOOL:
            return "condition_not_bool";
        case VITTE_SEMA_DIAGNOSTIC_RETURN_OUTSIDE_PROC:
            return "return_outside_proc";
        case VITTE_SEMA_DIAGNOSTIC_RETURN_TYPE_MISMATCH:
            return "return_type_mismatch";
        case VITTE_SEMA_DIAGNOSTIC_BREAK_OUTSIDE_LOOP:
            return "break_outside_loop";
        case VITTE_SEMA_DIAGNOSTIC_CONTINUE_OUTSIDE_LOOP:
            return "continue_outside_loop";
        case VITTE_SEMA_DIAGNOSTIC_UNREACHABLE_CODE:
            return "unreachable_code";
        case VITTE_SEMA_DIAGNOSTIC_RECURSION_LIMIT:
            return "recursion_limit";
        case VITTE_SEMA_DIAGNOSTIC_CODE_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_sema_init(
    vitte_sema_t *context,
    const vitte_parser_t *parser)
{
    if (context == NULL ||
        parser == NULL) {
        return false;
    }

    memset(
        context,
        0,
        sizeof(*context));

    context->magic =
        VITTE_SEMA_MAGIC;

    context->state =
        VITTE_SEMA_STATE_READY;

    context->last_error =
        VITTE_SEMA_ERROR_NONE;

    context->parser = parser;

    context->max_types =
        VITTE_SEMA_DEFAULT_MAX_TYPES;

    context->max_diagnostics =
        VITTE_SEMA_DEFAULT_MAX_DIAGNOSTICS;

    context->max_recursion_depth =
        VITTE_SEMA_DEFAULT_MAX_RECURSION_DEPTH;

    context->current_return_type =
        VITTE_SEMA_INVALID_TYPE_ID;

    context->current_self_type =
        VITTE_SEMA_INVALID_TYPE_ID;

    context->current_proc_scope =
        VITTE_SCOPE_INVALID_SCOPE_ID;

    context->generation =
        UINT64_C(1);

    if (!vitte_scope_init(
            &context->scopes)) {
        memset(
            context,
            0,
            sizeof(*context));

        return false;
    }

    if (!vitte_sema_create_builtin_types(
            context)) {
        vitte_scope_destroy(
            &context->scopes);

        context->state =
            VITTE_SEMA_STATE_FAILED;

        return false;
    }

    if (!vitte_sema_prepare_node_info(
            context)) {
        vitte_scope_destroy(
            &context->scopes);

        context->state =
            VITTE_SEMA_STATE_FAILED;

        return false;
    }

    return true;
}

bool
vitte_sema_reset(
    vitte_sema_t *context,
    const vitte_parser_t *parser)
{
    uint64_t generation;

    if (!vitte_sema_context_valid_private(
            context) ||
        parser == NULL) {
        return false;
    }

    generation =
        context->generation;

    free(context->types);
    free(context->node_info);
    free(context->diagnostics);

    context->types = NULL;
    context->type_count = 0u;
    context->type_capacity = 0u;

    context->node_info = NULL;
    context->node_info_count = 0u;
    context->node_info_capacity = 0u;

    context->diagnostics = NULL;
    context->diagnostic_count = 0u;
    context->diagnostic_capacity = 0u;

    if (!vitte_scope_reset(
            &context->scopes)) {
        return vitte_sema_fail(
            context,
            VITTE_SEMA_ERROR_INTERNAL);
    }

    memset(
        &context->stats,
        0,
        sizeof(context->stats));

    context->parser = parser;

    context->state =
        VITTE_SEMA_STATE_READY;

    context->last_error =
        VITTE_SEMA_ERROR_NONE;

    context->recursion_depth = 0u;
    context->loop_depth = 0u;

    context->current_return_type =
        VITTE_SEMA_INVALID_TYPE_ID;

    context->current_self_type =
        VITTE_SEMA_INVALID_TYPE_ID;

    context->current_proc_scope =
        VITTE_SCOPE_INVALID_SCOPE_ID;

    context->generation =
        generation == UINT64_MAX
            ? UINT64_MAX
            : generation + UINT64_C(1);

    if (!vitte_sema_create_builtin_types(
            context) ||
        !vitte_sema_prepare_node_info(
            context)) {
        return vitte_sema_fail(
            context,
            context->last_error);
    }

    return true;
}

void
vitte_sema_destroy(
    vitte_sema_t *context)
{
    if (context == NULL) {
        return;
    }

    if (context->magic ==
        VITTE_SEMA_MAGIC) {
        vitte_scope_destroy(
            &context->scopes);

        free(context->types);
        free(context->node_info);
        free(context->diagnostics);
    }

    memset(
        context,
        0,
        sizeof(*context));

    context->magic =
        VITTE_SEMA_DEAD_MAGIC;

    context->state =
        VITTE_SEMA_STATE_DESTROYED;
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_sema_set_limits(
    vitte_sema_t *context,
    size_t max_types,
    size_t max_diagnostics,
    size_t max_recursion_depth)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return false;
    }

    if (context->state !=
        VITTE_SEMA_STATE_READY) {
        return vitte_sema_fail(
            context,
            VITTE_SEMA_ERROR_INVALID_STATE);
    }

    if (max_types == 0u ||
        max_diagnostics == 0u ||
        max_recursion_depth == 0u ||
        max_types <
            context->type_count ||
        max_diagnostics <
            context->diagnostic_count) {
        vitte_sema_set_error(
            context,
            VITTE_SEMA_ERROR_INVALID_ARGUMENT);

        return false;
    }

    context->max_types =
        max_types;

    context->max_diagnostics =
        max_diagnostics;

    context->max_recursion_depth =
        max_recursion_depth;

    context->last_error =
        VITTE_SEMA_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Analysis                                                                  */
/* ========================================================================= */

bool
vitte_sema_run(
    vitte_sema_t *context)
{
    vitte_ast_node_id_t root;
    vitte_scope_id_t root_scope;

    if (!vitte_sema_context_valid_private(
            context)) {
        return false;
    }

    if (context->state !=
        VITTE_SEMA_STATE_READY) {
        return vitte_sema_fail(
            context,
            VITTE_SEMA_ERROR_INVALID_STATE);
    }

    root =
        vitte_parser_root(
            context->parser);

    if (root ==
        VITTE_AST_INVALID_ID) {
        return vitte_sema_fail(
            context,
            VITTE_SEMA_ERROR_INVALID_AST);
    }

    root_scope =
        vitte_scope_root(
            &context->scopes);

    if (root_scope ==
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        return vitte_sema_fail(
            context,
            VITTE_SEMA_ERROR_INVALID_SCOPE);
    }

    context->state =
        VITTE_SEMA_STATE_ANALYZING;

    context->last_error =
        VITTE_SEMA_ERROR_NONE;

    context->stats.runs =
        vitte_sema_u64_add_sat(
            context->stats.runs,
            UINT64_C(1));

    (void)vitte_sema_analyze_node(
        context,
        root,
        root_scope);

    if (context->last_error ==
            VITTE_SEMA_ERROR_OUT_OF_MEMORY ||
        context->last_error ==
            VITTE_SEMA_ERROR_OVERFLOW ||
        context->last_error ==
            VITTE_SEMA_ERROR_TYPE_LIMIT ||
        context->last_error ==
            VITTE_SEMA_ERROR_DIAGNOSTIC_LIMIT ||
        context->last_error ==
            VITTE_SEMA_ERROR_RECURSION_LIMIT) {
        context->stats.failed_runs =
            vitte_sema_u64_add_sat(
                context->stats.failed_runs,
                UINT64_C(1));

        context->state =
            VITTE_SEMA_STATE_FAILED;

        return false;
    }

    if (!vitte_scope_seal(
            &context->scopes)) {
        context->stats.failed_runs =
            vitte_sema_u64_add_sat(
                context->stats.failed_runs,
                UINT64_C(1));

        return vitte_sema_fail(
            context,
            VITTE_SEMA_ERROR_VALIDATION);
    }

    context->state =
        VITTE_SEMA_STATE_DONE;

    context->last_error =
        VITTE_SEMA_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Public semantic queries                                                   */
/* ========================================================================= */

const vitte_sema_type_t *
vitte_sema_get_type(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return NULL;
    }

    return vitte_sema_type_private(
        context,
        type_id);
}

const vitte_sema_node_info_t *
vitte_sema_get_node_info(
    const vitte_sema_t *context,
    vitte_ast_node_id_t node_id)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return NULL;
    }

    return vitte_sema_node_info_private(
        context,
        node_id);
}

vitte_sema_type_id_t
vitte_sema_node_type(
    const vitte_sema_t *context,
    vitte_ast_node_id_t node_id)
{
    const vitte_sema_node_info_t *info;

    if (!vitte_sema_context_valid_private(
            context)) {
        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    info =
        vitte_sema_node_info_private(
            context,
            node_id);

    if (info == NULL) {
        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    return info->type_id;
}

vitte_symbol_id_t
vitte_sema_node_symbol(
    const vitte_sema_t *context,
    vitte_ast_node_id_t node_id)
{
    const vitte_sema_node_info_t *info;

    if (!vitte_sema_context_valid_private(
            context)) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    info =
        vitte_sema_node_info_private(
            context,
            node_id);

    if (info == NULL) {
        return VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

    return info->symbol_id;
}

vitte_scope_id_t
vitte_sema_node_scope(
    const vitte_sema_t *context,
    vitte_ast_node_id_t node_id)
{
    const vitte_sema_node_info_t *info;

    if (!vitte_sema_context_valid_private(
            context)) {
        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    info =
        vitte_sema_node_info_private(
            context,
            node_id);

    if (info == NULL) {
        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    return info->scope_id;
}

bool
vitte_sema_type_assignable(
    const vitte_sema_t *context,
    vitte_sema_type_id_t destination,
    vitte_sema_type_id_t source)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return false;
    }

    return vitte_sema_type_assignable_private(
        context,
        destination,
        source);
}

bool
vitte_sema_type_is_numeric(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return false;
    }

    return vitte_sema_type_is_numeric_private(
        context,
        type_id);
}

bool
vitte_sema_type_is_integer(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return false;
    }

    return vitte_sema_type_is_integer_private(
        context,
        type_id);
}

bool
vitte_sema_type_is_pointer_like(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return false;
    }

    return vitte_sema_type_is_pointer_like_private(
        context,
        type_id);
}

/* ========================================================================= */
/* Diagnostics queries                                                       */
/* ========================================================================= */

size_t
vitte_sema_diagnostic_count(
    const vitte_sema_t *context)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return 0u;
    }

    return context->diagnostic_count;
}

const vitte_sema_diagnostic_t *
vitte_sema_diagnostic_at(
    const vitte_sema_t *context,
    size_t index)
{
    if (!vitte_sema_context_valid_private(
            context) ||
        index >=
            context->diagnostic_count) {
        return NULL;
    }

    return &context->diagnostics[index];
}

bool
vitte_sema_has_errors(
    const vitte_sema_t *context)
{
    size_t index;

    if (!vitte_sema_context_valid_private(
            context)) {
        return true;
    }

    for (index = 0u;
         index < context->diagnostic_count;
         ++index) {
        if (context->diagnostics[index].kind ==
            VITTE_SEMA_DIAGNOSTIC_ERROR) {
            return true;
        }
    }

    return false;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_sema_validate(
    vitte_sema_t *context)
{
    size_t index;

    if (!vitte_sema_context_valid_private(
            context)) {
        return false;
    }

    context->stats.validation_runs =
        vitte_sema_u64_add_sat(
            context->stats.validation_runs,
            UINT64_C(1));

    if (!vitte_scope_is_valid(
            &context->scopes)) {
        goto validation_failure;
    }

    if (context->state ==
            VITTE_SEMA_STATE_DONE &&
        !vitte_scope_validate(
            &context->scopes)) {
        goto validation_failure;
    }

    for (index = 0u;
         index < context->type_count;
         ++index) {
        const vitte_sema_type_t *type;

        type = &context->types[index];

        if (type->id !=
            (vitte_sema_type_id_t)(index + 1u)) {
            goto validation_failure;
        }

        if (type->kind <=
                VITTE_SEMA_TYPE_INVALID ||
            type->kind >=
                VITTE_SEMA_TYPE_COUNT) {
            goto validation_failure;
        }

        if (type->element_type !=
                VITTE_SEMA_INVALID_TYPE_ID &&
            vitte_sema_type_private(
                context,
                type->element_type) == NULL) {
            goto validation_failure;
        }

        if (type->return_type !=
                VITTE_SEMA_INVALID_TYPE_ID &&
            vitte_sema_type_private(
                context,
                type->return_type) == NULL) {
            goto validation_failure;
        }

        if (type->symbol_id !=
                VITTE_SCOPE_INVALID_SYMBOL_ID &&
            vitte_scope_get_symbol(
                &context->scopes,
                type->symbol_id) == NULL) {
            goto validation_failure;
        }
    }

    if (context->node_info_count !=
        context->parser->node_count) {
        goto validation_failure;
    }

    for (index = 0u;
         index < context->node_info_count;
         ++index) {
        const vitte_sema_node_info_t *info;

        info = &context->node_info[index];

        if (info->node_id !=
            (vitte_ast_node_id_t)(index + 1u)) {
            goto validation_failure;
        }

        if (info->type_id !=
                VITTE_SEMA_INVALID_TYPE_ID &&
            vitte_sema_type_private(
                context,
                info->type_id) == NULL) {
            goto validation_failure;
        }

        if (info->symbol_id !=
                VITTE_SCOPE_INVALID_SYMBOL_ID &&
            vitte_scope_get_symbol(
                &context->scopes,
                info->symbol_id) == NULL) {
            goto validation_failure;
        }

        if (info->scope_id !=
                VITTE_SCOPE_INVALID_SCOPE_ID &&
            vitte_scope_get_scope(
                &context->scopes,
                info->scope_id) == NULL) {
            goto validation_failure;
        }
    }

    for (index = 0u;
         index < context->diagnostic_count;
         ++index) {
        const vitte_sema_diagnostic_t *diagnostic;

        diagnostic =
            &context->diagnostics[index];

        if (diagnostic->kind >=
                VITTE_SEMA_DIAGNOSTIC_KIND_COUNT ||
            diagnostic->code >=
                VITTE_SEMA_DIAGNOSTIC_CODE_COUNT) {
            goto validation_failure;
        }
    }

    context->last_error =
        VITTE_SEMA_ERROR_NONE;

    return true;

validation_failure:

    context->stats.validation_failures =
        vitte_sema_u64_add_sat(
            context->stats.validation_failures,
            UINT64_C(1));

    context->last_error =
        VITTE_SEMA_ERROR_VALIDATION;

    return false;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_sema_fingerprint(
    vitte_sema_t *context)
{
    uint64_t hash;
    size_t index;

    if (!vitte_sema_context_valid_private(
            context)) {
        return UINT64_C(0);
    }

    hash =
        VITTE_SEMA_FNV_OFFSET;

    hash =
        vitte_sema_hash_u64(
            hash,
            vitte_scope_semantic_fingerprint(
                &context->scopes));

    hash =
        vitte_sema_hash_u64(
            hash,
            (uint64_t)context->type_count);

    for (index = 0u;
         index < context->type_count;
         ++index) {
        const vitte_sema_type_t *type;

        type = &context->types[index];

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)type->kind);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)type->element_type);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)type->array_length);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)type->symbol_id);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)type->return_type);

        hash =
            vitte_sema_hash_u64(
                hash,
                type->is_mutable
                    ? UINT64_C(1)
                    : UINT64_C(0));
    }

    for (index = 0u;
         index < context->node_info_count;
         ++index) {
        const vitte_sema_node_info_t *info;

        info = &context->node_info[index];

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)info->node_id);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)info->type_id);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)info->symbol_id);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)info->scope_id);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)info->flags);
    }

    for (index = 0u;
         index < context->diagnostic_count;
         ++index) {
        const vitte_sema_diagnostic_t *diagnostic;

        diagnostic =
            &context->diagnostics[index];

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)diagnostic->kind);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)diagnostic->code);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)diagnostic->node_id);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)diagnostic->symbol_id);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)diagnostic->expected_type);

        hash =
            vitte_sema_hash_u64(
                hash,
                (uint64_t)diagnostic->found_type);
    }

    /*
     * Keep the byte-hash primitive part of the fingerprint contract and
     * distinguish the semantic subsystem from structurally similar tables.
     */
    hash =
        vitte_sema_hash_bytes(
            hash,
            (const unsigned char *)"vitte-sema",
            sizeof("vitte-sema") - 1u);

    context->stats.hash_runs =
        vitte_sema_u64_add_sat(
            context->stats.hash_runs,
            UINT64_C(1));

    return hash;
}

/* ========================================================================= */
/* Statistics / state                                                        */
/* ========================================================================= */

vitte_sema_stats_t
vitte_sema_stats(
    const vitte_sema_t *context)
{
    vitte_sema_stats_t stats;

    memset(
        &stats,
        0,
        sizeof(stats));

    if (!vitte_sema_context_valid_private(
            context)) {
        return stats;
    }

    return context->stats;
}

bool
vitte_sema_is_valid(
    const vitte_sema_t *context)
{
    return vitte_sema_context_valid_private(
        context);
}

vitte_sema_error_t
vitte_sema_last_error(
    const vitte_sema_t *context)
{
    if (context == NULL ||
        context->magic !=
            VITTE_SEMA_MAGIC) {
        return VITTE_SEMA_ERROR_INVALID_CONTEXT;
    }

    return context->last_error;
}

uint64_t
vitte_sema_generation(
    const vitte_sema_t *context)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return UINT64_C(0);
    }

    return context->generation;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_sema_translation_unit_anchor(void)
{
    /*
     * Stable symbol for build-system/static-library linkage probes.
     */
}
