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

#include "../constant_fold/constant.h"
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Private constants                                                         */
/* ========================================================================= */

#define VITTE_SEMA_PRIVATE_NO_INDEX SIZE_MAX

static void
vitte_sema_mark_unreachable_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *node);

static bool
vitte_sema_is_unreachable_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *node);

static void
vitte_sema_warn_unreachable_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    const vitte_ast_node_t *cause,
    const char *cause_label,
    const char *help);

static bool
vitte_sema_space_scope_private(
    const vitte_sema_t *context,
    const vitte_symbol_t *symbol,
    const vitte_ast_node_t *declaration,
    vitte_scope_id_t *scope_id);

static void
vitte_sema_help_similar_name(
    vitte_sema_t *context,
    vitte_scope_id_t scope_id,
    vitte_scope_id_t requester,
    const char *name,
    size_t length,
    vitte_sema_diagnostic_code_t code,
    bool type_only);

static bool
vitte_sema_resolve_import_module(
    vitte_sema_t *context,
    const vitte_ast_node_t *path,
    vitte_scope_id_t requester_scope,
    vitte_scope_id_t *module_scope);

static bool
vitte_sema_help_module_member_import(
    vitte_sema_t *context,
    vitte_scope_id_t requester_scope,
    const char *name,
    size_t name_length);

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

static vitte_parser_span_t
vitte_sema_keyword_span_in_node_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *node,
    const char *keyword)
{
    size_t index;
    size_t keyword_length;
    vitte_parser_span_t span;

    memset(&span, 0, sizeof(span));
    if (context == NULL || context->parser == NULL || node == NULL ||
        keyword == NULL || !node->span.valid) {
        return node != NULL ? node->span : span;
    }
    keyword_length = strlen(keyword);
    for (index = 0u; index < context->parser->token_count; ++index) {
        const vitte_token_t *token = &context->parser->tokens[index];
        if (token->span.begin < node->span.begin ||
            token->span.end > node->span.end ||
            token->length != keyword_length || token->lexeme == NULL ||
            memcmp(token->lexeme, keyword, keyword_length) != 0) {
            continue;
        }
        span.file_id = node->span.file_id;
        span.begin = token->span.begin;
        span.end = token->span.end;
        span.line = token->line;
        span.column = token->column;
        span.valid = true;
        return span;
    }
    return node->span;
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
         token->kind != VITTE_TOKEN_KW_NULL &&
         !(token->kind == VITTE_TOKEN_KW_RESERVED &&
           token->lexeme != NULL &&
           ((token->length == 8u &&
             memcmp(token->lexeme, "compiler", 8u) == 0) ||
            (token->length == 10u &&
             memcmp(token->lexeme, "diagnostic", 10u) == 0)))) ||
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

    /* The same resolved type node can be visited by both declaration and
     * body passes. Keep one public diagnostic for one offending name and
     * retain the richer type information when the later pass has it. */
    if (code == VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE) {
        size_t index;
        for (index = 0u; index < context->diagnostic_count; ++index) {
            vitte_sema_diagnostic_t *previous =
                &context->diagnostics[index];
            if (previous->code != code ||
                previous->symbol_id != symbol_id ||
                previous->span.file_id != span.file_id ||
                previous->span.begin != span.begin ||
                previous->span.end != span.end) {
                continue;
            }
            if (previous->expected_type == VITTE_SEMA_INVALID_TYPE_ID &&
                expected_type != VITTE_SEMA_INVALID_TYPE_ID) {
                previous->expected_type = expected_type;
            }
            if (previous->found_type == VITTE_SEMA_INVALID_TYPE_ID &&
                found_type != VITTE_SEMA_INVALID_TYPE_ID) {
                previous->found_type = found_type;
            }
            return index + 1u == context->diagnostic_count;
        }
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
    diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_NONE;
    switch (code) {
        case VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_EXPRESSION;
            break;
        case VITTE_SEMA_DIAGNOSTIC_INVALID_UNARY_OPERAND:
        case VITTE_SEMA_DIAGNOSTIC_INVALID_BINARY_OPERANDS:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_OPERANDS;
            break;
        case VITTE_SEMA_DIAGNOSTIC_INVALID_CAST:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_CAST;
            break;
        case VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_TYPE;
            break;
        case VITTE_SEMA_DIAGNOSTIC_NOT_A_VALUE:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_VALUE;
            break;
        case VITTE_SEMA_DIAGNOSTIC_CANNOT_INFER:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_INFERENCE;
            break;
        case VITTE_SEMA_DIAGNOSTIC_GENERIC_BOUND_UNSATISFIED:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_BOUND;
            break;
        case VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_COUNT:
        case VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_KIND:
        case VITTE_SEMA_DIAGNOSTIC_GENERIC_INFERENCE_CONFLICT:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_GENERIC_ARGUMENT;
            break;
        case VITTE_SEMA_DIAGNOSTIC_INVALID_WHERE_PREDICATE:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_BOUND;
            break;
        case VITTE_SEMA_DIAGNOSTIC_MISSING_TRAIT_MEMBER:
        case VITTE_SEMA_DIAGNOSTIC_INVALID_TRAIT_MEMBER:
        case VITTE_SEMA_DIAGNOSTIC_INVALID_IMPL_TARGET:
        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_METHOD:
        case VITTE_SEMA_DIAGNOSTIC_TRAIT_NOT_IMPLEMENTED:
        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_IMPL:
        case VITTE_SEMA_DIAGNOSTIC_CONFLICTING_IMPL:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_TRAIT_MEMBER;
            break;
        case VITTE_SEMA_DIAGNOSTIC_INCOMPLETE_TYPE:
        case VITTE_SEMA_DIAGNOSTIC_RECURSIVE_TYPE:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_TYPE;
            break;
        case VITTE_SEMA_DIAGNOSTIC_NOT_ASSIGNABLE:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_ASSIGNMENT;
            break;
        case VITTE_SEMA_DIAGNOSTIC_NOT_CALLABLE:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_CALL;
            break;
        case VITTE_SEMA_DIAGNOSTIC_NOT_INDEXABLE:
        case VITTE_SEMA_DIAGNOSTIC_INVALID_INDEX_TYPE:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_INDEX;
            break;
        case VITTE_SEMA_DIAGNOSTIC_UNKNOWN_MEMBER:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_MEMBER;
            break;
        case VITTE_SEMA_DIAGNOSTIC_CONDITION_NOT_BOOL:
            diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_CONDITION;
            break;
        default:
            break;
    }
    diagnostic->context_index = SIZE_MAX;
    diagnostic->context_symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;

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

static void
vitte_sema_set_last_type_context(
    vitte_sema_t *context,
    vitte_sema_type_context_kind_t kind,
    size_t index,
    vitte_symbol_id_t symbol_id)
{
    vitte_sema_diagnostic_t *diagnostic;

    if (context == NULL || context->diagnostic_count == 0u ||
        kind <= VITTE_SEMA_TYPE_CONTEXT_NONE ||
        kind >= VITTE_SEMA_TYPE_CONTEXT_COUNT) {
        return;
    }
    diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
    diagnostic->type_context = kind;
    diagnostic->context_index = index;
    diagnostic->context_symbol_id = symbol_id;
}

static void
vitte_sema_set_last_related_location(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    const char *label)
{
    vitte_sema_diagnostic_t *diagnostic;

    if (context == NULL || node == NULL || label == NULL ||
        context->diagnostic_count == 0u) {
        return;
    }
    diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
    diagnostic->related_span = node->span;
    (void)snprintf(diagnostic->related_label,
        sizeof(diagnostic->related_label), "%s", label);
}

static bool
vitte_sema_defer_control_error(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    const char *help)
{
    const vitte_ast_node_t *defer_node;
    if (context == NULL || node == NULL ||
        context->current_defer_node_id == VITTE_AST_INVALID_ID ||
        context->current_defer_node_id == 0u) {
        return false;
    }
    if (!vitte_sema_add_diagnostic(context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_INVALID_DEFER_CONTROL_FLOW,
            node->span, node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID)) {
        return true;
    }
    (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
        sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
        "%s", help != NULL
            ? help
            : "Deferred work must complete within its enclosing scope and cannot transfer control out of it.");
    defer_node = vitte_sema_node(context, context->current_defer_node_id);
    vitte_sema_set_last_related_location(
        context, defer_node, "deferred action is registered here");
    return true;
}

static void
vitte_sema_set_last_related_span_private(
    vitte_sema_t *context,
    vitte_parser_span_t span,
    const char *label)
{
    vitte_sema_diagnostic_t *diagnostic;
    if (context == NULL || context->diagnostic_count == 0u ||
        !span.valid || label == NULL) {
        return;
    }
    diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
    diagnostic->related_span = span;
    (void)snprintf(diagnostic->related_label,
        sizeof(diagnostic->related_label), "%s", label);
}

static const vitte_ast_node_t *
vitte_sema_nearest_control_context_private(
    const vitte_sema_t *context,
    vitte_scope_id_t scope_id)
{
    size_t depth = 0u;

    while (context != NULL &&
           scope_id != VITTE_SCOPE_INVALID_SCOPE_ID &&
           depth++ < 128u) {
        const vitte_scope_entry_t *scope = vitte_scope_get_scope(
            &context->scopes, scope_id);
        const vitte_ast_node_t *owner;
        bool is_procedure;
        bool is_test;

        if (scope == NULL) {
            break;
        }
        owner = (const vitte_ast_node_t *)scope->payload;
        is_procedure = scope->kind == VITTE_SCOPE_KIND_PROC &&
            owner != NULL &&
            (owner->kind == VITTE_AST_NODE_PROC_DECL ||
             owner->kind == VITTE_AST_NODE_EXTERN_PROC_DECL);
        is_test = scope->kind == VITTE_SCOPE_KIND_TEST &&
            owner != NULL && owner->kind == VITTE_AST_NODE_TEST_DECL;
        if (is_procedure || is_test) {
            return owner;
        }
        scope_id = scope->parent_id;
    }
    return NULL;
}

static void
vitte_sema_set_nearest_control_context_private(
    vitte_sema_t *context,
    vitte_scope_id_t scope_id,
    const char *procedure_label,
    const char *test_label)
{
    const vitte_ast_node_t *owner =
        vitte_sema_nearest_control_context_private(
            context, scope_id);
    if (owner != NULL) {
        vitte_sema_set_last_related_location(
            context, owner,
            owner->kind == VITTE_AST_NODE_TEST_DECL
                ? test_label : procedure_label);
    }
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

static void
vitte_sema_release_types_private(
    vitte_sema_t *context)
{
    size_t index;

    if (context == NULL || context->types == NULL) {
        return;
    }

    for (index = 0u; index < context->type_count; ++index) {
        free(context->types[index].generic_arguments);
        context->types[index].generic_arguments = NULL;
        context->types[index].generic_argument_count = 0u;
        free(context->types[index].function_parameter_types);
        context->types[index].function_parameter_types = NULL;
        context->types[index].function_parameter_count = 0u;
    }
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

static vitte_sema_type_id_t
vitte_sema_create_function_type_private(
    vitte_sema_t *context,
    const vitte_sema_type_id_t *parameter_types,
    size_t parameter_count,
    vitte_sema_type_id_t return_type)
{
    size_t index;
    size_t required;
    size_t parameter_bytes;
    vitte_sema_type_id_t *owned_parameters;
    vitte_sema_type_t *type;

    if (parameter_count != 0u &&
        (parameter_types == NULL ||
         !vitte_sema_size_mul(
             parameter_count,
             sizeof(*owned_parameters),
             &parameter_bytes))) {
        context->last_error = VITTE_SEMA_ERROR_OVERFLOW;
        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    owned_parameters = NULL;
    if (parameter_count != 0u) {
        owned_parameters = (vitte_sema_type_id_t *)
            vitte_sema_reallocate(
                context,
                NULL,
                parameter_count,
                sizeof(*owned_parameters));
        if (owned_parameters == NULL) {
            return VITTE_SEMA_INVALID_TYPE_ID;
        }
        memcpy(owned_parameters, parameter_types, parameter_bytes);
    }

    if (context->type_count >= context->max_types ||
        !vitte_sema_size_add(context->type_count, 1u, &required) ||
        !vitte_sema_reserve_types(context, required)) {
        free(owned_parameters);
        if (context->last_error == VITTE_SEMA_ERROR_NONE) {
            context->last_error = VITTE_SEMA_ERROR_TYPE_LIMIT;
        }
        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    index = context->type_count;
    if ((uint64_t)index >= UINT64_MAX) {
        free(owned_parameters);
        context->last_error = VITTE_SEMA_ERROR_OVERFLOW;
        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    type = &context->types[index];
    memset(type, 0, sizeof(*type));
    type->id = (vitte_sema_type_id_t)(index + 1u);
    type->kind = VITTE_SEMA_TYPE_FUNCTION;
    type->element_type = VITTE_SEMA_INVALID_TYPE_ID;
    type->symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
    type->return_type = return_type;
    type->function_parameter_types = owned_parameters;
    type->function_parameter_count = parameter_count;

    ++context->type_count;
    context->stats.types_created = vitte_sema_u64_add_sat(
        context->stats.types_created, UINT64_C(1));
    return type->id;
}

static bool
vitte_sema_type_identical_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t left,
    vitte_sema_type_id_t right);

static vitte_sema_type_id_t
vitte_sema_analyze_type_node(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id,
    vitte_scope_id_t scope_id);

/*
 * Generic nominal types need a descriptor beyond the base symbol identity.
 * Keep the normal constructor small for the common case and use this helper
 * only when a type expression supplies concrete arguments.  Arguments are
 * copied into the semantic context so callers may use temporary storage.
 */
static bool
vitte_sema_type_arguments_equal_private(
    const vitte_sema_t *context,
    const vitte_sema_type_t *left,
    const vitte_sema_type_id_t *right,
    size_t right_count)
{
    size_t index;

    if (left == NULL ||
        left->generic_argument_count != right_count ||
        (right_count != 0u && right == NULL)) {
        return false;
    }

    for (index = 0u; index < right_count; ++index) {
        if (!vitte_sema_type_identical_private(
                context,
                left->generic_arguments[index],
                right[index])) {
            return false;
        }
    }

    return true;
}

static vitte_sema_type_id_t
vitte_sema_create_named_instance_private(
    vitte_sema_t *context,
    vitte_symbol_id_t symbol_id,
    const vitte_sema_type_id_t *arguments,
    size_t argument_count)
{
    size_t index;
    size_t required;
    size_t argument_bytes;
    vitte_sema_type_t *type;
    vitte_sema_type_id_t *owned_arguments;

    if (argument_count == 0u) {
        return vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_NAMED,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            symbol_id,
            false);
    }

    if (arguments == NULL ||
        !vitte_sema_size_mul(
            argument_count,
            sizeof(*owned_arguments),
            &argument_bytes)) {
        context->last_error = VITTE_SEMA_ERROR_OVERFLOW;
        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    for (index = 0u; index < context->type_count; ++index) {
        type = &context->types[index];
        if (type->kind == VITTE_SEMA_TYPE_NAMED &&
            type->symbol_id == symbol_id &&
            vitte_sema_type_arguments_equal_private(
                context,
                type,
                arguments,
                argument_count)) {
            context->stats.type_intern_hits =
                vitte_sema_u64_add_sat(
                    context->stats.type_intern_hits,
                    UINT64_C(1));
            return type->id;
        }
    }

    if (context->type_count >= context->max_types ||
        !vitte_sema_size_add(
            context->type_count,
            1u,
            &required) ||
        !vitte_sema_reserve_types(context, required)) {
        if (context->last_error == VITTE_SEMA_ERROR_NONE) {
            context->last_error = VITTE_SEMA_ERROR_TYPE_LIMIT;
        }
        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    owned_arguments =
        (vitte_sema_type_id_t *)vitte_sema_reallocate(
            context,
            NULL,
            argument_count,
            sizeof(*owned_arguments));
    if (owned_arguments == NULL) {
        return VITTE_SEMA_INVALID_TYPE_ID;
    }
    memcpy(
        owned_arguments,
        arguments,
        argument_bytes);

    index = context->type_count;
    if ((uint64_t)index >= UINT64_MAX) {
        free(owned_arguments);
        context->last_error = VITTE_SEMA_ERROR_OVERFLOW;
        return VITTE_SEMA_INVALID_TYPE_ID;
    }

    type = &context->types[index];
    memset(type, 0, sizeof(*type));
    type->id = (vitte_sema_type_id_t)(index + 1u);
    type->kind = VITTE_SEMA_TYPE_NAMED;
    type->element_type = VITTE_SEMA_INVALID_TYPE_ID;
    type->symbol_id = symbol_id;
    type->generic_arguments = owned_arguments;
    type->generic_argument_count = argument_count;

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

    context->type_f32 =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_F32,
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

    context->type_never =
        vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_NEVER,
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
        context->type_f32 ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_f64 ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_char ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_string ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_null ==
            VITTE_SEMA_INVALID_TYPE_ID ||
        context->type_never ==
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

static vitte_sema_type_id_t
vitte_sema_builtin_type_from_name(
    const vitte_sema_t *context,
    const char *name,
    size_t name_length);

/* `type Name = Target` is a transparent alias.  Imported aliases can be
 * referenced before their declaration has been revisited in the current
 * analysis walk, so follow the already analyzed target node rather than
 * treating the alias's predeclared nominal placeholder as a distinct type. */
static vitte_sema_type_id_t
vitte_sema_resolve_alias_type_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    size_t depth;

    for (depth = 0u; depth < 64u; ++depth) {
        const vitte_sema_type_t *type =
            vitte_sema_type_private(context, type_id);
        const vitte_symbol_t *symbol;
        const vitte_ast_node_t *declaration;
        const vitte_ast_node_t *target;
        const vitte_sema_node_info_t *target_info;
        size_t child_index;

        if (type == NULL || type->kind != VITTE_SEMA_TYPE_NAMED ||
            type->symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
            break;
        }
        symbol = vitte_scope_get_symbol(&context->scopes, type->symbol_id);
        declaration = symbol != NULL && symbol->payload != NULL
            ? (const vitte_ast_node_t *)symbol->payload
            : NULL;
        if (declaration == NULL ||
            declaration->kind != VITTE_AST_NODE_TYPE_DECL ||
            (declaration->flags & VITTE_AST_FLAG_TYPE_ALIAS) == 0u ||
            declaration->child_count < 2u) {
            break;
        }

        /* Generic aliases need substitution for each concrete application;
         * their template target's cached type is not a concrete alias type. */
        for (child_index = 1u;
             child_index + 1u < declaration->child_count;
             ++child_index) {
            const vitte_ast_node_t *parameter = vitte_sema_node(
                context, declaration->children[child_index]);
            if (parameter != NULL &&
                parameter->kind == VITTE_AST_NODE_GENERIC_PARAM) {
                return type_id;
            }
        }

        target = vitte_sema_node(
            context,
            declaration->children[declaration->child_count - 1u]);
        target_info = target != NULL
            ? vitte_sema_node_info_private(context, target->id)
            : NULL;
        if (target_info != NULL &&
            target_info->type_id != VITTE_SEMA_INVALID_TYPE_ID &&
            target_info->type_id != type_id) {
            type_id = target_info->type_id;
            continue;
        }

        /* A module can import an alias before the defining module's alias
         * target has been semantically visited. Resolve the common primitive
         * alias form directly from its syntax so IDs and pointer-sized
         * integer aliases retain their underlying scalar behavior. */
        if (target != NULL && target->kind == VITTE_AST_NODE_TYPE_EXPR &&
            target->child_count == 1u) {
            const vitte_ast_node_t *base = vitte_sema_node(
                context, target->children[0]);
            const char *base_name;
            size_t base_name_length;
            if (vitte_sema_token_name(
                    vitte_sema_node_token(context, base),
                    &base_name,
                    &base_name_length)) {
                vitte_sema_type_id_t builtin_type =
                    vitte_sema_builtin_type_from_name(
                        context, base_name, base_name_length);
                if (builtin_type != VITTE_SEMA_INVALID_TYPE_ID) {
                    type_id = builtin_type;
                    continue;
                }
            }
        }
        break;
    }
    return type_id;
}

static bool
vitte_sema_type_is_integer_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    const vitte_sema_type_t *type;

    type_id = vitte_sema_resolve_alias_type_private(context, type_id);
    type = vitte_sema_type_private(context, type_id);

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

    type_id = vitte_sema_resolve_alias_type_private(context, type_id);
    type = vitte_sema_type_private(context, type_id);

    return type != NULL &&
           (type->kind == VITTE_SEMA_TYPE_F32 ||
            type->kind == VITTE_SEMA_TYPE_F64);
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
vitte_sema_type_is_enum_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    const vitte_sema_type_t *type;
    const vitte_symbol_t *symbol;
    const vitte_ast_node_t *declaration;

    type_id = vitte_sema_resolve_alias_type_private(context, type_id);
    type = vitte_sema_type_private(context, type_id);
    if (type == NULL || type->kind != VITTE_SEMA_TYPE_NAMED) {
        return false;
    }

    symbol = vitte_scope_get_symbol(&context->scopes, type->symbol_id);
    declaration = symbol != NULL
        ? (const vitte_ast_node_t *)symbol->payload
        : NULL;
    return declaration != NULL &&
           declaration->kind == VITTE_AST_NODE_PICK_DECL;
}

static bool
vitte_sema_type_is_string_like_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    type_id = vitte_sema_resolve_alias_type_private(context, type_id);
    const vitte_sema_type_t *type =
        vitte_sema_type_private(context, type_id);

    return type_id == context->type_string ||
           (type != NULL &&
            (type->kind == VITTE_SEMA_TYPE_POINTER ||
             type->kind == VITTE_SEMA_TYPE_REFERENCE ||
             type->kind == VITTE_SEMA_TYPE_OPTIONAL) &&
            type->element_type == context->type_string);
}

static bool
vitte_sema_type_is_bool_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    type_id = vitte_sema_resolve_alias_type_private(context, type_id);
    return type_id ==
           context->type_bool;
}

static bool
vitte_sema_type_is_error_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    type_id = vitte_sema_resolve_alias_type_private(context, type_id);
    return type_id ==
           context->type_error;
}

static bool
vitte_sema_type_is_pointer_like_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id)
{
    const vitte_sema_type_t *type;

    type_id = vitte_sema_resolve_alias_type_private(context, type_id);
    type = vitte_sema_type_private(context, type_id);

    if (type == NULL) {
        return false;
    }

    return type->kind ==
               VITTE_SEMA_TYPE_POINTER ||
           type->kind ==
               VITTE_SEMA_TYPE_REFERENCE ||
           type->kind ==
               VITTE_SEMA_TYPE_OPTIONAL;
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
            vitte_sema_resolve_alias_type_private(context, type_id));
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
    if (left_type->symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID &&
        left_type->symbol_id == right_type->symbol_id) {
        return true;
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

    left = vitte_sema_resolve_alias_type_private(context, left);
    right = vitte_sema_resolve_alias_type_private(context, right);

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
        left_type->kind != right_type->kind) {
        return false;
    }

    if (left_type->kind == VITTE_SEMA_TYPE_NAMED) {
        if (left_type->generic_argument_count !=
                right_type->generic_argument_count ||
            !vitte_sema_type_arguments_equal_private(
                context,
                left_type,
                right_type->generic_arguments,
                right_type->generic_argument_count)) {
            return false;
        }
        return vitte_sema_type_same_nominal_private(
            context,
            left,
            right);
    }

    if (vitte_sema_type_same_nominal_private(
            context,
            left,
            right)) {
        return true;
    }

    if (left_type->kind == VITTE_SEMA_TYPE_FUNCTION) {
        size_t index;

        /* Named procedure declarations retain nominal identity. Structural
         * comparison applies to anonymous callback signatures. */
        if (left_type->symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID ||
            right_type->symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID) {
            return false;
        }
        if (left_type->function_parameter_count !=
                right_type->function_parameter_count ||
            !vitte_sema_type_identical_private(
                context,
                left_type->return_type,
                right_type->return_type)) {
            return false;
        }
        for (index = 0u;
             index < left_type->function_parameter_count;
             ++index) {
            if (!vitte_sema_type_identical_private(
                    context,
                    left_type->function_parameter_types[index],
                    right_type->function_parameter_types[index])) {
                return false;
            }
        }
        return true;
    }

    switch (left_type->kind) {
        case VITTE_SEMA_TYPE_REFERENCE:
        case VITTE_SEMA_TYPE_POINTER:
        case VITTE_SEMA_TYPE_SLICE:
            return left_type->is_mutable ==
                       right_type->is_mutable &&
                   vitte_sema_type_identical_private(
                       context,
                       left_type->element_type,
                       right_type->element_type);

        case VITTE_SEMA_TYPE_OPTIONAL:
            return vitte_sema_type_identical_private(
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

        case VITTE_SEMA_TYPE_GENERIC_PARAMETER:
            return left_type->symbol_id == right_type->symbol_id;

        case VITTE_SEMA_TYPE_TASK:
            return vitte_sema_type_identical_private(
                context,
                left_type->element_type,
                right_type->element_type);

        default:
            return false;
    }
}

static bool
vitte_sema_function_signature_compatible_private(
    const vitte_sema_t *context,
    const vitte_sema_type_t *destination,
    const vitte_sema_type_t *source)
{
    size_t index;

    if (context == NULL || destination == NULL || source == NULL ||
        destination->kind != VITTE_SEMA_TYPE_FUNCTION ||
        source->kind != VITTE_SEMA_TYPE_FUNCTION ||
        destination->function_parameter_count !=
            source->function_parameter_count ||
        !vitte_sema_type_identical_private(
            context,
            destination->return_type,
            source->return_type)) {
        return false;
    }
    for (index = 0u;
         index < destination->function_parameter_count;
         ++index) {
        if (!vitte_sema_type_identical_private(
                context,
                destination->function_parameter_types[index],
                source->function_parameter_types[index])) {
            return false;
        }
    }
    return true;
}

static vitte_sema_type_conversion_kind_t
vitte_sema_type_conversion_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t destination,
    vitte_sema_type_id_t source)
{
    const vitte_sema_type_t *destination_type;
    const vitte_sema_type_t *source_type;

    destination = vitte_sema_resolve_alias_type_private(
        context, destination);
    source = vitte_sema_resolve_alias_type_private(context, source);

    if (destination ==
        source) {
        return VITTE_SEMA_TYPE_CONVERSION_EXACT;
    }

    if (vitte_sema_type_is_error_private(
            context,
            destination) ||
        vitte_sema_type_is_error_private(
            context,
            source)) {
        return VITTE_SEMA_TYPE_CONVERSION_SAFE;
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
        return VITTE_SEMA_TYPE_CONVERSION_INCOMPATIBLE;
    }

    /* `never` is the bottom type: a computation that cannot return can be
     * used wherever a value is expected. */
    if (source_type->kind == VITTE_SEMA_TYPE_NEVER) {
        return VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }

    /* A named procedure value may be passed where an anonymous structural
     * callback signature is expected, including through a type alias. */
    if (vitte_sema_function_signature_compatible_private(
            context, destination_type, source_type)) {
        return VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }

    if (destination_type->kind == VITTE_SEMA_TYPE_OPTIONAL) {
        if (source_type->kind == VITTE_SEMA_TYPE_NULL) {
            return VITTE_SEMA_TYPE_CONVERSION_SAFE;
        }
        if (source_type->kind == VITTE_SEMA_TYPE_OPTIONAL) {
            return vitte_sema_type_conversion_private(
                context,
                destination_type->element_type,
                source_type->element_type);
        }
        {
            const vitte_sema_type_t *optional_element =
                vitte_sema_type_private(
                    context,
                    destination_type->element_type);

            /* A function item can initialize its nullable callback alias;
             * both use the same function-pointer representation in C. */
            if (optional_element != NULL &&
                optional_element->kind == VITTE_SEMA_TYPE_FUNCTION &&
                source_type->kind == VITTE_SEMA_TYPE_FUNCTION &&
                optional_element->function_parameter_count ==
                    source_type->function_parameter_count &&
                vitte_sema_type_identical_private(
                    context,
                    optional_element->return_type,
                    source_type->return_type)) {
                size_t parameter_index;

                for (parameter_index = 0u;
                     parameter_index <
                         optional_element->function_parameter_count;
                     ++parameter_index) {
                    if (!vitte_sema_type_identical_private(
                            context,
                            optional_element->function_parameter_types[
                                parameter_index],
                            source_type->function_parameter_types[
                                parameter_index])) {
                        break;
                    }
                }
                if (parameter_index ==
                    optional_element->function_parameter_count) {
                    return VITTE_SEMA_TYPE_CONVERSION_SAFE;
                }
            }
        }
        return vitte_sema_type_conversion_private(
            context,
            destination_type->element_type,
            source);
    }

    /* Generic parameters remain invariant until instantiation supplies a
       concrete substitution. */
    if (destination_type->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER ||
        source_type->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
        return VITTE_SEMA_TYPE_CONVERSION_INCOMPATIBLE;
    }

    if (vitte_sema_type_identical_private(
            context,
            destination,
            source)) {
        return VITTE_SEMA_TYPE_CONVERSION_EXACT;
    }

    /* A procedure item decays to a compatible pointer-to-procedure value. */
    if (destination_type->kind == VITTE_SEMA_TYPE_POINTER &&
        source_type->kind == VITTE_SEMA_TYPE_FUNCTION &&
        destination_type->element_type != VITTE_SEMA_INVALID_TYPE_ID) {
        const vitte_sema_type_t *signature = vitte_sema_type_private(
            context, destination_type->element_type);
        size_t parameter_index;

        if (signature != NULL &&
            signature->kind == VITTE_SEMA_TYPE_FUNCTION &&
            signature->function_parameter_count ==
                source_type->function_parameter_count &&
            vitte_sema_type_identical_private(
                context, signature->return_type, source_type->return_type)) {
            for (parameter_index = 0u;
                 parameter_index < signature->function_parameter_count;
                 ++parameter_index) {
                if (!vitte_sema_type_identical_private(
                        context,
                        signature->function_parameter_types[parameter_index],
                        source_type->function_parameter_types[parameter_index])) {
                    break;
                }
            }
            if (parameter_index == signature->function_parameter_count) {
                return VITTE_SEMA_TYPE_CONVERSION_SAFE;
            }
        }
    }

    if (destination_type->kind ==
            VITTE_SEMA_TYPE_ARRAY &&
        source_type->kind ==
            VITTE_SEMA_TYPE_ARRAY) {
        if (destination_type->array_length != 0u &&
            destination_type->array_length !=
                source_type->array_length) {
            return VITTE_SEMA_TYPE_CONVERSION_INCOMPATIBLE;
        }

        return vitte_sema_type_conversion_private(
            context,
            destination_type->element_type,
            source_type->element_type);
    }

    /* A fixed array can be viewed as an immutable slice without copying. */
    if (destination_type->kind == VITTE_SEMA_TYPE_SLICE &&
        source_type->kind == VITTE_SEMA_TYPE_ARRAY &&
        !destination_type->is_mutable) {
        vitte_sema_type_conversion_kind_t element_conversion =
            vitte_sema_type_conversion_private(
            context,
            destination_type->element_type,
            source_type->element_type);
        if (element_conversion == VITTE_SEMA_TYPE_CONVERSION_EXACT ||
            element_conversion == VITTE_SEMA_TYPE_CONVERSION_SAFE) {
            return VITTE_SEMA_TYPE_CONVERSION_SAFE;
        }
        if (element_conversion == VITTE_SEMA_TYPE_CONVERSION_LOSSY) {
            return VITTE_SEMA_TYPE_CONVERSION_LOSSY;
        }
    }

    /* Borrowing a fixed array also permits the usual array-to-slice view. */
    if ((destination_type->kind == VITTE_SEMA_TYPE_REFERENCE ||
         destination_type->kind == VITTE_SEMA_TYPE_POINTER) &&
        (source_type->kind == VITTE_SEMA_TYPE_REFERENCE ||
         source_type->kind == VITTE_SEMA_TYPE_POINTER)) {
        const vitte_sema_type_t *destination_element =
            vitte_sema_type_private(context, destination_type->element_type);
        const vitte_sema_type_t *source_element =
            vitte_sema_type_private(context, source_type->element_type);
        if (destination_element != NULL && source_element != NULL &&
            destination_element->kind == VITTE_SEMA_TYPE_SLICE &&
            source_element->kind == VITTE_SEMA_TYPE_ARRAY &&
            !destination_element->is_mutable &&
            (!destination_type->is_mutable || source_type->is_mutable) &&
            vitte_sema_type_identical_private(
                context,
                destination_element->element_type,
                source_element->element_type)) {
            return VITTE_SEMA_TYPE_CONVERSION_SAFE;
        }
    }

    if (destination_type->kind == VITTE_SEMA_TYPE_SLICE &&
        source_type->kind == VITTE_SEMA_TYPE_SLICE &&
        (destination_type->is_mutable == source_type->is_mutable ||
         (!destination_type->is_mutable && source_type->is_mutable)) &&
        vitte_sema_type_identical_private(
            context,
            destination_type->element_type,
            source_type->element_type)) {
        return destination_type->is_mutable == source_type->is_mutable
            ? VITTE_SEMA_TYPE_CONVERSION_EXACT
            : VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }

    /*
     * Numeric conversion policy. Small unsigned integers and characters
     * widen without loss. Conversions from i64/u64 to f64 or across signed
     * and unsigned 64-bit domains can lose precision or range and therefore
     * remain explicit-only.
     */
    if (source_type->kind == VITTE_SEMA_TYPE_U8 ||
        source_type->kind == VITTE_SEMA_TYPE_CHAR) {
        if (destination_type->kind == VITTE_SEMA_TYPE_I64 ||
            destination_type->kind == VITTE_SEMA_TYPE_U64 ||
            destination_type->kind == VITTE_SEMA_TYPE_F32 ||
            destination_type->kind == VITTE_SEMA_TYPE_F64) {
            return VITTE_SEMA_TYPE_CONVERSION_SAFE;
        }
    }

    if (source_type->kind == VITTE_SEMA_TYPE_F32 &&
        destination_type->kind == VITTE_SEMA_TYPE_F64) {
        return VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }

    if (source_type->kind == VITTE_SEMA_TYPE_F64 &&
        destination_type->kind == VITTE_SEMA_TYPE_F32) {
        return VITTE_SEMA_TYPE_CONVERSION_LOSSY;
    }

    if ((source_type->kind == VITTE_SEMA_TYPE_I64 ||
         source_type->kind == VITTE_SEMA_TYPE_U64) &&
        destination_type->kind == VITTE_SEMA_TYPE_F32) {
        return VITTE_SEMA_TYPE_CONVERSION_LOSSY;
    }

    if ((source_type->kind == VITTE_SEMA_TYPE_I64 ||
         source_type->kind == VITTE_SEMA_TYPE_U64) &&
        destination_type->kind == VITTE_SEMA_TYPE_F64) {
        return VITTE_SEMA_TYPE_CONVERSION_LOSSY;
    }

    if ((source_type->kind == VITTE_SEMA_TYPE_I64 &&
         destination_type->kind == VITTE_SEMA_TYPE_U64) ||
        (source_type->kind == VITTE_SEMA_TYPE_U64 &&
         destination_type->kind == VITTE_SEMA_TYPE_I64)) {
        return VITTE_SEMA_TYPE_CONVERSION_LOSSY;
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
        return destination_type->is_mutable ==
                source_type->is_mutable
            ? VITTE_SEMA_TYPE_CONVERSION_EXACT
            : VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }

    /*
     * Address-of expressions are represented as references by the semantic
     * layer, while nullable `*T` values are represented as pointers. Both
     * lower to C pointers; allow an address reference to initialize a
     * compatible pointer value.
     */
    if (destination_type->kind ==
            VITTE_SEMA_TYPE_POINTER &&
        source_type->kind ==
            VITTE_SEMA_TYPE_REFERENCE &&
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
        return VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }

    /*
     * null -> pointer/reference.
     */
    if ((destination_type->kind ==
             VITTE_SEMA_TYPE_POINTER ||
         destination_type->kind ==
             VITTE_SEMA_TYPE_REFERENCE ||
         destination_type->kind == VITTE_SEMA_TYPE_OPTIONAL) &&
        source_type->kind ==
            VITTE_SEMA_TYPE_NULL) {
        return VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }

    /* Borrowed Vitte strings and nullable string views share the same C
     * representation in the bootstrap backend. */
    if (destination_type->kind == VITTE_SEMA_TYPE_STRING &&
        (source_type->kind == VITTE_SEMA_TYPE_POINTER ||
         source_type->kind == VITTE_SEMA_TYPE_REFERENCE) &&
        source_type->element_type == context->type_string) {
        return VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }
    if ((destination_type->kind == VITTE_SEMA_TYPE_POINTER ||
         destination_type->kind == VITTE_SEMA_TYPE_REFERENCE) &&
        destination_type->element_type == context->type_string &&
        source_type->kind == VITTE_SEMA_TYPE_STRING &&
        !destination_type->is_mutable) {
        return VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }

    if (destination_type->kind ==
            VITTE_SEMA_TYPE_POINTER &&
        destination_type->element_type ==
            context->type_u8 &&
        !destination_type->is_mutable &&
        source_type->kind ==
            VITTE_SEMA_TYPE_STRING) {
        return VITTE_SEMA_TYPE_CONVERSION_SAFE;
    }

    return VITTE_SEMA_TYPE_CONVERSION_INCOMPATIBLE;
}

static bool
vitte_sema_type_assignable_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t destination,
    vitte_sema_type_id_t source)
{
    const vitte_sema_type_conversion_kind_t conversion =
        vitte_sema_type_conversion_private(
            context,
            destination,
            source);

    return conversion == VITTE_SEMA_TYPE_CONVERSION_EXACT ||
           conversion == VITTE_SEMA_TYPE_CONVERSION_SAFE;
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
    vitte_sema_type_id_t value_type = expected_type;

    expected_type = vitte_sema_resolve_alias_type_private(
        context, expected_type);

    if (context == NULL ||
        (expected_type != context->type_u8 &&
         expected_type != context->type_u64 &&
         expected_type != context->type_i64 &&
         expected_type != context->type_f32 &&
         expected_type != context->type_f64)) {
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
                value_type)) {
            return false;
        }

        info =
            vitte_sema_node_info_mut_private(
                context,
                node_id);

        if (info != NULL) {
            info->type_id = value_type;
            info->flags |= VITTE_SEMA_NODE_FLAG_IMPLICIT_CONVERSION;
        }

        return true;
    }

    if (!vitte_sema_parse_unsigned_integer_literal(
            context,
            node,
            &value)) {
        return false;
    }

    if (expected_type == context->type_u8 &&
        value > UINT8_MAX) {
        return false;
    }
    if (expected_type == context->type_i64 &&
        value > (uint64_t)INT64_MAX) {
        return false;
    }
    if (expected_type == context->type_f64 &&
        value > UINT64_C(9007199254740991)) {
        return false;
    }
    if (expected_type == context->type_f32 &&
        value > UINT64_C(16777215)) {
        return false;
    }

    info =
        vitte_sema_node_info_mut_private(
            context,
            node_id);

    if (info == NULL) {
        return false;
    }

    info->type_id = value_type;
    info->flags |= VITTE_SEMA_NODE_FLAG_IMPLICIT_CONVERSION;
    if (expected_type == context->type_f32 ||
        expected_type == context->type_f64) {
        info->constant.kind = VITTE_SEMA_CONSTANT_FLOAT;
        info->constant.value.float_value = (double)value;
    } else {
        info->constant.kind = VITTE_SEMA_CONSTANT_UNSIGNED_INTEGER;
        info->constant.value.unsigned_integer_value = value;
    }

    return true;
}

/* A floating literal can be typed by its destination when its magnitude is
 * representable.  This is distinct from converting an f64 variable to f32:
 * the latter remains an explicit, potentially lossy conversion. */
static bool
vitte_sema_contextualize_float_literal(
    vitte_sema_t *context,
    vitte_ast_node_id_t node_id,
    vitte_sema_type_id_t expected_type)
{
    const vitte_ast_node_t *node;
    const vitte_token_t *token;
    vitte_sema_node_info_t *info;
    char normalized[256];
    char *end;
    size_t index;
    size_t length;
    double value;
    vitte_sema_type_id_t value_type = expected_type;

    expected_type = vitte_sema_resolve_alias_type_private(
        context, expected_type);

    if (context == NULL ||
        (expected_type != context->type_f32 &&
         expected_type != context->type_f64)) {
        return false;
    }

    node = vitte_sema_node(context, node_id);
    if (node == NULL) {
        return false;
    }
    if (node->kind == VITTE_AST_NODE_UNARY_EXPR &&
        node->operator_kind == VITTE_TOKEN_MINUS &&
        node->child_count == 1u) {
        const vitte_sema_node_info_t *operand_info;

        if (!vitte_sema_contextualize_float_literal(
                context, node->children[0], value_type)) {
            return false;
        }
        info = vitte_sema_node_info_mut_private(context, node_id);
        operand_info = vitte_sema_node_info_private(
            context, node->children[0]);
        if (info == NULL || operand_info == NULL ||
            operand_info->constant.kind != VITTE_SEMA_CONSTANT_FLOAT) {
            return false;
        }
        info->type_id = value_type;
        info->flags |= VITTE_SEMA_NODE_FLAG_IMPLICIT_CONVERSION;
        info->constant.kind = VITTE_SEMA_CONSTANT_FLOAT;
        info->constant.value.float_value =
            -operand_info->constant.value.float_value;
        return true;
    }
    if (node->kind == VITTE_AST_NODE_GROUP_EXPR &&
        node->child_count == 1u) {
        if (!vitte_sema_contextualize_float_literal(
                context,
                node->children[0],
            value_type)) {
            return false;
        }
        info = vitte_sema_node_info_mut_private(context, node_id);
        if (info != NULL) {
            info->type_id = value_type;
            info->flags |= VITTE_SEMA_NODE_FLAG_IMPLICIT_CONVERSION;
        }
        return true;
    }
    if (node->kind != VITTE_AST_NODE_FLOAT_LITERAL) {
        return false;
    }

    token = vitte_sema_node_token(context, node);
    if (token == NULL || token->lexeme == NULL || token->length == 0u) {
        return false;
    }
    length = 0u;
    for (index = 0u; index < token->length; ++index) {
        if (token->lexeme[index] == '_') {
            continue;
        }
        if (length + 1u >= sizeof(normalized)) {
            return false;
        }
        normalized[length++] = token->lexeme[index];
    }
    normalized[length] = '\0';
    end = NULL;
    value = strtod(normalized, &end);
    if (end == normalized || end == NULL || *end != '\0' ||
        !isfinite(value) ||
        (expected_type == context->type_f32 &&
         fabs(value) > (double)FLT_MAX)) {
        return false;
    }

    info = vitte_sema_node_info_mut_private(context, node_id);

    if (info == NULL) {
        return false;
    }
    info->type_id = value_type;
    info->flags |= VITTE_SEMA_NODE_FLAG_IMPLICIT_CONVERSION;
    info->constant.kind = VITTE_SEMA_CONSTANT_FLOAT;
    info->constant.value.float_value = value;
    return true;
}

/* Apply the one implicit conversion policy used by every value context. */
static bool
vitte_sema_accept_value_private(
    vitte_sema_t *context,
    vitte_sema_type_id_t destination,
    vitte_sema_type_id_t source,
    vitte_ast_node_id_t source_node)
{
    const vitte_sema_type_t *destination_type;
    vitte_sema_type_id_t value_source;

    if (vitte_sema_type_assignable_private(
            context,
            destination,
            source)) {
        return true;
    }

    destination_type = vitte_sema_type_private(
        context,
        destination);
    if (destination_type != NULL &&
        destination_type->kind != VITTE_SEMA_TYPE_REFERENCE) {
        value_source = vitte_sema_value_type_private(
            context,
            source);
        if (vitte_sema_type_assignable_private(
                context,
                destination,
                value_source)) {
            return true;
        }
    }

    return vitte_sema_contextualize_integer_literal(
               context,
               source_node,
               destination) ||
           vitte_sema_contextualize_float_literal(
               context,
               source_node,
               destination);
}

static void
vitte_sema_append_diagnostic_help(
    vitte_sema_diagnostic_t *diagnostic,
    const char *addition)
{
    size_t used;

    if (diagnostic == NULL || addition == NULL || addition[0] == '\0') {
        return;
    }

    used = strlen(diagnostic->help);
    if (used >= sizeof(diagnostic->help)) {
        return;
    }

    (void)snprintf(
        diagnostic->help + used,
        sizeof(diagnostic->help) - used,
        "%s%s",
        used != 0u ? " " : "",
        addition);
}

static void
vitte_sema_append_symbol_origin_help(
    vitte_sema_t *context,
    const vitte_symbol_t *symbol,
    const char *description)
{
    vitte_sema_diagnostic_t *diagnostic;
    char addition[256];
    int shown_length;
    int written;

    if (context == NULL || symbol == NULL || symbol->name == NULL ||
        symbol->name_length == 0u || description == NULL ||
        context->diagnostic_count == 0u) {
        return;
    }

    diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
    shown_length = symbol->name_length > 128u
        ? 128
        : (int)symbol->name_length;
    written = snprintf(
        addition,
        sizeof(addition),
        "%s declaration `%.*s` is shown at the related location.",
        description,
        shown_length,
        symbol->name);
    if (written < 0 || (size_t)written >= sizeof(addition)) {
        return;
    }
    vitte_sema_append_diagnostic_help(diagnostic, addition);
}

static void
vitte_sema_help_private_symbol(
    vitte_sema_t *context,
    const vitte_symbol_t *symbol)
{
    vitte_sema_append_symbol_origin_help(context, symbol, "The private");
    if (context != NULL && context->diagnostic_count != 0u &&
        symbol != NULL && symbol->name != NULL) {
        char addition[256];
        int written;
        size_t shown_length = symbol->name_length < 96u
            ? symbol->name_length : 96u;
        written = snprintf(
            addition,
            sizeof(addition),
            " Add `pub` to the declaration of `%.*s` or add `export %.*s;` in the defining module.",
            (int)shown_length,
            symbol->name,
            (int)shown_length,
            symbol->name);
        if (written > 0 && (size_t)written < sizeof(addition)) {
            vitte_sema_append_diagnostic_help(
                &context->diagnostics[context->diagnostic_count - 1u],
                addition);
        }
    }
}

/*
 * Add a cast hint only for a plain non-negative integer literal whose value
 * is representable by the target type. This deliberately avoids suggesting
 * casts for variables, compound expressions, or conversions that may lose
 * information.
 */
static void
vitte_sema_add_safe_integer_conversion_help(
    vitte_sema_t *context,
    vitte_ast_node_id_t expression_id,
    vitte_sema_type_id_t expected_type_id,
    vitte_sema_type_id_t found_type_id)
{
    const vitte_sema_type_t *expected_type;
    const vitte_sema_type_t *found_type;
    const vitte_ast_node_t *expression;
    const vitte_token_t *token;
    vitte_sema_diagnostic_t *diagnostic;
    const char *target_name;
    char help[256];
    uint64_t value;
    size_t depth;

    if (context == NULL || context->diagnostic_count == 0u) {
        return;
    }

    expected_type = vitte_sema_type_private(context, expected_type_id);
    found_type = vitte_sema_type_private(context, found_type_id);
    if (expected_type == NULL || found_type == NULL ||
        found_type->kind != VITTE_SEMA_TYPE_I64) {
        return;
    }

    if (expected_type->kind == VITTE_SEMA_TYPE_U8) {
        target_name = "u8";
    } else if (expected_type->kind == VITTE_SEMA_TYPE_U64) {
        target_name = "u64";
    } else {
        return;
    }

    expression = vitte_sema_node(context, expression_id);
    for (depth = 0u;
         expression != NULL &&
             expression->kind == VITTE_AST_NODE_GROUP_EXPR &&
             expression->child_count == 1u &&
             depth < 16u;
         ++depth) {
        expression = vitte_sema_node(context, expression->children[0]);
    }
    if (expression == NULL ||
        expression->kind != VITTE_AST_NODE_INTEGER_LITERAL ||
        !vitte_sema_parse_unsigned_integer_literal(
            context,
            expression,
            &value)) {
        return;
    }

    if ((expected_type->kind == VITTE_SEMA_TYPE_U8 &&
         value > UINT8_MAX) ||
        (expected_type->kind == VITTE_SEMA_TYPE_U64 &&
         value > (uint64_t)INT64_MAX)) {
        return;
    }

    token = vitte_sema_node_token(context, expression);
    if (token == NULL || token->lexeme == NULL ||
        token->length == 0u || token->length > 96u) {
        return;
    }

    diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
    (void)snprintf(
        help,
        sizeof(help),
        "This literal fits in `%s`; make the conversion explicit with `%.*s as %s`.",
        target_name,
        (int)token->length,
        token->lexeme,
        target_name);
    vitte_sema_append_diagnostic_help(diagnostic, help);
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

    left = vitte_sema_resolve_alias_type_private(context, left);
    right = vitte_sema_resolve_alias_type_private(context, right);

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

    if (left_type->kind == VITTE_SEMA_TYPE_F32 ||
        right_type->kind == VITTE_SEMA_TYPE_F32) {
        return context->type_f32;
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
    vitte_sema_t *context,
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
    if (symbol != NULL && symbol->payload != NULL) {
        const vitte_ast_node_t *declaration;
        vitte_sema_node_info_t *declaration_info;

        declaration =
            (const vitte_ast_node_t *)symbol->payload;
        declaration_info =
            vitte_sema_node_info_mut_private(
                context,
                declaration->id);
        /* Declared symbols already carry their AST declaration. Resolve
         * its type directly; retain the scan for synthetic/untyped symbols. */
        if (declaration_info != NULL &&
            (declaration_info->symbol_id == symbol_id ||
             (symbol->flags & VITTE_SYMBOL_FLAG_IMPORTED) != 0u) &&
            vitte_sema_node(
                context,
                declaration->id) == declaration &&
            declaration_info->type_id !=
                VITTE_SEMA_INVALID_TYPE_ID) {
            if (declaration->kind == VITTE_AST_NODE_TYPE_DECL &&
                (declaration->flags & VITTE_AST_FLAG_TYPE_ALIAS) != 0u &&
                declaration_info->type_id != context->type_error) {
                const vitte_sema_type_t *declared_type =
                    vitte_sema_type_private(
                        context, declaration_info->type_id);
                const vitte_ast_node_t *target = declaration->child_count != 0u
                    ? vitte_sema_node(
                          context,
                          declaration->children[
                              declaration->child_count - 1u])
                    : NULL;
                bool has_generics = false;
                size_t child_index;

                for (child_index = 1u;
                     child_index + 1u < declaration->child_count;
                     ++child_index) {
                    const vitte_ast_node_t *parameter = vitte_sema_node(
                        context, declaration->children[child_index]);
                    if (parameter != NULL &&
                        parameter->kind == VITTE_AST_NODE_GENERIC_PARAM) {
                        has_generics = true;
                        break;
                    }
                }
                if (!has_generics && target != NULL &&
                    declared_type != NULL &&
                    declared_type->kind == VITTE_SEMA_TYPE_NAMED &&
                    declared_type->symbol_id == symbol_id) {
                    vitte_sema_type_id_t placeholder =
                        declaration_info->type_id;
                    vitte_sema_type_id_t alias_type;

                    /* Temporarily mark this alias as resolving so a cycle
                     * such as `A = B; B = A` terminates while resolving the
                     * target. Restore the placeholder if the target fails;
                     * the regular alias pass then reports the cycle. */
                    if (declaration_info != NULL) {
                        declaration_info->type_id = context->type_error;
                    }
                    alias_type = vitte_sema_analyze_type_node(
                        context,
                        target->id,
                        declaration_info != NULL
                            ? declaration_info->scope_id
                            : 0u);
                    if (declaration_info != NULL) {
                        declaration_info->type_id = alias_type !=
                                context->type_error
                            ? alias_type
                            : placeholder;
                    }
                    if (alias_type != context->type_error) {
                        return alias_type;
                    }
                }
            }
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

static size_t
vitte_sema_generic_parameter_count_private(
    const vitte_sema_t *context,
    vitte_symbol_id_t symbol_id)
{
    const vitte_symbol_t *symbol;
    const vitte_ast_node_t *declaration;
    size_t count;
    size_t index;

    if (context == NULL ||
        symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return 0u;
    }

    symbol = vitte_scope_get_symbol(&context->scopes, symbol_id);
    if (symbol == NULL || symbol->payload == NULL) {
        return 0u;
    }

    declaration = (const vitte_ast_node_t *)symbol->payload;
    count = 0u;
    for (index = 0u; index < declaration->child_count; ++index) {
        const vitte_ast_node_t *child =
            vitte_sema_node(context, declaration->children[index]);
        if (child != NULL &&
            child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
            ++count;
        }
    }
    return count;
}

static bool
vitte_sema_validate_generic_arguments_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *type_node,
    vitte_symbol_id_t symbol_id,
    size_t argument_count)
{
    const vitte_symbol_t *symbol;
    const vitte_ast_node_t *declaration;
    size_t expected;
    size_t index;
    vitte_sema_diagnostic_t *diagnostic;
    size_t diagnostic_index;

    if (context != NULL && type_node != NULL) {
        for (diagnostic_index = 0u;
             diagnostic_index < context->diagnostic_count;
             ++diagnostic_index) {
            const vitte_sema_diagnostic_t *prior =
                &context->diagnostics[diagnostic_index];
            if ((prior->code ==
                     VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_COUNT &&
                 prior->node_id == type_node->id) ||
                ((prior->code ==
                      VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_KIND) &&
                 prior->span.file_id == type_node->span.file_id &&
                 prior->span.begin >= type_node->span.begin &&
                 prior->span.end <= type_node->span.end)) {
                return false;
            }
        }
    }

    expected = vitte_sema_generic_parameter_count_private(
        context,
        symbol_id);
    if (expected == argument_count) {
        /* The current grammar accepts type arguments only. Keep this check
         * here, next to arity validation, so a const generic is never
         * silently treated as a type parameter. */
        symbol = vitte_scope_get_symbol(&context->scopes, symbol_id);
        declaration = symbol != NULL
            ? (const vitte_ast_node_t *)symbol->payload
            : NULL;
        if (declaration != NULL && type_node != NULL) {
            size_t argument_index = 0u;
            for (index = 0u; index < declaration->child_count; ++index) {
                const vitte_ast_node_t *parameter = vitte_sema_node(
                    context, declaration->children[index]);
                const vitte_ast_node_t *argument;
                if (parameter == NULL ||
                    parameter->kind != VITTE_AST_NODE_GENERIC_PARAM) {
                    continue;
                }
                if (argument_index >= argument_count) { break; }
                argument = vitte_sema_node(
                    context, type_node->children[argument_index + 1u]);
                if ((parameter->flags & VITTE_AST_FLAG_CONST_GENERIC) != 0u) {
                    if (vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_KIND,
                            argument != NULL ? argument->span : type_node->span,
                            argument != NULL ? argument->id : type_node->id,
                            symbol_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID)) {
                        diagnostic = &context->diagnostics[
                            context->diagnostic_count - 1u];
                        (void)snprintf(
                            diagnostic->help,
                            sizeof(diagnostic->help),
                            "Generic parameter %zu is a const parameter, but this syntax supplies a type argument. Const generic arguments are not supported in type applications yet.",
                            argument_index + 1u);
                        vitte_sema_set_last_related_location(
                            context, parameter,
                            "const generic parameter declared here");
                    }
                    return false;
                }
                ++argument_index;
            }
        }
        return true;
    }

    if (!vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_COUNT,
            type_node != NULL ? type_node->span : vitte_parser_span_invalid(),
            type_node != NULL ? type_node->id : VITTE_AST_INVALID_ID,
            symbol_id,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID)) {
        return false;
    }

    diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
    symbol = vitte_scope_get_symbol(&context->scopes, symbol_id);
    if (symbol != NULL && symbol->name != NULL) {
        (void)snprintf(
            diagnostic->help,
            sizeof(diagnostic->help),
            "Type `%.*s` expects %zu generic argument%s, found %zu.",
            (int)(symbol->name_length > 128u ? 128u : symbol->name_length),
            symbol->name,
            expected,
            expected == 1u ? "" : "s",
            argument_count);
    } else {
        (void)snprintf(
            diagnostic->help,
            sizeof(diagnostic->help),
            "Expected %zu generic argument%s, found %zu.",
            expected,
            expected == 1u ? "" : "s",
            argument_count);
    }
    if (symbol != NULL && symbol->payload != NULL) {
        vitte_sema_set_last_related_location(
            context,
            (const vitte_ast_node_t *)symbol->payload,
            "generic parameters declared here");
    }
    return false;
}

static bool
vitte_sema_report_incomplete_type_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *use_node,
    vitte_sema_type_id_t type_id)
{
    const vitte_sema_type_t *type;
    const vitte_symbol_t *symbol;

    if (context == NULL || use_node == NULL) { return false; }
    type = vitte_sema_type_private(context, type_id);
    if (type == NULL || type->kind != VITTE_SEMA_TYPE_NAMED) {
        return false;
    }
    symbol = vitte_scope_get_symbol(&context->scopes, type->symbol_id);
    if (symbol == NULL || symbol->kind != VITTE_SYMBOL_KIND_OPAQUE) {
        return false;
    }
    if (vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_INCOMPLETE_TYPE,
            use_node->span,
            use_node->id,
            type->symbol_id,
            VITTE_SEMA_INVALID_TYPE_ID,
            type_id)) {
        vitte_sema_append_diagnostic_help(
            &context->diagnostics[context->diagnostic_count - 1u],
            "This operation requires the definition of the opaque type, but only its declaration is visible.");
        vitte_sema_set_last_related_location(
            context,
            (const vitte_ast_node_t *)symbol->payload,
            "opaque type declared here");
    }
    return true;
}

/* Instantiate the target of a generic type alias in a temporary child scope.
 * The alias parameter symbols carry the concrete argument type IDs, so the
 * ordinary type analyzer can lower nested forms and pointer compositions. */
static vitte_sema_type_id_t
vitte_sema_expand_generic_alias_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *declaration,
    const vitte_ast_node_t *target,
    vitte_scope_id_t parent_scope,
    const vitte_sema_type_id_t *arguments,
    size_t argument_count)
{
    vitte_scope_id_t alias_scope;
    size_t argument_index;
    size_t child_index;

    if (context == NULL || declaration == NULL || target == NULL ||
        arguments == NULL || argument_count == 0u) {
        return context != NULL ? context->type_error
                               : VITTE_SEMA_INVALID_TYPE_ID;
    }
    if (context->recursion_depth >= context->max_recursion_depth) {
        context->last_error = VITTE_SEMA_ERROR_RECURSION_LIMIT;
        if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_RECURSIVE_TYPE,
                target->span,
                target->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
            vitte_sema_append_diagnostic_help(
                &context->diagnostics[context->diagnostic_count - 1u],
                "Expanding this generic type alias exceeds the supported recursion depth; remove the recursive alias cycle or introduce an indirection.");
            vitte_sema_set_last_related_location(
                context,
                declaration,
                "generic type alias declaration participating in the cycle");
        }
        return context->type_error;
    }
    ++context->recursion_depth;
    alias_scope = vitte_scope_create(
        &context->scopes,
        parent_scope,
        VITTE_SCOPE_KIND_FORM,
        vitte_sema_scope_span_from_parser(declaration->span),
        (void *)declaration);
    if (alias_scope == VITTE_SCOPE_INVALID_SCOPE_ID) {
        --context->recursion_depth;
        return context->type_error;
    }

    argument_index = 0u;
    for (child_index = 1u;
         child_index + 1u < declaration->child_count;
         ++child_index) {
        const vitte_ast_node_t *parameter = vitte_sema_node(
            context, declaration->children[child_index]);
        const vitte_ast_node_t *name_node;
        const vitte_token_t *name;
        vitte_symbol_id_t parameter_symbol;
        vitte_sema_node_info_t *parameter_info;

        if (parameter == NULL ||
            parameter->kind != VITTE_AST_NODE_GENERIC_PARAM) {
            continue;
        }
        if (argument_index >= argument_count) {
            --context->recursion_depth;
            return context->type_error;
        }
        name_node = parameter->child_count != 0u
            ? vitte_sema_node(context, parameter->children[0])
            : NULL;
        name = vitte_sema_node_token(context, name_node);
        if (name == NULL || name->lexeme == NULL || name->length == 0u) {
            --context->recursion_depth;
            return context->type_error;
        }
        parameter_symbol = vitte_scope_declare(
            &context->scopes,
            alias_scope,
            name->lexeme,
            name->length,
            VITTE_SYMBOL_KIND_GENERIC_PARAMETER,
            VITTE_SYMBOL_VISIBILITY_PRIVATE,
            0u,
            vitte_sema_scope_span_from_parser(parameter->span),
            (void *)parameter);
        if (parameter_symbol == VITTE_SCOPE_INVALID_SYMBOL_ID) {
            --context->recursion_depth;
            return context->type_error;
        }
        parameter_info = vitte_sema_node_info_mut_private(
            context, parameter->id);
        if (parameter_info == NULL) {
            --context->recursion_depth;
            return context->type_error;
        }
        parameter_info->symbol_id = parameter_symbol;
        parameter_info->scope_id = alias_scope;
        parameter_info->type_id = arguments[argument_index];
        parameter_info->flags |= VITTE_SEMA_NODE_FLAG_DECLARATION;
        ++argument_index;
    }
    if (argument_index != argument_count) {
        --context->recursion_depth;
        return context->type_error;
    }
    {
        vitte_sema_type_id_t result = vitte_sema_analyze_type_node(
            context, target->id, alias_scope);
        --context->recursion_depth;
        return result;
    }
}

static vitte_sema_type_id_t
vitte_sema_apply_generic_arguments_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *type_node,
    vitte_scope_id_t scope_id,
    vitte_symbol_id_t symbol_id,
    vitte_sema_type_id_t base_type)
{
    size_t argument_count;
    size_t index;
    size_t bytes;
    vitte_sema_type_id_t *arguments;
    vitte_sema_type_id_t result;

    const vitte_symbol_t *base_symbol;

    if (type_node == NULL ||
        type_node->kind != VITTE_AST_NODE_TYPE_EXPR ||
        type_node->child_count == 0u) {
        return base_type;
    }

    if (base_type == context->type_error) {
        return base_type;
    }

    base_symbol = vitte_scope_get_symbol(&context->scopes, symbol_id);
    if (base_symbol != NULL &&
        base_symbol->kind == VITTE_SYMBOL_KIND_GENERIC_PARAMETER) {
        return base_type;
    }

    argument_count = type_node->child_count - 1u;
    if (!vitte_sema_validate_generic_arguments_private(
            context,
            type_node,
            symbol_id,
            argument_count)) {
        return context->type_error;
    }

    if (!vitte_sema_size_mul(
            argument_count,
            sizeof(*arguments),
            &bytes)) {
        context->last_error = VITTE_SEMA_ERROR_OVERFLOW;
        return context->type_error;
    }
    arguments = (vitte_sema_type_id_t *)vitte_sema_reallocate(
        context,
        NULL,
        argument_count,
        sizeof(*arguments));
    if (arguments == NULL) {
        return context->type_error;
    }

    for (index = 0u; index < argument_count; ++index) {
        arguments[index] = vitte_sema_analyze_type_node(
            context,
            type_node->children[index + 1u],
            scope_id);
        if (arguments[index] == context->type_error) {
            free(arguments);
            return context->type_error;
        }
    }

    {
        const vitte_symbol_t *symbol = vitte_scope_get_symbol(
            &context->scopes, symbol_id);
        const vitte_ast_node_t *declaration = symbol != NULL
            ? (const vitte_ast_node_t *)symbol->payload
            : NULL;
        const vitte_ast_node_t *target = declaration != NULL &&
                declaration->kind == VITTE_AST_NODE_TYPE_DECL &&
                (declaration->flags & VITTE_AST_FLAG_TYPE_ALIAS) != 0u &&
                declaration->child_count >= 2u
            ? vitte_sema_node(
                  context,
                  declaration->children[declaration->child_count - 1u])
            : NULL;
        if (target != NULL && declaration->child_count > 2u) {
            result = vitte_sema_expand_generic_alias_private(
                context,
                declaration,
                target,
                scope_id,
                arguments,
                argument_count);
            free(arguments);
            return result;
        }
    }

    result = vitte_sema_create_named_instance_private(
        context,
        symbol_id,
        arguments,
        argument_count);
    free(arguments);
    (void)bytes;
    return result == VITTE_SEMA_INVALID_TYPE_ID
        ? context->type_error
        : result;
}

static vitte_sema_type_id_t
vitte_sema_substitute_generic_type_private(
    vitte_sema_t *context,
    vitte_sema_type_id_t type_id,
    vitte_sema_type_id_t instance_id)
{
    const vitte_sema_type_t *type;
    const vitte_sema_type_t *instance;
    const vitte_symbol_t *owner_symbol;
    const vitte_symbol_t *parameter_symbol;
    const vitte_ast_node_t *owner_declaration;
    vitte_sema_type_kind_t type_kind;
    vitte_sema_type_id_t element_type;
    vitte_symbol_id_t type_symbol_id;
    vitte_symbol_id_t instance_symbol_id;
    const vitte_sema_type_id_t *instance_arguments;
    const vitte_sema_type_id_t *type_arguments;
    size_t array_length;
    size_t argument_count;
    size_t instance_argument_count;
    bool is_mutable;
    size_t index;

    type = vitte_sema_type_private(context, type_id);
    instance = vitte_sema_type_private(context, instance_id);
    if (type == NULL || instance == NULL ||
        instance->kind != VITTE_SEMA_TYPE_NAMED ||
        instance->generic_argument_count == 0u) {
        return type_id;
    }

    type_kind = type->kind;
    element_type = type->element_type;
    type_symbol_id = type->symbol_id;
    array_length = type->array_length;
    is_mutable = type->is_mutable;
    argument_count = type->generic_argument_count;
    instance_symbol_id = instance->symbol_id;
    instance_argument_count = instance->generic_argument_count;
    instance_arguments = instance->generic_arguments;

    if (type_kind == VITTE_SEMA_TYPE_POINTER ||
        type_kind == VITTE_SEMA_TYPE_REFERENCE ||
        type_kind == VITTE_SEMA_TYPE_ARRAY ||
        type_kind == VITTE_SEMA_TYPE_SLICE ||
        type_kind == VITTE_SEMA_TYPE_OPTIONAL) {
        vitte_sema_type_id_t substituted_element =
            vitte_sema_substitute_generic_type_private(
                context,
                element_type,
                instance_id);
        if (substituted_element != element_type) {
            return vitte_sema_create_type_private(
                context,
                type_kind,
                substituted_element,
                array_length,
                type_symbol_id,
                is_mutable);
        }
        return type_id;
    }

    if (type_kind == VITTE_SEMA_TYPE_NAMED && argument_count != 0u) {
        const vitte_sema_type_t *current_type =
            vitte_sema_type_private(context, type_id);
        vitte_sema_type_id_t *substituted_arguments;
        size_t bytes;
        bool changed = false;

        if (current_type == NULL ||
            !vitte_sema_size_mul(
                argument_count,
                sizeof(*substituted_arguments),
                &bytes)) {
            context->last_error = VITTE_SEMA_ERROR_OVERFLOW;
            return context->type_error;
        }
        type_arguments = current_type->generic_arguments;
        substituted_arguments = (vitte_sema_type_id_t *)
            vitte_sema_reallocate(
                context,
                NULL,
                argument_count,
                sizeof(*substituted_arguments));
        if (substituted_arguments == NULL) {
            return context->type_error;
        }
        for (index = 0u; index < argument_count; ++index) {
            vitte_sema_type_id_t original_argument =
                type_arguments[index];
            substituted_arguments[index] =
                vitte_sema_substitute_generic_type_private(
                    context,
                    original_argument,
                    instance_id);
            if (substituted_arguments[index] == context->type_error) {
                free(substituted_arguments);
                return context->type_error;
            }
            if (substituted_arguments[index] != original_argument) {
                changed = true;
            }
        }
        if (!changed) {
            free(substituted_arguments);
            return type_id;
        }
        {
            vitte_sema_type_id_t result =
                vitte_sema_create_named_instance_private(
                    context,
                    type_symbol_id,
                    substituted_arguments,
                    argument_count);
            free(substituted_arguments);
            (void)bytes;
            return result == VITTE_SEMA_INVALID_TYPE_ID
                ? context->type_error
                : result;
        }
    }

    if (type_kind != VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
        return type_id;
    }

    owner_symbol = vitte_scope_get_symbol(
        &context->scopes,
        instance_symbol_id);
    owner_declaration = owner_symbol != NULL
        ? (const vitte_ast_node_t *)owner_symbol->payload
        : NULL;
    if (owner_declaration == NULL) {
        return type_id;
    }
    parameter_symbol = vitte_scope_get_symbol(
        &context->scopes,
        type_symbol_id);
    if (parameter_symbol == NULL || parameter_symbol->name == NULL ||
        parameter_symbol->kind != VITTE_SYMBOL_KIND_GENERIC_PARAMETER) {
        return type_id;
    }

    index = 0u;
    for (size_t child_index = 0u;
         child_index < owner_declaration->child_count;
         ++child_index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context,
            owner_declaration->children[child_index]);
        const vitte_ast_node_t *parameter_name;
        const vitte_token_t *parameter_token;
        if (child == NULL ||
            child->kind != VITTE_AST_NODE_GENERIC_PARAM) {
            continue;
        }
        parameter_name = child->child_count != 0u
            ? vitte_sema_node(context, child->children[0])
            : NULL;
        parameter_token = vitte_sema_node_token(context, parameter_name);
        if (parameter_token != NULL && parameter_token->lexeme != NULL &&
            parameter_symbol->name_length == parameter_token->length &&
            memcmp(
                parameter_symbol->name,
                parameter_token->lexeme,
                parameter_token->length) == 0 &&
            index < instance_argument_count) {
            return instance_arguments[index];
        }
        ++index;
    }
    return type_id;
}

typedef struct vitte_sema_generic_call_binding_private {
    vitte_sema_type_id_t parameter_type;
    vitte_sema_type_id_t concrete_type;
} vitte_sema_generic_call_binding_private_t;

static bool
vitte_sema_generic_call_type_contains_parameter_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id,
    size_t depth)
{
    const vitte_sema_type_t *type;
    size_t index;

    if (context == NULL || type_id == VITTE_SEMA_INVALID_TYPE_ID ||
        depth > VITTE_SEMA_DEFAULT_MAX_RECURSION_DEPTH) {
        return false;
    }
    type = vitte_sema_type_private(context, type_id);
    if (type == NULL) { return false; }
    if (type->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) { return true; }
    if (type->element_type != VITTE_SEMA_INVALID_TYPE_ID &&
        vitte_sema_generic_call_type_contains_parameter_private(
            context, type->element_type, depth + 1u)) {
        return true;
    }
    if (type->return_type != VITTE_SEMA_INVALID_TYPE_ID &&
        vitte_sema_generic_call_type_contains_parameter_private(
            context, type->return_type, depth + 1u)) {
        return true;
    }
    for (index = 0u; index < type->generic_argument_count; ++index) {
        if (vitte_sema_generic_call_type_contains_parameter_private(
                context, type->generic_arguments[index], depth + 1u)) {
            return true;
        }
    }
    for (index = 0u; index < type->function_parameter_count; ++index) {
        if (vitte_sema_generic_call_type_contains_parameter_private(
                context,
                type->function_parameter_types[index],
                depth + 1u)) {
            return true;
        }
    }
    return false;
}

static bool
vitte_sema_generic_call_bind_private(
    vitte_sema_t *context,
    vitte_sema_generic_call_binding_private_t *bindings,
    size_t *binding_count,
    size_t binding_capacity,
    vitte_sema_type_id_t parameter_type,
    vitte_sema_type_id_t concrete_type,
    const vitte_ast_node_t *source_node,
    bool report_conflicts)
{
    size_t index;
    size_t node_index;

    if (context == NULL || binding_count == NULL ||
        (binding_capacity != 0u && bindings == NULL)) {
        return false;
    }
    for (index = 0u; index < *binding_count; ++index) {
        if (bindings[index].parameter_type == parameter_type) {
            if (vitte_sema_type_identical_private(
                context,
                bindings[index].concrete_type,
                concrete_type)) {
                return true;
            }
            if (report_conflicts &&
                vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_GENERIC_INFERENCE_CONFLICT,
                    source_node != NULL
                        ? source_node->span
                        : vitte_parser_span_invalid(),
                    source_node != NULL
                        ? source_node->id
                        : VITTE_AST_INVALID_ID,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    bindings[index].concrete_type,
                    concrete_type)) {
                vitte_sema_diagnostic_t *diagnostic =
                    &context->diagnostics[context->diagnostic_count - 1u];
                (void)snprintf(
                    diagnostic->help,
                    sizeof(diagnostic->help),
                    "This argument conflicts with an earlier argument that inferred a different type for the same generic parameter. Make the argument types agree or add an explicit type annotation.");
                for (node_index = 0u;
                     node_index < context->node_info_count;
                     ++node_index) {
                    const vitte_ast_node_t *parameter = vitte_sema_node(
                        context, (vitte_ast_node_id_t)(node_index + 1u));
                    if (parameter != NULL &&
                        parameter->kind == VITTE_AST_NODE_GENERIC_PARAM &&
                        context->node_info[node_index].type_id ==
                            parameter_type) {
                        vitte_sema_set_last_related_location(
                            context,
                            parameter,
                            "generic parameter whose substitution conflicts");
                        break;
                    }
                }
            }
            return false;
        }
    }
    if (*binding_count >= binding_capacity) { return false; }
    bindings[*binding_count].parameter_type = parameter_type;
    bindings[*binding_count].concrete_type = concrete_type;
    ++*binding_count;
    return true;
}

static bool
vitte_sema_generic_call_unify_type_private(
    vitte_sema_t *context,
    vitte_sema_generic_call_binding_private_t *bindings,
    size_t *binding_count,
    size_t binding_capacity,
    vitte_sema_type_id_t expected_id,
    vitte_sema_type_id_t actual_id,
    const vitte_ast_node_t *source_node,
    bool report_conflicts,
    size_t depth)
{
    vitte_sema_type_t expected;
    vitte_sema_type_t actual;
    size_t index;

    if (context == NULL || expected_id == VITTE_SEMA_INVALID_TYPE_ID ||
        actual_id == VITTE_SEMA_INVALID_TYPE_ID ||
        depth > VITTE_SEMA_DEFAULT_MAX_RECURSION_DEPTH) {
        return false;
    }
    {
        const vitte_sema_type_t *expected_type =
            vitte_sema_type_private(context, expected_id);
        const vitte_sema_type_t *actual_type =
            vitte_sema_type_private(context, actual_id);
        if (expected_type == NULL || actual_type == NULL) { return false; }
        expected = *expected_type;
        actual = *actual_type;
    }

    if (expected.kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
        return vitte_sema_generic_call_bind_private(
            context,
            bindings,
            binding_count,
            binding_capacity,
            expected_id,
            actual_id,
            source_node,
            report_conflicts);
    }
    if (!vitte_sema_generic_call_type_contains_parameter_private(
            context, expected_id, depth)) {
        return vitte_sema_accept_value_private(
            context,
            expected_id,
            actual_id,
            source_node != NULL ? source_node->id : VITTE_AST_INVALID_ID);
    }
    if (expected.kind != actual.kind) {
        return vitte_sema_accept_value_private(
            context,
            expected_id,
            actual_id,
            source_node != NULL ? source_node->id : VITTE_AST_INVALID_ID);
    }

    if (expected.kind == VITTE_SEMA_TYPE_NAMED ||
        expected.kind == VITTE_SEMA_TYPE_DYN) {
        if (expected.symbol_id != actual.symbol_id ||
            expected.generic_argument_count !=
                actual.generic_argument_count) {
            return false;
        }
        for (index = 0u;
             index < expected.generic_argument_count;
             ++index) {
            if (!vitte_sema_generic_call_unify_type_private(
                    context,
                    bindings,
                    binding_count,
                    binding_capacity,
                    expected.generic_arguments[index],
                    actual.generic_arguments[index],
                    source_node,
                    report_conflicts,
                    depth + 1u)) {
                return false;
            }
        }
        return true;
    }

    if (expected.kind == VITTE_SEMA_TYPE_POINTER ||
        expected.kind == VITTE_SEMA_TYPE_REFERENCE ||
        expected.kind == VITTE_SEMA_TYPE_OPTIONAL ||
        expected.kind == VITTE_SEMA_TYPE_ARRAY ||
        expected.kind == VITTE_SEMA_TYPE_SLICE ||
        expected.kind == VITTE_SEMA_TYPE_RANGE) {
        if ((expected.kind == VITTE_SEMA_TYPE_ARRAY &&
             expected.array_length != actual.array_length) ||
            expected.is_mutable != actual.is_mutable) {
            return false;
        }
        return vitte_sema_generic_call_unify_type_private(
            context,
            bindings,
            binding_count,
            binding_capacity,
            expected.element_type,
            actual.element_type,
            source_node,
            report_conflicts,
            depth + 1u);
    }

    if (expected.kind == VITTE_SEMA_TYPE_FUNCTION) {
        if (expected.function_parameter_count !=
            actual.function_parameter_count) {
            return false;
        }
        for (index = 0u;
             index < expected.function_parameter_count;
             ++index) {
            if (!vitte_sema_generic_call_unify_type_private(
                    context,
                    bindings,
                    binding_count,
                    binding_capacity,
                    expected.function_parameter_types[index],
                    actual.function_parameter_types[index],
                    source_node,
                    report_conflicts,
                    depth + 1u)) {
                return false;
            }
        }
        return vitte_sema_generic_call_unify_type_private(
            context,
            bindings,
            binding_count,
            binding_capacity,
            expected.return_type,
            actual.return_type,
            source_node,
            report_conflicts,
            depth + 1u);
    }
    return vitte_sema_accept_value_private(
        context,
        expected_id,
        actual_id,
        source_node != NULL ? source_node->id : VITTE_AST_INVALID_ID);
}

static vitte_sema_type_id_t
vitte_sema_generic_call_substitute_type_private(
    vitte_sema_t *context,
    const vitte_sema_generic_call_binding_private_t *bindings,
    size_t binding_count,
    vitte_sema_type_id_t type_id,
    size_t depth)
{
    const vitte_sema_type_t *type_pointer;
    vitte_sema_type_t type;
    size_t index;

    if (context == NULL || type_id == VITTE_SEMA_INVALID_TYPE_ID ||
        depth > VITTE_SEMA_DEFAULT_MAX_RECURSION_DEPTH) {
        return VITTE_SEMA_INVALID_TYPE_ID;
    }
    type_pointer = vitte_sema_type_private(context, type_id);
    if (type_pointer == NULL) { return context->type_error; }
    type = *type_pointer;
    if (type.kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
        for (index = 0u; index < binding_count; ++index) {
            if (bindings[index].parameter_type == type_id) {
                return bindings[index].concrete_type;
            }
        }
        return type_id;
    }
    if (type.kind == VITTE_SEMA_TYPE_NAMED &&
        type.generic_argument_count != 0u) {
        vitte_sema_type_id_t *arguments = (vitte_sema_type_id_t *)calloc(
            type.generic_argument_count, sizeof(*arguments));
        bool changed = false;
        vitte_sema_type_id_t result;
        if (arguments == NULL) { return context->type_error; }
        for (index = 0u; index < type.generic_argument_count; ++index) {
            arguments[index] =
                vitte_sema_generic_call_substitute_type_private(
                    context,
                    bindings,
                    binding_count,
                    type.generic_arguments[index],
                    depth + 1u);
            if (arguments[index] == context->type_error ||
                arguments[index] == VITTE_SEMA_INVALID_TYPE_ID) {
                free(arguments);
                return context->type_error;
            }
            if (arguments[index] != type.generic_arguments[index]) {
                changed = true;
            }
        }
        result = changed
            ? vitte_sema_create_named_instance_private(
                  context,
                  type.symbol_id,
                  arguments,
                  type.generic_argument_count)
            : type_id;
        free(arguments);
        return result == VITTE_SEMA_INVALID_TYPE_ID
            ? context->type_error
            : result;
    }
    if (type.kind == VITTE_SEMA_TYPE_POINTER ||
        type.kind == VITTE_SEMA_TYPE_REFERENCE ||
        type.kind == VITTE_SEMA_TYPE_OPTIONAL ||
        type.kind == VITTE_SEMA_TYPE_ARRAY ||
        type.kind == VITTE_SEMA_TYPE_SLICE ||
        type.kind == VITTE_SEMA_TYPE_RANGE) {
        vitte_sema_type_id_t element =
            vitte_sema_generic_call_substitute_type_private(
                context,
                bindings,
                binding_count,
                type.element_type,
                depth + 1u);
        if (element == context->type_error ||
            element == VITTE_SEMA_INVALID_TYPE_ID) {
            return context->type_error;
        }
        if (element != type.element_type) {
            vitte_sema_type_id_t result = vitte_sema_create_type_private(
                context,
                type.kind,
                element,
                type.array_length,
                type.symbol_id,
                type.is_mutable);
            return result == VITTE_SEMA_INVALID_TYPE_ID
                ? context->type_error
                : result;
        }
        return type_id;
    }
    if (type.kind == VITTE_SEMA_TYPE_FUNCTION) {
        vitte_sema_type_id_t *parameters = NULL;
        vitte_sema_type_id_t return_type;
        vitte_sema_type_id_t result;
        bool changed = false;
        if (type.function_parameter_count != 0u) {
            parameters = (vitte_sema_type_id_t *)calloc(
                type.function_parameter_count, sizeof(*parameters));
            if (parameters == NULL) { return context->type_error; }
        }
        for (index = 0u; index < type.function_parameter_count; ++index) {
            parameters[index] =
                vitte_sema_generic_call_substitute_type_private(
                    context,
                    bindings,
                    binding_count,
                    type.function_parameter_types[index],
                    depth + 1u);
            if (parameters[index] == context->type_error ||
                parameters[index] == VITTE_SEMA_INVALID_TYPE_ID) {
                free(parameters);
                return context->type_error;
            }
            if (parameters[index] != type.function_parameter_types[index]) {
                changed = true;
            }
        }
        return_type = vitte_sema_generic_call_substitute_type_private(
            context,
            bindings,
            binding_count,
            type.return_type,
            depth + 1u);
        if (return_type == context->type_error ||
            return_type == VITTE_SEMA_INVALID_TYPE_ID) {
            free(parameters);
            return context->type_error;
        }
        if (return_type != type.return_type) { changed = true; }
        result = changed
            ? vitte_sema_create_function_type_private(
                  context,
                  parameters,
                  type.function_parameter_count,
                  return_type)
            : type_id;
        free(parameters);
        return result == VITTE_SEMA_INVALID_TYPE_ID
            ? context->type_error
            : result;
    }
    return type_id;
}

static vitte_symbol_id_t
vitte_sema_register_declaration(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id);

static vitte_scope_id_t
vitte_sema_implementation_scope_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *implementation,
    vitte_scope_id_t fallback_scope);

static bool
vitte_sema_generic_call_check_constraints_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *declaration,
    const vitte_sema_generic_call_binding_private_t *bindings,
    size_t binding_count,
    const vitte_ast_node_t *call_node,
    size_t constraint_depth);

static bool
vitte_sema_generic_trait_implemented_private(
    vitte_sema_t *context,
    vitte_sema_type_id_t trait_type_id,
    vitte_sema_type_id_t concrete_type_id,
    vitte_scope_id_t fallback_scope,
    size_t constraint_depth)
{
    const vitte_sema_type_t *trait_type;
    const vitte_symbol_t *trait_symbol;
    vitte_ast_node_id_t node_id;
    vitte_ast_node_id_t node_count;

    trait_type = vitte_sema_type_private(context, trait_type_id);
    if (constraint_depth > 64u || trait_type == NULL ||
        trait_type->kind != VITTE_SEMA_TYPE_NAMED ||
        trait_type->symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return false;
    }
    trait_symbol = vitte_scope_get_symbol(
        &context->scopes, trait_type->symbol_id);
    if (trait_symbol == NULL ||
        trait_symbol->kind != VITTE_SYMBOL_KIND_TRAIT) {
        return false;
    }

    node_count = vitte_parser_node_count(context->parser);
    for (node_id = 1u; node_id <= node_count; ++node_id) {
        const vitte_ast_node_t *implementation =
            vitte_sema_node(context, node_id);
        const vitte_ast_node_t *header_types[2] = { NULL, NULL };
        size_t header_count = 0u;
        size_t child_index;
        vitte_scope_id_t implementation_scope;
        size_t generic_count = 0u;

        if (implementation == NULL ||
            implementation->kind != VITTE_AST_NODE_IMPL_DECL) {
            continue;
        }
        for (child_index = 0u;
             child_index < implementation->child_count;
             ++child_index) {
            const vitte_ast_node_t *child = vitte_sema_node(
                context, implementation->children[child_index]);
            if (child == NULL ||
                (child->kind != VITTE_AST_NODE_TYPE_EXPR &&
                 child->kind != VITTE_AST_NODE_POINTER_TYPE &&
                 child->kind != VITTE_AST_NODE_REFERENCE_TYPE &&
                 child->kind != VITTE_AST_NODE_ARRAY_TYPE &&
                 child->kind != VITTE_AST_NODE_OPTIONAL_TYPE)) {
                continue;
            }
            if (header_count < 2u) {
                header_types[header_count] = child;
            }
            ++header_count;
        }
        if (header_count != 2u || header_types[0] == NULL ||
            header_types[1] == NULL) {
            continue;
        }
        implementation_scope = vitte_sema_implementation_scope_private(
            context, implementation, fallback_scope);
        if (implementation_scope == VITTE_SCOPE_INVALID_SCOPE_ID) {
            continue;
        }
        for (child_index = 0u;
             child_index < implementation->child_count;
             ++child_index) {
            const vitte_ast_node_t *child = vitte_sema_node(
                context, implementation->children[child_index]);
            if (child != NULL &&
                child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
                ++generic_count;
            }
        }
        {
            vitte_sema_type_id_t implemented_trait =
                vitte_sema_analyze_type_node(
                    context, header_types[0]->id, implementation_scope);
            vitte_sema_type_id_t implemented_type =
                vitte_sema_analyze_type_node(
                    context, header_types[1]->id, implementation_scope);
            bool matches = false;
            vitte_sema_generic_call_binding_private_t *bindings = NULL;
            size_t binding_count = 0u;
            if (generic_count == 0u) {
                matches = vitte_sema_type_identical_private(
                        context, trait_type_id, implemented_trait) &&
                    vitte_sema_type_identical_private(
                        context, concrete_type_id, implemented_type);
            } else {
                bindings =
                    (vitte_sema_generic_call_binding_private_t *)calloc(
                        generic_count, sizeof(*bindings));
                if (bindings == NULL) {
                    context->last_error =
                        VITTE_SEMA_ERROR_OUT_OF_MEMORY;
                    return false;
                }
                matches =
                    vitte_sema_generic_call_unify_type_private(
                        context,
                        bindings,
                        &binding_count,
                        generic_count,
                        implemented_trait,
                        trait_type_id,
                        header_types[0],
                        false,
                        0u) &&
                    vitte_sema_generic_call_unify_type_private(
                        context,
                        bindings,
                        &binding_count,
                        generic_count,
                        implemented_type,
                        concrete_type_id,
                        header_types[1],
                        false,
                        0u) &&
                    binding_count == generic_count;
            }
            if (matches) {
                size_t diagnostic_count = context->diagnostic_count;
                if (vitte_sema_generic_call_check_constraints_private(
                        context,
                        implementation,
                        bindings,
                        binding_count,
                        header_types[1],
                        constraint_depth + 1u)) {
                    free(bindings);
                    return true;
                }
                context->diagnostic_count = diagnostic_count;
            }
            free(bindings);
        }
    }
    return false;
}

static vitte_scope_id_t
vitte_sema_implementation_scope_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *implementation,
    vitte_scope_id_t fallback_scope)
{
    vitte_sema_node_info_t *implementation_info;
    vitte_scope_id_t implementation_scope;
    size_t index;

    if (context == NULL || implementation == NULL) {
        return VITTE_SCOPE_INVALID_SCOPE_ID;
    }
    implementation_info = vitte_sema_node_info_mut_private(
        context, implementation->id);
    implementation_scope = implementation_info != NULL
        ? implementation_info->scope_id
        : VITTE_SCOPE_INVALID_SCOPE_ID;
    if (implementation_scope == VITTE_SCOPE_INVALID_SCOPE_ID ||
        implementation_scope == fallback_scope) {
        for (index = 0u; index < implementation->child_count; ++index) {
            const vitte_ast_node_t *generic = vitte_sema_node(
                context, implementation->children[index]);
            const vitte_sema_node_info_t *generic_info;
            if (generic == NULL ||
                generic->kind != VITTE_AST_NODE_GENERIC_PARAM) {
                continue;
            }
            generic_info = vitte_sema_node_info_private(
                context, generic->id);
            if (generic_info != NULL &&
                generic_info->scope_id !=
                    VITTE_SCOPE_INVALID_SCOPE_ID &&
                generic_info->scope_id != fallback_scope) {
                implementation_scope = generic_info->scope_id;
                break;
            }
        }
    }
    if (implementation_scope == VITTE_SCOPE_INVALID_SCOPE_ID ||
        implementation_scope == fallback_scope) {
        implementation_scope = vitte_scope_create(
            &context->scopes,
            fallback_scope,
            VITTE_SCOPE_KIND_IMPL,
            vitte_sema_scope_span_from_parser(implementation->span),
            (void *)implementation);
        if (implementation_scope == VITTE_SCOPE_INVALID_SCOPE_ID) {
            return implementation_scope;
        }
        if (implementation_info != NULL) {
            implementation_info->scope_id = implementation_scope;
            implementation_info->type_id = context->type_void;
        }
    }
    if (implementation_info != NULL) {
        implementation_info->scope_id = implementation_scope;
    }
    for (index = 0u; index < implementation->child_count; ++index) {
        const vitte_ast_node_t *generic = vitte_sema_node(
            context, implementation->children[index]);
        vitte_sema_node_info_t *generic_info;
        vitte_symbol_id_t generic_symbol;
        vitte_sema_type_id_t generic_type;
        if (generic == NULL ||
            generic->kind != VITTE_AST_NODE_GENERIC_PARAM) {
            continue;
        }
        generic_info = vitte_sema_node_info_mut_private(
            context, generic->id);
        if (generic_info != NULL &&
            generic_info->symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID &&
            generic_info->scope_id == implementation_scope) {
            continue;
        }
        generic_symbol = vitte_sema_register_declaration(
            context, generic, implementation_scope);
        if (generic_symbol == VITTE_SCOPE_INVALID_SYMBOL_ID ||
            generic_info == NULL) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }
        generic_type = vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_GENERIC_PARAMETER,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            generic_symbol,
            false);
        if (generic_type == VITTE_SEMA_INVALID_TYPE_ID) {
            return VITTE_SCOPE_INVALID_SCOPE_ID;
        }
        generic_info->symbol_id = generic_symbol;
        generic_info->scope_id = implementation_scope;
        generic_info->type_id = generic_type;
        generic_info->flags |= VITTE_SEMA_NODE_FLAG_DECLARATION;
    }
    return implementation_scope;
}

static size_t
vitte_sema_impl_generic_parameter_ordinal_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *implementation,
    vitte_symbol_id_t symbol_id)
{
    size_t ordinal = 0u;
    size_t index;
    if (context == NULL || implementation == NULL ||
        symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return SIZE_MAX;
    }
    for (index = 0u; index < implementation->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, implementation->children[index]);
        if (child == NULL || child->kind != VITTE_AST_NODE_GENERIC_PARAM) {
            continue;
        }
        {
            const vitte_sema_node_info_t *info =
                vitte_sema_node_info_private(context, child->id);
            if (info != NULL && info->symbol_id == symbol_id) {
                return ordinal;
            }
        }
        ++ordinal;
    }
    return SIZE_MAX;
}

static bool
vitte_sema_impl_type_pattern_identical_private(
    vitte_sema_t *context,
    vitte_sema_type_id_t left_id,
    const vitte_ast_node_t *left_impl,
    vitte_sema_type_id_t right_id,
    const vitte_ast_node_t *right_impl,
    size_t depth)
{
    const vitte_sema_type_t *left = vitte_sema_type_private(context, left_id);
    const vitte_sema_type_t *right = vitte_sema_type_private(context, right_id);
    size_t index;
    if (left_id == right_id) { return true; }
    if (left == NULL || right == NULL || left->kind != right->kind ||
        depth >= context->max_recursion_depth) {
        return false;
    }
    if (left->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
        size_t left_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
            context, left_impl, left->symbol_id);
        size_t right_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
            context, right_impl, right->symbol_id);
        return left_ordinal != SIZE_MAX && right_ordinal != SIZE_MAX &&
               left_ordinal == right_ordinal;
    }
    if (left->kind == VITTE_SEMA_TYPE_NAMED) {
        if (!vitte_sema_type_same_nominal_private(context, left_id, right_id) ||
            left->generic_argument_count != right->generic_argument_count) {
            return false;
        }
        for (index = 0u; index < left->generic_argument_count; ++index) {
            if (!vitte_sema_impl_type_pattern_identical_private(
                    context, left->generic_arguments[index], left_impl,
                    right->generic_arguments[index], right_impl, depth + 1u)) {
                return false;
            }
        }
        return true;
    }
    if (left->kind == VITTE_SEMA_TYPE_FUNCTION) {
        if (left->function_parameter_count != right->function_parameter_count ||
            !vitte_sema_impl_type_pattern_identical_private(
                context, left->return_type, left_impl,
                right->return_type, right_impl, depth + 1u)) {
            return false;
        }
        for (index = 0u; index < left->function_parameter_count; ++index) {
            if (!vitte_sema_impl_type_pattern_identical_private(
                    context, left->function_parameter_types[index], left_impl,
                    right->function_parameter_types[index], right_impl,
                    depth + 1u)) {
                return false;
            }
        }
        return true;
    }
    switch (left->kind) {
        case VITTE_SEMA_TYPE_POINTER:
        case VITTE_SEMA_TYPE_REFERENCE:
        case VITTE_SEMA_TYPE_OPTIONAL:
        case VITTE_SEMA_TYPE_ARRAY:
        case VITTE_SEMA_TYPE_SLICE:
        case VITTE_SEMA_TYPE_RANGE:
        case VITTE_SEMA_TYPE_TASK:
            return left->array_length == right->array_length &&
                   left->is_mutable == right->is_mutable &&
                   vitte_sema_impl_type_pattern_identical_private(
                       context, left->element_type, left_impl,
                       right->element_type, right_impl, depth + 1u);
        default:
            return true;
    }
}

static bool
vitte_sema_impl_type_occurs_private(
    vitte_sema_t *context,
    vitte_sema_type_id_t needle_id,
    vitte_sema_type_id_t type_id,
    const vitte_ast_node_t *current_impl,
    const vitte_ast_node_t *candidate_impl,
    const vitte_sema_type_id_t *current_substitutions,
    const vitte_sema_type_id_t *candidate_substitutions,
    size_t depth)
{
    const vitte_sema_type_t *type;
    size_t index;
    if (type_id == needle_id) { return true; }
    if (depth >= context->max_recursion_depth) { return true; }
    type = vitte_sema_type_private(context, type_id);
    if (type == NULL) { return false; }
    if (type->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
        size_t ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
            context, current_impl, type->symbol_id);
        if (ordinal != SIZE_MAX && current_substitutions != NULL &&
            current_substitutions[ordinal] != VITTE_SEMA_INVALID_TYPE_ID) {
            return vitte_sema_impl_type_occurs_private(
                context, needle_id, current_substitutions[ordinal],
                current_impl, candidate_impl, current_substitutions,
                candidate_substitutions, depth + 1u);
        }
        ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
            context, candidate_impl, type->symbol_id);
        if (ordinal != SIZE_MAX && candidate_substitutions != NULL &&
            candidate_substitutions[ordinal] != VITTE_SEMA_INVALID_TYPE_ID) {
            return vitte_sema_impl_type_occurs_private(
                context, needle_id, candidate_substitutions[ordinal],
                current_impl, candidate_impl, current_substitutions,
                candidate_substitutions, depth + 1u);
        }
        return false;
    }
    if (type->kind == VITTE_SEMA_TYPE_NAMED) {
        for (index = 0u; index < type->generic_argument_count; ++index) {
            if (vitte_sema_impl_type_occurs_private(
                    context, needle_id, type->generic_arguments[index],
                    current_impl, candidate_impl, current_substitutions,
                    candidate_substitutions, depth + 1u)) {
                return true;
            }
        }
        return false;
    }
    if (type->kind == VITTE_SEMA_TYPE_FUNCTION) {
        if (vitte_sema_impl_type_occurs_private(
                context, needle_id, type->return_type, current_impl,
                candidate_impl, current_substitutions,
                candidate_substitutions, depth + 1u)) {
            return true;
        }
        for (index = 0u; index < type->function_parameter_count; ++index) {
            if (vitte_sema_impl_type_occurs_private(
                    context, needle_id, type->function_parameter_types[index],
                    current_impl, candidate_impl, current_substitutions,
                    candidate_substitutions, depth + 1u)) {
                return true;
            }
        }
        return false;
    }
    switch (type->kind) {
        case VITTE_SEMA_TYPE_POINTER:
        case VITTE_SEMA_TYPE_REFERENCE:
        case VITTE_SEMA_TYPE_OPTIONAL:
        case VITTE_SEMA_TYPE_ARRAY:
        case VITTE_SEMA_TYPE_SLICE:
        case VITTE_SEMA_TYPE_RANGE:
        case VITTE_SEMA_TYPE_TASK:
            return vitte_sema_impl_type_occurs_private(
                context, needle_id, type->element_type, current_impl,
                candidate_impl, current_substitutions,
                candidate_substitutions, depth + 1u);
        default:
            return false;
    }
}

static bool
vitte_sema_impl_types_overlap_private(
    vitte_sema_t *context,
    vitte_sema_type_id_t left_id,
    const vitte_ast_node_t *current_impl,
    vitte_sema_type_id_t right_id,
    const vitte_ast_node_t *candidate_impl,
    vitte_sema_type_id_t *current_substitutions,
    vitte_sema_type_id_t *candidate_substitutions,
    size_t depth)
{
    const vitte_sema_type_t *left;
    const vitte_sema_type_t *right;
    size_t left_current_ordinal = SIZE_MAX;
    size_t left_candidate_ordinal = SIZE_MAX;
    size_t right_current_ordinal = SIZE_MAX;
    size_t right_candidate_ordinal = SIZE_MAX;
    size_t index;

    if (depth >= context->max_recursion_depth) { return false; }
    for (index = 0u; index < depth + 1u; ++index) {
        bool substituted = false;
        left = vitte_sema_type_private(context, left_id);
        right = vitte_sema_type_private(context, right_id);
        if (left != NULL && left->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
            left_current_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
                context, current_impl, left->symbol_id);
            left_candidate_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
                context, candidate_impl, left->symbol_id);
            if (left_current_ordinal != SIZE_MAX && current_substitutions != NULL &&
                current_substitutions[left_current_ordinal] != VITTE_SEMA_INVALID_TYPE_ID) {
                left_id = current_substitutions[left_current_ordinal];
                substituted = true;
            } else if (left_candidate_ordinal != SIZE_MAX && candidate_substitutions != NULL &&
                       candidate_substitutions[left_candidate_ordinal] != VITTE_SEMA_INVALID_TYPE_ID) {
                left_id = candidate_substitutions[left_candidate_ordinal];
                substituted = true;
            }
        }
        if (right != NULL && right->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
            right_current_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
                context, current_impl, right->symbol_id);
            right_candidate_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
                context, candidate_impl, right->symbol_id);
            if (right_current_ordinal != SIZE_MAX && current_substitutions != NULL &&
                current_substitutions[right_current_ordinal] != VITTE_SEMA_INVALID_TYPE_ID) {
                right_id = current_substitutions[right_current_ordinal];
                substituted = true;
            } else if (right_candidate_ordinal != SIZE_MAX && candidate_substitutions != NULL &&
                       candidate_substitutions[right_candidate_ordinal] != VITTE_SEMA_INVALID_TYPE_ID) {
                right_id = candidate_substitutions[right_candidate_ordinal];
                substituted = true;
            }
        }
        if (!substituted) { break; }
    }
    if (left_id == right_id) { return true; }
    left = vitte_sema_type_private(context, left_id);
    right = vitte_sema_type_private(context, right_id);
    if (left == NULL || right == NULL) { return false; }
    if (left->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
        left_current_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
            context, current_impl, left->symbol_id);
        left_candidate_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
            context, candidate_impl, left->symbol_id);
        if (left_current_ordinal != SIZE_MAX && current_substitutions != NULL) {
            if (vitte_sema_impl_type_occurs_private(
                    context, left_id, right_id, current_impl, candidate_impl,
                    current_substitutions, candidate_substitutions, depth + 1u)) {
                return false;
            }
            current_substitutions[left_current_ordinal] = right_id;
            return true;
        }
        if (left_candidate_ordinal != SIZE_MAX && candidate_substitutions != NULL) {
            if (vitte_sema_impl_type_occurs_private(
                    context, left_id, right_id, current_impl, candidate_impl,
                    current_substitutions, candidate_substitutions, depth + 1u)) {
                return false;
            }
            candidate_substitutions[left_candidate_ordinal] = right_id;
            return true;
        }
        return right->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER &&
               left->symbol_id == right->symbol_id;
    }
    if (right->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER) {
        right_current_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
            context, current_impl, right->symbol_id);
        right_candidate_ordinal = vitte_sema_impl_generic_parameter_ordinal_private(
            context, candidate_impl, right->symbol_id);
        if (right_current_ordinal != SIZE_MAX && current_substitutions != NULL) {
            if (vitte_sema_impl_type_occurs_private(
                    context, right_id, left_id, current_impl, candidate_impl,
                    current_substitutions, candidate_substitutions, depth + 1u)) {
                return false;
            }
            current_substitutions[right_current_ordinal] = left_id;
            return true;
        }
        if (right_candidate_ordinal != SIZE_MAX && candidate_substitutions != NULL) {
            if (vitte_sema_impl_type_occurs_private(
                    context, right_id, left_id, current_impl, candidate_impl,
                    current_substitutions, candidate_substitutions, depth + 1u)) {
                return false;
            }
            candidate_substitutions[right_candidate_ordinal] = left_id;
            return true;
        }
        return false;
    }
    if (left->kind != right->kind) { return false; }
    if (left->kind == VITTE_SEMA_TYPE_NAMED) {
        if (!vitte_sema_type_same_nominal_private(context, left_id, right_id) ||
            left->generic_argument_count != right->generic_argument_count) {
            return false;
        }
        for (index = 0u; index < left->generic_argument_count; ++index) {
            if (!vitte_sema_impl_types_overlap_private(
                    context, left->generic_arguments[index], current_impl,
                    right->generic_arguments[index], candidate_impl,
                    current_substitutions, candidate_substitutions, depth + 1u)) {
                return false;
            }
        }
        return true;
    }
    if (left->kind == VITTE_SEMA_TYPE_FUNCTION) {
        if (left->function_parameter_count != right->function_parameter_count ||
            !vitte_sema_impl_types_overlap_private(
                context, left->return_type, current_impl,
                right->return_type, candidate_impl,
                current_substitutions, candidate_substitutions, depth + 1u)) {
            return false;
        }
        for (index = 0u; index < left->function_parameter_count; ++index) {
            if (!vitte_sema_impl_types_overlap_private(
                    context, left->function_parameter_types[index], current_impl,
                    right->function_parameter_types[index], candidate_impl,
                    current_substitutions, candidate_substitutions, depth + 1u)) {
                return false;
            }
        }
        return true;
    }
    switch (left->kind) {
        case VITTE_SEMA_TYPE_POINTER:
        case VITTE_SEMA_TYPE_REFERENCE:
        case VITTE_SEMA_TYPE_OPTIONAL:
        case VITTE_SEMA_TYPE_ARRAY:
        case VITTE_SEMA_TYPE_SLICE:
        case VITTE_SEMA_TYPE_RANGE:
        case VITTE_SEMA_TYPE_TASK:
            return left->array_length == right->array_length &&
                   left->is_mutable == right->is_mutable &&
                   vitte_sema_impl_types_overlap_private(
                       context, left->element_type, current_impl,
                       right->element_type, candidate_impl,
                       current_substitutions, candidate_substitutions,
                       depth + 1u);
        default:
            return left->symbol_id == right->symbol_id;
    }
}

static const vitte_ast_node_t *
vitte_sema_prior_overlapping_trait_impl_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *current,
    vitte_scope_id_t fallback_scope,
    vitte_sema_type_id_t trait_type,
    vitte_sema_type_id_t target_type)
{
    size_t node_id;
    size_t current_generic_count = 0u;
    size_t child_index;

    if (context == NULL || current == NULL) { return NULL; }
    for (child_index = 0u; child_index < current->child_count; ++child_index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, current->children[child_index]);
        if (child != NULL && child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
            ++current_generic_count;
        }
    }
    for (node_id = 1u; node_id < current->id; ++node_id) {
        const vitte_ast_node_t *candidate = vitte_sema_node(
            context, (vitte_ast_node_id_t)node_id);
        const vitte_ast_node_t *header_types[2] = {NULL, NULL};
        size_t header_count = 0u;
        size_t generic_count = 0u;
        size_t index;
        vitte_scope_id_t candidate_scope;
        vitte_sema_type_id_t candidate_trait;
        vitte_sema_type_id_t candidate_target;
        vitte_sema_type_id_t *current_substitutions = NULL;
        vitte_sema_type_id_t *candidate_substitutions = NULL;
        bool overlaps;

        if (candidate == NULL ||
            candidate->kind != VITTE_AST_NODE_IMPL_DECL) {
            continue;
        }
        for (index = 0u; index < candidate->child_count; ++index) {
            const vitte_ast_node_t *child = vitte_sema_node(
                context, candidate->children[index]);
            if (child == NULL) { continue; }
            if (child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
                ++generic_count;
            } else if (child->kind == VITTE_AST_NODE_TYPE_EXPR ||
                       child->kind == VITTE_AST_NODE_POINTER_TYPE ||
                       child->kind == VITTE_AST_NODE_REFERENCE_TYPE ||
                       child->kind == VITTE_AST_NODE_ARRAY_TYPE ||
                       child->kind == VITTE_AST_NODE_OPTIONAL_TYPE) {
                if (header_count < 2u) { header_types[header_count] = child; }
                ++header_count;
            }
        }
        if (generic_count != current_generic_count || header_count != 2u ||
            header_types[0] == NULL || header_types[1] == NULL) {
            continue;
        }
        candidate_scope = vitte_sema_implementation_scope_private(
            context, candidate, fallback_scope);
        if (candidate_scope == VITTE_SCOPE_INVALID_SCOPE_ID) { continue; }
        candidate_trait = vitte_sema_analyze_type_node(
            context, header_types[0]->id, candidate_scope);
        candidate_target = vitte_sema_analyze_type_node(
            context, header_types[1]->id, candidate_scope);
        if (!vitte_sema_impl_type_pattern_identical_private(
                context, trait_type, current,
                candidate_trait, candidate, 0u) ||
            !vitte_sema_impl_type_pattern_identical_private(
                context, target_type, current,
                candidate_target, candidate, 0u)) {
            continue;
        }
        if (current_generic_count != 0u) {
            current_substitutions = (vitte_sema_type_id_t *)calloc(
                current_generic_count, sizeof(*current_substitutions));
            if (current_substitutions == NULL) {
                context->last_error = VITTE_SEMA_ERROR_OUT_OF_MEMORY;
                continue;
            }
        }
        if (generic_count != 0u) {
            candidate_substitutions = (vitte_sema_type_id_t *)calloc(
                generic_count, sizeof(*candidate_substitutions));
            if (candidate_substitutions == NULL) {
                free(current_substitutions);
                context->last_error = VITTE_SEMA_ERROR_OUT_OF_MEMORY;
                continue;
            }
        }
        /* Unify both headers independently: differing generic arities and
         * parameter names can still describe intersecting implementation
         * sets (for example Box<T> and Box<i64>). */
        overlaps = vitte_sema_impl_types_overlap_private(
            context, trait_type, current, candidate_trait, candidate,
            current_substitutions, candidate_substitutions, 0u);
        if (overlaps) {
            overlaps = vitte_sema_impl_types_overlap_private(
                context, target_type, current, candidate_target, candidate,
                current_substitutions, candidate_substitutions, 0u);
        }
        free(current_substitutions);
        free(candidate_substitutions);
        if (overlaps) {
            return candidate;
        }
    }
    return NULL;
}

static bool
vitte_sema_generic_type_has_trait_assumption_private(
    vitte_sema_t *context,
    vitte_sema_type_id_t generic_type_id,
    vitte_sema_type_id_t trait_type_id)
{
    const vitte_sema_type_t *generic_type =
        vitte_sema_type_private(context, generic_type_id);
    vitte_ast_node_id_t node_id;
    vitte_ast_node_id_t node_count;

    if (generic_type == NULL ||
        generic_type->kind != VITTE_SEMA_TYPE_GENERIC_PARAMETER ||
        generic_type->symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return false;
    }
    node_count = vitte_parser_node_count(context->parser);
    for (node_id = 1u; node_id <= node_count; ++node_id) {
        const vitte_ast_node_t *generic =
            vitte_sema_node(context, node_id);
        const vitte_sema_node_info_t *generic_info;
        size_t bound_index;

        if (generic == NULL ||
            generic->kind != VITTE_AST_NODE_GENERIC_PARAM) {
            continue;
        }
        generic_info = vitte_sema_node_info_private(
            context, generic->id);
        if (generic_info == NULL ||
            generic_info->symbol_id != generic_type->symbol_id) {
            continue;
        }
        for (bound_index = 1u;
             bound_index < generic->child_count;
             ++bound_index) {
            const vitte_ast_node_t *bound = vitte_sema_node(
                context, generic->children[bound_index]);
            vitte_sema_type_id_t bound_type = bound != NULL
                ? vitte_sema_analyze_type_node(
                      context, bound->id, generic_info->scope_id)
                : context->type_error;
            if (vitte_sema_type_identical_private(
                    context, bound_type, trait_type_id)) {
                return true;
            }
        }

        {
            vitte_ast_node_id_t owner_id;
            for (owner_id = 1u; owner_id <= node_count; ++owner_id) {
                const vitte_ast_node_t *owner =
                    vitte_sema_node(context, owner_id);
                size_t owner_index;
                bool owns_generic = false;
                if (owner == NULL) { continue; }
                for (owner_index = 0u;
                     owner_index < owner->child_count;
                     ++owner_index) {
                    if (owner->children[owner_index] == generic->id) {
                        owns_generic = true;
                        break;
                    }
                }
                if (!owns_generic) { continue; }
                for (owner_index = 0u;
                     owner_index < owner->child_count;
                     ++owner_index) {
                    const vitte_ast_node_t *clause = vitte_sema_node(
                        context, owner->children[owner_index]);
                    size_t predicate_index;
                    if (clause == NULL ||
                        clause->kind != VITTE_AST_NODE_WHERE_CLAUSE) {
                        continue;
                    }
                    for (predicate_index = 0u;
                         predicate_index < clause->child_count;
                         ++predicate_index) {
                        const vitte_ast_node_t *predicate =
                            vitte_sema_node(
                                context,
                                clause->children[predicate_index]);
                        const vitte_ast_node_t *subject;
                        vitte_sema_type_id_t subject_type;
                        size_t trait_index;
                        if (predicate == NULL ||
                            predicate->kind !=
                                VITTE_AST_NODE_WHERE_PREDICATE ||
                            predicate->operator_kind !=
                                VITTE_TOKEN_COLON ||
                            predicate->child_count < 2u) {
                            continue;
                        }
                        subject = vitte_sema_node(
                            context, predicate->children[0]);
                        subject_type = subject != NULL
                            ? vitte_sema_analyze_type_node(
                                  context,
                                  subject->id,
                                  generic_info->scope_id)
                            : context->type_error;
                        if (!vitte_sema_type_identical_private(
                                context,
                                subject_type,
                                generic_type_id)) {
                            continue;
                        }
                        for (trait_index = 1u;
                             trait_index < predicate->child_count;
                             ++trait_index) {
                            const vitte_ast_node_t *bound =
                                vitte_sema_node(
                                    context,
                                    predicate->children[trait_index]);
                            vitte_sema_type_id_t bound_type = bound != NULL
                                ? vitte_sema_analyze_type_node(
                                      context,
                                      bound->id,
                                      generic_info->scope_id)
                                : context->type_error;
                            if (vitte_sema_type_identical_private(
                                    context,
                                    bound_type,
                                    trait_type_id)) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

static bool
vitte_sema_generic_call_check_trait_bound_private(
    vitte_sema_t *context,
    const vitte_sema_generic_call_binding_private_t *bindings,
    size_t binding_count,
    vitte_sema_type_id_t subject_type_id,
    const vitte_ast_node_t *bound_node,
    vitte_scope_id_t generic_scope,
    const vitte_ast_node_t *call_node,
    size_t constraint_depth)
{
    vitte_sema_type_id_t trait_type_id;
    vitte_sema_type_id_t concrete_type_id;
    const vitte_sema_type_t *trait_type;
    const vitte_symbol_t *trait_symbol;
    const vitte_ast_node_t *diagnostic_node;

    if (context == NULL || bound_node == NULL || call_node == NULL ||
        constraint_depth > 64u) {
        return false;
    }
    trait_type_id = vitte_sema_analyze_type_node(
        context, bound_node->id, generic_scope);
    trait_type = vitte_sema_type_private(context, trait_type_id);
    trait_symbol = trait_type != NULL &&
            trait_type->kind == VITTE_SEMA_TYPE_NAMED
        ? vitte_scope_get_symbol(&context->scopes, trait_type->symbol_id)
        : NULL;
    concrete_type_id =
        vitte_sema_generic_call_substitute_type_private(
            context, bindings, binding_count, subject_type_id, 0u);
    if (trait_symbol == NULL ||
        trait_symbol->kind != VITTE_SYMBOL_KIND_TRAIT) {
        if (trait_type_id != context->type_error &&
            vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE,
                bound_node->span,
                bound_node->id,
                trait_type != NULL
                    ? trait_type->symbol_id
                    : VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                trait_type_id)) {
            vitte_sema_append_diagnostic_help(
                &context->diagnostics[context->diagnostic_count - 1u],
                "A generic bound must name a trait, not an ordinary type.");
        }
        return false;
    }
    if (concrete_type_id == context->type_error) { return false; }
    if (!vitte_sema_generic_trait_implemented_private(
             context, trait_type_id, concrete_type_id, generic_scope,
             constraint_depth) &&
         !vitte_sema_generic_type_has_trait_assumption_private(
             context, concrete_type_id, trait_type_id)) {
        diagnostic_node = call_node;
        if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_GENERIC_BOUND_UNSATISFIED,
                diagnostic_node->span,
                diagnostic_node->id,
                trait_type != NULL
                    ? trait_type->symbol_id
                    : VITTE_SCOPE_INVALID_SYMBOL_ID,
                trait_type_id,
                concrete_type_id)) {
            vitte_sema_diagnostic_t *diagnostic =
                &context->diagnostics[context->diagnostic_count - 1u];
            (void)snprintf(
                diagnostic->help,
                sizeof(diagnostic->help),
                "The concrete type does not implement the required trait bound.");
            vitte_sema_set_last_related_location(
                context,
                bound_node,
                "required trait bound introduced here");
        }
        return false;
    }
    return true;
}

static bool
vitte_sema_generic_call_check_constraints_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *declaration,
    const vitte_sema_generic_call_binding_private_t *bindings,
    size_t binding_count,
    const vitte_ast_node_t *call_node,
    size_t constraint_depth)
{
    vitte_scope_id_t generic_scope = VITTE_SCOPE_INVALID_SCOPE_ID;
    size_t index;
    bool valid = true;

    if (context == NULL || declaration == NULL || call_node == NULL ||
        constraint_depth > 64u) {
        return false;
    }
    for (index = 0u; index < declaration->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, declaration->children[index]);
        const vitte_sema_node_info_t *info;
        if (child == NULL ||
            child->kind != VITTE_AST_NODE_GENERIC_PARAM) {
            continue;
        }
        info = vitte_sema_node_info_private(context, child->id);
        if (info != NULL && info->scope_id !=
                VITTE_SCOPE_INVALID_SCOPE_ID) {
            generic_scope = info->scope_id;
            break;
        }
    }
    if (generic_scope == VITTE_SCOPE_INVALID_SCOPE_ID) {
        const vitte_sema_node_info_t *info =
            vitte_sema_node_info_private(context, declaration->id);
        generic_scope = info != NULL
            ? info->scope_id
            : VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    for (index = 0u; index < declaration->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, declaration->children[index]);
        if (child != NULL &&
            child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
            const vitte_sema_node_info_t *parameter_info =
                vitte_sema_node_info_private(context, child->id);
            size_t bound_index;
            for (bound_index = 1u;
                 bound_index < child->child_count;
                 ++bound_index) {
                const vitte_ast_node_t *bound = vitte_sema_node(
                    context, child->children[bound_index]);
                if (parameter_info == NULL ||
                    !vitte_sema_generic_call_check_trait_bound_private(
                        context,
                        bindings,
                        binding_count,
                        parameter_info->type_id,
                        bound,
                        generic_scope,
                        call_node,
                        constraint_depth)) {
                    valid = false;
                }
            }
        } else if (child != NULL &&
                   child->kind == VITTE_AST_NODE_WHERE_CLAUSE) {
            size_t predicate_index;
            for (predicate_index = 0u;
                 predicate_index < child->child_count;
                 ++predicate_index) {
                const vitte_ast_node_t *predicate = vitte_sema_node(
                    context, child->children[predicate_index]);
                const vitte_ast_node_t *subject;
                vitte_sema_type_id_t subject_type;
                size_t bound_index;
                if (predicate == NULL ||
                    predicate->kind != VITTE_AST_NODE_WHERE_PREDICATE ||
                    predicate->operator_kind != VITTE_TOKEN_COLON ||
                    predicate->child_count < 2u) {
                    if (predicate != NULL &&
                        vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_WHERE_PREDICATE,
                            predicate->span,
                            predicate->id,
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID)) {
                        vitte_sema_append_diagnostic_help(
                            &context->diagnostics[
                                context->diagnostic_count - 1u],
                            "A where predicate must have the form `Type: Trait`.");
                    }
                    valid = false;
                    continue;
                }
                subject = vitte_sema_node(
                    context, predicate->children[0]);
                subject_type = subject != NULL
                    ? vitte_sema_analyze_type_node(
                          context, subject->id, generic_scope)
                    : context->type_error;
                for (bound_index = 1u;
                     bound_index < predicate->child_count;
                     ++bound_index) {
                    const vitte_ast_node_t *bound = vitte_sema_node(
                        context, predicate->children[bound_index]);
                    if (!vitte_sema_generic_call_check_trait_bound_private(
                            context,
                            bindings,
                            binding_count,
                            subject_type,
                            bound,
                            generic_scope,
                            call_node,
                            constraint_depth)) {
                        valid = false;
                    }
                }
            }
        }
    }
    return valid;
}

static bool
vitte_sema_generic_type_application_check_constraints_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *type_node,
    vitte_symbol_id_t symbol_id,
    vitte_scope_id_t parent_scope)
{
    const vitte_symbol_t *symbol;
    const vitte_ast_node_t *declaration;
    vitte_scope_id_t generic_scope;
    size_t generic_count;
    vitte_sema_generic_call_binding_private_t *bindings;
    size_t index;
    bool has_constraints = false;
    bool valid;

    if (context == NULL || type_node == NULL ||
        type_node->kind != VITTE_AST_NODE_TYPE_EXPR ||
        type_node->child_count < 2u) {
        return true;
    }
    {
        const vitte_sema_node_info_t *prior_info =
            vitte_sema_node_info_private(context, type_node->id);
        if (prior_info != NULL &&
            prior_info->type_id == context->type_error &&
            (prior_info->flags & VITTE_SEMA_NODE_FLAG_RESOLVED) != 0u) {
            return false;
        }
    }
    for (size_t diagnostic_index = 0u;
         diagnostic_index < context->diagnostic_count;
         ++diagnostic_index) {
        const vitte_sema_diagnostic_t *prior =
            &context->diagnostics[diagnostic_index];
        if (prior->kind == VITTE_SEMA_DIAGNOSTIC_ERROR &&
            prior->code ==
                VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE &&
            prior->node_id == type_node->id) {
            return false;
        }
    }
    symbol = vitte_scope_get_symbol(&context->scopes, symbol_id);
    declaration = symbol != NULL
        ? (const vitte_ast_node_t *)symbol->payload
        : NULL;
    if (declaration == NULL ||
        (declaration->kind != VITTE_AST_NODE_FORM_DECL &&
         declaration->kind != VITTE_AST_NODE_TYPE_DECL &&
         declaration->kind != VITTE_AST_NODE_OPAQUE_DECL &&
         declaration->kind != VITTE_AST_NODE_TRAIT_DECL)) {
        return true;
    }
    generic_count = vitte_sema_generic_parameter_count_private(
        context, symbol_id);
    if (generic_count == 0u) { return true; }
    if (generic_count != type_node->child_count - 1u) {
        return true;
    }
    for (size_t child_index = 0u;
         child_index < declaration->child_count;
         ++child_index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, declaration->children[child_index]);
        if (child == NULL) { continue; }
        if ((child->kind == VITTE_AST_NODE_GENERIC_PARAM &&
             child->child_count > 1u) ||
            (child->kind == VITTE_AST_NODE_WHERE_CLAUSE &&
             child->child_count > 0u)) {
            has_constraints = true;
            break;
        }
    }
    /* Unconstrained generic aliases are resolved by the existing alias
     * substitution path. Avoid creating a second generic scope for them:
     * doing so overwrites the declaration's cached parameter bindings. */
    if (!has_constraints) { return true; }
    bindings = (vitte_sema_generic_call_binding_private_t *)calloc(
        generic_count, sizeof(*bindings));
    if (bindings == NULL) {
        context->last_error = VITTE_SEMA_ERROR_OUT_OF_MEMORY;
        return false;
    }
    generic_scope = vitte_scope_create(
        &context->scopes,
        parent_scope,
        VITTE_SCOPE_KIND_FORM,
        vitte_sema_scope_span_from_parser(declaration->span),
        (void *)declaration);
    if (generic_scope == VITTE_SCOPE_INVALID_SCOPE_ID) {
        free(bindings);
        return false;
    }
    index = 0u;
    for (size_t child_index = 0u;
         child_index < declaration->child_count;
         ++child_index) {
        const vitte_ast_node_t *generic = vitte_sema_node(
            context, declaration->children[child_index]);
        vitte_symbol_id_t generic_symbol;
        vitte_sema_type_id_t generic_type;
        vitte_sema_type_id_t actual_type;
        vitte_sema_node_info_t *generic_info;
        if (generic == NULL ||
            generic->kind != VITTE_AST_NODE_GENERIC_PARAM) {
            continue;
        }
        if (index >= generic_count) {
            free(bindings);
            return false;
        }
        generic_symbol = vitte_sema_register_declaration(
            context, generic, generic_scope);
        generic_type = vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_GENERIC_PARAMETER,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            generic_symbol,
            false);
        generic_info = vitte_sema_node_info_mut_private(
            context, generic->id);
        actual_type = vitte_sema_analyze_type_node(
            context,
            type_node->children[index + 1u],
            parent_scope);
        if (generic_symbol == VITTE_SCOPE_INVALID_SYMBOL_ID ||
            generic_type == VITTE_SEMA_INVALID_TYPE_ID ||
            generic_info == NULL || actual_type == context->type_error) {
            free(bindings);
            return false;
        }
        generic_info->symbol_id = generic_symbol;
        generic_info->scope_id = generic_scope;
        generic_info->type_id = generic_type;
        generic_info->flags |= VITTE_SEMA_NODE_FLAG_DECLARATION;
        bindings[index].parameter_type = generic_type;
        bindings[index].concrete_type = actual_type;
        ++index;
    }
    if (index != generic_count) {
        free(bindings);
        return false;
    }
    valid = vitte_sema_generic_call_check_constraints_private(
        context,
        declaration,
        bindings,
        generic_count,
        type_node,
        0u);
    free(bindings);
    return valid;
}

static bool
vitte_sema_generic_constraint_validate_trait_type_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *bound_node,
    vitte_scope_id_t generic_scope)
{
    vitte_sema_type_id_t bound_type_id;
    const vitte_sema_type_t *bound_type;
    const vitte_symbol_t *bound_symbol;
    size_t diagnostic_index;
    bool diagnosed;

    if (context == NULL || bound_node == NULL) { return false; }
    bound_type_id = vitte_sema_analyze_type_node(
        context, bound_node->id, generic_scope);
    if (bound_type_id == context->type_error ||
        bound_type_id == VITTE_SEMA_INVALID_TYPE_ID) {
        diagnosed = false;
        for (diagnostic_index = 0u;
             diagnostic_index < context->diagnostic_count;
             ++diagnostic_index) {
            const vitte_sema_diagnostic_t *diagnostic =
                &context->diagnostics[diagnostic_index];
            if (diagnostic->node_id == bound_node->id &&
                diagnostic->code == VITTE_SEMA_DIAGNOSTIC_UNKNOWN_TYPE) {
                diagnosed = true;
                break;
            }
        }
        if (!diagnosed) {
            if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_UNKNOWN_TYPE,
                bound_node->span,
                bound_node->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
                /* Preserve the unknown-bound diagnostic as the root cause. */
            }
        }
        return false;
    }
    bound_type = vitte_sema_type_private(context, bound_type_id);
    bound_symbol = bound_type != NULL &&
            bound_type->kind == VITTE_SEMA_TYPE_NAMED
        ? vitte_scope_get_symbol(&context->scopes, bound_type->symbol_id)
        : NULL;
    if (bound_symbol == NULL ||
        bound_symbol->kind != VITTE_SYMBOL_KIND_TRAIT) {
        if (vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_NOT_A_TYPE,
            bound_node->span,
            bound_node->id,
            bound_type != NULL
                ? bound_type->symbol_id
                : VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            bound_type_id)) {
            /* A bound must resolve to a trait declaration. */
        }
        return false;
    }
    return true;
}

static bool
vitte_sema_generic_declaration_validate_constraints_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *declaration,
    vitte_scope_id_t generic_scope)
{
    size_t index;
    bool valid = true;

    if (context == NULL || declaration == NULL) { return false; }
    for (index = 0u; index < declaration->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, declaration->children[index]);
        if (child != NULL &&
            child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
            size_t bound_index;
            if ((child->flags & VITTE_AST_FLAG_CONST_GENERIC) != 0u) {
                continue;
            }
            for (bound_index = 1u;
                 bound_index < child->child_count;
                 ++bound_index) {
                if (!vitte_sema_generic_constraint_validate_trait_type_private(
                        context,
                        vitte_sema_node(
                            context, child->children[bound_index]),
                        generic_scope)) {
                    valid = false;
                }
            }
        } else if (child != NULL &&
                   child->kind == VITTE_AST_NODE_WHERE_CLAUSE) {
            size_t predicate_index;
            for (predicate_index = 0u;
                 predicate_index < child->child_count;
                 ++predicate_index) {
                const vitte_ast_node_t *predicate = vitte_sema_node(
                    context, child->children[predicate_index]);
                const vitte_ast_node_t *subject;
                vitte_sema_type_id_t subject_type;
                if (predicate == NULL ||
                    predicate->kind !=
                        VITTE_AST_NODE_WHERE_PREDICATE ||
                    predicate->operator_kind != VITTE_TOKEN_COLON ||
                    predicate->child_count < 2u) {
                    if (predicate != NULL &&
                        predicate->kind ==
                            VITTE_AST_NODE_WHERE_PREDICATE) {
                        (void)vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_WHERE_PREDICATE,
                            predicate->span,
                            predicate->id,
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID);
                    }
                    valid = false;
                    continue;
                }
                subject = vitte_sema_node(
                    context, predicate->children[0]);
                subject_type = subject != NULL
                    ? vitte_sema_analyze_type_node(
                          context, subject->id, generic_scope)
                    : context->type_error;
                if (subject_type == context->type_error) {
                    size_t diagnostic_index;
                    bool diagnosed = false;
                    if (subject != NULL) {
                        for (diagnostic_index = 0u;
                             diagnostic_index <
                                 context->diagnostic_count;
                             ++diagnostic_index) {
                            const vitte_sema_diagnostic_t *diagnostic =
                                &context->diagnostics[diagnostic_index];
                            if (diagnostic->node_id == subject->id &&
                                diagnostic->code ==
                                    VITTE_SEMA_DIAGNOSTIC_UNKNOWN_TYPE) {
                                diagnosed = true;
                                break;
                            }
                        }
                    }
                    if (subject != NULL && !diagnosed) {
                        (void)vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_UNKNOWN_TYPE,
                            subject->span,
                            subject->id,
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID);
                    }
                    valid = false;
                }
                for (size_t bound_index = 1u;
                     bound_index < predicate->child_count;
                     ++bound_index) {
                    if (!vitte_sema_generic_constraint_validate_trait_type_private(
                            context,
                            vitte_sema_node(
                                context,
                                predicate->children[bound_index]),
                            generic_scope)) {
                        valid = false;
                    }
                }
            }
        }
    }
    return valid;
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
        case VITTE_AST_NODE_TYPE_DECL:
        case VITTE_AST_NODE_OPAQUE_DECL:
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
        case VITTE_AST_NODE_FIELD_DECL:
        case VITTE_AST_NODE_VARIANT_DECL:
        case VITTE_AST_NODE_PARAMETER:
        case VITTE_AST_NODE_GENERIC_PARAM:
        case VITTE_AST_NODE_WHERE_CLAUSE:
        case VITTE_AST_NODE_WHERE_PREDICATE:
        case VITTE_AST_NODE_PATTERN:
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
        case VITTE_AST_NODE_OPTIONAL_TYPE:
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
        case VITTE_AST_NODE_SIZEOF_EXPR:
        case VITTE_AST_NODE_ALIGNOF_EXPR:
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
        case VITTE_AST_NODE_PATTERN:
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
        case VITTE_AST_NODE_OPTIONAL_TYPE:
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
        case VITTE_AST_NODE_SIZEOF_EXPR:
        case VITTE_AST_NODE_ALIGNOF_EXPR:
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
        if (child != NULL &&
            (child->kind == VITTE_AST_NODE_PATH ||
             child->kind == VITTE_AST_NODE_TYPE_EXPR) &&
            child->child_count != 0u) {
            const vitte_ast_node_t *nested =
                vitte_sema_find_identifier_child(
                    context,
                    child);
            if (nested != NULL) {
                return nested;
            }
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
    size_t export_index;

    if (context == NULL ||
        name == NULL ||
        name_length == 0u) {
        return false;
    }

    for (export_index = 0u;
         export_index < context->explicit_export_count;
         ++export_index) {
        const vitte_ast_node_t *export_node;
        const vitte_sema_node_info_t *export_info;
        size_t item_index;

        node_id = context->explicit_exports[export_index];
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

static vitte_sema_type_id_t
vitte_sema_builtin_type_from_name(
    const vitte_sema_t *context,
    const char *name,
    size_t name_length);

static bool
vitte_sema_resolve_qualified_import_type(
    vitte_sema_t *context,
    const vitte_ast_node_t *path,
    vitte_scope_id_t requester_scope,
    vitte_symbol_id_t *symbol_id);

static vitte_sema_type_id_t
vitte_sema_member_builtin_function(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    vitte_scope_id_t scope_id,
    vitte_sema_type_id_t return_type)
{
    vitte_sema_type_id_t function_type = vitte_sema_create_type_private(
        context,
        VITTE_SEMA_TYPE_FUNCTION,
        VITTE_SEMA_INVALID_TYPE_ID,
        0u,
        VITTE_SCOPE_INVALID_SYMBOL_ID,
        false);
    vitte_sema_type_t *function =
        vitte_sema_type_mut_private(context, function_type);
    vitte_sema_node_info_t *info;

    if (function == NULL) {
        return context->type_error;
    }
    function->return_type = return_type;
    info = vitte_sema_node_info_mut_private(context, node->id);
    if (info != NULL) {
        info->type_id = function_type;
        info->scope_id = scope_id;
        info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED |
                       VITTE_SEMA_NODE_FLAG_BUILTIN;
    }
    return function_type;
}

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

    info = vitte_sema_node_info_mut_private(context, node_id);

    /* Type nodes are revisited during declaration registration and body
     * checking. Cache an already-diagnosed failure in the same scope so the
     * original, more useful diagnostic is not emitted twice. */
    if (info != NULL && info->scope_id == scope_id &&
        info->type_id == context->type_error) {
        return context->type_error;
    }

    if (node->kind == VITTE_AST_NODE_TYPE_EXPR &&
        node->operator_kind == VITTE_TOKEN_KW_PROC) {
        bool has_return_type =
            (node->flags &
             VITTE_AST_FLAG_FUNCTION_TYPE_HAS_RETURN) != 0u;
        size_t parameter_count = node->child_count -
            (has_return_type && node->child_count != 0u ? 1u : 0u);
        vitte_sema_type_id_t *parameter_types = NULL;
        vitte_sema_type_id_t return_type = context->type_void;
        vitte_sema_type_id_t function_type;
        size_t index;

        if (has_return_type && node->child_count == 0u) {
            return context->type_error;
        }
        if (parameter_count != 0u) {
            parameter_types = (vitte_sema_type_id_t *)
                vitte_sema_reallocate(
                    context,
                    NULL,
                    parameter_count,
                    sizeof(*parameter_types));
            if (parameter_types == NULL) {
                return context->type_error;
            }
        }

        for (index = 0u; index < parameter_count; ++index) {
            parameter_types[index] = vitte_sema_analyze_type_node(
                context,
                node->children[index],
                scope_id);
            if (parameter_types[index] == context->type_error) {
                free(parameter_types);
                return context->type_error;
            }
        }
        if (has_return_type) {
            return_type = vitte_sema_analyze_type_node(
                context,
                node->children[node->child_count - 1u],
                scope_id);
            if (return_type == context->type_error) {
                free(parameter_types);
                return context->type_error;
            }
        }

        function_type = vitte_sema_create_function_type_private(
            context,
            parameter_types,
            parameter_count,
            return_type);
        free(parameter_types);
        if (info != NULL) {
            info->type_id = function_type;
            info->scope_id = scope_id;
        }
        return function_type != VITTE_SEMA_INVALID_TYPE_ID
            ? function_type
            : context->type_error;
    }

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

        case VITTE_AST_NODE_OPTIONAL_TYPE:
        {
            vitte_sema_type_id_t element;

            if (node->child_count != 1u) {
                return context->type_error;
            }

            element = vitte_sema_analyze_type_node(
                context,
                node->children[0],
                scope_id);

            if (element == context->type_error) {
                return element;
            }

            if (info != NULL) {
                info->type_id = vitte_sema_create_type_private(
                    context,
                    VITTE_SEMA_TYPE_OPTIONAL,
                    element,
                    0u,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    false);
            }

            return info != NULL ? info->type_id : context->type_error;
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
                        node->child_count == 1u
                            ? VITTE_SEMA_TYPE_SLICE
                            : VITTE_SEMA_TYPE_ARRAY,
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
            const vitte_ast_node_t *qualified_path;
            const vitte_ast_node_t *diagnostic_name_node;

            symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;

            qualified_path = node;
            if (node->kind == VITTE_AST_NODE_TYPE_EXPR &&
                node->child_count != 0u) {
                const vitte_ast_node_t *base_path =
                    vitte_sema_node(
                        context,
                        node->children[0]);
                if (base_path != NULL &&
                    base_path->kind == VITTE_AST_NODE_PATH) {
                    qualified_path = base_path;
                }
            }

            diagnostic_name_node = qualified_path;
            if (qualified_path->kind == VITTE_AST_NODE_PATH &&
                qualified_path->child_count != 0u) {
                const vitte_ast_node_t *last_segment = vitte_sema_node(
                    context,
                    qualified_path->children[qualified_path->child_count - 1u]);
                if (last_segment != NULL) diagnostic_name_node = last_segment;
            }

            /* A generic TYPE_EXPR stores the constructor name in its first
             * child; the wrapper node itself has no identifier token. */
            token =
                vitte_sema_node_token(
                    context,
                    qualified_path);

            /* Primitive spellings are language types even if an import or
             * prelude symbol with the same text is present. Resolve them
             * before nominal lookup so signatures, bindings and expressions
             * share the same canonical type IDs. */
            if (((qualified_path->kind == VITTE_AST_NODE_PATH &&
                  qualified_path->child_count == 1u) ||
                 qualified_path->kind == VITTE_AST_NODE_IDENTIFIER) &&
                (node->kind != VITTE_AST_NODE_TYPE_EXPR ||
                 node->child_count == 1u)) {
                const char *builtin_name = NULL;
                size_t builtin_name_length = 0u;
                if (vitte_sema_token_name(
                        token, &builtin_name, &builtin_name_length)) {
                    type_id = vitte_sema_builtin_type_from_name(
                        context, builtin_name, builtin_name_length);
                    if (type_id != VITTE_SEMA_INVALID_TYPE_ID) {
                        if (info != NULL) {
                            info->type_id = type_id;
                            info->scope_id = scope_id;
                        }
                        return type_id;
                    }
                }
            }

            if (qualified_path->kind == VITTE_AST_NODE_PATH &&
                qualified_path->child_count > 1u &&
                vitte_sema_resolve_qualified_import_type(
                    context,
                    qualified_path,
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
                        diagnostic_name_node->span,
                        diagnostic_name_node->id,
                        symbol_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        vitte_sema_type_for_symbol(context, symbol_id) !=
                                context->type_error
                            ? vitte_sema_type_for_symbol(context, symbol_id)
                            : VITTE_SEMA_INVALID_TYPE_ID);
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

                    type_id =
                        vitte_sema_apply_generic_arguments_private(
                            context,
                            node,
                            scope_id,
                            symbol_id,
                            type_id);
                    if (type_id != context->type_error &&
                        !vitte_sema_generic_type_application_check_constraints_private(
                            context,
                            node,
                            symbol_id,
                            scope_id)) {
                        type_id = context->type_error;
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
                        info->scope_id = scope_id;
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
            } else if (VITTE_SEMA_MATCH_BUILTIN("i8") ||
                       VITTE_SEMA_MATCH_BUILTIN("i16") ||
                       VITTE_SEMA_MATCH_BUILTIN("i32")) {
                /* Match the widened signed arithmetic model used for i64. */
                type_id = context->type_i64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("u64")) {
                type_id = context->type_u64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("u32")) {
                /* The bootstrap arithmetic model is 64-bit; the C backend
                 * still preserves the source width for declarations. */
                type_id = context->type_u64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("u16")) {
                /* Keep the same widened arithmetic model used for u32. */
                type_id = context->type_u64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("usize")) {
                type_id = context->type_u64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("isize")) {
                /* The bootstrap checker widens pointer-sized signed
                 * arithmetic to its canonical signed 64-bit model. */
                type_id = context->type_i64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("u8")) {
                type_id = context->type_u8;
            } else if (VITTE_SEMA_MATCH_BUILTIN("f32")) {
                type_id = context->type_f32;
            } else if (VITTE_SEMA_MATCH_BUILTIN("f64") ||
                       VITTE_SEMA_MATCH_BUILTIN("float")) {
                type_id = context->type_f64;
            } else if (VITTE_SEMA_MATCH_BUILTIN("char")) {
                type_id = context->type_char;
            } else if (VITTE_SEMA_MATCH_BUILTIN("string") ||
                       VITTE_SEMA_MATCH_BUILTIN("str")) {
                type_id = context->type_string;
            } else if (VITTE_SEMA_MATCH_BUILTIN("void")) {
                type_id = context->type_void;
            } else if (VITTE_SEMA_MATCH_BUILTIN("never")) {
                type_id = context->type_never;
            } else {
                symbol_id =
                    vitte_scope_lookup_n(
                        &context->scopes,
                        scope_id,
                        name,
                        name_length);
                if (symbol_id ==
                    VITTE_SCOPE_INVALID_SYMBOL_ID) {
                    if (vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_UNKNOWN_TYPE,
                            diagnostic_name_node->span,
                            diagnostic_name_node->id,
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID)) {
                        vitte_sema_help_similar_name(
                            context,
                            scope_id,
                            scope_id,
                            name,
                            name_length,
                            VITTE_SEMA_DIAGNOSTIC_UNKNOWN_TYPE,
                            true);
                    }

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
                            diagnostic_name_node->span,
                            diagnostic_name_node->id,
                            symbol_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            vitte_sema_type_for_symbol(context, symbol_id) !=
                                    context->type_error
                                ? vitte_sema_type_for_symbol(context, symbol_id)
                                : VITTE_SEMA_INVALID_TYPE_ID);

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
                info->scope_id = scope_id;
            }

            type_id =
                vitte_sema_apply_generic_arguments_private(
                    context,
                    node,
                    scope_id,
                    symbol_id,
                    type_id);
            if (type_id != context->type_error &&
                !vitte_sema_generic_type_application_check_constraints_private(
                    context,
                    node,
                    symbol_id,
                    scope_id)) {
                type_id = context->type_error;
            }

            if (info != NULL) {
                info->type_id = type_id;
                info->scope_id = scope_id;
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
    size_t name_length,
    vitte_symbol_id_t *private_symbol_id);

typedef enum vitte_sema_builtin_kind {
    VITTE_SEMA_BUILTIN_NONE = 0,
    VITTE_SEMA_BUILTIN_PRINT,
    VITTE_SEMA_BUILTIN_LEN,
    VITTE_SEMA_BUILTIN_TRIM,
    VITTE_SEMA_BUILTIN_STARTS_WITH,
    VITTE_SEMA_BUILTIN_ENDS_WITH,
    VITTE_SEMA_BUILTIN_STRING_FROM,
    VITTE_SEMA_BUILTIN_STRING_FROM_SLICE,
    VITTE_SEMA_BUILTIN_STRING_FROM_BYTE,
    VITTE_SEMA_BUILTIN_STRING_FROM_BYTES,
    VITTE_SEMA_BUILTIN_STRING_WITH_CAPACITY,
    VITTE_SEMA_BUILTIN_STRING_SLICE,
    VITTE_SEMA_BUILTIN_STRING_AS_BYTES,
    VITTE_SEMA_BUILTIN_STRING_AS_PTR,
    VITTE_SEMA_BUILTIN_MEMBER_LEN,
    VITTE_SEMA_BUILTIN_ARRAY_PUSH,
    VITTE_SEMA_BUILTIN_ARRAY_RESERVE
} vitte_sema_builtin_kind_t;

static vitte_sema_builtin_kind_t
vitte_sema_builtin_kind_from_name(
    const char *name,
    size_t name_length)
{
    if (name == NULL) {
        return VITTE_SEMA_BUILTIN_NONE;
    }
    if (name_length == 5u && memcmp(name, "print", 5u) == 0) {
        return VITTE_SEMA_BUILTIN_PRINT;
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
    if (node->kind == VITTE_AST_NODE_MEMBER_EXPR &&
        node->child_count >= 2u) {
        const vitte_ast_node_t *base =
            vitte_sema_node(context, node->children[0]);
        const vitte_ast_node_t *member =
            vitte_sema_node(context, node->children[1]);
        const vitte_token_t *base_token =
            vitte_sema_node_token(context, base);
        const vitte_token_t *member_token =
            vitte_sema_node_token(context, member);

        if (base != NULL &&
            (base->kind == VITTE_AST_NODE_IDENTIFIER ||
             (base->kind == VITTE_AST_NODE_PATH &&
              base->child_count == 1u)) &&
            base_token != NULL && base_token->length == 6u &&
            memcmp(base_token->lexeme, "string", 6u) == 0 &&
            member_token != NULL) {
            if (member_token->length == 4u &&
                memcmp(member_token->lexeme, "from", 4u) == 0) {
                return VITTE_SEMA_BUILTIN_STRING_FROM;
            }
            if (member_token->length == 10u &&
                memcmp(member_token->lexeme, "from_slice", 10u) == 0) {
                return VITTE_SEMA_BUILTIN_STRING_FROM_SLICE;
            }
            if (member_token->length == 9u &&
                memcmp(member_token->lexeme, "from_byte", 9u) == 0) {
                return VITTE_SEMA_BUILTIN_STRING_FROM_BYTE;
            }
            if (member_token->length == 10u &&
                memcmp(member_token->lexeme, "from_bytes", 10u) == 0) {
                return VITTE_SEMA_BUILTIN_STRING_FROM_BYTES;
            }
            if (member_token->length == 13u &&
                memcmp(member_token->lexeme, "with_capacity", 13u) == 0) {
                return VITTE_SEMA_BUILTIN_STRING_WITH_CAPACITY;
            }
        }

        if (member_token != NULL &&
            member_token->length == 3u &&
            memcmp(member_token->lexeme, "len", 3u) == 0) {
            return VITTE_SEMA_BUILTIN_MEMBER_LEN;
        }

        {
            const vitte_sema_node_info_t *base_info = base != NULL
                ? vitte_sema_node_info_private(context, base->id)
                : NULL;
            const vitte_sema_type_t *base_type = base_info != NULL
                ? vitte_sema_type_private(context, base_info->type_id)
                : NULL;

            while (base_type != NULL &&
                   (base_type->kind == VITTE_SEMA_TYPE_REFERENCE ||
                    base_type->kind == VITTE_SEMA_TYPE_POINTER)) {
                base_type = vitte_sema_type_private(
                    context, base_type->element_type);
            }
            if (base_type != NULL &&
                base_type->kind == VITTE_SEMA_TYPE_STRING &&
                member_token != NULL) {
                if (member_token->length == 8u &&
                    memcmp(member_token->lexeme, "as_bytes", 8u) == 0) {
                    return VITTE_SEMA_BUILTIN_STRING_AS_BYTES;
                }
                if (member_token->length == 6u &&
                    memcmp(member_token->lexeme, "as_ptr", 6u) == 0) {
                    return VITTE_SEMA_BUILTIN_STRING_AS_PTR;
                }
                if (member_token->length == 5u &&
                    memcmp(member_token->lexeme, "slice", 5u) == 0) {
                    return VITTE_SEMA_BUILTIN_STRING_SLICE;
                }
            }
            if (base_type != NULL &&
                (base_type->kind == VITTE_SEMA_TYPE_ARRAY ||
                 base_type->kind == VITTE_SEMA_TYPE_SLICE) &&
                member_token != NULL) {
                if (member_token->length == 6u &&
                    memcmp(member_token->lexeme, "as_ptr", 6u) == 0) {
                    return VITTE_SEMA_BUILTIN_STRING_AS_PTR;
                }
                if (member_token->length == 4u &&
                    memcmp(member_token->lexeme, "push", 4u) == 0) {
                    return VITTE_SEMA_BUILTIN_ARRAY_PUSH;
                }
                if (member_token->length == 7u &&
                    memcmp(member_token->lexeme, "reserve", 7u) == 0) {
                    return VITTE_SEMA_BUILTIN_ARRAY_RESERVE;
                }
            }
        }
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

/* Return an optimal-string-alignment distance, counting an adjacent
 * transposition as one edit. Non-ASCII and unusually long names are ignored
 * so this recovery path stays bounded and avoids byte-level Unicode guesses. */
static size_t
vitte_sema_name_distance(const char *a, size_t an, const char *b, size_t bn)
{
    size_t distance[65][65];
    size_t i;
    size_t j;

    if (a == NULL || b == NULL || an < 3u || bn < 3u ||
        an > 64u || bn > 64u) {
        return SIZE_MAX;
    }

    for (i = 0u; i < an; ++i) {
        if ((unsigned char)a[i] >= 128u) return SIZE_MAX;
    }
    for (j = 0u; j < bn; ++j) {
        if ((unsigned char)b[j] >= 128u) return SIZE_MAX;
    }

    for (i = 0u; i <= an; ++i) distance[i][0] = i;
    for (j = 0u; j <= bn; ++j) distance[0][j] = j;

    for (i = 1u; i <= an; ++i) {
        for (j = 1u; j <= bn; ++j) {
            size_t deletion = distance[i - 1u][j] + 1u;
            size_t insertion = distance[i][j - 1u] + 1u;
            size_t substitution = distance[i - 1u][j - 1u] +
                (a[i - 1u] == b[j - 1u] ? 0u : 1u);
            size_t best = deletion < insertion ? deletion : insertion;

            if (substitution < best) best = substitution;
            if (i > 1u && j > 1u &&
                a[i - 1u] == b[j - 2u] &&
                a[i - 2u] == b[j - 1u] &&
                distance[i - 2u][j - 2u] + 1u < best) {
                best = distance[i - 2u][j - 2u] + 1u;
            }
            distance[i][j] = best;
        }
    }

    return distance[an][bn];
}

static void
vitte_sema_help_similar_name(vitte_sema_t *context, vitte_scope_id_t scope_id,
                             vitte_scope_id_t requester, const char *name,
                             size_t length, vitte_sema_diagnostic_code_t code,
                             bool type_only)
{
    const vitte_symbol_t *candidate = NULL;
    size_t best_distance = SIZE_MAX;
    size_t i;
    bool tied = false;
    vitte_sema_diagnostic_t *diagnostic;
    if (context->diagnostic_count == 0u) return;
    diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
    if (diagnostic->code != code) return;
    for (i = 0u; i < context->scopes.symbol_count; ++i) {
        const vitte_symbol_t *symbol = &context->scopes.symbols[i];
        size_t distance;
        size_t max_distance;

        if (symbol->name == NULL ||
            (type_only && !vitte_symbol_kind_is_type(symbol->kind)) ||
            vitte_scope_lookup_visible_n(&context->scopes, scope_id, requester,
                symbol->name, symbol->name_length) != symbol->id) continue;

        distance = vitte_sema_name_distance(
            name, length, symbol->name, symbol->name_length);
        max_distance = length <= 5u ? 1u : 2u;
        if (distance == 0u || distance > max_distance) continue;

        if (distance < best_distance) {
            candidate = symbol;
            best_distance = distance;
            tied = false;
        } else if (distance == best_distance) {
            tied = true;
        }
    }
    if (candidate != NULL && !tied) {
        (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
            type_only
                ? "Did you mean type `%s` instead of `%.*s`? `%s` is visible in this scope; verify that it is the intended type."
                : "Did you mean `%s` instead of `%.*s`? `%s` is visible in this scope; verify that it is the intended declaration.",
            candidate->name, (int)length, name, candidate->name);
    }
}

static vitte_sema_type_id_t
vitte_sema_builtin_type_from_name(
    const vitte_sema_t *context,
    const char *name,
    size_t name_length)
{
#define VITTE_SEMA_MATCH_TYPE_NAME(text_) \
    (name_length == sizeof(text_) - 1u && \
     memcmp(name, text_, sizeof(text_) - 1u) == 0)
    if (context == NULL || name == NULL) return VITTE_SEMA_INVALID_TYPE_ID;
    if (VITTE_SEMA_MATCH_TYPE_NAME("bool")) return context->type_bool;
    if (VITTE_SEMA_MATCH_TYPE_NAME("i64") || VITTE_SEMA_MATCH_TYPE_NAME("int") ||
        VITTE_SEMA_MATCH_TYPE_NAME("i8") || VITTE_SEMA_MATCH_TYPE_NAME("i16") ||
        VITTE_SEMA_MATCH_TYPE_NAME("i32") || VITTE_SEMA_MATCH_TYPE_NAME("isize"))
        return context->type_i64;
    if (VITTE_SEMA_MATCH_TYPE_NAME("u64") || VITTE_SEMA_MATCH_TYPE_NAME("u32") ||
        VITTE_SEMA_MATCH_TYPE_NAME("u16") || VITTE_SEMA_MATCH_TYPE_NAME("usize"))
        return context->type_u64;
    if (VITTE_SEMA_MATCH_TYPE_NAME("u8")) return context->type_u8;
    if (VITTE_SEMA_MATCH_TYPE_NAME("f32")) return context->type_f32;
    if (VITTE_SEMA_MATCH_TYPE_NAME("f64") || VITTE_SEMA_MATCH_TYPE_NAME("float"))
        return context->type_f64;
    if (VITTE_SEMA_MATCH_TYPE_NAME("char")) return context->type_char;
    if (VITTE_SEMA_MATCH_TYPE_NAME("string") || VITTE_SEMA_MATCH_TYPE_NAME("str"))
        return context->type_string;
    if (VITTE_SEMA_MATCH_TYPE_NAME("void")) return context->type_void;
    if (VITTE_SEMA_MATCH_TYPE_NAME("never")) return context->type_never;
#undef VITTE_SEMA_MATCH_TYPE_NAME
    return VITTE_SEMA_INVALID_TYPE_ID;
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
    const vitte_symbol_t *resolved_symbol;
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

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if ((context->current_contract_ensures ||
         context->current_contract_requires) &&
        name_length == 3u && memcmp(name, "old", 3u) == 0 &&
        vitte_scope_lookup_n(&context->scopes, scope_id, name,
            name_length) == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_CONTRACT_OLD_VALUE,
                node->span, node->id, VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
            (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                "`old(...)` requires an entry-state snapshot, which is not available for this contract reference. Remove `old` or use values directly available to the clause.");
        }
        if (info != NULL) info->type_id = context->type_error;
        return context->type_error;
    }

    /* Postconditions refer to the value returned by the procedure through
       the implicit `result` binding.  Keep it contextual so a misspelled or
       accidental use in the procedure body still receives the normal name
       resolution diagnostic. */
    if (context->current_contract_ensures &&
        name_length == 6u &&
        memcmp(name, "result", 6u) == 0 &&
        context->current_return_type != VITTE_SEMA_INVALID_TYPE_ID &&
        context->current_return_type != context->type_void &&
        context->current_return_type != context->type_never) {
        if (info != NULL) {
            info->type_id = context->current_return_type;
            info->scope_id = scope_id;
            info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
        }
        return context->current_return_type;
    }

    if ((context->current_contract_ensures ||
         context->current_contract_requires) &&
        name_length == 6u && memcmp(name, "result", 6u) == 0) {
        (void)vitte_sema_add_diagnostic(
            context, VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_CONTRACT_RESULT,
            node->span, node->id, VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID);
        if (info != NULL) {
            info->type_id = context->type_error;
        }
        return context->type_error;
    }

    symbol_id =
        vitte_scope_lookup_n(
            &context->scopes,
            scope_id,
            name,
            name_length);

    if (symbol_id ==
        VITTE_SCOPE_INVALID_SYMBOL_ID) {
        vitte_sema_diagnostic_code_t diagnostic_code;
        vitte_symbol_id_t private_symbol_id;

        private_symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;

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
                builtin_kind == VITTE_SEMA_BUILTIN_PRINT
                    ? context->type_void
                    : builtin_kind == VITTE_SEMA_BUILTIN_TRIM
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

        type_id = vitte_sema_builtin_type_from_name(
            context, name, name_length);
        if (type_id != VITTE_SEMA_INVALID_TYPE_ID) {
            if (vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_NOT_A_VALUE,
                    node->span,
                    node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    type_id)) {
                vitte_sema_append_diagnostic_help(
                    &context->diagnostics[context->diagnostic_count - 1u],
                    "This name denotes a type, not a runtime value. Use it in a type position or construct a value with the type's supported syntax.");
            }
            if (info != NULL) info->type_id = context->type_error;
            return context->type_error;
        }

        diagnostic_code =
            vitte_sema_is_private_glob_import_name(
                context,
                scope_id,
                name,
                name_length,
                &private_symbol_id)
                ? VITTE_SEMA_DIAGNOSTIC_PRIVATE_SYMBOL
                : VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME;

        if (vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            diagnostic_code,
            node->span,
            node->id,
            private_symbol_id,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID)) {
            if (diagnostic_code == VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME) {
                if (!vitte_sema_help_module_member_import(
                        context,
                        scope_id,
                        name,
                        name_length)) {
                    vitte_sema_help_similar_name(
                        context,
                        scope_id,
                        scope_id,
                        name,
                        name_length,
                        diagnostic_code,
                        false);
                }
                if (context->current_contract_ensures ||
                    context->current_contract_requires) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[context->diagnostic_count - 1u],
                        "Contract clauses are checked at the procedure boundary. Refer to a declared parameter or visible constant; local variables from the procedure body are unavailable here.");
                }
            } else {
                vitte_sema_help_private_symbol(
                    context,
                    private_symbol_id !=
                            VITTE_SCOPE_INVALID_SYMBOL_ID
                        ? vitte_scope_get_symbol(
                              &context->scopes,
                              private_symbol_id)
                        : NULL);
            }
        }

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
    resolved_symbol = vitte_scope_get_symbol(
        &context->scopes,
        symbol_id);

    if (resolved_symbol != NULL &&
        vitte_symbol_kind_is_type(resolved_symbol->kind)) {
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_NOT_A_VALUE,
            node->span,
            node->id,
            symbol_id,
            VITTE_SEMA_INVALID_TYPE_ID,
            type_id);
        if (info != NULL) info->type_id = context->type_error;
        return context->type_error;
    }

    if (info != NULL) {
        info->symbol_id = symbol_id;
        info->type_id = type_id;
        info->scope_id = scope_id;
        info->flags |=
            VITTE_SEMA_NODE_FLAG_RESOLVED;
        /* Module constants have static storage in the generated unit.  They
         * remain immutable places, but callers may take a shared reference
         * to an element (for example, a read-only descriptor table entry). */
        if (resolved_symbol != NULL &&
            (resolved_symbol->kind == VITTE_SYMBOL_KIND_CONST ||
             resolved_symbol->kind == VITTE_SYMBOL_KIND_STATIC)) {
            info->flags |= VITTE_SEMA_NODE_FLAG_ADDRESSABLE;
        }
        if (resolved_symbol != NULL &&
            (resolved_symbol->flags & VITTE_SYMBOL_FLAG_MUTABLE) != 0u) {
            info->flags |= VITTE_SEMA_NODE_FLAG_MUTABLE;
        }
    }

    context->stats.names_resolved =
        vitte_sema_u64_add_sat(
            context->stats.names_resolved,
            UINT64_C(1));

    return type_id;
}

#include "constant_eval.inc"

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
    const vitte_token_t *operator_token;
    vitte_parser_span_t operator_span;
    vitte_sema_node_info_t *info;

    if (node->child_count == 0u) {
        return context->type_error;
    }

    operator_token = vitte_sema_node_token(context, node);
    operator_span = node->span;
    if (operator_token != NULL) {
        operator_span.file_id = operator_token->span.file_id;
        operator_span.begin = operator_token->span.begin;
        operator_span.end = operator_token->span.end;
        operator_span.line = operator_token->line;
        operator_span.column = operator_token->column;
        operator_span.valid = operator_token->span.valid;
    }
    operand_type =
        vitte_sema_analyze_expression(
            context,
            node->children[0],
            scope_id);

    if (operand_type == context->type_error) {
        info = vitte_sema_node_info_mut_private(context, node->id);
        if (info != NULL) info->type_id = context->type_error;
        return context->type_error;
    }

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

        case VITTE_TOKEN_KW_MOVE:
            /* `move` transfers the binding's value; representation lowering
             * is identity in C17 and the reference pass enforces consumption. */
            result = operand_type;
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
                info = vitte_sema_node_info_mut_private(
                    context,
                    node->id);
                if (info != NULL) {
                    info->flags |= VITTE_SEMA_NODE_FLAG_LVALUE |
                                   VITTE_SEMA_NODE_FLAG_ADDRESSABLE;
                    if (operand->is_mutable) {
                        info->flags |= VITTE_SEMA_NODE_FLAG_MUTABLE;
                    }
                }
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
            /* The borrow mode is part of the operator spelling.  Taking an
             * immutable reference to a mutable binding must stay immutable;
             * otherwise `&value` could silently satisfy a `&mut T`
             * parameter. */
            is_mutable =
                (node->flags & VITTE_AST_FLAG_MUTABLE) != 0u;

            if (operand_info == NULL ||
                (operand_info->flags & VITTE_SEMA_NODE_FLAG_ADDRESSABLE) == 0u ||
                (is_mutable &&
                 (operand_info->flags & VITTE_SEMA_NODE_FLAG_MUTABLE) == 0u)) {
                break;
            }

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
        if (vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_INVALID_UNARY_OPERAND,
            operator_span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            operand_type)) {
            vitte_sema_diagnostic_t *diagnostic =
                &context->diagnostics[context->diagnostic_count - 1u];
            if (node->operator_kind == VITTE_TOKEN_AMP ||
                node->operator_kind == VITTE_TOKEN_KW_REF) {
                const vitte_sema_node_info_t *operand_info =
                    vitte_sema_node_info_private(context, node->children[0]);
                if (operand_info == NULL ||
                    (operand_info->flags & VITTE_SEMA_NODE_FLAG_ADDRESSABLE) == 0u) {
                    (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                        "`&` can only borrow an addressable place such as a local, parameter, field, indexed element, or dereferenced reference. This operand is a value, not a place.");
                } else {
                    (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                        "`&mut` requires a mutable place. Declare the binding mutable or use `&` for a shared reference.");
                }
            } else if (node->operator_kind == VITTE_TOKEN_STAR) {
                (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                    "`*` requires a pointer or reference operand; this expression has a non-pointer type.");
            } else {
                (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                    "The operand type is not valid for this unary operator.");
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

    /* Equality on borrowed values compares their addresses. Preserve
     * reference types for `==`/`!=`; value operators still transparently
     * read through references. */
    if (node->operator_kind != VITTE_TOKEN_EQUAL_EQUAL &&
        node->operator_kind != VITTE_TOKEN_BANG_EQUAL &&
        node->operator_kind != VITTE_TOKEN_QUESTION_QUESTION) {
        left = vitte_sema_value_type_private(context, left);
        right = vitte_sema_value_type_private(context, right);
    }

    /* A floating literal can adopt the type of an f32 peer in the same
     * operation when representable, without narrowing an actual f64 value. */
    if (left == context->type_f32 &&
        right == context->type_f64 &&
        vitte_sema_contextualize_float_literal(
            context,
            node->children[1],
            context->type_f32)) {
        right = context->type_f32;
    } else if (right == context->type_f32 &&
               left == context->type_f64 &&
               vitte_sema_contextualize_float_literal(
                   context,
                   node->children[0],
                   context->type_f32)) {
        left = context->type_f32;
    }

    if (vitte_sema_type_is_integer_private(context, left) &&
        !vitte_sema_type_identical_private(context, left, right) &&
        vitte_sema_contextualize_integer_literal(
            context,
            node->children[1],
            left)) {
        right = vitte_sema_resolve_alias_type_private(context, left);
    } else if (vitte_sema_type_is_integer_private(context, right) &&
               !vitte_sema_type_identical_private(context, left, right) &&
               vitte_sema_contextualize_integer_literal(
                   context,
                   node->children[0],
                   right)) {
        left = vitte_sema_resolve_alias_type_private(context, right);
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
            } else if (vitte_sema_type_is_enum_private(context, left) &&
                       vitte_sema_type_is_enum_private(context, right) &&
                       vitte_sema_type_identical_private(
                           context, left, right)) {
                /* Pick values have a stable declaration-order tag.  Ordered
                 * comparisons operate on that tag, which is useful for
                 * validating reserved sentinel ranges. */
                result = context->type_bool;
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
        case VITTE_TOKEN_AMP_AMP:
        case VITTE_TOKEN_KW_OR:
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
            if (left == context->type_null) {
                result = right;
            } else {
                const vitte_sema_type_t *left_type =
                    vitte_sema_type_private(context, left);

                if (left_type != NULL &&
                    left_type->kind == VITTE_SEMA_TYPE_OPTIONAL) {
                    if (vitte_sema_type_assignable_private(
                            context,
                            left_type->element_type,
                            right)) {
                        result = left_type->element_type;
                    } else {
                        const vitte_ast_node_t *right_node =
                            vitte_sema_node(context, node->children[1]);

                        (void)vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                            right_node != NULL
                                ? right_node->span
                                : node->span,
                            right_node != NULL
                                ? right_node->id
                                : node->id,
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            left_type->element_type,
                            right);
                        result = left_type->element_type;
                    }
                }
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
        vitte_sema_type_id_t expected_type = left;
        vitte_sema_type_id_t found_type = right;
        if (node->operator_kind == VITTE_TOKEN_KW_AND ||
            node->operator_kind == VITTE_TOKEN_AMP_AMP ||
            node->operator_kind == VITTE_TOKEN_KW_OR ||
            node->operator_kind == VITTE_TOKEN_PIPE_PIPE) {
            expected_type = context->type_bool;
            found_type = left != context->type_bool ? left : right;
        }
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_INVALID_BINARY_OPERANDS,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            expected_type,
            found_type);
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
    const vitte_ast_node_t *left_node;
    const vitte_ast_node_t *right_node;
    bool direct_binding;
    bool immutable_binding;

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
    left_node = vitte_sema_node(context, node->children[0]);
    right_node = vitte_sema_node(context, node->children[1]);
    direct_binding = left_node != NULL &&
        (left_node->kind == VITTE_AST_NODE_IDENTIFIER ||
         left_node->kind == VITTE_AST_NODE_PATH);
    immutable_binding = direct_binding && left_symbol != NULL &&
        (left_symbol->kind == VITTE_SYMBOL_KIND_LOCAL ||
         left_symbol->kind == VITTE_SYMBOL_KIND_PARAMETER ||
         left_symbol->kind == VITTE_SYMBOL_KIND_STATIC) &&
        (left_symbol->flags & VITTE_SYMBOL_FLAG_MUTABLE) == 0u;

    if (left_info == NULL ||
        (left_info->flags &
         VITTE_SEMA_NODE_FLAG_LVALUE) == 0u ||
        immutable_binding ||
        (!direct_binding &&
         (left_info->flags & VITTE_SEMA_NODE_FLAG_MUTABLE) == 0u)) {
        vitte_symbol_id_t diagnostic_symbol_id;

        diagnostic_symbol_id = immutable_binding
            ? left_symbol->id
            : VITTE_SCOPE_INVALID_SYMBOL_ID;

        if (vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_NOT_ASSIGNABLE,
            left_node != NULL ? left_node->span : node->span,
            left_node != NULL ? left_node->id : node->id,
            diagnostic_symbol_id,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID)) {
            vitte_sema_diagnostic_t *diagnostic;

            vitte_sema_set_last_type_context(
                context,
                VITTE_SEMA_TYPE_CONTEXT_ASSIGNMENT,
                SIZE_MAX,
                diagnostic_symbol_id);

            diagnostic =
                &context->diagnostics[context->diagnostic_count - 1u];
            if (!immutable_binding) {
                (void)snprintf(
                    diagnostic->help,
                    sizeof(diagnostic->help),
                    "This expression is not an assignable place.");
            } else if (left_symbol->kind == VITTE_SYMBOL_KIND_LOCAL) {
                (void)snprintf(
                    diagnostic->help,
                    sizeof(diagnostic->help),
                    "This binding is immutable. Declare it as `let mut %.*s` to assign to it.",
                    (int)(left_symbol->name_length < 160u
                        ? left_symbol->name_length
                        : 160u),
                    left_symbol->name);
            } else if (left_symbol->kind == VITTE_SYMBOL_KIND_PARAMETER) {
                (void)snprintf(
                    diagnostic->help,
                    sizeof(diagnostic->help),
                    "This parameter binding is immutable. Declare it as `mut %.*s` in the procedure signature to allow local reassignment, or copy it into `let mut %.*s_copy = %.*s` before assigning.",
                    (int)(left_symbol->name_length < 120u
                        ? left_symbol->name_length
                        : 120u),
                    left_symbol->name,
                    (int)(left_symbol->name_length < 120u
                        ? left_symbol->name_length
                        : 120u),
                    left_symbol->name,
                    (int)(left_symbol->name_length < 120u
                        ? left_symbol->name_length
                        : 120u),
                    left_symbol->name);
            } else {
                (void)snprintf(
                    diagnostic->help,
                    sizeof(diagnostic->help),
                    "Static declarations are immutable. Copy the value into a mutable local before updating it.");
            }
        }

        return context->type_error;
    }

    if (!vitte_sema_accept_value_private(
            context,
            left,
            right,
            node->children[1])) {
        if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                right_node != NULL ? right_node->span : node->span,
                right_node != NULL ? right_node->id : node->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                left,
                right)) {
            vitte_sema_set_last_type_context(
                context,
                VITTE_SEMA_TYPE_CONTEXT_ASSIGNMENT,
                SIZE_MAX,
                left_symbol != NULL
                    ? left_symbol->id
                    : VITTE_SCOPE_INVALID_SYMBOL_ID);
            vitte_sema_add_safe_integer_conversion_help(
                context,
                node->children[1],
                left,
                right);
        }

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

typedef struct vitte_sema_pick_variant_resolution {
    const vitte_ast_node_t *pick_declaration;
    const vitte_ast_node_t *variant_declaration;
    vitte_symbol_id_t pick_symbol_id;
    vitte_scope_id_t pick_scope_id;
    vitte_sema_type_id_t pick_type_id;
} vitte_sema_pick_variant_resolution_t;

static bool
vitte_sema_resolve_pick_variant_path(
    vitte_sema_t *context,
    const vitte_ast_node_t *path,
    vitte_scope_id_t scope_id,
    vitte_sema_pick_variant_resolution_t *resolution)
{
    vitte_scope_id_t path_scope;
    size_t index;

    if (context == NULL || path == NULL || resolution == NULL ||
        path->child_count < 2u) {
        return false;
    }

    /* `Maybe<i64>::Some` is represented as a member expression whose base is
     * the complete generic type expression. Resolve it through that concrete
     * type so payload checks and bindings can substitute `T` with `i64`. */
    if (path->kind == VITTE_AST_NODE_MEMBER_EXPR) {
        const vitte_ast_node_t *type_node =
            vitte_sema_node(context, path->children[0]);
        const vitte_ast_node_t *variant_node =
            vitte_sema_node(context, path->children[path->child_count - 1u]);
        const vitte_token_t *variant_token =
            vitte_sema_node_token(context, variant_node);
        vitte_sema_type_id_t pick_type_id;
        const vitte_sema_type_t *pick_type;
        const vitte_symbol_t *pick_symbol;
        const vitte_ast_node_t *pick;
        vitte_symbol_id_t pick_symbol_id;
        size_t child_index;

        if (type_node == NULL || variant_token == NULL ||
            variant_token->lexeme == NULL) {
            return false;
        }

        /* Generic variant constructors carry a concrete TYPE_EXPR (for
         * example `Maybe[i64]::Some`).  Ordinary enum-like variants use the
         * same member AST shape with an identifier or path on the left
         * (`Endianness.Little`).  Resolve that spelling as a type before the
         * normal member analyzer treats it as a value expression. */
        pick_symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
        if (type_node->kind == VITTE_AST_NODE_TYPE_EXPR) {
            pick_type_id = vitte_sema_analyze_type_node(
                context, type_node->id, scope_id);
            pick_type = vitte_sema_type_private(context, pick_type_id);
            if (pick_type == NULL ||
                pick_type->kind != VITTE_SEMA_TYPE_NAMED ||
                pick_type->symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
                return false;
            }
            pick_symbol_id = pick_type->symbol_id;
        } else if (type_node->kind == VITTE_AST_NODE_IDENTIFIER ||
                   type_node->kind == VITTE_AST_NODE_PATH) {
            const vitte_token_t *type_token =
                vitte_sema_node_token(context, type_node);
            const char *type_name;
            size_t type_name_length;

            if (type_node->kind == VITTE_AST_NODE_PATH &&
                type_node->child_count > 1u) {
                if (!vitte_sema_resolve_qualified_import_type(
                        context, type_node, scope_id, &pick_symbol_id)) {
                    return false;
                }
            } else {
                if (!vitte_sema_token_name(
                        type_token, &type_name, &type_name_length)) {
                    return false;
                }
                pick_symbol_id = vitte_scope_lookup_visible_n(
                    &context->scopes,
                    scope_id,
                    scope_id,
                    type_name,
                    type_name_length);
            }
            pick_symbol = pick_symbol_id !=
                    VITTE_SCOPE_INVALID_SYMBOL_ID
                ? vitte_scope_get_symbol(&context->scopes, pick_symbol_id)
                : NULL;
            pick = pick_symbol != NULL &&
                    pick_symbol->kind == VITTE_SYMBOL_KIND_PICK
                ? (const vitte_ast_node_t *)pick_symbol->payload
                : NULL;
            if (pick == NULL || pick->kind != VITTE_AST_NODE_PICK_DECL) {
                return false;
            }
            pick_type_id = vitte_sema_type_for_symbol(
                context, pick_symbol_id);
        } else {
            return false;
        }

        if (type_node->kind == VITTE_AST_NODE_TYPE_EXPR) {
            pick_symbol = vitte_scope_get_symbol(
                &context->scopes, pick_symbol_id);
            pick = pick_symbol != NULL &&
                    pick_symbol->kind == VITTE_SYMBOL_KIND_PICK
                ? (const vitte_ast_node_t *)pick_symbol->payload
                : NULL;
            if (pick == NULL || pick->kind != VITTE_AST_NODE_PICK_DECL) {
                return false;
            }
        }
        resolution->variant_declaration = NULL;
        for (child_index = 1u; child_index < pick->child_count; ++child_index) {
            const vitte_ast_node_t *variant =
                vitte_sema_node(context, pick->children[child_index]);
            const vitte_ast_node_t *name = variant != NULL &&
                    variant->child_count != 0u
                ? vitte_sema_node(context, variant->children[0])
                : NULL;
            const vitte_token_t *name_token =
                vitte_sema_node_token(context, name);
            if (variant != NULL &&
                variant->kind == VITTE_AST_NODE_VARIANT_DECL &&
                name_token != NULL && name_token->lexeme != NULL &&
                name_token->length == variant_token->length &&
                memcmp(name_token->lexeme, variant_token->lexeme,
                       name_token->length) == 0) {
                resolution->variant_declaration = variant;
                break;
            }
        }
        if (resolution->variant_declaration == NULL) {
            return false;
        }
        resolution->pick_declaration = pick;
        resolution->pick_symbol_id = pick_symbol_id;
        resolution->pick_type_id = pick_type_id;
        resolution->pick_scope_id = VITTE_SCOPE_INVALID_SCOPE_ID;
        for (child_index = 0u;
             child_index < context->scopes.scope_count;
             ++child_index) {
            const vitte_scope_entry_t *candidate =
                &context->scopes.scopes[child_index];
            if (candidate->kind == VITTE_SCOPE_KIND_PICK &&
                candidate->payload == (void *)pick) {
                resolution->pick_scope_id = candidate->id;
                break;
            }
        }
        return resolution->pick_scope_id != VITTE_SCOPE_INVALID_SCOPE_ID;
    }

    if (path->kind != VITTE_AST_NODE_PATH) {
        return false;
    }
    path_scope = scope_id;
    for (index = 0u; index + 1u < path->child_count; ++index) {
        const vitte_ast_node_t *segment =
            vitte_sema_node(context, path->children[index]);
        const vitte_token_t *segment_token =
            vitte_sema_node_token(context, segment);
        const char *segment_name;
        size_t segment_name_length;
        vitte_symbol_id_t symbol_id;
        const vitte_symbol_t *symbol;

        if (!vitte_sema_token_name(
                segment_token, &segment_name, &segment_name_length)) {
            return false;
        }
        symbol_id = vitte_scope_lookup_visible_n(
            &context->scopes,
            path_scope,
            scope_id,
            segment_name,
            segment_name_length);
        symbol = symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID
            ? vitte_scope_get_symbol(&context->scopes, symbol_id)
            : NULL;
        if (symbol == NULL) {
            return false;
        }
        if (symbol->kind == VITTE_SYMBOL_KIND_PICK &&
            index + 2u == path->child_count &&
            symbol->payload != NULL) {
            const vitte_ast_node_t *pick =
                (const vitte_ast_node_t *)symbol->payload;
            const vitte_ast_node_t *variant_name =
                vitte_sema_node(
                    context, path->children[path->child_count - 1u]);
            const vitte_token_t *variant_token =
                vitte_sema_node_token(context, variant_name);
            size_t child_index;

            if (pick->kind != VITTE_AST_NODE_PICK_DECL ||
                variant_token == NULL || variant_token->lexeme == NULL) {
                return false;
            }
            resolution->variant_declaration = NULL;
            for (child_index = 1u;
                 child_index < pick->child_count;
                 ++child_index) {
                const vitte_ast_node_t *variant =
                    vitte_sema_node(context, pick->children[child_index]);
                const vitte_ast_node_t *name = variant != NULL &&
                        variant->child_count != 0u
                    ? vitte_sema_node(context, variant->children[0])
                    : NULL;
                const vitte_token_t *name_token =
                    vitte_sema_node_token(context, name);
                if (variant != NULL &&
                    variant->kind == VITTE_AST_NODE_VARIANT_DECL &&
                    name_token != NULL && name_token->lexeme != NULL &&
                    name_token->length == variant_token->length &&
                    memcmp(
                        name_token->lexeme,
                        variant_token->lexeme,
                        name_token->length) == 0) {
                    resolution->variant_declaration = variant;
                    break;
                }
            }
            if (resolution->variant_declaration == NULL) {
                return false;
            }
            resolution->pick_declaration = pick;
            resolution->pick_symbol_id = symbol_id;
            resolution->pick_type_id = vitte_sema_type_for_symbol(
                context, symbol_id);
            resolution->pick_scope_id = VITTE_SCOPE_INVALID_SCOPE_ID;
            for (child_index = 0u;
                 child_index < context->scopes.scope_count;
                 ++child_index) {
                const vitte_scope_entry_t *candidate =
                    &context->scopes.scopes[child_index];
                if (candidate->kind == VITTE_SCOPE_KIND_PICK &&
                    candidate->payload == (void *)pick) {
                    resolution->pick_scope_id = candidate->id;
                    break;
                }
            }
            return resolution->pick_scope_id !=
                VITTE_SCOPE_INVALID_SCOPE_ID;
        }
        if (symbol->kind != VITTE_SYMBOL_KIND_SPACE ||
            symbol->payload == NULL) {
            return false;
        }
        {
            const vitte_ast_node_t *space =
                (const vitte_ast_node_t *)symbol->payload;
            vitte_scope_id_t child_scope =
                VITTE_SCOPE_INVALID_SCOPE_ID;
            if (!vitte_sema_space_scope_private(
                    context, symbol, space, &child_scope)) {
                const vitte_sema_node_info_t *space_info =
                    space != NULL
                        ? vitte_sema_node_info_private(
                              context, space->id)
                        : NULL;
                if (space_info == NULL) {
                    return false;
                }
                child_scope = space_info->scope_id;
            }
            path_scope = child_scope;
        }
    }
    return false;
}

static bool
vitte_sema_procedure_has_generics_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *declaration)
{
    size_t index;
    if (context == NULL || declaration == NULL) { return false; }
    for (index = 0u; index < declaration->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, declaration->children[index]);
        if (child != NULL && child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
            return true;
        }
    }
    return false;
}

static size_t
vitte_sema_procedure_parameter_count_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *declaration)
{
    size_t index;
    size_t count = 0u;
    if (context == NULL || declaration == NULL) { return 0u; }
    for (index = 0u; index < declaration->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, declaration->children[index]);
        if (child != NULL && child->kind == VITTE_AST_NODE_PARAMETER) {
            ++count;
        }
    }
    return count;
}

static const vitte_ast_node_t *
vitte_sema_procedure_parameter_at_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *declaration,
    size_t ordinal)
{
    size_t index;
    size_t current = 0u;
    if (context == NULL || declaration == NULL) { return NULL; }
    for (index = 0u; index < declaration->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, declaration->children[index]);
        if (child == NULL || child->kind != VITTE_AST_NODE_PARAMETER) {
            continue;
        }
        if (current == ordinal) { return child; }
        ++current;
    }
    return NULL;
}

static const vitte_ast_node_t *
vitte_sema_parameter_type_node_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *parameter)
{
    size_t index;
    if (context == NULL || parameter == NULL) { return NULL; }
    for (index = 0u; index < parameter->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, parameter->children[index]);
        if (child != NULL &&
            (child->kind == VITTE_AST_NODE_TYPE_EXPR ||
             child->kind == VITTE_AST_NODE_POINTER_TYPE ||
             child->kind == VITTE_AST_NODE_REFERENCE_TYPE ||
             child->kind == VITTE_AST_NODE_ARRAY_TYPE ||
             child->kind == VITTE_AST_NODE_OPTIONAL_TYPE)) {
            return child;
        }
    }
    return NULL;
}

static bool
vitte_sema_type_syntax_equal_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *left,
    const vitte_ast_node_t *right,
    size_t depth)
{
    const vitte_token_t *left_token;
    const vitte_token_t *right_token;
    size_t index;
    if (left == NULL || right == NULL || left->kind != right->kind ||
        left->operator_kind != right->operator_kind ||
        left->flags != right->flags || left->child_count != right->child_count ||
        depth >= context->max_recursion_depth) {
        return false;
    }
    left_token = vitte_sema_node_token(context, left);
    right_token = vitte_sema_node_token(context, right);
    if ((left_token == NULL) != (right_token == NULL)) { return false; }
    if (left_token != NULL &&
        (left_token->kind != right_token->kind ||
         left_token->length != right_token->length ||
         memcmp(left_token->lexeme, right_token->lexeme,
                left_token->length) != 0)) {
        return false;
    }
    for (index = 0u; index < left->child_count; ++index) {
        if (!vitte_sema_type_syntax_equal_private(
                context,
                vitte_sema_node(context, left->children[index]),
                vitte_sema_node(context, right->children[index]),
                depth + 1u)) {
            return false;
        }
    }
    return true;
}

static bool
vitte_sema_procedure_signatures_identical_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *left,
    const vitte_ast_node_t *right)
{
    size_t parameter_count;
    size_t index;
    if (context == NULL || left == NULL || right == NULL ||
        vitte_sema_procedure_has_generics_private(context, left) ||
        vitte_sema_procedure_has_generics_private(context, right)) {
        return true;
    }
    parameter_count = vitte_sema_procedure_parameter_count_private(
        context, left);
    if (parameter_count !=
        vitte_sema_procedure_parameter_count_private(context, right)) {
        return false;
    }
    for (index = 0u; index < parameter_count; ++index) {
        const vitte_ast_node_t *left_parameter =
            vitte_sema_procedure_parameter_at_private(context, left, index);
        const vitte_ast_node_t *right_parameter =
            vitte_sema_procedure_parameter_at_private(context, right, index);
        const vitte_ast_node_t *left_type =
            vitte_sema_parameter_type_node_private(context, left_parameter);
        const vitte_ast_node_t *right_type =
            vitte_sema_parameter_type_node_private(context, right_parameter);
        if (left_parameter == NULL || right_parameter == NULL ||
            left_parameter->flags != right_parameter->flags ||
            !vitte_sema_type_syntax_equal_private(
                context, left_type, right_type, 0u)) {
            return false;
        }
    }
    return true;
}

static bool
vitte_sema_procedure_overload_distinct_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *left,
    const vitte_ast_node_t *right)
{
    if (context == NULL || left == NULL || right == NULL ||
        vitte_sema_procedure_has_generics_private(context, left) ||
        vitte_sema_procedure_has_generics_private(context, right)) {
        return false;
    }
    return !vitte_sema_procedure_signatures_identical_private(
        context, left, right);
}

static void
vitte_sema_append_signature_type_tokens_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *type_node,
    char *buffer,
    size_t capacity,
    size_t *length)
{
    size_t index;
    char previous_character = '\0';
    if (context == NULL || type_node == NULL || buffer == NULL ||
        capacity == 0u || length == NULL || !type_node->span.valid) {
        return;
    }
    for (index = 0u; index < context->parser->token_count; ++index) {
        const vitte_token_t *token = &context->parser->tokens[index];
        size_t character_index;
        if (token->span.begin < type_node->span.begin ||
            token->span.end > type_node->span.end ||
            token->lexeme == NULL || token->length == 0u ||
            *length >= capacity - 1u) {
            continue;
        }
        if ((previous_character == '_' ||
             (previous_character >= 'A' && previous_character <= 'Z') ||
             (previous_character >= 'a' && previous_character <= 'z') ||
             (previous_character >= '0' && previous_character <= '9')) &&
            ((token->lexeme[0] >= 'A' && token->lexeme[0] <= 'Z') ||
             (token->lexeme[0] >= 'a' && token->lexeme[0] <= 'z') ||
             token->lexeme[0] == '_')) {
            buffer[(*length)++] = ' ';
        }
        for (character_index = 0u;
             character_index < token->length && *length < capacity - 1u;
             ++character_index) {
            buffer[(*length)++] = token->lexeme[character_index];
        }
        previous_character = token->length != 0u
            ? token->lexeme[token->length - 1u]
            : previous_character;
    }
    buffer[*length] = '\0';
}

static void
vitte_sema_format_procedure_candidate_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *declaration,
    const char *fallback_name,
    char *buffer,
    size_t capacity)
{
    const char *name = fallback_name;
    size_t name_length = fallback_name != NULL ? strlen(fallback_name) : 0u;
    size_t length = 0u;
    size_t index;
    if (context == NULL || declaration == NULL || buffer == NULL ||
        capacity == 0u) { return; }
    buffer[0] = '\0';
    (void)vitte_sema_declaration_name(
        context, declaration, &name, &name_length);
    if (name == NULL) { name = "procedure"; name_length = 9u; }
    (void)snprintf(buffer, capacity, "%.*s(",
        (int)(name_length > 40u ? 40u : name_length), name);
    length = strlen(buffer);
    for (index = 0u;
         index < vitte_sema_procedure_parameter_count_private(
                     context, declaration) && length < capacity - 1u;
         ++index) {
        const vitte_ast_node_t *parameter =
            vitte_sema_procedure_parameter_at_private(
                context, declaration, index);
        const vitte_ast_node_t *type_node =
            vitte_sema_parameter_type_node_private(context, parameter);
        if (index != 0u && length < capacity - 1u) {
            buffer[length++] = ',';
            buffer[length] = '\0';
        }
        if (type_node != NULL) {
            vitte_sema_append_signature_type_tokens_private(
                context, type_node, buffer, capacity, &length);
        } else if (length < capacity - 1u) {
            buffer[length++] = '?';
            buffer[length] = '\0';
        }
    }
    if (length < capacity - 1u) {
        buffer[length++] = ')';
        buffer[length] = '\0';
    }
}

typedef enum vitte_sema_overload_result {
    VITTE_SEMA_OVERLOAD_NONE = 0,
    VITTE_SEMA_OVERLOAD_SELECTED,
    VITTE_SEMA_OVERLOAD_NO_MATCH,
    VITTE_SEMA_OVERLOAD_AMBIGUOUS,
    VITTE_SEMA_OVERLOAD_ARGUMENT_ERROR
} vitte_sema_overload_result_t;

static vitte_sema_overload_result_t
vitte_sema_resolve_procedure_overload_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *call,
    vitte_symbol_id_t symbol_id,
    const vitte_ast_node_t *fallback_declaration,
    const vitte_symbol_t *symbol,
    const vitte_ast_node_t **selected_declaration)
{
    size_t node_index;
    size_t candidate_count = 0u;
    size_t match_count = 0u;
    const vitte_ast_node_t *first_candidate = NULL;
    const vitte_ast_node_t *second_candidate = NULL;
    const vitte_ast_node_t *first_match = NULL;
    const vitte_ast_node_t *second_match = NULL;
    size_t supplied_count;
    bool argument_error = false;
    if (context == NULL || context->parser == NULL || call == NULL ||
        symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID ||
        selected_declaration == NULL) {
        return VITTE_SEMA_OVERLOAD_NONE;
    }
    supplied_count = call->child_count > 0u ? call->child_count - 1u : 0u;
    for (node_index = 0u; node_index < call->child_count - 1u; ++node_index) {
        const vitte_sema_node_info_t *argument_info =
            vitte_sema_node_info_private(context,
                                         call->children[node_index + 1u]);
        if (argument_info != NULL &&
            argument_info->type_id == context->type_error) {
            argument_error = true;
        }
    }
    if (argument_error) { return VITTE_SEMA_OVERLOAD_ARGUMENT_ERROR; }

    for (node_index = 0u; node_index < context->parser->node_count;
         ++node_index) {
        const vitte_ast_node_t *candidate = vitte_sema_node(
            context, (vitte_ast_node_id_t)(node_index + 1u));
        const vitte_sema_node_info_t *candidate_info;
        size_t parameter_count;
        size_t parameter_index;
        bool matches = true;
        if (candidate == NULL ||
            (candidate->kind != VITTE_AST_NODE_PROC_DECL &&
             candidate->kind != VITTE_AST_NODE_EXTERN_PROC_DECL)) {
            continue;
        }
        candidate_info = vitte_sema_node_info_private(context, candidate->id);
        if (candidate_info == NULL || candidate_info->symbol_id != symbol_id ||
            (candidate_info->flags & VITTE_SEMA_NODE_FLAG_PREDECLARED) == 0u) {
            continue;
        }
        if (first_candidate == NULL) { first_candidate = candidate; }
        else if (second_candidate == NULL) { second_candidate = candidate; }
        ++candidate_count;
        if (vitte_sema_procedure_has_generics_private(context, candidate)) {
            matches = false;
        }
        parameter_count = vitte_sema_procedure_parameter_count_private(
            context, candidate);
        if (parameter_count != supplied_count) { matches = false; }
        if (matches) {
            for (parameter_index = 0u;
                 parameter_index < parameter_count;
                 ++parameter_index) {
                const vitte_ast_node_t *parameter =
                    vitte_sema_procedure_parameter_at_private(
                        context, candidate, parameter_index);
                const vitte_sema_node_info_t *parameter_info = parameter != NULL
                    ? vitte_sema_node_info_private(context, parameter->id)
                    : NULL;
                const vitte_sema_node_info_t *argument_info =
                    vitte_sema_node_info_private(
                        context, call->children[parameter_index + 1u]);
                vitte_sema_type_id_t argument_type;
                const vitte_ast_node_t *argument_node;
                bool compatible;
                if (parameter_info == NULL || argument_info == NULL ||
                    parameter_info->type_id == VITTE_SEMA_INVALID_TYPE_ID ||
                    argument_info->type_id == context->type_error) {
                    matches = false;
                    break;
                }
                argument_type = vitte_sema_type_is_pointer_like_private(
                    context, parameter_info->type_id)
                    ? argument_info->type_id
                    : vitte_sema_value_type_private(
                          context, argument_info->type_id);
                compatible = vitte_sema_type_assignable_private(
                    context, parameter_info->type_id, argument_type);
                argument_node = vitte_sema_node(
                    context, call->children[parameter_index + 1u]);
                if (!compatible && argument_node != NULL &&
                    ((argument_node->kind == VITTE_AST_NODE_INTEGER_LITERAL &&
                      vitte_sema_type_is_integer_private(
                          context, parameter_info->type_id)) ||
                     (argument_node->kind == VITTE_AST_NODE_FLOAT_LITERAL &&
                      vitte_sema_type_is_float_private(
                          context, parameter_info->type_id)))) {
                    compatible = true;
                }
                if (!compatible) { matches = false; break; }
            }
        }
        if (matches) {
            if (first_match == NULL) { first_match = candidate; }
            else if (second_match == NULL) { second_match = candidate; }
            ++match_count;
        }
    }
    if (candidate_count <= 1u) {
        return VITTE_SEMA_OVERLOAD_NONE;
    }
    if (match_count == 1u) {
        *selected_declaration = first_match;
        return VITTE_SEMA_OVERLOAD_SELECTED;
    }
    if (match_count == 0u) {
        char first_signature[160];
        char second_signature[160];
        char help[480];
        vitte_sema_format_procedure_candidate_private(
            context, first_candidate, symbol != NULL ? symbol->name : NULL,
            first_signature, sizeof(first_signature));
        vitte_sema_format_procedure_candidate_private(
            context, second_candidate != NULL ? second_candidate
                                              : fallback_declaration,
            symbol != NULL ? symbol->name : NULL,
            second_signature, sizeof(second_signature));
        (void)snprintf(help, sizeof(help),
            "No overload accepts these arguments. Candidates include `%s` and `%s`; adjust the argument count or types.",
            first_signature, second_signature);
        if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_NO_MATCHING_PROCEDURE,
                call->span,
                call->id,
                symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
            vitte_sema_append_diagnostic_help(
                &context->diagnostics[context->diagnostic_count - 1u], help);
            vitte_sema_set_last_related_location(
                context, first_candidate,
                "candidate procedure declared here");
        }
        return VITTE_SEMA_OVERLOAD_NO_MATCH;
    }
    {
        char first_signature[160];
        char second_signature[160];
        char help[480];
        vitte_sema_format_procedure_candidate_private(
            context, first_match, symbol != NULL ? symbol->name : NULL,
            first_signature, sizeof(first_signature));
        vitte_sema_format_procedure_candidate_private(
            context, second_match, symbol != NULL ? symbol->name : NULL,
            second_signature, sizeof(second_signature));
        (void)snprintf(help, sizeof(help),
            "More than one overload matches this call: `%s` and `%s`. Add an explicit conversion or rename one procedure to make the selection unique.",
            first_signature, second_signature);
        if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_CALL,
                call->span,
                call->id,
                symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
            vitte_sema_append_diagnostic_help(
                &context->diagnostics[context->diagnostic_count - 1u], help);
            vitte_sema_set_last_related_location(
                context, second_match,
                "other matching procedure candidate declared here");
        }
        return VITTE_SEMA_OVERLOAD_AMBIGUOUS;
    }
}

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
    bool unsafe_call_error = false;
    bool procedure_modifier_error = false;
    bool argument_error = false;

    if (node->child_count == 0u) {
        return context->type_error;
    }

    /* `old(value)` is parsed as an ordinary call, but this implementation
       does not retain entry-state snapshots. Diagnose the contract construct
       directly instead of reporting `old` as an unrelated missing name. */
    if (context->current_contract_ensures || context->current_contract_requires) {
        const vitte_ast_node_t *callee = vitte_sema_node(context, node->children[0]);
        const vitte_ast_node_t *callee_name = callee;
        const vitte_token_t *callee_token;
        if (callee != NULL && callee->kind == VITTE_AST_NODE_PATH && callee->child_count != 0u)
            callee_name = vitte_sema_node(context, callee->children[callee->child_count - 1u]);
        callee_token = vitte_sema_node_token(context, callee_name);
        if ((callee != NULL && (callee->kind == VITTE_AST_NODE_IDENTIFIER ||
                (callee->kind == VITTE_AST_NODE_PATH && callee->child_count == 1u))) &&
            callee_token != NULL && callee_token->length == 3u &&
            memcmp(callee_token->lexeme, "old", 3u) == 0 &&
            vitte_scope_lookup_n(&context->scopes, scope_id,
                callee_token->lexeme, callee_token->length) == VITTE_SCOPE_INVALID_SYMBOL_ID) {
            vitte_sema_node_info_t *old_info = vitte_sema_node_info_mut_private(context, node->id);
            if (vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_CONTRACT_OLD_VALUE,
                    node->span, node->id, VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID, VITTE_SEMA_INVALID_TYPE_ID)) {
                const char *help = context->current_contract_requires
                    ? "`old(...)` cannot be used by a requires clause because there is no procedure-entry snapshot at the call site. Use a condition over the current arguments."
                    : "`old(...)` requires an entry-state snapshot, which is not available for this contract reference. Remove `old` or use values directly available to the clause.";
                (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                    sizeof(context->diagnostics[context->diagnostic_count - 1u].help), "%s", help);
            }
            if (old_info != NULL) {
                old_info->type_id = context->type_error;
                old_info->scope_id = scope_id;
                old_info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
            }
            return context->type_error;
        }
    }

    {
        const vitte_ast_node_t *callee_node =
            vitte_sema_node(context, node->children[0]);
        const vitte_ast_node_t *callee_name_node = callee_node;
        const vitte_token_t *callee_token =
            NULL;

        if (callee_node != NULL &&
            callee_node->kind == VITTE_AST_NODE_PATH &&
            callee_node->child_count != 0u) {
            callee_name_node = vitte_sema_node(
                context,
                callee_node->children[callee_node->child_count - 1u]);
        }
        callee_token =
            vitte_sema_node_token(context, callee_name_node);
        if (callee_node != NULL &&
            (callee_node->kind == VITTE_AST_NODE_IDENTIFIER ||
             (callee_node->kind == VITTE_AST_NODE_PATH &&
              callee_node->child_count == 1u)) &&
            callee_token != NULL &&
            callee_token->length == 5u &&
            memcmp(callee_token->lexeme, "print", 5u) == 0 &&
            vitte_scope_lookup_n(
                &context->scopes,
                scope_id,
                callee_token->lexeme,
                callee_token->length) ==
                VITTE_SCOPE_INVALID_SYMBOL_ID) {
            size_t argument_count = node->child_count - 1u;

            if (argument_count != 1u) {
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_COUNT,
                    node->span,
                    node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID);
            }
            for (index = 0u; index < argument_count; ++index) {
                vitte_sema_type_id_t argument_type =
                    vitte_sema_analyze_expression(
                        context, node->children[index + 1u], scope_id);

                if (index == 0u &&
                    argument_type != context->type_error &&
                    !vitte_sema_type_is_string_like_private(
                        context, argument_type)) {
                    const vitte_ast_node_t *argument =
                        vitte_sema_node(
                            context, node->children[index + 1u]);
                    (void)vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE,
                        argument != NULL ? argument->span : node->span,
                        argument != NULL ? argument->id : node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        context->type_string,
                        argument_type);
                }
            }
            result = context->type_void;
            info = vitte_sema_node_info_mut_private(context, node->id);
            if (info != NULL) {
                info->type_id = result;
                info->scope_id = scope_id;
                info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED |
                               VITTE_SEMA_NODE_FLAG_BUILTIN;
            }
            return result;
        }
    }

    {
        const vitte_ast_node_t *callee_node =
            vitte_sema_node(context, node->children[0]);
        vitte_sema_pick_variant_resolution_t pick_resolution;

        if (vitte_sema_resolve_pick_variant_path(
                context,
                callee_node,
                scope_id,
                &pick_resolution)) {
            size_t payload_count =
                pick_resolution.variant_declaration->child_count - 1u;
            size_t argument_count = node->child_count - 1u;
            bool valid = argument_count == payload_count;

            for (index = 0u; index < argument_count; ++index) {
                vitte_sema_type_id_t argument_type =
                    vitte_sema_analyze_expression(
                        context,
                        node->children[index + 1u],
                        scope_id);

                if (index >= payload_count) {
                    continue;
                }
                {
                    vitte_sema_type_id_t expected_type =
                        vitte_sema_analyze_type_node(
                            context,
                            pick_resolution.variant_declaration->children[
                                index + 1u],
                            pick_resolution.pick_scope_id);
                    expected_type =
                        vitte_sema_substitute_generic_type_private(
                            context,
                            expected_type,
                            pick_resolution.pick_type_id);
                    if (!vitte_sema_accept_value_private(
                            context,
                            expected_type,
                            argument_type,
                            node->children[index + 1u])) {
                        const vitte_ast_node_t *argument =
                            vitte_sema_node(
                                context,
                                node->children[index + 1u]);
                        (void)vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE,
                            argument != NULL ? argument->span : node->span,
                            argument != NULL ? argument->id : node->id,
                            pick_resolution.pick_symbol_id,
                            expected_type,
                            argument_type);
                        valid = false;
                    }
                }
            }

            if (argument_count != payload_count) {
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_COUNT,
                    node->span,
                    node->id,
                    pick_resolution.pick_symbol_id,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID);
            }

            result = valid
                ? pick_resolution.pick_type_id
                : context->type_error;
            info = vitte_sema_node_info_mut_private(context, node->id);
            if (info != NULL) {
                info->type_id = result;
                info->symbol_id = pick_resolution.pick_symbol_id;
                info->scope_id = scope_id;
                info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
            }
            return result;
        }
    }

    {
        const vitte_ast_node_t *callee_node =
            vitte_sema_node(context, node->children[0]);
        if (callee_node != NULL &&
            callee_node->kind == VITTE_AST_NODE_MEMBER_EXPR &&
            callee_node->child_count >= 2u &&
            vitte_sema_builtin_kind_from_node(
                context, callee_node) == VITTE_SEMA_BUILTIN_MEMBER_LEN) {
            vitte_sema_type_id_t receiver_type =
                vitte_sema_analyze_expression(
                    context, callee_node->children[0], scope_id);
            const vitte_sema_type_t *receiver =
                vitte_sema_type_private(
                    context,
                    vitte_sema_value_type_private(context, receiver_type));
            while (receiver != NULL &&
                   (receiver->kind == VITTE_SEMA_TYPE_POINTER ||
                    receiver->kind == VITTE_SEMA_TYPE_REFERENCE)) {
                receiver = vitte_sema_type_private(
                    context, receiver->element_type);
            }
            if (receiver != NULL &&
                (receiver->kind == VITTE_SEMA_TYPE_STRING ||
                 receiver->kind == VITTE_SEMA_TYPE_ARRAY ||
                 receiver->kind == VITTE_SEMA_TYPE_SLICE)) {
                vitte_sema_type_id_t function_type =
                    vitte_sema_create_function_type_private(
                        context, NULL, 0u, context->type_u64);
                vitte_sema_node_info_t *member_info =
                    vitte_sema_node_info_mut_private(
                        context, callee_node->id);
                callee_type = function_type;
                if (member_info != NULL) {
                    member_info->type_id = function_type;
                    member_info->scope_id = scope_id;
                    member_info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED |
                                          VITTE_SEMA_NODE_FLAG_BUILTIN;
                }
            } else {
                callee_type = vitte_sema_analyze_expression(
                    context, node->children[0], scope_id);
            }
        } else {
            callee_type = vitte_sema_analyze_expression(
                context, node->children[0], scope_id);
        }
    }
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

    callee_type = vitte_sema_resolve_alias_type_private(
        context, callee_type);
    type =
        vitte_sema_type_private(
            context,
            callee_type);
    while (type != NULL &&
           (type->kind == VITTE_SEMA_TYPE_POINTER ||
            type->kind == VITTE_SEMA_TYPE_REFERENCE)) {
        type = vitte_sema_type_private(context, type->element_type);
    }

    if (callee_type == context->type_error) {
        result = context->type_error;
    } else if (type == NULL ||
        type->kind !=
            VITTE_SEMA_TYPE_FUNCTION) {
        const vitte_ast_node_t *callee_node = node->child_count != 0u
            ? vitte_sema_node(context, node->children[0]) : NULL;
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_NOT_CALLABLE,
            callee_node != NULL ? callee_node->span : node->span,
            callee_node != NULL ? callee_node->id : node->id,
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
            builtin_kind == VITTE_SEMA_BUILTIN_STRING_FROM_SLICE
                ? 3u
                : builtin_kind == VITTE_SEMA_BUILTIN_STRING_FROM_BYTES ||
                  builtin_kind == VITTE_SEMA_BUILTIN_STRING_SLICE ||
                  builtin_kind == VITTE_SEMA_BUILTIN_STARTS_WITH ||
                  builtin_kind == VITTE_SEMA_BUILTIN_ENDS_WITH
                    ? 2u
                    : builtin_kind == VITTE_SEMA_BUILTIN_STRING_AS_BYTES ||
                      builtin_kind == VITTE_SEMA_BUILTIN_STRING_AS_PTR ||
                      builtin_kind == VITTE_SEMA_BUILTIN_MEMBER_LEN
                        ? 0u
                        : 1u;
        result = type->return_type;
        if (node->child_count != expected_arguments + 1u) {
            if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_COUNT,
                node->span,
                node->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_diagnostic_t *diagnostic =
                    &context->diagnostics[context->diagnostic_count - 1u];
                (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                    "This call requires %zu argument(s), but %zu were supplied. %s %zu argument(s) to match the declaration.",
                    expected_arguments, node->child_count - 1u,
                    node->child_count - 1u < expected_arguments ? "Add" : "Remove",
                    node->child_count - 1u < expected_arguments
                        ? expected_arguments - (node->child_count - 1u)
                        : node->child_count - 1u - expected_arguments);
            }
            result = context->type_error;
        } else {
            for (index = 0u;
                 index < expected_arguments;
                 ++index) {
                const vitte_sema_node_info_t *argument_info;
                const vitte_ast_node_t *argument;
                const vitte_ast_node_t *callee_node;
                const vitte_token_t *callee_token;
                const vitte_sema_type_t *argument_type_record;
                vitte_sema_type_id_t argument_expected_type;
                bool argument_valid;

                argument_info =
                    vitte_sema_node_info_private(
                        context,
                        node->children[index + 1u]);
                if (argument_info == NULL) {
                    continue;
                }
                argument_expected_type = context->type_string;
                if (builtin_kind == VITTE_SEMA_BUILTIN_LEN) {
                    argument_type_record = vitte_sema_type_private(
                        context,
                        argument_info->type_id);
                    while (argument_type_record != NULL &&
                           (argument_type_record->kind ==
                                VITTE_SEMA_TYPE_REFERENCE ||
                            argument_type_record->kind ==
                                VITTE_SEMA_TYPE_POINTER)) {
                        argument_type_record = vitte_sema_type_private(
                            context,
                            argument_type_record->element_type);
                    }
                    argument_valid = argument_type_record != NULL &&
                        (argument_type_record->kind ==
                             VITTE_SEMA_TYPE_STRING ||
                         argument_type_record->kind ==
                             VITTE_SEMA_TYPE_ARRAY ||
                         argument_type_record->kind ==
                             VITTE_SEMA_TYPE_SLICE);
                } else if (builtin_kind == VITTE_SEMA_BUILTIN_STRING_FROM ||
                    builtin_kind ==
                        VITTE_SEMA_BUILTIN_STRING_FROM_SLICE) {
                    if (index == 0u) {
                        argument_valid =
                            vitte_sema_type_is_string_like_private(
                                context,
                                argument_info->type_id);
                    } else {
                        argument_expected_type = context->type_u64;
                        argument_valid =
                            vitte_sema_type_is_integer_private(
                                context,
                                argument_info->type_id);
                    }
                } else if (builtin_kind ==
                               VITTE_SEMA_BUILTIN_STRING_FROM_BYTE ||
                           builtin_kind ==
                               VITTE_SEMA_BUILTIN_STRING_WITH_CAPACITY) {
                    argument_expected_type = context->type_u64;
                    argument_valid = vitte_sema_type_is_integer_private(
                        context, argument_info->type_id);
                } else if (builtin_kind ==
                           VITTE_SEMA_BUILTIN_STRING_FROM_BYTES) {
                    if (index == 0u) {
                        argument_expected_type = vitte_sema_create_type_private(
                            context,
                            VITTE_SEMA_TYPE_POINTER,
                            context->type_u8,
                            0u,
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            false);
                        argument_valid =
                            vitte_sema_type_is_pointer_like_private(
                                context, argument_info->type_id) ||
                            vitte_sema_type_is_string_like_private(
                                context, argument_info->type_id);
                    } else {
                        argument_expected_type = context->type_u64;
                        argument_valid = vitte_sema_type_is_integer_private(
                            context, argument_info->type_id);
                    }
                } else if (builtin_kind ==
                           VITTE_SEMA_BUILTIN_STRING_SLICE) {
                    argument_expected_type = context->type_u64;
                    argument_valid = vitte_sema_type_is_integer_private(
                        context, argument_info->type_id);
                } else if (builtin_kind ==
                           VITTE_SEMA_BUILTIN_ARRAY_RESERVE) {
                    argument_expected_type = context->type_u64;
                    argument_valid = vitte_sema_type_is_integer_private(
                        context, argument_info->type_id);
                } else if (builtin_kind ==
                           VITTE_SEMA_BUILTIN_ARRAY_PUSH) {
                    const vitte_ast_node_t *callee =
                        vitte_sema_node(context, node->children[0]);
                    const vitte_ast_node_t *receiver = callee != NULL &&
                            callee->kind == VITTE_AST_NODE_MEMBER_EXPR &&
                            callee->child_count != 0u
                        ? vitte_sema_node(context, callee->children[0])
                        : NULL;
                    const vitte_sema_node_info_t *receiver_info =
                        receiver != NULL
                        ? vitte_sema_node_info_private(context, receiver->id)
                        : NULL;
                    const vitte_sema_type_t *array_type = receiver_info != NULL
                        ? vitte_sema_type_private(context, receiver_info->type_id)
                        : NULL;

                    while (array_type != NULL &&
                           (array_type->kind == VITTE_SEMA_TYPE_REFERENCE ||
                            array_type->kind == VITTE_SEMA_TYPE_POINTER)) {
                        array_type = vitte_sema_type_private(
                            context, array_type->element_type);
                    }
                    if (array_type != NULL &&
                        (array_type->kind == VITTE_SEMA_TYPE_ARRAY ||
                         array_type->kind == VITTE_SEMA_TYPE_SLICE)) {
                        argument_expected_type = array_type->element_type;
                        argument_valid = vitte_sema_type_assignable_private(
                            context,
                            argument_expected_type,
                            argument_info->type_id);
                    } else {
                        argument_valid = false;
                    }
                } else {
                    argument_valid =
                        vitte_sema_type_assignable_private(
                            context,
                            context->type_string,
                            argument_info->type_id);
                }
                if (argument_valid) {
                    continue;
                }
                argument =
                    vitte_sema_node(
                        context,
                        node->children[index + 1u]);
                if (vitte_sema_add_diagnostic(
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
                        argument_expected_type,
                        argument_info->type_id)) {
                    vitte_sema_diagnostic_t *diagnostic;
                    char help[256];

                    vitte_sema_set_last_type_context(
                        context,
                        VITTE_SEMA_TYPE_CONTEXT_ARGUMENT,
                        index,
                        VITTE_SCOPE_INVALID_SYMBOL_ID);

                    diagnostic =
                        &context->diagnostics[
                            context->diagnostic_count - 1u];
                    callee_node = vitte_sema_node(
                        context,
                        node->children[0]);
                    callee_token = vitte_sema_node_token(
                        context,
                        callee_node);
                    if (callee_token != NULL &&
                        callee_token->lexeme != NULL &&
                        callee_token->length != 0u) {
                        size_t name_length =
                            callee_token->length < 96u
                                ? callee_token->length
                                : 96u;
                        (void)snprintf(
                            help,
                            sizeof(help),
                            "Argument %zu to builtin `%.*s` has an incompatible type.",
                            index + 1u,
                            (int)name_length,
                            callee_token->lexeme);
                    } else {
                        (void)snprintf(
                            help,
                            sizeof(help),
                            "Argument %zu to this builtin has an incompatible type.",
                            index + 1u);
                    }
                    vitte_sema_append_diagnostic_help(
                        diagnostic,
                        help);
                    vitte_sema_add_safe_integer_conversion_help(
                        context,
                        node->children[index + 1u],
                        argument_expected_type,
                        argument_info->type_id);
                }
                result = context->type_error;
            }
        }
    } else {
        const vitte_symbol_t *function_symbol;
        const vitte_ast_node_t *function_declaration;
        size_t parameter_count;
        size_t parameter_index;
        size_t argument_index;
        size_t generic_parameter_count;
        size_t generic_binding_count;
        vitte_sema_generic_call_binding_private_t *generic_bindings;

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
        if (function_declaration != NULL &&
            type->symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID) {
            vitte_sema_overload_result_t overload_result =
                vitte_sema_resolve_procedure_overload_private(
                    context,
                    node,
                    type->symbol_id,
                    function_declaration,
                    function_symbol,
                    &function_declaration);
            if (overload_result == VITTE_SEMA_OVERLOAD_NO_MATCH ||
                overload_result == VITTE_SEMA_OVERLOAD_AMBIGUOUS ||
                overload_result == VITTE_SEMA_OVERLOAD_ARGUMENT_ERROR) {
                info = vitte_sema_node_info_mut_private(context, node->id);
                if (info != NULL) {
                    info->type_id = context->type_error;
                    info->scope_id = scope_id;
                }
                context->stats.calls_checked =
                    vitte_sema_u64_add_sat(
                        context->stats.calls_checked, UINT64_C(1));
                return context->type_error;
            }
        }
        if (function_declaration != NULL) {
            bool callee_comptime =
                (function_declaration->flags & VITTE_AST_FLAG_COMPTIME) != 0u;
            bool callee_external =
                (function_declaration->flags & VITTE_AST_FLAG_EXTERN) != 0u;
            bool callee_intrinsic =
                (function_declaration->flags & VITTE_AST_FLAG_INTRINSIC) != 0u;
            if ((callee_comptime && !context->current_comptime) ||
                (context->current_comptime && !callee_comptime) ||
                (context->current_comptime &&
                 (callee_external || callee_intrinsic))) {
                const char *help = callee_comptime
                    ? "A `comptime` procedure must be called from a compile-time procedure or constant-evaluation context."
                    : "A `comptime` procedure cannot call a runtime, external, or intrinsic procedure.";
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_PROCEDURE_MODIFIER,
                        node->span,
                        node->id,
                        type->symbol_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[
                            context->diagnostic_count - 1u],
                        help);
                    vitte_sema_set_last_related_location(
                        context,
                        function_declaration,
                        "called procedure declared here");
                }
                procedure_modifier_error = true;
            }
        }
        if (function_declaration != NULL &&
            (function_declaration->flags & VITTE_AST_FLAG_UNSAFE) != 0u &&
            !context->current_unsafe) {
            if (vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_UNSAFE_REQUIRED,
                    vitte_sema_keyword_span_in_node_private(
                        context, node, "asm"),
                    node->id,
                    type->symbol_id,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_diagnostic_t *diagnostic =
                    &context->diagnostics[context->diagnostic_count - 1u];
                (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                    "Calling an unsafe procedure requires an unsafe context. Wrap this call in `unsafe { ... }` or call it from an unsafe procedure.");
                vitte_sema_set_last_related_location(
                    context, function_declaration,
                    "unsafe procedure declared here");
            }
            unsafe_call_error = true;
        }
        generic_parameter_count = function_declaration != NULL
            ? vitte_sema_generic_parameter_count_private(
                  context, type->symbol_id)
            : 0u;
        generic_binding_count = 0u;
        generic_bindings = generic_parameter_count != 0u
            ? (vitte_sema_generic_call_binding_private_t *)calloc(
                  generic_parameter_count,
                  sizeof(*generic_bindings))
            : NULL;
        if (generic_parameter_count != 0u && generic_bindings == NULL) {
            context->last_error = VITTE_SEMA_ERROR_OUT_OF_MEMORY;
            result = context->type_error;
        }
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
        } else if (type->symbol_id ==
                   VITTE_SCOPE_INVALID_SYMBOL_ID) {
            parameter_count = type->function_parameter_count;
        }

        if (parameter_count != node->child_count - 1u) {
            if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_COUNT,
                node->span,
                node->id,
                type->symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_diagnostic_t *diagnostic =
                    &context->diagnostics[context->diagnostic_count - 1u];
                const vitte_ast_node_t *related_declaration =
                    function_declaration;
                if (function_declaration != NULL &&
                    node->child_count - 1u < parameter_count) {
                    size_t ordinal = 0u;
                    for (parameter_index = 0u;
                         parameter_index < function_declaration->child_count;
                         ++parameter_index) {
                        const vitte_ast_node_t *parameter = vitte_sema_node(
                            context,
                            function_declaration->children[parameter_index]);
                        if (parameter == NULL ||
                            parameter->kind != VITTE_AST_NODE_PARAMETER) {
                            continue;
                        }
                        if (ordinal == node->child_count - 1u) {
                            related_declaration = parameter;
                            break;
                        }
                        ++ordinal;
                    }
                }
                (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                    "This call requires %zu argument(s), but %zu were supplied. %s %zu argument(s) to match the declaration.",
                    parameter_count, node->child_count - 1u,
                    node->child_count - 1u < parameter_count ? "Add" : "Remove",
                    node->child_count - 1u < parameter_count
                        ? parameter_count - (node->child_count - 1u)
                        : node->child_count - 1u - parameter_count);
                if (related_declaration == function_declaration) {
                    vitte_sema_set_last_related_location(
                        context,
                        related_declaration,
                        "procedure signature declared here");
                } else {
                    const char *parameter_name = NULL;
                    size_t parameter_name_length = 0u;
                    char label[128];
                    (void)vitte_sema_declaration_name(
                        context,
                        related_declaration,
                        &parameter_name,
                        &parameter_name_length);
                    (void)snprintf(
                        label,
                        sizeof(label),
                        "missing parameter %zu%s%.*s%s declared here",
                        node->child_count,
                        parameter_name != NULL ? " (`" : "",
                        (int)(parameter_name_length > 64u
                                  ? 64u
                                  : parameter_name_length),
                        parameter_name != NULL ? parameter_name : "",
                        parameter_name != NULL ? "`)" : "");
                    vitte_sema_set_last_related_location(
                        context, related_declaration, label);
                }
            }
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

                if (argument_info == NULL ||
                    argument_info->type_id == context->type_error ||
                    parameter_info == NULL ||
                    parameter_info->type_id == context->type_error) {
                    argument_error = true;
                }

                if (parameter_info != NULL &&
                    argument_info != NULL &&
                    parameter_info->type_id != context->type_error &&
                    argument_info->type_id != context->type_error) {
                    vitte_sema_type_id_t argument_type;
                    bool argument_compatible;
                    size_t diagnostic_count_before_unify =
                        context->diagnostic_count;

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

                    if (generic_parameter_count != 0u &&
                        generic_bindings != NULL) {
                        argument_compatible =
                            vitte_sema_generic_call_unify_type_private(
                                context,
                                generic_bindings,
                                &generic_binding_count,
                                generic_parameter_count,
                                parameter_info->type_id,
                                argument_type,
                                vitte_sema_node(
                                    context,
                                    node->children[argument_index]),
                                true,
                                0u);
                    } else {
                        argument_compatible =
                            vitte_sema_accept_value_private(
                                context,
                                parameter_info->type_id,
                                argument_type,
                                node->children[argument_index]);
                    }

                    if (!argument_compatible) {
                        const vitte_ast_node_t *argument;

                        argument =
                            vitte_sema_node(
                                context,
                                node->children[argument_index]);

                        bool inference_conflict =
                            context->diagnostic_count >
                                diagnostic_count_before_unify &&
                            context->diagnostics[
                                context->diagnostic_count - 1u].code ==
                                VITTE_SEMA_DIAGNOSTIC_GENERIC_INFERENCE_CONFLICT;
                        if (!inference_conflict && vitte_sema_add_diagnostic(
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
                                argument_info->type_id)) {
                            const char *parameter_name;
                            size_t parameter_name_length;
                            vitte_sema_diagnostic_t *diagnostic;
                            char help[256];
                            char related_label[128];

                            vitte_sema_set_last_type_context(
                                context,
                                VITTE_SEMA_TYPE_CONTEXT_ARGUMENT,
                                argument_index - 1u,
                                parameter_info->symbol_id);

                            diagnostic =
                                &context->diagnostics[
                                    context->diagnostic_count - 1u];
                            parameter_name = NULL;
                            parameter_name_length = 0u;
                            (void)vitte_sema_declaration_name(
                                context,
                                parameter,
                                &parameter_name,
                                &parameter_name_length);
                            if (function_symbol != NULL &&
                                function_symbol->name != NULL &&
                                parameter_name != NULL &&
                                parameter_name_length != 0u) {
                                size_t shown_parameter_length =
                                    parameter_name_length < 72u
                                        ? parameter_name_length
                                        : 72u;
                                size_t function_name_length =
                                    function_symbol->name_length < 72u
                                        ? function_symbol->name_length
                                        : 72u;
                                (void)snprintf(
                                    help,
                                    sizeof(help),
                                    "Argument %zu does not match parameter `%.*s` of procedure `%.*s`.",
                                    argument_index,
                                    (int)shown_parameter_length,
                                    parameter_name,
                                    (int)function_name_length,
                                    function_symbol->name);
                            } else if (parameter_name != NULL &&
                                       parameter_name_length != 0u) {
                                size_t shown_parameter_length =
                                    parameter_name_length < 128u
                                        ? parameter_name_length
                                        : 128u;
                                (void)snprintf(
                                    help,
                                    sizeof(help),
                                    "Argument %zu does not match parameter `%.*s`.",
                                    argument_index,
                                    (int)shown_parameter_length,
                                    parameter_name);
                            } else {
                                (void)snprintf(
                                    help,
                                    sizeof(help),
                                    "Argument %zu does not match this procedure's parameter type.",
                                    argument_index);
                            }
                            vitte_sema_append_diagnostic_help(
                                diagnostic,
                                help);
                            if (parameter_name != NULL &&
                                parameter_name_length != 0u) {
                                (void)snprintf(
                                    related_label,
                                    sizeof(related_label),
                                    "parameter %zu (`%.*s`) declared here",
                                    argument_index,
                                    (int)(parameter_name_length > 64u
                                              ? 64u
                                              : parameter_name_length),
                                    parameter_name);
                            } else {
                                (void)snprintf(
                                    related_label,
                                    sizeof(related_label),
                                    "parameter %zu declared here",
                                    argument_index);
                            }
                            vitte_sema_set_last_related_location(
                                context, parameter, related_label);
                            vitte_sema_add_safe_integer_conversion_help(
                                context,
                                node->children[argument_index],
                                parameter_info->type_id,
                                argument_info->type_id);
                        }
                        result = context->type_error;
                    }
                }

                ++parameter_index;
                ++argument_index;
            }
            if (generic_parameter_count != 0u &&
                generic_bindings != NULL) {
                if (generic_binding_count != generic_parameter_count) {
                    if (result != context->type_error && !argument_error) {
                        const vitte_ast_node_t *callee =
                            node->child_count > 0u
                                ? vitte_sema_node(
                                      context, node->children[0])
                                : NULL;
                        if (vitte_sema_add_diagnostic(
                                context,
                                VITTE_SEMA_DIAGNOSTIC_ERROR,
                                VITTE_SEMA_DIAGNOSTIC_CANNOT_INFER,
                                callee != NULL ? callee->span : node->span,
                                callee != NULL ? callee->id : node->id,
                                type->symbol_id,
                                VITTE_SEMA_INVALID_TYPE_ID,
                                VITTE_SEMA_INVALID_TYPE_ID)) {
                            vitte_sema_set_last_type_context(
                                context,
                                VITTE_SEMA_TYPE_CONTEXT_INFERENCE,
                                SIZE_MAX,
                                type->symbol_id);
                            (void)snprintf(
                                context->diagnostics[
                                    context->diagnostic_count - 1u].help,
                                sizeof(context->diagnostics[
                                    context->diagnostic_count - 1u].help),
                                "Provide explicit generic arguments at this call or annotate an argument so every generic type can be inferred.");
                        }
                    }
                    result = context->type_error;
                } else {
                    result = vitte_sema_generic_call_substitute_type_private(
                        context,
                        generic_bindings,
                        generic_binding_count,
                        type->return_type,
                        0u);
                    if (result == context->type_error ||
                        result == VITTE_SEMA_INVALID_TYPE_ID) {
                        result = context->type_error;
                    }
                }
                if (result != context->type_error &&
                    generic_binding_count == generic_parameter_count &&
                    !vitte_sema_generic_call_check_constraints_private(
                        context,
                        function_declaration,
                        generic_bindings,
                        generic_binding_count,
                        node,
                        0u)) {
                    result = context->type_error;
                }
            }
            if (function_declaration != NULL &&
                (function_declaration->flags & VITTE_AST_FLAG_ASYNC) != 0u &&
                result != context->type_error) {
                result = vitte_sema_create_type_private(
                    context,
                    VITTE_SEMA_TYPE_TASK,
                    result,
                    0u,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    false);
                if (result == VITTE_SEMA_INVALID_TYPE_ID) {
                    result = context->type_error;
                }
            }
            free(generic_bindings);
        } else if (type->symbol_id ==
                   VITTE_SCOPE_INVALID_SYMBOL_ID) {
            for (parameter_index = 0u;
                 parameter_index < type->function_parameter_count;
                 ++parameter_index) {
                const vitte_sema_node_info_t *argument_info =
                    vitte_sema_node_info_private(
                        context,
                        node->children[parameter_index + 1u]);
                vitte_sema_type_id_t expected_type =
                    type->function_parameter_types[parameter_index];
                vitte_sema_type_id_t argument_type;

                if (argument_info == NULL) {
                    continue;
                }
                argument_type = vitte_sema_type_is_pointer_like_private(
                    context, expected_type)
                    ? argument_info->type_id
                    : vitte_sema_value_type_private(
                          context, argument_info->type_id);
                if (vitte_sema_accept_value_private(
                        context,
                        expected_type,
                        argument_type,
                        node->children[parameter_index + 1u])) {
                    continue;
                }
                const vitte_ast_node_t *argument = vitte_sema_node(
                    context,
                    node->children[parameter_index + 1u]);
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_ARGUMENT_TYPE,
                        argument != NULL ? argument->span : node->span,
                        argument != NULL ? argument->id : node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        expected_type,
                        argument_info->type_id)) {
                    vitte_sema_set_last_type_context(
                        context,
                        VITTE_SEMA_TYPE_CONTEXT_ARGUMENT,
                        parameter_index,
                        VITTE_SCOPE_INVALID_SYMBOL_ID);
                }
                result = context->type_error;
            }
        }
    }

    if (unsafe_call_error || procedure_modifier_error) {
        result = context->type_error;
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
    const vitte_sema_type_t *original_object;
    const vitte_sema_node_info_t *object_info;
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

    if (index_type != context->type_error && !vitte_sema_type_is_integer_private(
            context,
            index_type)) {
        const vitte_ast_node_t *index_node = node->child_count > 1u
            ? vitte_sema_node(context, node->children[1]) : NULL;
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_INVALID_INDEX_TYPE,
            index_node != NULL ? index_node->span : node->span,
            index_node != NULL ? index_node->id : node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            index_type);
    }

    original_object =
        vitte_sema_type_private(
            context,
            object_type);

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
                VITTE_SEMA_TYPE_SLICE ||
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

    if (object_type != context->type_error && result ==
        context->type_error) {
        const vitte_ast_node_t *object_node = node->child_count != 0u
            ? vitte_sema_node(context, node->children[0]) : NULL;
        (void)vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_NOT_INDEXABLE,
            object_node != NULL ? object_node->span : node->span,
            object_node != NULL ? object_node->id : node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            object_type);
    }

    if (index_type == context->type_error ||
        !vitte_sema_type_is_integer_private(context, index_type)) result = context->type_error;

    info =
        vitte_sema_node_info_mut_private(
            context,
            node->id);

    if (info != NULL) {
        info->type_id = result;
        object_info = vitte_sema_node_info_private(
            context,
            node->children[0]);
        if (object_info != NULL) {
            info->symbol_id = object_info->symbol_id;
            if ((object_info->flags &
                 VITTE_SEMA_NODE_FLAG_ADDRESSABLE) != 0u ||
                (object != NULL &&
                 object->kind == VITTE_SEMA_TYPE_POINTER) ||
                (original_object != NULL &&
                 original_object->kind == VITTE_SEMA_TYPE_POINTER)) {
                info->flags |= VITTE_SEMA_NODE_FLAG_LVALUE |
                               VITTE_SEMA_NODE_FLAG_ADDRESSABLE;
            }
            if ((object_info->flags &
                 VITTE_SEMA_NODE_FLAG_MUTABLE) != 0u) {
                info->flags |= VITTE_SEMA_NODE_FLAG_MUTABLE;
            }
        }
        if (original_object != NULL &&
            (original_object->kind == VITTE_SEMA_TYPE_POINTER ||
             original_object->kind == VITTE_SEMA_TYPE_REFERENCE) &&
            original_object->is_mutable) {
            info->flags |= VITTE_SEMA_NODE_FLAG_MUTABLE;
        }
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
            if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_UNKNOWN_MEMBER,
                name->span,
                name->id,
                form_type->symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                type_id)) {
                vitte_sema_set_last_related_location(
                    context,
                    form_declaration,
                    "form fields declared here");
            }
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
        field_type = vitte_sema_substitute_generic_type_private(
            context,
            field_type,
            type_id);
        value_type = vitte_sema_analyze_expression(
            context,
            initializer->children[1],
            scope_id);
        if (!vitte_sema_accept_value_private(
                context,
                field_type,
                value_type,
                initializer->children[1])) {
            if (vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                    initializer->span,
                    initializer->id,
                    field_symbol_id,
                    field_type,
                    value_type)) {
                vitte_sema_diagnostic_t *diagnostic;
                char help[256];

                vitte_sema_set_last_type_context(
                    context,
                    VITTE_SEMA_TYPE_CONTEXT_FIELD,
                    SIZE_MAX,
                    field_symbol_id);

                diagnostic =
                    &context->diagnostics[
                        context->diagnostic_count - 1u];
                if (field_symbol != NULL &&
                    field_symbol->name != NULL &&
                    form_symbol->name != NULL) {
                    size_t field_name_length =
                        field_symbol->name_length < 72u
                            ? field_symbol->name_length
                            : 72u;
                    size_t form_name_length =
                        form_symbol->name_length < 72u
                            ? form_symbol->name_length
                            : 72u;
                    (void)snprintf(
                        help,
                        sizeof(help),
                        "Value for field `%.*s` of form `%.*s` does not match its declared type.",
                        (int)field_name_length,
                        field_symbol->name,
                        (int)form_name_length,
                        form_symbol->name);
                } else if (field_symbol != NULL &&
                           field_symbol->name != NULL) {
                    size_t field_name_length =
                        field_symbol->name_length < 128u
                            ? field_symbol->name_length
                            : 128u;
                    (void)snprintf(
                        help,
                        sizeof(help),
                        "Value for field `%.*s` does not match its declared type.",
                        (int)field_name_length,
                        field_symbol->name);
                } else {
                    (void)snprintf(
                        help,
                        sizeof(help),
                        "This field value does not match the field's declared type.");
                }
                vitte_sema_append_diagnostic_help(
                    diagnostic,
                    help);
                vitte_sema_add_safe_integer_conversion_help(
                    context,
                    initializer->children[1],
                    field_type,
                    value_type);
            }
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
        {
            vitte_sema_pick_variant_resolution_t pick_resolution;

            if (vitte_sema_resolve_pick_variant_path(
                    context,
                    node,
                    scope_id,
                    &pick_resolution)) {
                result = vitte_sema_type_for_symbol(
                    context,
                    pick_resolution.pick_symbol_id);
                if (info != NULL) {
                    info->type_id = result;
                    info->symbol_id = pick_resolution.pick_symbol_id;
                    info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
                }
            } else if (node->child_count == 1u) {
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

                            if (vitte_sema_add_diagnostic(
                                context,
                                VITTE_SEMA_DIAGNOSTIC_ERROR,
                                diagnostic_code,
                                segment->span,
                                segment->id,
                                hidden_symbol_id,
                                VITTE_SEMA_INVALID_TYPE_ID,
                                VITTE_SEMA_INVALID_TYPE_ID)) {
                                if (diagnostic_code ==
                                    VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME) {
                                    vitte_sema_help_similar_name(
                                        context,
                                        path_scope,
                                        scope_id,
                                        segment_name,
                                        segment_name_length,
                                        diagnostic_code,
                                        false);
                                } else {
                                    vitte_sema_help_private_symbol(
                                        context,
                                        hidden_symbol);
                                }
                            }
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
                        if (vitte_symbol_kind_is_type(symbol->kind)) {
                            const vitte_ast_node_t *last_segment = segment;
                            if (vitte_sema_add_diagnostic(
                                    context,
                                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                                    VITTE_SEMA_DIAGNOSTIC_NOT_A_VALUE,
                                    last_segment != NULL
                                        ? last_segment->span : node->span,
                                    last_segment != NULL
                                        ? last_segment->id : node->id,
                                    path_symbol,
                                    VITTE_SEMA_INVALID_TYPE_ID,
                                    vitte_sema_type_for_symbol(
                                        context, path_symbol))) {
                                vitte_sema_append_diagnostic_help(
                                    &context->diagnostics[
                                        context->diagnostic_count - 1u],
                                    "This path names a type, not a runtime value. Use it in a type position or construct a value with the type's supported syntax.");
                            }
                            result = context->type_error;
                        } else {
                            result = vitte_sema_type_for_symbol(
                                context, path_symbol);
                            if (info != NULL) {
                                info->symbol_id = path_symbol;
                                info->flags |=
                                    VITTE_SEMA_NODE_FLAG_RESOLVED;
                            }
                        }
                    }
                }
            }
            break;
        }

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
                        VITTE_SEMA_NODE_FLAG_LVALUE |
                        VITTE_SEMA_NODE_FLAG_ADDRESSABLE;

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
                const vitte_sema_node_info_t *child_info;

                result =
                    vitte_sema_analyze_expression(
                        context,
                        node->children[0],
                        scope_id);

                child_info = vitte_sema_node_info_private(
                    context,
                    node->children[0]);
                if (info != NULL && child_info != NULL) {
                    info->symbol_id = child_info->symbol_id;
                    info->flags |= child_info->flags &
                        (VITTE_SEMA_NODE_FLAG_LVALUE |
                         VITTE_SEMA_NODE_FLAG_MUTABLE |
                         VITTE_SEMA_NODE_FLAG_ADDRESSABLE);
                }
            } else {
                result =
                    context->type_error;
            }
            break;

        case VITTE_AST_NODE_IF_EXPR:
        {
            vitte_constant_t condition_value;
            vitte_sema_type_id_t condition_type;
            vitte_sema_type_id_t then_type;
            vitte_sema_type_id_t else_type;
            const vitte_ast_node_t *condition;
            const vitte_ast_node_t *then_branch;
            const vitte_ast_node_t *else_branch;

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
            then_branch = vitte_sema_node(
                context, node->children[1]);
            else_branch = vitte_sema_node(
                context, node->children[2]);
            if (condition_type != context->type_bool &&
                condition_type != context->type_error &&
                condition != NULL) {
                if (vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_CONDITION_NOT_BOOL,
                    condition->span,
                    condition->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    context->type_bool,
                    condition_type)) {
                    vitte_sema_set_last_type_context(
                        context,
                        VITTE_SEMA_TYPE_CONTEXT_CONDITION,
                        SIZE_MAX,
                        VITTE_SCOPE_INVALID_SYMBOL_ID);
                }
            }
            condition_value = vitte_sema_known_value(context, condition);
            if (!vitte_sema_is_unreachable_private(context, node) &&
                condition_value.kind == VITTE_CONSTANT_KIND_BOOL) {
                const vitte_ast_node_t *dead_branch =
                    condition_value.as.boolean ? else_branch : then_branch;
                if (dead_branch != NULL) {
                    vitte_sema_warn_unreachable_private(
                        context, dead_branch, condition,
                        "this condition makes the branch unreachable",
                        condition_value.as.boolean
                            ? "This condition is always true, so this alternative cannot run. Change the condition or remove the unreachable alternative."
                            : "This condition is always false, so this branch cannot run. Change the condition or remove the unreachable branch.");
                }
            } else if (vitte_sema_is_unreachable_private(context, node)) {
                vitte_sema_mark_unreachable_private(context, then_branch);
                vitte_sema_mark_unreachable_private(context, else_branch);
            }
            then_type = vitte_sema_analyze_expression(
                context,
                node->children[1],
                scope_id);
            else_type = vitte_sema_analyze_expression(
                context,
                node->children[2],
                scope_id);
            if (vitte_sema_accept_value_private(
                    context,
                    then_type,
                    else_type,
                    node->children[2])) {
                result = then_type;
            } else if (vitte_sema_accept_value_private(
                           context,
                           else_type,
                           then_type,
                           node->children[1])) {
                result = else_type;
            } else {
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                        else_branch != NULL
                            ? else_branch->span
                            : node->span,
                        else_branch != NULL
                            ? else_branch->id
                            : node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        then_type,
                        else_type)) {
                    vitte_sema_add_safe_integer_conversion_help(
                        context,
                        node->children[2],
                        then_type,
                        else_type);
                }
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
                } else if (!vitte_sema_accept_value_private(
                               context,
                               element_type,
                               item_type,
                               node->children[child_index])) {
                    const vitte_ast_node_t *item;

                    if (vitte_sema_accept_value_private(
                            context,
                            item_type,
                            element_type,
                            node->children[child_index - 1u])) {
                        element_type = item_type;
                    } else {
                        item = vitte_sema_node(
                            context,
                            node->children[child_index]);
                        if (item != NULL) {
                            if (vitte_sema_add_diagnostic(
                                    context,
                                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                                    VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                                    item->span,
                                    item->id,
                                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                                    element_type,
                                    item_type)) {
                                vitte_sema_add_safe_integer_conversion_help(
                                    context,
                                    node->children[child_index],
                                    element_type,
                                    item_type);
                            }
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
                const vitte_ast_node_t *source_node;

                source_node = vitte_sema_node(
                    context,
                    node->children[0]);
                source_type = vitte_sema_analyze_expression(
                    context,
                    node->children[0],
                    scope_id);
                target_type = vitte_sema_analyze_type_node(
                    context,
                    node->children[1],
                    scope_id);
                if (target_type == context->type_u64 &&
                    source_type != target_type &&
                    vitte_sema_contextualize_integer_literal(
                        context,
                        node->children[0],
                        context->type_u64)) {
                    source_type = context->type_u64;
                }
                if (source_type == context->type_error ||
                    target_type == context->type_error) {
                    result = context->type_error;
                } else if (vitte_sema_type_conversion_private(
                               context,
                               target_type,
                               source_type) !=
                               VITTE_SEMA_TYPE_CONVERSION_INCOMPATIBLE ||
                           (vitte_sema_type_is_pointer_like_private(
                                context, source_type) &&
                            vitte_sema_type_is_pointer_like_private(
                                context, target_type)) ||
                           (vitte_sema_type_is_pointer_like_private(
                                context, source_type) &&
                            vitte_sema_type_is_integer_private(
                                context, target_type)) ||
                           (vitte_sema_type_is_integer_private(
                                context, source_type) &&
                            vitte_sema_type_is_pointer_like_private(
                                context, target_type)) ||
                           (vitte_sema_type_is_numeric_private(
                                context,
                                source_type) &&
                            vitte_sema_type_is_numeric_private(
                                context,
                                target_type)) ||
                           (vitte_sema_type_is_enum_private(
                                context,
                                source_type) &&
                            vitte_sema_type_is_integer_private(
                                context,
                                target_type)) ||
                           (vitte_sema_type_is_integer_private(
                                context,
                                source_type) &&
                            vitte_sema_type_is_enum_private(
                                context,
                                target_type))) {
                    result = target_type;
                } else {
                    (void)vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_CAST,
                        source_node != NULL
                            ? source_node->span
                            : node->span,
                        source_node != NULL
                            ? source_node->id
                            : node->id,
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
            result = context->type_error;
            if (node->child_count == 1u) {
                vitte_sema_type_id_t task_type_id =
                    vitte_sema_analyze_expression(
                        context,
                        node->children[0],
                        scope_id);
                const vitte_sema_type_t *task_type =
                    vitte_sema_type_private(context, task_type_id);
                if (!context->current_async) {
                    const vitte_token_t *await_token =
                        vitte_sema_node_token(context, node);
                    vitte_parser_span_t await_span = await_token != NULL
                        ? vitte_sema_keyword_span_in_node_private(
                              context, node, "await")
                        : node->span;
                    if (vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_ASYNC_CONTEXT,
                            await_span,
                            node->id,
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            task_type_id)) {
                        vitte_sema_append_diagnostic_help(
                            &context->diagnostics[
                                context->diagnostic_count - 1u],
                            "`await` is only valid inside an `async proc`; mark the enclosing procedure `async` or return the task to its caller.");
                    }
                    result = context->type_error;
                } else if (task_type != NULL &&
                    task_type->kind == VITTE_SEMA_TYPE_TASK) {
                    result = task_type->element_type;
                } else if (task_type_id != context->type_error) {
                    if (vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_TASK_TYPE,
                            vitte_sema_node(context, node->children[0]) != NULL
                                ? vitte_sema_node(
                                      context, node->children[0])->span
                                : node->span,
                            node->children[0],
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            task_type_id)) {
                        vitte_sema_append_diagnostic_help(
                            &context->diagnostics[
                                context->diagnostic_count - 1u],
                            "`await` requires a task value returned by an async procedure; remove `await` or call an async procedure.");
                    }
                    result = context->type_error;
                }
            } else {
                result =
                    context->type_error;
                (void)vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_INVALID_TYPE,
                    node->span,
                    node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    result);
            }
            break;

        case VITTE_AST_NODE_TRY_EXPR:
            if (node->child_count != 0u) {
                vitte_sema_type_id_t operand_type_id;
                const vitte_sema_type_t *operand_type;

                operand_type_id = vitte_sema_analyze_expression(
                    context,
                    node->children[0],
                    scope_id);
                operand_type = vitte_sema_type_private(
                    context,
                    operand_type_id);

                if (context->current_defer_node_id != VITTE_AST_INVALID_ID &&
                    context->current_defer_node_id != 0u) {
                    (void)vitte_sema_defer_control_error(context, node,
                        "Remove `?` from deferred work. Propagate the optional result before registering cleanup, then let the deferred action complete normally.");
                    result = context->type_error;
                    break;
                }

                if (operand_type_id == context->type_error) {
                    result = context->type_error;
                } else if (operand_type != NULL &&
                           operand_type->kind == VITTE_SEMA_TYPE_OPTIONAL) {
                    result = operand_type->element_type;
                } else {
                    (void)vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_UNARY_OPERAND,
                        node->span,
                        node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        operand_type_id);
                    result = context->type_error;
                }
            } else {
                result =
                    context->type_error;
            }
            break;

        case VITTE_AST_NODE_SIZEOF_EXPR:
            if (node->child_count == 1u) {
                vitte_sema_type_id_t queried_type;
                const vitte_ast_node_t *operand_node = vitte_sema_node(
                    context, node->children[0]);
                bool named_type_operand = false;

                /* The parser can distinguish `sizeof(pointer_type)` from
                 * `sizeof(pointer_value)` syntactically, but a bare nominal
                 * name is ambiguous until name resolution.  Resolve that
                 * case here so `sizeof(MyStruct)` queries a type instead of
                 * trying to evaluate the type name as a value. */
                if (operand_node != NULL &&
                    (operand_node->kind == VITTE_AST_NODE_IDENTIFIER ||
                     operand_node->kind == VITTE_AST_NODE_PATH)) {
                    vitte_symbol_id_t operand_symbol_id =
                        VITTE_SCOPE_INVALID_SYMBOL_ID;
                    const vitte_symbol_t *operand_symbol;

                    if (operand_node->kind == VITTE_AST_NODE_PATH &&
                        operand_node->child_count > 1u) {
                        (void)vitte_sema_resolve_qualified_import_type(
                            context,
                            operand_node,
                            scope_id,
                            &operand_symbol_id);
                    } else {
                        const char *operand_name;
                        size_t operand_name_length;
                        if (vitte_sema_token_name(
                                vitte_sema_node_token(
                                    context, operand_node),
                                &operand_name,
                                &operand_name_length)) {
                            operand_symbol_id =
                                vitte_scope_lookup_visible_n(
                                    &context->scopes,
                                    scope_id,
                                    scope_id,
                                    operand_name,
                                    operand_name_length);
                        }
                    }
                    operand_symbol = operand_symbol_id !=
                            VITTE_SCOPE_INVALID_SYMBOL_ID
                        ? vitte_scope_get_symbol(
                              &context->scopes, operand_symbol_id)
                        : NULL;
                    named_type_operand = operand_symbol != NULL &&
                        vitte_symbol_kind_is_type(operand_symbol->kind);
                }

                if ((node->flags &
                     VITTE_AST_FLAG_TYPE_QUERY_TYPE_OPERAND) != 0u ||
                    named_type_operand) {
                    queried_type = vitte_sema_analyze_type_node(
                        context,
                        node->children[0],
                        scope_id);
                } else {
                    queried_type = vitte_sema_analyze_expression(
                        context,
                        node->children[0],
                        scope_id);
                }
                if (queried_type != context->type_error &&
                    vitte_sema_report_incomplete_type_private(
                        context, operand_node, queried_type)) {
                    queried_type = context->type_error;
                }
                result = queried_type == context->type_error
                    ? context->type_error
                    : context->type_u64;
            } else {
                result = context->type_error;
            }
            break;

        case VITTE_AST_NODE_ALIGNOF_EXPR:
            if (node->child_count == 1u) {
                vitte_sema_type_id_t queried_type;

                queried_type = vitte_sema_analyze_type_node(
                    context,
                    node->children[0],
                    scope_id);
                const vitte_ast_node_t *operand_node = vitte_sema_node(
                    context, node->children[0]);
                if (queried_type != context->type_error &&
                    vitte_sema_report_incomplete_type_private(
                        context, operand_node, queried_type)) {
                    queried_type = context->type_error;
                }
                result = queried_type == context->type_error
                    ? context->type_error
                    : context->type_u64;
            } else {
                result = context->type_error;
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
        case VITTE_AST_NODE_OPTIONAL_TYPE:
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
        vitte_symbol_id_t existing_id;
        const vitte_symbol_t *existing_symbol;

        scope_error =
            vitte_scope_last_error(
                &context->scopes);

        if (scope_error ==
            VITTE_SCOPE_ERROR_DUPLICATE_SYMBOL) {
            const vitte_ast_node_t *name_node;

            existing_id = vitte_scope_lookup_local_n(
                &context->scopes,
                scope_id,
                name,
                name_length);
            existing_symbol = existing_id !=
                    VITTE_SCOPE_INVALID_SYMBOL_ID
                ? vitte_scope_get_symbol(
                      &context->scopes,
                      existing_id)
                : NULL;
            if (existing_symbol != NULL &&
                existing_symbol->kind == VITTE_SYMBOL_KIND_PROC &&
                kind == VITTE_SYMBOL_KIND_PROC &&
                existing_symbol->payload != NULL) {
                const vitte_ast_node_t *existing_declaration =
                    (const vitte_ast_node_t *)existing_symbol->payload;
                if ((existing_declaration->kind ==
                         VITTE_AST_NODE_PROC_DECL ||
                     existing_declaration->kind ==
                         VITTE_AST_NODE_EXTERN_PROC_DECL) &&
                    vitte_sema_procedure_overload_distinct_private(
                        context, existing_declaration, node)) {
                    info = vitte_sema_node_info_mut_private(
                        context, node->id);
                    if (info != NULL) {
                        info->symbol_id = existing_id;
                        info->scope_id = scope_id;
                        info->flags |= VITTE_SEMA_NODE_FLAG_DECLARATION;
                    }
                    context->stats.declarations =
                        vitte_sema_u64_add_sat(
                            context->stats.declarations, UINT64_C(1));
                    return existing_id;
                }
            }
            name_node = vitte_sema_find_identifier_child(
                context,
                node);
            if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                name_node != NULL ? name_node->span : node->span,
                name_node != NULL ? name_node->id : node->id,
                existing_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_append_symbol_origin_help(
                    context,
                    existing_symbol,
                    "The existing");
            }

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

typedef struct vitte_sema_pattern_binding_private {
    const vitte_token_t *token;
    uint32_t flags;
} vitte_sema_pattern_binding_private_t;

static size_t
vitte_sema_pattern_binding_count_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *pattern,
    uint32_t inherited_flags,
    size_t depth)
{
    size_t count = 0u;
    size_t index;
    uint32_t flags;

    if (context == NULL || pattern == NULL || depth >= 128u) return 0u;
    flags = inherited_flags | pattern->flags;
    if (pattern->kind == VITTE_AST_NODE_IDENTIFIER ||
        pattern->kind == VITTE_AST_NODE_PATH) {
        const vitte_token_t *token = vitte_sema_node_token(context, pattern);
        return token != NULL && !(token->length == 1u &&
            token->lexeme != NULL && token->lexeme[0] == '_') &&
            (pattern->kind != VITTE_AST_NODE_PATH ||
             pattern->child_count <= 1u) ? 1u : 0u;
    }
    if (pattern->kind != VITTE_AST_NODE_PATTERN ||
        (flags & VITTE_AST_FLAG_PATTERN_RANGE) != 0u) return 0u;
    if ((flags & VITTE_AST_FLAG_PATTERN_OR) != 0u) {
        return pattern->child_count == 0u ? 0u :
            vitte_sema_pattern_binding_count_private(
                context,
                vitte_sema_node(context, pattern->children[0]),
                flags,
                depth + 1u);
    }
    index = (flags & VITTE_AST_FLAG_PATTERN_HEAD) != 0u ? 1u : 0u;
    for (; index < pattern->child_count; ++index) {
        count += vitte_sema_pattern_binding_count_private(
            context,
            vitte_sema_node(context, pattern->children[index]),
            flags,
            depth + 1u);
    }
    return count;
}

static vitte_sema_pattern_binding_private_t
vitte_sema_pattern_binding_at_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *pattern,
    size_t wanted,
    uint32_t inherited_flags,
    size_t depth)
{
    vitte_sema_pattern_binding_private_t result;
    uint32_t flags;
    size_t index;
    memset(&result, 0, sizeof(result));
    if (context == NULL || pattern == NULL || depth >= 128u) return result;
    flags = inherited_flags | pattern->flags;
    if (pattern->kind == VITTE_AST_NODE_IDENTIFIER ||
        pattern->kind == VITTE_AST_NODE_PATH) {
        if (wanted == 0u &&
            (pattern->kind != VITTE_AST_NODE_PATH ||
             pattern->child_count <= 1u)) {
            const vitte_token_t *token = vitte_sema_node_token(context, pattern);
            if (token != NULL && !(token->length == 1u &&
                token->lexeme != NULL && token->lexeme[0] == '_')) {
                result.token = token;
                result.flags = flags & (VITTE_AST_FLAG_PATTERN_MUTABLE |
                                        VITTE_AST_FLAG_PATTERN_REFERENCE);
            }
        }
        return result;
    }
    if (pattern->kind != VITTE_AST_NODE_PATTERN ||
        (flags & VITTE_AST_FLAG_PATTERN_RANGE) != 0u) return result;
    if ((flags & VITTE_AST_FLAG_PATTERN_OR) != 0u) {
        return pattern->child_count == 0u ? result :
            vitte_sema_pattern_binding_at_private(
                context,
                vitte_sema_node(context, pattern->children[0]),
                wanted,
                flags,
                depth + 1u);
    }
    index = (flags & VITTE_AST_FLAG_PATTERN_HEAD) != 0u ? 1u : 0u;
    for (; index < pattern->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, pattern->children[index]);
        size_t child_count = vitte_sema_pattern_binding_count_private(
            context, child, flags, depth + 1u);
        if (wanted < child_count) {
            return vitte_sema_pattern_binding_at_private(
                context, child, wanted, flags, depth + 1u);
        }
        wanted -= child_count;
    }
    return result;
}

static bool
vitte_sema_pattern_alternatives_valid_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *pattern,
    uint32_t inherited_flags,
    size_t depth)
{
    uint32_t flags;
    size_t index;
    bool valid = true;

    if (context == NULL || pattern == NULL || depth >= 128u) return true;
    flags = inherited_flags | pattern->flags;
    if (pattern->kind == VITTE_AST_NODE_PATTERN &&
        (flags & VITTE_AST_FLAG_PATTERN_OR) != 0u) {
        const vitte_ast_node_t *first = pattern->child_count != 0u
            ? vitte_sema_node(context, pattern->children[0]) : NULL;
        const vitte_ast_node_t *offending_alternative = NULL;
        size_t first_count = vitte_sema_pattern_binding_count_private(
            context, first, flags, depth + 1u);
        if (pattern->child_count < 2u) valid = false;
        for (index = 1u; valid && index < pattern->child_count; ++index) {
            const vitte_ast_node_t *alternative = vitte_sema_node(
                context, pattern->children[index]);
            size_t alternative_count =
                vitte_sema_pattern_binding_count_private(
                    context, alternative, flags, depth + 1u);
            size_t binding_index;
            if (alternative_count != first_count) {
                valid = false;
                offending_alternative = alternative;
                break;
            }
            for (binding_index = 0u; binding_index < first_count;
                 ++binding_index) {
                vitte_sema_pattern_binding_private_t left =
                    vitte_sema_pattern_binding_at_private(
                        context, first, binding_index, flags, depth + 1u);
                vitte_sema_pattern_binding_private_t right =
                    vitte_sema_pattern_binding_at_private(
                        context, alternative, binding_index, flags,
                        depth + 1u);
                if (left.token == NULL || right.token == NULL ||
                    left.token->lexeme == NULL || right.token->lexeme == NULL ||
                    left.token->length != right.token->length ||
                    memcmp(left.token->lexeme, right.token->lexeme,
                           left.token->length) != 0 ||
                    left.flags != right.flags) {
                    valid = false;
                    offending_alternative = alternative;
                    break;
                }
            }
        }
        if (!valid && vitte_sema_add_diagnostic(
                context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_INVALID_PATTERN_BINDINGS,
                offending_alternative != NULL
                    ? offending_alternative->span : pattern->span,
                offending_alternative != NULL
                    ? offending_alternative->id : pattern->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
            vitte_sema_diagnostic_t *diagnostic =
                &context->diagnostics[context->diagnostic_count - 1u];
            (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                "Every `|` alternative must bind the same names in the same order with the same `mut` and `ref` modes.");
            if (first != NULL) {
                vitte_sema_set_last_related_location(
                    context, first, "first alternative establishes the bindings");
            }
            {
                vitte_sema_node_info_t *pattern_info =
                    vitte_sema_node_info_mut_private(context, pattern->id);
                if (pattern_info != NULL)
                    pattern_info->type_id = context->type_error;
            }
        }
    }
    for (index = 0u; index < pattern->child_count; ++index) {
        if (!vitte_sema_pattern_alternatives_valid_private(
                context,
                vitte_sema_node(context, pattern->children[index]),
                flags,
                depth + 1u)) valid = false;
    }
    return valid;
}

/* Register names introduced by a `for` pattern in the loop scope.  A loop
 * binding is not a declaration node in the parser tree, so it cannot go
 * through vitte_sema_register_declaration directly. */
static void
vitte_sema_bind_pattern(
    vitte_sema_t *context,
    vitte_ast_node_id_t pattern_id,
    vitte_scope_id_t scope_id,
    vitte_sema_type_id_t binding_type,
    bool inherited_mutable)
{
    const vitte_ast_node_t *pattern;
    const vitte_token_t *token;
    const char *name;
    size_t name_length;
    vitte_symbol_id_t symbol_id;
    vitte_sema_node_info_t *info;
    size_t index;
    uint32_t flags = inherited_mutable
        ? VITTE_SYMBOL_FLAG_MUTABLE
        : VITTE_SYMBOL_FLAG_NONE;

    if (context == NULL ||
        pattern_id == VITTE_AST_INVALID_ID ||
        scope_id == VITTE_SCOPE_INVALID_SCOPE_ID) {
        return;
    }

    pattern = vitte_sema_node(context, pattern_id);
    if (pattern == NULL) {
        return;
    }

    if (pattern->kind == VITTE_AST_NODE_PATTERN) {
        if ((pattern->flags & VITTE_AST_FLAG_PATTERN_OR) != 0u) {
            if (!vitte_sema_pattern_alternatives_valid_private(
                    context, pattern, 0u, 0u)) {
                return;
            }
            if (pattern->child_count != 0u) {
                vitte_sema_bind_pattern(
                    context, pattern->children[0], scope_id,
                    binding_type, inherited_mutable);
            }
            return;
        }
        if ((pattern->flags & VITTE_AST_FLAG_PATTERN_TUPLE) != 0u) {
            if (vitte_sema_add_diagnostic(context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE,
                    pattern->span, pattern->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    binding_type, VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_diagnostic_t *diagnostic =
                    &context->diagnostics[context->diagnostic_count - 1u];
                (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                    "Tuple patterns require a tuple value type, which Vitte does not define. Use a form or a pick variant with payload fields.");
            }
            info = vitte_sema_node_info_mut_private(context, pattern->id);
            if (info != NULL) info->type_id = context->type_error;
            for (index = 0u; index < pattern->child_count; ++index) {
                vitte_sema_bind_pattern(
                    context, pattern->children[index], scope_id,
                    context->type_error, inherited_mutable);
            }
            return;
        }
        if ((pattern->flags & VITTE_AST_FLAG_PATTERN_ARRAY) != 0u) {
            const vitte_sema_type_t *array_type =
                vitte_sema_type_private(context, binding_type);
            if (array_type == NULL ||
                (array_type->kind != VITTE_SEMA_TYPE_ARRAY &&
                 array_type->kind != VITTE_SEMA_TYPE_SLICE) ||
                (array_type->kind == VITTE_SEMA_TYPE_ARRAY &&
                 array_type->array_length != 0u &&
                 array_type->array_length != pattern->child_count)) {
                if (vitte_sema_add_diagnostic(context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE,
                        pattern->span, pattern->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        binding_type, VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_diagnostic_t *diagnostic =
                        &context->diagnostics[context->diagnostic_count - 1u];
                    if (array_type != NULL &&
                        array_type->kind == VITTE_SEMA_TYPE_ARRAY &&
                        array_type->array_length != 0u) {
                        (void)snprintf(diagnostic->help,
                            sizeof(diagnostic->help),
                            "This array pattern has %zu element pattern(s), but the matched array has %zu elements.",
                            pattern->child_count, array_type->array_length);
                    } else {
                        (void)snprintf(diagnostic->help,
                            sizeof(diagnostic->help),
                            "Use an array pattern for an array or slice value, and match its element shape.");
                    }
                }
                info = vitte_sema_node_info_mut_private(context, pattern->id);
                if (info != NULL) info->type_id = context->type_error;
                for (index = 0u; index < pattern->child_count; ++index) {
                    vitte_sema_bind_pattern(
                        context, pattern->children[index], scope_id,
                        context->type_error, inherited_mutable);
                }
                return;
            }
            for (index = 0u; index < pattern->child_count; ++index) {
                vitte_sema_bind_pattern(
                    context, pattern->children[index], scope_id,
                    array_type->element_type, inherited_mutable);
            }
            return;
        }
        if ((pattern->flags & VITTE_AST_FLAG_PATTERN_MUTABLE) != 0u) {
            flags |= VITTE_SYMBOL_FLAG_MUTABLE;
        }
        if ((pattern->flags & VITTE_AST_FLAG_PATTERN_HEAD) != 0u &&
            pattern->child_count != 0u) {
            const vitte_ast_node_t *head =
                vitte_sema_node(context, pattern->children[0]);

            if ((pattern->flags & VITTE_AST_FLAG_PATTERN_STRUCT) != 0u) {
                vitte_sema_type_id_t aggregate_type =
                    vitte_sema_analyze_type_node(context, head->id, scope_id);
                const vitte_sema_type_t *aggregate =
                    vitte_sema_type_private(context, aggregate_type);
                const vitte_symbol_t *aggregate_symbol = aggregate != NULL &&
                        aggregate->kind == VITTE_SEMA_TYPE_NAMED
                    ? vitte_scope_get_symbol(
                          &context->scopes, aggregate->symbol_id)
                    : NULL;
                const vitte_ast_node_t *form_declaration = aggregate_symbol != NULL
                    ? (const vitte_ast_node_t *)aggregate_symbol->payload : NULL;
                vitte_scope_id_t form_scope = VITTE_SCOPE_INVALID_SCOPE_ID;
                size_t scope_index;

                if (binding_type != context->type_error &&
                    aggregate_type != context->type_error &&
                    !vitte_sema_type_identical_private(
                        context, binding_type, aggregate_type) &&
                    vitte_sema_add_diagnostic(context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE,
                        pattern->span, pattern->id,
                        aggregate_symbol != NULL ? aggregate_symbol->id
                            : VITTE_SCOPE_INVALID_SYMBOL_ID,
                        binding_type, aggregate_type)) {
                    vitte_sema_diagnostic_t *diagnostic =
                        &context->diagnostics[context->diagnostic_count - 1u];
                    (void)snprintf(diagnostic->help,
                        sizeof(diagnostic->help),
                        "Use a struct pattern whose type matches the value being matched.");
                    if (form_declaration != NULL) {
                        vitte_sema_set_last_related_location(
                            context, form_declaration,
                            "this form defines the pattern type");
                    }
                    {
                        vitte_sema_node_info_t *pattern_info =
                            vitte_sema_node_info_mut_private(
                                context, pattern->id);
                        if (pattern_info != NULL)
                            pattern_info->type_id = context->type_error;
                    }
                }
                if (form_declaration == NULL ||
                    form_declaration->kind != VITTE_AST_NODE_FORM_DECL) {
                    return;
                }
                for (scope_index = 0u;
                     scope_index < context->scopes.scope_count;
                     ++scope_index) {
                    const vitte_scope_entry_t *candidate =
                        &context->scopes.scopes[scope_index];
                    if (candidate->kind == VITTE_SCOPE_KIND_FORM &&
                        candidate->payload == (void *)form_declaration) {
                        form_scope = candidate->id;
                        break;
                    }
                }
                if (form_scope == VITTE_SCOPE_INVALID_SCOPE_ID) return;
                for (index = 1u; index < pattern->child_count; ++index) {
                    const vitte_ast_node_t *field = vitte_sema_node(
                        context, pattern->children[index]);
                    const vitte_ast_node_t *field_name = field;
                    const vitte_ast_node_t *field_binding = field;
                    const vitte_token_t *field_token;
                    vitte_symbol_id_t field_symbol_id;
                    const vitte_symbol_t *field_symbol;
                    const vitte_ast_node_t *field_declaration;
                    vitte_sema_type_id_t field_type = context->type_error;
                    size_t previous_index;

                    if (field == NULL) continue;
                    if (field->kind == VITTE_AST_NODE_PATTERN &&
                        (field->flags & VITTE_AST_FLAG_PATTERN_HEAD) != 0u &&
                        field->child_count >= 2u) {
                        field_name = vitte_sema_node(
                            context, field->children[0]);
                        field_binding = vitte_sema_node(
                            context, field->children[1]);
                    }
                    field_token = vitte_sema_node_token(context, field_name);
                    for (previous_index = 1u;
                         previous_index < index;
                         ++previous_index) {
                        const vitte_ast_node_t *previous_field =
                            vitte_sema_node(context,
                                pattern->children[previous_index]);
                        const vitte_ast_node_t *previous_name = previous_field;
                        const vitte_token_t *previous_token;
                        if (previous_field != NULL &&
                            previous_field->kind == VITTE_AST_NODE_PATTERN &&
                            (previous_field->flags &
                             VITTE_AST_FLAG_PATTERN_HEAD) != 0u &&
                            previous_field->child_count >= 2u) {
                            previous_name = vitte_sema_node(context,
                                previous_field->children[0]);
                        }
                        previous_token = vitte_sema_node_token(
                            context, previous_name);
                        if (field_token != NULL && previous_token != NULL &&
                            field_token->length == previous_token->length &&
                            memcmp(field_token->lexeme, previous_token->lexeme,
                                   field_token->length) == 0) {
                            (void)vitte_sema_add_diagnostic(context,
                                VITTE_SEMA_DIAGNOSTIC_ERROR,
                                VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE,
                                field_name != NULL ? field_name->span : field->span,
                                field_name != NULL ? field_name->id : field->id,
                                aggregate_symbol != NULL ? aggregate_symbol->id
                                    : VITTE_SCOPE_INVALID_SYMBOL_ID,
                                VITTE_SEMA_INVALID_TYPE_ID, aggregate_type);
                            if (context->diagnostic_count != 0u) {
                                vitte_sema_diagnostic_t *diagnostic =
                                    &context->diagnostics[context->diagnostic_count - 1u];
                                (void)snprintf(diagnostic->help,
                                    sizeof(diagnostic->help),
                                    "Field `%.*s` appears more than once in this struct pattern. Keep one field entry.",
                                    (int)field_token->length, field_token->lexeme);
                                vitte_sema_set_last_related_location(
                                    context, previous_name,
                                    "first field entry appears here");
                            }
                            {
                                vitte_sema_node_info_t *pattern_info =
                                    vitte_sema_node_info_mut_private(
                                        context, pattern->id);
                                if (pattern_info != NULL)
                                    pattern_info->type_id = context->type_error;
                            }
                            return;
                        }
                    }
                    field_symbol_id = field_token != NULL
                        ? vitte_scope_lookup_local_n(
                              &context->scopes, form_scope,
                              field_token->lexeme, field_token->length)
                        : VITTE_SCOPE_INVALID_SYMBOL_ID;
                    field_symbol = field_symbol_id !=
                            VITTE_SCOPE_INVALID_SYMBOL_ID
                        ? vitte_scope_get_symbol(
                              &context->scopes, field_symbol_id) : NULL;
                    field_declaration = field_symbol != NULL
                        ? (const vitte_ast_node_t *)field_symbol->payload
                        : NULL;
                    if (field_declaration == NULL ||
                        field_declaration->kind != VITTE_AST_NODE_FIELD_DECL) {
                        (void)vitte_sema_add_diagnostic(context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_UNKNOWN_MEMBER,
                            field_name != NULL ? field_name->span : field->span,
                            field_name != NULL ? field_name->id : field->id,
                            aggregate_symbol != NULL ? aggregate_symbol->id
                                : VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID, aggregate_type);
                        if (context->diagnostic_count != 0u) {
                            vitte_sema_set_last_related_location(
                                context,
                                form_declaration,
                                "available fields are declared here");
                        }
                    } else if (field_declaration->child_count > 1u) {
                        field_type = vitte_sema_analyze_type_node(
                            context, field_declaration->children[1], form_scope);
                        field_type =
                            vitte_sema_substitute_generic_type_private(
                                context, field_type, aggregate_type);
                    }
                    vitte_sema_bind_pattern(
                        context,
                        field_binding != NULL ? field_binding->id : field->id,
                        scope_id,
                        field_type,
                        inherited_mutable ||
                            (pattern->flags &
                             VITTE_AST_FLAG_PATTERN_MUTABLE) != 0u);
                }
                return;
            }
            vitte_sema_pick_variant_resolution_t resolution;
            size_t payload_count;
            size_t pattern_payload_count;

            if (head == NULL) {
                return;
            }
            if (!vitte_sema_resolve_pick_variant_path(
                    context, head, scope_id, &resolution)) {
                (void)vitte_sema_analyze_expression(
                    context, head->id, scope_id);
                {
                    vitte_sema_node_info_t *pattern_info =
                        vitte_sema_node_info_mut_private(
                            context, pattern->id);
                    if (pattern_info != NULL)
                        pattern_info->type_id = context->type_error;
                }
                return;
            }
            payload_count =
                resolution.variant_declaration->child_count - 1u;
            pattern_payload_count = pattern->child_count - 1u;
            if (binding_type != context->type_error &&
                !vitte_sema_type_identical_private(
                    context, binding_type, resolution.pick_type_id) &&
                vitte_sema_add_diagnostic(context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE,
                    pattern->span, pattern->id,
                    resolution.pick_symbol_id,
                    binding_type, resolution.pick_type_id)) {
                vitte_sema_diagnostic_t *diagnostic =
                    &context->diagnostics[context->diagnostic_count - 1u];
                (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                    "Use a variant declared by the matched pick type.");
                vitte_sema_set_last_related_location(
                    context, resolution.pick_declaration,
                    "this pick declaration owns the variant");
                {
                    vitte_sema_node_info_t *pattern_info =
                        vitte_sema_node_info_mut_private(
                            context, pattern->id);
                    if (pattern_info != NULL)
                        pattern_info->type_id = context->type_error;
                }
            }
            if (payload_count != pattern_payload_count) {
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE,
                        pattern->span,
                        pattern->id,
                        resolution.pick_symbol_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_diagnostic_t *diagnostic =
                        &context->diagnostics[context->diagnostic_count - 1u];
                    (void)snprintf(diagnostic->help,
                        sizeof(diagnostic->help),
                        "This variant expects %zu payload pattern%s but this arm supplies %zu. Match the declared constructor shape.",
                        payload_count, payload_count == 1u ? "" : "s",
                        pattern_payload_count);
                    vitte_sema_set_last_related_location(
                        context, resolution.variant_declaration,
                        "variant payloads are declared here");
                    {
                        vitte_sema_node_info_t *pattern_info =
                            vitte_sema_node_info_mut_private(
                                context, pattern->id);
                        if (pattern_info != NULL)
                            pattern_info->type_id = context->type_error;
                    }
                }
                for (index = 0u;
                     index < pattern_payload_count;
                     ++index) {
                    vitte_sema_type_id_t payload_type = context->type_error;
                    if (index < payload_count) {
                        payload_type = vitte_sema_analyze_type_node(
                            context,
                            resolution.variant_declaration->children[index + 1u],
                            resolution.pick_scope_id);
                        payload_type =
                            vitte_sema_substitute_generic_type_private(
                                context, payload_type,
                                resolution.pick_type_id);
                    }
                    vitte_sema_bind_pattern(
                        context, pattern->children[index + 1u],
                        scope_id, payload_type, inherited_mutable ||
                            (pattern->flags &
                             VITTE_AST_FLAG_PATTERN_MUTABLE) != 0u);
                }
                return;
            }
            for (index = 0u; index < pattern_payload_count; ++index) {
                vitte_sema_type_id_t payload_type =
                    vitte_sema_analyze_type_node(
                        context,
                        resolution.variant_declaration->children[index + 1u],
                        resolution.pick_scope_id);
                payload_type = vitte_sema_substitute_generic_type_private(
                    context, payload_type, resolution.pick_type_id);
                vitte_sema_bind_pattern(
                    context,
                    pattern->children[index + 1u],
                    scope_id,
                    payload_type,
                    inherited_mutable ||
                        (pattern->flags &
                         VITTE_AST_FLAG_PATTERN_MUTABLE) != 0u);
            }
            return;
        }
        for (index = 0u; index < pattern->child_count; ++index) {
            if (index == 0u &&
                (pattern->flags & VITTE_AST_FLAG_PATTERN_HEAD) != 0u) {
                continue;
            }
            vitte_sema_bind_pattern(
                context,
                pattern->children[index],
                scope_id,
                binding_type,
                inherited_mutable ||
                    (pattern->flags & VITTE_AST_FLAG_PATTERN_MUTABLE) != 0u);
        }
        return;
    }

    if (pattern->kind == VITTE_AST_NODE_PATH) {
        if (pattern->child_count != 1u) {
            (void)vitte_sema_analyze_expression(
                context, pattern->id, scope_id);
            return;
        }
        pattern = vitte_sema_node(context, pattern->children[0]);
    }

    if (pattern == NULL || pattern->kind != VITTE_AST_NODE_IDENTIFIER) {
        return;
    }

    token = vitte_sema_node_token(context, pattern);
    if (!vitte_sema_token_name(token, &name, &name_length) ||
        (name_length == 1u && name[0] == '_')) {
        return;
    }

    symbol_id = vitte_scope_declare(
        &context->scopes,
        scope_id,
        name,
        name_length,
        VITTE_SYMBOL_KIND_LOCAL,
        VITTE_SYMBOL_VISIBILITY_PRIVATE,
        flags,
        vitte_sema_scope_span_from_parser(pattern->span),
        (void *)pattern);
    if (symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        vitte_symbol_id_t existing_id = vitte_scope_lookup_local_n(
            &context->scopes,
            scope_id,
            name,
            name_length);
        if (existing_id != VITTE_SCOPE_INVALID_SYMBOL_ID) {
            const vitte_symbol_t *existing_symbol =
                vitte_scope_get_symbol(&context->scopes, existing_id);
            if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_DUPLICATE_BINDING,
                pattern->span,
                pattern->id,
                existing_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
                const vitte_ast_node_t *previous_pattern =
                    existing_symbol != NULL && existing_symbol->payload != NULL
                        ? (const vitte_ast_node_t *)existing_symbol->payload
                        : NULL;
                if (previous_pattern != NULL) {
                    vitte_sema_set_last_related_location(
                        context, previous_pattern,
                        "first binding declared here");
                }
            }
            info = vitte_sema_node_info_mut_private(context, pattern->id);
            if (info != NULL) info->type_id = context->type_error;
            context->stats.duplicate_declarations =
                vitte_sema_u64_add_sat(
                    context->stats.duplicate_declarations,
                    UINT64_C(1));
        }
        return;
    }

    info = vitte_sema_node_info_mut_private(context, pattern->id);
    if (info != NULL) {
        info->symbol_id = symbol_id;
        info->scope_id = scope_id;
        info->type_id = binding_type;
        info->flags |= VITTE_SEMA_NODE_FLAG_DECLARATION;
        if ((flags & VITTE_SYMBOL_FLAG_MUTABLE) != 0u) {
            info->flags |= VITTE_SEMA_NODE_FLAG_MUTABLE;
        }
    }
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
    size_t name_length,
    vitte_symbol_id_t *private_symbol_id)
{
    size_t node_index;

    if (context == NULL ||
        context->parser == NULL ||
        name == NULL ||
        name_length == 0u) {
        if (private_symbol_id != NULL) {
            *private_symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
        }
        return false;
    }

    if (private_symbol_id != NULL) {
        *private_symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
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

        import_info = vitte_sema_node_info_private(
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
            if (private_symbol_id != NULL) {
                *private_symbol_id = symbol_id;
            }
            return true;
        }
    }

    return false;
}

static bool
vitte_sema_scope_module_path_text(
    vitte_sema_t *context,
    vitte_scope_id_t module_scope,
    char *buffer,
    size_t capacity)
{
    vitte_scope_id_t chain[64];
    size_t chain_count = 0u;
    size_t used;
    size_t chain_index;

    if (context == NULL || buffer == NULL || capacity == 0u) {
        return false;
    }

    buffer[0] = '\0';
    while (module_scope != 0u && chain_count < sizeof(chain) / sizeof(chain[0])) {
        const vitte_scope_entry_t *entry =
            vitte_scope_get_scope(&context->scopes, module_scope);
        if (entry == NULL) {
            return false;
        }
        chain[chain_count++] = module_scope;
        module_scope = entry->parent_id;
    }
    if (module_scope != 0u || chain_count == 0u) {
        return false;
    }

    used = 0u;
    for (chain_index = chain_count; chain_index != 0u; --chain_index) {
        const vitte_scope_entry_t *entry = vitte_scope_get_scope(
            &context->scopes, chain[chain_index - 1u]);
        const vitte_ast_node_t *declaration = entry != NULL
            ? (const vitte_ast_node_t *)entry->payload
            : NULL;
        const vitte_ast_node_t *path = declaration != NULL &&
                declaration->kind == VITTE_AST_NODE_SPACE_DECL &&
                declaration->child_count != 0u
            ? vitte_sema_node(context, declaration->children[0])
            : NULL;
        size_t segment_index;
        if (path == NULL || path->kind != VITTE_AST_NODE_PATH) {
            continue;
        }
        for (segment_index = 0u;
             segment_index < path->child_count;
             ++segment_index) {
            const vitte_ast_node_t *segment =
                vitte_sema_node(context, path->children[segment_index]);
            const char *name;
            size_t name_length;
            size_t separator_length = used == 0u ? 0u : 2u;
            if (segment == NULL ||
                !vitte_sema_token_name(
                    vitte_sema_node_token(context, segment),
                    &name,
                    &name_length) ||
                separator_length > capacity - used - 1u ||
                name_length > capacity - used - separator_length - 1u) {
                buffer[0] = '\0';
                return false;
            }
            if (separator_length != 0u) {
                memcpy(buffer + used, "::", separator_length);
                used += separator_length;
            }
            memcpy(buffer + used, name, name_length);
            used += name_length;
            buffer[used] = '\0';
        }
    }

    return used != 0u;
}

static bool
vitte_sema_help_module_member_import(
    vitte_sema_t *context,
    vitte_scope_id_t requester_scope,
    const char *name,
    size_t name_length)
{
    char modules[2][128];
    char aliases[2][96];
    vitte_symbol_id_t symbols[2];
    size_t candidate_count;
    size_t node_index;
    vitte_sema_diagnostic_t *diagnostic;
    int shown_name_length;

    if (context == NULL || context->parser == NULL || name == NULL ||
        name_length == 0u || context->diagnostic_count == 0u) {
        return false;
    }

    diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
    if (diagnostic->code != VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME) {
        return false;
    }
    candidate_count = 0u;
    shown_name_length = name_length > 40u ? 40 : (int)name_length;

    for (node_index = 0u;
         node_index < context->parser->node_count;
         ++node_index) {
        const vitte_ast_node_t *use_node;
        const vitte_sema_node_info_t *use_info;
        const vitte_ast_node_t *module_path;
        const vitte_ast_node_t *alias_node;
        const char *alias_name;
        size_t alias_length;
        vitte_scope_id_t module_scope;
        vitte_symbol_id_t symbol_id;
        const vitte_symbol_t *symbol;
        size_t candidate_index;

        use_node = vitte_sema_node(
            context,
            (vitte_ast_node_id_t)(node_index + 1u));
        if (use_node == NULL ||
            use_node->kind != VITTE_AST_NODE_USE_DECL ||
            (use_node->flags & VITTE_AST_FLAG_GLOB_IMPORT) != 0u ||
            (use_node->child_count != 1u && use_node->child_count != 2u) ||
            (use_node->child_count == 2u &&
             (vitte_sema_node(
                  context,
                  use_node->children[1]) == NULL ||
              vitte_sema_node(
                  context,
                  use_node->children[1])->kind !=
                  VITTE_AST_NODE_IDENTIFIER))) {
            continue;
        }

        use_info = vitte_sema_node_info_private(context, use_node->id);
        if (use_info == NULL ||
            !vitte_scope_is_ancestor(
                &context->scopes,
                use_info->scope_id,
                requester_scope)) {
            continue;
        }

        module_path = vitte_sema_node(context, use_node->children[0]);
        if (module_path == NULL ||
            !vitte_sema_resolve_import_module(
                context,
                module_path,
                use_info->scope_id,
                &module_scope)) {
            continue;
        }

        symbol_id = vitte_scope_lookup_local_n(
            &context->scopes,
            module_scope,
            name,
            name_length);
        symbol = symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID
            ? vitte_scope_get_symbol(&context->scopes, symbol_id)
            : NULL;
        if (!vitte_symbol_is_public(symbol)) {
            continue;
        }

        alias_node = use_node->child_count == 2u
            ? vitte_sema_node(context, use_node->children[1])
            : module_path->kind == VITTE_AST_NODE_PATH &&
                    module_path->child_count != 0u
                ? vitte_sema_node(
                      context,
                      module_path->children[module_path->child_count - 1u])
                : NULL;
        if (alias_node == NULL ||
            !vitte_sema_token_name(
                vitte_sema_node_token(context, alias_node),
                &alias_name,
                &alias_length)) {
            continue;
        }

        for (candidate_index = 0u;
             candidate_index < candidate_count && candidate_index < 2u;
             ++candidate_index) {
            if (symbols[candidate_index] == symbol_id) {
                break;
            }
        }
        if (candidate_index < candidate_count) {
            continue;
        }
        if (candidate_count < 2u) {
            size_t copy_length;

            symbols[candidate_count] = symbol_id;
            if (!vitte_sema_scope_module_path_text(
                    context,
                    module_scope,
                    modules[candidate_count],
                    sizeof(modules[candidate_count]))) {
                modules[candidate_count][0] = '\0';
            }
            copy_length = alias_length < sizeof(aliases[candidate_count]) - 1u
                ? alias_length
                : sizeof(aliases[candidate_count]) - 1u;
            memcpy(aliases[candidate_count], alias_name, copy_length);
            aliases[candidate_count][copy_length] = '\0';
        }
        ++candidate_count;
    }

    if (candidate_count == 0u) {
        return false;
    }
    if (candidate_count == 1u && modules[0][0] != '\0') {
        (void)snprintf(
            diagnostic->help,
            sizeof(diagnostic->help),
            "`%.*s` is in module `%.*s`; importing it binds module `%.*s`, not its members. Use `%.*s::%.*s` or `use %.*s::{%.*s};` (add `as alias` to rename the item).",
            shown_name_length,
            name,
            64,
            modules[0],
            48,
            aliases[0],
            48,
            aliases[0],
            shown_name_length,
            name,
            64,
            modules[0],
            shown_name_length,
            name);
    } else if (candidate_count >= 2u && modules[0][0] != '\0' &&
               modules[1][0] != '\0') {
        (void)snprintf(
            diagnostic->help,
            sizeof(diagnostic->help),
            "`%.*s` is exported by both `%s` and `%s`. Import one through a group with `as alias` to select it explicitly.",
            shown_name_length,
            name,
            modules[0],
            modules[1]);
    } else {
        return false;
    }

    return diagnostic->help[0] != '\0';
}

static bool
vitte_sema_add_import_ambiguity(
    vitte_sema_t *context,
    const vitte_ast_node_t *current_declaration,
    vitte_scope_id_t current_module_scope,
    const vitte_symbol_t *existing_import)
{
    const vitte_ast_node_t *existing_declaration;
    const vitte_ast_node_t *current_name_node;
    const vitte_sema_node_info_t *current_info;
    const vitte_sema_node_info_t *existing_info;
    const char *name;
    size_t name_length;
    char current_module[96];
    char existing_module[96];
    vitte_symbol_id_t existing_declaration_id;

    if (context == NULL || current_declaration == NULL ||
        existing_import == NULL || existing_import->payload == NULL ||
        (existing_import->flags & VITTE_SYMBOL_FLAG_IMPORTED) == 0u) {
        return false;
    }

    existing_declaration = (const vitte_ast_node_t *)existing_import->payload;
    current_name_node = vitte_sema_find_identifier_child(
        context,
        current_declaration);
    if (current_name_node == NULL ||
        !vitte_sema_declaration_name(
            context,
            current_declaration,
            &name,
            &name_length)) {
        return false;
    }

    current_info = vitte_sema_node_info_private(
        context,
        current_declaration->id);
    if (current_info != NULL &&
        current_info->scope_id != VITTE_SCOPE_INVALID_SCOPE_ID) {
        current_module_scope = current_info->scope_id;
    }
    existing_info = vitte_sema_node_info_private(
        context,
        existing_declaration->id);
    if (existing_info == NULL ||
        !vitte_sema_scope_module_path_text(
            context,
            current_module_scope,
            current_module,
            sizeof(current_module)) ||
        !vitte_sema_scope_module_path_text(
            context,
            existing_info->scope_id,
            existing_module,
            sizeof(existing_module))) {
        return false;
    }

    existing_declaration_id = vitte_scope_lookup_local_n(
        &context->scopes,
        existing_info->scope_id,
        existing_import->name,
        existing_import->name_length);
    if (existing_declaration_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        existing_declaration_id = existing_import->id;
    }

    if (!vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_NAME,
            current_name_node->span,
            current_name_node->id,
            existing_declaration_id,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID)) {
        return true;
    }

    (void)snprintf(
        context->diagnostics[context->diagnostic_count - 1u].help,
        sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
        "Ambiguous imported name `%.*s` has candidates in `%.*s` and `%.*s`. Alias one grouped item, e.g. `use %.*s::{%.*s as local};`, or qualify it through its module.",
        (int)(name_length > 32u ? 32u : name_length),
        name,
        64,
        current_module,
        64,
        existing_module,
        64,
        current_module,
        (int)(name_length > 32u ? 32u : name_length),
        name);
    return true;
}

static bool
vitte_sema_resolve_import_item(
    vitte_sema_t *context,
    const vitte_ast_node_t *item,
    vitte_scope_id_t module_scope,
    vitte_scope_id_t requester_scope,
    vitte_symbol_id_t *resolved_symbol,
    vitte_symbol_id_t *private_symbol_id)
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
    if (private_symbol_id != NULL) {
        *private_symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
    }

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
        if (*resolved_symbol == VITTE_SCOPE_INVALID_SYMBOL_ID &&
            private_symbol_id != NULL) {
            vitte_symbol_id_t hidden_id;
            const vitte_symbol_t *hidden_symbol;

            hidden_id = vitte_scope_lookup_n(
                &context->scopes,
                current_scope,
                name,
                name_length);
            hidden_symbol = hidden_id != VITTE_SCOPE_INVALID_SYMBOL_ID
                ? vitte_scope_get_symbol(&context->scopes, hidden_id)
                : NULL;
            if (vitte_symbol_is_private(hidden_symbol)) {
                *private_symbol_id = hidden_id;
            }
        }
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
            if (private_symbol_id != NULL) {
                vitte_symbol_id_t hidden_id;
                const vitte_symbol_t *hidden_symbol;

                hidden_id = vitte_scope_lookup_n(
                    &context->scopes,
                    current_scope,
                    name,
                    name_length);
                hidden_symbol = hidden_id != VITTE_SCOPE_INVALID_SYMBOL_ID
                    ? vitte_scope_get_symbol(&context->scopes, hidden_id)
                    : NULL;
                if (vitte_symbol_is_private(hidden_symbol)) {
                    *private_symbol_id = hidden_id;
                }
            }
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
        const vitte_ast_node_t *module_path;
        const vitte_ast_node_t *alias;
        const char *candidate_name;
        size_t candidate_name_length;
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
            use_node->child_count == 0u ||
            use_node->child_count > 2u) {
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

        module_path =
            vitte_sema_node(
                context,
                use_node->children[0]);
        alias = NULL;
        if (use_node->child_count == 2u) {
            alias =
                vitte_sema_node(
                    context,
                    use_node->children[1]);
        } else if (module_path != NULL &&
                   module_path->kind == VITTE_AST_NODE_PATH &&
                   module_path->child_count != 0u) {
            /* A bare module import uses its final path segment as alias. */
            alias =
                vitte_sema_node(
                    context,
                    module_path->children[
                        module_path->child_count - 1u]);
        }
        if (alias == NULL ||
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

static void
vitte_sema_help_import_alias_conflict(
    vitte_sema_t *context,
    const char *alias,
    size_t alias_length,
    bool module_alias,
    bool explicit_alias,
    const vitte_symbol_t *existing_symbol)
{
    vitte_sema_diagnostic_t *diagnostic;
    int shown_length;

    if (context == NULL || alias == NULL || alias_length == 0u ||
        context->diagnostic_count == 0u) {
        return;
    }
    diagnostic = &context->diagnostics[context->diagnostic_count - 1u];
    if (diagnostic->code !=
        VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION) {
        return;
    }
    shown_length = alias_length > 128u ? 128 : (int)alias_length;

    if (module_alias && explicit_alias) {
        (void)snprintf(
            diagnostic->help,
            sizeof(diagnostic->help),
            "Module alias `%.*s` conflicts with an existing declaration. Change this alias after `as`.",
            shown_length,
            alias);
    } else if (module_alias) {
        (void)snprintf(
            diagnostic->help,
            sizeof(diagnostic->help),
            "Default module alias `%.*s` conflicts with an existing declaration. Add `as <new_name>` to change this alias.",
            shown_length,
            alias);
    } else if (explicit_alias) {
        (void)snprintf(
            diagnostic->help,
            sizeof(diagnostic->help),
            "Imported symbol alias `%.*s` conflicts with an existing declaration. Change this alias after `as`.",
            shown_length,
            alias);
    } else {
        (void)snprintf(
            diagnostic->help,
            sizeof(diagnostic->help),
            "Imported name `%.*s` conflicts with an existing declaration. Add `as <new_name>` or change this import's alias.",
            shown_length,
            alias);
    }

    vitte_sema_append_symbol_origin_help(
        context,
        existing_symbol,
        "The existing");
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
    vitte_sema_node_info_t *import_info;
    size_t item_index;
    bool explicit_module_alias;

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

    import_info = vitte_sema_node_info_mut_private(
        context,
        node->id);
    if (import_info != NULL) {
        /* Imports are visited once in the translation-unit prepass and may
         * also be reached through a containing namespace body.  Keep the
         * operation idempotent for that same AST declaration and scope;
         * distinct repeated `use` declarations have distinct node IDs and
         * still receive the duplicate-import diagnostic below. */
        if ((import_info->flags & VITTE_SEMA_NODE_FLAG_RESOLVED) != 0u &&
            import_info->scope_id == scope_id) {
            return context->type_void;
        }
        import_info->scope_id = scope_id;
        import_info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
    }

    module_alias = NULL;
    explicit_module_alias = false;
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
            explicit_module_alias = true;
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
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_DUPLICATE_IMPORT,
                        module_alias->span,
                        module_alias->id,
                        existing_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    (void)snprintf(
                        context->diagnostics[
                            context->diagnostic_count - 1u].help,
                        sizeof(context->diagnostics[
                            context->diagnostic_count - 1u].help),
                        "Module `%.*s` is already imported as `%.*s`; remove this repeated use declaration.",
                        (int)(alias_length > 96u ? 96u : alias_length),
                        alias_name,
                        (int)(alias_length > 96u ? 96u : alias_length),
                        alias_name);
                }
                return context->type_void;
            }

            if (vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                    module_alias->span,
                    module_alias->id,
                    existing_id,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_help_import_alias_conflict(
                    context,
                    alias_name,
                    alias_length,
                    true,
                    explicit_module_alias,
                    existing_symbol);
            }
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
                    if (existing_symbol != NULL &&
                        vitte_sema_add_import_ambiguity(
                            context,
                            target_declaration,
                            module_scope,
                            existing_symbol)) {
                        continue;
                    }
                    if (vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                            node->span,
                            node->id,
                            existing_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID) &&
                        existing_symbol != NULL) {
                        vitte_sema_diagnostic_t *diagnostic =
                            &context->diagnostics[
                                context->diagnostic_count - 1u];
                        int shown_length = target_symbol->name_length > 128u
                            ? 128
                            : (int)target_symbol->name_length;
                        (void)snprintf(
                            diagnostic->help,
                            sizeof(diagnostic->help),
                            "Glob import brings in `%.*s`, which conflicts with an existing declaration. Replace `*` with an explicit import and give it a different alias.",
                            shown_length,
                            target_symbol->name);
                        vitte_sema_append_symbol_origin_help(
                            context,
                            existing_symbol,
                            "The existing");
                    }
                }
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
        vitte_symbol_id_t private_symbol_id;
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
                &target_id,
                &private_symbol_id)) {
            if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                private_symbol_id != VITTE_SCOPE_INVALID_SYMBOL_ID
                    ? VITTE_SEMA_DIAGNOSTIC_PRIVATE_SYMBOL
                    : VITTE_SEMA_DIAGNOSTIC_UNRESOLVED_NAME,
                item->span,
                item->id,
                private_symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
                if (private_symbol_id !=
                    VITTE_SCOPE_INVALID_SYMBOL_ID) {
                    vitte_sema_help_private_symbol(
                        context,
                        vitte_scope_get_symbol(
                            &context->scopes,
                            private_symbol_id));
                }
            }
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
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_DUPLICATE_IMPORT,
                        alias_node != NULL ? alias_node->span : item->span,
                        alias_node != NULL ? alias_node->id : item->id,
                        existing_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    (void)snprintf(
                        context->diagnostics[
                            context->diagnostic_count - 1u].help,
                        sizeof(context->diagnostics[
                            context->diagnostic_count - 1u].help),
                        "`%.*s` is imported more than once under the same local name; remove the repeated item or give one import a distinct alias.",
                        (int)(alias_length > 96u ? 96u : alias_length),
                        alias);
                }
                continue;
            }
            if (alias_node == NULL &&
                existing_symbol != NULL &&
                (existing_symbol->flags &
                 VITTE_SYMBOL_FLAG_IMPORTED) != 0u &&
                existing_symbol->kind == target_symbol->kind &&
                !vitte_sema_same_declaration_private(
                    existing_symbol->payload,
                    target_declaration) &&
                vitte_sema_add_import_ambiguity(
                    context,
                    target_declaration,
                    module_scope,
                    existing_symbol)) {
                continue;
            }
            if (vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                    alias_node != NULL ? alias_node->span : item->span,
                    alias_node != NULL ? alias_node->id : item->id,
                    existing_id,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_help_import_alias_conflict(
                    context,
                    alias,
                    alias_length,
                    false,
                    alias_node != NULL,
                    existing_symbol);
            }
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
    vitte_scope_id_t scope_id,
    bool diagnose_missing)
{
    size_t declaration_index;
    const vitte_scope_entry_t *export_scope;
    bool export_scope_valid;


    if (context == NULL ||
        body == NULL ||
        (body->kind != VITTE_AST_NODE_BLOCK &&
         body->kind != VITTE_AST_NODE_TRANSLATION_UNIT)) {
        return;
    }

    export_scope = vitte_scope_get_scope(&context->scopes, scope_id);
    export_scope_valid = export_scope != NULL &&
        (export_scope->kind == VITTE_SCOPE_KIND_ROOT ||
         export_scope->kind == VITTE_SCOPE_KIND_SPACE);

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

        if (!export_scope_valid) {
            if (diagnose_missing && vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_INVALID_SCOPE,
                    export_node->span,
                    export_node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_append_diagnostic_help(
                    &context->diagnostics[context->diagnostic_count - 1u],
                    "Export clauses belong at module scope. Add `pub` to an individual declaration inside a trait or impl block.");
            }
            continue;
        }

        if (export_node->child_count == 0u) {
            size_t symbol_index;
            for (symbol_index = 0u;
                 symbol_index < context->scopes.symbol_count;
                 ++symbol_index) {
                const vitte_symbol_t *symbol =
                    &context->scopes.symbols[symbol_index];
                if (symbol->scope_id != scope_id ||
                    (symbol->flags & VITTE_SYMBOL_FLAG_IMPORTED) == 0u) {
                    continue;
                }
                if (diagnose_missing && vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT,
                        export_node->span,
                        export_node->id,
                        symbol->id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[
                            context->diagnostic_count - 1u],
                        "`export *;` exports declarations from this module only. Add `pub` to local declarations and import dependencies directly in the consuming module.");
                }
            }
            continue;
        }

        for (item_index = 0u;
             item_index < export_node->child_count;
             ++item_index) {
            const vitte_ast_node_t *item;
            const vitte_ast_node_t *alias_node;
            const vitte_ast_node_t *segment;
            const vitte_ast_node_t *prefix_segment;
            const vitte_symbol_t *target_symbol;
            const vitte_symbol_t *prefix_symbol;
            const char *source_name;
            const char *alias;
            const char *prefix_name;
            size_t source_name_length;
            size_t alias_length;
            size_t prefix_name_length;
            vitte_symbol_id_t target_id;
            vitte_symbol_id_t prefix_id;
            vitte_symbol_id_t qualified_target_id;
            vitte_scope_id_t imported_module_scope;
            bool imported_module_prefix;

            item =
                vitte_sema_node(
                    context,
                    export_node->children[item_index]);
            alias_node = item_index + 1u < export_node->child_count
                ? vitte_sema_node(
                      context,
                      export_node->children[item_index + 1u])
                : NULL;
            if (alias_node != NULL && alias_node->kind != VITTE_AST_NODE_IDENTIFIER) {
                alias_node = NULL;
            }
            if (item == NULL ||
                item->kind != VITTE_AST_NODE_PATH ||
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
                (alias_node != NULL &&
                 !vitte_sema_token_name(
                     vitte_sema_node_token(context, alias_node),
                     &alias,
                     &alias_length))) {
                continue;
            }

            prefix_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
            prefix_symbol = NULL;
            imported_module_prefix = false;
            if (item->child_count > 1u) {
                prefix_segment = vitte_sema_node(
                    context,
                    item->children[0u]);
                if (prefix_segment != NULL &&
                    vitte_sema_token_name(
                        vitte_sema_node_token(context, prefix_segment),
                        &prefix_name,
                        &prefix_name_length)) {
                    prefix_id = vitte_scope_lookup_local_n(
                        &context->scopes,
                        scope_id,
                        prefix_name,
                        prefix_name_length);
                    prefix_symbol = prefix_id !=
                            VITTE_SCOPE_INVALID_SYMBOL_ID
                        ? vitte_scope_get_symbol(
                              &context->scopes,
                              prefix_id)
                        : NULL;
                    imported_module_prefix = prefix_symbol != NULL &&
                        prefix_symbol->kind == VITTE_SYMBOL_KIND_SPACE &&
                        (prefix_symbol->flags &
                         VITTE_SYMBOL_FLAG_IMPORTED) != 0u;
                }
            }

            if (alias_node != NULL) {
                ++item_index;
            } else {
                alias = source_name;
                alias_length = source_name_length;
            }
            target_id =
                vitte_scope_lookup_local_n(
                    &context->scopes,
                    scope_id,
                    source_name,
                    source_name_length);
            /*
             * Export aliases are attached to a space body, while a parser
             * may place declarations from a compact body in its parent
             * lexical scope.  Resolve the local scope first, then walk the
             * lexical ancestors so the alias still points at the exact
             * declaration instead of producing a misleading unknown-name
             * diagnostic.
             */
            if (target_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
                target_id =
                    vitte_scope_lookup_n(
                        &context->scopes,
                        scope_id,
                        source_name,
                        source_name_length);
            }
            target_symbol =
                target_id != VITTE_SCOPE_INVALID_SYMBOL_ID
                    ? vitte_scope_get_symbol(
                          &context->scopes,
                          target_id)
                    : NULL;
            if (imported_module_prefix) {
                const vitte_ast_node_t *module_declaration =
                    (const vitte_ast_node_t *)prefix_symbol->payload;
                const vitte_symbol_t *qualified_target_symbol = NULL;
                imported_module_scope = VITTE_SCOPE_INVALID_SCOPE_ID;
                qualified_target_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
                if (module_declaration != NULL &&
                    vitte_sema_space_scope_private(
                        context,
                        prefix_symbol,
                        module_declaration,
                        &imported_module_scope)) {
                    qualified_target_id = vitte_scope_lookup_local_n(
                        &context->scopes,
                        imported_module_scope,
                        source_name,
                        source_name_length);
                    qualified_target_symbol = qualified_target_id !=
                            VITTE_SCOPE_INVALID_SYMBOL_ID
                        ? vitte_scope_get_symbol(
                              &context->scopes,
                              qualified_target_id)
                        : NULL;
                }
                if (qualified_target_symbol != NULL &&
                    qualified_target_symbol->visibility !=
                        VITTE_SYMBOL_VISIBILITY_PUBLIC) {
                    if (diagnose_missing && vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_PRIVATE_SYMBOL,
                            segment->span,
                            segment->id,
                            qualified_target_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID)) {
                        vitte_sema_help_private_symbol(
                            context,
                            qualified_target_symbol);
                    }
                    continue;
                }
                if (qualified_target_symbol == NULL) {
                    if (diagnose_missing && vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT,
                            segment->span,
                            segment->id,
                            prefix_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID)) {
                        vitte_sema_append_diagnostic_help(
                            &context->diagnostics[
                                context->diagnostic_count - 1u],
                            "This imported module has no declaration with that name. Correct the member path or export a name declared in the current module; re-exports are unsupported.");
                    }
                    continue;
                }
                if (diagnose_missing && vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT,
                        segment->span,
                        segment->id,
                        qualified_target_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[
                            context->diagnostic_count - 1u],
                        "Re-exporting a member through an imported module is unsupported. Add `pub` to its declaration in that module, then use `use module::{name};` in the consuming module.");
                }
                continue;
            }
            if (target_symbol == NULL) {
                if (diagnose_missing &&
                    vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT,
                        segment->span,
                        segment->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[
                            context->diagnostic_count - 1u],
                        "Declare this name in the current module, or correct its spelling. Export paths use `::`, for example `export local_name as public_name;`.");
                }
                continue;
            }

            if ((target_symbol->flags & VITTE_SYMBOL_FLAG_IMPORTED) != 0u) {
                if (diagnose_missing && vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT,
                        segment->span,
                        segment->id,
                        target_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[
                            context->diagnostic_count - 1u],
                        "Re-exporting an imported symbol is unsupported. Add `pub` to its declaration in the defining module, then import it with `use module::{name};`.");
                }
                continue;
            }

            if (alias_node == NULL ||
                (source_name_length == alias_length &&
                 memcmp(source_name, alias, alias_length) == 0)) {
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
                vitte_symbol_id_t existing_id;
                const vitte_symbol_t *existing_symbol;

                existing_id = vitte_scope_lookup_local_n(
                    &context->scopes,
                    scope_id,
                    alias,
                    alias_length);
                existing_symbol = existing_id !=
                        VITTE_SCOPE_INVALID_SYMBOL_ID
                    ? vitte_scope_get_symbol(
                          &context->scopes,
                          existing_id)
                    : NULL;
                if (existing_symbol != NULL &&
                    (existing_symbol->flags &
                     VITTE_SYMBOL_FLAG_IMPORTED) != 0u &&
                    existing_symbol->kind == target_symbol->kind &&
                    vitte_sema_same_declaration_private(
                        existing_symbol->payload,
                        target_symbol->payload)) {
                    continue;
                }
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_DUPLICATE_DECLARATION,
                        alias_node->span,
                        alias_node->id,
                        existing_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[
                            context->diagnostic_count - 1u],
                        "Choose an unused public name after `as`, for example `export local_name as public_name;`, or remove the conflicting export.");
                }
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
    const vitte_ast_node_t *initializer_node;

    explicit_type =
        VITTE_SEMA_INVALID_TYPE_ID;

    initializer_type =
        VITTE_SEMA_INVALID_TYPE_ID;
    initializer_node_id =
        VITTE_AST_INVALID_ID;
    initializer_node = NULL;

    if (node != NULL &&
        node->kind == VITTE_AST_NODE_GENERIC_PARAM) {
        const vitte_sema_node_info_t *generic_info =
            vitte_sema_node_info_private(context, node->id);
        vitte_symbol_id_t symbol_id = generic_info != NULL
            ? generic_info->symbol_id
            : VITTE_SCOPE_INVALID_SYMBOL_ID;
        if ((node->flags & VITTE_AST_FLAG_CONST_GENERIC) != 0u) {
            const vitte_ast_node_t *declared_type =
                node->child_count > 1u
                    ? vitte_sema_node(context, node->children[1])
                    : NULL;
            return declared_type != NULL
                ? vitte_sema_analyze_type_node(
                      context, declared_type->id, scope_id)
                : context->type_error;
        }
        return vitte_sema_create_type_private(
            context,
            VITTE_SEMA_TYPE_GENERIC_PARAMETER,
            VITTE_SEMA_INVALID_TYPE_ID,
            0u,
            symbol_id,
            false);
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

        /* A compound binding is declared after its initializer has been
         * typed; it is not an expression to resolve here. */
        if (child->kind == VITTE_AST_NODE_PATTERN ||
            (index == 0u &&
             (child->kind == VITTE_AST_NODE_IDENTIFIER ||
              child->kind == VITTE_AST_NODE_PATH))) {
            continue;
        }

        if (child->kind ==
                VITTE_AST_NODE_TYPE_EXPR ||
            child->kind ==
                VITTE_AST_NODE_POINTER_TYPE ||
            child->kind ==
                VITTE_AST_NODE_REFERENCE_TYPE ||
            child->kind ==
                VITTE_AST_NODE_OPTIONAL_TYPE ||
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
            initializer_node = child;
        }
    }

    if (explicit_type !=
        VITTE_SEMA_INVALID_TYPE_ID) {
        if (initializer_type !=
                VITTE_SEMA_INVALID_TYPE_ID &&
            !vitte_sema_accept_value_private(
                context,
                explicit_type,
                initializer_type,
                initializer_node_id)) {
            if (vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH,
                    initializer_node != NULL
                        ? initializer_node->span
                        : node->span,
                    initializer_node != NULL
                        ? initializer_node->id
                        : node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    explicit_type,
                    initializer_type)) {
                vitte_sema_add_safe_integer_conversion_help(
                    context,
                    initializer_node_id,
                    explicit_type,
                    initializer_type);
            }
        }

        return explicit_type;
    }

    if (initializer_type !=
        VITTE_SEMA_INVALID_TYPE_ID) {
        return initializer_type;
    }

    return context->type_error;
}

static const vitte_ast_node_t *
vitte_sema_trait_method_for_bound_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *bound_node,
    vitte_scope_id_t generic_scope,
    const vitte_token_t *method_name,
    vitte_symbol_id_t *trait_symbol_id)
{
    vitte_sema_type_id_t bound_type_id;
    const vitte_sema_type_t *bound_type;
    const vitte_symbol_t *trait_symbol;
    const vitte_ast_node_t *trait_declaration;
    size_t index;

    if (trait_symbol_id != NULL) {
        *trait_symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
    }
    if (context == NULL || bound_node == NULL || method_name == NULL ||
        method_name->lexeme == NULL || method_name->length == 0u) {
        return NULL;
    }
    bound_type_id = vitte_sema_analyze_type_node(
        context, bound_node->id, generic_scope);
    bound_type = vitte_sema_type_private(context, bound_type_id);
    trait_symbol = bound_type != NULL &&
            bound_type->kind == VITTE_SEMA_TYPE_NAMED
        ? vitte_scope_get_symbol(&context->scopes, bound_type->symbol_id)
        : NULL;
    if (trait_symbol == NULL ||
        trait_symbol->kind != VITTE_SYMBOL_KIND_TRAIT ||
        trait_symbol->payload == NULL) {
        return NULL;
    }
    if (trait_symbol_id != NULL) {
        *trait_symbol_id = trait_symbol->id;
    }
    trait_declaration = (const vitte_ast_node_t *)trait_symbol->payload;
    for (index = 0u; index < trait_declaration->child_count; ++index) {
        const vitte_ast_node_t *member = vitte_sema_node(
            context, trait_declaration->children[index]);
        const char *member_name;
        size_t member_name_length;
        if (member == NULL ||
            (member->kind != VITTE_AST_NODE_PROC_DECL &&
             member->kind != VITTE_AST_NODE_EXTERN_PROC_DECL) ||
            !vitte_sema_declaration_name(
                context, member, &member_name, &member_name_length)) {
            continue;
        }
        if (member_name_length == method_name->length &&
            memcmp(member_name, method_name->lexeme, member_name_length) == 0) {
            return member;
        }
    }
    return NULL;
}

static vitte_sema_type_id_t
vitte_sema_trait_method_signature_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *method,
    vitte_scope_id_t fallback_scope)
{
    vitte_sema_type_id_t *parameter_types = NULL;
    vitte_sema_type_id_t return_type = 0u;
    vitte_sema_type_id_t signature;
    const vitte_sema_node_info_t *method_info;
    vitte_scope_id_t method_scope;
    size_t parameter_count = 0u;
    size_t parameter_capacity = 0u;
    size_t index;
    bool saw_return = false;

    if (context == NULL || method == NULL) {
        return VITTE_SEMA_INVALID_TYPE_ID;
    }
    method_info = vitte_sema_node_info_private(context, method->id);
    method_scope = method_info != NULL &&
            method_info->scope_id != VITTE_SCOPE_INVALID_SCOPE_ID
        ? method_info->scope_id
        : fallback_scope;
    if (method_info != NULL &&
        method_info->type_id != VITTE_SEMA_INVALID_TYPE_ID) {
        const vitte_sema_type_t *stored_type = vitte_sema_type_private(
            context, method_info->type_id);
        if (stored_type != NULL &&
            stored_type->kind == VITTE_SEMA_TYPE_FUNCTION) {
            return method_info->type_id;
        }
    }
    return_type = context->type_void;
    for (index = 1u; index < method->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, method->children[index]);
        const vitte_ast_node_t *parameter_type;
        vitte_sema_type_id_t type_id;
        if (child == NULL) {
            continue;
        }
        if (child->kind == VITTE_AST_NODE_PARAMETER) {
            parameter_type = child->child_count > 1u
                ? vitte_sema_node(context, child->children[1])
                : NULL;
            type_id = parameter_type != NULL
                ? vitte_sema_analyze_type_node(
                      context, parameter_type->id, method_scope)
                : context->type_error;
            if (type_id == context->type_error) {
                free(parameter_types);
                return VITTE_SEMA_INVALID_TYPE_ID;
            }
            if (parameter_count == parameter_capacity) {
                size_t next_capacity = parameter_capacity == 0u
                    ? 4u
                    : parameter_capacity * 2u;
                vitte_sema_type_id_t *next_parameters =
                    (vitte_sema_type_id_t *)vitte_sema_reallocate(
                        context, parameter_types, next_capacity,
                        sizeof(*parameter_types));
                if (next_parameters == NULL) {
                    free(parameter_types);
                    return VITTE_SEMA_INVALID_TYPE_ID;
                }
                parameter_types = next_parameters;
                parameter_capacity = next_capacity;
            }
            parameter_types[parameter_count++] = type_id;
        } else if (!saw_return &&
                   (child->kind == VITTE_AST_NODE_TYPE_EXPR ||
                    child->kind == VITTE_AST_NODE_POINTER_TYPE ||
                    child->kind == VITTE_AST_NODE_REFERENCE_TYPE ||
                    child->kind == VITTE_AST_NODE_ARRAY_TYPE ||
                    child->kind == VITTE_AST_NODE_OPTIONAL_TYPE)) {
            return_type = vitte_sema_analyze_type_node(
                context, child->id, method_scope);
            if (return_type == context->type_error) {
                free(parameter_types);
                return VITTE_SEMA_INVALID_TYPE_ID;
            }
            saw_return = true;
        }
    }
    signature = vitte_sema_create_function_type_private(
        context, parameter_types, parameter_count, return_type);
    free(parameter_types);
    return signature;
}

static size_t
vitte_sema_method_generic_parameter_ordinal_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *method,
    const vitte_token_t *name)
{
    size_t ordinal = 0u;
    size_t index;
    if (context == NULL || method == NULL || name == NULL ||
        name->lexeme == NULL) {
        return SIZE_MAX;
    }
    for (index = 1u; index < method->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, method->children[index]);
        const vitte_ast_node_t *generic_name;
        const vitte_token_t *generic_token;
        if (child == NULL || child->kind != VITTE_AST_NODE_GENERIC_PARAM) {
            continue;
        }
        generic_name = child->child_count != 0u
            ? vitte_sema_node(context, child->children[0])
            : NULL;
        generic_token = vitte_sema_node_token(context, generic_name);
        if (generic_token != NULL &&
            generic_token->length == name->length &&
            memcmp(generic_token->lexeme, name->lexeme, name->length) == 0) {
            return ordinal;
        }
        ++ordinal;
    }
    return SIZE_MAX;
}

static bool
vitte_sema_method_type_syntax_equal_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *left,
    const vitte_ast_node_t *left_method,
    const vitte_ast_node_t *right,
    const vitte_ast_node_t *right_method,
    size_t depth)
{
    const vitte_token_t *left_token;
    const vitte_token_t *right_token;
    size_t index;
    if (left == NULL || right == NULL || depth >= 32u ||
        left->kind != right->kind || left->operator_kind != right->operator_kind ||
        left->child_count != right->child_count) {
        return false;
    }
    left_token = vitte_sema_node_token(context, left);
    right_token = vitte_sema_node_token(context, right);
    if ((left_token == NULL) != (right_token == NULL)) { return false; }
    if (left_token != NULL) {
        size_t left_generic_ordinal = SIZE_MAX;
        size_t right_generic_ordinal = SIZE_MAX;
        if (left_token->kind == VITTE_TOKEN_IDENTIFIER) {
            left_generic_ordinal =
                vitte_sema_method_generic_parameter_ordinal_private(
                    context, left_method, left_token);
        }
        if (right_token->kind == VITTE_TOKEN_IDENTIFIER) {
            right_generic_ordinal =
                vitte_sema_method_generic_parameter_ordinal_private(
                    context, right_method, right_token);
        }
        if (left_generic_ordinal != SIZE_MAX ||
            right_generic_ordinal != SIZE_MAX) {
            if (left_generic_ordinal == SIZE_MAX ||
                right_generic_ordinal == SIZE_MAX ||
                left_generic_ordinal != right_generic_ordinal) {
                return false;
            }
        } else if (left_token->kind != right_token->kind ||
                   left_token->length != right_token->length ||
                   memcmp(left_token->lexeme, right_token->lexeme,
                          left_token->length) != 0) {
            return false;
        }
    }
    for (index = 0u; index < left->child_count; ++index) {
        const vitte_ast_node_t *left_child = vitte_sema_node(
            context, left->children[index]);
        const vitte_ast_node_t *right_child = vitte_sema_node(
            context, right->children[index]);
        if (!vitte_sema_method_type_syntax_equal_private(
                context, left_child, left_method, right_child,
                right_method, depth + 1u)) {
            return false;
        }
    }
    return true;
}

static const vitte_ast_node_t *
vitte_sema_method_return_type_syntax_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *method)
{
    size_t index;
    if (context == NULL || method == NULL) { return NULL; }
    for (index = 1u; index < method->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, method->children[index]);
        if (child != NULL &&
            (child->kind == VITTE_AST_NODE_TYPE_EXPR ||
             child->kind == VITTE_AST_NODE_POINTER_TYPE ||
             child->kind == VITTE_AST_NODE_REFERENCE_TYPE ||
             child->kind == VITTE_AST_NODE_ARRAY_TYPE ||
             child->kind == VITTE_AST_NODE_OPTIONAL_TYPE)) {
            return child;
        }
    }
    return NULL;
}

static bool
vitte_sema_trait_method_signatures_syntax_equal_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *left,
    const vitte_ast_node_t *right)
{
    size_t left_parameter_count = 0u;
    size_t right_parameter_count = 0u;
    size_t left_generic_count = 0u;
    size_t right_generic_count = 0u;
    size_t left_index;
    size_t right_index;
    size_t left_generic_index;
    size_t right_generic_index;
    const vitte_ast_node_t *left_return;
    const vitte_ast_node_t *right_return;
    const vitte_ast_node_t *left_where = NULL;
    const vitte_ast_node_t *right_where = NULL;
    if (context == NULL || left == NULL || right == NULL ||
        left->flags != right->flags) {
        return false;
    }
    for (left_index = 0u; left_index < left->child_count; ++left_index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, left->children[left_index]);
        if (child != NULL && child->kind == VITTE_AST_NODE_PARAMETER) {
            ++left_parameter_count;
        } else if (child != NULL && child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
            ++left_generic_count;
        }
    }
    for (right_index = 0u; right_index < right->child_count; ++right_index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, right->children[right_index]);
        if (child != NULL && child->kind == VITTE_AST_NODE_PARAMETER) {
            ++right_parameter_count;
        } else if (child != NULL && child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
            ++right_generic_count;
        }
    }
    if (left_parameter_count != right_parameter_count ||
        left_generic_count != right_generic_count) {
        return false;
    }
    left_generic_index = 0u;
    right_generic_index = 0u;
    while (left_generic_index < left->child_count &&
           right_generic_index < right->child_count) {
        const vitte_ast_node_t *left_child = vitte_sema_node(
            context, left->children[left_generic_index++]);
        const vitte_ast_node_t *right_child = vitte_sema_node(
            context, right->children[right_generic_index++]);
        if (left_child == NULL || right_child == NULL) { return false; }
        if (left_child->kind == VITTE_AST_NODE_GENERIC_PARAM &&
            right_child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
            size_t bound_index;
            if ((left_child->flags & VITTE_AST_FLAG_CONST_GENERIC) !=
                    (right_child->flags & VITTE_AST_FLAG_CONST_GENERIC) ||
                left_child->child_count != right_child->child_count) {
                return false;
            }
            for (bound_index = 1u;
                 bound_index < left_child->child_count;
                 ++bound_index) {
                const vitte_ast_node_t *left_bound = vitte_sema_node(
                    context, left_child->children[bound_index]);
                const vitte_ast_node_t *right_bound = vitte_sema_node(
                    context, right_child->children[bound_index]);
                if (!vitte_sema_method_type_syntax_equal_private(
                        context, left_bound, left, right_bound, right, 0u)) {
                    return false;
                }
            }
        }
    }
    for (left_index = 0u; left_index < left->child_count; ++left_index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, left->children[left_index]);
        if (child != NULL && child->kind == VITTE_AST_NODE_WHERE_CLAUSE) {
            left_where = child;
            break;
        }
    }
    for (right_index = 0u; right_index < right->child_count; ++right_index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, right->children[right_index]);
        if (child != NULL && child->kind == VITTE_AST_NODE_WHERE_CLAUSE) {
            right_where = child;
            break;
        }
    }
    if ((left_where == NULL) != (right_where == NULL) ||
        (left_where != NULL &&
         !vitte_sema_method_type_syntax_equal_private(
             context, left_where, left, right_where, right, 0u))) {
        return false;
    }
    left_index = 0u;
    right_index = 0u;
    while (left_index < left->child_count && right_index < right->child_count) {
        const vitte_ast_node_t *left_child = vitte_sema_node(
            context, left->children[left_index++]);
        const vitte_ast_node_t *right_child = vitte_sema_node(
            context, right->children[right_index++]);
        if (left_child == NULL || right_child == NULL) { return false; }
        if (left_child->kind == VITTE_AST_NODE_PARAMETER &&
            right_child->kind == VITTE_AST_NODE_PARAMETER) {
            const vitte_ast_node_t *left_type = left_child->child_count > 1u
                ? vitte_sema_node(context, left_child->children[1])
                : NULL;
            const vitte_ast_node_t *right_type = right_child->child_count > 1u
                ? vitte_sema_node(context, right_child->children[1])
                : NULL;
            if (left_child->flags != right_child->flags ||
                !vitte_sema_method_type_syntax_equal_private(
                    context, left_type, left, right_type, right, 0u)) {
                return false;
            }
        }
    }
    left_return = vitte_sema_method_return_type_syntax_private(context, left);
    right_return = vitte_sema_method_return_type_syntax_private(context, right);
    if ((left_return == NULL) != (right_return == NULL)) { return false; }
    return left_return == NULL ||
        vitte_sema_method_type_syntax_equal_private(
            context, left_return, left, right_return, right, 0u);
}

static bool
vitte_sema_consider_generic_method_bound_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *bound_node,
    vitte_scope_id_t generic_scope,
    const vitte_token_t *method_name,
    const vitte_ast_node_t *member_node,
    vitte_symbol_id_t generic_symbol_id,
    vitte_symbol_id_t *first_trait_symbol_id,
    const vitte_ast_node_t **first_method,
    vitte_sema_type_id_t *first_signature)
{
    vitte_symbol_id_t trait_symbol_id;
    const vitte_ast_node_t *method = vitte_sema_trait_method_for_bound_private(
        context, bound_node, generic_scope, method_name, &trait_symbol_id);
    vitte_sema_type_id_t signature;

    if (method == NULL || trait_symbol_id == *first_trait_symbol_id) {
        return true;
    }
    signature = vitte_sema_trait_method_signature_private(
        context, method, generic_scope);
    if (signature == VITTE_SEMA_INVALID_TYPE_ID) {
        return true;
    }
    if (*first_method == NULL) {
        *first_trait_symbol_id = trait_symbol_id;
        *first_method = method;
        *first_signature = signature;
        return true;
    }
    if (!vitte_sema_type_identical_private(
            context, *first_signature, signature) &&
        !vitte_sema_trait_method_signatures_syntax_equal_private(
            context, *first_method, method)) {
        if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_METHOD,
                member_node->span,
                member_node->id,
                generic_symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
            vitte_sema_diagnostic_t *diagnostic =
                &context->diagnostics[context->diagnostic_count - 1u];
            (void)snprintf(
                diagnostic->help,
                sizeof(diagnostic->help),
                "This method is provided by multiple applicable trait bounds with incompatible signatures. Qualify the trait or remove the conflicting bound.");
            vitte_sema_set_last_related_location(
                context, *first_method,
                "other applicable method candidate declared here");
        }
        return false;
    }
    return true;
}

static vitte_sema_type_id_t
vitte_sema_generic_bound_method_type_private(
    vitte_sema_t *context,
    vitte_sema_type_id_t generic_type_id,
    const vitte_ast_node_t *member_node,
    const vitte_token_t *method_name)
{
    const vitte_sema_type_t *generic_type = vitte_sema_type_private(
        context, generic_type_id);
    size_t node_id;
    size_t node_count;
    vitte_symbol_id_t first_trait_symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
    const vitte_ast_node_t *first_method = NULL;
    vitte_sema_type_id_t first_signature = VITTE_SEMA_INVALID_TYPE_ID;

    if (generic_type == NULL ||
        generic_type->kind != VITTE_SEMA_TYPE_GENERIC_PARAMETER ||
        generic_type->symbol_id == VITTE_SCOPE_INVALID_SYMBOL_ID) {
        return VITTE_SEMA_INVALID_TYPE_ID;
    }
    node_count = vitte_parser_node_count(context->parser);
    for (node_id = 1u; node_id <= node_count; ++node_id) {
        const vitte_ast_node_t *generic = vitte_sema_node(
            context, (vitte_ast_node_id_t)node_id);
        const vitte_sema_node_info_t *generic_info;
        size_t bound_index;
        if (generic == NULL || generic->kind != VITTE_AST_NODE_GENERIC_PARAM) {
            continue;
        }
        generic_info = vitte_sema_node_info_private(context, generic->id);
        if (generic_info == NULL ||
            generic_info->symbol_id != generic_type->symbol_id) {
            continue;
        }
        for (bound_index = 1u; bound_index < generic->child_count; ++bound_index) {
            const vitte_ast_node_t *bound = vitte_sema_node(
                context, generic->children[bound_index]);
            if (bound != NULL &&
                !vitte_sema_consider_generic_method_bound_private(
                    context, bound, generic_info->scope_id, method_name,
                    member_node, generic_type->symbol_id,
                    &first_trait_symbol_id, &first_method, &first_signature)) {
                return context->type_error;
            }
        }
        for (size_t owner_id = 1u; owner_id <= node_count; ++owner_id) {
            const vitte_ast_node_t *owner = vitte_sema_node(
                context, (vitte_ast_node_id_t)owner_id);
            bool owns_generic = false;
            size_t owner_index;
            if (owner == NULL) { continue; }
            for (owner_index = 0u; owner_index < owner->child_count; ++owner_index) {
                if (owner->children[owner_index] == generic->id) {
                    owns_generic = true;
                    break;
                }
            }
            if (!owns_generic) { continue; }
            for (owner_index = 0u; owner_index < owner->child_count; ++owner_index) {
                const vitte_ast_node_t *clause = vitte_sema_node(
                    context, owner->children[owner_index]);
                size_t predicate_index;
                if (clause == NULL || clause->kind != VITTE_AST_NODE_WHERE_CLAUSE) {
                    continue;
                }
                for (predicate_index = 0u;
                     predicate_index < clause->child_count;
                     ++predicate_index) {
                    const vitte_ast_node_t *predicate = vitte_sema_node(
                        context, clause->children[predicate_index]);
                    const vitte_ast_node_t *subject;
                    vitte_sema_type_id_t subject_type;
                    size_t where_bound_index;
                    if (predicate == NULL ||
                        predicate->kind != VITTE_AST_NODE_WHERE_PREDICATE ||
                        predicate->operator_kind != VITTE_TOKEN_COLON ||
                        predicate->child_count < 2u) {
                        continue;
                    }
                    subject = vitte_sema_node(context, predicate->children[0]);
                    subject_type = subject != NULL
                        ? vitte_sema_analyze_type_node(
                              context, subject->id, generic_info->scope_id)
                        : context->type_error;
                    if (!vitte_sema_type_identical_private(
                            context, subject_type, generic_type_id)) {
                        continue;
                    }
                    for (where_bound_index = 1u;
                         where_bound_index < predicate->child_count;
                         ++where_bound_index) {
                        const vitte_ast_node_t *bound = vitte_sema_node(
                            context, predicate->children[where_bound_index]);
                        if (bound != NULL &&
                            !vitte_sema_consider_generic_method_bound_private(
                                context, bound, generic_info->scope_id,
                                method_name, member_node,
                                generic_type->symbol_id,
                                &first_trait_symbol_id, &first_method,
                                &first_signature)) {
                            return context->type_error;
                        }
                    }
                }
            }
        }
    }
    return first_method != NULL
        ? first_signature
        : VITTE_SEMA_INVALID_TYPE_ID;
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
    vitte_sema_pick_variant_resolution_t pick_resolution;
    size_t scope_index;
    bool mutable_receiver;
    bool pointer_receiver;

    if (context == NULL ||
        node == NULL ||
        node->child_count < 2u) {
        return context != NULL
            ? context->type_error
            : VITTE_SEMA_INVALID_TYPE_ID;
    }

    if (vitte_sema_resolve_pick_variant_path(
            context, node, scope_id, &pick_resolution)) {
        vitte_sema_type_id_t variant_type = pick_resolution.pick_type_id;
        info = vitte_sema_node_info_mut_private(context, node->id);
        if (info != NULL) {
            info->type_id = variant_type;
            info->symbol_id = pick_resolution.pick_symbol_id;
            info->scope_id = scope_id;
            info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
        }
        return variant_type;
    }

    {
        const vitte_ast_node_t *base_node =
            vitte_sema_node(context, node->children[0]);
        const vitte_ast_node_t *static_member =
            vitte_sema_node(context, node->children[1]);
        const vitte_token_t *base_token =
            vitte_sema_node_token(context, base_node);
        const vitte_token_t *static_member_token =
            vitte_sema_node_token(context, static_member);
        bool is_string_constructor;
        bool is_usize_max;

        is_usize_max =
            base_node != NULL &&
            (base_node->kind == VITTE_AST_NODE_IDENTIFIER ||
             (base_node->kind == VITTE_AST_NODE_PATH &&
              base_node->child_count == 1u)) &&
            base_token != NULL &&
            base_token->length == 5u &&
            memcmp(base_token->lexeme, "usize", 5u) == 0 &&
            static_member_token != NULL &&
            static_member_token->length == 3u &&
            memcmp(static_member_token->lexeme, "MAX", 3u) == 0;

        if (is_usize_max) {
            info = vitte_sema_node_info_mut_private(context, node->id);
            if (info != NULL) {
                info->type_id = context->type_u64;
                info->scope_id = scope_id;
                info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
            }
            return context->type_u64;
        }

        is_string_constructor =
            base_node != NULL &&
            (base_node->kind == VITTE_AST_NODE_IDENTIFIER ||
             (base_node->kind == VITTE_AST_NODE_PATH &&
              base_node->child_count == 1u)) &&
            base_token != NULL &&
            base_token->length == 6u &&
            memcmp(base_token->lexeme, "string", 6u) == 0 &&
            static_member_token != NULL &&
            ((static_member_token->length == 4u &&
              memcmp(static_member_token->lexeme, "from", 4u) == 0) ||
             (static_member_token->length == 10u &&
              memcmp(static_member_token->lexeme, "from_slice", 10u) == 0) ||
             (static_member_token->length == 9u &&
              memcmp(static_member_token->lexeme, "from_byte", 9u) == 0) ||
             (static_member_token->length == 10u &&
              memcmp(static_member_token->lexeme, "from_bytes", 10u) == 0) ||
             (static_member_token->length == 13u &&
              memcmp(static_member_token->lexeme, "with_capacity", 13u) == 0));

        if (is_string_constructor) {
            return vitte_sema_member_builtin_function(
                context, node, scope_id, context->type_string);
        }
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
    pointer_receiver = false;

    while (base_type != NULL &&
           (base_type->kind ==
                VITTE_SEMA_TYPE_REFERENCE ||
            base_type->kind ==
                VITTE_SEMA_TYPE_POINTER)) {
        if (base_type->kind == VITTE_SEMA_TYPE_POINTER) {
            pointer_receiver = true;
        }
        if (base_type->is_mutable) {
            mutable_receiver = true;
        }

        base_type_id = base_type->element_type;
        base_type =
            vitte_sema_type_private(
                context,
                base_type_id);
    }


    member_node = vitte_sema_node(context, node->children[1]);
    member_token = vitte_sema_node_token(context, member_node);
    if (base_type != NULL && member_token != NULL &&
        base_type->kind == VITTE_SEMA_TYPE_STRING) {
        if (member_token->length == 6u &&
            memcmp(member_token->lexeme, "length", 6u) == 0) {
            info = vitte_sema_node_info_mut_private(context, node->id);
            if (info != NULL) {
                info->type_id = context->type_u64;
                info->scope_id = scope_id;
                info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
            }
            return context->type_u64;
        }
        if (member_token->length == 3u &&
            memcmp(member_token->lexeme, "len", 3u) == 0) {
            info = vitte_sema_node_info_mut_private(context, node->id);
            if (info != NULL) {
                info->type_id = context->type_u64;
                info->scope_id = scope_id;
                info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
            }
            return context->type_u64;
        }
        if (member_token->length == 8u &&
            memcmp(member_token->lexeme, "as_bytes", 8u) == 0) {
            vitte_sema_type_id_t bytes_type = vitte_sema_create_type_private(
                context,
                VITTE_SEMA_TYPE_SLICE,
                context->type_u8,
                0u,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                false);
            bytes_type = vitte_sema_create_type_private(
                context,
                VITTE_SEMA_TYPE_REFERENCE,
                bytes_type,
                0u,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                false);
            return vitte_sema_member_builtin_function(
                context, node, scope_id, bytes_type);
        }
        if (member_token->length == 6u &&
            memcmp(member_token->lexeme, "as_ptr", 6u) == 0) {
            vitte_sema_type_id_t pointer_type = vitte_sema_create_type_private(
                context,
                VITTE_SEMA_TYPE_POINTER,
                context->type_u8,
                0u,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                false);
            return vitte_sema_member_builtin_function(
                context, node, scope_id, pointer_type);
        }
        if (member_token->length == 5u &&
            memcmp(member_token->lexeme, "slice", 5u) == 0) {
            return vitte_sema_member_builtin_function(
                context, node, scope_id, context->type_string);
        }
    }
    if (base_type != NULL &&
        (base_type->kind == VITTE_SEMA_TYPE_ARRAY ||
         base_type->kind == VITTE_SEMA_TYPE_SLICE) &&
        member_token != NULL) {
        if (base_type->kind == VITTE_SEMA_TYPE_SLICE &&
            member_token->length == 6u &&
            memcmp(member_token->lexeme, "length", 6u) == 0) {
            info = vitte_sema_node_info_mut_private(context, node->id);
            if (info != NULL) {
                info->type_id = context->type_u64;
                info->scope_id = scope_id;
                info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
            }
            return context->type_u64;
        }
        if (member_token->length == 3u &&
            memcmp(member_token->lexeme, "len", 3u) == 0) {
            info = vitte_sema_node_info_mut_private(context, node->id);
            if (info != NULL) {
                info->type_id = context->type_u64;
                info->scope_id = scope_id;
                info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
            }
            return context->type_u64;
        }
        if (member_token->length == 6u &&
            memcmp(member_token->lexeme, "as_ptr", 6u) == 0) {
            vitte_sema_type_id_t pointer_type = vitte_sema_create_type_private(
                context,
                VITTE_SEMA_TYPE_POINTER,
                base_type->element_type,
                0u,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                false);
            return vitte_sema_member_builtin_function(
                context, node, scope_id, pointer_type);
        }
        if (member_token->length == 5u &&
            memcmp(member_token->lexeme, "slice", 5u) == 0) {
            vitte_sema_type_id_t slice_type = vitte_sema_create_type_private(
                context,
                VITTE_SEMA_TYPE_SLICE,
                base_type->element_type,
                0u,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                false);
            return vitte_sema_member_builtin_function(
                context, node, scope_id, slice_type);
        }
    }
    if (base_type != NULL &&
        (base_type->kind == VITTE_SEMA_TYPE_ARRAY ||
         base_type->kind == VITTE_SEMA_TYPE_SLICE) &&
        member_token != NULL &&
        ((member_token->length == 4u &&
          memcmp(member_token->lexeme, "push", 4u) == 0) ||
         (member_token->length == 7u &&
          memcmp(member_token->lexeme, "reserve", 7u) == 0))) {
        vitte_sema_type_id_t function_type =
            vitte_sema_create_type_private(
                context,
                VITTE_SEMA_TYPE_FUNCTION,
                VITTE_SEMA_INVALID_TYPE_ID,
                0u,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                false);
        vitte_sema_type_t *function =
            vitte_sema_type_mut_private(context, function_type);

        if (function == NULL) {
            return context->type_error;
        }
        function->return_type = context->type_bool;
        info = vitte_sema_node_info_mut_private(context, node->id);
        if (info != NULL) {
            info->type_id = function_type;
            info->scope_id = scope_id;
            info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED |
                           VITTE_SEMA_NODE_FLAG_BUILTIN;
        }
        return function_type;
    }

    if (base_type != NULL &&
        base_type->kind == VITTE_SEMA_TYPE_GENERIC_PARAMETER &&
        member_node != NULL && member_token != NULL) {
        vitte_sema_type_id_t bound_method_type =
            vitte_sema_generic_bound_method_type_private(
                context, base_type_id, member_node, member_token);
        if (bound_method_type == context->type_error) {
            return context->type_error;
        }
        if (bound_method_type != VITTE_SEMA_INVALID_TYPE_ID) {
            info = vitte_sema_node_info_mut_private(context, node->id);
            if (info != NULL) {
                info->type_id = bound_method_type;
                info->scope_id = scope_id;
                info->flags |= VITTE_SEMA_NODE_FLAG_RESOLVED;
            }
            return bound_method_type;
        }
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

    if (type_declaration == NULL ||
        type_declaration->kind !=
            VITTE_AST_NODE_FORM_DECL ||
        member_token == NULL ||
        member_token->lexeme == NULL ||
        member_token->length == 0u) {
        if (member_token != NULL &&
            vitte_sema_report_incomplete_type_private(
                context,
                member_node != NULL ? member_node : node,
                base_type_id)) {
            return context->type_error;
        }
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
        if (vitte_sema_add_diagnostic(
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
            base_type_id)) {
            vitte_sema_set_last_related_location(
                context,
                type_declaration,
                "receiver type declared here");
        }

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

        field_type_id = vitte_sema_substitute_generic_type_private(
            context,
            field_type_id,
            base_type_id);

        info =
            vitte_sema_node_info_mut_private(
                context,
                node->id);

        if (info != NULL) {
            info->type_id = field_type_id;
            info->symbol_id = field_symbol_id;
            if ((base_info != NULL &&
                 (base_info->flags & VITTE_SEMA_NODE_FLAG_ADDRESSABLE) != 0u) ||
                pointer_receiver) {
                info->flags |= VITTE_SEMA_NODE_FLAG_LVALUE |
                               VITTE_SEMA_NODE_FLAG_ADDRESSABLE;
            }

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

#include "flow.inc"

static void
vitte_sema_mark_unreachable_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *node)
{
    vitte_sema_node_info_t *info;

    if (context == NULL || node == NULL) {
        return;
    }
    info = vitte_sema_node_info_mut_private(context, node->id);
    if (info != NULL) {
        info->flags |= VITTE_SEMA_NODE_FLAG_UNREACHABLE;
    }
}

static bool
vitte_sema_is_unreachable_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *node)
{
    const vitte_sema_node_info_t *info;

    if (context == NULL || node == NULL) {
        return false;
    }
    info = vitte_sema_node_info_private(context, node->id);
    return info != NULL &&
        (info->flags & VITTE_SEMA_NODE_FLAG_UNREACHABLE) != 0u;
}

static void
vitte_sema_warn_unreachable_private(
    vitte_sema_t *context,
    const vitte_ast_node_t *node,
    const vitte_ast_node_t *cause,
    const char *cause_label,
    const char *help)
{
    vitte_sema_node_info_t *info;

    if (context == NULL || node == NULL) {
        return;
    }
    info = vitte_sema_node_info_mut_private(context, node->id);
    if (info != NULL &&
        (info->flags & VITTE_SEMA_NODE_FLAG_UNREACHABLE) != 0u) {
        return;
    }
    if (vitte_sema_add_diagnostic(
            context, VITTE_SEMA_DIAGNOSTIC_WARNING,
            VITTE_SEMA_DIAGNOSTIC_UNREACHABLE_CODE,
            node->span, node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID)) {
        vitte_sema_diagnostic_t *diagnostic =
            &context->diagnostics[context->diagnostic_count - 1u];
        if (help != NULL) {
            (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                "%s", help);
        }
        if (cause != NULL && cause_label != NULL) {
            vitte_sema_set_last_related_location(
                context, cause, cause_label);
        }
    }
    vitte_sema_mark_unreachable_private(context, node);
}

static bool
vitte_sema_parameter_pattern_refutable_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *pattern,
    size_t depth)
{
    size_t index;

    if (context == NULL || pattern == NULL || depth >= 128u) {
        return true;
    }
    if (pattern->kind == VITTE_AST_NODE_INTEGER_LITERAL ||
        pattern->kind == VITTE_AST_NODE_FLOAT_LITERAL ||
        pattern->kind == VITTE_AST_NODE_STRING_LITERAL ||
        pattern->kind == VITTE_AST_NODE_CHARACTER_LITERAL ||
        pattern->kind == VITTE_AST_NODE_BOOL_LITERAL ||
        pattern->kind == VITTE_AST_NODE_NULL_LITERAL) {
        return true;
    }
    if (pattern->kind == VITTE_AST_NODE_IDENTIFIER) {
        return false;
    }
    if (pattern->kind == VITTE_AST_NODE_PATH) {
        return pattern->child_count > 1u;
    }
    if (pattern->kind != VITTE_AST_NODE_PATTERN ||
        (pattern->flags & (VITTE_AST_FLAG_PATTERN_RANGE |
                           VITTE_AST_FLAG_PATTERN_OR)) != 0u ||
        ((pattern->flags & VITTE_AST_FLAG_PATTERN_HEAD) != 0u &&
         (pattern->flags & VITTE_AST_FLAG_PATTERN_STRUCT) == 0u &&
         pattern->child_count != 0u &&
         !(pattern->child_count == 2u &&
           vitte_sema_node(context, pattern->children[0]) != NULL &&
           vitte_sema_node(context, pattern->children[0])->kind ==
               VITTE_AST_NODE_IDENTIFIER))) {
        return true;
    }
    for (index = 0u; index < pattern->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, pattern->children[index]);
        if (child != NULL &&
            vitte_sema_parameter_pattern_refutable_private(
                context, child, depth + 1u) &&
            !((pattern->flags & VITTE_AST_FLAG_PATTERN_STRUCT) != 0u &&
              index == 0u && child->kind == VITTE_AST_NODE_IDENTIFIER)) {
            /* In a struct pattern the head is a type name, not a binding. */
            return true;
        }
    }
    return false;
}

static void
vitte_sema_suppress_pattern_type_cascades_private(
    vitte_sema_t *context,
    size_t first_diagnostic,
    const vitte_parser_span_t *pattern_span)
{
    size_t index;

    if (context == NULL || pattern_span == NULL || !pattern_span->valid) {
        return;
    }
    index = first_diagnostic;
    while (index < context->diagnostic_count) {
        vitte_sema_diagnostic_t *diagnostic = &context->diagnostics[index];
        bool inside_pattern = diagnostic->span.valid &&
            diagnostic->span.file_id == pattern_span->file_id &&
            diagnostic->span.begin >= pattern_span->begin &&
            diagnostic->span.end <= pattern_span->end;
        if (diagnostic->code == VITTE_SEMA_DIAGNOSTIC_TYPE_MISMATCH &&
            inside_pattern) {
            if (diagnostic->kind == VITTE_SEMA_DIAGNOSTIC_ERROR &&
                context->stats.errors != 0u) {
                --context->stats.errors;
            } else if (diagnostic->kind == VITTE_SEMA_DIAGNOSTIC_WARNING &&
                       context->stats.warnings != 0u) {
                --context->stats.warnings;
            }
            if (context->stats.diagnostics != 0u) {
                --context->stats.diagnostics;
            }
            --context->diagnostic_count;
            if (index < context->diagnostic_count) {
                memmove(&context->diagnostics[index],
                        &context->diagnostics[index + 1u],
                        (context->diagnostic_count - index) *
                            sizeof(*context->diagnostics));
            }
            continue;
        }
        ++index;
    }
}

static void
vitte_sema_format_type_label_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id,
    char *buffer,
    size_t capacity,
    size_t depth)
{
    const vitte_sema_type_t *type;
    const vitte_symbol_t *symbol;
    char element[128];
    int written;

    if (buffer == NULL || capacity == 0u) { return; }
    buffer[0] = '\0';
    type = vitte_sema_type_private(context, type_id);
    if (type == NULL || depth >= 8u) {
        (void)snprintf(buffer, capacity, "<unknown>");
        return;
    }
    if (type->kind == VITTE_SEMA_TYPE_NAMED ||
        type->kind == VITTE_SEMA_TYPE_DYN) {
        symbol = vitte_scope_get_symbol(&context->scopes, type->symbol_id);
        if (symbol != NULL && symbol->name != NULL &&
            symbol->name_length != 0u) {
            written = snprintf(buffer, capacity, "%.*s",
                (int)(symbol->name_length > 96u ? 96u : symbol->name_length),
                symbol->name);
            if (written >= 0 && (size_t)written < capacity &&
                type->kind == VITTE_SEMA_TYPE_NAMED &&
                type->generic_argument_count != 0u) {
                size_t offset = (size_t)written;
                size_t index;
                for (index = 0u; index < type->generic_argument_count; ++index) {
                    if (offset + 2u >= capacity) { break; }
                    vitte_sema_format_type_label_private(
                        context, type->generic_arguments[index], element,
                        sizeof(element), depth + 1u);
                    written = snprintf(buffer + offset, capacity - offset,
                        index == 0u ? "<%s" : ", %s", element);
                    if (written < 0 || (size_t)written >= capacity - offset) {
                        break;
                    }
                    offset += (size_t)written;
                }
                if (offset + 1u < capacity) {
                    buffer[offset++] = '>';
                    buffer[offset] = '\0';
                }
            }
            return;
        }
    }
    if (type->kind == VITTE_SEMA_TYPE_POINTER ||
        type->kind == VITTE_SEMA_TYPE_REFERENCE ||
        type->kind == VITTE_SEMA_TYPE_OPTIONAL) {
        vitte_sema_format_type_label_private(
            context, type->element_type, element, sizeof(element), depth + 1u);
        if (type->kind == VITTE_SEMA_TYPE_POINTER) {
            (void)snprintf(buffer, capacity, "%s%s",
                type->is_mutable ? "*mut " : "*", element);
        } else if (type->kind == VITTE_SEMA_TYPE_REFERENCE) {
            (void)snprintf(buffer, capacity, "%s%s",
                type->is_mutable ? "&mut " : "&", element);
        } else {
            (void)snprintf(buffer, capacity, "?%s", element);
        }
        return;
    }
    (void)snprintf(buffer, capacity, "%s",
        vitte_sema_type_kind_name(type->kind));
}

static bool
vitte_sema_first_type_name_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *node,
    char *buffer,
    size_t capacity,
    size_t depth)
{
    const vitte_token_t *token;
    size_t index;

    if (context == NULL || node == NULL || buffer == NULL ||
        capacity == 0u || depth >= 32u) {
        return false;
    }
    token = vitte_sema_node_token(context, node);
    if (token != NULL && token->kind == VITTE_TOKEN_IDENTIFIER &&
        token->lexeme != NULL && token->length != 0u) {
        size_t length = token->length < capacity - 1u
            ? token->length : capacity - 1u;
        memcpy(buffer, token->lexeme, length);
        buffer[length] = '\0';
        return true;
    }
    for (index = 0u; index < node->child_count; ++index) {
        if (vitte_sema_first_type_name_private(
                context,
                vitte_sema_node(context, node->children[index]),
                buffer, capacity, depth + 1u)) {
            return true;
        }
    }
    return false;
}

static const vitte_token_t *
vitte_sema_external_abi_token_private(
    const vitte_sema_t *context,
    const vitte_ast_node_t *declaration)
{
    size_t index;
    bool saw_extern = false;

    if (context == NULL || context->parser == NULL || declaration == NULL) {
        return NULL;
    }
    for (index = 0u; index < context->parser->token_count; ++index) {
        const vitte_token_t *token = &context->parser->tokens[index];
        if (!token->span.valid || token->span.begin < declaration->span.begin ||
            token->span.end > declaration->span.end) {
            continue;
        }
        if (!saw_extern) {
            saw_extern = token->kind == VITTE_TOKEN_KW_EXTERN;
            continue;
        }
        if (token->kind == VITTE_TOKEN_KW_PROC) {
            break;
        }
        if (token->kind == VITTE_TOKEN_STRING_LITERAL) {
            return token;
        }
    }
    return NULL;
}

static bool
vitte_sema_ffi_type_supported_depth_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id,
    bool allow_void,
    size_t depth)
{
    const vitte_sema_type_t *type;

    if (context == NULL || depth > 32u) {
        return false;
    }
    type_id = vitte_sema_resolve_alias_type_private(context, type_id);
    type = vitte_sema_type_private(context, type_id);

    if (type == NULL || type->kind == VITTE_SEMA_TYPE_ERROR) {
        return true;
    }
    if (type->kind == VITTE_SEMA_TYPE_POINTER ||
        type->kind == VITTE_SEMA_TYPE_REFERENCE) {
        /* Pointer representation is ABI-stable even when the pointee is opaque. */
        return true;
    }
    if (type->kind == VITTE_SEMA_TYPE_BOOL ||
        type->kind == VITTE_SEMA_TYPE_I64 ||
        type->kind == VITTE_SEMA_TYPE_U64 ||
        type->kind == VITTE_SEMA_TYPE_U8 ||
        type->kind == VITTE_SEMA_TYPE_F32 ||
        type->kind == VITTE_SEMA_TYPE_F64 ||
        type->kind == VITTE_SEMA_TYPE_CHAR ||
        type->kind == VITTE_SEMA_TYPE_STRING) {
        return true;
    }
    if (allow_void && type->kind == VITTE_SEMA_TYPE_VOID) {
        return true;
    }

    /* Payload-free picks are emitted as C enums and share the platform C
     * enum ABI. Payload picks use a tagged aggregate and are not a scalar FFI
     * value. */
    if (vitte_sema_type_is_enum_private(context, type_id)) {
        const vitte_symbol_t *symbol = vitte_scope_get_symbol(
            &context->scopes, type->symbol_id);
        const vitte_ast_node_t *declaration = symbol != NULL &&
                symbol->payload != NULL
            ? (const vitte_ast_node_t *)symbol->payload : NULL;
        size_t index;
        if (declaration == NULL || type->generic_argument_count != 0u) {
            return false;
        }
        for (index = 0u; index < declaration->child_count; ++index) {
            const vitte_ast_node_t *child = vitte_sema_node(
                context, declaration->children[index]);
            if (child != NULL && child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
                return false;
            }
            if (child != NULL && child->kind == VITTE_AST_NODE_VARIANT_DECL &&
                child->child_count > 1u) {
                return false;
            }
        }
        return true;
    }

    /* Concrete forms lower to ordinary C structs. Permit them by value only
     * when every field is itself ABI-safe; pointer recursion terminates at the
     * pointer case above. */
    if (type->kind == VITTE_SEMA_TYPE_NAMED) {
        const vitte_symbol_t *symbol = vitte_scope_get_symbol(
            &context->scopes, type->symbol_id);
        const vitte_ast_node_t *declaration = symbol != NULL &&
                symbol->payload != NULL
            ? (const vitte_ast_node_t *)symbol->payload : NULL;
        const vitte_sema_node_info_t *declaration_info;
        vitte_scope_id_t form_scope;
        size_t index;
        bool has_generic_parameter = false;

        if (declaration == NULL ||
            declaration->kind != VITTE_AST_NODE_FORM_DECL ||
            type->generic_argument_count != 0u) {
            return false;
        }
        for (index = 0u; index < declaration->child_count; ++index) {
            const vitte_ast_node_t *child = vitte_sema_node(
                context, declaration->children[index]);
            if (child != NULL && child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
                has_generic_parameter = true;
                break;
            }
        }
        if (has_generic_parameter) {
            return false;
        }
        declaration_info = vitte_sema_node_info_private(
            context, declaration->id);
        form_scope = declaration_info != NULL
            ? declaration_info->scope_id : 0u;
        for (index = 0u; index < declaration->child_count; ++index) {
            const vitte_ast_node_t *field = vitte_sema_node(
                context, declaration->children[index]);
            vitte_sema_type_id_t field_type;
            if (field == NULL || field->kind != VITTE_AST_NODE_FIELD_DECL) {
                continue;
            }
            if (field->child_count < 2u) {
                return false;
            }
            field_type = vitte_sema_analyze_type_node(
                (vitte_sema_t *)context,
                field->children[1],
                form_scope);
            if (!vitte_sema_ffi_type_supported_depth_private(
                    context, field_type, false, depth + 1u)) {
                return false;
            }
        }
        return true;
    }

    return false;
}

static bool
vitte_sema_ffi_type_supported_private(
    const vitte_sema_t *context,
    vitte_sema_type_id_t type_id,
    bool allow_void)
{
    return vitte_sema_ffi_type_supported_depth_private(
        context, type_id, allow_void, 0u);
}

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
    bool previous_contract_ensures;
    bool previous_contract_requires;
    bool previous_unsafe;
    bool previous_async;
    bool previous_comptime;
    size_t index;
    size_t parameter_count;
    vitte_sema_type_id_t *parameter_types;
    vitte_sema_type_id_t return_type;
    const vitte_ast_node_t *return_type_node;
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
    previous_contract_ensures =
        context->current_contract_ensures;
    previous_contract_requires = context->current_contract_requires;
    previous_unsafe = context->current_unsafe;
    previous_async = context->current_async;
    previous_comptime = context->current_comptime;

    context->current_proc_scope =
        proc_scope;
    context->current_contract_ensures = false;
    context->current_contract_requires = false;
    context->current_unsafe =
        (node->flags & VITTE_AST_FLAG_UNSAFE) != 0u;
    context->current_async =
        (node->flags & VITTE_AST_FLAG_ASYNC) != 0u;
    context->current_comptime =
        (node->flags & VITTE_AST_FLAG_COMPTIME) != 0u;

    {
        bool is_external = (node->flags & VITTE_AST_FLAG_EXTERN) != 0u;
        bool is_intrinsic = (node->flags & VITTE_AST_FLAG_INTRINSIC) != 0u;
        bool has_async = (node->flags & VITTE_AST_FLAG_ASYNC) != 0u;
        bool has_comptime = (node->flags & VITTE_AST_FLAG_COMPTIME) != 0u;
        const char *primary_modifier = NULL;
        const char *related_modifier = NULL;
        const char *help = NULL;

        if (has_async && has_comptime) {
            primary_modifier = "comptime";
            related_modifier = "async";
            help = "A procedure cannot be both `async` and `comptime`; choose runtime asynchronous execution or compile-time evaluation.";
        } else if ((is_external || is_intrinsic) && has_async) {
            primary_modifier = "async";
            related_modifier = is_intrinsic ? "intrinsic" : "extern";
            help = "External and intrinsic procedure declarations cannot use `async` until the target ABI defines task-returning calls.";
        } else if ((is_external || is_intrinsic) && has_comptime) {
            primary_modifier = "comptime";
            related_modifier = is_intrinsic ? "intrinsic" : "extern";
            help = "External and intrinsic procedures cannot be evaluated at compile time; remove `comptime` or provide a compile-time procedure body.";
        }
        if (primary_modifier != NULL &&
            vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_INVALID_PROCEDURE_MODIFIER,
                vitte_sema_keyword_span_in_node_private(
                    context, node, primary_modifier),
                node->id,
                symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                VITTE_SEMA_INVALID_TYPE_ID)) {
            vitte_sema_append_diagnostic_help(
                &context->diagnostics[context->diagnostic_count - 1u],
                help);
            vitte_sema_set_last_related_span_private(
                context,
                vitte_sema_keyword_span_in_node_private(
                    context, node, related_modifier),
                "conflicting procedure modifier declared here");
        }
    }

    return_type =
        context->type_void;
    return_type_node = NULL;

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
            size_t parameter_diagnostic_start = context->diagnostic_count;

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

            {
                const vitte_ast_node_t *pattern = child->child_count != 0u
                    ? vitte_sema_node(context, child->children[0])
                    : NULL;
                const vitte_ast_node_t *parameter_type_node =
                    child->child_count > 1u
                        ? vitte_sema_node(context, child->children[1])
                        : NULL;
                bool pattern_refutable =
                    vitte_sema_parameter_pattern_refutable_private(
                        context, pattern, 0u);
                uint32_t mode = child->flags &
                    (VITTE_AST_FLAG_PARAMETER_MOVE |
                     VITTE_AST_FLAG_PARAMETER_OWNED |
                     VITTE_AST_FLAG_PARAMETER_BORROW);

                if (pattern_refutable) {
                    if (vitte_sema_add_diagnostic(
                            context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_PROCEDURE_PARAMETER,
                            pattern != NULL ? pattern->span : child->span,
                            pattern != NULL ? pattern->id : child->id,
                            parameter_symbol, parameter_type,
                            VITTE_SEMA_INVALID_TYPE_ID)) {
                        vitte_sema_append_diagnostic_help(
                            &context->diagnostics[
                                context->diagnostic_count - 1u],
                            "Procedure parameters must use irrefutable binding patterns. Replace this literal, range, alternative, or variant pattern with a name, `_`, tuple, or struct binding pattern.");
                    }
                    vitte_sema_suppress_pattern_type_cascades_private(
                        context, parameter_diagnostic_start,
                        pattern != NULL ? &pattern->span : &child->span);
                }
                if ((mode & (VITTE_AST_FLAG_PARAMETER_OWNED |
                             VITTE_AST_FLAG_PARAMETER_BORROW)) != 0u) {
                    bool is_borrow =
                        (mode & VITTE_AST_FLAG_PARAMETER_BORROW) != 0u;
                    const char *mode_name = is_borrow ? "borrow" : "owned";
                    const char *help = is_borrow
                        ? "The `borrow` parameter mode has no defined calling convention yet. Use a reference type such as `&T` to express borrowing explicitly, or remove the mode."
                        : "The `owned` parameter mode has no separate semantics yet. Parameters already receive their declared value; remove `owned` or use `move` at the call site when transferring a value.";
                    if (vitte_sema_add_diagnostic(
                            context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_PROCEDURE_PARAMETER,
                            vitte_sema_keyword_span_in_node_private(
                                context, child, mode_name),
                            child->id, parameter_symbol, parameter_type,
                            VITTE_SEMA_INVALID_TYPE_ID)) {
                        vitte_sema_append_diagnostic_help(
                            &context->diagnostics[
                                context->diagnostic_count - 1u], help);
                        if (parameter_type_node != NULL) {
                            vitte_sema_set_last_related_location(
                                context, parameter_type_node,
                                "parameter type declared here");
                        }
                    }
                }
            }

            (void)parameter_symbol;
        }
    }

    (void)vitte_sema_generic_declaration_validate_constraints_private(
        context,
        node,
        proc_scope);

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
                VITTE_AST_NODE_OPTIONAL_TYPE ||
            child->kind ==
                VITTE_AST_NODE_ARRAY_TYPE) {
            return_type_node = child;
            return_type =
                vitte_sema_analyze_type_node(
                    context,
                    child->id,
                    proc_scope);
        }
    }

    context->current_return_type =
        return_type;

    parameter_count = 0u;
    for (index = 0u; index < node->child_count; ++index) {
        const vitte_ast_node_t *child = vitte_sema_node(
            context, node->children[index]);
        if (child != NULL && child->kind == VITTE_AST_NODE_PARAMETER) {
            ++parameter_count;
        }
    }
    parameter_types = NULL;
    if (parameter_count != 0u) {
        parameter_types = (vitte_sema_type_id_t *)vitte_sema_reallocate(
            context, NULL, parameter_count, sizeof(*parameter_types));
        if (parameter_types == NULL) {
            context->current_return_type = previous_return;
            context->current_proc_scope = previous_proc_scope;
            context->current_contract_ensures = previous_contract_ensures;
            context->current_contract_requires = previous_contract_requires;
            context->current_unsafe = previous_unsafe;
            context->current_async = previous_async;
            context->current_comptime = previous_comptime;
            return context->type_error;
        }
        parameter_count = 0u;
        for (index = 0u; index < node->child_count; ++index) {
            const vitte_ast_node_t *child = vitte_sema_node(
                context, node->children[index]);
            const vitte_sema_node_info_t *parameter_info;
            if (child == NULL || child->kind != VITTE_AST_NODE_PARAMETER) {
                continue;
            }
            parameter_info = vitte_sema_node_info_private(
                context, child->id);
            parameter_types[parameter_count++] = parameter_info != NULL
                ? parameter_info->type_id
                : context->type_error;
        }
    }

    function_type = vitte_sema_create_function_type_private(
        context, parameter_types, parameter_count, return_type);

    if ((node->flags & VITTE_AST_FLAG_EXTERN) != 0u &&
        (node->flags & VITTE_AST_FLAG_INTRINSIC) == 0u) {
        const vitte_token_t *abi_token =
            vitte_sema_external_abi_token_private(context, node);
        if (abi_token != NULL &&
            (abi_token->lexeme == NULL || abi_token->length != 3u ||
             abi_token->lexeme[0] != '"' || abi_token->lexeme[1] != 'C' ||
             abi_token->lexeme[2] != '"')) {
            if (vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_FFI_ABI,
                    vitte_parser_span_make(
                        abi_token->span.file_id,
                        abi_token->span.begin,
                        abi_token->span.end,
                        abi_token->line,
                        abi_token->column),
                    node->id,
                    symbol_id,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_append_diagnostic_help(
                    &context->diagnostics[context->diagnostic_count - 1u],
                    "The C17 backend implements the platform C calling convention only. Use `extern \"C\"` or omit the ABI string to select it.");
                vitte_sema_set_last_related_location(
                    context, node,
                    "external procedure declaration is here");
            }
        }

        for (index = 0u; index < node->child_count; ++index) {
            const vitte_ast_node_t *child = vitte_sema_node(
                context, node->children[index]);
            if (child != NULL && child->kind == VITTE_AST_NODE_GENERIC_PARAM &&
                vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_FFI_TYPE,
                    child->span,
                    child->id,
                    symbol_id,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_append_diagnostic_help(
                    &context->diagnostics[context->diagnostic_count - 1u],
                    "Generic external procedures do not have a supported C ABI specialization. Declare a concrete external signature or add a Vitte wrapper with a concrete type.");
                vitte_sema_set_last_related_location(
                    context, node,
                    "generic external declaration is here");
            }
        }

        {
            size_t parameter_index = 0u;
            for (index = 0u; index < node->child_count; ++index) {
                const vitte_ast_node_t *parameter = vitte_sema_node(
                    context, node->children[index]);
                const vitte_ast_node_t *type_node;
                vitte_sema_type_id_t type_id;
                if (parameter == NULL ||
                    parameter->kind != VITTE_AST_NODE_PARAMETER) {
                    continue;
                }
                type_node = parameter->child_count > 1u
                    ? vitte_sema_node(context, parameter->children[1])
                    : NULL;
                type_id = parameter_index < parameter_count &&
                    parameter_types != NULL
                    ? parameter_types[parameter_index]
                    : context->type_error;
                ++parameter_index;
                if (type_node == NULL ||
                    vitte_sema_ffi_type_supported_private(
                        context, type_id, false)) {
                    continue;
                }
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_FFI_TYPE,
                        type_node->span,
                        type_node->id,
                        symbol_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        type_id)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[context->diagnostic_count - 1u],
                        "C procedure parameters must use scalar, string, pointer, or reference types. Marshal aggregate, optional, array, slice, function, and generic values through a concrete C-compatible wrapper.");
                    vitte_sema_set_last_related_location(
                        context, node,
                        "external procedure signature is declared here");
                }
            }
        }

        if (return_type_node != NULL &&
            !vitte_sema_ffi_type_supported_private(
                context, return_type, true) &&
            vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_FFI_TYPE,
                return_type_node->span,
                return_type_node->id,
                symbol_id,
                VITTE_SEMA_INVALID_TYPE_ID,
                return_type)) {
            vitte_sema_append_diagnostic_help(
                &context->diagnostics[context->diagnostic_count - 1u],
                "C procedure results must use `void`, scalar, string, pointer, or reference types. Return aggregates, optional values, arrays, slices, functions, and generic values through a concrete C-compatible wrapper.");
            vitte_sema_set_last_related_location(
                context, node,
                "external procedure signature is declared here");
        }
    }
    free(parameter_types);

    if (function_type !=
        VITTE_SEMA_INVALID_TYPE_ID) {
        vitte_sema_type_t *type;

        type =
            vitte_sema_type_mut_private(
                context,
                function_type);

        if (type != NULL) {
            type->symbol_id = symbol_id;
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
            size_t first_contract_diagnostic = context->diagnostic_count;
            size_t diagnostic_index;

            context->current_contract_ensures =
                (child->flags & VITTE_AST_FLAG_CONTRACT_ENSURES) != 0u;
            context->current_contract_requires = !context->current_contract_ensures;
            contract_type =
                vitte_sema_analyze_expression(
                    context,
                    child->id,
                    proc_scope);
            if (contract_type != context->type_bool &&
                contract_type != context->type_error) {
                char actual_type[128];
                vitte_sema_format_type_label_private(
                    context, contract_type, actual_type, sizeof(actual_type), 0u);
                if (vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    context->current_contract_ensures
                        ? VITTE_SEMA_DIAGNOSTIC_CONTRACT_ENSURES
                        : VITTE_SEMA_DIAGNOSTIC_CONTRACT_REQUIRES,
                    child->span,
                    child->id,
                    symbol_id,
                    context->type_bool,
                    contract_type)) {
                    (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                        sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                        "This %s clause must evaluate to `bool`, but this expression has type `%s`. Change the expression to a boolean condition.",
                        context->current_contract_ensures ? "ensures" : "requires",
                        actual_type);
                }
            }
            for (diagnostic_index = first_contract_diagnostic;
                 diagnostic_index < context->diagnostic_count;
                 ++diagnostic_index) {
                vitte_sema_diagnostic_t *diagnostic =
                    &context->diagnostics[diagnostic_index];
                diagnostic->contract_flags = child->flags;
                diagnostic->contract_span = child->span;
                diagnostic->contract_symbol_id = symbol_id;
            }
            context->current_contract_ensures = false;
            context->current_contract_requires = false;
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
                VITTE_AST_NODE_OPTIONAL_TYPE ||
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

        if (child->kind == VITTE_AST_NODE_BLOCK &&
            return_type != context->type_void &&
            return_type != context->type_error &&
            return_type != VITTE_SEMA_INVALID_TYPE_ID &&
            vitte_sema_exit_flow(context, child, 0u) ==
                VITTE_SEMA_EXIT_FALLTHROUGH) {
            vitte_parser_span_t end_span = child->span;
            if (end_span.end > end_span.begin) {
                end_span.begin = end_span.end - 1u;
            }
            if (vitte_sema_add_diagnostic(
                context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_MISSING_RETURN,
                end_span, child->id, symbol_id, return_type,
                VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_diagnostic_t *diagnostic =
                    &context->diagnostics[context->diagnostic_count - 1u];
                char declared_type[128];
                vitte_sema_format_type_label_private(
                    context, return_type, declared_type,
                    sizeof(declared_type), 0u);
                if (return_type_node != NULL) {
                    (void)vitte_sema_first_type_name_private(
                        context, return_type_node, declared_type,
                        sizeof(declared_type), 0u);
                }
                (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                    "This procedure declares return type `%s`, but control flow can reach its closing brace. Return a value of that type on every path or change the signature to return `void`.",
                    declared_type);
                diagnostic->type_context = VITTE_SEMA_TYPE_CONTEXT_RETURN;
                if (return_type_node != NULL) {
                    vitte_sema_set_last_related_location(
                        context, return_type_node,
                        "declared return type is here");
                }
            }
        }
    }

    context->current_return_type =
        previous_return;

    context->current_proc_scope =
        previous_proc_scope;
    context->current_contract_ensures =
        previous_contract_ensures;
    context->current_contract_requires = previous_contract_requires;
    context->current_unsafe = previous_unsafe;
    context->current_async = previous_async;
    context->current_comptime = previous_comptime;

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
    bool inherited_unreachable;
    const vitte_ast_node_t *termination_node;
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

    inherited_unreachable = info != NULL &&
        (info->flags & VITTE_SEMA_NODE_FLAG_UNREACHABLE) != 0u;
    terminated = false;
    termination_node = NULL;

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

        if (inherited_unreachable) {
            vitte_sema_mark_unreachable_private(context, child);
        } else if (terminated) {
            vitte_sema_warn_unreachable_private(
                context, child, termination_node,
                "preceding statement exits control flow here", NULL);

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
                VITTE_AST_NODE_CONTINUE_STMT ||
            vitte_sema_exit_flow(context, child, 0u) ==
                VITTE_SEMA_EXIT_TERMINATED) {
            if (!terminated) {
                termination_node = child;
            }
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
    const vitte_scope_entry_t *procedure_scope;
    const vitte_ast_node_t *procedure_declaration;
    const vitte_ast_node_t *procedure_name_node;
    const vitte_ast_node_t *return_value;
    const vitte_token_t *procedure_name_token;

    if (context->current_defer_node_id != VITTE_AST_INVALID_ID &&
        context->current_defer_node_id != 0u) {
        size_t value_index;
        for (value_index = 0u; value_index < node->child_count; ++value_index) {
            const vitte_ast_node_t *value = vitte_sema_node(
                context, node->children[value_index]);
            if (value != NULL) {
                (void)vitte_sema_analyze_expression(context, value->id, scope_id);
            }
        }
        (void)vitte_sema_defer_control_error(context, node,
            "Remove `return` from this deferred action. Return from the enclosing procedure before scheduling cleanup, or let the deferred action finish normally.");
        return context->type_error;
    }

    if (context->current_proc_scope ==
        VITTE_SCOPE_INVALID_SCOPE_ID) {
        if (vitte_sema_add_diagnostic(
            context,
            VITTE_SEMA_DIAGNOSTIC_ERROR,
            VITTE_SEMA_DIAGNOSTIC_RETURN_OUTSIDE_PROC,
            node->span,
            node->id,
            VITTE_SCOPE_INVALID_SYMBOL_ID,
            VITTE_SEMA_INVALID_TYPE_ID,
            VITTE_SEMA_INVALID_TYPE_ID)) {
            vitte_sema_diagnostic_t *diagnostic =
                &context->diagnostics[context->diagnostic_count - 1u];
            (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                "A `return` statement must be inside a procedure body. Move it into the procedure whose result it should produce.");
            vitte_sema_set_nearest_control_context_private(
                context, scope_id,
                "enclosing procedure declaration is here",
                "enclosing test is here; return requires a procedure");
        }

        return context->type_error;
    }

    return_type =
        vitte_sema_type_private(
            context,
            context->current_return_type);

    return_value = node->child_count != 0u
        ? vitte_sema_node(context, node->children[0])
        : NULL;

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
            (return_type->kind !=
                 VITTE_SEMA_TYPE_REFERENCE &&
             return_type->kind !=
                 VITTE_SEMA_TYPE_POINTER &&
             return_type->kind !=
                 VITTE_SEMA_TYPE_OPTIONAL)) {
            found =
                vitte_sema_value_type_private(
                    context,
                    found);
        }
    }

    /* Preserve the first unresolved name/type diagnostic.  A follow-up
     * return mismatch would only report the error type as though it were a
     * real source value. */
    if (found != context->type_error &&
        context->current_return_type != context->type_error &&
        !vitte_sema_accept_value_private(
            context,
            context->current_return_type,
            found,
            node->child_count != 0u
                ? node->children[0]
                : VITTE_AST_INVALID_ID)) {
        if (vitte_sema_add_diagnostic(
                context,
                VITTE_SEMA_DIAGNOSTIC_ERROR,
                VITTE_SEMA_DIAGNOSTIC_RETURN_TYPE_MISMATCH,
                return_value != NULL ? return_value->span : node->span,
                return_value != NULL ? return_value->id : node->id,
                VITTE_SCOPE_INVALID_SYMBOL_ID,
                context->current_return_type,
                found)) {
            vitte_sema_diagnostic_t *diagnostic;
            char help[256];
            char declared_type[128];
            const vitte_ast_node_t *declared_return_type = NULL;
            size_t related_index;

            vitte_sema_set_last_type_context(
                context,
                VITTE_SEMA_TYPE_CONTEXT_RETURN,
                SIZE_MAX,
                VITTE_SCOPE_INVALID_SYMBOL_ID);

            procedure_scope = vitte_scope_get_scope(
                &context->scopes,
                context->current_proc_scope);
            procedure_declaration = procedure_scope != NULL
                ? (const vitte_ast_node_t *)procedure_scope->payload
                : NULL;
            procedure_name_node =
                procedure_declaration != NULL &&
                        procedure_declaration->child_count != 0u
                    ? vitte_sema_node(
                          context,
                          procedure_declaration->children[0])
                    : NULL;
            procedure_name_token = vitte_sema_node_token(
                context,
                procedure_name_node);
            vitte_sema_format_type_label_private(
                context, context->current_return_type, declared_type,
                sizeof(declared_type), 0u);
            if (procedure_declaration != NULL) {
                for (related_index = 0u;
                     related_index < procedure_declaration->child_count;
                     ++related_index) {
                    const vitte_ast_node_t *candidate = vitte_sema_node(
                        context, procedure_declaration->children[related_index]);
                    if (candidate != NULL &&
                        (candidate->kind == VITTE_AST_NODE_TYPE_EXPR ||
                         candidate->kind == VITTE_AST_NODE_POINTER_TYPE ||
                         candidate->kind == VITTE_AST_NODE_REFERENCE_TYPE ||
                         candidate->kind == VITTE_AST_NODE_OPTIONAL_TYPE ||
                         candidate->kind == VITTE_AST_NODE_ARRAY_TYPE)) {
                        declared_return_type = candidate;
                        break;
                    }
                }
            }
            if (declared_return_type != NULL) {
                (void)vitte_sema_first_type_name_private(
                    context, declared_return_type, declared_type,
                    sizeof(declared_type), 0u);
            }
            if (procedure_name_token != NULL &&
                procedure_name_token->lexeme != NULL &&
                procedure_name_token->length != 0u) {
                size_t procedure_name_length =
                    procedure_name_token->length < 128u
                        ? procedure_name_token->length
                        : 128u;
                (void)snprintf(
                    help,
                    sizeof(help),
                    "Return value in procedure `%.*s` does not match its declared return type `%s`.",
                    (int)procedure_name_length,
                    procedure_name_token->lexeme,
                    declared_type);
            } else {
                (void)snprintf(
                    help,
                    sizeof(help),
                    "This return value does not match the procedure's declared return type `%s`.",
                    declared_type);
            }
            diagnostic =
                &context->diagnostics[
                    context->diagnostic_count - 1u];
            vitte_sema_append_diagnostic_help(
                diagnostic,
                help);
            if (declared_return_type != NULL) {
                vitte_sema_set_last_related_location(
                    context, declared_return_type,
                    "declared return type is here");
            } else if (procedure_declaration != NULL) {
                vitte_sema_set_last_related_location(
                    context, procedure_declaration,
                    "procedure signature declared here");
            }
            if (node->child_count != 0u) {
                vitte_sema_add_safe_integer_conversion_help(
                    context,
                    node->children[0],
                    context->current_return_type,
                    found);
            }
        }
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
vitte_sema_predeclare_values(
    vitte_sema_t *context,
    const vitte_ast_node_id_t *declarations,
    size_t declaration_count,
    vitte_scope_id_t scope_id)
{
    size_t index;

    for (index = 0u; index < declaration_count; ++index) {
        const vitte_ast_node_t *declaration = vitte_sema_node(
            context, declarations[index]);
        vitte_sema_node_info_t *info;

        if (declaration == NULL ||
            (declaration->kind != VITTE_AST_NODE_CONST_DECL &&
             declaration->kind != VITTE_AST_NODE_STATIC_DECL)) {
            continue;
        }
        info = vitte_sema_node_info_mut_private(context, declaration->id);
        if (info != NULL &&
            (info->flags & VITTE_SEMA_NODE_FLAG_PREDECLARED) != 0u) {
            continue;
        }
        (void)vitte_sema_register_declaration(
            context, declaration, scope_id);
        info = vitte_sema_node_info_mut_private(context, declaration->id);
        if (info != NULL) {
            info->flags |= VITTE_SEMA_NODE_FLAG_PREDECLARED;
        }
    }
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
            const vitte_ast_node_t *child = vitte_sema_node(
                context, declaration->children[child_index]);
            if (child != NULL &&
                child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
                has_generic_parameters = true;
                break;
            }
        }

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
                continue;
            }

            if (child->kind ==
                VITTE_AST_NODE_PARAMETER) {
                if (has_generic_parameters) {
                    continue;
                }
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
                    VITTE_AST_NODE_OPTIONAL_TYPE ||
                child->kind ==
                    VITTE_AST_NODE_ARRAY_TYPE) {
                if (has_generic_parameters) {
                    continue;
                }
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

    /* Callback aliases must be materialized before procedure signatures are
     * predeclared, since their parameters can use those aliases directly. */
    for (index = 0u; index < declaration_count; ++index) {
        const vitte_ast_node_t *declaration = vitte_sema_node(
            context, declarations[index]);
        const vitte_ast_node_t *target;
        vitte_sema_node_info_t *declaration_info;
        vitte_sema_type_id_t alias_type;
        bool has_generics;
        size_t child_index;

        if (declaration == NULL ||
            declaration->kind != VITTE_AST_NODE_TYPE_DECL ||
            (declaration->flags & VITTE_AST_FLAG_TYPE_ALIAS) == 0u ||
            declaration->child_count < 2u) {
            continue;
        }
        has_generics = false;
        for (child_index = 1u;
             child_index < declaration->child_count;
             ++child_index) {
            const vitte_ast_node_t *child = vitte_sema_node(
                context, declaration->children[child_index]);
            if (child != NULL &&
                child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
                has_generics = true;
                break;
            }
        }
        if (has_generics) {
            continue;
        }
        declaration_info = vitte_sema_node_info_mut_private(
            context, declaration->id);
        target = vitte_sema_node(
            context,
            declaration->children[declaration->child_count - 1u]);
        if (target == NULL ||
            (target->kind != VITTE_AST_NODE_TYPE_EXPR &&
             target->kind != VITTE_AST_NODE_POINTER_TYPE &&
             target->kind != VITTE_AST_NODE_REFERENCE_TYPE &&
             target->kind != VITTE_AST_NODE_OPTIONAL_TYPE &&
             target->kind != VITTE_AST_NODE_ARRAY_TYPE)) {
            continue;
        }

        alias_type = vitte_sema_analyze_type_node(
            context, target->id, scope_id);
        if (alias_type != context->type_error) {
            const vitte_sema_type_t *resolved_alias =
                vitte_sema_type_private(context, alias_type);
            if (declaration_info != NULL &&
                resolved_alias != NULL &&
                resolved_alias->kind == VITTE_SEMA_TYPE_NAMED &&
                resolved_alias->symbol_id == declaration_info->symbol_id) {
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_RECURSIVE_TYPE,
                        target->span,
                        target->id,
                        declaration_info->symbol_id,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        alias_type)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[
                            context->diagnostic_count - 1u],
                        "This type alias refers to itself without an intervening type definition; introduce a concrete type or indirection.");
                    vitte_sema_set_last_related_location(
                        context,
                        declaration,
                        "recursive type alias declared here");
                }
                alias_type = context->type_error;
            }
        }
        declaration_info = vitte_sema_node_info_mut_private(
            context, declaration->id);
        if (declaration_info != NULL && alias_type != context->type_error) {
            declaration_info->type_id = alias_type;
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
            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *declaration;

                declaration =
                    vitte_sema_node(
                        context,
                        node->children[index]);
                if (declaration != NULL &&
                    declaration->kind ==
                        VITTE_AST_NODE_SPACE_DECL &&
                    declaration->child_count != 1u) {
                    (void)vitte_sema_analyze_node(
                        context,
                        declaration->id,
                        scope_id);
                }
            }

            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *declaration;

                declaration =
                    vitte_sema_node(
                        context,
                        node->children[index]);
                if (declaration != NULL &&
                    declaration->kind ==
                        VITTE_AST_NODE_USE_DECL &&
                    (declaration->flags &
                     VITTE_AST_FLAG_PUBLIC) == 0u) {
                    (void)vitte_sema_analyze_import(
                        context,
                        declaration,
                        scope_id);
                }
            }

            vitte_sema_predeclare_types(
                context,
                node->children,
                node->child_count,
                scope_id);

            vitte_sema_predeclare_values(
                context,
                node->children,
                node->child_count,
                scope_id);

            /* Type aliases are needed while procedure signatures are
             * predeclared. */
            vitte_sema_register_export_aliases(
                context,
                node,
                scope_id,
                false);

            vitte_sema_predeclare_procedures(
                context,
                node->children,
                node->child_count,
                scope_id);

            /* Type and procedure aliases must exist before signatures are
             * analyzed; value aliases are completed after constants below. */
            vitte_sema_register_export_aliases(
                context,
                node,
                scope_id,
                false);

            /* Register value declarations before aliases so an alias can
             * refer to a constant declared in the same module header. */
            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *declaration;

                declaration =
                    vitte_sema_node(
                        context,
                        node->children[index]);
                if (declaration == NULL ||
                    (declaration->kind !=
                         VITTE_AST_NODE_CONST_DECL &&
                     declaration->kind !=
                         VITTE_AST_NODE_STATIC_DECL)) {
                    continue;
                }

                (void)vitte_sema_analyze_node(
                    context,
                    declaration->id,
                    scope_id);
            }

            /* A module header (`space path;`) leaves declarations in the
             * translation-unit scope.  Resolve direct export aliases after
             * values are registered and before procedure bodies are checked. */
            vitte_sema_register_export_aliases(
                context,
                node,
                scope_id,
                true);

            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *declaration;

                declaration =
                    vitte_sema_node(
                        context,
                        node->children[index]);
                if (declaration == NULL ||
                    declaration->kind ==
                        VITTE_AST_NODE_SPACE_DECL ||
                    declaration->kind ==
                        VITTE_AST_NODE_USE_DECL ||
                    declaration->kind ==
                        VITTE_AST_NODE_CONST_DECL ||
                    declaration->kind ==
                        VITTE_AST_NODE_STATIC_DECL) {
                    continue;
                }

                (void)vitte_sema_analyze_node(
                    context,
                    declaration->id,
                    scope_id);
            }

            return context->type_void;

        case VITTE_AST_NODE_CONST_DECL:
        case VITTE_AST_NODE_STATIC_DECL:
        case VITTE_AST_NODE_FIELD_DECL:
        case VITTE_AST_NODE_VARIANT_DECL:
        {
            vitte_symbol_id_t symbol_id;

            symbol_id = info != NULL &&
                    (info->flags & VITTE_SEMA_NODE_FLAG_PREDECLARED) != 0u
                ? info->symbol_id
                : vitte_sema_register_declaration(
                    context, node, scope_id);

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

        case VITTE_AST_NODE_LET_STMT:
        {
            vitte_sema_type_id_t let_result;
            const vitte_ast_node_t *binding;
            const vitte_ast_node_t *initializer;
            const vitte_ast_node_t *diagnostic_node;
            vitte_sema_node_info_t *let_info;
            size_t child_index;
            bool has_explicit_type = false;
            bool has_initializer = false;

            binding = node->child_count != 0u
                ? vitte_sema_node(context, node->children[0])
                : NULL;
            initializer = NULL;
            diagnostic_node = binding;
            while (diagnostic_node != NULL &&
                   diagnostic_node->kind == VITTE_AST_NODE_PATTERN &&
                   diagnostic_node->child_count == 1u &&
                   (diagnostic_node->flags &
                    (VITTE_AST_FLAG_PATTERN_HEAD |
                     VITTE_AST_FLAG_PATTERN_OR |
                     VITTE_AST_FLAG_PATTERN_RANGE)) == 0u) {
                diagnostic_node = vitte_sema_node(
                    context, diagnostic_node->children[0]);
            }
            let_result = vitte_sema_infer_declaration_type(
                context,
                node,
                scope_id);
            for (child_index = 1u;
                 child_index < node->child_count;
                 ++child_index) {
                const vitte_ast_node_t *child =
                    vitte_sema_node(context, node->children[child_index]);
                if (child == NULL) {
                    continue;
                }
                if (child->kind == VITTE_AST_NODE_TYPE_EXPR ||
                    child->kind == VITTE_AST_NODE_POINTER_TYPE ||
                    child->kind == VITTE_AST_NODE_REFERENCE_TYPE ||
                    child->kind == VITTE_AST_NODE_ARRAY_TYPE ||
                    child->kind == VITTE_AST_NODE_OPTIONAL_TYPE) {
                    has_explicit_type = true;
                } else {
                    has_initializer = true;
                    initializer = child;
                }
            }
            if ((!has_explicit_type && !has_initializer) ||
                (!has_explicit_type && initializer != NULL &&
                 initializer->kind == VITTE_AST_NODE_ARRAY_EXPR &&
                 initializer->child_count == 0u)) {
                const vitte_ast_node_t *cause_node =
                    has_initializer ? initializer : diagnostic_node;
                if (cause_node == NULL) cause_node = node;
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_CANNOT_INFER,
                        cause_node->span,
                        cause_node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[context->diagnostic_count - 1u],
                        has_initializer
                            ? "Add an explicit element type after the binding, such as `let values: [ElementType] = [];`."
                            : "Add a type annotation after this binding or provide an initializer so its type can be inferred.");
                }
            }
            if (binding == NULL ||
                binding->kind != VITTE_AST_NODE_PATTERN) {
                vitte_symbol_id_t symbol_id =
                    vitte_sema_register_declaration(
                        context,
                        node,
                        scope_id);

                info = vitte_sema_node_info_mut_private(
                    context,
                    node->id);
                if (info != NULL) {
                    info->symbol_id = symbol_id;
                    info->type_id = let_result;
                }
                return let_result;
            }

            let_info = vitte_sema_node_info_mut_private(
                context,
                node->id);
            if (let_info != NULL) {
                let_info->type_id = let_result;
            }
            vitte_sema_bind_pattern(
                context,
                binding->id,
                scope_id,
                let_result,
                (node->flags & VITTE_AST_FLAG_MUTABLE) != 0u);
            return let_result;
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

            /* Generic parameters belong to the declaration's child scope,
               so bounds and member fields can resolve `T` before the rest of
               the declaration is analyzed. */
            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *generic;
                vitte_sema_node_info_t *generic_info;
                vitte_symbol_id_t generic_symbol;

                generic = vitte_sema_node(
                    context, node->children[index]);
                if (generic == NULL ||
                    generic->kind != VITTE_AST_NODE_GENERIC_PARAM) {
                    continue;
                }
                generic_symbol = vitte_sema_register_declaration(
                    context, generic, child_scope);
                generic_info = vitte_sema_node_info_mut_private(
                    context, generic->id);
                if (generic_info != NULL) {
                    generic_info->symbol_id = generic_symbol;
                    generic_info->scope_id = child_scope;
                    generic_info->type_id =
                        vitte_sema_infer_declaration_type(
                            context, generic, child_scope);
                    generic_info->flags |=
                        VITTE_SEMA_NODE_FLAG_DECLARATION;
                }
            }

            (void)vitte_sema_generic_declaration_validate_constraints_private(
                context,
                node,
                child_scope);

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
                        VITTE_AST_NODE_GENERIC_PARAM) {
                    continue;
                }

                if (node->kind == VITTE_AST_NODE_TRAIT_DECL &&
                    child->kind != VITTE_AST_NODE_GENERIC_PARAM &&
                    child->kind != VITTE_AST_NODE_WHERE_CLAUSE &&
                    child->kind != VITTE_AST_NODE_PROC_DECL &&
                    child->kind != VITTE_AST_NODE_EXTERN_PROC_DECL) {
                    if (vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_TRAIT_MEMBER,
                            child->span,
                            child->id,
                            symbol_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID)) {
                        vitte_sema_append_diagnostic_help(
                            &context->diagnostics[
                                context->diagnostic_count - 1u],
                            "Trait bodies currently support procedure signatures and default procedure bodies. Move this declaration outside the trait or express it as a procedure member.");
                    }
                    continue;
                }

                if (node->kind == VITTE_AST_NODE_TYPE_DECL &&
                    (node->flags & VITTE_AST_FLAG_TYPE_ALIAS) != 0u &&
                    index + 1u == node->child_count) {
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
                                VITTE_AST_NODE_USE_DECL &&
                            (declaration->flags &
                             VITTE_AST_FLAG_PUBLIC) == 0u) {
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
                    vitte_sema_predeclare_values(
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
                            child_scope,
                            true);
                    }
                }
            }

            return context->type_void;
        }

        case VITTE_AST_NODE_USE_DECL:
            if ((node->flags &
                 VITTE_AST_FLAG_PUBLIC) != 0u) {
                const vitte_scope_entry_t *scope_entry =
                    vitte_scope_get_scope(&context->scopes, scope_id);
                if (scope_entry != NULL &&
                    scope_entry->kind != VITTE_SCOPE_KIND_ROOT &&
                    scope_entry->kind != VITTE_SCOPE_KIND_SPACE &&
                    vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_SCOPE,
                        node->span,
                        node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_append_diagnostic_help(
                        &context->diagnostics[
                            context->diagnostic_count - 1u],
                        "Export clauses belong at module scope. Add `pub` to an individual declaration inside a trait or impl block.");
                }
                return context->type_void;
            }
            return vitte_sema_analyze_import(
                context,
                node,
                scope_id);

        case VITTE_AST_NODE_IMPL_DECL:
        {
            vitte_scope_id_t impl_scope;
            vitte_sema_type_id_t previous_self_type;
            vitte_sema_type_id_t impl_self_type;
            const vitte_ast_node_t *first_type_node;
            const vitte_ast_node_t *target_type_node;
            const vitte_ast_node_t *trait_declaration;
            vitte_symbol_id_t trait_symbol_id;
            size_t header_type_count;

            impl_scope = info != NULL
                ? info->scope_id
                : VITTE_SCOPE_INVALID_SCOPE_ID;
            if (impl_scope == VITTE_SCOPE_INVALID_SCOPE_ID ||
                impl_scope == scope_id) {
                for (index = 0u; index < node->child_count; ++index) {
                    const vitte_ast_node_t *generic = vitte_sema_node(
                        context, node->children[index]);
                    const vitte_sema_node_info_t *generic_info;
                    if (generic == NULL ||
                        generic->kind !=
                            VITTE_AST_NODE_GENERIC_PARAM) {
                        continue;
                    }
                    generic_info = vitte_sema_node_info_private(
                        context, generic->id);
                    if (generic_info != NULL &&
                        generic_info->scope_id !=
                            VITTE_SCOPE_INVALID_SCOPE_ID &&
                        generic_info->scope_id != scope_id) {
                        impl_scope = generic_info->scope_id;
                        break;
                    }
                }
            }
            if (impl_scope == VITTE_SCOPE_INVALID_SCOPE_ID ||
                impl_scope == scope_id) {
                impl_scope = vitte_scope_create(
                    &context->scopes,
                    scope_id,
                    VITTE_SCOPE_KIND_IMPL,
                    vitte_sema_scope_span_from_parser(node->span),
                    (void *)node);
            }

            if (impl_scope ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
                impl_scope = scope_id;
            }

            if (info != NULL) {
                info->scope_id = impl_scope;
                info->type_id =
                    context->type_void;
            }

            /* Make impl-level generic parameters visible to the implemented
             * type, where bounds and member declarations are analyzed. */
            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *generic;
                vitte_sema_node_info_t *generic_info;
                vitte_symbol_id_t generic_symbol;

                generic = vitte_sema_node(
                    context,
                    node->children[index]);
                if (generic == NULL ||
                    generic->kind != VITTE_AST_NODE_GENERIC_PARAM) {
                    continue;
                }
                generic_info = vitte_sema_node_info_mut_private(
                    context,
                    generic->id);
                if (generic_info != NULL &&
                    generic_info->symbol_id !=
                        VITTE_SCOPE_INVALID_SYMBOL_ID &&
                    generic_info->scope_id == impl_scope) {
                    continue;
                }
                generic_symbol = vitte_sema_register_declaration(
                    context,
                    generic,
                    impl_scope);
                if (generic_info != NULL) {
                    generic_info->symbol_id = generic_symbol;
                    generic_info->scope_id = impl_scope;
                    generic_info->type_id =
                        vitte_sema_infer_declaration_type(
                            context,
                            generic,
                            impl_scope);
                    generic_info->flags |=
                        VITTE_SEMA_NODE_FLAG_DECLARATION;
                }
            }

            (void)vitte_sema_generic_declaration_validate_constraints_private(
                context,
                node,
                impl_scope);

            /* An impl has one header type for an inherent impl and two for
             * `impl Trait for Type`. Resolve the implementing type before
             * checking members so `self` has a real type in every method. */
            first_type_node = NULL;
            target_type_node = NULL;
            header_type_count = 0u;
            for (index = 0u; index < node->child_count; ++index) {
                const vitte_ast_node_t *child = vitte_sema_node(
                    context, node->children[index]);
                if (child == NULL ||
                    (child->kind != VITTE_AST_NODE_TYPE_EXPR &&
                     child->kind != VITTE_AST_NODE_POINTER_TYPE &&
                     child->kind != VITTE_AST_NODE_REFERENCE_TYPE &&
                     child->kind != VITTE_AST_NODE_ARRAY_TYPE &&
                     child->kind != VITTE_AST_NODE_OPTIONAL_TYPE)) {
                    continue;
                }
                if (header_type_count == 0u) {
                    first_type_node = child;
                } else if (header_type_count == 1u) {
                    target_type_node = child;
                }
                ++header_type_count;
            }

            impl_self_type = context->type_error;
            trait_declaration = NULL;
            trait_symbol_id = VITTE_SCOPE_INVALID_SYMBOL_ID;
            if (header_type_count == 1u && first_type_node != NULL) {
                impl_self_type = vitte_sema_analyze_type_node(
                    context, first_type_node->id, impl_scope);
            } else if (header_type_count == 2u &&
                       first_type_node != NULL &&
                       target_type_node != NULL) {
                vitte_sema_type_id_t trait_type_id =
                    vitte_sema_analyze_type_node(
                        context, first_type_node->id, impl_scope);
                const vitte_sema_type_t *trait_type =
                    vitte_sema_type_private(context, trait_type_id);
                const vitte_symbol_t *trait_symbol = trait_type != NULL &&
                        trait_type->kind == VITTE_SEMA_TYPE_NAMED
                    ? vitte_scope_get_symbol(
                          &context->scopes, trait_type->symbol_id)
                    : NULL;
                vitte_symbol_id_t resolved_trait_symbol_id =
                    trait_type != NULL
                        ? trait_type->symbol_id
                        : VITTE_SCOPE_INVALID_SYMBOL_ID;
                bool resolved_trait = trait_symbol != NULL &&
                    trait_symbol->kind == VITTE_SYMBOL_KIND_TRAIT;

                if (trait_type_id != context->type_error &&
                    !resolved_trait) {
                    if (vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_IMPL_TARGET,
                            first_type_node->span,
                            first_type_node->id,
                            resolved_trait_symbol_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            trait_type_id)) {
                        vitte_sema_diagnostic_t *diagnostic =
                            &context->diagnostics[
                                context->diagnostic_count - 1u];
                        (void)snprintf(
                            diagnostic->help,
                            sizeof(diagnostic->help),
                            "The type before `for` in this impl must name a trait.");
                    }
                } else if (resolved_trait) {
                    trait_symbol_id = resolved_trait_symbol_id;
                    trait_declaration =
                        (const vitte_ast_node_t *)trait_symbol->payload;
                }
                impl_self_type = vitte_sema_analyze_type_node(
                    context, target_type_node->id, impl_scope);
            }

            if (trait_declaration != NULL &&
                impl_self_type != context->type_error) {
                const vitte_ast_node_t *prior_implementation = NULL;
                if (header_type_count == 2u && first_type_node != NULL) {
                    vitte_sema_type_id_t current_trait_type =
                        vitte_sema_analyze_type_node(
                            context, first_type_node->id, impl_scope);
                    prior_implementation =
                        vitte_sema_prior_overlapping_trait_impl_private(
                            context,
                            node,
                            scope_id,
                            current_trait_type,
                            impl_self_type);
                }
                if (prior_implementation != NULL) {
                    if (vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_CONFLICTING_IMPL,
                            node->span,
                            node->id,
                            trait_symbol_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            impl_self_type)) {
                        vitte_sema_append_diagnostic_help(
                            &context->diagnostics[
                                context->diagnostic_count - 1u],
                            "This trait and target type already have an implementation that overlaps this header. Keep one implementation and merge its members.");
                        vitte_sema_set_last_related_location(
                            context,
                            prior_implementation,
                            "conflicting implementation declared here");
                    }
                } else {
                size_t trait_index;
                for (trait_index = 0u;
                     trait_index < trait_declaration->child_count;
                     ++trait_index) {
                    const vitte_ast_node_t *required = vitte_sema_node(
                        context,
                        trait_declaration->children[trait_index]);
                    const char *required_name;
                    size_t required_name_length;
                    bool has_default_body = false;
                    bool found_member = false;
                    size_t member_index;
                    size_t required_child_index;

                    if (required == NULL ||
                        required->kind != VITTE_AST_NODE_PROC_DECL ||
                        !vitte_sema_declaration_name(
                            context,
                            required,
                            &required_name,
                            &required_name_length)) {
                        continue;
                    }
                    for (required_child_index = 0u;
                         required_child_index < required->child_count;
                         ++required_child_index) {
                        const vitte_ast_node_t *required_child =
                            vitte_sema_node(
                                context,
                                required->children[required_child_index]);
                        if (required_child != NULL &&
                            required_child->kind == VITTE_AST_NODE_BLOCK) {
                            has_default_body = true;
                            break;
                        }
                    }
                    if (has_default_body) {
                        continue;
                    }
                    for (member_index = 0u;
                         member_index < node->child_count;
                         ++member_index) {
                        const vitte_ast_node_t *member = vitte_sema_node(
                            context, node->children[member_index]);
                        const char *member_name;
                        size_t member_name_length;
                        if (member == NULL ||
                            (member->kind != VITTE_AST_NODE_PROC_DECL &&
                             member->kind !=
                                 VITTE_AST_NODE_EXTERN_PROC_DECL) ||
                            !vitte_sema_declaration_name(
                                context,
                                member,
                                &member_name,
                                &member_name_length)) {
                            continue;
                        }
                        if (required_name_length == member_name_length &&
                            memcmp(
                                required_name,
                                member_name,
                                required_name_length) == 0) {
                            found_member = true;
                            break;
                        }
                    }
                    if (!found_member &&
                        vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_MISSING_TRAIT_MEMBER,
                            node->span,
                            node->id,
                            trait_symbol_id,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            impl_self_type)) {
                        vitte_sema_diagnostic_t *diagnostic =
                            &context->diagnostics[
                                context->diagnostic_count - 1u];
                        (void)snprintf(
                            diagnostic->help,
                            sizeof(diagnostic->help),
                            "Implementation is missing required trait method `%.*s`.",
                            (int)(required_name_length > 96u
                                      ? 96u
                                      : required_name_length),
                            required_name);
                        vitte_sema_set_last_related_location(
                            context,
                            required,
                            "required trait method declared here");
                    }
                }
                }
            }

            previous_self_type = context->current_self_type;
            context->current_self_type = impl_self_type;

            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *child = vitte_sema_node(
                    context,
                    node->children[index]);
                if (child != NULL &&
                    child->kind == VITTE_AST_NODE_GENERIC_PARAM) {
                    continue;
                }
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    impl_scope);
            }

            context->current_self_type = previous_self_type;

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
        {
            const vitte_ast_node_t *condition = node->child_count != 0u
                ? vitte_sema_node(context, node->children[0]) : NULL;
            vitte_constant_t condition_value;
            bool unreachable_parent =
                vitte_sema_is_unreachable_private(context, node);

            if (node->child_count != 0u) {
                (void)vitte_sema_analyze_condition(
                    context,
                    node->children[0],
                    scope_id);
            }

            condition_value = vitte_sema_known_value(context, condition);
            if (!unreachable_parent &&
                condition_value.kind == VITTE_CONSTANT_KIND_BOOL &&
                node->child_count >= 2u) {
                const vitte_ast_node_t *dead_branch =
                    vitte_sema_node(context, condition_value.as.boolean
                        ? (node->child_count >= 3u
                            ? node->children[2] : VITTE_AST_INVALID_ID)
                        : node->children[1]);
                if (condition_value.as.boolean) {
                    if (dead_branch != NULL) {
                        vitte_sema_warn_unreachable_private(
                            context, dead_branch, condition,
                            "this condition makes the branch unreachable",
                            "This condition is always true, so this alternative cannot run. Change the condition or remove the unreachable alternative.");
                    }
                } else {
                    vitte_sema_warn_unreachable_private(
                        context, dead_branch, condition,
                        "this condition makes the branch unreachable",
                        "This condition is always false, so this branch cannot run. Change the condition or remove the unreachable branch.");
                }
            } else if (unreachable_parent) {
                for (index = 1u; index < node->child_count; ++index) {
                    vitte_sema_mark_unreachable_private(
                        context,
                        vitte_sema_node(context, node->children[index]));
                }
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
        }

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

            if (node->kind == VITTE_AST_NODE_FOR_STMT) {
                vitte_sema_type_id_t iterable_type;
                vitte_sema_type_id_t binding_type;
                const vitte_sema_type_t *iterable;

                iterable_type = node->child_count > 1u
                    ? vitte_sema_analyze_expression(
                          context,
                          node->children[1],
                          loop_scope)
                    : context->type_error;
                iterable = vitte_sema_type_private(
                    context,
                    iterable_type);
                binding_type = iterable != NULL &&
                        (iterable->kind == VITTE_SEMA_TYPE_ARRAY ||
                         iterable->kind == VITTE_SEMA_TYPE_SLICE ||
                         iterable->kind == VITTE_SEMA_TYPE_RANGE)
                    ? iterable->element_type
                    : context->type_error;
                if (node->child_count != 0u) {
                    vitte_sema_bind_pattern(
                        context,
                        node->children[0],
                        loop_scope,
                        binding_type,
                        false);
                }
                if (node->child_count > 2u) {
                    (void)vitte_sema_analyze_node(
                        context,
                        node->children[2],
                        loop_scope);
                }
            } else {
                const vitte_ast_node_t *condition =
                    node->child_count != 0u
                        ? vitte_sema_node(context, node->children[0])
                        : NULL;
                vitte_constant_t condition_value;
                bool unreachable_parent =
                    vitte_sema_is_unreachable_private(context, node);

                if (node->child_count != 0u) {
                    (void)vitte_sema_analyze_condition(
                        context,
                        node->children[0],
                        loop_scope);
                }

                condition_value = vitte_sema_known_value(
                    context, condition);
                if (node->kind == VITTE_AST_NODE_WHILE_STMT &&
                    node->child_count > 1u && !unreachable_parent &&
                    condition_value.kind == VITTE_CONSTANT_KIND_BOOL &&
                    !condition_value.as.boolean) {
                    const vitte_ast_node_t *body = vitte_sema_node(
                        context, node->children[1]);
                    vitte_sema_warn_unreachable_private(
                        context, body, condition,
                        "this condition makes the loop body unreachable",
                        "This loop condition is always false, so the body cannot run. Change the condition or remove the loop.");
                } else if (unreachable_parent) {
                    for (index = 1u; index < node->child_count; ++index) {
                        vitte_sema_mark_unreachable_private(
                            context,
                            vitte_sema_node(context, node->children[index]));
                    }
                }

                for (index = 1u;
                     index < node->child_count;
                     ++index) {
                    (void)vitte_sema_analyze_node(
                        context,
                        node->children[index],
                        loop_scope);
                }
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
            if (context->current_defer_node_id != VITTE_AST_INVALID_ID &&
                context->current_defer_node_id != 0u &&
                context->loop_depth <= context->current_defer_loop_depth) {
                (void)vitte_sema_defer_control_error(context, node,
                    "Remove `break` or `continue` from deferred work. Perform the loop exit in the enclosing body and let cleanup complete normally.");
            } else if (context->loop_depth == 0u) {
                if (vitte_sema_add_diagnostic(
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
                    VITTE_SEMA_INVALID_TYPE_ID)) {
                    vitte_sema_diagnostic_t *diagnostic =
                        &context->diagnostics[
                            context->diagnostic_count - 1u];
                    if (node->kind == VITTE_AST_NODE_BREAK_STMT) {
                        (void)snprintf(diagnostic->help,
                            sizeof(diagnostic->help),
                            "Place `break` inside a loop body to exit that loop.");
                        vitte_sema_set_nearest_control_context_private(
                            context, scope_id,
                            "enclosing procedure is here; place a loop here",
                            "enclosing test is here; place a loop here");
                    } else {
                        (void)snprintf(diagnostic->help,
                            sizeof(diagnostic->help),
                            "Place `continue` inside a loop body to start its next iteration.");
                        vitte_sema_set_nearest_control_context_private(
                            context, scope_id,
                            "enclosing procedure is here; place a loop here",
                            "enclosing test is here; place a loop here");
                    }
                }
            }

            return context->type_void;

        case VITTE_AST_NODE_MATCH_STMT:
        {
            vitte_sema_type_id_t subject_type;
            vitte_scope_id_t match_scope;
            const vitte_ast_node_t *subject;

            subject = node->child_count != 0u
                ? vitte_sema_node(context, node->children[0])
                : NULL;
            subject_type = subject != NULL
                ? vitte_sema_analyze_expression(
                      context, subject->id, scope_id)
                : context->type_error;

            match_scope =
                vitte_scope_create(
                    &context->scopes,
                    scope_id,
                    VITTE_SCOPE_KIND_MATCH,
                    vitte_sema_scope_span_from_parser(
                        node->span),
                    (void *)node);

            if (match_scope ==
                VITTE_SCOPE_INVALID_SCOPE_ID) {
                match_scope = scope_id;
            }

            for (index = 1u;
                 index < node->child_count;
                 ++index) {
                const vitte_ast_node_t *arm;
                const vitte_ast_node_t *pattern;
                vitte_scope_id_t arm_scope;
                size_t child_index;

                arm = vitte_sema_node(context, node->children[index]);
                if (arm == NULL ||
                    arm->kind != VITTE_AST_NODE_MATCH_ARM ||
                    arm->child_count < 2u) {
                    continue;
                }

                arm_scope = vitte_scope_create(
                    &context->scopes,
                    match_scope,
                    VITTE_SCOPE_KIND_MATCH_ARM,
                    vitte_sema_scope_span_from_parser(arm->span),
                    (void *)arm);
                if (arm_scope == VITTE_SCOPE_INVALID_SCOPE_ID) {
                    arm_scope = match_scope;
                }
                pattern = vitte_sema_node(context, arm->children[0]);
                if (pattern != NULL &&
                    (pattern->kind == VITTE_AST_NODE_BOOL_LITERAL ||
                     pattern->kind == VITTE_AST_NODE_INTEGER_LITERAL ||
                     pattern->kind == VITTE_AST_NODE_FLOAT_LITERAL ||
                     pattern->kind == VITTE_AST_NODE_STRING_LITERAL ||
                     pattern->kind == VITTE_AST_NODE_CHARACTER_LITERAL ||
                     pattern->kind == VITTE_AST_NODE_NULL_LITERAL)) {
                    vitte_sema_type_id_t pattern_type = vitte_sema_analyze_expression(context, pattern->id, arm_scope);
                    if (subject_type != context->type_error && pattern_type != context->type_error &&
                        !vitte_sema_accept_value_private(context, subject_type, pattern_type, pattern->id)) {
                        vitte_sema_node_info_t *pattern_info;
                        (void)vitte_sema_add_diagnostic(context, VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE, pattern->span, pattern->id,
                            VITTE_SCOPE_INVALID_SYMBOL_ID, subject_type, pattern_type);
                        pattern_info = vitte_sema_node_info_mut_private(context, pattern->id);
                        if (pattern_info != NULL) pattern_info->type_id = context->type_error;
                    }
                }
                /* Binding handles `|` alternatives by validating their name
                 * sets and registering one shared binding set. */
                if (pattern != NULL) {
                    vitte_sema_bind_pattern(
                        context,
                        pattern->id,
                        arm_scope,
                        subject_type,
                        false);
                }
                for (child_index = 1u;
                     child_index < arm->child_count;
                     ++child_index) {
                    const vitte_ast_node_t *part = vitte_sema_node(context, arm->children[child_index]);
                    if (arm->child_count == 3u && child_index == 1u) {
                        (void)vitte_sema_analyze_condition(context, arm->children[child_index], arm_scope);
                        {
                            vitte_constant_t guard_value =
                                vitte_sema_known_value(context, part);
                            if (guard_value.kind ==
                                    VITTE_CONSTANT_KIND_BOOL &&
                                !guard_value.as.boolean) {
                                vitte_sema_mark_unreachable_private(
                                    context,
                                    vitte_sema_node(context,
                                        arm->children[
                                            arm->child_count - 1u]));
                            }
                        }
                    } else if (part != NULL && part->kind != VITTE_AST_NODE_BLOCK &&
                        part->kind != VITTE_AST_NODE_RETURN_STMT && part->kind != VITTE_AST_NODE_IF_STMT &&
                        part->kind != VITTE_AST_NODE_WHILE_STMT && part->kind != VITTE_AST_NODE_LOOP_STMT &&
                        part->kind != VITTE_AST_NODE_MATCH_STMT && part->kind != VITTE_AST_NODE_BREAK_STMT &&
                        part->kind != VITTE_AST_NODE_CONTINUE_STMT && part->kind != VITTE_AST_NODE_ASSERT_STMT &&
                        part->kind != VITTE_AST_NODE_LET_STMT && part->kind != VITTE_AST_NODE_EXPR_STMT) {
                        vitte_ast_node_t implicit_return = *part;
                        vitte_ast_node_id_t value_id = part->id;
                        implicit_return.kind = VITTE_AST_NODE_RETURN_STMT;
                        implicit_return.children = &value_id;
                        implicit_return.child_count = 1u;
                        (void)vitte_sema_analyze_return(context, &implicit_return, arm_scope);
                    } else {
                        (void)vitte_sema_analyze_node(context, arm->children[child_index], arm_scope);
                    }
                }
            }

            return context->type_void;
        }

        case VITTE_AST_NODE_MATCH_ARM:
            /* Match arms are normally analyzed by their owning match node,
             * which supplies the scrutinee type and registers bindings. */
            for (index = 1u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    scope_id);
            }
            return context->type_void;

        case VITTE_AST_NODE_DEFER_STMT:
            if (context->current_defer_node_id != VITTE_AST_INVALID_ID &&
                context->current_defer_node_id != 0u) {
                (void)vitte_sema_defer_control_error(context, node,
                    "A deferred action cannot register another `defer`. Move the nested cleanup into the enclosing block.");
                return context->type_void;
            }
            {
                vitte_ast_node_id_t previous_defer =
                    context->current_defer_node_id;
                size_t previous_defer_loop_depth =
                    context->current_defer_loop_depth;
                context->current_defer_node_id = node->id;
                context->current_defer_loop_depth = context->loop_depth;
                for (index = 0u; index < node->child_count; ++index) {
                    (void)vitte_sema_analyze_node(
                        context, node->children[index], scope_id);
                }
                context->current_defer_node_id = previous_defer;
                context->current_defer_loop_depth = previous_defer_loop_depth;
            }

            return context->type_void;

        case VITTE_AST_NODE_ASM_STMT:
        {
            const vitte_token_t *syntax_token =
                vitte_parser_token_at(context->parser, node->token_index + 1u);
            bool template_form = syntax_token != NULL &&
                syntax_token->kind == VITTE_TOKEN_STRING_LITERAL;

            if (!context->current_unsafe &&
                vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_UNSAFE_REQUIRED,
                    node->span,
                    node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID)) {
                vitte_sema_diagnostic_t *diagnostic =
                    &context->diagnostics[context->diagnostic_count - 1u];
                (void)snprintf(diagnostic->help, sizeof(diagnostic->help),
                    "Inline assembly is an unsafe operation. Wrap this `asm` statement in `unsafe { ... }` after checking the target instruction and its memory effects.");
            }

            if (node->child_count > 512u &&
                vitte_sema_add_diagnostic(
                    context,
                    VITTE_SEMA_DIAGNOSTIC_ERROR,
                    VITTE_SEMA_DIAGNOSTIC_INVALID_ASM,
                    node->span,
                    node->id,
                    VITTE_SCOPE_INVALID_SYMBOL_ID,
                    VITTE_SEMA_INVALID_TYPE_ID,
                    VITTE_SEMA_INVALID_TYPE_ID)) {
                (void)snprintf(
                    context->diagnostics[context->diagnostic_count - 1u].help,
                    sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                    "The C17 backend supports at most 512 inline assembly inputs. Split this block into smaller assembly statements.");
            }

            if (template_form) {
                const vitte_ast_node_t *template_node = node->child_count == 1u
                    ? vitte_sema_node(context, node->children[0]) : NULL;
                if (template_node == NULL ||
                    template_node->kind != VITTE_AST_NODE_STRING_LITERAL) {
                    if (vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_ASM,
                            node->span,
                            node->id,
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            VITTE_SEMA_INVALID_TYPE_ID)) {
                        (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                            sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                            "The template form is `asm(\"instructions\");` and accepts exactly one string literal. Use `asm(expr, ...);` for input operands.");
                    }
                } else {
                    (void)vitte_sema_analyze_expression(
                        context, template_node->id, scope_id);
                }
            } else if (node->child_count == 0u) {
                if (vitte_sema_add_diagnostic(
                        context,
                        VITTE_SEMA_DIAGNOSTIC_ERROR,
                        VITTE_SEMA_DIAGNOSTIC_INVALID_ASM,
                        node->span,
                        node->id,
                        VITTE_SCOPE_INVALID_SYMBOL_ID,
                        VITTE_SEMA_INVALID_TYPE_ID,
                        VITTE_SEMA_INVALID_TYPE_ID)) {
                    (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                        sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                        "The operand form is `asm(value, ...);` and needs at least one input operand.");
                }
            } else {
                for (index = 0u; index < node->child_count; ++index) {
                    const vitte_ast_node_t *operand_node =
                        vitte_sema_node(context, node->children[index]);
                    vitte_sema_type_id_t operand_type =
                        vitte_sema_analyze_expression(
                            context, node->children[index], scope_id);
                    const vitte_sema_type_t *operand_record =
                        vitte_sema_type_private(context, operand_type);
                    bool supported = operand_record != NULL &&
                        (vitte_sema_type_is_numeric_private(context, operand_type) ||
                         vitte_sema_type_is_bool_private(context, operand_type) ||
                         operand_record->kind == VITTE_SEMA_TYPE_POINTER ||
                         operand_record->kind == VITTE_SEMA_TYPE_REFERENCE ||
                         vitte_sema_type_is_enum_private(context, operand_type));
                    if (operand_type != context->type_error && !supported &&
                        vitte_sema_add_diagnostic(
                            context,
                            VITTE_SEMA_DIAGNOSTIC_ERROR,
                            VITTE_SEMA_DIAGNOSTIC_INVALID_ASM,
                            operand_node != NULL ? operand_node->span : node->span,
                            operand_node != NULL ? operand_node->id : node->id,
                            VITTE_SCOPE_INVALID_SYMBOL_ID,
                            VITTE_SEMA_INVALID_TYPE_ID,
                            operand_type)) {
                        (void)snprintf(context->diagnostics[context->diagnostic_count - 1u].help,
                            sizeof(context->diagnostics[context->diagnostic_count - 1u].help),
                            "This input cannot use the C17 backend's `\"g\"` assembly constraint. Use a scalar, pointer, reference, or pick value; aggregates, strings, functions, and `void` are unsupported.");
                    }
                }
            }
            return context->type_void;
        }

        case VITTE_AST_NODE_UNSAFE_BLOCK: {
            bool previous_unsafe = context->current_unsafe;

            context->current_unsafe = true;
            for (index = 0u;
                 index < node->child_count;
                 ++index) {
                (void)vitte_sema_analyze_node(
                    context,
                    node->children[index],
                    scope_id);
            }
            context->current_unsafe = previous_unsafe;
            return context->type_void;
        }

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
        case VITTE_AST_NODE_OPTIONAL_TYPE:
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
        case VITTE_AST_NODE_SIZEOF_EXPR:
        case VITTE_AST_NODE_ALIGNOF_EXPR:
            return vitte_sema_analyze_expression(
                context,
                node_id,
                scope_id);

        case VITTE_AST_NODE_PATH:
        case VITTE_AST_NODE_PARAMETER:
        case VITTE_AST_NODE_GENERIC_PARAM:
        case VITTE_AST_NODE_WHERE_CLAUSE:
        case VITTE_AST_NODE_WHERE_PREDICATE:
        case VITTE_AST_NODE_PATTERN:
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
        case VITTE_SEMA_TYPE_F32:
            return "f32";
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
        case VITTE_SEMA_TYPE_OPTIONAL:
            return "optional";
        case VITTE_SEMA_TYPE_ARRAY:
            return "array";
        case VITTE_SEMA_TYPE_SLICE:
            return "slice";
        case VITTE_SEMA_TYPE_RANGE:
            return "range";
        case VITTE_SEMA_TYPE_FUNCTION:
            return "function";
        case VITTE_SEMA_TYPE_NAMED:
            return "named";
        case VITTE_SEMA_TYPE_DYN:
            return "dyn";
        case VITTE_SEMA_TYPE_GENERIC_PARAMETER:
            return "generic_parameter";
        case VITTE_SEMA_TYPE_TASK:
            return "task";
        case VITTE_SEMA_TYPE_NEVER:
            return "never";
        case VITTE_SEMA_TYPE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_sema_type_conversion_kind_name(
    vitte_sema_type_conversion_kind_t kind)
{
    switch (kind) {
        case VITTE_SEMA_TYPE_CONVERSION_INCOMPATIBLE:
            return "incompatible";
        case VITTE_SEMA_TYPE_CONVERSION_EXACT:
            return "exact";
        case VITTE_SEMA_TYPE_CONVERSION_SAFE:
            return "safe";
        case VITTE_SEMA_TYPE_CONVERSION_LOSSY:
            return "lossy";
        case VITTE_SEMA_TYPE_CONVERSION_COUNT:
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
        case VITTE_SEMA_DIAGNOSTIC_INVALID_CAST:
            return "invalid_cast";
        case VITTE_SEMA_DIAGNOSTIC_NOT_A_VALUE:
            return "not_a_value";
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
        case VITTE_SEMA_DIAGNOSTIC_INCOMPLETE_TYPE:
            return "incomplete_type";
        case VITTE_SEMA_DIAGNOSTIC_RECURSIVE_TYPE:
            return "recursive_type";
        case VITTE_SEMA_DIAGNOSTIC_GENERIC_BOUND_UNSATISFIED:
            return "generic_bound_unsatisfied";
        case VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_COUNT:
            return "generic_argument_count";
        case VITTE_SEMA_DIAGNOSTIC_GENERIC_ARGUMENT_KIND:
            return "generic_argument_kind";
        case VITTE_SEMA_DIAGNOSTIC_GENERIC_INFERENCE_CONFLICT:
            return "generic_inference_conflict";
        case VITTE_SEMA_DIAGNOSTIC_TRAIT_NOT_IMPLEMENTED:
            return "trait_not_implemented";
        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_IMPL:
            return "ambiguous_impl";
        case VITTE_SEMA_DIAGNOSTIC_CONFLICTING_IMPL:
            return "conflicting_impl";
        case VITTE_SEMA_DIAGNOSTIC_MISSING_TRAIT_MEMBER:
            return "missing_trait_member";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_TRAIT_MEMBER:
            return "invalid_trait_member";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_IMPL_TARGET:
            return "invalid_impl_target";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_WHERE_PREDICATE:
            return "invalid_where_predicate";
        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_METHOD:
            return "ambiguous_method";
        case VITTE_SEMA_DIAGNOSTIC_NO_MATCHING_PROCEDURE:
            return "no_matching_procedure";
        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_CALL:
            return "ambiguous_call";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_PROCEDURE_MODIFIER:
            return "invalid_procedure_modifier";
        case VITTE_SEMA_DIAGNOSTIC_ASYNC_CONTEXT:
            return "async_context";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_TASK_TYPE:
            return "invalid_task_type";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_PROCEDURE_PARAMETER:
            return "invalid_procedure_parameter";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_PATTERN_BINDINGS:
            return "invalid_pattern_bindings";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_DEFER_CONTROL_FLOW:
            return "invalid_defer_control_flow";
        case VITTE_SEMA_DIAGNOSTIC_OVERLAPPING_PATTERN:
            return "overlapping_pattern";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_REQUIRES:
            return "contract_requires";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_ENSURES:
            return "contract_ensures";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_RESULT:
            return "contract_result";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_CONTRADICTION:
            return "contract_contradiction";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_POSTCONDITION:
            return "contract_postcondition";
        case VITTE_SEMA_DIAGNOSTIC_MISSING_RETURN:
            return "missing_return";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_PRECONDITION:
            return "contract_precondition";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_OVERFLOW:
            return "constant_overflow";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_DIVIDE_BY_ZERO:
            return "constant_divide_by_zero";
        case VITTE_SEMA_DIAGNOSTIC_INDEX_OUT_OF_BOUNDS:
            return "index_out_of_bounds";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_EFFECT:
            return "contract_effect";
        case VITTE_SEMA_DIAGNOSTIC_CONTRACT_OLD_VALUE:
            return "contract_old_value";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_NOT_CONSTANT:
            return "constant_not_constant";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_REMAINDER_BY_ZERO:
            return "constant_remainder_by_zero";
        case VITTE_SEMA_DIAGNOSTIC_MATCH_NON_EXHAUSTIVE:
            return "match_non_exhaustive";
        case VITTE_SEMA_DIAGNOSTIC_MATCH_UNREACHABLE:
            return "match_unreachable";
        case VITTE_SEMA_DIAGNOSTIC_CONSTANT_CYCLE:
            return "constant_cycle";
        case VITTE_SEMA_DIAGNOSTIC_MATCH_TYPE:
            return "match_type";
        case VITTE_SEMA_DIAGNOSTIC_UNSAFE_REQUIRED:
            return "unsafe_required";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_ASM:
            return "invalid_asm";
        case VITTE_SEMA_DIAGNOSTIC_FFI_ABI:
            return "ffi_abi";
        case VITTE_SEMA_DIAGNOSTIC_FFI_TYPE:
            return "ffi_type";
        case VITTE_SEMA_DIAGNOSTIC_REFERENCE_ESCAPE:
            return "reference_escape";
        case VITTE_SEMA_DIAGNOSTIC_BORROW_CONFLICT:
            return "borrow_conflict";
        case VITTE_SEMA_DIAGNOSTIC_BORROWED_MUTATION:
            return "borrowed_mutation";
        case VITTE_SEMA_DIAGNOSTIC_MOVE_BORROWED:
            return "move_borrowed";
        case VITTE_SEMA_DIAGNOSTIC_USE_AFTER_MOVE:
            return "use_after_move";
        case VITTE_SEMA_DIAGNOSTIC_CANNOT_INFER:
            return "cannot_infer";
        case VITTE_SEMA_DIAGNOSTIC_AMBIGUOUS_NAME:
            return "ambiguous_name";
        case VITTE_SEMA_DIAGNOSTIC_DUPLICATE_IMPORT:
            return "duplicate_import";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_SCOPE:
            return "invalid_scope";
        case VITTE_SEMA_DIAGNOSTIC_DUPLICATE_BINDING:
            return "duplicate_binding";
        case VITTE_SEMA_DIAGNOSTIC_INVALID_EXPORT:
            return "invalid_export";
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
        case VITTE_SEMA_DIAGNOSTIC_INVALID_TYPE:
            return "invalid_type";
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

    vitte_sema_release_types_private(context);
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

        vitte_sema_release_types_private(context);
        free(context->types);
        free(context->node_info);
        free(context->diagnostics);
        free(context->explicit_exports);
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

#include "static_checks.inc"

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

    /* The parser snapshot is immutable during analysis. Index explicit
     * exports once instead of scanning the AST for every private name. */
    size_t export_count = 0u;
    for (size_t index = 0u; index < context->parser->node_count; ++index) {
        const vitte_ast_node_t *node = &context->parser->nodes[index];
        if (node->kind == VITTE_AST_NODE_USE_DECL &&
            (node->flags & VITTE_AST_FLAG_PUBLIC) != 0u) {
            ++export_count;
        }
    }
    if (export_count != 0u) {
        size_t export_bytes;
        if (!vitte_sema_size_mul(export_count, sizeof(*context->explicit_exports),
                                 &export_bytes)) {
            return vitte_sema_fail(context, VITTE_SEMA_ERROR_OVERFLOW);
        }
        context->explicit_exports = malloc(export_bytes);
        if (context->explicit_exports == NULL) {
            return vitte_sema_fail(context, VITTE_SEMA_ERROR_OUT_OF_MEMORY);
        }
        for (size_t index = 0u; index < context->parser->node_count; ++index) {
            const vitte_ast_node_t *node = &context->parser->nodes[index];
            if (node->kind == VITTE_AST_NODE_USE_DECL &&
                (node->flags & VITTE_AST_FLAG_PUBLIC) != 0u) {
                context->explicit_exports[context->explicit_export_count++] = node->id;
            }
        }
    }

    (void)vitte_sema_analyze_node(
        context,
        root,
        root_scope);

    /* Literal/range/variant patterns cannot bind a procedure parameter.
     * Older declaration inference tried to compare those pattern nodes with
     * the parameter type before procedure analysis emitted E0608, producing
     * a secondary value/type mismatch at the same token. Keep the dedicated
     * parameter diagnostic and remove only those redundant mismatches. */
    for (size_t node_index = 0u;
         node_index < context->parser->node_count;
         ++node_index) {
        const vitte_ast_node_t *parameter =
            &context->parser->nodes[node_index];
        const vitte_ast_node_t *pattern;
        if (parameter->kind != VITTE_AST_NODE_PARAMETER ||
            parameter->child_count == 0u) {
            continue;
        }
        pattern = vitte_sema_node(context, parameter->children[0]);
        if (vitte_sema_parameter_pattern_refutable_private(
                context, pattern, 0u)) {
            vitte_sema_suppress_pattern_type_cascades_private(
                context, 0u,
                pattern != NULL ? &pattern->span : &parameter->span);
        }
    }

    if (context->last_error == VITTE_SEMA_ERROR_NONE) {
        vitte_sema_check_static_program(context);
    }

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

vitte_sema_type_conversion_kind_t
vitte_sema_type_conversion(
    const vitte_sema_t *context,
    vitte_sema_type_id_t destination,
    vitte_sema_type_id_t source)
{
    if (!vitte_sema_context_valid_private(
            context)) {
        return VITTE_SEMA_TYPE_CONVERSION_INCOMPATIBLE;
    }

    return vitte_sema_type_conversion_private(
        context,
        destination,
        source);
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

        if (type->generic_argument_count != 0u &&
            type->generic_arguments == NULL) {
            goto validation_failure;
        }
        if (type->function_parameter_count != 0u &&
            type->function_parameter_types == NULL) {
            goto validation_failure;
        }
        for (size_t argument_index = 0u;
             argument_index < type->generic_argument_count;
             ++argument_index) {
            if (vitte_sema_type_private(
                    context,
                    type->generic_arguments[argument_index]) == NULL) {
                goto validation_failure;
            }
        }
        for (size_t parameter_index = 0u;
             parameter_index < type->function_parameter_count;
             ++parameter_index) {
            if (vitte_sema_type_private(
                    context,
                    type->function_parameter_types[parameter_index]) ==
                NULL) {
                goto validation_failure;
            }
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
                VITTE_SEMA_DIAGNOSTIC_CODE_COUNT ||
            diagnostic->type_context >=
                VITTE_SEMA_TYPE_CONTEXT_COUNT ||
            (diagnostic->context_symbol_id !=
                 VITTE_SCOPE_INVALID_SYMBOL_ID &&
             vitte_scope_get_symbol(
                 &context->scopes,
                 diagnostic->context_symbol_id) == NULL)) {
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

        hash = vitte_sema_hash_u64(
            hash,
            (uint64_t)type->generic_argument_count);
        for (size_t argument_index = 0u;
             argument_index < type->generic_argument_count;
             ++argument_index) {
            hash = vitte_sema_hash_u64(
                hash,
                (uint64_t)type->generic_arguments[argument_index]);
        }
        hash = vitte_sema_hash_u64(
            hash,
            (uint64_t)type->function_parameter_count);
        for (size_t parameter_index = 0u;
             parameter_index < type->function_parameter_count;
             ++parameter_index) {
            hash = vitte_sema_hash_u64(
                hash,
                (uint64_t)type->function_parameter_types[
                    parameter_index]);
        }
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

        diagnostic = &context->diagnostics[index];
        hash = vitte_sema_hash_u64(
            hash,
            (uint64_t)diagnostic->type_context);
        hash = vitte_sema_hash_u64(
            hash,
            (uint64_t)diagnostic->context_index);
        hash = vitte_sema_hash_u64(
            hash,
            (uint64_t)diagnostic->context_symbol_id);
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
