/*
 * Vitte Compiler
 * src/parser/parser.c
 *
 * Canonical recursive-descent parser implementation.
 *
 * Public contract: parser.h
 *
 * Design goals:
 *   - ISO C17;
 *   - parser.h is the public authority;
 *   - deterministic parsing;
 *   - no filesystem dependency;
 *   - no lexer implementation dependency beyond lexer.h;
 *   - source-span preservation;
 *   - recursive-descent declarations/statements;
 *   - precedence-climbing expression parser;
 *   - bounded recursion;
 *   - bounded AST allocation;
 *   - recoverable syntax errors;
 *   - multiple diagnostics;
 *   - parser statistics;
 *   - structural AST validation;
 *   - deterministic AST fingerprint;
 *   - suitable for bootstrap/compiler tooling.
 */

#include "parser.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Private constants                                                         */
/* ========================================================================= */

#define VITTE_PARSER_PRIVATE_NONE ((size_t)0u)

/* ========================================================================= */
/* Private arithmetic helpers                                                */
/* ========================================================================= */

static bool
vitte_parser_size_add(
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
vitte_parser_size_mul(
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
vitte_parser_u64_add_sat(
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
vitte_parser_hash_bytes(
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
        hash *= VITTE_PARSER_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_parser_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned int shift;

    /*
     * Explicit serialization avoids host-endianness dependence.
     */
    for (shift = 0u; shift < 64u; shift += 8u) {
        hash ^= (value >> shift) & UINT64_C(0xff);
        hash *= VITTE_PARSER_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_parser_hash_bool(
    uint64_t hash,
    bool value)
{
    return vitte_parser_hash_u64(
        hash,
        value ? UINT64_C(1) : UINT64_C(0));
}

/* ========================================================================= */
/* Context validation                                                        */
/* ========================================================================= */

static bool
vitte_parser_context_valid(
    const vitte_parser_t *parser)
{
    if (parser == NULL) {
        return false;
    }

    if (parser->magic != VITTE_PARSER_MAGIC) {
        return false;
    }

    if (parser->state <= VITTE_PARSER_STATE_INVALID ||
        parser->state >= VITTE_PARSER_STATE_DESTROYED) {
        return false;
    }

    if (parser->tokens == NULL &&
        parser->token_count != 0u) {
        return false;
    }

    if (parser->position > parser->token_count) {
        return false;
    }

    if (parser->node_count > parser->node_capacity) {
        return false;
    }

    if (parser->node_count != 0u &&
        parser->nodes == NULL) {
        return false;
    }

    if (parser->diagnostic_count >
        parser->diagnostic_capacity) {
        return false;
    }

    if (parser->diagnostic_count != 0u &&
        parser->diagnostics == NULL) {
        return false;
    }

    return true;
}

static bool
vitte_parser_mutable(
    const vitte_parser_t *parser)
{
    return vitte_parser_context_valid(parser) &&
           (parser->state == VITTE_PARSER_STATE_READY ||
            parser->state == VITTE_PARSER_STATE_PARSING);
}

static bool
vitte_parser_fail(
    vitte_parser_t *parser,
    vitte_parser_error_t error)
{
    if (parser != NULL &&
        parser->magic == VITTE_PARSER_MAGIC) {
        parser->last_error = error;
    }

    return false;
}

static vitte_ast_node_id_t
vitte_parser_fail_node(
    vitte_parser_t *parser,
    vitte_parser_error_t error)
{
    (void)vitte_parser_fail(parser, error);

    return VITTE_AST_INVALID_ID;
}

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

static bool
vitte_parser_reserve_raw(
    vitte_parser_t *parser,
    void **storage,
    size_t *capacity,
    size_t required,
    size_t element_size)
{
    size_t next_capacity;
    size_t bytes;
    void *replacement;

    if (parser == NULL ||
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
        next_capacity =
            VITTE_PARSER_DEFAULT_INITIAL_CAPACITY;
    }

    while (next_capacity < required) {
        size_t doubled;

        if (!vitte_parser_size_mul(
                next_capacity,
                (size_t)2u,
                &doubled) ||
            doubled <= next_capacity) {
            next_capacity = required;
            break;
        }

        next_capacity = doubled;
    }

    if (!vitte_parser_size_mul(
            next_capacity,
            element_size,
            &bytes)) {
        return vitte_parser_fail(
            parser,
            VITTE_PARSER_ERROR_OVERFLOW);
    }

    replacement = realloc(*storage, bytes);

    if (replacement == NULL) {
        parser->stats.allocation_failures =
            vitte_parser_u64_add_sat(
                parser->stats.allocation_failures,
                UINT64_C(1));

        return vitte_parser_fail(
            parser,
            VITTE_PARSER_ERROR_OUT_OF_MEMORY);
    }

    if (*storage == NULL) {
        parser->stats.allocations =
            vitte_parser_u64_add_sat(
                parser->stats.allocations,
                UINT64_C(1));
    } else {
        parser->stats.reallocations =
            vitte_parser_u64_add_sat(
                parser->stats.reallocations,
                UINT64_C(1));
    }

    *storage = replacement;
    *capacity = next_capacity;

    return true;
}

static bool
vitte_parser_reserve_nodes(
    vitte_parser_t *parser,
    size_t required)
{
    return vitte_parser_reserve_raw(
        parser,
        (void **)&parser->nodes,
        &parser->node_capacity,
        required,
        sizeof(*parser->nodes));
}

static bool
vitte_parser_reserve_diagnostics(
    vitte_parser_t *parser,
    size_t required)
{
    return vitte_parser_reserve_raw(
        parser,
        (void **)&parser->diagnostics,
        &parser->diagnostic_capacity,
        required,
        sizeof(*parser->diagnostics));
}

static bool
vitte_parser_reserve_node_ids(
    vitte_parser_t *parser,
    vitte_ast_node_id_t **storage,
    size_t *capacity,
    size_t required)
{
    return vitte_parser_reserve_raw(
        parser,
        (void **)storage,
        capacity,
        required,
        sizeof(**storage));
}

/* ========================================================================= */
/* Token access                                                              */
/* ========================================================================= */

static const vitte_token_t *
vitte_parser_token_at_private(
    const vitte_parser_t *parser,
    size_t position)
{
    if (parser == NULL ||
        parser->tokens == NULL ||
        position >= parser->token_count) {
        return NULL;
    }

    return &parser->tokens[position];
}

static const vitte_token_t *
vitte_parser_current(
    const vitte_parser_t *parser)
{
    return vitte_parser_token_at_private(
        parser,
        parser != NULL ? parser->position : 0u);
}

static const vitte_token_t *
vitte_parser_previous(
    const vitte_parser_t *parser)
{
    if (parser == NULL ||
        parser->position == 0u) {
        return NULL;
    }

    return vitte_parser_token_at_private(
        parser,
        parser->position - 1u);
}

static const vitte_token_t *
vitte_parser_peek(
    const vitte_parser_t *parser,
    size_t distance)
{
    size_t position;

    if (parser == NULL) {
        return NULL;
    }

    if (!vitte_parser_size_add(
            parser->position,
            distance,
            &position)) {
        return NULL;
    }

    return vitte_parser_token_at_private(
        parser,
        position);
}

static bool
vitte_parser_at_end(
    const vitte_parser_t *parser)
{
    const vitte_token_t *token;

    token = vitte_parser_current(parser);

    return token == NULL ||
           token->kind == VITTE_TOKEN_EOF;
}

static bool
vitte_parser_check(
    const vitte_parser_t *parser,
    vitte_token_kind_t kind)
{
    const vitte_token_t *token;

    token = vitte_parser_current(parser);

    return token != NULL &&
           token->kind == kind;
}

static bool
vitte_parser_check_next(
    const vitte_parser_t *parser,
    vitte_token_kind_t kind)
{
    const vitte_token_t *token;

    token = vitte_parser_peek(parser, 1u);

    return token != NULL &&
           token->kind == kind;
}

static const vitte_token_t *
vitte_parser_advance(
    vitte_parser_t *parser)
{
    const vitte_token_t *token;

    if (parser == NULL) {
        return NULL;
    }

    token = vitte_parser_current(parser);

    if (token != NULL &&
        token->kind != VITTE_TOKEN_EOF &&
        parser->position < parser->token_count) {
        ++parser->position;
    }

    if (token != NULL) {
        parser->stats.tokens_consumed =
            vitte_parser_u64_add_sat(
                parser->stats.tokens_consumed,
                UINT64_C(1));
    }

    return token;
}

static bool
vitte_parser_match(
    vitte_parser_t *parser,
    vitte_token_kind_t kind)
{
    if (!vitte_parser_check(parser, kind)) {
        return false;
    }

    (void)vitte_parser_advance(parser);

    return true;
}

/* ========================================================================= */
/* Spans                                                                     */
/* ========================================================================= */

static vitte_parser_span_t
vitte_parser_span_invalid_private(void)
{
    vitte_parser_span_t span;

    memset(&span, 0, sizeof(span));

    span.valid = false;

    return span;
}

static vitte_parser_span_t
vitte_parser_span_from_token(
    const vitte_token_t *token)
{
    vitte_parser_span_t span;

    span = vitte_parser_span_invalid_private();

    if (token == NULL ||
        !token->span.valid) {
        return span;
    }

    span.file_id = token->span.file_id;
    span.begin = token->span.begin;
    span.end = token->span.end;
    span.line = token->line;
    span.column = token->column;
    span.valid = true;

    return span;
}

static vitte_parser_span_t
vitte_parser_span_join(
    vitte_parser_span_t left,
    vitte_parser_span_t right)
{
    vitte_parser_span_t span;

    if (!left.valid) {
        return right;
    }

    if (!right.valid) {
        return left;
    }

    span = left;

    if (right.begin < span.begin) {
        span.begin = right.begin;
        span.line = right.line;
        span.column = right.column;
    }

    if (right.end > span.end) {
        span.end = right.end;
    }

    span.valid = true;

    return span;
}

static vitte_parser_span_t
vitte_parser_span_from_range(
    const vitte_parser_t *parser,
    size_t first,
    size_t last_exclusive)
{
    const vitte_token_t *first_token;
    const vitte_token_t *last_token;
    vitte_parser_span_t left;
    vitte_parser_span_t right;

    if (parser == NULL ||
        first >= parser->token_count ||
        last_exclusive <= first) {
        return vitte_parser_span_invalid_private();
    }

    first_token =
        vitte_parser_token_at_private(
            parser,
            first);

    last_token =
        vitte_parser_token_at_private(
            parser,
            last_exclusive - 1u);

    left = vitte_parser_span_from_token(first_token);
    right = vitte_parser_span_from_token(last_token);

    return vitte_parser_span_join(left, right);
}

/* ========================================================================= */
/* Diagnostics                                                               */
/* ========================================================================= */

static bool
vitte_parser_emit_diagnostic(
    vitte_parser_t *parser,
    vitte_parser_diagnostic_kind_t kind,
    vitte_parser_error_t error,
    vitte_parser_span_t span,
    vitte_token_kind_t expected,
    vitte_token_kind_t found)
{
    size_t required;
    vitte_parser_diagnostic_t *diagnostic;

    if (parser == NULL ||
        parser->magic != VITTE_PARSER_MAGIC) {
        return false;
    }

    if (parser->diagnostic_count >=
        parser->max_diagnostics) {
        parser->last_error =
            VITTE_PARSER_ERROR_DIAGNOSTIC_LIMIT;

        return false;
    }

    if (!vitte_parser_size_add(
            parser->diagnostic_count,
            1u,
            &required)) {
        return vitte_parser_fail(
            parser,
            VITTE_PARSER_ERROR_OVERFLOW);
    }

    if (!vitte_parser_reserve_diagnostics(
            parser,
            required)) {
        return false;
    }

    diagnostic =
        &parser->diagnostics[
            parser->diagnostic_count];

    memset(diagnostic, 0, sizeof(*diagnostic));

    diagnostic->kind = kind;
    diagnostic->error = error;
    diagnostic->span = span;
    diagnostic->expected = expected;
    diagnostic->found = found;

    parser->diagnostic_count = required;

    parser->stats.diagnostics =
        vitte_parser_u64_add_sat(
            parser->stats.diagnostics,
            UINT64_C(1));

    if (kind == VITTE_PARSER_DIAGNOSTIC_ERROR) {
        parser->stats.errors =
            vitte_parser_u64_add_sat(
                parser->stats.errors,
                UINT64_C(1));
    }

    parser->last_error = error;

    return true;
}

static bool
vitte_parser_error_here(
    vitte_parser_t *parser,
    vitte_parser_error_t error,
    vitte_token_kind_t expected)
{
    const vitte_token_t *token;
    vitte_parser_span_t span;
    vitte_token_kind_t found;

    token = vitte_parser_current(parser);

    if (token != NULL) {
        span = vitte_parser_span_from_token(token);
        found = token->kind;
    } else {
        span = vitte_parser_span_invalid_private();
        found = VITTE_TOKEN_INVALID;
    }

    (void)vitte_parser_emit_diagnostic(
        parser,
        VITTE_PARSER_DIAGNOSTIC_ERROR,
        error,
        span,
        expected,
        found);

    return false;
}

static const vitte_token_t *
vitte_parser_expect(
    vitte_parser_t *parser,
    vitte_token_kind_t kind)
{
    if (vitte_parser_check(parser, kind)) {
        return vitte_parser_advance(parser);
    }

    (void)vitte_parser_error_here(
        parser,
        VITTE_PARSER_ERROR_EXPECTED_TOKEN,
        kind);

    return NULL;
}

/* ========================================================================= */
/* Error recovery                                                            */
/* ========================================================================= */

static bool
vitte_parser_is_declaration_start(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_KW_PUB:
        case VITTE_TOKEN_KW_SPACE:
        case VITTE_TOKEN_KW_USE:
        case VITTE_TOKEN_KW_CONST:
        case VITTE_TOKEN_KW_STATIC:
        case VITTE_TOKEN_KW_TYPE:
        case VITTE_TOKEN_KW_OPAQUE:
        case VITTE_TOKEN_KW_FORM:
        case VITTE_TOKEN_KW_PICK:
        case VITTE_TOKEN_KW_TRAIT:
        case VITTE_TOKEN_KW_IMPL:
        case VITTE_TOKEN_KW_EXTERN:
        case VITTE_TOKEN_KW_PROC:
        case VITTE_TOKEN_KW_MACRO:
        case VITTE_TOKEN_KW_TEST:
        case VITTE_TOKEN_KW_ASYNC:
        case VITTE_TOKEN_KW_UNSAFE:
            return true;

        default:
            return false;
    }
}

static bool
vitte_parser_is_statement_start(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_KW_LET:
        case VITTE_TOKEN_KW_RETURN:
        case VITTE_TOKEN_KW_DEFER:
        case VITTE_TOKEN_KW_IF:
        case VITTE_TOKEN_KW_WHILE:
        case VITTE_TOKEN_KW_LOOP:
        case VITTE_TOKEN_KW_FOR:
        case VITTE_TOKEN_KW_BREAK:
        case VITTE_TOKEN_KW_CONTINUE:
        case VITTE_TOKEN_KW_MATCH:
        case VITTE_TOKEN_KW_UNSAFE:
        case VITTE_TOKEN_KW_ASM:
        case VITTE_TOKEN_KW_ASSERT:
        case VITTE_TOKEN_LEFT_BRACE:
            return true;

        default:
            return false;
    }
}

static void
vitte_parser_synchronize(
    vitte_parser_t *parser)
{
    size_t consumed;

    if (parser == NULL) {
        return;
    }

    consumed = 0u;

    while (!vitte_parser_at_end(parser) &&
           consumed < parser->max_recovery_tokens) {
        const vitte_token_t *previous;
        const vitte_token_t *current;

        previous = vitte_parser_previous(parser);
        current = vitte_parser_current(parser);

        if (previous != NULL &&
            previous->kind ==
                VITTE_TOKEN_SEMICOLON) {
            return;
        }

        if (current != NULL &&
            (vitte_parser_is_declaration_start(
                 current->kind) ||
             vitte_parser_is_statement_start(
                 current->kind) ||
             current->kind ==
                 VITTE_TOKEN_RIGHT_BRACE)) {
            return;
        }

        (void)vitte_parser_advance(parser);
        ++consumed;
    }

    parser->stats.recoveries =
        vitte_parser_u64_add_sat(
            parser->stats.recoveries,
            UINT64_C(1));
}

/* ========================================================================= */
/* AST storage                                                               */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_add_node(
    vitte_parser_t *parser,
    vitte_ast_node_kind_t kind,
    vitte_parser_span_t span)
{
    size_t required;
    vitte_ast_node_t *node;

    if (!vitte_parser_mutable(parser)) {
        return vitte_parser_fail_node(
            parser,
            VITTE_PARSER_ERROR_INVALID_STATE);
    }

    if (kind <= VITTE_AST_NODE_INVALID ||
        kind >= VITTE_AST_NODE_COUNT) {
        return vitte_parser_fail_node(
            parser,
            VITTE_PARSER_ERROR_INVALID_NODE);
    }

    if (parser->node_count >=
        parser->max_nodes) {
        return vitte_parser_fail_node(
            parser,
            VITTE_PARSER_ERROR_NODE_LIMIT);
    }

    if (!vitte_parser_size_add(
            parser->node_count,
            1u,
            &required)) {
        return vitte_parser_fail_node(
            parser,
            VITTE_PARSER_ERROR_OVERFLOW);
    }

    if (!vitte_parser_reserve_nodes(
            parser,
            required)) {
        return VITTE_AST_INVALID_ID;
    }

    node = &parser->nodes[parser->node_count];

    memset(node, 0, sizeof(*node));

    node->id =
        (vitte_ast_node_id_t)required;

    node->kind = kind;
    node->span = span;

    parser->node_count = required;

    parser->stats.nodes_created =
        vitte_parser_u64_add_sat(
            parser->stats.nodes_created,
            UINT64_C(1));

    return node->id;
}

const vitte_ast_node_t *
vitte_parser_get_node(
    const vitte_parser_t *parser,
    vitte_ast_node_id_t id)
{
    size_t index;

    if (!vitte_parser_context_valid(parser) ||
        id == VITTE_AST_INVALID_ID ||
        id > (vitte_ast_node_id_t)SIZE_MAX) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= parser->node_count ||
        parser->nodes[index].id != id) {
        return NULL;
    }

    return &parser->nodes[index];
}

vitte_ast_node_t *
vitte_parser_get_node_mut(
    vitte_parser_t *parser,
    vitte_ast_node_id_t id)
{
    size_t index;

    if (!vitte_parser_mutable(parser) ||
        id == VITTE_AST_INVALID_ID ||
        id > (vitte_ast_node_id_t)SIZE_MAX) {
        return NULL;
    }

    index = (size_t)(id - UINT64_C(1));

    if (index >= parser->node_count ||
        parser->nodes[index].id != id) {
        return NULL;
    }

    return &parser->nodes[index];
}

static bool
vitte_parser_node_append_child(
    vitte_parser_t *parser,
    vitte_ast_node_id_t parent_id,
    vitte_ast_node_id_t child_id)
{
    vitte_ast_node_t *parent;
    size_t required;

    if (child_id == VITTE_AST_INVALID_ID) {
        return false;
    }

    parent =
        vitte_parser_get_node_mut(
            parser,
            parent_id);

    if (parent == NULL) {
        return false;
    }

    if (!vitte_parser_size_add(
            parent->child_count,
            1u,
            &required)) {
        return vitte_parser_fail(
            parser,
            VITTE_PARSER_ERROR_OVERFLOW);
    }

    if (!vitte_parser_reserve_node_ids(
            parser,
            &parent->children,
            &parent->child_capacity,
            required)) {
        return false;
    }

    parent->children[parent->child_count] =
        child_id;

    parent->child_count = required;

    return true;
}

/* ========================================================================= */
/* Recursion guard                                                           */
/* ========================================================================= */

static bool
vitte_parser_enter_recursion(
    vitte_parser_t *parser)
{
    if (parser == NULL) {
        return false;
    }

    if (parser->recursion_depth >=
        parser->max_recursion_depth) {
        (void)vitte_parser_error_here(
            parser,
            VITTE_PARSER_ERROR_RECURSION_LIMIT,
            VITTE_TOKEN_INVALID);

        return false;
    }

    ++parser->recursion_depth;

    if ((uint64_t)parser->recursion_depth >
        parser->stats.max_recursion_depth) {
        parser->stats.max_recursion_depth =
            (uint64_t)parser->recursion_depth;
    }

    return true;
}

static void
vitte_parser_leave_recursion(
    vitte_parser_t *parser)
{
    if (parser != NULL &&
        parser->recursion_depth != 0u) {
        --parser->recursion_depth;
    }
}

/* ========================================================================= */
/* Forward declarations                                                      */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_declaration(
    vitte_parser_t *parser);

static vitte_ast_node_id_t
vitte_parser_parse_statement(
    vitte_parser_t *parser);

static vitte_ast_node_id_t
vitte_parser_parse_expression(
    vitte_parser_t *parser);

static vitte_ast_node_id_t
vitte_parser_parse_type_expression(
    vitte_parser_t *parser);

static vitte_ast_node_id_t
vitte_parser_parse_block(
    vitte_parser_t *parser);

static vitte_ast_node_id_t
vitte_parser_parse_space_body(
    vitte_parser_t *parser);

static vitte_ast_node_id_t
vitte_parser_parse_block_expression(
    vitte_parser_t *parser);

static vitte_ast_node_id_t
vitte_parser_parse_if_expression(
    vitte_parser_t *parser);

/* ========================================================================= */
/* Identifier / path parsing                                                 */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_identifier_node(
    vitte_parser_t *parser)
{
    const vitte_token_t *token;
    vitte_ast_node_id_t id;
    vitte_ast_node_t *node;

    token = vitte_parser_current(parser);

    if (token != NULL &&
        token->kind == VITTE_TOKEN_KW_NULL &&
        vitte_parser_check_next(
            parser,
            VITTE_TOKEN_LEFT_PAREN)) {
        (void)vitte_parser_advance(parser);
    } else {
        token =
            vitte_parser_expect(
                parser,
                VITTE_TOKEN_IDENTIFIER);
    }

    if (token == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    id =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_IDENTIFIER,
            vitte_parser_span_from_token(token));

    node = vitte_parser_get_node_mut(parser, id);

    if (node == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    node->token_index =
        (size_t)(token - parser->tokens);

    return id;
}

static vitte_ast_node_id_t
vitte_parser_parse_path(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t path_id;
    vitte_ast_node_id_t segment_id;
    vitte_ast_node_t *path;

    begin = parser->position;

    segment_id =
        vitte_parser_parse_identifier_node(parser);

    if (segment_id == VITTE_AST_INVALID_ID) {
        return VITTE_AST_INVALID_ID;
    }

    path_id =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_PATH,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (path_id == VITTE_AST_INVALID_ID) {
        return VITTE_AST_INVALID_ID;
    }

    if (!vitte_parser_node_append_child(
            parser,
            path_id,
            segment_id)) {
        return VITTE_AST_INVALID_ID;
    }

    while ((vitte_parser_check(
                parser,
                VITTE_TOKEN_COLON_COLON) &&
            vitte_parser_peek(parser, 1u) != NULL &&
            vitte_parser_peek(parser, 1u)->kind ==
                VITTE_TOKEN_IDENTIFIER) ||
           (vitte_parser_check(
                parser,
                VITTE_TOKEN_SLASH) &&
            vitte_parser_peek(parser, 1u) != NULL &&
            vitte_parser_peek(parser, 1u)->kind ==
                VITTE_TOKEN_IDENTIFIER)) {
        (void)vitte_parser_advance(parser);

        segment_id =
            vitte_parser_parse_identifier_node(
                parser);

        if (segment_id == VITTE_AST_INVALID_ID) {
            return VITTE_AST_INVALID_ID;
        }

        if (!vitte_parser_node_append_child(
                parser,
                path_id,
                segment_id)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    path = vitte_parser_get_node_mut(parser, path_id);

    if (path != NULL) {
        path->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return path_id;
}

/* ========================================================================= */
/* Literal parsing                                                           */
/* ========================================================================= */

static vitte_ast_node_kind_t
vitte_parser_literal_node_kind(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_INTEGER_LITERAL:
            return VITTE_AST_NODE_INTEGER_LITERAL;

        case VITTE_TOKEN_FLOAT_LITERAL:
            return VITTE_AST_NODE_FLOAT_LITERAL;

        case VITTE_TOKEN_STRING_LITERAL:
            return VITTE_AST_NODE_STRING_LITERAL;

        case VITTE_TOKEN_CHARACTER_LITERAL:
            return VITTE_AST_NODE_CHARACTER_LITERAL;

        case VITTE_TOKEN_KW_TRUE:
        case VITTE_TOKEN_KW_FALSE:
            return VITTE_AST_NODE_BOOL_LITERAL;

        case VITTE_TOKEN_KW_NULL:
            return VITTE_AST_NODE_NULL_LITERAL;

        default:
            return VITTE_AST_NODE_INVALID;
    }
}

static vitte_ast_node_id_t
vitte_parser_parse_literal(
    vitte_parser_t *parser)
{
    const vitte_token_t *token;
    vitte_ast_node_kind_t kind;
    vitte_ast_node_id_t id;
    vitte_ast_node_t *node;

    token = vitte_parser_current(parser);

    if (token == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    kind = vitte_parser_literal_node_kind(
        token->kind);

    if (kind == VITTE_AST_NODE_INVALID) {
        return VITTE_AST_INVALID_ID;
    }

    (void)vitte_parser_advance(parser);

    id =
        vitte_parser_add_node(
            parser,
            kind,
            vitte_parser_span_from_token(token));

    node = vitte_parser_get_node_mut(parser, id);

    if (node != NULL) {
        node->token_index =
            (size_t)(token - parser->tokens);
    }

    return id;
}

/* ========================================================================= */
/* Expression precedence                                                     */
/* ========================================================================= */

static int
vitte_parser_binary_precedence(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_EQUAL:
        case VITTE_TOKEN_PLUS_EQUAL:
        case VITTE_TOKEN_MINUS_EQUAL:
        case VITTE_TOKEN_STAR_EQUAL:
        case VITTE_TOKEN_SLASH_EQUAL:
        case VITTE_TOKEN_PERCENT_EQUAL:
        case VITTE_TOKEN_AMP_EQUAL:
        case VITTE_TOKEN_PIPE_EQUAL:
        case VITTE_TOKEN_CARET_EQUAL:
        case VITTE_TOKEN_SHIFT_LEFT_EQUAL:
        case VITTE_TOKEN_SHIFT_RIGHT_EQUAL:
            return 1;

        case VITTE_TOKEN_QUESTION_QUESTION:
            return 2;

        case VITTE_TOKEN_KW_OR:
        case VITTE_TOKEN_PIPE_PIPE:
            return 3;

        case VITTE_TOKEN_KW_AND:
        case VITTE_TOKEN_AMP_AMP:
            return 4;

        case VITTE_TOKEN_PIPE:
            return 5;

        case VITTE_TOKEN_CARET:
            return 6;

        case VITTE_TOKEN_AMP:
            return 7;

        case VITTE_TOKEN_EQUAL_EQUAL:
        case VITTE_TOKEN_BANG_EQUAL:
            return 8;

        case VITTE_TOKEN_LESS:
        case VITTE_TOKEN_LESS_EQUAL:
        case VITTE_TOKEN_GREATER:
        case VITTE_TOKEN_GREATER_EQUAL:
            return 9;

        case VITTE_TOKEN_SHIFT_LEFT:
        case VITTE_TOKEN_SHIFT_RIGHT:
            return 10;

        case VITTE_TOKEN_PLUS:
        case VITTE_TOKEN_MINUS:
            return 11;

        case VITTE_TOKEN_STAR:
        case VITTE_TOKEN_SLASH:
        case VITTE_TOKEN_PERCENT:
            return 12;

        case VITTE_TOKEN_DOT_DOT:
        case VITTE_TOKEN_DOT_DOT_EQUAL:
            return 13;

        default:
            return 0;
    }
}

static bool
vitte_parser_binary_right_associative(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_EQUAL:
        case VITTE_TOKEN_PLUS_EQUAL:
        case VITTE_TOKEN_MINUS_EQUAL:
        case VITTE_TOKEN_STAR_EQUAL:
        case VITTE_TOKEN_SLASH_EQUAL:
        case VITTE_TOKEN_PERCENT_EQUAL:
        case VITTE_TOKEN_AMP_EQUAL:
        case VITTE_TOKEN_PIPE_EQUAL:
        case VITTE_TOKEN_CARET_EQUAL:
        case VITTE_TOKEN_SHIFT_LEFT_EQUAL:
        case VITTE_TOKEN_SHIFT_RIGHT_EQUAL:
        case VITTE_TOKEN_QUESTION_QUESTION:
            return true;

        default:
            return false;
    }
}

/* ========================================================================= */
/* Primary expressions                                                       */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_primary(
    vitte_parser_t *parser)
{
    const vitte_token_t *token;
    vitte_ast_node_id_t id;
    size_t begin;

    token = vitte_parser_current(parser);

    if (token == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_literal_node_kind(
            token->kind) !=
        VITTE_AST_NODE_INVALID) {
        return vitte_parser_parse_literal(parser);
    }

    if (token->kind == VITTE_TOKEN_KW_NULL &&
        vitte_parser_check_next(
            parser,
            VITTE_TOKEN_LEFT_PAREN)) {
        return vitte_parser_parse_path(parser);
    }

    if (token->kind == VITTE_TOKEN_IDENTIFIER ||
        token->kind == VITTE_TOKEN_KW_SELF) {
        if (token->kind == VITTE_TOKEN_KW_SELF) {
            vitte_ast_node_t *node;

            (void)vitte_parser_advance(parser);

            id =
                vitte_parser_add_node(
                    parser,
                    VITTE_AST_NODE_SELF_EXPR,
                    vitte_parser_span_from_token(
                        token));

            node =
                vitte_parser_get_node_mut(
                    parser,
                    id);

            if (node != NULL) {
                node->token_index =
                    (size_t)(token -
                             parser->tokens);
            }

            return id;
        }

        return vitte_parser_parse_path(parser);
    }

    if (token->kind == VITTE_TOKEN_KW_IF) {
        return vitte_parser_parse_if_expression(parser);
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_LEFT_BRACKET)) {
        vitte_ast_node_t *node;

        begin = parser->position - 1u;
        id = vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_ARRAY_EXPR,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));
        if (id == VITTE_AST_INVALID_ID) {
            return id;
        }

        while (!vitte_parser_at_end(parser) &&
               !vitte_parser_check(parser, VITTE_TOKEN_RIGHT_BRACKET)) {
            vitte_ast_node_id_t item;

            item = vitte_parser_parse_expression(parser);
            if (item == VITTE_AST_INVALID_ID ||
                !vitte_parser_node_append_child(parser, id, item)) {
                return VITTE_AST_INVALID_ID;
            }

            if (!vitte_parser_match(parser, VITTE_TOKEN_COMMA)) {
                break;
            }
        }

        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_RIGHT_BRACKET) == NULL) {
            return VITTE_AST_INVALID_ID;
        }

        node = vitte_parser_get_node_mut(parser, id);
        if (node != NULL) {
            node->span = vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
        }

        return id;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_LEFT_PAREN)) {
        vitte_ast_node_id_t expression;

        begin = parser->position - 1u;

        expression =
            vitte_parser_parse_expression(parser);

        if (expression == VITTE_AST_INVALID_ID) {
            return VITTE_AST_INVALID_ID;
        }

        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_RIGHT_PAREN) == NULL) {
            return VITTE_AST_INVALID_ID;
        }

        id =
            vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_GROUP_EXPR,
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position));

        if (id == VITTE_AST_INVALID_ID) {
            return id;
        }

        if (!vitte_parser_node_append_child(
                parser,
                id,
                expression)) {
            return VITTE_AST_INVALID_ID;
        }

        return id;
    }

    if (vitte_parser_check(
            parser,
            VITTE_TOKEN_LEFT_BRACE)) {
        return vitte_parser_parse_block(parser);
    }

    (void)vitte_parser_error_here(
        parser,
        VITTE_PARSER_ERROR_EXPECTED_EXPRESSION,
        VITTE_TOKEN_INVALID);

    return VITTE_AST_INVALID_ID;
}

/* ========================================================================= */
/* Postfix expressions                                                       */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_postfix(
    vitte_parser_t *parser)
{
    vitte_ast_node_id_t expression;

    expression = vitte_parser_parse_primary(parser);

    if (expression == VITTE_AST_INVALID_ID) {
        return expression;
    }

    for (;;) {
        if (vitte_parser_check(
                parser,
                VITTE_TOKEN_LEFT_BRACE)) {
            const vitte_ast_node_t *type_path;
            vitte_ast_node_id_t form;
            size_t begin;

            type_path =
                vitte_parser_get_node(parser, expression);
            if (type_path == NULL ||
                (type_path->kind != VITTE_AST_NODE_PATH &&
                 type_path->kind != VITTE_AST_NODE_IDENTIFIER) ||
                parser->position + 2u >= parser->token_count ||
                parser->tokens[parser->position + 1u].kind !=
                    VITTE_TOKEN_IDENTIFIER ||
                parser->tokens[parser->position + 2u].kind !=
                    VITTE_TOKEN_COLON) {
                break;
            }

            begin = type_path->span.begin;
            (void)vitte_parser_advance(parser);
            form = vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_FORM_EXPR,
                type_path->span);
            if (form == VITTE_AST_INVALID_ID ||
                !vitte_parser_node_append_child(
                    parser,
                    form,
                    expression)) {
                return VITTE_AST_INVALID_ID;
            }

            while (!vitte_parser_at_end(parser) &&
                   !vitte_parser_check(
                       parser,
                       VITTE_TOKEN_RIGHT_BRACE)) {
                vitte_ast_node_id_t field_name;
                vitte_ast_node_id_t value;
                vitte_ast_node_id_t field;

                field_name =
                    vitte_parser_parse_identifier_node(parser);
                if (field_name == VITTE_AST_INVALID_ID ||
                    vitte_parser_expect(
                        parser,
                        VITTE_TOKEN_COLON) == NULL) {
                    return VITTE_AST_INVALID_ID;
                }
                value = vitte_parser_parse_expression(parser);
                if (value == VITTE_AST_INVALID_ID) {
                    return value;
                }
                field = vitte_parser_add_node(
                    parser,
                    VITTE_AST_NODE_FIELD_INIT,
                    vitte_parser_get_node(parser, field_name)->span);
                if (field == VITTE_AST_INVALID_ID ||
                    !vitte_parser_node_append_child(
                        parser,
                        field,
                        field_name) ||
                    !vitte_parser_node_append_child(
                        parser,
                        field,
                        value) ||
                    !vitte_parser_node_append_child(
                        parser,
                        form,
                        field)) {
                    return VITTE_AST_INVALID_ID;
                }
                if (vitte_parser_check(
                        parser,
                        VITTE_TOKEN_RIGHT_BRACE)) {
                    break;
                }
                if (!vitte_parser_match(
                        parser,
                        VITTE_TOKEN_COMMA)) {
                    (void)vitte_parser_error_here(
                        parser,
                        VITTE_PARSER_ERROR_EXPECTED_TOKEN,
                        VITTE_TOKEN_COMMA);
                    return VITTE_AST_INVALID_ID;
                }
            }
            if (vitte_parser_expect(
                    parser,
                    VITTE_TOKEN_RIGHT_BRACE) == NULL) {
                return VITTE_AST_INVALID_ID;
            }
            {
                vitte_ast_node_t *node;
                node = vitte_parser_get_node_mut(parser, form);
                if (node != NULL) {
                    node->span = vitte_parser_span_from_range(
                        parser,
                        begin,
                        parser->position);
                }
            }
            expression = form;
            continue;
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_LEFT_PAREN)) {
            vitte_ast_node_id_t call;
            vitte_ast_node_t *node;
            vitte_parser_span_t span;

            span =
                vitte_parser_get_node(
                    parser,
                    expression)->span;

            call =
                vitte_parser_add_node(
                    parser,
                    VITTE_AST_NODE_CALL_EXPR,
                    span);

            if (call == VITTE_AST_INVALID_ID) {
                return call;
            }

            if (!vitte_parser_node_append_child(
                    parser,
                    call,
                    expression)) {
                return VITTE_AST_INVALID_ID;
            }

            if (!vitte_parser_check(
                    parser,
                    VITTE_TOKEN_RIGHT_PAREN)) {
                for (;;) {
                    vitte_ast_node_id_t argument;

                    argument =
                        vitte_parser_parse_expression(
                            parser);

                    if (argument ==
                        VITTE_AST_INVALID_ID) {
                        return argument;
                    }

                    if (!vitte_parser_node_append_child(
                            parser,
                            call,
                            argument)) {
                        return VITTE_AST_INVALID_ID;
                    }

                    if (!vitte_parser_match(
                            parser,
                            VITTE_TOKEN_COMMA)) {
                        break;
                    }

                    if (vitte_parser_check(
                            parser,
                            VITTE_TOKEN_RIGHT_PAREN)) {
                        break;
                    }
                }
            }

            if (vitte_parser_expect(
                    parser,
                    VITTE_TOKEN_RIGHT_PAREN) == NULL) {
                return VITTE_AST_INVALID_ID;
            }

            node =
                vitte_parser_get_node_mut(
                    parser,
                    call);

            if (node != NULL) {
                const vitte_token_t *end;

                end = vitte_parser_previous(parser);

                node->span =
                    vitte_parser_span_join(
                        node->span,
                        vitte_parser_span_from_token(
                            end));
            }

            expression = call;
            continue;
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_LEFT_BRACKET)) {
            vitte_ast_node_id_t index_expression;
            vitte_ast_node_id_t index_node;
            vitte_ast_node_t *node;
            vitte_parser_span_t start_span;

            start_span =
                vitte_parser_get_node(
                    parser,
                    expression)->span;

            index_expression =
                vitte_parser_parse_expression(
                    parser);

            if (index_expression ==
                VITTE_AST_INVALID_ID) {
                return index_expression;
            }

            if (vitte_parser_expect(
                    parser,
                    VITTE_TOKEN_RIGHT_BRACKET) ==
                NULL) {
                return VITTE_AST_INVALID_ID;
            }

            index_node =
                vitte_parser_add_node(
                    parser,
                    VITTE_AST_NODE_INDEX_EXPR,
                    start_span);

            if (index_node ==
                VITTE_AST_INVALID_ID) {
                return index_node;
            }

            if (!vitte_parser_node_append_child(
                    parser,
                    index_node,
                    expression) ||
                !vitte_parser_node_append_child(
                    parser,
                    index_node,
                    index_expression)) {
                return VITTE_AST_INVALID_ID;
            }

            node =
                vitte_parser_get_node_mut(
                    parser,
                    index_node);

            if (node != NULL) {
                node->span =
                    vitte_parser_span_join(
                        start_span,
                        vitte_parser_span_from_token(
                            vitte_parser_previous(
                                parser)));
            }

            expression = index_node;
            continue;
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_DOT)) {
            vitte_ast_node_id_t member;
            vitte_ast_node_id_t member_name;
            vitte_ast_node_t *node;
            vitte_parser_span_t start_span;

            start_span =
                vitte_parser_get_node(
                    parser,
                    expression)->span;

            member_name =
                vitte_parser_parse_identifier_node(
                    parser);

            if (member_name ==
                VITTE_AST_INVALID_ID) {
                return member_name;
            }

            member =
                vitte_parser_add_node(
                    parser,
                    VITTE_AST_NODE_MEMBER_EXPR,
                    start_span);

            if (member == VITTE_AST_INVALID_ID) {
                return member;
            }

            if (!vitte_parser_node_append_child(
                    parser,
                    member,
                    expression) ||
                !vitte_parser_node_append_child(
                    parser,
                    member,
                    member_name)) {
                return VITTE_AST_INVALID_ID;
            }

            node =
                vitte_parser_get_node_mut(
                    parser,
                    member);

            if (node != NULL) {
                const vitte_ast_node_t *name_node;

                name_node =
                    vitte_parser_get_node(
                        parser,
                        member_name);

                if (name_node != NULL) {
                    node->span =
                        vitte_parser_span_join(
                            start_span,
                            name_node->span);
                }
            }

            expression = member;
            continue;
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_QUESTION)) {
            vitte_ast_node_id_t postfix;
            vitte_ast_node_t *node;

            postfix =
                vitte_parser_add_node(
                    parser,
                    VITTE_AST_NODE_TRY_EXPR,
                    vitte_parser_get_node(
                        parser,
                        expression)->span);

            if (postfix == VITTE_AST_INVALID_ID) {
                return postfix;
            }

            if (!vitte_parser_node_append_child(
                    parser,
                    postfix,
                    expression)) {
                return VITTE_AST_INVALID_ID;
            }

            node =
                vitte_parser_get_node_mut(
                    parser,
                    postfix);

            if (node != NULL) {
                node->span =
                    vitte_parser_span_join(
                        node->span,
                        vitte_parser_span_from_token(
                            vitte_parser_previous(
                                parser)));
            }

            expression = postfix;
            continue;
        }

        break;
    }

    return expression;
}

/* ========================================================================= */
/* Unary expressions                                                         */
/* ========================================================================= */

static bool
vitte_parser_is_unary_operator(
    vitte_token_kind_t kind)
{
    switch (kind) {
        case VITTE_TOKEN_PLUS:
        case VITTE_TOKEN_MINUS:
        case VITTE_TOKEN_BANG:
        case VITTE_TOKEN_TILDE:
        case VITTE_TOKEN_STAR:
        case VITTE_TOKEN_AMP:
        case VITTE_TOKEN_KW_NOT:
        case VITTE_TOKEN_KW_MOVE:
        case VITTE_TOKEN_KW_REF:
        case VITTE_TOKEN_KW_AWAIT:
            return true;

        default:
            return false;
    }
}

static vitte_ast_node_id_t
vitte_parser_parse_unary(
    vitte_parser_t *parser)
{
    const vitte_token_t *operator_token;

    operator_token = vitte_parser_current(parser);

    if (operator_token != NULL &&
        vitte_parser_is_unary_operator(
            operator_token->kind)) {
        vitte_ast_node_id_t operand;
        vitte_ast_node_id_t unary;
        vitte_ast_node_t *node;

        (void)vitte_parser_advance(parser);

        operand = vitte_parser_parse_unary(parser);

        if (operand == VITTE_AST_INVALID_ID) {
            return operand;
        }

        unary =
            vitte_parser_add_node(
                parser,
                operator_token->kind ==
                        VITTE_TOKEN_KW_AWAIT
                    ? VITTE_AST_NODE_AWAIT_EXPR
                    : VITTE_AST_NODE_UNARY_EXPR,
                vitte_parser_span_from_token(
                    operator_token));

        if (unary == VITTE_AST_INVALID_ID) {
            return unary;
        }

        node =
            vitte_parser_get_node_mut(
                parser,
                unary);

        if (node != NULL) {
            const vitte_ast_node_t *operand_node;

            node->operator_kind =
                operator_token->kind;

            node->token_index =
                (size_t)(operator_token -
                         parser->tokens);

            operand_node =
                vitte_parser_get_node(
                    parser,
                    operand);

            if (operand_node != NULL) {
                node->span =
                    vitte_parser_span_join(
                        node->span,
                        operand_node->span);
            }
        }

        if (!vitte_parser_node_append_child(
                parser,
                unary,
                operand)) {
            return VITTE_AST_INVALID_ID;
        }

        return unary;
    }

    {
        vitte_ast_node_id_t expression;

        expression = vitte_parser_parse_postfix(parser);
        if (expression == VITTE_AST_INVALID_ID) {
            return expression;
        }

        while (vitte_parser_match(parser, VITTE_TOKEN_KW_AS)) {
            const vitte_token_t *cast_token;
            vitte_ast_node_id_t target_type;
            vitte_ast_node_id_t cast;
            vitte_ast_node_t *cast_node;
            const vitte_ast_node_t *expression_node;

            cast_token = vitte_parser_previous(parser);
            target_type = vitte_parser_parse_type_expression(parser);
            if (target_type == VITTE_AST_INVALID_ID) {
                return target_type;
            }

            expression_node =
                vitte_parser_get_node(parser, expression);
            cast = vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_CAST_EXPR,
                expression_node != NULL
                    ? expression_node->span
                    : vitte_parser_span_from_token(cast_token));
            if (cast == VITTE_AST_INVALID_ID) {
                return cast;
            }

            cast_node = vitte_parser_get_node_mut(parser, cast);
            if (cast_node != NULL) {
                const vitte_ast_node_t *target_node;

                target_node =
                    vitte_parser_get_node(parser, target_type);
                if (target_node != NULL) {
                    cast_node->span = vitte_parser_span_join(
                        cast_node->span,
                        target_node->span);
                }
            }

            if (!vitte_parser_node_append_child(
                    parser,
                    cast,
                    expression) ||
                !vitte_parser_node_append_child(
                    parser,
                    cast,
                    target_type)) {
                return VITTE_AST_INVALID_ID;
            }
            expression = cast;
        }

        return expression;
    }
}

/* ========================================================================= */
/* Binary expressions                                                        */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_binary_precedence(
    vitte_parser_t *parser,
    int minimum_precedence)
{
    vitte_ast_node_id_t left;

    left = vitte_parser_parse_unary(parser);

    if (left == VITTE_AST_INVALID_ID) {
        return left;
    }

    for (;;) {
        const vitte_token_t *operator_token;
        int precedence;
        int next_precedence;
        vitte_ast_node_id_t right;
        vitte_ast_node_id_t binary;
        vitte_ast_node_t *node;
        const vitte_ast_node_t *left_node;
        const vitte_ast_node_t *right_node;

        operator_token = vitte_parser_current(parser);

        if (operator_token == NULL) {
            break;
        }

        precedence =
            vitte_parser_binary_precedence(
                operator_token->kind);

        if (precedence == 0 ||
            precedence < minimum_precedence) {
            break;
        }

        (void)vitte_parser_advance(parser);

        next_precedence =
            vitte_parser_binary_right_associative(
                operator_token->kind)
                ? precedence
                : precedence + 1;

        right =
            vitte_parser_parse_binary_precedence(
                parser,
                next_precedence);

        if (right == VITTE_AST_INVALID_ID) {
            return right;
        }

        left_node =
            vitte_parser_get_node(
                parser,
                left);

        right_node =
            vitte_parser_get_node(
                parser,
                right);

        binary =
            vitte_parser_add_node(
                parser,
                vitte_parser_binary_right_associative(
                    operator_token->kind) &&
                    precedence == 1
                    ? VITTE_AST_NODE_ASSIGN_EXPR
                    : VITTE_AST_NODE_BINARY_EXPR,
                left_node != NULL
                    ? left_node->span
                    : vitte_parser_span_from_token(
                          operator_token));

        if (binary == VITTE_AST_INVALID_ID) {
            return binary;
        }

        node =
            vitte_parser_get_node_mut(
                parser,
                binary);

        if (node != NULL) {
            node->operator_kind =
                operator_token->kind;

            node->token_index =
                (size_t)(operator_token -
                         parser->tokens);

            if (right_node != NULL) {
                node->span =
                    vitte_parser_span_join(
                        node->span,
                        right_node->span);
            }
        }

        if (!vitte_parser_node_append_child(
                parser,
                binary,
                left) ||
            !vitte_parser_node_append_child(
                parser,
                binary,
                right)) {
            return VITTE_AST_INVALID_ID;
        }

        left = binary;
    }

    return left;
}

static vitte_ast_node_id_t
vitte_parser_parse_expression(
    vitte_parser_t *parser)
{
    vitte_ast_node_id_t result;

    if (!vitte_parser_enter_recursion(parser)) {
        return VITTE_AST_INVALID_ID;
    }

    result =
        vitte_parser_parse_binary_precedence(
            parser,
            1);

    vitte_parser_leave_recursion(parser);

    return result;
}

/* ========================================================================= */
/* Type expressions                                                          */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_type_expression(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t type;
    vitte_ast_node_id_t base;
    vitte_ast_node_t *node;

    if (!vitte_parser_enter_recursion(parser)) {
        return VITTE_AST_INVALID_ID;
    }

    begin = parser->position;

    if (vitte_parser_match(parser, VITTE_TOKEN_AMP) ||
        vitte_parser_match(parser, VITTE_TOKEN_KW_REF)) {
        vitte_ast_node_id_t pointee;

        (void)vitte_parser_match(
            parser,
            VITTE_TOKEN_KW_MUT);

        pointee =
            vitte_parser_parse_type_expression(
                parser);

        if (pointee == VITTE_AST_INVALID_ID) {
            vitte_parser_leave_recursion(parser);
            return pointee;
        }

        type =
            vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_REFERENCE_TYPE,
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position));

        if (type != VITTE_AST_INVALID_ID) {
            (void)vitte_parser_node_append_child(
                parser,
                type,
                pointee);
        }

        vitte_parser_leave_recursion(parser);

        return type;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_STAR)) {
        vitte_ast_node_id_t pointee;

        pointee =
            vitte_parser_parse_type_expression(
                parser);

        if (pointee == VITTE_AST_INVALID_ID) {
            vitte_parser_leave_recursion(parser);
            return pointee;
        }

        type =
            vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_POINTER_TYPE,
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position));

        if (type != VITTE_AST_INVALID_ID) {
            (void)vitte_parser_node_append_child(
                parser,
                type,
                pointee);
        }

        vitte_parser_leave_recursion(parser);

        return type;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_LEFT_BRACKET)) {
        vitte_ast_node_id_t length_expression;
        vitte_ast_node_id_t element_type;
        const vitte_token_t *first_item;

        first_item = vitte_parser_current(parser);
        if (first_item != NULL &&
            first_item->kind != VITTE_TOKEN_INTEGER_LITERAL) {
            element_type =
                vitte_parser_parse_type_expression(parser);
            if (element_type == VITTE_AST_INVALID_ID ||
                vitte_parser_expect(
                    parser,
                    VITTE_TOKEN_RIGHT_BRACKET) == NULL) {
                vitte_parser_leave_recursion(parser);
                return VITTE_AST_INVALID_ID;
            }

            type = vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_ARRAY_TYPE,
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position));
            if (type != VITTE_AST_INVALID_ID &&
                !vitte_parser_node_append_child(
                    parser,
                    type,
                    element_type)) {
                type = VITTE_AST_INVALID_ID;
            }

            vitte_parser_leave_recursion(parser);
            return type;
        }

        length_expression =
            vitte_parser_parse_expression(parser);

        if (length_expression ==
            VITTE_AST_INVALID_ID ||
            vitte_parser_expect(
                parser,
                VITTE_TOKEN_RIGHT_BRACKET) ==
                NULL) {
            vitte_parser_leave_recursion(parser);
            return VITTE_AST_INVALID_ID;
        }

        element_type =
            vitte_parser_parse_type_expression(
                parser);

        if (element_type == VITTE_AST_INVALID_ID) {
            vitte_parser_leave_recursion(parser);
            return element_type;
        }

        type =
            vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_ARRAY_TYPE,
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position));

        if (type != VITTE_AST_INVALID_ID) {
            (void)vitte_parser_node_append_child(
                parser,
                type,
                length_expression);

            (void)vitte_parser_node_append_child(
                parser,
                type,
                element_type);
        }

        vitte_parser_leave_recursion(parser);

        return type;
    }

    base = vitte_parser_parse_path(parser);

    if (base == VITTE_AST_INVALID_ID) {
        vitte_parser_leave_recursion(parser);
        return base;
    }

    type =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_TYPE_EXPR,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (type == VITTE_AST_INVALID_ID) {
        vitte_parser_leave_recursion(parser);
        return type;
    }

    (void)vitte_parser_node_append_child(
        parser,
        type,
        base);

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_LESS) ||
        vitte_parser_match(
            parser,
            VITTE_TOKEN_LEFT_BRACKET)) {
        vitte_token_kind_t closing_kind;

        closing_kind =
            parser->tokens[parser->position - 1u].kind ==
                    VITTE_TOKEN_LESS
                ? VITTE_TOKEN_GREATER
                : VITTE_TOKEN_RIGHT_BRACKET;

        if (!vitte_parser_check(
                parser,
                closing_kind)) {
            for (;;) {
                vitte_ast_node_id_t argument;

                argument =
                    vitte_parser_parse_type_expression(
                        parser);

                if (argument ==
                    VITTE_AST_INVALID_ID) {
                    vitte_parser_leave_recursion(
                        parser);

                    return VITTE_AST_INVALID_ID;
                }

                if (!vitte_parser_node_append_child(
                        parser,
                        type,
                        argument)) {
                    vitte_parser_leave_recursion(
                        parser);

                    return VITTE_AST_INVALID_ID;
                }

                if (!vitte_parser_match(
                        parser,
                        VITTE_TOKEN_COMMA)) {
                    break;
                }
            }
        }

        if (vitte_parser_expect(
                parser,
                closing_kind) == NULL) {
            vitte_parser_leave_recursion(parser);
            return VITTE_AST_INVALID_ID;
        }
    }

    node = vitte_parser_get_node_mut(parser, type);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    vitte_parser_leave_recursion(parser);

    return type;
}

/* ========================================================================= */
/* Generic parameter list                                                    */
/* ========================================================================= */

static bool
vitte_parser_parse_generic_parameters(
    vitte_parser_t *parser,
    vitte_ast_node_id_t owner)
{
    if (!vitte_parser_match(
            parser,
            VITTE_TOKEN_LESS)) {
        return true;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_GREATER)) {
        return true;
    }

    for (;;) {
        vitte_ast_node_id_t parameter;
        vitte_ast_node_id_t name;

        name =
            vitte_parser_parse_identifier_node(
                parser);

        if (name == VITTE_AST_INVALID_ID) {
            return false;
        }

        parameter =
            vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_GENERIC_PARAM,
                vitte_parser_get_node(
                    parser,
                    name)->span);

        if (parameter == VITTE_AST_INVALID_ID) {
            return false;
        }

        if (!vitte_parser_node_append_child(
                parser,
                parameter,
                name)) {
            return false;
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_COLON)) {
            vitte_ast_node_id_t bound;

            bound =
                vitte_parser_parse_type_expression(
                    parser);

            if (bound == VITTE_AST_INVALID_ID ||
                !vitte_parser_node_append_child(
                    parser,
                    parameter,
                    bound)) {
                return false;
            }
        }

        if (!vitte_parser_node_append_child(
                parser,
                owner,
                parameter)) {
            return false;
        }

        if (!vitte_parser_match(
                parser,
                VITTE_TOKEN_COMMA)) {
            break;
        }
    }

    return vitte_parser_expect(
               parser,
               VITTE_TOKEN_GREATER) != NULL;
}

/* ========================================================================= */
/* where clause                                                              */
/* ========================================================================= */

static bool
vitte_parser_parse_where_clause(
    vitte_parser_t *parser,
    vitte_ast_node_id_t owner)
{
    vitte_ast_node_id_t where_node;

    if (!vitte_parser_match(
            parser,
            VITTE_TOKEN_KW_WHERE)) {
        return true;
    }

    where_node =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_WHERE_CLAUSE,
            vitte_parser_span_from_token(
                vitte_parser_previous(parser)));

    if (where_node == VITTE_AST_INVALID_ID) {
        return false;
    }

    for (;;) {
        vitte_ast_node_id_t predicate;
        vitte_ast_node_id_t subject;
        vitte_ast_node_id_t bound;

        subject =
            vitte_parser_parse_type_expression(
                parser);

        if (subject == VITTE_AST_INVALID_ID) {
            return false;
        }

        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_COLON) == NULL) {
            return false;
        }

        bound =
            vitte_parser_parse_type_expression(
                parser);

        if (bound == VITTE_AST_INVALID_ID) {
            return false;
        }

        predicate =
            vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_WHERE_PREDICATE,
                vitte_parser_span_join(
                    vitte_parser_get_node(
                        parser,
                        subject)->span,
                    vitte_parser_get_node(
                        parser,
                        bound)->span));

        if (predicate == VITTE_AST_INVALID_ID) {
            return false;
        }

        if (!vitte_parser_node_append_child(
                parser,
                predicate,
                subject) ||
            !vitte_parser_node_append_child(
                parser,
                predicate,
                bound) ||
            !vitte_parser_node_append_child(
                parser,
                where_node,
                predicate)) {
            return false;
        }

        if (!vitte_parser_match(
                parser,
                VITTE_TOKEN_COMMA)) {
            break;
        }
    }

    return vitte_parser_node_append_child(
        parser,
        owner,
        where_node);
}

/* ========================================================================= */
/* Parameters                                                                */
/* ========================================================================= */

static bool
vitte_parser_parse_parameters(
    vitte_parser_t *parser,
    vitte_ast_node_id_t function)
{
    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_LEFT_PAREN) == NULL) {
        return false;
    }

    if (!vitte_parser_check(
            parser,
            VITTE_TOKEN_RIGHT_PAREN)) {
        for (;;) {
            vitte_ast_node_id_t parameter;
            vitte_ast_node_id_t name;
            vitte_ast_node_id_t type;
            size_t begin;

            begin = parser->position;

            name =
                vitte_parser_parse_identifier_node(
                    parser);

            if (name == VITTE_AST_INVALID_ID) {
                return false;
            }

            if (vitte_parser_expect(
                    parser,
                    VITTE_TOKEN_COLON) == NULL) {
                return false;
            }

            type =
                vitte_parser_parse_type_expression(
                    parser);

            if (type == VITTE_AST_INVALID_ID) {
                return false;
            }

            parameter =
                vitte_parser_add_node(
                    parser,
                    VITTE_AST_NODE_PARAMETER,
                    vitte_parser_span_from_range(
                        parser,
                        begin,
                        parser->position));

            if (parameter == VITTE_AST_INVALID_ID) {
                return false;
            }

            if (!vitte_parser_node_append_child(
                    parser,
                    parameter,
                    name) ||
                !vitte_parser_node_append_child(
                    parser,
                    parameter,
                    type) ||
                !vitte_parser_node_append_child(
                    parser,
                    function,
                    parameter)) {
                return false;
            }

            if (!vitte_parser_match(
                    parser,
                    VITTE_TOKEN_COMMA)) {
                break;
            }

            if (vitte_parser_check(
                    parser,
                    VITTE_TOKEN_RIGHT_PAREN)) {
                break;
            }
        }
    }

    return vitte_parser_expect(
               parser,
               VITTE_TOKEN_RIGHT_PAREN) != NULL;
}

/* ========================================================================= */
/* Block                                                                     */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_block(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t block;
    vitte_ast_node_t *node;

    if (!vitte_parser_enter_recursion(parser)) {
        return VITTE_AST_INVALID_ID;
    }

    begin = parser->position;

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_LEFT_BRACE) == NULL) {
        vitte_parser_leave_recursion(parser);
        return VITTE_AST_INVALID_ID;
    }

    block =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_BLOCK,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (block == VITTE_AST_INVALID_ID) {
        vitte_parser_leave_recursion(parser);
        return block;
    }

    while (!vitte_parser_at_end(parser) &&
           !vitte_parser_check(
               parser,
               VITTE_TOKEN_RIGHT_BRACE)) {
        vitte_ast_node_id_t statement;
        size_t before;

        before = parser->position;

        statement =
            vitte_parser_parse_statement(parser);

        if (statement != VITTE_AST_INVALID_ID) {
            if (!vitte_parser_node_append_child(
                    parser,
                    block,
                    statement)) {
                vitte_parser_leave_recursion(parser);
                return VITTE_AST_INVALID_ID;
            }
        } else {
            vitte_parser_synchronize(parser);
        }

        if (parser->position == before &&
            !vitte_parser_at_end(parser)) {
            (void)vitte_parser_advance(parser);
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_RIGHT_BRACE) == NULL) {
        vitte_parser_leave_recursion(parser);
        return VITTE_AST_INVALID_ID;
    }

    node = vitte_parser_get_node_mut(parser, block);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    vitte_parser_leave_recursion(parser);

    return block;
}

static bool
vitte_parser_if_has_else_ahead(
    const vitte_parser_t *parser);

static vitte_ast_node_id_t
vitte_parser_parse_block_expression(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t block;
    vitte_ast_node_t *node;

    if (!vitte_parser_enter_recursion(parser)) {
        return VITTE_AST_INVALID_ID;
    }
    begin = parser->position;
    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_LEFT_BRACE) == NULL) {
        vitte_parser_leave_recursion(parser);
        return VITTE_AST_INVALID_ID;
    }
    block = vitte_parser_add_node(
        parser,
        VITTE_AST_NODE_BLOCK_EXPR,
        vitte_parser_span_from_range(parser, begin, parser->position));
    if (block == VITTE_AST_INVALID_ID) {
        vitte_parser_leave_recursion(parser);
        return block;
    }

    while (!vitte_parser_at_end(parser) &&
           !vitte_parser_check(parser, VITTE_TOKEN_RIGHT_BRACE)) {
        const vitte_token_t *current;
        size_t before;
        vitte_ast_node_id_t item;

        current = vitte_parser_current(parser);
        before = parser->position;
        if (current != NULL &&
            ((current->kind == VITTE_TOKEN_KW_IF &&
              !vitte_parser_if_has_else_ahead(parser)) ||
             current->kind == VITTE_TOKEN_KW_LET ||
             current->kind == VITTE_TOKEN_KW_RETURN ||
             current->kind == VITTE_TOKEN_KW_GIVE ||
             current->kind == VITTE_TOKEN_KW_SET ||
             current->kind == VITTE_TOKEN_KW_WHILE ||
             current->kind == VITTE_TOKEN_KW_LOOP ||
             current->kind == VITTE_TOKEN_KW_FOR ||
             current->kind == VITTE_TOKEN_KW_BREAK ||
             current->kind == VITTE_TOKEN_KW_CONTINUE ||
             current->kind == VITTE_TOKEN_KW_DEFER ||
             current->kind == VITTE_TOKEN_KW_ASSERT)) {
            item = vitte_parser_parse_statement(parser);
        } else {
            item = vitte_parser_parse_expression(parser);
            if (item != VITTE_AST_INVALID_ID &&
                vitte_parser_match(
                    parser,
                    VITTE_TOKEN_SEMICOLON)) {
                vitte_ast_node_id_t statement;
                statement = vitte_parser_add_node(
                    parser,
                    VITTE_AST_NODE_EXPR_STMT,
                    vitte_parser_get_node(parser, item)->span);
                if (statement == VITTE_AST_INVALID_ID ||
                    !vitte_parser_node_append_child(
                        parser,
                        statement,
                        item)) {
                    item = VITTE_AST_INVALID_ID;
                } else {
                    item = statement;
                }
            } else if (item != VITTE_AST_INVALID_ID) {
                if (!vitte_parser_check(
                        parser,
                        VITTE_TOKEN_RIGHT_BRACE)) {
                    (void)vitte_parser_error_here(
                        parser,
                        VITTE_PARSER_ERROR_EXPECTED_TOKEN,
                        VITTE_TOKEN_RIGHT_BRACE);
                    item = VITTE_AST_INVALID_ID;
                }
            }
        }

        if (item == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(parser, block, item)) {
            vitte_parser_leave_recursion(parser);
            return VITTE_AST_INVALID_ID;
        }
        if (parser->position == before &&
            !vitte_parser_at_end(parser)) {
            (void)vitte_parser_advance(parser);
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_RIGHT_BRACE) == NULL) {
        vitte_parser_leave_recursion(parser);
        return VITTE_AST_INVALID_ID;
    }
    node = vitte_parser_get_node_mut(parser, block);
    if (node != NULL) {
        node->span = vitte_parser_span_from_range(
            parser,
            begin,
            parser->position);
    }
    vitte_parser_leave_recursion(parser);
    return block;
}

static vitte_ast_node_id_t
vitte_parser_parse_if_expression(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t condition;
    vitte_ast_node_id_t then_branch;
    vitte_ast_node_id_t else_branch;
    vitte_ast_node_id_t expression;
    vitte_ast_node_t *node;

    begin = parser->position;
    if (!vitte_parser_check(parser, VITTE_TOKEN_KW_IF) &&
        !vitte_parser_check(parser, VITTE_TOKEN_KW_ELIF)) {
        (void)vitte_parser_error_here(
            parser,
            VITTE_PARSER_ERROR_EXPECTED_TOKEN,
            VITTE_TOKEN_KW_IF);
        return VITTE_AST_INVALID_ID;
    }
    (void)vitte_parser_advance(parser);
    condition = vitte_parser_parse_expression(parser);
    if (condition == VITTE_AST_INVALID_ID) {
        return condition;
    }
    then_branch = vitte_parser_parse_block_expression(parser);
    if (then_branch == VITTE_AST_INVALID_ID) {
        return then_branch;
    }
    if (vitte_parser_check(parser, VITTE_TOKEN_KW_ELIF)) {
        else_branch = vitte_parser_parse_if_expression(parser);
    } else if (vitte_parser_match(parser, VITTE_TOKEN_KW_ELSE)) {
        if (vitte_parser_check(parser, VITTE_TOKEN_KW_IF) ||
            vitte_parser_check(parser, VITTE_TOKEN_KW_ELIF)) {
            else_branch = vitte_parser_parse_if_expression(parser);
        } else {
            else_branch = vitte_parser_parse_block_expression(parser);
        }
    } else {
        (void)vitte_parser_error_here(
            parser,
            VITTE_PARSER_ERROR_EXPECTED_TOKEN,
            VITTE_TOKEN_KW_ELSE);
        return VITTE_AST_INVALID_ID;
    }
    if (else_branch == VITTE_AST_INVALID_ID) {
        return else_branch;
    }

    expression = vitte_parser_add_node(
        parser,
        VITTE_AST_NODE_IF_EXPR,
        vitte_parser_span_from_range(parser, begin, parser->position));
    if (expression == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(parser, expression, condition) ||
        !vitte_parser_node_append_child(parser, expression, then_branch) ||
        !vitte_parser_node_append_child(parser, expression, else_branch)) {
        return VITTE_AST_INVALID_ID;
    }
    node = vitte_parser_get_node_mut(parser, expression);
    if (node != NULL) {
        node->span = vitte_parser_span_from_range(
            parser,
            begin,
            parser->position);
    }
    return expression;
}

static bool
vitte_parser_if_has_else_ahead(
    const vitte_parser_t *parser)
{
    size_t index;
    size_t brace_depth;
    bool found_body;

    if (parser == NULL || parser->position >= parser->token_count) {
        return false;
    }
    index = parser->position + 1u;
    brace_depth = 0u;
    found_body = false;
    for (; index < parser->token_count; ++index) {
        const vitte_token_t *token;

        token = &parser->tokens[index];
        if (!found_body) {
            if (token->kind == VITTE_TOKEN_LEFT_BRACE) {
                found_body = true;
                brace_depth = 1u;
            }
            continue;
        }
        if (token->kind == VITTE_TOKEN_LEFT_BRACE) {
            ++brace_depth;
        } else if (token->kind == VITTE_TOKEN_RIGHT_BRACE) {
            if (--brace_depth == 0u) {
                return index + 1u < parser->token_count &&
                    (parser->tokens[index + 1u].kind ==
                         VITTE_TOKEN_KW_ELSE ||
                     parser->tokens[index + 1u].kind ==
                         VITTE_TOKEN_KW_ELIF);
            }
        }
    }
    return false;
}

/* ========================================================================= */
/* let                                                                       */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_let_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_id_t name;
    vitte_ast_node_t *node;

    begin = parser->position;

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_KW_LET) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_LET_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID) {
        return statement;
    }

    node = vitte_parser_get_node_mut(
        parser,
        statement);

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_KW_MUT)) {
        if (node != NULL) {
            node->flags |=
                VITTE_AST_FLAG_MUTABLE;
        }
    }

    name = vitte_parser_parse_identifier_node(parser);

    if (name == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            name)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_COLON)) {
        vitte_ast_node_id_t type;

        type =
            vitte_parser_parse_type_expression(
                parser);

        if (type == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                statement,
                type)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_EQUAL)) {
        vitte_ast_node_id_t initializer;

        initializer =
            vitte_parser_parse_expression(parser);

        if (initializer == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                statement,
                initializer)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_SEMICOLON) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    node = vitte_parser_get_node_mut(
        parser,
        statement);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return statement;
}

/* ========================================================================= */
/* return                                                                    */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_return_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_t *node;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_RETURN_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID) {
        return statement;
    }

    if (!vitte_parser_check(
            parser,
            VITTE_TOKEN_SEMICOLON)) {
        vitte_ast_node_id_t expression;

        expression =
            vitte_parser_parse_expression(parser);

        if (expression == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                statement,
                expression)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_SEMICOLON) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    node = vitte_parser_get_node_mut(
        parser,
        statement);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return statement;
}

/* ========================================================================= */
/* break / continue                                                          */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_jump_statement(
    vitte_parser_t *parser,
    vitte_ast_node_kind_t kind)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_t *node;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    statement =
        vitte_parser_add_node(
            parser,
            kind,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID) {
        return statement;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_SEMICOLON) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            statement);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return statement;
}

/* ========================================================================= */
/* defer                                                                     */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_defer_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_id_t body;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_DEFER_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID) {
        return statement;
    }

    if (vitte_parser_check(
            parser,
            VITTE_TOKEN_LEFT_BRACE)) {
        body = vitte_parser_parse_block(parser);
    } else {
        body = vitte_parser_parse_expression(parser);

        if (body != VITTE_AST_INVALID_ID &&
            vitte_parser_expect(
                parser,
                VITTE_TOKEN_SEMICOLON) == NULL) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (body == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            body)) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                statement);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return statement;
}

/* ========================================================================= */
/* assert                                                                    */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_assert_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_id_t condition;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_LEFT_PAREN)) {
        condition =
            vitte_parser_parse_expression(parser);

        if (condition == VITTE_AST_INVALID_ID ||
            vitte_parser_expect(
                parser,
                VITTE_TOKEN_RIGHT_PAREN) == NULL) {
            return VITTE_AST_INVALID_ID;
        }
    } else {
        condition =
            vitte_parser_parse_expression(parser);

        if (condition == VITTE_AST_INVALID_ID) {
            return condition;
        }
    }

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_ASSERT_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            condition)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_SEMICOLON) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                statement);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return statement;
}

/* ========================================================================= */
/* if                                                                        */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_if_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_id_t condition;
    vitte_ast_node_id_t then_branch;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    condition =
        vitte_parser_parse_expression(parser);

    if (condition == VITTE_AST_INVALID_ID) {
        return condition;
    }

    then_branch = vitte_parser_parse_block(parser);

    if (then_branch == VITTE_AST_INVALID_ID) {
        return then_branch;
    }

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_IF_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            condition) ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            then_branch)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_KW_ELSE)) {
        vitte_ast_node_id_t else_branch;

        if (vitte_parser_check(
                parser,
                VITTE_TOKEN_KW_IF)) {
            else_branch =
                vitte_parser_parse_if_statement(
                    parser);
        } else {
            else_branch =
                vitte_parser_parse_block(parser);
        }

        if (else_branch == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                statement,
                else_branch)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                statement);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return statement;
}

/* ========================================================================= */
/* while                                                                     */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_while_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_id_t condition;
    vitte_ast_node_id_t body;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    condition =
        vitte_parser_parse_expression(parser);

    if (condition == VITTE_AST_INVALID_ID) {
        return condition;
    }

    body = vitte_parser_parse_block(parser);

    if (body == VITTE_AST_INVALID_ID) {
        return body;
    }

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_WHILE_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            condition) ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            body)) {
        return VITTE_AST_INVALID_ID;
    }

    return statement;
}

/* ========================================================================= */
/* loop                                                                      */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_loop_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_id_t body;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    body = vitte_parser_parse_block(parser);

    if (body == VITTE_AST_INVALID_ID) {
        return body;
    }

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_LOOP_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            body)) {
        return VITTE_AST_INVALID_ID;
    }

    return statement;
}

/* ========================================================================= */
/* for                                                                       */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_for_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_id_t binding;
    vitte_ast_node_id_t iterable;
    vitte_ast_node_id_t body;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    binding =
        vitte_parser_parse_identifier_node(
            parser);

    if (binding == VITTE_AST_INVALID_ID) {
        return binding;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_KW_IN) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    iterable =
        vitte_parser_parse_expression(parser);

    if (iterable == VITTE_AST_INVALID_ID) {
        return iterable;
    }

    body = vitte_parser_parse_block(parser);

    if (body == VITTE_AST_INVALID_ID) {
        return body;
    }

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_FOR_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            binding) ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            iterable) ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            body)) {
        return VITTE_AST_INVALID_ID;
    }

    return statement;
}

/* ========================================================================= */
/* unsafe                                                                    */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_unsafe_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;
    vitte_ast_node_id_t body;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    body = vitte_parser_parse_block(parser);

    if (body == VITTE_AST_INVALID_ID) {
        return body;
    }

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_UNSAFE_BLOCK,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            body)) {
        return VITTE_AST_INVALID_ID;
    }

    return statement;
}

/* ========================================================================= */
/* asm                                                                       */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_asm_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t statement;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_ASM_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID) {
        return statement;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_LEFT_PAREN)) {
        if (!vitte_parser_check(
                parser,
                VITTE_TOKEN_RIGHT_PAREN)) {
            for (;;) {
                vitte_ast_node_id_t operand;

                operand =
                    vitte_parser_parse_expression(
                        parser);

                if (operand == VITTE_AST_INVALID_ID ||
                    !vitte_parser_node_append_child(
                        parser,
                        statement,
                        operand)) {
                    return VITTE_AST_INVALID_ID;
                }

                if (!vitte_parser_match(
                        parser,
                        VITTE_TOKEN_COMMA)) {
                    break;
                }
            }
        }

        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_RIGHT_PAREN) == NULL) {
            return VITTE_AST_INVALID_ID;
        }
    } else if (vitte_parser_check(
                   parser,
                   VITTE_TOKEN_STRING_LITERAL)) {
        vitte_ast_node_id_t literal;

        literal = vitte_parser_parse_literal(parser);

        if (literal == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                statement,
                literal)) {
            return VITTE_AST_INVALID_ID;
        }
    } else {
        (void)vitte_parser_error_here(
            parser,
            VITTE_PARSER_ERROR_EXPECTED_EXPRESSION,
            VITTE_TOKEN_STRING_LITERAL);

        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_SEMICOLON) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                statement);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return statement;
}

/* ========================================================================= */
/* match                                                                     */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_match_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t match_node;
    vitte_ast_node_id_t subject;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    subject =
        vitte_parser_parse_expression(parser);

    if (subject == VITTE_AST_INVALID_ID) {
        return subject;
    }

    match_node =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_MATCH_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (match_node == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            match_node,
            subject)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_LEFT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    while (!vitte_parser_at_end(parser) &&
           !vitte_parser_check(
               parser,
               VITTE_TOKEN_RIGHT_BRACE)) {
        vitte_ast_node_id_t arm;
        vitte_ast_node_id_t pattern;
        vitte_ast_node_id_t body;
        size_t arm_begin;

        arm_begin = parser->position;

        pattern =
            vitte_parser_parse_expression(parser);

        if (pattern == VITTE_AST_INVALID_ID) {
            vitte_parser_synchronize(parser);
            continue;
        }

        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_FAT_ARROW) == NULL) {
            return VITTE_AST_INVALID_ID;
        }

        if (vitte_parser_check(
                parser,
                VITTE_TOKEN_LEFT_BRACE)) {
            body = vitte_parser_parse_block(parser);
        } else {
            body = vitte_parser_parse_expression(parser);
        }

        if (body == VITTE_AST_INVALID_ID) {
            return body;
        }

        arm =
            vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_MATCH_ARM,
                vitte_parser_span_from_range(
                    parser,
                    arm_begin,
                    parser->position));

        if (arm == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                arm,
                pattern) ||
            !vitte_parser_node_append_child(
                parser,
                arm,
                body) ||
            !vitte_parser_node_append_child(
                parser,
                match_node,
                arm)) {
            return VITTE_AST_INVALID_ID;
        }

        if (!vitte_parser_match(
                parser,
                VITTE_TOKEN_COMMA)) {
            if (!vitte_parser_check(
                    parser,
                    VITTE_TOKEN_RIGHT_BRACE)) {
                (void)vitte_parser_error_here(
                    parser,
                    VITTE_PARSER_ERROR_EXPECTED_TOKEN,
                    VITTE_TOKEN_COMMA);

                return VITTE_AST_INVALID_ID;
            }
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_RIGHT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                match_node);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return match_node;
}

/* ========================================================================= */
/* Expression statement                                                      */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_expression_statement(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t expression;
    vitte_ast_node_id_t statement;

    begin = parser->position;

    expression =
        vitte_parser_parse_expression(parser);

    if (expression == VITTE_AST_INVALID_ID) {
        return expression;
    }

    statement =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_EXPR_STMT,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (statement == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            statement,
            expression)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_SEMICOLON)) {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                statement);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return statement;
}

/* ========================================================================= */
/* Statements                                                                */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_statement(
    vitte_parser_t *parser)
{
    const vitte_token_t *token;

    token = vitte_parser_current(parser);

    if (token == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    switch (token->kind) {
        case VITTE_TOKEN_KW_LET:
            return vitte_parser_parse_let_statement(
                parser);

        case VITTE_TOKEN_KW_RETURN:
        case VITTE_TOKEN_KW_GIVE:
            return vitte_parser_parse_return_statement(
                parser);

        case VITTE_TOKEN_KW_SET:
            (void)vitte_parser_advance(parser);
            return vitte_parser_parse_expression_statement(parser);

        case VITTE_TOKEN_KW_BREAK:
            return vitte_parser_parse_jump_statement(
                parser,
                VITTE_AST_NODE_BREAK_STMT);

        case VITTE_TOKEN_KW_CONTINUE:
            return vitte_parser_parse_jump_statement(
                parser,
                VITTE_AST_NODE_CONTINUE_STMT);

        case VITTE_TOKEN_KW_DEFER:
            return vitte_parser_parse_defer_statement(
                parser);

        case VITTE_TOKEN_KW_ASSERT:
            return vitte_parser_parse_assert_statement(
                parser);

        case VITTE_TOKEN_KW_IF:
            return vitte_parser_parse_if_statement(
                parser);

        case VITTE_TOKEN_KW_WHILE:
            return vitte_parser_parse_while_statement(
                parser);

        case VITTE_TOKEN_KW_LOOP:
            return vitte_parser_parse_loop_statement(
                parser);

        case VITTE_TOKEN_KW_FOR:
            return vitte_parser_parse_for_statement(
                parser);

        case VITTE_TOKEN_KW_MATCH:
            return vitte_parser_parse_match_statement(
                parser);

        case VITTE_TOKEN_KW_UNSAFE:
            return vitte_parser_parse_unsafe_statement(
                parser);

        case VITTE_TOKEN_KW_ASM:
            return vitte_parser_parse_asm_statement(
                parser);

        case VITTE_TOKEN_LEFT_BRACE:
            return vitte_parser_parse_block(parser);

        default:
            break;
    }

    /*
     * Local declarations are accepted where appropriate by the semantic
     * phase. Parsing them here keeps syntax recovery deterministic.
     */
    if (vitte_parser_is_declaration_start(
            token->kind)) {
        return vitte_parser_parse_declaration(parser);
    }

    return vitte_parser_parse_expression_statement(
        parser);
}

/* ========================================================================= */
/* space declaration                                                         */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_space_declaration(
    vitte_parser_t *parser,
    uint32_t flags)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t path;
    vitte_ast_node_t *node;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    path = vitte_parser_parse_path(parser);

    if (path == VITTE_AST_INVALID_ID) {
        return path;
    }

    declaration =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_SPACE_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            path)) {
        return VITTE_AST_INVALID_ID;
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->flags = flags;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_SEMICOLON)) {
        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }

        return declaration;
    }

    if (vitte_parser_check(
            parser,
            VITTE_TOKEN_LEFT_BRACE)) {
        vitte_ast_node_id_t body;

        body = vitte_parser_parse_space_body(parser);

        if (body == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                body)) {
            return VITTE_AST_INVALID_ID;
        }

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }

        return declaration;
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return declaration;
}

static vitte_ast_node_id_t
vitte_parser_parse_space_body(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t body;
    vitte_ast_node_t *body_node;

    if (parser == NULL ||
        !vitte_parser_check(
            parser,
            VITTE_TOKEN_LEFT_BRACE)) {
        return VITTE_AST_INVALID_ID;
    }

    begin = parser->position;
    (void)vitte_parser_advance(parser);

    body =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_BLOCK,
            vitte_parser_span_invalid_private());
    if (body == VITTE_AST_INVALID_ID) {
        return body;
    }

    while (!vitte_parser_at_end(parser) &&
           !vitte_parser_check(
               parser,
               VITTE_TOKEN_RIGHT_BRACE)) {
        vitte_ast_node_id_t declaration;
        size_t before;

        before = parser->position;
        declaration =
            vitte_parser_parse_declaration(parser);

        if (declaration != VITTE_AST_INVALID_ID) {
            if (!vitte_parser_node_append_child(
                    parser,
                    body,
                    declaration)) {
                return VITTE_AST_INVALID_ID;
            }
        } else {
            vitte_parser_synchronize(parser);
        }

        if (parser->position == before &&
            !vitte_parser_at_end(parser)) {
            (void)vitte_parser_advance(parser);
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_RIGHT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    body_node =
        vitte_parser_get_node_mut(
            parser,
            body);
    if (body_node != NULL) {
        body_node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return body;
}

/* ========================================================================= */
/* use declaration                                                           */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_use_declaration(
    vitte_parser_t *parser,
    uint32_t flags)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t path;
    vitte_ast_node_t *node;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    path = vitte_parser_parse_path(parser);

    if (path == VITTE_AST_INVALID_ID) {
        return path;
    }

    declaration =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_USE_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            path)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_match(parser, VITTE_TOKEN_COLON_COLON) ||
        vitte_parser_match(parser, VITTE_TOKEN_DOT)) {
        if (vitte_parser_match(parser, VITTE_TOKEN_STAR)) {
            flags |= VITTE_AST_FLAG_GLOB_IMPORT;
        } else if (vitte_parser_match(parser, VITTE_TOKEN_LEFT_BRACE)) {
            while (!vitte_parser_at_end(parser) &&
                   !vitte_parser_check(parser, VITTE_TOKEN_RIGHT_BRACE)) {
                vitte_ast_node_id_t item;
                const vitte_token_t *alias_keyword;

                item = vitte_parser_parse_path(parser);
                if (item == VITTE_AST_INVALID_ID ||
                    !vitte_parser_node_append_child(
                        parser,
                        declaration,
                        item)) {
                    return VITTE_AST_INVALID_ID;
                }

                alias_keyword = vitte_parser_current(parser);
                if (alias_keyword != NULL &&
                    (alias_keyword->kind == VITTE_TOKEN_KW_AS ||
                     (alias_keyword->kind == VITTE_TOKEN_IDENTIFIER &&
                      vitte_token_text_equal(alias_keyword, "as", 2u)))) {
                    (void)vitte_parser_advance(parser);
                    item = vitte_parser_parse_identifier_node(parser);
                    if (item == VITTE_AST_INVALID_ID ||
                        !vitte_parser_node_append_child(
                            parser,
                            declaration,
                            item)) {
                        return VITTE_AST_INVALID_ID;
                    }
                }

                if (!vitte_parser_match(parser, VITTE_TOKEN_COMMA) &&
                    !vitte_parser_check(parser, VITTE_TOKEN_RIGHT_BRACE)) {
                    (void)vitte_parser_error_here(
                        parser,
                        VITTE_PARSER_ERROR_EXPECTED_TOKEN,
                        VITTE_TOKEN_COMMA);
                    return VITTE_AST_INVALID_ID;
                }
            }

            if (vitte_parser_expect(
                    parser,
                    VITTE_TOKEN_RIGHT_BRACE) == NULL) {
                return VITTE_AST_INVALID_ID;
            }
        } else {
            (void)vitte_parser_error_here(
                parser,
                VITTE_PARSER_ERROR_EXPECTED_TOKEN,
                VITTE_TOKEN_IDENTIFIER);
            return VITTE_AST_INVALID_ID;
        }
    } else {
        const vitte_token_t *alias_keyword;

        alias_keyword = vitte_parser_current(parser);
        if (alias_keyword != NULL &&
            (alias_keyword->kind == VITTE_TOKEN_KW_AS ||
             (alias_keyword->kind == VITTE_TOKEN_IDENTIFIER &&
              vitte_token_text_equal(alias_keyword, "as", 2u)))) {
            vitte_ast_node_id_t alias;

            (void)vitte_parser_advance(parser);
            alias = vitte_parser_parse_identifier_node(parser);
            if (alias == VITTE_AST_INVALID_ID ||
                !vitte_parser_node_append_child(
                    parser,
                    declaration,
                    alias)) {
                return VITTE_AST_INVALID_ID;
            }
        }
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->flags = flags;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_SEMICOLON) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return declaration;
}

static vitte_ast_node_id_t
vitte_parser_parse_export_clause(
    vitte_parser_t *parser)
{
    size_t begin;
    vitte_ast_node_id_t export_node;

    begin = parser->position;
    (void)vitte_parser_advance(parser);

    export_node = vitte_parser_add_node(
        parser,
        VITTE_AST_NODE_USE_DECL,
        vitte_parser_span_from_range(parser, begin, parser->position));
    if (export_node == VITTE_AST_INVALID_ID) {
        return export_node;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                export_node);
        if (node != NULL) {
            node->flags = VITTE_AST_FLAG_PUBLIC;
        }
    }

    if (vitte_parser_match(parser, VITTE_TOKEN_STAR)) {
        /* Export-all is represented by the declaration flag. */
        vitte_ast_node_t *node;

        node = vitte_parser_get_node_mut(parser, export_node);
        if (node != NULL) {
            node->flags = VITTE_AST_FLAG_PUBLIC;
        }
    } else {
        bool grouped;

        grouped = vitte_parser_match(parser, VITTE_TOKEN_LEFT_BRACE);
        do {
            vitte_ast_node_id_t item;
            const vitte_token_t *alias_keyword;

            item = vitte_parser_parse_path(parser);
            if (item == VITTE_AST_INVALID_ID ||
                !vitte_parser_node_append_child(
                    parser,
                    export_node,
                    item)) {
                return VITTE_AST_INVALID_ID;
            }

            alias_keyword = vitte_parser_current(parser);
            if (alias_keyword != NULL &&
                (alias_keyword->kind == VITTE_TOKEN_KW_AS ||
                 (alias_keyword->kind == VITTE_TOKEN_IDENTIFIER &&
                  vitte_token_text_equal(alias_keyword, "as", 2u)))) {
                (void)vitte_parser_advance(parser);
                item = vitte_parser_parse_identifier_node(parser);
                if (item == VITTE_AST_INVALID_ID ||
                    !vitte_parser_node_append_child(
                        parser,
                        export_node,
                        item)) {
                    return VITTE_AST_INVALID_ID;
                }
            }

            if (!grouped) {
                break;
            }

            if (!vitte_parser_match(parser, VITTE_TOKEN_COMMA)) {
                break;
            }

            if (vitte_parser_check(parser, VITTE_TOKEN_RIGHT_BRACE)) {
                break;
            }
        } while (!vitte_parser_at_end(parser));

        if (grouped &&
            vitte_parser_expect(
                parser,
                VITTE_TOKEN_RIGHT_BRACE) == NULL) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_expect(parser, VITTE_TOKEN_SEMICOLON) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    return export_node;
}

/* ========================================================================= */
/* const/static declaration                                                  */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_value_declaration(
    vitte_parser_t *parser,
    uint32_t flags,
    bool is_static)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t name;
    vitte_ast_node_t *node;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    name =
        vitte_parser_parse_identifier_node(
            parser);

    if (name == VITTE_AST_INVALID_ID) {
        return name;
    }

    declaration =
        vitte_parser_add_node(
            parser,
            is_static
                ? VITTE_AST_NODE_STATIC_DECL
                : VITTE_AST_NODE_CONST_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            name)) {
        return VITTE_AST_INVALID_ID;
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->flags = flags;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_COLON)) {
        vitte_ast_node_id_t type;

        type =
            vitte_parser_parse_type_expression(
                parser);

        if (type == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                type)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_EQUAL) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_id_t initializer;

        initializer =
            vitte_parser_parse_expression(parser);

        if (initializer == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                initializer)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_SEMICOLON) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return declaration;
}

/* ========================================================================= */
/* type/opaque declaration                                                   */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_type_declaration(
    vitte_parser_t *parser,
    uint32_t flags,
    bool opaque)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t name;
    vitte_ast_node_t *node;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    name =
        vitte_parser_parse_identifier_node(
            parser);

    if (name == VITTE_AST_INVALID_ID) {
        return name;
    }

    declaration =
        vitte_parser_add_node(
            parser,
            opaque
                ? VITTE_AST_NODE_OPAQUE_DECL
                : VITTE_AST_NODE_TYPE_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            name)) {
        return VITTE_AST_INVALID_ID;
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->flags = flags;
    }

    if (!vitte_parser_parse_generic_parameters(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    if (!vitte_parser_parse_where_clause(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    if (opaque) {
        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_SEMICOLON) == NULL) {
            return VITTE_AST_INVALID_ID;
        }
    } else {
        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_EQUAL) == NULL) {
            return VITTE_AST_INVALID_ID;
        }

        {
            vitte_ast_node_id_t target;

            target =
                vitte_parser_parse_type_expression(
                    parser);

            if (target == VITTE_AST_INVALID_ID ||
                !vitte_parser_node_append_child(
                    parser,
                    declaration,
                    target)) {
                return VITTE_AST_INVALID_ID;
            }
        }

        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_SEMICOLON) == NULL) {
            return VITTE_AST_INVALID_ID;
        }
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return declaration;
}

/* ========================================================================= */
/* form                                                                      */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_form_declaration(
    vitte_parser_t *parser,
    uint32_t flags)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t name;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    name =
        vitte_parser_parse_identifier_node(
            parser);

    if (name == VITTE_AST_INVALID_ID) {
        return name;
    }

    declaration =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_FORM_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            name)) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->flags = flags;
        }
    }

    if (!vitte_parser_parse_generic_parameters(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    if (!vitte_parser_parse_where_clause(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_LEFT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    while (!vitte_parser_at_end(parser) &&
           !vitte_parser_check(
               parser,
               VITTE_TOKEN_RIGHT_BRACE)) {
        size_t field_begin;
        vitte_ast_node_id_t field;
        vitte_ast_node_id_t field_name;
        vitte_ast_node_id_t field_type;

        field_begin = parser->position;

        field_name =
            vitte_parser_parse_identifier_node(
                parser);

        if (field_name == VITTE_AST_INVALID_ID ||
            vitte_parser_expect(
                parser,
                VITTE_TOKEN_COLON) == NULL) {
            return VITTE_AST_INVALID_ID;
        }

        field_type =
            vitte_parser_parse_type_expression(
                parser);

        if (field_type == VITTE_AST_INVALID_ID) {
            return field_type;
        }

        field =
            vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_FIELD_DECL,
                vitte_parser_span_from_range(
                    parser,
                    field_begin,
                    parser->position));

        if (field == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                field,
                field_name) ||
            !vitte_parser_node_append_child(
                parser,
                field,
                field_type) ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                field)) {
            return VITTE_AST_INVALID_ID;
        }

        if (vitte_parser_check(
                parser,
                VITTE_TOKEN_RIGHT_BRACE)) {
            continue;
        }

        if (!vitte_parser_match(
                parser,
                VITTE_TOKEN_COMMA) &&
            !vitte_parser_match(
                parser,
                VITTE_TOKEN_SEMICOLON)) {
            (void)vitte_parser_error_here(
                parser,
                VITTE_PARSER_ERROR_EXPECTED_TOKEN,
                VITTE_TOKEN_COMMA);

            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_RIGHT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return declaration;
}

/* ========================================================================= */
/* pick                                                                      */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_pick_declaration(
    vitte_parser_t *parser,
    uint32_t flags)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t name;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    name =
        vitte_parser_parse_identifier_node(
            parser);

    if (name == VITTE_AST_INVALID_ID) {
        return name;
    }

    declaration =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_PICK_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            name)) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->flags = flags;
        }
    }

    if (!vitte_parser_parse_generic_parameters(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_LEFT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    while (!vitte_parser_at_end(parser) &&
           !vitte_parser_check(
               parser,
               VITTE_TOKEN_RIGHT_BRACE)) {
        size_t variant_begin;
        vitte_ast_node_id_t variant;
        vitte_ast_node_id_t variant_name;

        variant_begin = parser->position;

        variant_name =
            vitte_parser_parse_identifier_node(
                parser);

        if (variant_name == VITTE_AST_INVALID_ID) {
            return variant_name;
        }

        variant =
            vitte_parser_add_node(
                parser,
                VITTE_AST_NODE_VARIANT_DECL,
                vitte_parser_span_from_range(
                    parser,
                    variant_begin,
                    parser->position));

        if (variant == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                variant,
                variant_name)) {
            return VITTE_AST_INVALID_ID;
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_LEFT_PAREN)) {
            if (!vitte_parser_check(
                    parser,
                    VITTE_TOKEN_RIGHT_PAREN)) {
                for (;;) {
                    vitte_ast_node_id_t payload_type;

                    payload_type =
                        vitte_parser_parse_type_expression(
                            parser);

                    if (payload_type ==
                            VITTE_AST_INVALID_ID ||
                        !vitte_parser_node_append_child(
                            parser,
                            variant,
                            payload_type)) {
                        return VITTE_AST_INVALID_ID;
                    }

                    if (!vitte_parser_match(
                            parser,
                            VITTE_TOKEN_COMMA)) {
                        break;
                    }
                }
            }

            if (vitte_parser_expect(
                    parser,
                    VITTE_TOKEN_RIGHT_PAREN) == NULL) {
                return VITTE_AST_INVALID_ID;
            }
        }

        {
            vitte_ast_node_t *variant_node;

            variant_node =
                vitte_parser_get_node_mut(
                    parser,
                    variant);

            if (variant_node != NULL) {
                variant_node->span =
                    vitte_parser_span_from_range(
                        parser,
                        variant_begin,
                        parser->position);
            }
        }

        if (!vitte_parser_node_append_child(
                parser,
                declaration,
                variant)) {
            return VITTE_AST_INVALID_ID;
        }

        if (!vitte_parser_match(
                parser,
                VITTE_TOKEN_COMMA) &&
            !vitte_parser_check(
                parser,
                VITTE_TOKEN_RIGHT_BRACE)) {
            (void)vitte_parser_error_here(
                parser,
                VITTE_PARSER_ERROR_EXPECTED_TOKEN,
                VITTE_TOKEN_COMMA);

            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_RIGHT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return declaration;
}

/* ========================================================================= */
/* proc / extern proc                                                        */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_proc_declaration(
    vitte_parser_t *parser,
    uint32_t flags,
    bool external)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t name;
    vitte_ast_node_t *node;

    begin = parser->position;

    if (external) {
        const vitte_token_t *external_token;
        bool intrinsic;

        external_token = vitte_parser_current(parser);
        intrinsic =
            external_token != NULL &&
            vitte_token_text_equal(
                external_token,
                "intrinsic",
                9u);

        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_KW_EXTERN) == NULL) {
            return VITTE_AST_INVALID_ID;
        }

        if (!intrinsic &&
            vitte_parser_check(
                parser,
                VITTE_TOKEN_STRING_LITERAL)) {
            /*
             * Optional ABI literal.
             */
            (void)vitte_parser_advance(parser);
        }
    }

    if (!external ||
        !vitte_token_text_equal(
            vitte_parser_token_at_private(parser, begin),
            "intrinsic",
            9u)) {
        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_KW_PROC) == NULL) {
            return VITTE_AST_INVALID_ID;
        }
    }

    name =
        vitte_parser_parse_identifier_node(
            parser);

    if (name == VITTE_AST_INVALID_ID) {
        return name;
    }

    declaration =
        vitte_parser_add_node(
            parser,
            external
                ? VITTE_AST_NODE_EXTERN_PROC_DECL
                : VITTE_AST_NODE_PROC_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            name)) {
        return VITTE_AST_INVALID_ID;
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->flags = flags;

        if (external) {
            node->flags |=
                VITTE_AST_FLAG_EXTERN;
        }
    }

    if (!vitte_parser_parse_generic_parameters(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    if (!vitte_parser_parse_parameters(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_ARROW)) {
        vitte_ast_node_id_t return_type;

        return_type =
            vitte_parser_parse_type_expression(
                parser);

        if (return_type == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                return_type)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (!vitte_parser_parse_where_clause(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    while (vitte_parser_check(parser, VITTE_TOKEN_KW_REQUIRES) ||
           vitte_parser_check(parser, VITTE_TOKEN_KW_ENSURES)) {
        vitte_ast_node_id_t contract;
        vitte_ast_flags_t contract_flag;

        contract_flag =
            vitte_parser_check(parser, VITTE_TOKEN_KW_REQUIRES)
                ? VITTE_AST_FLAG_CONTRACT_REQUIRES
                : VITTE_AST_FLAG_CONTRACT_ENSURES;
        (void)vitte_parser_advance(parser);
        contract = vitte_parser_parse_expression(parser);
        node = vitte_parser_get_node_mut(parser, contract);
        if (node != NULL) {
            node->flags |= contract_flag;
        }
        if (contract == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                contract) ||
            vitte_parser_expect(
                parser,
                VITTE_TOKEN_SEMICOLON) == NULL) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (external) {
        if (vitte_parser_expect(
                parser,
                VITTE_TOKEN_SEMICOLON) == NULL) {
            return VITTE_AST_INVALID_ID;
        }
    } else if (vitte_parser_match(
                   parser,
                   VITTE_TOKEN_SEMICOLON)) {
        /* A declaration without a body is a procedure prototype. */
    } else {
        vitte_ast_node_id_t body;

        body = vitte_parser_parse_block(parser);

        if (body == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                body)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    node =
        vitte_parser_get_node_mut(
            parser,
            declaration);

    if (node != NULL) {
        node->span =
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position);
    }

    return declaration;
}

/* ========================================================================= */
/* trait                                                                     */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_trait_declaration(
    vitte_parser_t *parser,
    uint32_t flags)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t name;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    name =
        vitte_parser_parse_identifier_node(
            parser);

    if (name == VITTE_AST_INVALID_ID) {
        return name;
    }

    declaration =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_TRAIT_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            name)) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->flags = flags;
        }
    }

    if (!vitte_parser_parse_generic_parameters(
            parser,
            declaration) ||
        !vitte_parser_parse_where_clause(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_LEFT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    while (!vitte_parser_at_end(parser) &&
           !vitte_parser_check(
               parser,
               VITTE_TOKEN_RIGHT_BRACE)) {
        vitte_ast_node_id_t member;

        member =
            vitte_parser_parse_declaration(parser);

        if (member == VITTE_AST_INVALID_ID) {
            vitte_parser_synchronize(parser);
            continue;
        }

        if (!vitte_parser_node_append_child(
                parser,
                declaration,
                member)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_RIGHT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return declaration;
}

/* ========================================================================= */
/* impl                                                                      */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_impl_declaration(
    vitte_parser_t *parser,
    uint32_t flags)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t first_type;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    declaration =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_IMPL_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID) {
        return declaration;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->flags = flags;
        }
    }

    if (!vitte_parser_parse_generic_parameters(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    first_type =
        vitte_parser_parse_type_expression(parser);

    if (first_type == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            first_type)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_match(
            parser,
            VITTE_TOKEN_KW_FOR)) {
        vitte_ast_node_id_t target_type;

        target_type =
            vitte_parser_parse_type_expression(
                parser);

        if (target_type == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                target_type)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (!vitte_parser_parse_where_clause(
            parser,
            declaration)) {
        return VITTE_AST_INVALID_ID;
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_LEFT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    while (!vitte_parser_at_end(parser) &&
           !vitte_parser_check(
               parser,
               VITTE_TOKEN_RIGHT_BRACE)) {
        vitte_ast_node_id_t member;

        member =
            vitte_parser_parse_declaration(parser);

        if (member == VITTE_AST_INVALID_ID) {
            vitte_parser_synchronize(parser);
            continue;
        }

        if (!vitte_parser_node_append_child(
                parser,
                declaration,
                member)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    if (vitte_parser_expect(
            parser,
            VITTE_TOKEN_RIGHT_BRACE) == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return declaration;
}

/* ========================================================================= */
/* macro                                                                     */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_macro_declaration(
    vitte_parser_t *parser,
    uint32_t flags)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t name;
    vitte_ast_node_id_t body;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    name =
        vitte_parser_parse_identifier_node(
            parser);

    if (name == VITTE_AST_INVALID_ID) {
        return name;
    }

    declaration =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_MACRO_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            name)) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->flags = flags;
        }
    }

    if (vitte_parser_check(
            parser,
            VITTE_TOKEN_LEFT_PAREN)) {
        if (!vitte_parser_parse_parameters(
                parser,
                declaration)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    body = vitte_parser_parse_block(parser);

    if (body == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            body)) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return declaration;
}

/* ========================================================================= */
/* test                                                                      */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_test_declaration(
    vitte_parser_t *parser,
    uint32_t flags)
{
    size_t begin;
    vitte_ast_node_id_t declaration;
    vitte_ast_node_id_t body;

    begin = parser->position;

    (void)vitte_parser_advance(parser);

    declaration =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_TEST_DECL,
            vitte_parser_span_from_range(
                parser,
                begin,
                parser->position));

    if (declaration == VITTE_AST_INVALID_ID) {
        return declaration;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->flags = flags;
        }
    }

    if (vitte_parser_check(
            parser,
            VITTE_TOKEN_STRING_LITERAL)) {
        vitte_ast_node_id_t name;

        name = vitte_parser_parse_literal(parser);

        if (name == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                name)) {
            return VITTE_AST_INVALID_ID;
        }
    } else if (vitte_parser_check(
                   parser,
                   VITTE_TOKEN_IDENTIFIER)) {
        vitte_ast_node_id_t name;

        name =
            vitte_parser_parse_identifier_node(
                parser);

        if (name == VITTE_AST_INVALID_ID ||
            !vitte_parser_node_append_child(
                parser,
                declaration,
                name)) {
            return VITTE_AST_INVALID_ID;
        }
    }

    body = vitte_parser_parse_block(parser);

    if (body == VITTE_AST_INVALID_ID ||
        !vitte_parser_node_append_child(
            parser,
            declaration,
            body)) {
        return VITTE_AST_INVALID_ID;
    }

    {
        vitte_ast_node_t *node;

        node =
            vitte_parser_get_node_mut(
                parser,
                declaration);

        if (node != NULL) {
            node->span =
                vitte_parser_span_from_range(
                    parser,
                    begin,
                    parser->position);
        }
    }

    return declaration;
}

/* ========================================================================= */
/* Declaration dispatch                                                      */
/* ========================================================================= */

static bool
vitte_parser_export_precedes_declaration(
    const vitte_parser_t *parser)
{
    const vitte_token_t *next;

    next = vitte_parser_peek(parser, 1u);
    if (next == NULL) {
        return false;
    }

    switch (next->kind) {
        case VITTE_TOKEN_KW_PUB:
        case VITTE_TOKEN_KW_EXPORT:
        case VITTE_TOKEN_KW_ASYNC:
        case VITTE_TOKEN_KW_UNSAFE:
        case VITTE_TOKEN_KW_COMPTIME:
        case VITTE_TOKEN_KW_SPACE:
        case VITTE_TOKEN_KW_USE:
        case VITTE_TOKEN_KW_CONST:
        case VITTE_TOKEN_KW_STATIC:
        case VITTE_TOKEN_KW_TYPE:
        case VITTE_TOKEN_KW_OPAQUE:
        case VITTE_TOKEN_KW_FORM:
        case VITTE_TOKEN_KW_PICK:
        case VITTE_TOKEN_KW_TRAIT:
        case VITTE_TOKEN_KW_IMPL:
        case VITTE_TOKEN_KW_PROC:
        case VITTE_TOKEN_KW_EXTERN:
        case VITTE_TOKEN_KW_MACRO:
        case VITTE_TOKEN_KW_TEST:
            return true;

        default:
            return false;
    }
}

static vitte_ast_node_id_t
vitte_parser_parse_declaration(
    vitte_parser_t *parser)
{
    const vitte_token_t *token;
    uint32_t flags;

    flags = VITTE_AST_FLAG_NONE;

    while (true) {
        if (vitte_parser_check(parser, VITTE_TOKEN_KW_EXPORT)) {
            if (vitte_parser_export_precedes_declaration(parser)) {
                (void)vitte_parser_advance(parser);
                flags |= VITTE_AST_FLAG_PUBLIC;
                continue;
            }

            return vitte_parser_parse_export_clause(parser);
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_KW_PUB)) {
            flags |= VITTE_AST_FLAG_PUBLIC;
            continue;
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_KW_ASYNC)) {
            flags |= VITTE_AST_FLAG_ASYNC;
            continue;
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_KW_UNSAFE)) {
            flags |= VITTE_AST_FLAG_UNSAFE;
            continue;
        }

        if (vitte_parser_match(
                parser,
                VITTE_TOKEN_KW_COMPTIME)) {
            flags |= VITTE_AST_FLAG_COMPTIME;
            continue;
        }

        break;
    }

    token = vitte_parser_current(parser);

    if (token == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    switch (token->kind) {
        case VITTE_TOKEN_KW_SPACE:
            return vitte_parser_parse_space_declaration(
                parser,
                flags);

        case VITTE_TOKEN_KW_USE:
            return vitte_parser_parse_use_declaration(
                parser,
                flags);

        case VITTE_TOKEN_KW_CONST:
            return vitte_parser_parse_value_declaration(
                parser,
                flags,
                false);

        case VITTE_TOKEN_KW_STATIC:
            return vitte_parser_parse_value_declaration(
                parser,
                flags,
                true);

        case VITTE_TOKEN_KW_TYPE:
            return vitte_parser_parse_type_declaration(
                parser,
                flags,
                false);

        case VITTE_TOKEN_KW_OPAQUE:
            return vitte_parser_parse_type_declaration(
                parser,
                flags,
                true);

        case VITTE_TOKEN_KW_FORM:
            return vitte_parser_parse_form_declaration(
                parser,
                flags);

        case VITTE_TOKEN_KW_PICK:
            return vitte_parser_parse_pick_declaration(
                parser,
                flags);

        case VITTE_TOKEN_KW_TRAIT:
            return vitte_parser_parse_trait_declaration(
                parser,
                flags);

        case VITTE_TOKEN_KW_IMPL:
            return vitte_parser_parse_impl_declaration(
                parser,
                flags);

        case VITTE_TOKEN_KW_PROC:
            return vitte_parser_parse_proc_declaration(
                parser,
                flags,
                false);

        case VITTE_TOKEN_KW_EXTERN:
            if (!vitte_parser_check_next(
                    parser,
                    VITTE_TOKEN_KW_PROC) &&
                !(vitte_parser_peek(parser, 1u) != NULL &&
                  vitte_parser_peek(parser, 1u)->kind ==
                      VITTE_TOKEN_STRING_LITERAL &&
                  vitte_parser_peek(parser, 2u) != NULL &&
                  vitte_parser_peek(parser, 2u)->kind ==
                     VITTE_TOKEN_KW_PROC) &&
                !(vitte_parser_peek(parser, 1u) != NULL &&
                  vitte_parser_peek(parser, 1u)->kind ==
                     VITTE_TOKEN_IDENTIFIER &&
                  vitte_token_text_equal(
                     token,
                     "intrinsic",
                     9u))) {
                (void)vitte_parser_error_here(
                    parser,
                    VITTE_PARSER_ERROR_EXPECTED_DECLARATION,
                    VITTE_TOKEN_KW_PROC);

                return VITTE_AST_INVALID_ID;
            }

            return vitte_parser_parse_proc_declaration(
                parser,
                flags,
                true);

        case VITTE_TOKEN_KW_MACRO:
            return vitte_parser_parse_macro_declaration(
                parser,
                flags);

        case VITTE_TOKEN_KW_TEST:
            return vitte_parser_parse_test_declaration(
                parser,
                flags);

        default:
            (void)vitte_parser_error_here(
                parser,
                VITTE_PARSER_ERROR_EXPECTED_DECLARATION,
                VITTE_TOKEN_INVALID);

            return VITTE_AST_INVALID_ID;
    }
}

/* ========================================================================= */
/* Translation unit                                                          */
/* ========================================================================= */

static vitte_ast_node_id_t
vitte_parser_parse_translation_unit(
    vitte_parser_t *parser)
{
    vitte_ast_node_id_t root;
    vitte_ast_node_t *root_node;

    root =
        vitte_parser_add_node(
            parser,
            VITTE_AST_NODE_TRANSLATION_UNIT,
            vitte_parser_span_invalid_private());

    if (root == VITTE_AST_INVALID_ID) {
        return root;
    }

    while (!vitte_parser_at_end(parser)) {
        vitte_ast_node_id_t declaration;
        size_t before;

        before = parser->position;

        declaration =
            vitte_parser_parse_declaration(parser);

        if (declaration != VITTE_AST_INVALID_ID) {
            if (!vitte_parser_node_append_child(
                    parser,
                    root,
                    declaration)) {
                return VITTE_AST_INVALID_ID;
            }

            parser->stats.declarations =
                vitte_parser_u64_add_sat(
                    parser->stats.declarations,
                    UINT64_C(1));
        } else {
            vitte_parser_synchronize(parser);
        }

        if (parser->position == before &&
            !vitte_parser_at_end(parser)) {
            (void)vitte_parser_advance(parser);
        }
    }

    root_node =
        vitte_parser_get_node_mut(
            parser,
            root);

    if (root_node != NULL &&
        parser->token_count != 0u) {
        root_node->span =
            vitte_parser_span_from_range(
                parser,
                0u,
                parser->token_count);
    }

    return root;
}

/* ========================================================================= */
/* AST validation                                                           */
/* ========================================================================= */

static bool
vitte_parser_validate_node(
    const vitte_parser_t *parser,
    const vitte_ast_node_t *node)
{
    size_t index;

    if (parser == NULL ||
        node == NULL) {
        return false;
    }

    if (node->id == VITTE_AST_INVALID_ID ||
        node->id >
            (vitte_ast_node_id_t)parser->node_count) {
        return false;
    }

    if (node->kind <= VITTE_AST_NODE_INVALID ||
        node->kind >= VITTE_AST_NODE_COUNT) {
        return false;
    }

    if (node->child_count >
        node->child_capacity) {
        return false;
    }

    if (node->child_count != 0u &&
        node->children == NULL) {
        return false;
    }

    for (index = 0u;
         index < node->child_count;
         ++index) {
        if (vitte_parser_get_node(
                parser,
                node->children[index]) == NULL) {
            return false;
        }
    }

    if (node->token_index !=
            VITTE_PARSER_NO_TOKEN &&
        node->token_index >=
            parser->token_count) {
        return false;
    }

    if (node->span.valid &&
        node->span.end < node->span.begin) {
        return false;
    }

    return true;
}

static bool
vitte_parser_validate_ast_cycles_visit(
    const vitte_parser_t *parser,
    vitte_ast_node_id_t id,
    unsigned char *state)
{
    const vitte_ast_node_t *node;
    size_t index;
    size_t array_index;

    if (id == VITTE_AST_INVALID_ID ||
        id > (vitte_ast_node_id_t)SIZE_MAX) {
        return false;
    }

    array_index = (size_t)(id - UINT64_C(1));

    if (array_index >= parser->node_count) {
        return false;
    }

    if (state[array_index] == 1u) {
        return false;
    }

    if (state[array_index] == 2u) {
        return true;
    }

    state[array_index] = 1u;

    node = vitte_parser_get_node(parser, id);

    if (node == NULL) {
        return false;
    }

    for (index = 0u;
         index < node->child_count;
         ++index) {
        if (!vitte_parser_validate_ast_cycles_visit(
                parser,
                node->children[index],
                state)) {
            return false;
        }
    }

    state[array_index] = 2u;

    return true;
}

bool
vitte_parser_validate(
    vitte_parser_t *parser)
{
    size_t index;
    unsigned char *state;
    bool valid;

    if (!vitte_parser_context_valid(parser)) {
        return false;
    }

    parser->stats.validation_runs =
        vitte_parser_u64_add_sat(
            parser->stats.validation_runs,
            UINT64_C(1));

    valid = true;

    if (parser->node_count >
            parser->max_nodes ||
        parser->diagnostic_count >
            parser->max_diagnostics) {
        valid = false;
    }

    for (index = 0u;
         valid && index < parser->node_count;
         ++index) {
        if (parser->nodes[index].id !=
                (vitte_ast_node_id_t)(index + 1u) ||
            !vitte_parser_validate_node(
                parser,
                &parser->nodes[index])) {
            valid = false;
        }
    }

    if (valid &&
        parser->root != VITTE_AST_INVALID_ID &&
        vitte_parser_get_node(
            parser,
            parser->root) == NULL) {
        valid = false;
    }

    state = NULL;

    if (valid &&
        parser->node_count != 0u) {
        state =
            (unsigned char *)calloc(
                parser->node_count,
                sizeof(*state));

        if (state == NULL) {
            parser->stats.allocation_failures =
                vitte_parser_u64_add_sat(
                    parser->stats.allocation_failures,
                    UINT64_C(1));

            parser->last_error =
                VITTE_PARSER_ERROR_OUT_OF_MEMORY;

            return false;
        }

        parser->stats.allocations =
            vitte_parser_u64_add_sat(
                parser->stats.allocations,
                UINT64_C(1));

        for (index = 0u;
             valid && index < parser->node_count;
             ++index) {
            if (state[index] == 0u &&
                !vitte_parser_validate_ast_cycles_visit(
                    parser,
                    parser->nodes[index].id,
                    state)) {
                valid = false;
            }
        }

        free(state);
    }

    if (!valid) {
        parser->stats.validation_failures =
            vitte_parser_u64_add_sat(
                parser->stats.validation_failures,
                UINT64_C(1));

        parser->last_error =
            VITTE_PARSER_ERROR_VALIDATION;

        return false;
    }

    parser->last_error =
        VITTE_PARSER_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_parser_fingerprint(
    vitte_parser_t *parser)
{
    uint64_t hash;
    size_t index;

    if (!vitte_parser_context_valid(parser)) {
        return UINT64_C(0);
    }

    parser->stats.hash_runs =
        vitte_parser_u64_add_sat(
            parser->stats.hash_runs,
            UINT64_C(1));

    hash = VITTE_PARSER_FNV_OFFSET;

    hash =
        vitte_parser_hash_u64(
            hash,
            (uint64_t)VITTE_PARSER_API_VERSION_MAJOR);

    hash =
        vitte_parser_hash_u64(
            hash,
            (uint64_t)VITTE_PARSER_API_VERSION_MINOR);

    hash =
        vitte_parser_hash_u64(
            hash,
            (uint64_t)VITTE_PARSER_API_VERSION_PATCH);

    hash =
        vitte_parser_hash_u64(
            hash,
            (uint64_t)parser->node_count);

    hash =
        vitte_parser_hash_u64(
            hash,
            parser->root);

    for (index = 0u;
         index < parser->node_count;
         ++index) {
        const vitte_ast_node_t *node;
        size_t child_index;

        node = &parser->nodes[index];

        hash =
            vitte_parser_hash_u64(
                hash,
                node->id);

        hash =
            vitte_parser_hash_u64(
                hash,
                (uint64_t)node->kind);

        hash =
            vitte_parser_hash_u64(
                hash,
                (uint64_t)node->flags);

        hash =
            vitte_parser_hash_u64(
                hash,
                (uint64_t)node->operator_kind);

        hash =
            vitte_parser_hash_u64(
                hash,
                node->token_index ==
                        VITTE_PARSER_NO_TOKEN
                    ? UINT64_MAX
                    : (uint64_t)node->token_index);

        hash =
            vitte_parser_hash_bool(
                hash,
                node->span.valid);

        if (node->span.valid) {
            hash =
                vitte_parser_hash_u64(
                    hash,
                    (uint64_t)node->span.file_id);

            hash =
                vitte_parser_hash_u64(
                    hash,
                    (uint64_t)node->span.begin);

            hash =
                vitte_parser_hash_u64(
                    hash,
                    (uint64_t)node->span.end);

            hash =
                vitte_parser_hash_u64(
                    hash,
                    (uint64_t)node->span.line);

            hash =
                vitte_parser_hash_u64(
                    hash,
                    (uint64_t)node->span.column);
        }

        hash =
            vitte_parser_hash_u64(
                hash,
                (uint64_t)node->child_count);

        for (child_index = 0u;
             child_index < node->child_count;
             ++child_index) {
            hash =
                vitte_parser_hash_u64(
                    hash,
                    node->children[child_index]);
        }

        /*
         * Token text is included for token-backed leaves so changing an
         * identifier/literal changes the AST fingerprint even if topology
         * remains identical.
         */
        if (node->token_index !=
                VITTE_PARSER_NO_TOKEN &&
            node->token_index <
                parser->token_count) {
            const vitte_token_t *token;

            token =
                &parser->tokens[
                    node->token_index];

            hash =
                vitte_parser_hash_u64(
                    hash,
                    (uint64_t)token->kind);

            hash =
                vitte_parser_hash_u64(
                    hash,
                    (uint64_t)token->length);

            hash =
                vitte_parser_hash_bytes(
                    hash,
                    token->lexeme,
                    token->length);
        }
    }

    return hash;
}

/* ========================================================================= */
/* Parse                                                                     */
/* ========================================================================= */

bool
vitte_parser_run(
    vitte_parser_t *parser)
{
    vitte_ast_node_id_t root;

    if (!vitte_parser_context_valid(parser)) {
        return false;
    }

    if (parser->state !=
        VITTE_PARSER_STATE_READY) {
        return vitte_parser_fail(
            parser,
            VITTE_PARSER_ERROR_INVALID_STATE);
    }

    parser->stats.runs =
        vitte_parser_u64_add_sat(
            parser->stats.runs,
            UINT64_C(1));

    parser->state =
        VITTE_PARSER_STATE_PARSING;

    parser->last_error =
        VITTE_PARSER_ERROR_NONE;

    parser->position = 0u;
    parser->recursion_depth = 0u;

    root =
        vitte_parser_parse_translation_unit(
            parser);

    if (root == VITTE_AST_INVALID_ID) {
        parser->state =
            VITTE_PARSER_STATE_FAILED;

        parser->stats.failed_runs =
            vitte_parser_u64_add_sat(
                parser->stats.failed_runs,
                UINT64_C(1));

        return false;
    }

    parser->root = root;

    if (parser->diagnostic_count != 0u) {
        size_t index;

        for (index = 0u;
             index < parser->diagnostic_count;
             ++index) {
            if (parser->diagnostics[index].kind ==
                VITTE_PARSER_DIAGNOSTIC_ERROR) {
                parser->state =
                    VITTE_PARSER_STATE_FAILED;

                parser->stats.failed_runs =
                    vitte_parser_u64_add_sat(
                        parser->stats.failed_runs,
                        UINT64_C(1));

                return false;
            }
        }
    }

    if (!vitte_parser_validate(parser)) {
        parser->state =
            VITTE_PARSER_STATE_FAILED;

        parser->stats.failed_runs =
            vitte_parser_u64_add_sat(
                parser->stats.failed_runs,
                UINT64_C(1));

        return false;
    }

    parser->state =
        VITTE_PARSER_STATE_DONE;

    parser->last_error =
        VITTE_PARSER_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Diagnostic access                                                         */
/* ========================================================================= */

const vitte_parser_diagnostic_t *
vitte_parser_diagnostic_at(
    const vitte_parser_t *parser,
    size_t index)
{
    if (!vitte_parser_context_valid(parser) ||
        index >= parser->diagnostic_count) {
        return NULL;
    }

    return &parser->diagnostics[index];
}

size_t
vitte_parser_diagnostic_count(
    const vitte_parser_t *parser)
{
    if (!vitte_parser_context_valid(parser)) {
        return 0u;
    }

    return parser->diagnostic_count;
}

/* ========================================================================= */
/* Root                                                                      */
/* ========================================================================= */

vitte_ast_node_id_t
vitte_parser_root(
    const vitte_parser_t *parser)
{
    if (!vitte_parser_context_valid(parser)) {
        return VITTE_AST_INVALID_ID;
    }

    return parser->root;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_parser_stats_t
vitte_parser_stats(
    const vitte_parser_t *parser)
{
    vitte_parser_stats_t stats;

    memset(&stats, 0, sizeof(stats));

    if (!vitte_parser_context_valid(parser)) {
        return stats;
    }

    return parser->stats;
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_parser_set_limits(
    vitte_parser_t *parser,
    size_t max_nodes,
    size_t max_diagnostics,
    size_t max_recursion_depth,
    size_t max_recovery_tokens)
{
    if (!vitte_parser_context_valid(parser) ||
        parser->state !=
            VITTE_PARSER_STATE_READY) {
        return vitte_parser_fail(
            parser,
            VITTE_PARSER_ERROR_INVALID_STATE);
    }

    if (max_nodes == 0u ||
        max_diagnostics == 0u ||
        max_recursion_depth == 0u ||
        max_recovery_tokens == 0u) {
        return vitte_parser_fail(
            parser,
            VITTE_PARSER_ERROR_INVALID_ARGUMENT);
    }

    if (max_nodes < parser->node_count ||
        max_diagnostics <
            parser->diagnostic_count) {
        return vitte_parser_fail(
            parser,
            VITTE_PARSER_ERROR_INVALID_ARGUMENT);
    }

    parser->max_nodes = max_nodes;
    parser->max_diagnostics = max_diagnostics;
    parser->max_recursion_depth =
        max_recursion_depth;
    parser->max_recovery_tokens =
        max_recovery_tokens;

    parser->last_error =
        VITTE_PARSER_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Release                                                                   */
/* ========================================================================= */

static void
vitte_parser_release_contents(
    vitte_parser_t *parser)
{
    size_t index;

    if (parser == NULL) {
        return;
    }

    for (index = 0u;
         index < parser->node_count;
         ++index) {
        free(parser->nodes[index].children);

        parser->nodes[index].children = NULL;
        parser->nodes[index].child_count = 0u;
        parser->nodes[index].child_capacity = 0u;
    }

    free(parser->nodes);
    free(parser->diagnostics);

    parser->nodes = NULL;
    parser->node_count = 0u;
    parser->node_capacity = 0u;

    parser->diagnostics = NULL;
    parser->diagnostic_count = 0u;
    parser->diagnostic_capacity = 0u;

    parser->root = VITTE_AST_INVALID_ID;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_parser_init(
    vitte_parser_t *parser,
    const vitte_token_t *tokens,
    size_t token_count)
{
    if (parser == NULL) {
        return false;
    }

    if (tokens == NULL && token_count != 0u) {
        return false;
    }

    memset(parser, 0, sizeof(*parser));

    parser->magic = VITTE_PARSER_MAGIC;

    parser->state =
        VITTE_PARSER_STATE_READY;

    parser->last_error =
        VITTE_PARSER_ERROR_NONE;

    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->position = 0u;

    parser->root =
        VITTE_AST_INVALID_ID;

    parser->max_nodes =
        VITTE_PARSER_DEFAULT_MAX_NODES;

    parser->max_diagnostics =
        VITTE_PARSER_DEFAULT_MAX_DIAGNOSTICS;

    parser->max_recursion_depth =
        VITTE_PARSER_DEFAULT_MAX_RECURSION_DEPTH;

    parser->max_recovery_tokens =
        VITTE_PARSER_DEFAULT_MAX_RECOVERY_TOKENS;

    parser->generation = UINT64_C(1);

    return true;
}

bool
vitte_parser_reset(
    vitte_parser_t *parser,
    const vitte_token_t *tokens,
    size_t token_count)
{
    size_t max_nodes;
    size_t max_diagnostics;
    size_t max_recursion_depth;
    size_t max_recovery_tokens;
    uint64_t generation;

    if (!vitte_parser_context_valid(parser)) {
        return false;
    }

    if (tokens == NULL && token_count != 0u) {
        return vitte_parser_fail(
            parser,
            VITTE_PARSER_ERROR_INVALID_ARGUMENT);
    }

    max_nodes = parser->max_nodes;
    max_diagnostics = parser->max_diagnostics;
    max_recursion_depth =
        parser->max_recursion_depth;
    max_recovery_tokens =
        parser->max_recovery_tokens;
    generation = parser->generation;

    vitte_parser_release_contents(parser);

    memset(&parser->stats, 0, sizeof(parser->stats));

    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->position = 0u;

    parser->state =
        VITTE_PARSER_STATE_READY;

    parser->last_error =
        VITTE_PARSER_ERROR_NONE;

    parser->max_nodes = max_nodes;
    parser->max_diagnostics = max_diagnostics;
    parser->max_recursion_depth =
        max_recursion_depth;
    parser->max_recovery_tokens =
        max_recovery_tokens;

    parser->generation =
        generation == UINT64_MAX
            ? UINT64_MAX
            : generation + UINT64_C(1);

    return true;
}

void
vitte_parser_destroy(
    vitte_parser_t *parser)
{
    if (parser == NULL) {
        return;
    }

    if (parser->magic == VITTE_PARSER_MAGIC) {
        vitte_parser_release_contents(parser);
    }

    memset(parser, 0, sizeof(*parser));

    parser->magic =
        VITTE_PARSER_DEAD_MAGIC;

    parser->state =
        VITTE_PARSER_STATE_DESTROYED;
}

bool
vitte_parser_is_valid(
    const vitte_parser_t *parser)
{
    return vitte_parser_context_valid(parser);
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_parser_error_name(
    vitte_parser_error_t error)
{
    switch (error) {
        case VITTE_PARSER_ERROR_NONE:
            return "none";

        case VITTE_PARSER_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case VITTE_PARSER_ERROR_INVALID_CONTEXT:
            return "invalid_context";

        case VITTE_PARSER_ERROR_INVALID_STATE:
            return "invalid_state";

        case VITTE_PARSER_ERROR_INVALID_TOKEN:
            return "invalid_token";

        case VITTE_PARSER_ERROR_INVALID_NODE:
            return "invalid_node";

        case VITTE_PARSER_ERROR_EXPECTED_TOKEN:
            return "expected_token";

        case VITTE_PARSER_ERROR_EXPECTED_IDENTIFIER:
            return "expected_identifier";

        case VITTE_PARSER_ERROR_EXPECTED_EXPRESSION:
            return "expected_expression";

        case VITTE_PARSER_ERROR_EXPECTED_TYPE:
            return "expected_type";

        case VITTE_PARSER_ERROR_EXPECTED_STATEMENT:
            return "expected_statement";

        case VITTE_PARSER_ERROR_EXPECTED_DECLARATION:
            return "expected_declaration";

        case VITTE_PARSER_ERROR_UNEXPECTED_TOKEN:
            return "unexpected_token";

        case VITTE_PARSER_ERROR_NODE_LIMIT:
            return "node_limit";

        case VITTE_PARSER_ERROR_DIAGNOSTIC_LIMIT:
            return "diagnostic_limit";

        case VITTE_PARSER_ERROR_RECURSION_LIMIT:
            return "recursion_limit";

        case VITTE_PARSER_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_PARSER_ERROR_OUT_OF_MEMORY:
            return "out_of_memory";

        case VITTE_PARSER_ERROR_VALIDATION:
            return "validation";

        case VITTE_PARSER_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_PARSER_ERROR_UNSUPPORTED:
            return "unsupported";

        case VITTE_PARSER_ERROR_INTERNAL:
            return "internal";

        case VITTE_PARSER_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_parser_state_name(
    vitte_parser_state_t state)
{
    switch (state) {
        case VITTE_PARSER_STATE_INVALID:
            return "invalid";

        case VITTE_PARSER_STATE_READY:
            return "ready";

        case VITTE_PARSER_STATE_PARSING:
            return "parsing";

        case VITTE_PARSER_STATE_DONE:
            return "done";

        case VITTE_PARSER_STATE_FAILED:
            return "failed";

        case VITTE_PARSER_STATE_DESTROYED:
            return "destroyed";

        case VITTE_PARSER_STATE_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_ast_node_kind_name(
    vitte_ast_node_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_NODE_INVALID:
            return "invalid";

        case VITTE_AST_NODE_TRANSLATION_UNIT:
            return "translation_unit";

        case VITTE_AST_NODE_IDENTIFIER:
            return "identifier";

        case VITTE_AST_NODE_PATH:
            return "path";

        case VITTE_AST_NODE_SPACE_DECL:
            return "space_decl";

        case VITTE_AST_NODE_USE_DECL:
            return "use_decl";

        case VITTE_AST_NODE_CONST_DECL:
            return "const_decl";

        case VITTE_AST_NODE_STATIC_DECL:
            return "static_decl";

        case VITTE_AST_NODE_TYPE_DECL:
            return "type_decl";

        case VITTE_AST_NODE_OPAQUE_DECL:
            return "opaque_decl";

        case VITTE_AST_NODE_FORM_DECL:
            return "form_decl";

        case VITTE_AST_NODE_FIELD_DECL:
            return "field_decl";

        case VITTE_AST_NODE_PICK_DECL:
            return "pick_decl";

        case VITTE_AST_NODE_VARIANT_DECL:
            return "variant_decl";

        case VITTE_AST_NODE_TRAIT_DECL:
            return "trait_decl";

        case VITTE_AST_NODE_IMPL_DECL:
            return "impl_decl";

        case VITTE_AST_NODE_PROC_DECL:
            return "proc_decl";

        case VITTE_AST_NODE_EXTERN_PROC_DECL:
            return "extern_proc_decl";

        case VITTE_AST_NODE_PARAMETER:
            return "parameter";

        case VITTE_AST_NODE_GENERIC_PARAM:
            return "generic_param";

        case VITTE_AST_NODE_WHERE_CLAUSE:
            return "where_clause";

        case VITTE_AST_NODE_WHERE_PREDICATE:
            return "where_predicate";

        case VITTE_AST_NODE_MACRO_DECL:
            return "macro_decl";

        case VITTE_AST_NODE_TEST_DECL:
            return "test_decl";

        case VITTE_AST_NODE_BLOCK:
            return "block";

        case VITTE_AST_NODE_LET_STMT:
            return "let_stmt";

        case VITTE_AST_NODE_RETURN_STMT:
            return "return_stmt";

        case VITTE_AST_NODE_DEFER_STMT:
            return "defer_stmt";

        case VITTE_AST_NODE_IF_STMT:
            return "if_stmt";

        case VITTE_AST_NODE_IF_EXPR:
            return "if_expr";

        case VITTE_AST_NODE_BLOCK_EXPR:
            return "block_expr";

        case VITTE_AST_NODE_WHILE_STMT:
            return "while_stmt";

        case VITTE_AST_NODE_LOOP_STMT:
            return "loop_stmt";

        case VITTE_AST_NODE_FOR_STMT:
            return "for_stmt";

        case VITTE_AST_NODE_BREAK_STMT:
            return "break_stmt";

        case VITTE_AST_NODE_CONTINUE_STMT:
            return "continue_stmt";

        case VITTE_AST_NODE_MATCH_STMT:
            return "match_stmt";

        case VITTE_AST_NODE_MATCH_ARM:
            return "match_arm";

        case VITTE_AST_NODE_UNSAFE_BLOCK:
            return "unsafe_block";

        case VITTE_AST_NODE_ASM_STMT:
            return "asm_stmt";

        case VITTE_AST_NODE_ASSERT_STMT:
            return "assert_stmt";

        case VITTE_AST_NODE_EXPR_STMT:
            return "expr_stmt";

        case VITTE_AST_NODE_TYPE_EXPR:
            return "type_expr";

        case VITTE_AST_NODE_POINTER_TYPE:
            return "pointer_type";

        case VITTE_AST_NODE_REFERENCE_TYPE:
            return "reference_type";

        case VITTE_AST_NODE_ARRAY_TYPE:
            return "array_type";

        case VITTE_AST_NODE_INTEGER_LITERAL:
            return "integer_literal";

        case VITTE_AST_NODE_FLOAT_LITERAL:
            return "float_literal";

        case VITTE_AST_NODE_STRING_LITERAL:
            return "string_literal";

        case VITTE_AST_NODE_CHARACTER_LITERAL:
            return "character_literal";

        case VITTE_AST_NODE_BOOL_LITERAL:
            return "bool_literal";

        case VITTE_AST_NODE_NULL_LITERAL:
            return "null_literal";

        case VITTE_AST_NODE_SELF_EXPR:
            return "self_expr";

        case VITTE_AST_NODE_GROUP_EXPR:
            return "group_expr";

        case VITTE_AST_NODE_ARRAY_EXPR:
            return "array_expr";

        case VITTE_AST_NODE_FORM_EXPR:
            return "form_expr";

        case VITTE_AST_NODE_FIELD_INIT:
            return "field_init";

        case VITTE_AST_NODE_UNARY_EXPR:
            return "unary_expr";

        case VITTE_AST_NODE_BINARY_EXPR:
            return "binary_expr";

        case VITTE_AST_NODE_CAST_EXPR:
            return "cast_expr";

        case VITTE_AST_NODE_ASSIGN_EXPR:
            return "assign_expr";

        case VITTE_AST_NODE_CALL_EXPR:
            return "call_expr";

        case VITTE_AST_NODE_INDEX_EXPR:
            return "index_expr";

        case VITTE_AST_NODE_MEMBER_EXPR:
            return "member_expr";

        case VITTE_AST_NODE_AWAIT_EXPR:
            return "await_expr";

        case VITTE_AST_NODE_TRY_EXPR:
            return "try_expr";

        case VITTE_AST_NODE_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_parser_translation_unit_anchor(void)
{
    /*
     * Intentionally empty.
     *
     * Stable linkage probe / static archive anchor.
     */
}
#if 0 /* Accidentally concatenated header text. */
#ifndef VITTE_PARSER_PARSER_H
#define VITTE_PARSER_PARSER_H

/*
 * Vitte Compiler
 * src/parser/parser.h
 *
 * Public parser and AST contract.
 *
 * Synchronized with parser.c.
 *
 * Design:
 *   - ISO C17;
 *   - lexer.h is the token contract;
 *   - deterministic AST;
 *   - explicit parser lifecycle;
 *   - stable node identifiers;
 *   - source spans preserved;
 *   - bounded AST/diagnostic/recursion resources;
 *   - structured diagnostics;
 *   - AST validation;
 *   - deterministic fingerprinting;
 *   - C/C++ compatible API.
 */

#include "../lexer/lexer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

#define VITTE_PARSER_API_VERSION_MAJOR 1u
#define VITTE_PARSER_API_VERSION_MINOR 0u
#define VITTE_PARSER_API_VERSION_PATCH 0u

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_PARSER_MAGIC \
    UINT64_C(0x5649545450415253)

#define VITTE_PARSER_DEAD_MAGIC \
    UINT64_C(0x4445414450415253)

#define VITTE_PARSER_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_PARSER_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_PARSER_DEFAULT_INITIAL_CAPACITY \
    ((size_t)64u)

#define VITTE_PARSER_DEFAULT_MAX_NODES \
    ((size_t)16777216u)

#define VITTE_PARSER_DEFAULT_MAX_DIAGNOSTICS \
    ((size_t)4096u)

#define VITTE_PARSER_DEFAULT_MAX_RECURSION_DEPTH \
    ((size_t)1024u)

#define VITTE_PARSER_DEFAULT_MAX_RECOVERY_TOKENS \
    ((size_t)4096u)

#define VITTE_PARSER_NO_TOKEN SIZE_MAX

/* ========================================================================= */
/* AST identifiers                                                           */
/* ========================================================================= */

typedef uint64_t vitte_ast_node_id_t;

#define VITTE_AST_INVALID_ID UINT64_C(0)

/* ========================================================================= */
/* Parser errors                                                             */
/* ========================================================================= */

typedef enum vitte_parser_error {
    VITTE_PARSER_ERROR_NONE = 0,

    VITTE_PARSER_ERROR_INVALID_ARGUMENT,
    VITTE_PARSER_ERROR_INVALID_CONTEXT,
    VITTE_PARSER_ERROR_INVALID_STATE,

    VITTE_PARSER_ERROR_INVALID_TOKEN,
    VITTE_PARSER_ERROR_INVALID_NODE,

    VITTE_PARSER_ERROR_EXPECTED_TOKEN,
    VITTE_PARSER_ERROR_EXPECTED_IDENTIFIER,
    VITTE_PARSER_ERROR_EXPECTED_EXPRESSION,
    VITTE_PARSER_ERROR_EXPECTED_TYPE,
    VITTE_PARSER_ERROR_EXPECTED_STATEMENT,
    VITTE_PARSER_ERROR_EXPECTED_DECLARATION,

    VITTE_PARSER_ERROR_UNEXPECTED_TOKEN,

    VITTE_PARSER_ERROR_NODE_LIMIT,
    VITTE_PARSER_ERROR_DIAGNOSTIC_LIMIT,
    VITTE_PARSER_ERROR_RECURSION_LIMIT,

    VITTE_PARSER_ERROR_OVERFLOW,
    VITTE_PARSER_ERROR_OUT_OF_MEMORY,

    VITTE_PARSER_ERROR_VALIDATION,
    VITTE_PARSER_ERROR_CORRUPTION,
    VITTE_PARSER_ERROR_UNSUPPORTED,
    VITTE_PARSER_ERROR_INTERNAL,

    VITTE_PARSER_ERROR_COUNT
} vitte_parser_error_t;

/* ========================================================================= */
/* Parser states                                                             */
/* ========================================================================= */

typedef enum vitte_parser_state {
    VITTE_PARSER_STATE_INVALID = 0,
    VITTE_PARSER_STATE_READY,
    VITTE_PARSER_STATE_PARSING,
    VITTE_PARSER_STATE_DONE,
    VITTE_PARSER_STATE_FAILED,
    VITTE_PARSER_STATE_DESTROYED,
    VITTE_PARSER_STATE_COUNT
} vitte_parser_state_t;

/* ========================================================================= */
/* Diagnostic kinds                                                          */
/* ========================================================================= */

typedef enum vitte_parser_diagnostic_kind {
    VITTE_PARSER_DIAGNOSTIC_INVALID = 0,
    VITTE_PARSER_DIAGNOSTIC_NOTE,
    VITTE_PARSER_DIAGNOSTIC_HELP,
    VITTE_PARSER_DIAGNOSTIC_WARNING,
    VITTE_PARSER_DIAGNOSTIC_ERROR,
    VITTE_PARSER_DIAGNOSTIC_COUNT
} vitte_parser_diagnostic_kind_t;

/* ========================================================================= */
/* Source span                                                               */
/* ========================================================================= */

typedef struct vitte_parser_span {
    uint32_t file_id;

    size_t begin;
    size_t end;

    size_t line;
    size_t column;

    bool valid;
} vitte_parser_span_t;

/* ========================================================================= */
/* AST node kinds                                                            */
/* ========================================================================= */

typedef enum vitte_ast_node_kind {
    VITTE_AST_NODE_INVALID = 0,

    /* Translation unit / names. */
    VITTE_AST_NODE_TRANSLATION_UNIT,
    VITTE_AST_NODE_IDENTIFIER,
    VITTE_AST_NODE_PATH,

    /* Top-level declarations. */
    VITTE_AST_NODE_SPACE_DECL,
    VITTE_AST_NODE_USE_DECL,

    VITTE_AST_NODE_CONST_DECL,
    VITTE_AST_NODE_STATIC_DECL,

    VITTE_AST_NODE_TYPE_DECL,
    VITTE_AST_NODE_OPAQUE_DECL,

    VITTE_AST_NODE_FORM_DECL,
    VITTE_AST_NODE_FIELD_DECL,

    VITTE_AST_NODE_PICK_DECL,
    VITTE_AST_NODE_VARIANT_DECL,

    VITTE_AST_NODE_TRAIT_DECL,
    VITTE_AST_NODE_IMPL_DECL,

    VITTE_AST_NODE_PROC_DECL,
    VITTE_AST_NODE_EXTERN_PROC_DECL,

    VITTE_AST_NODE_PARAMETER,
    VITTE_AST_NODE_GENERIC_PARAM,

    VITTE_AST_NODE_WHERE_CLAUSE,
    VITTE_AST_NODE_WHERE_PREDICATE,

    VITTE_AST_NODE_MACRO_DECL,
    VITTE_AST_NODE_TEST_DECL,

    /* Statements / blocks. */
    VITTE_AST_NODE_BLOCK,

    VITTE_AST_NODE_LET_STMT,
    VITTE_AST_NODE_RETURN_STMT,
    VITTE_AST_NODE_DEFER_STMT,

    VITTE_AST_NODE_IF_STMT,
    VITTE_AST_NODE_WHILE_STMT,
    VITTE_AST_NODE_LOOP_STMT,
    VITTE_AST_NODE_FOR_STMT,

    VITTE_AST_NODE_BREAK_STMT,
    VITTE_AST_NODE_CONTINUE_STMT,

    VITTE_AST_NODE_MATCH_STMT,
    VITTE_AST_NODE_MATCH_ARM,

    VITTE_AST_NODE_UNSAFE_BLOCK,
    VITTE_AST_NODE_ASM_STMT,
    VITTE_AST_NODE_ASSERT_STMT,

    VITTE_AST_NODE_EXPR_STMT,

    /* Types. */
    VITTE_AST_NODE_TYPE_EXPR,
    VITTE_AST_NODE_POINTER_TYPE,
    VITTE_AST_NODE_REFERENCE_TYPE,
    VITTE_AST_NODE_ARRAY_TYPE,

    /* Literals. */
    VITTE_AST_NODE_INTEGER_LITERAL,
    VITTE_AST_NODE_FLOAT_LITERAL,
    VITTE_AST_NODE_STRING_LITERAL,
    VITTE_AST_NODE_CHARACTER_LITERAL,
    VITTE_AST_NODE_BOOL_LITERAL,
    VITTE_AST_NODE_NULL_LITERAL,

    /* Expressions. */
    VITTE_AST_NODE_SELF_EXPR,
    VITTE_AST_NODE_GROUP_EXPR,

    VITTE_AST_NODE_UNARY_EXPR,
    VITTE_AST_NODE_BINARY_EXPR,
    VITTE_AST_NODE_ASSIGN_EXPR,

    VITTE_AST_NODE_CALL_EXPR,
    VITTE_AST_NODE_INDEX_EXPR,
    VITTE_AST_NODE_MEMBER_EXPR,

    VITTE_AST_NODE_AWAIT_EXPR,
    VITTE_AST_NODE_TRY_EXPR,

    VITTE_AST_NODE_COUNT
} vitte_ast_node_kind_t;

/* ========================================================================= */
/* AST flags                                                                 */
/* ========================================================================= */

typedef uint32_t vitte_ast_flags_t;

#define VITTE_AST_FLAG_NONE \
    UINT32_C(0)

#define VITTE_AST_FLAG_PUBLIC \
    (UINT32_C(1) << 0u)

#define VITTE_AST_FLAG_MUTABLE \
    (UINT32_C(1) << 1u)

#define VITTE_AST_FLAG_EXTERN \
    (UINT32_C(1) << 2u)

#define VITTE_AST_FLAG_ASYNC \
    (UINT32_C(1) << 3u)

#define VITTE_AST_FLAG_UNSAFE \
    (UINT32_C(1) << 4u)

#define VITTE_AST_FLAG_COMPTIME \
    (UINT32_C(1) << 5u)

/*
 * Reserved bits are intentionally left available for future declaration
 * modifiers without changing the node structure.
 */
#define VITTE_AST_FLAG_RESERVED_6 \
    (UINT32_C(1) << 6u)

#define VITTE_AST_FLAG_RESERVED_7 \
    (UINT32_C(1) << 7u)

/* ========================================================================= */
/* AST node                                                                  */
/* ========================================================================= */

typedef struct vitte_ast_node {
    /*
     * Stable 1-based identifier.
     * Zero is VITTE_AST_INVALID_ID.
     */
    vitte_ast_node_id_t id;

    vitte_ast_node_kind_t kind;

    /*
     * Modifier/property flags.
     */
    vitte_ast_flags_t flags;

    /*
     * Operator associated with unary/binary/assignment expressions.
     * VITTE_TOKEN_INVALID for nodes without an operator.
     */
    vitte_token_kind_t operator_kind;

    /*
     * Index into parser->tokens for token-backed leaves/operators.
     * VITTE_PARSER_NO_TOKEN means no directly associated token.
     */
    size_t token_index;

    vitte_parser_span_t span;

    /*
     * Generic ordered AST children.
     *
     * Interpretation depends on node kind, while ownership remains
     * exclusively with the parser context.
     */
    vitte_ast_node_id_t *children;
    size_t child_count;
    size_t child_capacity;
} vitte_ast_node_t;

/* ========================================================================= */
/* Diagnostics                                                               */
/* ========================================================================= */

typedef struct vitte_parser_diagnostic {
    vitte_parser_diagnostic_kind_t kind;

    vitte_parser_error_t error;

    vitte_parser_span_t span;

    /*
     * Token expectation information.
     *
     * VITTE_TOKEN_INVALID means the field is not applicable.
     */
    vitte_token_kind_t expected;
    vitte_token_kind_t found;
} vitte_parser_diagnostic_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_parser_stats {
    uint64_t runs;
    uint64_t failed_runs;

    uint64_t tokens_consumed;

    uint64_t nodes_created;
    uint64_t declarations;

    uint64_t diagnostics;
    uint64_t errors;
    uint64_t recoveries;

    uint64_t max_recursion_depth;

    uint64_t validation_runs;
    uint64_t validation_failures;

    uint64_t hash_runs;

    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;
} vitte_parser_stats_t;

/* ========================================================================= */
/* Parser context                                                            */
/* ========================================================================= */

typedef struct vitte_parser {
    uint64_t magic;

    vitte_parser_state_t state;
    vitte_parser_error_t last_error;

    /*
     * Borrowed immutable token stream.
     *
     * The caller must keep this memory alive for the entire parser
     * lifetime or until vitte_parser_reset().
     */
    const vitte_token_t *tokens;
    size_t token_count;
    size_t position;

    /*
     * Parser-owned AST storage.
     */
    vitte_ast_node_t *nodes;
    size_t node_count;
    size_t node_capacity;

    /*
     * Parser-owned diagnostics.
     */
    vitte_parser_diagnostic_t *diagnostics;
    size_t diagnostic_count;
    size_t diagnostic_capacity;

    /*
     * Root AST node.
     */
    vitte_ast_node_id_t root;

    /*
     * Resource limits.
     */
    size_t max_nodes;
    size_t max_diagnostics;
    size_t max_recursion_depth;
    size_t max_recovery_tokens;

    /*
     * Current recursive-descent depth.
     */
    size_t recursion_depth;

    vitte_parser_stats_t stats;

    /*
     * Incremented on reset so external tooling can detect stale views.
     */
    uint64_t generation;
} vitte_parser_t;

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_parser_error_name(
    vitte_parser_error_t error);

const char *
vitte_parser_state_name(
    vitte_parser_state_t state);

const char *
vitte_ast_node_kind_name(
    vitte_ast_node_kind_t kind);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_parser_init(
    vitte_parser_t *parser,
    const vitte_token_t *tokens,
    size_t token_count);

bool
vitte_parser_reset(
    vitte_parser_t *parser,
    const vitte_token_t *tokens,
    size_t token_count);

void
vitte_parser_destroy(
    vitte_parser_t *parser);

bool
vitte_parser_is_valid(
    const vitte_parser_t *parser);

/* ========================================================================= */
/* Parsing                                                                   */
/* ========================================================================= */

bool
vitte_parser_run(
    vitte_parser_t *parser);

/* ========================================================================= */
/* AST access                                                                */
/* ========================================================================= */

const vitte_ast_node_t *
vitte_parser_get_node(
    const vitte_parser_t *parser,
    vitte_ast_node_id_t id);

vitte_ast_node_t *
vitte_parser_get_node_mut(
    vitte_parser_t *parser,
    vitte_ast_node_id_t id);

vitte_ast_node_id_t
vitte_parser_root(
    const vitte_parser_t *parser);

/* ========================================================================= */
/* Diagnostic access                                                         */
/* ========================================================================= */

const vitte_parser_diagnostic_t *
vitte_parser_diagnostic_at(
    const vitte_parser_t *parser,
    size_t index);

size_t
vitte_parser_diagnostic_count(
    const vitte_parser_t *parser);

/* ========================================================================= */
/* Validation / fingerprint                                                  */
/* ========================================================================= */

bool
vitte_parser_validate(
    vitte_parser_t *parser);

uint64_t
vitte_parser_fingerprint(
    vitte_parser_t *parser);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_parser_stats_t
vitte_parser_stats(
    const vitte_parser_t *parser);

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_parser_set_limits(
    vitte_parser_t *parser,
    size_t max_nodes,
    size_t max_diagnostics,
    size_t max_recursion_depth,
    size_t max_recovery_tokens);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_parser_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline span helpers                                                       */
/* ========================================================================= */

static inline vitte_parser_span_t
vitte_parser_span_invalid(void)
{
    vitte_parser_span_t span;

    span.file_id = UINT32_C(0);
    span.begin = 0u;
    span.end = 0u;
    span.line = 0u;
    span.column = 0u;
    span.valid = false;

    return span;
}

static inline vitte_parser_span_t
vitte_parser_span_make(
    uint32_t file_id,
    size_t begin,
    size_t end,
    size_t line,
    size_t column)
{
    vitte_parser_span_t span;

    span.file_id = file_id;
    span.begin = begin;
    span.end = end;
    span.line = line;
    span.column = column;
    span.valid = end >= begin;

    return span;
}

static inline size_t
vitte_parser_span_length(
    vitte_parser_span_t span)
{
    if (!span.valid ||
        span.end < span.begin) {
        return 0u;
    }

    return span.end - span.begin;
}

static inline bool
vitte_parser_span_is_empty(
    vitte_parser_span_t span)
{
    return !span.valid ||
           span.begin == span.end;
}

static inline bool
vitte_parser_span_contains_offset(
    vitte_parser_span_t span,
    size_t offset)
{
    return span.valid &&
           offset >= span.begin &&
           offset < span.end;
}

/* ========================================================================= */
/* Inline node helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_ast_node_id_is_valid(
    vitte_ast_node_id_t id)
{
    return id != VITTE_AST_INVALID_ID;
}

static inline bool
vitte_ast_node_kind_is_declaration(
    vitte_ast_node_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_NODE_SPACE_DECL:
        case VITTE_AST_NODE_USE_DECL:
        case VITTE_AST_NODE_CONST_DECL:
        case VITTE_AST_NODE_STATIC_DECL:
        case VITTE_AST_NODE_TYPE_DECL:
        case VITTE_AST_NODE_OPAQUE_DECL:
        case VITTE_AST_NODE_FORM_DECL:
        case VITTE_AST_NODE_FIELD_DECL:
        case VITTE_AST_NODE_PICK_DECL:
        case VITTE_AST_NODE_VARIANT_DECL:
        case VITTE_AST_NODE_TRAIT_DECL:
        case VITTE_AST_NODE_IMPL_DECL:
        case VITTE_AST_NODE_PROC_DECL:
        case VITTE_AST_NODE_EXTERN_PROC_DECL:
        case VITTE_AST_NODE_PARAMETER:
        case VITTE_AST_NODE_GENERIC_PARAM:
        case VITTE_AST_NODE_MACRO_DECL:
        case VITTE_AST_NODE_TEST_DECL:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_ast_node_kind_is_statement(
    vitte_ast_node_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_NODE_BLOCK:
        case VITTE_AST_NODE_LET_STMT:
        case VITTE_AST_NODE_RETURN_STMT:
        case VITTE_AST_NODE_DEFER_STMT:
        case VITTE_AST_NODE_IF_STMT:
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
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_ast_node_kind_is_type(
    vitte_ast_node_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_NODE_TYPE_EXPR:
        case VITTE_AST_NODE_POINTER_TYPE:
        case VITTE_AST_NODE_REFERENCE_TYPE:
        case VITTE_AST_NODE_ARRAY_TYPE:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_ast_node_kind_is_literal(
    vitte_ast_node_kind_t kind)
{
    switch (kind) {
        case VITTE_AST_NODE_INTEGER_LITERAL:
        case VITTE_AST_NODE_FLOAT_LITERAL:
        case VITTE_AST_NODE_STRING_LITERAL:
        case VITTE_AST_NODE_CHARACTER_LITERAL:
        case VITTE_AST_NODE_BOOL_LITERAL:
        case VITTE_AST_NODE_NULL_LITERAL:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_ast_node_kind_is_expression(
    vitte_ast_node_kind_t kind)
{
    if (vitte_ast_node_kind_is_literal(kind)) {
        return true;
    }

    switch (kind) {
        case VITTE_AST_NODE_IDENTIFIER:
        case VITTE_AST_NODE_PATH:
        case VITTE_AST_NODE_SELF_EXPR:
        case VITTE_AST_NODE_GROUP_EXPR:
        case VITTE_AST_NODE_UNARY_EXPR:
        case VITTE_AST_NODE_BINARY_EXPR:
        case VITTE_AST_NODE_ASSIGN_EXPR:
        case VITTE_AST_NODE_CALL_EXPR:
        case VITTE_AST_NODE_INDEX_EXPR:
        case VITTE_AST_NODE_MEMBER_EXPR:
        case VITTE_AST_NODE_AWAIT_EXPR:
        case VITTE_AST_NODE_TRY_EXPR:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_ast_node_has_flag(
    const vitte_ast_node_t *node,
    vitte_ast_flags_t flag)
{
    return node != NULL &&
           (node->flags & flag) != 0u;
}

static inline bool
vitte_ast_node_is_public(
    const vitte_ast_node_t *node)
{
    return vitte_ast_node_has_flag(
        node,
        VITTE_AST_FLAG_PUBLIC);
}

static inline bool
vitte_ast_node_is_mutable(
    const vitte_ast_node_t *node)
{
    return vitte_ast_node_has_flag(
        node,
        VITTE_AST_FLAG_MUTABLE);
}

static inline bool
vitte_ast_node_is_external(
    const vitte_ast_node_t *node)
{
    return vitte_ast_node_has_flag(
        node,
        VITTE_AST_FLAG_EXTERN);
}

static inline bool
vitte_ast_node_is_async(
    const vitte_ast_node_t *node)
{
    return vitte_ast_node_has_flag(
        node,
        VITTE_AST_FLAG_ASYNC);
}

static inline bool
vitte_ast_node_is_unsafe(
    const vitte_ast_node_t *node)
{
    return vitte_ast_node_has_flag(
        node,
        VITTE_AST_FLAG_UNSAFE);
}

static inline bool
vitte_ast_node_is_comptime(
    const vitte_ast_node_t *node)
{
    return vitte_ast_node_has_flag(
        node,
        VITTE_AST_FLAG_COMPTIME);
}

static inline bool
vitte_ast_node_has_token(
    const vitte_ast_node_t *node)
{
    return node != NULL &&
           node->token_index !=
               VITTE_PARSER_NO_TOKEN;
}

static inline size_t
vitte_ast_node_child_count(
    const vitte_ast_node_t *node)
{
    return node != NULL
        ? node->child_count
        : 0u;
}

static inline vitte_ast_node_id_t
vitte_ast_node_child_at(
    const vitte_ast_node_t *node,
    size_t index)
{
    if (node == NULL ||
        index >= node->child_count ||
        node->children == NULL) {
        return VITTE_AST_INVALID_ID;
    }

    return node->children[index];
}

/* ========================================================================= */
/* Inline parser helpers                                                     */
/* ========================================================================= */

static inline bool
vitte_parser_is_ready(
    const vitte_parser_t *parser)
{
    return parser != NULL &&
           parser->magic == VITTE_PARSER_MAGIC &&
           parser->state ==
               VITTE_PARSER_STATE_READY;
}

static inline bool
vitte_parser_is_parsing(
    const vitte_parser_t *parser)
{
    return parser != NULL &&
           parser->magic == VITTE_PARSER_MAGIC &&
           parser->state ==
               VITTE_PARSER_STATE_PARSING;
}

static inline bool
vitte_parser_is_done(
    const vitte_parser_t *parser)
{
    return parser != NULL &&
           parser->magic == VITTE_PARSER_MAGIC &&
           parser->state ==
               VITTE_PARSER_STATE_DONE;
}

static inline bool
vitte_parser_has_failed(
    const vitte_parser_t *parser)
{
    return parser != NULL &&
           parser->magic == VITTE_PARSER_MAGIC &&
           parser->state ==
               VITTE_PARSER_STATE_FAILED;
}

static inline vitte_parser_error_t
vitte_parser_last_error(
    const vitte_parser_t *parser)
{
    if (parser == NULL ||
        parser->magic != VITTE_PARSER_MAGIC) {
        return VITTE_PARSER_ERROR_INVALID_CONTEXT;
    }

    return parser->last_error;
}

static inline size_t
vitte_parser_node_count(
    const vitte_parser_t *parser)
{
    if (parser == NULL ||
        parser->magic != VITTE_PARSER_MAGIC) {
        return 0u;
    }

    return parser->node_count;
}

static inline size_t
vitte_parser_token_count(
    const vitte_parser_t *parser)
{
    if (parser == NULL ||
        parser->magic != VITTE_PARSER_MAGIC) {
        return 0u;
    }

    return parser->token_count;
}

static inline size_t
vitte_parser_position(
    const vitte_parser_t *parser)
{
    if (parser == NULL ||
        parser->magic != VITTE_PARSER_MAGIC) {
        return 0u;
    }

    return parser->position;
}

static inline uint64_t
vitte_parser_generation(
    const vitte_parser_t *parser)
{
    if (parser == NULL ||
        parser->magic != VITTE_PARSER_MAGIC) {
        return UINT64_C(0);
    }

    return parser->generation;
}

static inline bool
vitte_parser_has_diagnostics(
    const vitte_parser_t *parser)
{
    return parser != NULL &&
           parser->magic == VITTE_PARSER_MAGIC &&
           parser->diagnostic_count != 0u;
}

static inline bool
vitte_parser_has_root(
    const vitte_parser_t *parser)
{
    return parser != NULL &&
           parser->magic == VITTE_PARSER_MAGIC &&
           parser->root !=
               VITTE_AST_INVALID_ID;
}

static inline const vitte_token_t *
vitte_parser_token_at(
    const vitte_parser_t *parser,
    size_t index)
{
    if (parser == NULL ||
        parser->magic != VITTE_PARSER_MAGIC ||
        parser->tokens == NULL ||
        index >= parser->token_count) {
        return NULL;
    }

    return &parser->tokens[index];
}

static inline const vitte_token_t *
vitte_parser_node_token(
    const vitte_parser_t *parser,
    const vitte_ast_node_t *node)
{
    if (parser == NULL ||
        node == NULL ||
        parser->magic != VITTE_PARSER_MAGIC ||
        node->token_index ==
            VITTE_PARSER_NO_TOKEN ||
        node->token_index >=
            parser->token_count ||
        parser->tokens == NULL) {
        return NULL;
    }

    return &parser->tokens[node->token_index];
}

/* ========================================================================= */
/* ABI / compile-time checks                                                 */
/* ========================================================================= */

#if defined(__cplusplus)

static_assert(
    sizeof(vitte_ast_node_id_t) == 8u,
    "vitte_ast_node_id_t must be 64-bit");

static_assert(
    VITTE_AST_INVALID_ID == UINT64_C(0),
    "AST invalid identifier must be zero");

static_assert(
    VITTE_PARSER_API_VERSION_MAJOR == 1u,
    "unexpected parser API major version");

#else

_Static_assert(
    sizeof(vitte_ast_node_id_t) == 8u,
    "vitte_ast_node_id_t must be 64-bit");

_Static_assert(
    VITTE_AST_INVALID_ID == UINT64_C(0),
    "AST invalid identifier must be zero");

_Static_assert(
    VITTE_PARSER_API_VERSION_MAJOR == 1u,
    "unexpected parser API major version");

#endif

#ifdef __cplusplus
}
#endif

#endif /* VITTE_PARSER_PARSER_H */
#endif
