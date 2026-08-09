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
    return kind == VITTE_TOKEN_KW_EXPORT ||
        kind == VITTE_TOKEN_KW_PROC ||
        kind == VITTE_TOKEN_KW_CONST;
}

static bool vitte_parser_is_top_level_start(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_KW_SPACE ||
        kind == VITTE_TOKEN_KW_USE ||
        vitte_parser_is_decl_start(kind);
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
        case VITTE_TOKEN_KW_OR:
        case VITTE_TOKEN_PIPE_PIPE:
            return "||";
        case VITTE_TOKEN_KW_AND:
        case VITTE_TOKEN_AMP_AMP:
            return "&&";
        case VITTE_TOKEN_PIPE:
            return "|";
        case VITTE_TOKEN_CARET:
            return "^";
        case VITTE_TOKEN_AMP:
            return "&";
        case VITTE_TOKEN_SHIFT_LEFT:
            return "<<";
        case VITTE_TOKEN_SHIFT_RIGHT:
            return ">>";
        case VITTE_TOKEN_PLUS:
            return "+";
        case VITTE_TOKEN_MINUS:
            return "-";
        case VITTE_TOKEN_STAR:
            return "*";
        case VITTE_TOKEN_SLASH:
            return "/";
        case VITTE_TOKEN_PERCENT:
            return "%";
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
            if (vitte_parser_is_top_level_start(parser->current.kind)) {
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

static bool vitte_parser_token_is_path_segment(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_IDENTIFIER;
}

static bool vitte_parser_token_is_path_separator(vitte_token_kind_t kind, bool allow_slash) {
    return kind == VITTE_TOKEN_DOUBLE_COLON || (allow_slash && kind == VITTE_TOKEN_SLASH);
}

static bool vitte_parser_token_text_is(const vitte_token_t *token, const char *text) {
    size_t length;

    if (token == NULL || text == NULL || token->lexeme_start == NULL) {
        return false;
    }
    length = strlen(text);
    return token->lexeme_length == length && memcmp(token->lexeme_start, text, length) == 0;
}

static char *vitte_parser_join_with_dot(vitte_parser_t *parser, const char *left, const char *right) {
    size_t left_length;
    size_t right_length;
    char *joined;

    if (parser == NULL || right == NULL) {
        return NULL;
    }
    if (left == NULL || left[0] == '\0') {
        return vitte_parser_copy_text(parser, right, strlen(right));
    }

    left_length = strlen(left);
    right_length = strlen(right);
    joined = (char *)vitte_parser_arena_alloc(parser, left_length + 1u + right_length + 1u);
    if (joined == NULL) {
        vitte_ast_span_t span = vitte_parser_span_from_token(&parser->current);
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate normalized path",
            right,
            &span
        );
        return NULL;
    }
    (void)memcpy(joined, left, left_length);
    joined[left_length] = '.';
    (void)memcpy(joined + left_length + 1u, right, right_length);
    joined[left_length + 1u + right_length] = '\0';
    return joined;
}

static char *vitte_parser_join_with_dot_token(
    vitte_parser_t *parser,
    const char *left,
    const vitte_token_t *right
) {
    char *segment;

    if (parser == NULL || right == NULL) {
        return NULL;
    }
    segment = vitte_parser_copy_token_text(parser, right);
    if (segment == NULL) {
        return NULL;
    }
    return vitte_parser_join_with_dot(parser, left, segment);
}

static bool vitte_parser_import_path_is_relative(const char *path) {
    return path != NULL &&
        (strcmp(path, "self") == 0 ||
            strcmp(path, "super") == 0 ||
            strncmp(path, "self.", 5u) == 0 ||
            strncmp(path, "super.", 6u) == 0);
}

static char *vitte_parser_import_dependency_path(
    vitte_parser_t *parser,
    const char *path,
    vitte_ast_import_kind_t import_kind
) {
    const char *last_dot;

    if (parser == NULL || path == NULL) {
        return NULL;
    }
    if (import_kind != VITTE_AST_IMPORT_SYMBOL) {
        return vitte_parser_copy_text(parser, path, strlen(path));
    }
    last_dot = strrchr(path, '.');
    if (last_dot == NULL) {
        return vitte_parser_copy_text(parser, path, strlen(path));
    }
    return vitte_parser_copy_text(parser, path, (size_t)(last_dot - path));
}

static char *vitte_parser_parse_path_text(
    vitte_parser_t *parser,
    bool allow_slash,
    const char *code,
    const char *message,
    vitte_ast_span_t *span_out
) {
    vitte_token_t first;
    vitte_ast_span_t span;
    const char *start;
    size_t length;

    if (parser == NULL) {
        return NULL;
    }
    if (!vitte_parser_token_is_path_segment(parser->current.kind)) {
        (void)vitte_parser_fail_current(parser, code, message);
        return NULL;
    }

    first = parser->current;
    span = vitte_parser_span_from_token(&first);
    (void)vitte_parser_advance(parser);

    while (vitte_parser_token_is_path_separator(parser->current.kind, allow_slash)) {
        vitte_ast_span_t separator_span = vitte_parser_span_from_token(&parser->current);
        vitte_ast_span_t segment_span;

        (void)vitte_parser_advance(parser);
        if (!vitte_parser_token_is_path_segment(parser->current.kind)) {
            (void)vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_PARSE,
                code,
                message,
                NULL,
                &separator_span
            );
            return NULL;
        }
        segment_span = vitte_parser_span_from_token(&parser->current);
        span = vitte_parser_span_merge(&span, &segment_span);
        (void)vitte_parser_advance(parser);
    }

    if (!vitte_ast_span_is_valid(&span)) {
        (void)vitte_parser_fail(parser, VITTE_STATUS_ERROR_PARSE, code, message, NULL, &span);
        return NULL;
    }

    start = first.lexeme_start;
    length = span.end_offset - first.start_offset;
    if (span_out != NULL) {
        *span_out = span;
    }
    return vitte_parser_copy_text(parser, start, length);
}

static char *vitte_parser_parse_module_path_normalized(
    vitte_parser_t *parser,
    bool allow_slash,
    const char *code,
    const char *message,
    vitte_ast_span_t *span_out
) {
    vitte_ast_span_t span;
    char *path;

    if (parser == NULL || !vitte_parser_token_is_path_segment(parser->current.kind)) {
        (void)vitte_parser_fail_current(parser, code, message);
        return NULL;
    }

    span = vitte_parser_span_from_token(&parser->current);
    path = vitte_parser_copy_token_text(parser, &parser->current);
    if (path == NULL) {
        return NULL;
    }
    (void)vitte_parser_advance(parser);

    while (vitte_parser_token_is_path_separator(parser->current.kind, allow_slash)) {
        vitte_ast_span_t separator_span = vitte_parser_span_from_token(&parser->current);
        vitte_ast_span_t segment_span;
        char *joined;

        (void)vitte_parser_advance(parser);
        if (!vitte_parser_token_is_path_segment(parser->current.kind)) {
            (void)vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_PARSE,
                code,
                message,
                NULL,
                &separator_span
            );
            return NULL;
        }
        joined = vitte_parser_join_with_dot_token(parser, path, &parser->current);
        if (joined == NULL) {
            return NULL;
        }
        path = joined;
        segment_span = vitte_parser_span_from_token(&parser->current);
        span = vitte_parser_span_merge(&span, &separator_span);
        span = vitte_parser_span_merge(&span, &segment_span);
        (void)vitte_parser_advance(parser);
    }

    if (span_out != NULL) {
        *span_out = span;
    }
    return path;
}

static vitte_ast_type_ref_t *vitte_parser_parse_type_ref(vitte_parser_t *parser) {
    vitte_ast_span_t span;
    char *name;
    vitte_ast_type_ref_t *type_ref;

    if (parser == NULL) {
        return NULL;
    }
    name = vitte_parser_parse_path_text(parser, false, "VITTE_PARSER_E_TYPE", "expected type name", &span);
    if (name == NULL) {
        return NULL;
    }
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
            text = vitte_parser_parse_path_text(parser, false, "VITTE_PARSER_E_EXPR", "expected identifier", &span);
            if (text == NULL) {
                return NULL;
            }
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
    if (parser != NULL &&
        (parser->current.kind == VITTE_TOKEN_PLUS ||
            parser->current.kind == VITTE_TOKEN_MINUS ||
            parser->current.kind == VITTE_TOKEN_BANG ||
            parser->current.kind == VITTE_TOKEN_KW_NOT)) {
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
        if (operator_token.kind == VITTE_TOKEN_BANG || operator_token.kind == VITTE_TOKEN_KW_NOT) {
            vitte_ast_span_t false_span = vitte_parser_span_from_token(&operator_token);
            vitte_ast_expr_t *false_expr = vitte_ast_make_identifier(&parser->builder, "false", false_span);
            vitte_ast_expr_t *expr;
            vitte_ast_span_t merged_span;

            if (false_expr == NULL) {
                (void)vitte_parser_fail(
                    parser,
                    VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                    "VITTE_PARSER_E_MEMORY",
                    "failed to allocate unary not seed",
                    NULL,
                    &false_span
                );
                return NULL;
            }
            parser->stats.expr_count++;
            merged_span = vitte_parser_span_merge(&false_expr->span, &right->span);
            expr = vitte_ast_make_binary_expr(&parser->builder, "==", right, false_expr, merged_span);
            if (expr == NULL) {
                (void)vitte_parser_fail(
                    parser,
                    VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                    "VITTE_PARSER_E_MEMORY",
                    "failed to allocate unary not expression",
                    NULL,
                    &merged_span
                );
                return NULL;
            }
            parser->stats.expr_count++;
            return expr;
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
    return kind == VITTE_TOKEN_STAR || kind == VITTE_TOKEN_SLASH || kind == VITTE_TOKEN_PERCENT;
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

static bool vitte_parser_is_shift_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_SHIFT_LEFT || kind == VITTE_TOKEN_SHIFT_RIGHT;
}

static bool vitte_parser_is_bitwise_and_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_AMP;
}

static bool vitte_parser_is_bitwise_xor_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_CARET;
}

static bool vitte_parser_is_bitwise_or_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_PIPE;
}

static bool vitte_parser_is_logical_and_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_AMP_AMP || kind == VITTE_TOKEN_KW_AND;
}

static bool vitte_parser_is_logical_or_op(vitte_token_kind_t kind) {
    return kind == VITTE_TOKEN_PIPE_PIPE || kind == VITTE_TOKEN_KW_OR;
}

static vitte_ast_expr_t *vitte_parser_parse_factor(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_unary, vitte_parser_is_factor_op);
}

static vitte_ast_expr_t *vitte_parser_parse_term(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_factor, vitte_parser_is_term_op);
}

static vitte_ast_expr_t *vitte_parser_parse_shift(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_term, vitte_parser_is_shift_op);
}

static vitte_ast_expr_t *vitte_parser_parse_comparison(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_shift, vitte_parser_is_compare_op);
}

static vitte_ast_expr_t *vitte_parser_parse_equality(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_comparison, vitte_parser_is_equality_op);
}

static vitte_ast_expr_t *vitte_parser_parse_bitwise_and(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_equality, vitte_parser_is_bitwise_and_op);
}

static vitte_ast_expr_t *vitte_parser_parse_bitwise_xor(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_bitwise_and, vitte_parser_is_bitwise_xor_op);
}

static vitte_ast_expr_t *vitte_parser_parse_bitwise_or(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_bitwise_xor, vitte_parser_is_bitwise_or_op);
}

static vitte_ast_expr_t *vitte_parser_parse_logical_and(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_bitwise_or, vitte_parser_is_logical_and_op);
}

static vitte_ast_expr_t *vitte_parser_parse_logical_or(vitte_parser_t *parser) {
    return vitte_parser_parse_binary_chain(parser, vitte_parser_parse_logical_and, vitte_parser_is_logical_or_op);
}

static vitte_ast_expr_t *vitte_parser_parse_expr_impl(vitte_parser_t *parser) {
    return vitte_parser_parse_logical_or(parser);
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
    bool mutable_value = false;

    keyword_span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);
    mutable_value = vitte_parser_match(parser, VITTE_TOKEN_KW_MUT);
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
    stmt = vitte_ast_make_let_stmt(&parser->builder, name, type, value, mutable_value, span);
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

static vitte_ast_stmt_t *vitte_parser_parse_expr_stmt(vitte_parser_t *parser) {
    vitte_ast_expr_t *value;
    vitte_ast_stmt_t *stmt;
    vitte_ast_span_t span;

    if (parser == NULL) {
        return NULL;
    }
    value = vitte_parser_parse_expr(parser);
    if (value == NULL) {
        return NULL;
    }
    vitte_parser_optional_semicolon(parser);
    span = value->span;
    stmt = vitte_ast_make_expr_stmt(&parser->builder, value, span);
    if (stmt == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate expression statement",
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
            return vitte_parser_parse_expr_stmt(parser);
    }
}

static vitte_status_t vitte_parser_record_import(
    vitte_parser_t *parser,
    vitte_ast_module_t *module_node,
    const char *path,
    const char *alias,
    vitte_ast_import_kind_t import_kind,
    vitte_ast_span_t span
) {
    bool relative;
    vitte_ast_decl_t *decl;

    if (parser == NULL || module_node == NULL || path == NULL || path[0] == '\0') {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    relative = vitte_parser_import_path_is_relative(path);
    decl = vitte_ast_make_import_decl(&parser->builder, path, alias, relative, import_kind, span);
    if (decl == NULL) {
        return vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate import declaration",
            path,
            &span
        );
    }
    if (!vitte_ast_module_add_import(module_node, decl)) {
        return vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_INTERNAL,
            "VITTE_PARSER_E_IMPORT",
            "failed to append import declaration",
            path,
            &span
        );
    }
    if (parser->module != NULL) {
        char *dependency_path = vitte_parser_import_dependency_path(parser, path, import_kind);
        vitte_status_t status;
        if (dependency_path == NULL) {
            return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
        }
        status = vitte_module_add_import(parser->module, dependency_path, relative);
        if (status != VITTE_STATUS_OK) {
            const vitte_error_t *error = vitte_module_last_error(parser->module);
            return vitte_parser_fail(
                parser,
                status,
                error != NULL && error->code != NULL ? error->code : "VITTE_PARSER_E_IMPORT",
                error != NULL && error->message != NULL ? error->message : "failed to register module import",
                error != NULL ? error->details : path,
                &span
            );
        }
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_parser_parse_import_item(
    vitte_parser_t *parser,
    vitte_ast_module_t *module_node,
    const char *prefix,
    vitte_ast_span_t *item_span_out
);

static vitte_status_t vitte_parser_record_export(
    vitte_parser_t *parser,
    vitte_ast_module_t *module_node,
    const char *local_name,
    const char *export_name,
    vitte_ast_span_t span
) {
    vitte_ast_decl_t *decl;

    if (parser == NULL || module_node == NULL || local_name == NULL || export_name == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    decl = vitte_ast_make_export_decl(&parser->builder, local_name, export_name, span);
    if (decl == NULL) {
        return vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate export declaration",
            export_name,
            &span
        );
    }
    if (!vitte_ast_module_add_export(module_node, decl)) {
        return vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_INTERNAL,
            "VITTE_PARSER_E_EXPORT",
            "failed to append export declaration",
            export_name,
            &span
        );
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_parser_parse_export_item(
    vitte_parser_t *parser,
    vitte_ast_module_t *module_node,
    vitte_ast_span_t *item_span_out
) {
    vitte_ast_span_t span;
    char *local_name;
    char *export_name;

    if (parser == NULL || module_node == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
        return vitte_parser_fail_current(parser, "VITTE_PARSER_E_EXPORT", "expected exported name");
    }

    span = vitte_parser_span_from_token(&parser->current);
    local_name = vitte_parser_copy_token_text(parser, &parser->current);
    if (local_name == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }
    export_name = local_name;
    (void)vitte_parser_advance(parser);

    if (vitte_parser_token_is_path_separator(parser->current.kind, true)) {
        return vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_PARSER_E_REEXPORT",
            "bootstrap re-exports are unsupported; export local declarations only",
            local_name,
            &span
        );
    }

    if (vitte_parser_match(parser, VITTE_TOKEN_KW_AS)) {
        vitte_ast_span_t alias_span;

        if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
            return vitte_parser_fail_current(parser, "VITTE_PARSER_E_EXPORT", "expected export alias after 'as'");
        }
        export_name = vitte_parser_copy_token_text(parser, &parser->current);
        if (export_name == NULL) {
            return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
        }
        alias_span = vitte_parser_span_from_token(&parser->current);
        span = vitte_parser_span_merge(&span, &alias_span);
        (void)vitte_parser_advance(parser);
    }

    if (item_span_out != NULL) {
        *item_span_out = span;
    }
    return vitte_parser_record_export(parser, module_node, local_name, export_name, span);
}

static vitte_status_t vitte_parser_parse_export_group(
    vitte_parser_t *parser,
    vitte_ast_module_t *module_node,
    vitte_ast_span_t *group_span_out
) {
    vitte_ast_span_t span;

    if (parser == NULL || module_node == NULL || parser->current.kind != VITTE_TOKEN_LBRACE) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);

    while (parser->current.kind != VITTE_TOKEN_RBRACE &&
        parser->current.kind != VITTE_TOKEN_EOF &&
        parser->current.kind != VITTE_TOKEN_ERROR) {
        vitte_ast_span_t item_span;
        vitte_status_t status = vitte_parser_parse_export_item(parser, module_node, &item_span);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        span = vitte_parser_span_merge(&span, &item_span);
        if (!vitte_parser_match(parser, VITTE_TOKEN_COMMA)) {
            break;
        }
    }

    if (!vitte_parser_expect(parser, VITTE_TOKEN_RBRACE, "VITTE_PARSER_E_EXPORT", "expected '}' after export group")) {
        return VITTE_STATUS_ERROR_PARSE;
    }
    {
        vitte_ast_span_t end_span = vitte_parser_span_from_token(&parser->previous);
        span = vitte_parser_span_merge(&span, &end_span);
    }
    if (group_span_out != NULL) {
        *group_span_out = span;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_parser_parse_import_group(
    vitte_parser_t *parser,
    vitte_ast_module_t *module_node,
    const char *prefix,
    vitte_ast_span_t *group_span_out
) {
    vitte_ast_span_t span;

    if (parser == NULL || module_node == NULL || parser->current.kind != VITTE_TOKEN_LBRACE) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);

    while (parser->current.kind != VITTE_TOKEN_RBRACE &&
        parser->current.kind != VITTE_TOKEN_EOF &&
        parser->current.kind != VITTE_TOKEN_ERROR) {
        vitte_ast_span_t item_span;
        vitte_status_t status = vitte_parser_parse_import_item(parser, module_node, prefix, &item_span);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        span = vitte_parser_span_merge(&span, &item_span);
        if (!vitte_parser_match(parser, VITTE_TOKEN_COMMA)) {
            break;
        }
    }

    if (!vitte_parser_expect(parser, VITTE_TOKEN_RBRACE, "VITTE_PARSER_E_IMPORT", "expected '}' after import group")) {
        return VITTE_STATUS_ERROR_PARSE;
    }
    {
        vitte_ast_span_t end_span = vitte_parser_span_from_token(&parser->previous);
        span = vitte_parser_span_merge(&span, &end_span);
    }
    if (group_span_out != NULL) {
        *group_span_out = span;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_parser_parse_import_item(
    vitte_parser_t *parser,
    vitte_ast_module_t *module_node,
    const char *prefix,
    vitte_ast_span_t *item_span_out
) {
    vitte_ast_span_t span;
    char *path = NULL;
    char *alias = NULL;

    if (parser == NULL || module_node == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (parser->current.kind == VITTE_TOKEN_STAR) {
        span = vitte_parser_span_from_token(&parser->current);
        if (prefix == NULL || prefix[0] == '\0') {
            return vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_PARSER_E_IMPORT",
                "glob import requires a module prefix",
                NULL,
                &span
            );
        }
        (void)vitte_parser_advance(parser);
        if (item_span_out != NULL) {
            *item_span_out = span;
        }
        return vitte_parser_record_import(parser, module_node, prefix, NULL, VITTE_AST_IMPORT_GLOB, span);
    }

    if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
        return vitte_parser_fail_current(parser, "VITTE_PARSER_E_IMPORT", "expected import item");
    }

    span = vitte_parser_span_from_token(&parser->current);
    path = prefix != NULL && prefix[0] != '\0' ?
        vitte_parser_join_with_dot_token(parser, prefix, &parser->current) :
        vitte_parser_copy_token_text(parser, &parser->current);
    if (path == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }
    (void)vitte_parser_advance(parser);

    while (vitte_parser_token_is_path_separator(parser->current.kind, true)) {
        vitte_ast_span_t separator_span = vitte_parser_span_from_token(&parser->current);
        (void)vitte_parser_advance(parser);

        if (parser->current.kind == VITTE_TOKEN_LBRACE) {
            vitte_status_t status;
            span = vitte_parser_span_merge(&span, &separator_span);
            status = vitte_parser_parse_import_group(parser, module_node, path, &span);
            if (item_span_out != NULL) {
                *item_span_out = span;
            }
            return status;
        }
        if (parser->current.kind == VITTE_TOKEN_STAR) {
            vitte_ast_span_t star_span = vitte_parser_span_from_token(&parser->current);
            span = vitte_parser_span_merge(&span, &separator_span);
            span = vitte_parser_span_merge(&span, &star_span);
            (void)vitte_parser_advance(parser);
            if (item_span_out != NULL) {
                *item_span_out = span;
            }
            return vitte_parser_record_import(
                parser,
                module_node,
                path,
                NULL,
                VITTE_AST_IMPORT_GLOB,
                span
            );
        }
        if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
            return vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_PARSER_E_IMPORT",
                "expected import path segment",
                path,
                &separator_span
            );
        }
        path = vitte_parser_join_with_dot_token(parser, path, &parser->current);
        if (path == NULL) {
            return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
        }
        span = vitte_parser_span_merge(&span, &separator_span);
        {
            vitte_ast_span_t segment_span = vitte_parser_span_from_token(&parser->current);
            span = vitte_parser_span_merge(&span, &segment_span);
        }
        (void)vitte_parser_advance(parser);
    }

    if (vitte_parser_match(parser, VITTE_TOKEN_KW_AS)) {
        vitte_ast_span_t alias_span;

        if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
            return vitte_parser_fail_current(parser, "VITTE_PARSER_E_IMPORT", "expected alias name after 'as'");
        }
        alias = vitte_parser_copy_token_text(parser, &parser->current);
        if (alias == NULL) {
            return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
        }
        alias_span = vitte_parser_span_from_token(&parser->current);
        span = vitte_parser_span_merge(&span, &alias_span);
        (void)vitte_parser_advance(parser);
    }

    if (item_span_out != NULL) {
        *item_span_out = span;
    }
    return vitte_parser_record_import(
        parser,
        module_node,
        path,
        alias,
        prefix != NULL && prefix[0] != '\0' ? VITTE_AST_IMPORT_SYMBOL : VITTE_AST_IMPORT_MODULE,
        span
    );
}

static vitte_status_t vitte_parser_parse_imports(
    vitte_parser_t *parser,
    vitte_ast_module_t *module_node,
    vitte_ast_span_t *span_out
) {
    vitte_ast_span_t use_span;
    vitte_ast_span_t item_span;
    vitte_status_t status;

    if (parser == NULL || module_node == NULL || parser->current.kind != VITTE_TOKEN_KW_USE) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    use_span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);
    if (parser->current.kind == VITTE_TOKEN_LBRACE) {
        status = vitte_parser_parse_import_group(parser, module_node, NULL, &item_span);
    } else {
        status = vitte_parser_parse_import_item(parser, module_node, NULL, &item_span);
    }
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    use_span = vitte_parser_span_merge(&use_span, &item_span);
    vitte_parser_optional_semicolon(parser);
    if (parser->previous.kind == VITTE_TOKEN_SEMICOLON) {
        vitte_ast_span_t semicolon_span = vitte_parser_span_from_token(&parser->previous);
        use_span = vitte_parser_span_merge(&use_span, &semicolon_span);
    }
    if (span_out != NULL) {
        *span_out = use_span;
    }
    return VITTE_STATUS_OK;
}

static vitte_ast_node_t *vitte_parser_parse_param(vitte_parser_t *parser) {
    vitte_ast_span_t start_span;
    vitte_ast_span_t span;
    char *name;
    vitte_ast_type_ref_t *type;
    bool mutable_value = false;
    bool by_ref = false;
    vitte_ast_node_t *param;

    if (parser == NULL) {
        return NULL;
    }

    start_span = vitte_parser_span_from_token(&parser->current);
    if (parser->current.kind == VITTE_TOKEN_IDENTIFIER && vitte_parser_token_text_is(&parser->current, "ref")) {
        by_ref = true;
        (void)vitte_parser_advance(parser);
        if (parser->current.kind == VITTE_TOKEN_KW_MUT) {
            mutable_value = true;
            (void)vitte_parser_advance(parser);
        }
    } else if (parser->current.kind == VITTE_TOKEN_KW_MUT) {
        mutable_value = true;
        (void)vitte_parser_advance(parser);
    } else if (parser->current.kind == VITTE_TOKEN_IDENTIFIER &&
        (vitte_parser_token_text_is(&parser->current, "move") ||
            vitte_parser_token_text_is(&parser->current, "copy") ||
            vitte_parser_token_text_is(&parser->current, "const"))) {
        (void)vitte_parser_advance(parser);
    }

    if (parser->current.kind != VITTE_TOKEN_IDENTIFIER) {
        (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_PARAM", "expected parameter name");
        return NULL;
    }
    name = vitte_parser_copy_token_text(parser, &parser->current);
    if (name == NULL) {
        return NULL;
    }
    {
        vitte_ast_span_t name_span = vitte_parser_span_from_token(&parser->current);
        span = vitte_parser_span_merge(&start_span, &name_span);
    }
    (void)vitte_parser_advance(parser);

    if (!vitte_parser_expect(parser, VITTE_TOKEN_COLON, "VITTE_PARSER_E_PARAM", "expected ':' after parameter name")) {
        return NULL;
    }
    type = vitte_parser_parse_type_ref(parser);
    if (type == NULL) {
        return NULL;
    }
    span = vitte_parser_span_merge(&span, &type->span);

    param = vitte_ast_make_param_decl(&parser->builder, name, type, mutable_value, by_ref, span);
    if (param == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate parameter declaration",
            name,
            &span
        );
    }
    return param;
}

static vitte_ast_decl_t *vitte_parser_parse_proc(vitte_parser_t *parser, bool exported) {
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
    decl = vitte_ast_make_proc_decl(&parser->builder, name, exported, NULL, NULL, keyword_span);
    if (decl == NULL) {
        (void)vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_PARSER_E_MEMORY",
            "failed to allocate procedure declaration",
            name,
            &keyword_span
        );
        return NULL;
    }
    if (parser->current.kind != VITTE_TOKEN_RPAREN) {
        for (;;) {
            vitte_ast_node_t *param = vitte_parser_parse_param(parser);
            if (param == NULL) {
                return NULL;
            }
            if (!vitte_ast_proc_add_param(decl, param)) {
                (void)vitte_parser_fail(
                    parser,
                    VITTE_STATUS_ERROR_INTERNAL,
                    "VITTE_PARSER_E_PARAM",
                    "failed to append procedure parameter",
                    name,
                    &param->span
                );
                return NULL;
            }
            if (!vitte_parser_match(parser, VITTE_TOKEN_COMMA)) {
                break;
            }
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
        return_type = vitte_ast_make_type_name(&parser->builder, "void", keyword_span);
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
    decl->as.proc_decl.return_type = return_type;
    decl->as.proc_decl.body = body;
    decl->span = span;
    parser->stats.decl_count++;
    return decl;
}

static vitte_ast_decl_t *vitte_parser_parse_const(vitte_parser_t *parser, bool exported) {
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
    decl = vitte_ast_make_const_decl(&parser->builder, name, exported, type, value, span);
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
    bool exported = false;

    if (parser == NULL) {
        return NULL;
    }
    if (parser->current.kind == VITTE_TOKEN_KW_EXPORT) {
        vitte_ast_span_t export_span = vitte_parser_span_from_token(&parser->current);

        exported = true;
        (void)vitte_parser_advance(parser);
        if (parser->current.kind != VITTE_TOKEN_KW_PROC &&
            parser->current.kind != VITTE_TOKEN_KW_CONST) {
            (void)vitte_parser_fail(
                parser,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_PARSER_E_EXPORT",
                "expected 'proc' or 'const' after 'export'",
                NULL,
                &export_span
            );
            return NULL;
        }
    }
    if (parser->current.kind == VITTE_TOKEN_KW_PROC) {
        return vitte_parser_parse_proc(parser, exported);
    }
    if (parser->current.kind == VITTE_TOKEN_KW_CONST) {
        return vitte_parser_parse_const(parser, exported);
    }
    (void)vitte_parser_fail_current(parser, "VITTE_PARSER_E_DECL", "expected top-level declaration");
    return NULL;
}

static vitte_status_t vitte_parser_parse_export_top_level(
    vitte_parser_t *parser,
    vitte_ast_module_t *module_node,
    vitte_ast_decl_t **decl_out,
    vitte_ast_span_t *span_out
) {
    vitte_ast_span_t export_span;
    vitte_ast_span_t clause_span;
    vitte_status_t status;

    if (parser == NULL || module_node == NULL || parser->current.kind != VITTE_TOKEN_KW_EXPORT) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (decl_out != NULL) {
        *decl_out = NULL;
    }

    export_span = vitte_parser_span_from_token(&parser->current);
    clause_span = export_span;
    (void)vitte_parser_advance(parser);

    if (parser->current.kind == VITTE_TOKEN_KW_PROC) {
        vitte_ast_decl_t *decl = vitte_parser_parse_proc(parser, true);
        if (decl == NULL) {
            return VITTE_STATUS_ERROR_PARSE;
        }
        decl->span = vitte_parser_span_merge(&export_span, &decl->span);
        clause_span = decl->span;
        if (decl_out != NULL) {
            *decl_out = decl;
        }
    } else if (parser->current.kind == VITTE_TOKEN_KW_CONST) {
        vitte_ast_decl_t *decl = vitte_parser_parse_const(parser, true);
        if (decl == NULL) {
            return VITTE_STATUS_ERROR_PARSE;
        }
        decl->span = vitte_parser_span_merge(&export_span, &decl->span);
        clause_span = decl->span;
        if (decl_out != NULL) {
            *decl_out = decl;
        }
    } else if (parser->current.kind == VITTE_TOKEN_STAR) {
        vitte_ast_span_t star_span = vitte_parser_span_from_token(&parser->current);
        vitte_ast_module_set_export_all(module_node, true);
        clause_span = vitte_parser_span_merge(&export_span, &star_span);
        (void)vitte_parser_advance(parser);
        vitte_parser_optional_semicolon(parser);
        if (parser->previous.kind == VITTE_TOKEN_SEMICOLON) {
            vitte_ast_span_t semicolon_span = vitte_parser_span_from_token(&parser->previous);
            clause_span = vitte_parser_span_merge(&clause_span, &semicolon_span);
        }
    } else if (parser->current.kind == VITTE_TOKEN_LBRACE) {
        status = vitte_parser_parse_export_group(parser, module_node, &clause_span);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        clause_span = vitte_parser_span_merge(&export_span, &clause_span);
        vitte_parser_optional_semicolon(parser);
        if (parser->previous.kind == VITTE_TOKEN_SEMICOLON) {
            vitte_ast_span_t semicolon_span = vitte_parser_span_from_token(&parser->previous);
            clause_span = vitte_parser_span_merge(&clause_span, &semicolon_span);
        }
    } else if (parser->current.kind == VITTE_TOKEN_IDENTIFIER) {
        status = vitte_parser_parse_export_item(parser, module_node, &clause_span);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        clause_span = vitte_parser_span_merge(&export_span, &clause_span);
        vitte_parser_optional_semicolon(parser);
        if (parser->previous.kind == VITTE_TOKEN_SEMICOLON) {
            vitte_ast_span_t semicolon_span = vitte_parser_span_from_token(&parser->previous);
            clause_span = vitte_parser_span_merge(&clause_span, &semicolon_span);
        }
    } else {
        return vitte_parser_fail(
            parser,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_PARSER_E_EXPORT",
            "expected '*', '{', exported name, 'proc', or 'const' after 'export'",
            NULL,
            &export_span
        );
    }

    if (span_out != NULL) {
        *span_out = clause_span;
    }
    return VITTE_STATUS_OK;
}

static char *vitte_parser_parse_module_header(vitte_parser_t *parser, vitte_ast_span_t *header_span) {
    char *name;
    vitte_ast_span_t span;

    if (parser == NULL || parser->current.kind != VITTE_TOKEN_KW_SPACE) {
        return NULL;
    }

    span = vitte_parser_span_from_token(&parser->current);
    (void)vitte_parser_advance(parser);
    name = vitte_parser_parse_module_path_normalized(
        parser,
        true,
        "VITTE_PARSER_E_MODULE",
        "expected module path after 'space'",
        &span
    );
    if (name == NULL) {
        return NULL;
    }
    vitte_parser_optional_semicolon(parser);
    if (header_span != NULL) {
        *header_span = span;
    }
    return name;
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
    vitte_ast_span_t header_span;
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
    vitte_ast_span_init(&header_span);

    if (parser->current.kind == VITTE_TOKEN_KW_SPACE) {
        module_name = vitte_parser_parse_module_header(parser, &header_span);
        if (module_name != NULL) {
            module_span = vitte_parser_span_merge(&module_span, &header_span);
        }
    } else {
        module_name = NULL;
    }
    if (module_name == NULL) {
        const char *fallback_name = parser->module != NULL && parser->module->module_name[0] != '\0' ?
            parser->module->module_name :
            parser->lexer.source_name;
        module_name = vitte_parser_copy_text(parser, fallback_name, strlen(fallback_name));
    }
    if (module_name == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }
    if (parser->module != NULL && header_span.valid) {
        status = vitte_module_set_name(parser->module, module_name);
        if (status != VITTE_STATUS_OK) {
            const vitte_error_t *error = vitte_module_last_error(parser->module);
            return vitte_parser_fail(
                parser,
                status,
                error != NULL && error->code != NULL ? error->code : "VITTE_PARSER_E_MODULE",
                error != NULL && error->message != NULL ? error->message : "failed to set module name",
                error != NULL ? error->details : module_name,
                &header_span
            );
        }
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
            vitte_ast_span_t import_span;
            bool have_import_span = false;

            if (parser->current.kind == VITTE_TOKEN_SEMICOLON) {
                (void)vitte_parser_advance(parser);
                continue;
            }
            if (parser->current.kind == VITTE_TOKEN_KW_USE) {
                status = vitte_parser_parse_imports(parser, module_node, &import_span);
                if (status != VITTE_STATUS_OK) {
                    if (!parser->options.recover_errors) {
                        break;
                    }
                    vitte_parser_synchronize(parser, true);
                    continue;
                }
                have_import_span = true;
            } else if (parser->current.kind == VITTE_TOKEN_KW_EXPORT) {
                vitte_ast_span_t export_span;
                status = vitte_parser_parse_export_top_level(parser, module_node, &decl, &export_span);
                if (status != VITTE_STATUS_OK) {
                    if (!parser->options.recover_errors) {
                        break;
                    }
                    vitte_parser_synchronize(parser, true);
                    continue;
                }
                have_import_span = true;
                import_span = export_span;
                if (decl != NULL && !vitte_ast_module_add_decl(module_node, decl)) {
                    status = vitte_parser_fail(
                        parser,
                        VITTE_STATUS_ERROR_INTERNAL,
                        "VITTE_PARSER_E_MODULE",
                        "failed to append exported module declaration",
                        module_name,
                        &decl->span
                    );
                    break;
                }
            } else {
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
            }
            if (have_import_span) {
                module_node->span = vitte_parser_span_merge(&module_node->span, &import_span);
            } else {
                module_node->span = vitte_parser_span_merge(&module_node->span, &decl->span);
            }
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
