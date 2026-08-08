#include "parser.h"

#include <string.h>

static void vitte_parser_set_error(
    vitte_parser_t *parser,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (parser == NULL) {
        return;
    }
    vitte_error_set_details(&parser->last_error, status, code, message, details);
    if (parser->module != NULL) {
        vitte_error_copy(&parser->module->last_error, &parser->last_error);
        parser->module->state = VITTE_MODULE_STATE_FAILED;
    }
}

static vitte_status_t vitte_parser_add_diagnostic(
    vitte_parser_t *parser,
    vitte_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
) {
    if (parser == NULL || parser->diagnostics == NULL) {
        return VITTE_STATUS_OK;
    }
    return vitte_diagnostic_add(parser->diagnostics, severity, code, message, details, span);
}

static vitte_ast_span_t vitte_parser_span_from_token(const vitte_token_t *token) {
    vitte_ast_span_t span;

    vitte_ast_span_init(&span);
    if (token == NULL) {
        return span;
    }
    span.source_name = token->source_name;
    span.start_offset = token->start_offset;
    span.end_offset = token->end_offset;
    span.start_line = token->start_line;
    span.start_column = token->start_column;
    span.end_line = token->end_line;
    span.end_column = token->end_column;
    span.valid = true;
    return span;
}

static vitte_ast_span_t vitte_parser_span_merge(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right
) {
    vitte_ast_span_t merged;

    vitte_ast_span_init(&merged);
    if (vitte_ast_span_merge(left, right, &merged)) {
        return merged;
    }
    if (vitte_ast_span_is_valid(left)) {
        return *left;
    }
    if (vitte_ast_span_is_valid(right)) {
        return *right;
    }
    return merged;
}

static vitte_status_t vitte_parser_fail(
    vitte_parser_t *parser,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
) {
    if (parser == NULL) {
        return status;
    }
    parser->stats.error_count++;
    vitte_parser_set_error(parser, status, code, message, details);
    (void)vitte_parser_add_diagnostic(parser, VITTE_DIAGNOSTIC_ERROR, code, message, details, span);
    return status;
}

static vitte_status_t vitte_parser_fail_current(
    vitte_parser_t *parser,
    const char *code,
    const char *message
) {
    vitte_ast_span_t span = vitte_parser_span_from_token(parser != NULL ? &parser->current : NULL);
    const char *details = parser != NULL ? vitte_token_kind_name(parser->current.kind) : NULL;
    return vitte_parser_fail(parser, VITTE_STATUS_ERROR_PARSE, code, message, details, &span);
}

static bool vitte_parser_enter_depth(vitte_parser_t *parser) {
    vitte_ast_span_t span;

    if (parser == NULL) {
        return false;
    }
    parser->depth++;
    if (parser->depth > parser->stats.max_depth_reached) {
        parser->stats.max_depth_reached = parser->depth;
    }
    if (parser->depth > parser->options.max_depth) {
        span = vitte_parser_span_from_token(&parser->current);
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_INVALID_STATE,
            "VITTE_PARSER_E_DEPTH",
            "parser recursion depth exceeded",
            parser->current.source_name,
            &span
        );
        parser->depth--;
        return false;
    }
    return true;
}

static void vitte_parser_leave_depth(vitte_parser_t *parser) {
    if (parser != NULL && parser->depth > 0u) {
        parser->depth--;
    }
}

static bool vitte_parser_is_decl_start(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_KW_PROC || kind == VITTE_TOKEN_KW_CONST;
}

static bool vitte_parser_is_stmt_start(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_LBRACE ||
        kind == VITTE_TOKEN_KW_GIVE ||
        kind == VITTE_TOKEN_KW_LET ||
        kind == VITTE_TOKEN_KW_IF;
}

static void *vitte_parser_arena_alloc(vitte_parser_t *parser, size_t size) {
    if (parser == NULL || parser->ast == NULL || !vitte_ast_is_initialized(parser->ast)) {
        return NULL;
    }
    return vitte_arena_alloc(parser->ast->arena, size, 1u);
}

static char *vitte_parser_copy_text(vitte_parser_t *parser, const char *text, size_t length) {
    char *copy;

    if (parser == NULL || text == NULL) {
        return NULL;
    }
    copy = (char *)vitte_parser_arena_alloc(parser, length + 1u);
    if (copy == NULL) {
        vitte_ast_span_t span = vitte_parser_span_from_token(&parser->current);
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate parser string",
            NULL,
            &span
        );
        return NULL;
    }
    if (length > 0u) {
        (void)memcpy(copy, text, length);
    }
    copy[length] = '\0';
    return copy;
}

static char *vitte_parser_copy_token_text(vitte_parser_t *parser, const vitte_token_t *token) {
    if (token == NULL || token->lexeme_start == NULL) {
        return NULL;
    }
    return vitte_parser_copy_text(parser, token->lexeme_start, token->lexeme_length);
}

static char *vitte_parser_decode_string(vitte_parser_t *parser, const vitte_token_t *token) {
    size_t index;
    size_t write_index = 0u;
    size_t raw_length;
    char *decoded;

    if (parser == NULL || token == NULL || token->lexeme_start == NULL || token->lexeme_length < 2u) {
        return NULL;
    }
    raw_length = token->lexeme_length - 2u;
    decoded = (char *)vitte_parser_arena_alloc(parser, raw_length + 1u);
    if (decoded == NULL) {
        vitte_ast_span_t span = vitte_parser_span_from_token(token);
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate decoded string literal",
            NULL,
            &span
        );
        return NULL;
    }

    for (index = 1u; index + 1u < token->lexeme_length; index++) {
        char value = token->lexeme_start[index];
        if (value == '\\' && index + 2u < token->lexeme_length) {
            index++;
            switch (token->lexeme_start[index]) {
                case 'n':
                    decoded[write_index++] = '\n';
                    break;
                case 'r':
                    decoded[write_index++] = '\r';
                    break;
                case 't':
                    decoded[write_index++] = '\t';
                    break;
                case '\\':
                    decoded[write_index++] = '\\';
                    break;
                case '"':
                    decoded[write_index++] = '"';
                    break;
                default:
                    decoded[write_index++] = token->lexeme_start[index];
                    break;
            }
        } else {
            decoded[write_index++] = value;
        }
    }
    decoded[write_index] = '\0';
    return decoded;
}

static const char *vitte_parser_operator_text(vitte_token_kind_t kind) {
    switch (kind) {
        case VITTE_TOKEN_PLUS:
            return "+";
        case VITTE_TOKEN_MINUS:
            return "-";
        case VITTE_TOKEN_STAR:
            return "*";
        case VITTE_TOKEN_SLASH:
            return "/";
        case VITTE_TOKEN_EQUAL_EQUAL:
            return "==";
        case VITTE_TOKEN_BANG_EQUAL:
            return "!=";
        case VITTE_TOKEN_LESS:
            return "<";
        case VITTE_TOKEN_LESS_EQUAL:
            return "<=";
        case VITTE_TOKEN_GREATER:
            return ">";
        case VITTE_TOKEN_GREATER_EQUAL:
            return ">=";
        default:
            return NULL;
    }
}

static vitte_status_t vitte_parser_advance(vitte_parser_t *parser) {
    vitte_status_t status;

    if (parser == NULL || !parser->initialized) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    parser->previous = parser->current;
    status = vitte_lexer_next(&parser->lexer, &parser->current);
    parser->stats.token_count++;
    if (status != VITTE_STATUS_OK) {
        vitte_ast_span_t span = vitte_parser_span_from_token(&parser->current);
        const vitte_error_t *lexer_error = vitte_lexer_last_error(&parser->lexer);
        return vitte_parser_fail(
            parser,
            status,
            lexer_error != NULL && lexer_error->code != NULL ? lexer_error->code : "VITTE_PARSER_E_LEX",
            lexer_error != NULL && lexer_error->message != NULL ? lexer_error->message : "lexer failed during parsing",
            lexer_error != NULL ? lexer_error->details : parser->current.message,
            &span
        );
    }
    return VITTE_STATUS_OK;
}

static bool vitte_parser_match(vitte_parser_t *parser, vitte_token_kind_t kind) {
    if (parser == NULL || parser->current.kind != kind) {
        return false;
    }
    (void)vitte_parser_advance(parser);
    return true;
}

static bool vitte_parser_expect(
    vitte_parser_t *parser,
    vitte_token_kind_t kind,
    const char *code,
    const char *message
) {
    if (parser != NULL && parser->current.kind == kind) {
        (void)vitte_parser_advance(parser);
        return true;
    }
    (void)vitte_parser_fail_current(parser, code, message);
    return false;
}

static void vitte_parser_optional_semicolon(vitte_parser_t *parser) {
    if (parser == NULL) {
        return;
    }
    if (parser->options.require_semicolons) {
        (void)vitte_parser_expect(parser, VITTE_TOKEN_SEMICOLON, "VITTE_PARSER_E_SEMICOLON", "expected ';'");
    } else if (parser->current.kind == VITTE_TOKEN_SEMICOLON) {
        (void)vitte_parser_advance(parser);
    }
}

static void vitte_parser_synchronize(vitte_parser_t *parser, bool top_level) {
    if (parser == NULL || !parser->options.recover_errors) {
        return;
    }
    parser->stats.recovery_count++;
    while (parser->current.kind != VITTE_TOKEN_EOF && parser->current.kind != VITTE_TOKEN_ERROR) {
        if (top_level) {
            if (vitte_parser_is_decl_start(parser->current.kind)) {
                return;
            }
        } else if (parser->current.kind == VITTE_TOKEN_RBRACE || vitte_parser_is_stmt_start(parser->current.kind)) {
            return;
        }
        if (parser->current.kind == VITTE_TOKEN_SEMICOLON) {
            (void)vitte_parser_advance(parser);
            return;
        }
        (void)vitte_parser_advance(parser);
    }
}

static vitte_ast_type_ref_t *vitte_parser_parse_type_ref(vitte_parser_t *parser) {
    vitte_token_t name_token;
    vitte_ast_span_t span;
    char *name;
    vitte_ast_type_ref_t *type_ref;

    if (parser == NULL) {
        return NULL;
    }
    if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
        (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_TYPE", "expected type name");
        return NULL;
    }

    name_token = parser->current;
    span = vitte_parser_span_from_token(&name_token);
    name = vitte_parser_copy_token_text(parser, &name_token);
    if (name == NULL) {
        return NULL;
    }
    (void)vitte_parser_advance(parser);
    type_ref = vitte_ast_make_type_name(&parser->builder, name, span);
    if (type_ref == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate type reference",
            name,
            &span
        );
        return NULL;
    }
    return type_ref;
}

static vitte_ast_expr_t *vitte_parser_parse_expr_impl(vitte_parser_t *parser);

static vitte_ast_expr_t *vitte_parser_parse_primary(vitte_parser_t *parser) {
    vitte_token_t token;
    vitte_ast_span_t span;
    vitte_ast_expr_t *expr;
    char *text;

    if (parser == NULL) {
        return NULL;
    }

    token = parser->current;
    span = vitte_parser_span_from_token(&token);
    switch (token.kind) {
        case VITTE_TOKEN_INTEGER:
            (void)vitte_parser_advance(parser);
            expr = vitte_ast_make_integer_literal(&parser->builder, token.integer_value, span);
            break;
        case VITTE_TOKEN_STRING:
            text = vitte_parser_decode_string(parser, &token);
            if (text == NULL) {
                return NULL;
            }
            (void)vitte_parser_advance(parser);
            expr = vitte_ast_make_string_literal(&parser->builder, text, span);
            break;
        case VITTE_TOKEN_IDENTIFIER:
            text = vitte_parser_copy_token_text(parser, &token);
            if (text == NULL) {
                return NULL;
            }
            (void)vitte_parser_advance(parser);
            expr = vitte_ast_make_identifier(&parser->builder, text, span);
            break;
        case VITTE_TOKEN_LPAREN:
            (void)vitte_parser_advance(parser);
            expr = vitte_parser_parse_expr_impl(parser);
            if (expr == NULL) {
                return NULL;
            }
            if (!vitte_parser_expect(parser, VITTE_TOKEN_RPAREN, "VITTE_PARSER_E_GROUP", "expected ')' after grouped expression")) {
                return NULL;
            }
            return expr;
        default:
            (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_EXPR", "expected expression");
            return NULL;
    }

    if (expr == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate primary expression",
            NULL,
            &span
        );
        return NULL;
    }
    parser->stats.expr_count++;
    return expr;
}

static vitte_ast_expr_t *vitte_parser_parse_call(vitte_parser_t *parser) {
    vitte_ast_expr_t *callee;

    callee = vitte_parser_parse_primary(parser);
    if (callee == NULL) {
        return NULL;
    }

    while (parser->current.kind == VITTE_TOKEN_LPAREN) {
        vitte_ast_span_t call_span = callee->span;
        vitte_ast_expr_t *call;

        (void)vitte_parser_advance(parser);
        call = vitte_ast_make_call_expr(&parser->builder, callee, call_span);
        if (call == NULL) {
            (void)vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                "VITTE_PARSER_E_MEMORY",
                "failed to allocate call expression",
                NULL,
                &call_span
            );
            return NULL;
        }
        parser->stats.expr_count++;

        if (parser->current.kind != VITTE_TOKEN_RPAREN) {
            for (;;) {
                vitte_ast_expr_t *argument = vitte_parser_parse_expr_impl(parser);
                if (argument == NULL) {
                    return NULL;
                }
                if (!vitte_ast_call_add_arg(call, argument)) {
                    (void)vitte_parser_fail(
                        parser,
                        VITTE_STATUS_ERROR_INTERNAL,
                        "VITTE_PARSER_E_CALL",
                        "failed to append call argument",
                        NULL,
                        &call_span
                    );
                    return NULL;
                }
                if (!vitte_parser_match(parser, VITTE_TOKEN_COMMA)) {
                    break;
                }
            }
        }
        if (!vitte_parser_expect(parser, VITTE_TOKEN_RPAREN, "VITTE_PARSER_E_CALL", "expected ')' after call arguments")) {
            return NULL;
        }
        {
            vitte_ast_span_t close_span = vitte_parser_span_from_token(&parser->previous);
            call->span = vitte_parser_span_merge(&call->span, &close_span);
        }
        callee = call;
    }

    return callee;
}

static vitte_ast_expr_t *vitte_parser_parse_unary(vitte_parser_t *parser) {
    if (parser != NULL && (parser->current.kind == VITTE_TOKEN_PLUS || parser->current.kind == VITTE_TOKEN_MINUS)) {
        vitte_token_t operator_token = parser->current;
        vitte_ast_expr_t *right;

        (void)vitte_parser_advance(parser);
        right = vitte_parser_parse_unary(parser);
        if (right == NULL) {
            return NULL;
        }
        if (operator_token.kind == VITTE_TOKEN_PLUS) {
            return right;
        }
        {
            vitte_ast_span_t zero_span = vitte_parser_span_from_token(&operator_token);
            vitte_ast_expr_t *zero = vitte_ast_make_integer_literal(&parser->builder, 0, zero_span);
            vitte_ast_expr_t *expr;
            vitte_ast_span_t span;

            if (zero == NULL) {
                (void)vitte_parser_fail(
                    parser,
                    VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                    "VITTE_PARSER_E_MEMORY",
                    "failed to allocate unary expression seed",
                    NULL,
                    &zero_span
                );
                return NULL;
            }
            parser->stats.expr_count++;
            span = vitte_parser_span_merge(&zero->span, &right->span);
            expr = vitte_ast_make_binary_expr(&parser->builder, "-", zero, right, span);
            if (expr == NULL) {
                (void)vitte_parser_fail(
                    parser,
                    VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                    "VITTE_PARSER_E_MEMORY",
                    "failed to allocate unary expression",
                    NULL,
                    &span
                );
                return NULL;
            }
            parser->stats.expr_count++;
            return expr;
        }
    }
    return vitte_parser_parse_call(parser);
}

static vitte_ast_expr_t *vitte_parser_parse_binary_chain(
    vitte_parser_t *parser,
    vitte_ast_expr_t *(*operand)(vitte_parser_t *parser),
    bool (*accept)(vitte_token_kind_t kind)
) {
    vitte_ast_expr_t *left;

    left = operand(parser);
    if (left == NULL) {
        return NULL;
    }

    while (parser != NULL && accept(parser->current.kind)) {
        vitte_token_kind_t operator_kind = parser->current.kind;
        vitte_ast_span_t span;
        vitte_ast_expr_t *right;
        vitte_ast_expr_t *expr;

        (void)vitte_parser_advance(parser);
        right = operand(parser);
        if (right == NULL) {
            return NULL;
        }
        span = vitte_parser_span_merge(&left->span, &right->span);
        expr = vitte_ast_make_binary_expr(
            &parser->builder,
            vitte_parser_operator_text(operator_kind),
            left,
            right,
            span
        );
        if (expr == NULL) {
            (void)vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                "VITTE_PARSER_E_MEMORY",
                "failed to allocate binary expression",
                vitte_parser_operator_text(operator_kind),
                &span
            );
            return NULL;
        }
        parser->stats.expr_count++;
        left = expr;
    }
    return left;
}

static bool vitte_parser_is_factor_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_STAR || kind == VITTE_TOKEN_SLASH;
}

static bool vitte_parser_is_term_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_PLUS || kind == VITTE_TOKEN_MINUS;
}

static bool vitte_parser_is_compare_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_LESS ||
        kind == VITTE_TOKEN_LESS_EQUAL ||
        kind == VITTE_TOKEN_GREATER ||
        kind == VITTE_TOKEN_GREATER_EQUAL;
}

static bool vitte_parser_is_equality_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_EQUAL_EQUAL || kind == VITTE_TOKEN_BANG_EQUAL;
}

static vitte_ast_expr_t *vitte_parser_parse_factor(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_unary, vitte_parser_is_factor_op);
}

static vitte_ast_expr_t *vitte_parser_parse_term(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_factor, vitte_parser_is_term_op);
}

static vitte_ast_expr_t *vitte_parser_parse_comparison(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_term, vitte_parser_is_compare_op);
}

static vitte_ast_expr_t *vitte_parser_parse_equality(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_comparison, vitte_parser_is_equality_op);
}

static vitte_ast_expr_t *vitte_parser_parse_expr_impl(vitte_parser_t *parser) {
    return vitte_parser_parse_equality(parser);
}

static vitte_ast_type_ref_t *vitte_parser_infer_expr_type(
    vitte_parser_t *parser,
    const vitte_ast_expr_t *expr
) {
    const char *name = NULL;
    vitte_ast_span_t span;
    const char *operator_text;

    if (parser == NULL || expr == NULL) {
        return NULL;
    }
    switch (expr->kind) {
        case VITTE_AST_NODE_INTEGER_LITERAL:
            name = "int";
            break;
        case VITTE_AST_NODE_STRING_LITERAL:
            name = "string";
            break;
        case VITTE_AST_NODE_BINARY_EXPR:
            operator_text = expr->as.binary_expr.operator_text;
            if (operator_text != NULL &&
                (strcmp(operator_text, "==") == 0 ||
                    strcmp(operator_text, "!=") == 0 ||
                    strcmp(operator_text, "<") == 0 ||
                    strcmp(operator_text, "<=") == 0 ||
                    strcmp(operator_text, ">") == 0 ||
                    strcmp(operator_text, ">=") == 0)) {
                name = "bool";
            } else {
                name = "int";
            }
            break;
        default:
            break;
    }
    if (name == NULL) {
        return NULL;
    }
    span = expr->span;
    return vitte_ast_make_type_name(&parser->builder, name, span);
}

static vitte_ast_stmt_t *vitte_parser_parse_stmt_impl(vitte_parser_t *parser);

static vitte_ast_stmt_t *vitte_parser_parse_block(vitte_parser_t *parser) {
    vitte_ast_span_t open_span;
    vitte_ast_stmt_t *block;

    if (parser == NULL || parser->current.kind != VITTE_TOKEN_LBRACE) {
        (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_BLOCK", "expected '{' to start block");
        return NULL;
    }
    open_span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);

    block = vitte_ast_make_block_stmt(&parser->builder, open_span);
    if (block == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate block statement",
            NULL,
            &open_span
        );
        return NULL;
    }
    parser->stats.stmt_count++;

    while (parser->current.kind != VITTE_TOKEN_RBRACE &&
        parser->current.kind != VITTE_TOKEN_EOF &&
        parser->current.kind != VITTE_TOKEN_ERROR) {
        vitte_ast_stmt_t *stmt;

        if (parser->current.kind == VITTE_TOKEN_SEMICOLON) {
            (void)vitte_parser_advance(parser);
            continue;
        }
        stmt = vitte_parser_parse_stmt(parser);
        if (stmt == NULL) {
            if (!parser->options.recover_errors) {
                return NULL;
            }
            vitte_parser_synchronize(parser, false);
            continue;
        }
        if (!vitte_ast_block_add_stmt(block, stmt)) {
            (void)vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_INTERNAL,
                "VITTE_PARSER_E_BLOCK",
                "failed to append block statement",
                NULL,
                &block->span
            );
            return NULL;
        }
    }

    if (!vitte_parser_expect(parser, VITTE_TOKEN_RBRACE, "VITTE_PARSER_E_BLOCK", "expected '}' to close block")) {
        return NULL;
    }
    {
        vitte_ast_span_t close_span = vitte_parser_span_from_token(&parser->previous);
        block->span = vitte_parser_span_merge(&open_span, &close_span);
    }
    return block;
}

static vitte_ast_stmt_t *vitte_parser_parse_give(vitte_parser_t *parser) {
    vitte_ast_span_t keyword_span;
    vitte_ast_expr_t *value = NULL;
    vitte_ast_stmt_t *stmt;

    keyword_span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);

    if (parser->current.kind != VITTE_TOKEN_SEMICOLON &&
        parser->current.kind != VITTE_TOKEN_RBRACE &&
        parser->current.kind != VITTE_TOKEN_EOF) {
        value = vitte_parser_parse_expr(parser);
        if (value == NULL) {
            return NULL;
        }
    }
    vitte_parser_optional_semicolon(parser);

    stmt = vitte_ast_make_give_stmt(&parser->builder, value, value != NULL ? vitte_parser_span_merge(&keyword_span, &value->span) : keyword_span);
    if (stmt == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate give statement",
            NULL,
            &keyword_span
        );
        return NULL;
    }
    parser->stats.stmt_count++;
    return stmt;
}

static vitte_ast_stmt_t *vitte_parser_parse_let(vitte_parser_t *parser) {
    vitte_ast_span_t keyword_span;
    vitte_token_t name_token;
    char *name;
    vitte_ast_type_ref_t *type = NULL;
    vitte_ast_expr_t *value = NULL;
    vitte_ast_stmt_t *stmt;
    vitte_ast_span_t span;

    keyword_span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);
    if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
        (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_LET", "expected identifier after 'let'");
        return NULL;
    }
    name_token = parser->current;
    name = vitte_parser_copy_token_text(parser, &name_token);
    if (name == NULL) {
        return NULL;
    }
    (void)vitte_parser_advance(parser);

    if (vitte_parser_match(parser, VITTE_TOKEN_COLON)) {
        type = vitte_parser_parse_type_ref(parser);
        if (type == NULL) {
            return NULL;
        }
    } else {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_PARSER_E_LET",
            "bootstrap let statement requires an explicit type",
            name,
            &keyword_span
        );
        return NULL;
    }

    if (vitte_parser_match(parser, VITTE_TOKEN_EQUAL)) {
        value = vitte_parser_parse_expr(parser);
        if (value == NULL) {
            return NULL;
        }
    }
    vitte_parser_optional_semicolon(parser);

    span = type != NULL ? vitte_parser_span_merge(&keyword_span, &type->span) : keyword_span;
    if (value != NULL) {
        span = vitte_parser_span_merge(&span, &value->span);
    }
    stmt = vitte_ast_make_let_stmt(&parser->builder, name, type, value, span);
    if (stmt == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate let statement",
            name,
            &span
        );
        return NULL;
    }
    parser->stats.stmt_count++;
    return stmt;
}

static vitte_ast_stmt_t *vitte_parser_parse_if(vitte_parser_t *parser) {
    vitte_ast_span_t keyword_span;
    vitte_ast_expr_t *condition;
    vitte_ast_stmt_t *then_branch;
    vitte_ast_stmt_t *else_branch = NULL;
    vitte_ast_stmt_t *stmt;
    vitte_ast_span_t span;

    keyword_span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);
    condition = vitte_parser_parse_expr(parser);
    if (condition == NULL) {
        return NULL;
    }
    then_branch = vitte_parser_parse_stmt(parser);
    if (then_branch == NULL) {
        return NULL;
    }
    if (vitte_parser_match(parser, VITTE_TOKEN_KW_ELSE)) {
        else_branch = parser->current.kind == VITTE_TOKEN_KW_IF ?
            vitte_parser_parse_if(parser) :
            vitte_parser_parse_stmt(parser);
        if (else_branch == NULL) {
            return NULL;
        }
    }

    span = vitte_parser_span_merge(&keyword_span, &then_branch->span);
    if (else_branch != NULL) {
        span = vitte_parser_span_merge(&span, &else_branch->span);
    } else {
        span = vitte_parser_span_merge(&span, &condition->span);
    }
    stmt = vitte_ast_make_if_stmt(&parser->builder, condition, then_branch, else_branch, span);
    if (stmt == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate if statement",
            NULL,
            &span
        );
        return NULL;
    }
    parser->stats.stmt_count++;
    return stmt;
}

static vitte_ast_stmt_t *vitte_parser_parse_stmt_impl(vitte_parser_t *parser) {
    if (parser == NULL) {
        return NULL;
    }
    switch (parser->current.kind) {
        case VITTE_TOKEN_LBRACE:
            return vitte_parser_parse_block(parser);
        case VITTE_TOKEN_KW_GIVE:
            return vitte_parser_parse_give(parser);
        case VITTE_TOKEN_KW_LET:
            return vitte_parser_parse_let(parser);
        case VITTE_TOKEN_KW_IF:
            return vitte_parser_parse_if(parser);
        default:
            (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_STMT", "expected statement");
            return NULL;
    }
}

static vitte_ast_decl_t *vitte_parser_parse_proc(vitte_parser_t *parser) {
    vitte_ast_span_t keyword_span;
    vitte_token_t name_token;
    char *name;
    vitte_ast_type_ref_t *return_type = NULL;
    vitte_ast_stmt_t *body;
    vitte_ast_decl_t *decl;
    vitte_ast_span_t span;

    keyword_span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);
    if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
        (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_PROC", "expected procedure name");
        return NULL;
    }
    name_token = parser->current;
    name = vitte_parser_copy_token_text(parser, &name_token);
    if (name == NULL) {
        return NULL;
    }
    (void)vitte_parser_advance(parser);

    if (!vitte_parser_expect(parser, VITTE_TOKEN_LPAREN, "VITTE_PARSER_E_PROC", "expected '(' after procedure name")) {
        return NULL;
    }
    if (parser->current.kind != VITTE_TOKEN_RPAREN) {
        vitte_ast_span_t span_params = vitte_parser_span_from_token(&parser->current);
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_UNSUPPORTED,
            "VITTE_PARSER_E_PARAM",
            "bootstrap parser does not support procedure parameters yet",
            name,
            &span_params
        );
        while (parser->current.kind != VITTE_TOKEN_RPAREN &&
            parser->current.kind != VITTE_TOKEN_EOF &&
            parser->current.kind != VITTE_TOKEN_ERROR) {
            (void)vitte_parser_advance(parser);
        }
    }
    if (!vitte_parser_expect(parser, VITTE_TOKEN_RPAREN, "VITTE_PARSER_E_PROC", "expected ')' after procedure parameters")) {
        return NULL;
    }

    if (vitte_parser_match(parser, VITTE_TOKEN_ARROW)) {
        return_type = vitte_parser_parse_type_ref(parser);
        if (return_type == NULL) {
            return NULL;
        }
    } else {
        return_type = vitte_ast_make_type_name(&parser->builder, "int", keyword_span);
        if (return_type == NULL) {
            (void)vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                "VITTE_PARSER_E_MEMORY",
                "failed to allocate default return type",
                name,
                &keyword_span
            );
            return NULL;
        }
    }

    body = vitte_parser_parse_block(parser);
    if (body == NULL) {
        return NULL;
    }

    span = vitte_parser_span_merge(&keyword_span, &body->span);
    decl = vitte_ast_make_proc_decl(&parser->builder, name, return_type, body, span);
    if (decl == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate procedure declaration",
            name,
            &span
        );
        return NULL;
    }
    parser->stats.decl_count++;
    return decl;
}

static vitte_ast_decl_t *vitte_parser_parse_const(vitte_parser_t *parser) {
    vitte_ast_span_t keyword_span;
    vitte_token_t name_token;
    char *name;
    vitte_ast_type_ref_t *type = NULL;
    vitte_ast_expr_t *value;
    vitte_ast_decl_t *decl;
    vitte_ast_span_t span;

    keyword_span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);
    if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
        (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_CONST", "expected constant name");
        return NULL;
    }
    name_token = parser->current;
    name = vitte_parser_copy_token_text(parser, &name_token);
    if (name == NULL) {
        return NULL;
    }
    (void)vitte_parser_advance(parser);

    if (vitte_parser_match(parser, VITTE_TOKEN_COLON)) {
        type = vitte_parser_parse_type_ref(parser);
        if (type == NULL) {
            return NULL;
        }
    }
    if (!vitte_parser_expect(parser, VITTE_TOKEN_EQUAL, "VITTE_PARSER_E_CONST", "expected '=' in constant declaration")) {
        return NULL;
    }
    value = vitte_parser_parse_expr(parser);
    if (value == NULL) {
        return NULL;
    }
    if (type == NULL) {
        type = vitte_parser_infer_expr_type(parser, value);
        if (type == NULL) {
            (void)vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_PARSER_E_CONST",
                "constant declaration requires an explicit type for this expression",
                name,
                &value->span
            );
            return NULL;
        }
    }
    vitte_parser_optional_semicolon(parser);

    span = vitte_parser_span_merge(&keyword_span, &value->span);
    decl = vitte_ast_make_const_decl(&parser->builder, name, type, value, span);
    if (decl == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate constant declaration",
            name,
            &span
        );
        return NULL;
    }
    parser->stats.decl_count++;
    return decl;
}

static vitte_ast_decl_t *vitte_parser_parse_decl_impl(vitte_parser_t *parser) {
    if (parser == NULL) {
        return NULL;
    }
    if (parser->current.kind == VITTE_TOKEN_KW_PROC) {
        return vitte_parser_parse_proc(parser);
    }
    if (parser->current.kind == VITTE_TOKEN_KW_CONST) {
        return vitte_parser_parse_const(parser);
    }
    (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_DECL", "expected top-level declaration");
    return NULL;
}

void vitte_parser_options_init(vitte_parser_options_t *options) {
    if (options == NULL) {
        return;
    }
    memset(options, 0, sizeof(*options));
    options->max_depth = VITTE_PARSER_DEFAULT_MAX_DEPTH;
    options->recover_errors = true;
    vitte_lexer_options_init(&options->lexer_options);
}

void vitte_parser_stats_init(vitte_parser_stats_t *stats) {
    if (stats != NULL) {
        memset(stats, 0, sizeof(*stats));
    }
}

void vitte_parser_result_init(vitte_parser_result_t *result) {
    if (result == NULL) {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
    vitte_error_init(&result->last_error);
}

vitte_status_t vitte_parser_init(
    vitte_parser_t *parser,
    vitte_ast_t *ast,
    const char *source_name,
    const char *source,
    size_t source_size,
    const vitte_parser_options_t *options,
    vitte_diagnostic_bag_t *diagnostics
) {
    vitte_parser_options_t defaults;
    vitte_status_t status;

    if (parser == NULL || ast == NULL || source == NULL || !vitte_ast_is_initialized(ast)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(parser, 0, sizeof(*parser));
    parser->ast = ast;
    parser->diagnostics = diagnostics;
    vitte_error_init(&parser->last_error);
    vitte_parser_stats_init(&parser->stats);
    if (options == NULL) {
        vitte_parser_options_init(&defaults);
        parser->options = defaults;
    } else {
        parser->options = *options;
    }
    if (parser->options.max_depth == 0u) {
        parser->options.max_depth = VITTE_PARSER_DEFAULT_MAX_DEPTH;
    }
    parser->options.lexer_options.emit_comments = false;
    parser->options.lexer_options.emit_whitespace = false;
    parser->options.lexer_options.keywords_enabled = true;
    vitte_ast_builder_init(&parser->builder, parser->ast);
    vitte_token_init(&parser->current);
    vitte_token_init(&parser->previous);

    status = vitte_lexer_init(
        &parser->lexer,
        source_name != NULL ? source_name : "<memory>",
        source,
        source_size,
        &parser->options.lexer_options
    );
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&parser->last_error, vitte_lexer_last_error(&parser->lexer));
        return status;
    }

    parser->initialized = true;
    status = vitte_parser_advance(parser);
    if (status != VITTE_STATUS_OK) {
        parser->initialized = false;
        return status;
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_parser_init_module(
    vitte_parser_t *parser,
    vitte_module_t *module,
    vitte_ast_t *ast,
    const vitte_parser_options_t *options,
    vitte_diagnostic_bag_t *diagnostics
) {
    vitte_ast_t *effective_ast = ast;
    vitte_status_t status;

    if (parser == NULL || module == NULL || !vitte_module_is_initialized(module) || module->source_buffer == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (effective_ast == NULL) {
        effective_ast = module->ast;
    }
    if (effective_ast == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_parser_init(
        parser,
        effective_ast,
        module->source_name != NULL ? module->source_name : module->source_path,
        module->source_buffer,
        module->source_size,
        options,
        diagnostics
    );
    if (status == VITTE_STATUS_OK) {
        parser->module = module;
    }
    return status;
}

void vitte_parser_destroy(vitte_parser_t *parser) {
    if (parser == NULL) {
        return;
    }
    memset(parser, 0, sizeof(*parser));
}

bool vitte_parser_is_initialized(const vitte_parser_t *parser) {
    return parser != NULL && parser->initialized && parser->ast != NULL;
}

const vitte_error_t *vitte_parser_last_error(const vitte_parser_t *parser) {
    return parser != NULL ? &parser->last_error : vitte_error_last();
}

const vitte_parser_stats_t *vitte_parser_stats(const vitte_parser_t *parser) {
    return parser != NULL ? &parser->stats : NULL;
}

vitte_status_t vitte_parser_parse_module(vitte_parser_t *parser, vitte_parser_result_t *result) {
    vitte_ast_module_t *module_node;
    vitte_ast_span_t module_span;
    char *module_name;
    vitte_status_t status = VITTE_STATUS_OK;

    if (result != NULL) {
        vitte_parser_result_init(result);
    }
    if (!vitte_parser_is_initialized(parser) || parser->ast->root != NULL || parser->ast->node_count != 0u) {
        if (result != NULL) {
            result->status = VITTE_STATUS_ERROR_INVALID_STATE;
            vitte_error_set_details(&result->last_error, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_PARSER_E_STATE", "parser requires an empty initialized AST", NULL);
        }
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_ast_span_init(&module_span);
    module_span.source_name = parser->lexer.source_name;
    module_span.start_offset = 0u;
    module_span.end_offset = 0u;
    module_span.start_line = 1u;
    module_span.start_column = 1u;
    module_span.end_line = 1u;
    module_span.end_column = 1u;
    module_span.valid = true;
    module_name = vitte_parser_copy_text(
        parser,
        parser->module != NULL && parser->module->module_name[0] != '\0' ? parser->module->module_name : parser->lexer.source_name,
        strlen(parser->module != NULL && parser->module->module_name[0] != '\0' ? parser->module->module_name : parser->lexer.source_name)
    );
    if (module_name == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    module_node = vitte_ast_make_module(&parser->builder, module_name, module_span);
    if (module_node == NULL) {
        status = vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate module root",
            module_name,
            &module_span
        );
    } else {
        while (status == VITTE_STATUS_OK &&
            parser->current.kind != VITTE_TOKEN_EOF &&
            parser->current.kind != VITTE_TOKEN_ERROR) {
            vitte_ast_decl_t *decl;

            if (parser->current.kind == VITTE_TOKEN_SEMICOLON) {
                (void)vitte_parser_advance(parser);
                continue;
            }
            decl = vitte_parser_parse_decl(parser);
            if (decl == NULL) {
                status = VITTE_STATUS_ERROR_PARSE;
                if (!parser->options.recover_errors) {
                    break;
                }
                vitte_parser_synchronize(parser, true);
                continue;
            }
            if (!vitte_ast_module_add_decl(module_node, decl)) {
                status = vitte_parser_fail(
                    parser,
                    VITTE_STATUS_ERROR_INTERNAL,
                    "VITTE_PARSER_E_MODULE",
                    "failed to append module declaration",
                    module_name,
                    &decl->span
                );
                break;
            }
            module_node->span = vitte_parser_span_merge(&module_node->span, &decl->span);
        }
    }

    if (status == VITTE_STATUS_OK && parser->current.kind == VITTE_TOKEN_ERROR) {
        status = VITTE_STATUS_ERROR_PARSE;
    }
    if (status == VITTE_STATUS_OK && parser->stats.error_count > 0u) {
        status = VITTE_STATUS_ERROR_PARSE;
    }
    if (status == VITTE_STATUS_OK) {
        vitte_error_reset(&parser->last_error);
    }

    if (parser->module != NULL) {
        parser->module->ast = parser->ast;
        parser->module->token_count = parser->stats.token_count;
        parser->module->stats.token_count = parser->stats.token_count;
        parser->module->state = status == VITTE_STATUS_OK ? VITTE_MODULE_STATE_PARSED : VITTE_MODULE_STATE_FAILED;
        if (status == VITTE_STATUS_OK) {
            vitte_error_reset(&parser->module->last_error);
        }
    }
    if (result != NULL) {
        result->status = status;
        result->root = module_node;
        result->token_count = parser->stats.token_count;
        result->decl_count = parser->stats.decl_count;
        result->stmt_count = parser->stats.stmt_count;
        result->expr_count = parser->stats.expr_count;
        result->error_count = parser->stats.error_count;
        if (status == VITTE_STATUS_OK) {
            vitte_error_reset(&result->last_error);
        } else {
            vitte_error_copy(&result->last_error, &parser->last_error);
        }
    }
    return status;
}

vitte_ast_decl_t *vitte_parser_parse_decl(vitte_parser_t *parser) {
    vitte_ast_decl_t *decl;

    if (!vitte_parser_enter_depth(parser)) {
        return NULL;
    }
    decl = vitte_parser_parse_decl_impl(parser);
    vitte_parser_leave_depth(parser);
    return decl;
}

vitte_ast_stmt_t *vitte_parser_parse_stmt(vitte_parser_t *parser) {
    vitte_ast_stmt_t *stmt;

    if (!vitte_parser_enter_depth(parser)) {
        return NULL;
    }
    stmt = vitte_parser_parse_stmt_impl(parser);
    vitte_parser_leave_depth(parser);
    return stmt;
}

vitte_ast_expr_t *vitte_parser_parse_expr(vitte_parser_t *parser) {
    vitte_ast_expr_t *expr;

    if (!vitte_parser_enter_depth(parser)) {
        return NULL;
    }
    expr = vitte_parser_parse_expr_impl(parser);
    vitte_parser_leave_depth(parser);
    return expr;
}
