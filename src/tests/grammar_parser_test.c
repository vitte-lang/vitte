#include "lexer/lexer.h"
#include "parser/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
parse_source(
    const char *source,
    vitte_lexer_t *lexer,
    vitte_parser_t *parser)
{
    if (!vitte_lexer_init(
            lexer,
            source,
            strlen(source),
            UINT32_C(1))) {
        return 0;
    }

    if (!vitte_lexer_run(lexer)) {
        vitte_lexer_destroy(lexer);
        return 0;
    }

    if (!vitte_parser_init(
            parser,
            lexer->tokens,
            lexer->token_count)) {
        vitte_lexer_destroy(lexer);
        return 0;
    }

    return 1;
}

static void
destroy_frontend(
    vitte_lexer_t *lexer,
    vitte_parser_t *parser)
{
    vitte_parser_destroy(parser);
    vitte_lexer_destroy(lexer);
}

static const vitte_ast_node_t *
find_kind(
    const vitte_parser_t *parser,
    vitte_ast_node_id_t id,
    vitte_ast_node_kind_t kind)
{
    const vitte_ast_node_t *node;
    size_t i;

    node = vitte_parser_get_node(parser, id);
    if (node == NULL) {
        return NULL;
    }
    if (node->kind == kind) {
        return node;
    }

    for (i = 0u; i < node->child_count; ++i) {
        const vitte_ast_node_t *found;

        found = find_kind(parser, node->children[i], kind);
        if (found != NULL) {
            return found;
        }
    }

    return NULL;
}

static const vitte_ast_node_t *
find_operator(
    const vitte_parser_t *parser,
    vitte_ast_node_id_t id,
    vitte_token_kind_t operator_kind)
{
    const vitte_ast_node_t *node;
    size_t i;

    node = vitte_parser_get_node(parser, id);
    if (node == NULL) {
        return NULL;
    }

    if (node->kind == VITTE_AST_NODE_BINARY_EXPR &&
        node->operator_kind == operator_kind) {
        return node;
    }

    for (i = 0u; i < node->child_count; ++i) {
        const vitte_ast_node_t *found;

        found = find_operator(parser, node->children[i], operator_kind);
        if (found != NULL) {
            return found;
        }
    }

    return NULL;
}

static const vitte_ast_node_t *
find_named_child(
    const vitte_parser_t *parser,
    const vitte_ast_node_t *parent,
    vitte_ast_node_kind_t kind,
    const char *name)
{
    size_t i;
    size_t name_length = strlen(name);

    if (parent == NULL) {
        return NULL;
    }
    for (i = 0u; i < parent->child_count; ++i) {
        const vitte_ast_node_t *child =
            vitte_parser_get_node(parser, parent->children[i]);
        const vitte_ast_node_t *name_node;
        const vitte_token_t *token;

        if (child == NULL || child->kind != kind ||
            child->child_count == 0u) {
            continue;
        }
        name_node = vitte_parser_get_node(parser, child->children[0]);
        if (name_node == NULL ||
            name_node->token_index >= parser->token_count) {
            continue;
        }
        token = &parser->tokens[name_node->token_index];
        if (token->lexeme != NULL && token->length == name_length &&
            memcmp(token->lexeme, name, name_length) == 0) {
            return child;
        }
    }
    return NULL;
}

static const vitte_ast_node_t *
find_pattern_flag(
    const vitte_parser_t *parser,
    vitte_ast_node_id_t id,
    vitte_ast_flags_t flag)
{
    const vitte_ast_node_t *node;
    size_t index;

    node = vitte_parser_get_node(parser, id);
    if (node == NULL) {
        return NULL;
    }
    if (node->kind == VITTE_AST_NODE_PATTERN &&
        (node->flags & flag) != 0u) {
        return node;
    }
    for (index = 0u; index < node->child_count; ++index) {
        const vitte_ast_node_t *found = find_pattern_flag(
            parser, node->children[index], flag);
        if (found != NULL) {
            return found;
        }
    }
    return NULL;
}

static int
check_precedence(void)
{
    static const char source[] =
        "space tests::precedence;\n"
        "proc precedence() {\n"
        "  left + right * third << high == equal and truth or alternative;\n"
        "}\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    const vitte_ast_node_t *or_node;
    const vitte_ast_node_t *and_node;
    const vitte_ast_node_t *equal_node;
    const vitte_ast_node_t *shift_node;
    const vitte_ast_node_t *add_node;
    const vitte_ast_node_t *multiply_node;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] failed to initialize parser\n");
        return 0;
    }

    ok = vitte_parser_run(&parser);
    if (!ok) {
        (void)fprintf(stderr, "[GRAMMAR] precedence source did not parse\n");
        destroy_frontend(&lexer, &parser);
        return 0;
    }

    or_node = find_operator(
        &parser,
        vitte_parser_root(&parser),
        VITTE_TOKEN_KW_OR);
    and_node = (or_node != NULL && or_node->child_count == 2u)
                   ? find_operator(&parser, or_node->children[0],
                                   VITTE_TOKEN_KW_AND)
                   : NULL;
    equal_node = (and_node != NULL && and_node->child_count == 2u)
                     ? find_operator(&parser, and_node->children[0],
                                     VITTE_TOKEN_EQUAL_EQUAL)
                     : NULL;
    shift_node = (equal_node != NULL && equal_node->child_count == 2u)
                     ? find_operator(&parser, equal_node->children[0],
                                     VITTE_TOKEN_SHIFT_LEFT)
                     : NULL;
    add_node = (shift_node != NULL && shift_node->child_count == 2u)
                   ? find_operator(&parser, shift_node->children[0],
                                   VITTE_TOKEN_PLUS)
                   : NULL;
    multiply_node = (add_node != NULL && add_node->child_count == 2u)
                        ? find_operator(&parser, add_node->children[1],
                                        VITTE_TOKEN_STAR)
                        : NULL;

    if (or_node == NULL || and_node == NULL || equal_node == NULL ||
        shift_node == NULL || add_node == NULL || multiply_node == NULL) {
        (void)fprintf(
            stderr,
            "[GRAMMAR] operator precedence/associativity AST mismatch\n");
        ok = 0;
    }

    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
check_division_is_not_a_path_separator(void)
{
    static const char source[] =
        "space tests::arithmetic;\n"
        "proc arithmetic() {\n"
        "  left / right;\n"
        "  left * right;\n"
        "}\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(
            stderr,
            "[GRAMMAR] failed to initialize arithmetic parser test\n");
        return 0;
    }

    ok = vitte_parser_run(&parser);
    if (!ok ||
        find_operator(
            &parser,
            vitte_parser_root(&parser),
            VITTE_TOKEN_SLASH) == NULL ||
        find_operator(
            &parser,
            vitte_parser_root(&parser),
            VITTE_TOKEN_STAR) == NULL) {
        (void)fprintf(
            stderr,
            "[GRAMMAR] division or multiplication parsed as a path\n");
        ok = 0;
    }

    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
check_syntax_surface(void)
{
    static const char source[] =
        "space parser/surface;\n"
        "use math/core::{sum as add, value};\n"
        "export { local as public_local, other };\n"
        "export { trailing_comma, };\n"
        "export *;\n"
        "form Pair { left: int, right: int }\n"
        "pick Mode { Fast, Safe }\n"
        "type Box<T> = T;\n"
        "export type PublicBox<T> = T;\n"
        "intrinsic write_file(path: string) -> int;\n"
        "export proc calculate(value: ref int) -> int\n"
        "  requires value > 0;\n"
        "  ensures value > 0;\n"
        "{\n"
        "  set result = add(value);\n"
        "  return result;\n"
        "}\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] failed to initialize syntax surface\n");
        return 0;
    }

    ok = vitte_parser_run(&parser);
    if (!ok) {
        const vitte_parser_diagnostic_t *diagnostic;

        diagnostic = vitte_parser_diagnostic_count(&parser) != 0u
                         ? vitte_parser_diagnostic_at(&parser, 0u)
                         : NULL;
        if (diagnostic != NULL) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] syntax surface rejected: %s at %zu:%zu "
                "(expected %s, found %s)\n",
                vitte_parser_error_name(diagnostic->error),
                diagnostic->span.line,
                diagnostic->span.column,
                vitte_token_kind_name(diagnostic->expected),
                vitte_token_kind_name(diagnostic->found));
        } else {
            (void)fprintf(stderr, "[GRAMMAR] syntax surface rejected\n");
        }
    }

    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
check_slice_and_cast_syntax(void)
{
    static const char source[] =
        "space tests::slice_cast;\n"
        "form Buffer { values: [i64] }\n"
        "proc convert(value: f64) -> i64 {\n"
        "  return value as i64;\n"
        "}\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    const vitte_ast_node_t *slice_type;
    const vitte_ast_node_t *cast_expression;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] failed to initialize slice/cast test\n");
        return 0;
    }

    ok = vitte_parser_run(&parser);
    slice_type = find_kind(
        &parser,
        vitte_parser_root(&parser),
        VITTE_AST_NODE_ARRAY_TYPE);
    cast_expression = find_kind(
        &parser,
        vitte_parser_root(&parser),
        VITTE_AST_NODE_CAST_EXPR);
    if (!ok && vitte_parser_diagnostic_count(&parser) != 0u) {
        const vitte_parser_diagnostic_t *diagnostic;

        diagnostic = vitte_parser_diagnostic_at(&parser, 0u);
        if (diagnostic != NULL) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] slice/cast parse error: %s at %zu:%zu "
                "(expected %s, found %s)\n",
                vitte_parser_error_name(diagnostic->error),
                diagnostic->span.line,
                diagnostic->span.column,
                vitte_token_kind_name(diagnostic->expected),
                vitte_token_kind_name(diagnostic->found));
        }
    }
    if (!ok || slice_type == NULL || slice_type->child_count != 1u ||
        cast_expression == NULL || cast_expression->child_count != 2u) {
        (void)fprintf(
            stderr,
            "[GRAMMAR] slice/cast AST shape mismatch "
            "(parsed=%d, slice children=%zu, cast children=%zu)\n",
            ok,
            slice_type != NULL ? slice_type->child_count : 0u,
            cast_expression != NULL ? cast_expression->child_count : 0u);
        ok = 0;
    }

    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
check_keyword_named_procedure(void)
{
    static const char source[] =
        "space tests::keyword_names;\n"
        "proc null() -> int { return 0; }\n"
        "proc declared() -> int;\n"
        "proc use_null() -> int { return null(); }\n"
        "proc use_null_literal() { set value = null; }\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    const vitte_ast_node_t *call;
    const vitte_ast_node_t *callee;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(
            stderr,
            "[GRAMMAR] failed to initialize keyword-name test\n");
        return 0;
    }

    ok = vitte_parser_run(&parser);
    call = ok
        ? find_kind(
              &parser,
              vitte_parser_root(&parser),
              VITTE_AST_NODE_CALL_EXPR)
        : NULL;
    callee = call != NULL && call->child_count != 0u
        ? vitte_parser_get_node(&parser, call->children[0])
        : NULL;
    if (!ok) {
        const vitte_parser_diagnostic_t *diagnostic;

        diagnostic = vitte_parser_diagnostic_count(&parser) != 0u
                         ? vitte_parser_diagnostic_at(&parser, 0u)
                         : NULL;
        if (diagnostic != NULL) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] keyword-name source rejected: %s at %zu:%zu\n",
                vitte_parser_error_name(diagnostic->error),
                diagnostic->span.line,
                diagnostic->span.column);
        }
    } else if (callee == NULL ||
               callee->kind != VITTE_AST_NODE_PATH) {
        (void)fprintf(
            stderr,
            "[GRAMMAR] null() was parsed as a null literal, not a procedure call\n");
        ok = 0;
    }

    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
check_invalid_syntax(void)
{
    static const char *const invalid_sources[] = {
        "proc broken( { }",
        "proc broken() { return 1 + ; }",
        "proc broken() { let value = ; }",
        "form Broken { first: int second: int }"
    };
    size_t i;
    int ok;

    ok = 1;
    for (i = 0u; i < sizeof(invalid_sources) / sizeof(invalid_sources[0]); ++i) {
        vitte_lexer_t lexer;
        vitte_parser_t parser;

        if (!parse_source(invalid_sources[i], &lexer, &parser)) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] failed to initialize invalid syntax case %zu\n",
                i + 1u);
            ok = 0;
            continue;
        }

        if (vitte_parser_run(&parser)) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] invalid syntax case %zu was accepted\n",
                i + 1u);
            ok = 0;
        } else {
            size_t diagnostic_index;
            size_t source_length = strlen(invalid_sources[i]);

            if (vitte_parser_diagnostic_count(&parser) == 0u ||
                vitte_parser_last_error(&parser) == VITTE_PARSER_ERROR_NONE ||
                !vitte_parser_has_failed(&parser) ||
                !vitte_parser_validate(&parser)) {
                (void)fprintf(
                    stderr,
                    "[GRAMMAR] invalid syntax case %zu lost error/AST state\n",
                    i + 1u);
                ok = 0;
            }
            for (diagnostic_index = 0u;
                 diagnostic_index < vitte_parser_diagnostic_count(&parser);
                 ++diagnostic_index) {
                const vitte_parser_diagnostic_t *diagnostic =
                    vitte_parser_diagnostic_at(&parser, diagnostic_index);
                if (diagnostic == NULL ||
                    diagnostic->kind != VITTE_PARSER_DIAGNOSTIC_ERROR ||
                    !diagnostic->span.valid ||
                    diagnostic->span.file_id != UINT32_C(1) ||
                    diagnostic->span.begin > diagnostic->span.end ||
                    diagnostic->span.end > source_length ||
                    diagnostic->span.line == 0u ||
                    diagnostic->span.column == 0u) {
                    (void)fprintf(
                        stderr,
                        "[GRAMMAR] invalid syntax case %zu has a malformed diagnostic\n",
                        i + 1u);
                    ok = 0;
                    break;
                }
            }
        }

        destroy_frontend(&lexer, &parser);
    }

    return ok;
}

static int
check_parser_limits_and_reset(void)
{
    static const char first_source[] =
        "space tests::first; proc first() {}";
    static const char second_source[] =
        "space tests::second; proc second() { return 1; }";
    static const char node_limit_source[] =
        "space tests::limited; proc limited() {}";
    static const char recursion_limit_source[] =
        "space tests::recursion; proc nested() { return (((1))); }";
    static const char diagnostic_limit_source[] =
        "proc broken() { let first = ; let second = ; }";
    vitte_lexer_t first_lexer;
    vitte_lexer_t second_lexer;
    vitte_lexer_t limited_lexer;
    vitte_parser_t parser;
    uint64_t generation;
    vitte_parser_error_t parser_error;
    int ok = 1;

    if (!parse_source(first_source, &first_lexer, &parser) ||
        !vitte_parser_run(&parser) ||
        !vitte_parser_validate(&parser) ||
        !vitte_lexer_init(
            &second_lexer, second_source, sizeof(second_source) - 1u, 2u) ||
        !vitte_lexer_run(&second_lexer)) {
        (void)fprintf(stderr, "[GRAMMAR] parser reset setup failed\n");
        return 0;
    }
    generation = vitte_parser_generation(&parser);
    if (!vitte_parser_reset(
            &parser, second_lexer.tokens, second_lexer.token_count) ||
        vitte_parser_generation(&parser) != generation + UINT64_C(1) ||
        !vitte_parser_is_ready(&parser) ||
        !vitte_parser_run(&parser) ||
        !vitte_parser_validate(&parser) ||
        vitte_parser_diagnostic_count(&parser) != 0u) {
        (void)fprintf(stderr, "[GRAMMAR] parser reset did not clear and rerun cleanly\n");
        ok = 0;
    }
    destroy_frontend(&second_lexer, &parser);
    vitte_lexer_destroy(&first_lexer);

    if (!vitte_lexer_init(
            &limited_lexer,
            node_limit_source,
            sizeof(node_limit_source) - 1u,
            3u) ||
        !vitte_lexer_run(&limited_lexer) ||
        !vitte_parser_init(
            &parser, limited_lexer.tokens, limited_lexer.token_count) ||
        !vitte_parser_set_limits(&parser, 1u, 8u, 32u, 32u)) {
        (void)fprintf(stderr, "[GRAMMAR] node-limit setup failed\n");
        vitte_lexer_destroy(&limited_lexer);
        return 0;
    }
    {
        int parse_ok = vitte_parser_run(&parser);
        parser_error = vitte_parser_last_error(&parser);
        if (parse_ok ||
            parser_error != VITTE_PARSER_ERROR_NODE_LIMIT ||
            !vitte_parser_has_failed(&parser)) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] parser node limit was not enforced cleanly "
                "(parsed=%d, error=%d, state=%d, nodes=%zu)\n",
                parse_ok,
                (int)parser_error,
                (int)parser.state,
                vitte_parser_node_count(&parser));
            ok = 0;
        }
    }
    destroy_frontend(&limited_lexer, &parser);

    if (!vitte_lexer_init(
            &limited_lexer,
            recursion_limit_source,
            sizeof(recursion_limit_source) - 1u,
            4u) ||
        !vitte_lexer_run(&limited_lexer) ||
        !vitte_parser_init(
            &parser, limited_lexer.tokens, limited_lexer.token_count) ||
        !vitte_parser_set_limits(&parser, 256u, 8u, 1u, 32u)) {
        (void)fprintf(stderr, "[GRAMMAR] recursion-limit setup failed\n");
        vitte_lexer_destroy(&limited_lexer);
        return 0;
    }
    if (vitte_parser_run(&parser) ||
        vitte_parser_last_error(&parser) != VITTE_PARSER_ERROR_RECURSION_LIMIT ||
        !vitte_parser_has_failed(&parser)) {
        (void)fprintf(stderr, "[GRAMMAR] parser recursion limit was not enforced cleanly\n");
        ok = 0;
    }
    destroy_frontend(&limited_lexer, &parser);

    if (!vitte_lexer_init(
            &limited_lexer,
            diagnostic_limit_source,
            sizeof(diagnostic_limit_source) - 1u,
            5u) ||
        !vitte_lexer_run(&limited_lexer) ||
        !vitte_parser_init(
            &parser, limited_lexer.tokens, limited_lexer.token_count) ||
        !vitte_parser_set_limits(&parser, 256u, 1u, 32u, 32u)) {
        (void)fprintf(stderr, "[GRAMMAR] diagnostic-limit setup failed\n");
        vitte_lexer_destroy(&limited_lexer);
        return 0;
    }
    if (vitte_parser_run(&parser) ||
        vitte_parser_last_error(&parser) != VITTE_PARSER_ERROR_DIAGNOSTIC_LIMIT ||
        vitte_parser_diagnostic_count(&parser) > 1u ||
        !vitte_parser_has_failed(&parser)) {
        (void)fprintf(stderr, "[GRAMMAR] parser diagnostic limit was not enforced cleanly\n");
        ok = 0;
    }
    destroy_frontend(&limited_lexer, &parser);
    return ok;
}

static int
check_parser_diagnostic_details(void)
{
    typedef struct diagnostic_case {
        const char *source;
        vitte_parser_error_t error;
        vitte_token_kind_t expected;
        vitte_token_kind_t found;
    } diagnostic_case_t;
    static const diagnostic_case_t cases[] = {
        {
            "proc broken() { let value = ; }",
            VITTE_PARSER_ERROR_INVALID_LET_STATEMENT,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc broken(value: ) {}",
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_PARAMETERS,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_RIGHT_PAREN
        },
        {
            "form Broken { field int }",
            VITTE_PARSER_ERROR_INVALID_FORM_DECLARATION,
            VITTE_TOKEN_COLON,
            VITTE_TOKEN_IDENTIFIER
        },
        {
            "type Bad = proc(int bool);",
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_TYPE,
            VITTE_TOKEN_COMMA,
            VITTE_TOKEN_IDENTIFIER
        },
        {
            "type Bad = &;",
            VITTE_PARSER_ERROR_INVALID_REFERENCE_TYPE,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "type Bad = *;",
            VITTE_PARSER_ERROR_INVALID_POINTER_TYPE,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "type Bad = [int; ];",
            VITTE_PARSER_ERROR_INVALID_ARRAY_TYPE,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_RIGHT_BRACKET
        },
        {
            "type Bad = [];",
            VITTE_PARSER_ERROR_INVALID_SLICE_TYPE,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_RIGHT_BRACKET
        },
        {
            "type Bad = ?;",
            VITTE_PARSER_ERROR_INVALID_OPTIONAL_TYPE,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "type Bad = Item::;",
            VITTE_PARSER_ERROR_INVALID_TYPE_PATH,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "type Bad = Item: Other;",
            VITTE_PARSER_ERROR_INVALID_TYPE_PATH,
            VITTE_TOKEN_COLON_COLON,
            VITTE_TOKEN_COLON
        },
        {
            "type Bad = Map<int bool>;",
            VITTE_PARSER_ERROR_INVALID_GENERIC_ARGUMENTS,
            VITTE_TOKEN_COMMA,
            VITTE_TOKEN_IDENTIFIER
        },
        {
            "type Bad = Map[int bool];",
            VITTE_PARSER_ERROR_INVALID_GENERIC_ARGUMENTS,
            VITTE_TOKEN_COMMA,
            VITTE_TOKEN_IDENTIFIER
        },
        {
            "proc bad() { let = 1; }",
            VITTE_PARSER_ERROR_INVALID_LET_STATEMENT,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_EQUAL
        },
        {
            "proc bad() { let value: ; }",
            VITTE_PARSER_ERROR_INVALID_LET_STATEMENT,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = ; }",
            VITTE_PARSER_ERROR_INVALID_LET_STATEMENT,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { set value; }",
            VITTE_PARSER_ERROR_INVALID_ASSIGNMENT_OPERATOR,
            VITTE_TOKEN_EQUAL,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { set value == 1; }",
            VITTE_PARSER_ERROR_INVALID_ASSIGNMENT_OPERATOR,
            VITTE_TOKEN_EQUAL,
            VITTE_TOKEN_EQUAL_EQUAL
        },
        {
            "proc bad() { set value = ; }",
            VITTE_PARSER_ERROR_INVALID_SET_STATEMENT,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { return 1 2; }",
            VITTE_PARSER_ERROR_INVALID_RETURN_STATEMENT,
            VITTE_TOKEN_SEMICOLON,
            VITTE_TOKEN_INTEGER_LITERAL
        },
        {
            "proc bad() { break value; }",
            VITTE_PARSER_ERROR_INVALID_JUMP_STATEMENT,
            VITTE_TOKEN_SEMICOLON,
            VITTE_TOKEN_IDENTIFIER
        },
        {
            "proc bad() { let value = 1 }",
            VITTE_PARSER_ERROR_INVALID_LET_STATEMENT,
            VITTE_TOKEN_SEMICOLON,
            VITTE_TOKEN_RIGHT_BRACE
        },
        {
            "proc bad() { set value = 1 }",
            VITTE_PARSER_ERROR_INVALID_SET_STATEMENT,
            VITTE_TOKEN_SEMICOLON,
            VITTE_TOKEN_RIGHT_BRACE
        },
        {
            "proc bad() { break }",
            VITTE_PARSER_ERROR_INVALID_JUMP_STATEMENT,
            VITTE_TOKEN_SEMICOLON,
            VITTE_TOKEN_RIGHT_BRACE
        },
        {
            "return 1;",
            VITTE_PARSER_ERROR_INVALID_RETURN_STATEMENT,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_KW_RETURN
        },
        {
            "break;",
            VITTE_PARSER_ERROR_INVALID_JUMP_STATEMENT,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_KW_BREAK
        },
        {
            "proc bad() { defer; }",
            VITTE_PARSER_ERROR_INVALID_DEFER_STATEMENT,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { assert true; }",
            VITTE_PARSER_ERROR_INVALID_ASSERT_STATEMENT,
            VITTE_TOKEN_LEFT_PAREN,
            VITTE_TOKEN_KW_TRUE
        },
        {
            "proc bad() { unsafe value; }",
            VITTE_PARSER_ERROR_INVALID_UNSAFE_STATEMENT,
            VITTE_TOKEN_LEFT_BRACE,
            VITTE_TOKEN_IDENTIFIER
        },
        {
            "proc bad() { asm(,); }",
            VITTE_PARSER_ERROR_INVALID_ASM_STATEMENT,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_COMMA
        },
        {
            "proc bad() { if; }",
            VITTE_PARSER_ERROR_INVALID_CONTROL_FLOW_STATEMENT,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { else {} }",
            VITTE_PARSER_ERROR_INVALID_CONTROL_FLOW_STATEMENT,
            VITTE_TOKEN_KW_IF,
            VITTE_TOKEN_KW_ELSE
        },
        {
            "proc bad() { for item values {} }",
            VITTE_PARSER_ERROR_INVALID_CONTROL_FLOW_STATEMENT,
            VITTE_TOKEN_KW_IN,
            VITTE_TOKEN_IDENTIFIER
        },
        {
            "proc bad() { match; }",
            VITTE_PARSER_ERROR_INVALID_MATCH_STATEMENT,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { match true { true false => 0, } }",
            VITTE_PARSER_ERROR_INVALID_MATCH_ARM,
            VITTE_TOKEN_FAT_ARROW,
            VITTE_TOKEN_KW_FALSE
        },
        {
            "proc bad() { match true { (true,, false) => 1, } }",
            VITTE_PARSER_ERROR_INVALID_PATTERN,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_COMMA
        },
        {
            "proc bad() { match true { Point { .., x: } => 1, } }",
            VITTE_PARSER_ERROR_INVALID_PATTERN,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_RIGHT_BRACE
        },
        {
            "proc bad() { match value { item..4 => 0, } }",
            VITTE_PARSER_ERROR_INVALID_RANGE_PATTERN,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_IDENTIFIER
        },
        {
            "proc bad() { let value = target = ; }",
            VITTE_PARSER_ERROR_INVALID_ASSIGNMENT_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = target ?? ; }",
            VITTE_PARSER_ERROR_INVALID_COALESCING_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = target and ; }",
            VITTE_PARSER_ERROR_INVALID_LOGICAL_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = target | ; }",
            VITTE_PARSER_ERROR_INVALID_BITWISE_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = target == ; }",
            VITTE_PARSER_ERROR_INVALID_COMPARISON_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = target << ; }",
            VITTE_PARSER_ERROR_INVALID_SHIFT_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = target + ; }",
            VITTE_PARSER_ERROR_INVALID_ARITHMETIC_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = target .. ; }",
            VITTE_PARSER_ERROR_INVALID_RANGE_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = -; }",
            VITTE_PARSER_ERROR_INVALID_UNARY_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = as int; }",
            VITTE_PARSER_ERROR_INVALID_CAST_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_KW_AS
        },
        {
            "proc bad() { let value = left + right * / divisor; }",
            VITTE_PARSER_ERROR_INVALID_ARITHMETIC_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SLASH
        },
        {
            "proc bad() { let value = invoke(1; }",
            VITTE_PARSER_ERROR_INVALID_POSTFIX_EXPRESSION,
            VITTE_TOKEN_RIGHT_PAREN,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = items[]; }",
            VITTE_PARSER_ERROR_INVALID_POSTFIX_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_RIGHT_BRACKET
        },
        {
            "proc bad() { let value = item.; }",
            VITTE_PARSER_ERROR_INVALID_POSTFIX_EXPRESSION,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = ?item; }",
            VITTE_PARSER_ERROR_INVALID_POSTFIX_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_QUESTION
        },
        {
            "proc bad() { let value = Point { x: }; }",
            VITTE_PARSER_ERROR_INVALID_POSTFIX_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_RIGHT_BRACE
        },
        {
            "proc bad() { let value = if ready { 1 }; }",
            VITTE_PARSER_ERROR_INVALID_IF_EXPRESSION,
            VITTE_TOKEN_KW_ELSE,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = if ready + ; { 1 } else { 0 }; }",
            VITTE_PARSER_ERROR_INVALID_ARITHMETIC_EXPRESSION,
            VITTE_TOKEN_INVALID,
            VITTE_TOKEN_SEMICOLON
        },
        {
            "proc bad() { let value = { 1 2 }; }",
            VITTE_PARSER_ERROR_INVALID_BLOCK_EXPRESSION,
            VITTE_TOKEN_SEMICOLON,
            VITTE_TOKEN_INTEGER_LITERAL
        },
        {
            "type Bad = Map<>;",
            VITTE_PARSER_ERROR_INVALID_GENERIC_ARGUMENTS,
            VITTE_TOKEN_IDENTIFIER,
            VITTE_TOKEN_GREATER
        }
    };
    size_t case_index;
    int ok = 1;

    for (case_index = 0u;
         case_index < sizeof(cases) / sizeof(cases[0]);
         ++case_index) {
        vitte_lexer_t lexer;
        vitte_parser_t parser;
        size_t diagnostic_index;
        int matched = 0;

        if (!parse_source(cases[case_index].source, &lexer, &parser)) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] diagnostic case %zu setup failed\n",
                case_index + 1u);
            ok = 0;
            continue;
        }
        if (vitte_parser_run(&parser)) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] diagnostic case %zu was accepted\n",
                case_index + 1u);
            ok = 0;
        }
        for (diagnostic_index = 0u;
             diagnostic_index < vitte_parser_diagnostic_count(&parser);
             ++diagnostic_index) {
            const vitte_parser_diagnostic_t *diagnostic =
                vitte_parser_diagnostic_at(&parser, diagnostic_index);

            if (diagnostic != NULL &&
                diagnostic->error == cases[case_index].error &&
                diagnostic->expected == cases[case_index].expected &&
                diagnostic->found == cases[case_index].found) {
                matched = 1;
                break;
            }
        }
        if (!matched) {
            const vitte_parser_diagnostic_t *diagnostic =
                vitte_parser_diagnostic_count(&parser) != 0u
                    ? vitte_parser_diagnostic_at(&parser, 0u)
                    : NULL;
            (void)fprintf(
                stderr,
                "[GRAMMAR] diagnostic case %zu reported the wrong error/token "
                "(actual=%s expected=%s found=%s)\n",
                case_index + 1u,
                diagnostic != NULL
                    ? vitte_parser_error_name(diagnostic->error)
                    : "none",
                diagnostic != NULL
                    ? vitte_token_kind_name(diagnostic->expected)
                    : "none",
                diagnostic != NULL
                    ? vitte_token_kind_name(diagnostic->found)
                    : "none");
            ok = 0;
        }
        destroy_frontend(&lexer, &parser);
    }
    return ok;
}

static int
check_recovery(void)
{
    static const char list_source[] =
        "form Packet { first: i64, broken: , last: i64 }\n"
        "proc calls(a: i64, bad: , z: i64) {\n"
        "  consume(\"ok\", , \"still\");\n"
        "  [1, , 3];\n"
        "  asm(\"nop\", , \"clobber\");\n"
        "}\n"
        "proc tail() {}\n";
    static const char nested_source[] =
        "trait Service {\n"
        "  proc prototype();\n"
        "  ,\n"
        "  proc broken() -> { let x: i64 = 1; }\n"
        "  proc inside() {}\n"
        "}\n"
        "proc later_broken() { return 1 + ; }\n"
        "proc outside() {}\n";
    const char *sources[] = { list_source, nested_source };
    size_t case_index;
    int ok = 1;

    for (case_index = 0u; case_index < 2u; ++case_index) {
        vitte_lexer_t lexer;
        vitte_parser_t parser;
        const vitte_ast_node_t *root;
        const vitte_ast_node_t *owner;
        const vitte_ast_node_t *tail;
        size_t minimum_diagnostics = case_index == 0u ? 5u : 3u;
        size_t expected_declarations = 3u;

        if (!parse_source(sources[case_index], &lexer, &parser)) {
            (void)fprintf(stderr, "[GRAMMAR] recovery setup failed\n");
            ok = 0;
            continue;
        }
        if (vitte_parser_run(&parser)) {
            (void)fprintf(stderr, "[GRAMMAR] malformed recovery source accepted\n");
            ok = 0;
        }
        root = vitte_parser_get_node(&parser, vitte_parser_root(&parser));
        owner = find_named_child(
            &parser, root,
            case_index == 0u ? VITTE_AST_NODE_FORM_DECL
                             : VITTE_AST_NODE_TRAIT_DECL,
            case_index == 0u ? "Packet" : "Service");
        tail = find_named_child(
            &parser, root, VITTE_AST_NODE_PROC_DECL,
            case_index == 0u ? "tail" : "outside");
        if (root == NULL || root->child_count != expected_declarations ||
            owner == NULL || tail == NULL ||
            vitte_parser_diagnostic_count(&parser) < minimum_diagnostics ||
            !vitte_parser_validate(&parser)) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] recovery stopped before later declarations "
                "(case=%zu, root children=%zu, diagnostics=%zu)\n",
                case_index + 1u,
                root != NULL ? root->child_count : 0u,
                vitte_parser_diagnostic_count(&parser));
            ok = 0;
        } else if (case_index == 0u) {
            const vitte_ast_node_t *calls = find_named_child(
                &parser, root, VITTE_AST_NODE_PROC_DECL, "calls");
            const vitte_ast_node_t *call = find_kind(
                &parser, calls != NULL ? calls->id : VITTE_AST_INVALID_ID,
                VITTE_AST_NODE_CALL_EXPR);
            const vitte_ast_node_t *array = find_kind(
                &parser, calls != NULL ? calls->id : VITTE_AST_INVALID_ID,
                VITTE_AST_NODE_ARRAY_EXPR);
            const vitte_ast_node_t *asm_statement = find_kind(
                &parser, calls != NULL ? calls->id : VITTE_AST_INVALID_ID,
                VITTE_AST_NODE_ASM_STMT);

            if (owner->child_count != 3u || calls == NULL ||
                call == NULL || call->child_count != 3u ||
                array == NULL || array->child_count != 2u ||
                asm_statement == NULL || asm_statement->child_count != 2u) {
                (void)fprintf(stderr, "[GRAMMAR] list recovery lost valid items\n");
                ok = 0;
            }
        } else {
            size_t diagnostic_index;
            int first_error = 0;
            int later_error = 0;

            for (diagnostic_index = 0u;
                 diagnostic_index < vitte_parser_diagnostic_count(&parser);
                 ++diagnostic_index) {
                const vitte_parser_diagnostic_t *diagnostic =
                    vitte_parser_diagnostic_at(&parser, diagnostic_index);

                if (diagnostic != NULL && diagnostic->span.line == 4u) {
                    first_error = 1;
                }
                if (diagnostic != NULL && diagnostic->span.line == 7u) {
                    later_error = 1;
                }
            }
            if (find_named_child(
                    &parser, owner, VITTE_AST_NODE_PROC_DECL,
                    "inside") == NULL ||
                !first_error || !later_error) {
                (void)fprintf(
                    stderr,
                    "[GRAMMAR] nested recovery missed a later error\n");
                ok = 0;
            }
        }
        destroy_frontend(&lexer, &parser);
    }

    return ok;
}

static int
check_nested_list_recovery(void)
{
    static const char generic_source[] =
        "form Box<T, , U> where T: Trait, U: { value: T }\n"
        "pick Choice<T> { None, Some(T, , i64), After }\n"
        "proc generic(value: Box<i64, , u64>) {}\n"
        "proc tail() {}\n";
    static const char match_source[] =
        "proc choose() {\n"
        "  match value { 0 => 0, 1 => , 2 => 2, }\n"
        "  return 3;\n"
        "}\n"
        "proc tail() {}\n";
    const char *sources[] = { generic_source, match_source };
    size_t case_index;
    int ok = 1;

    for (case_index = 0u; case_index < 2u; ++case_index) {
        vitte_lexer_t lexer;
        vitte_parser_t parser;
        const vitte_ast_node_t *root;
        const vitte_ast_node_t *tail;
        int case_ok;

        if (!parse_source(sources[case_index], &lexer, &parser)) {
            (void)fprintf(stderr, "[GRAMMAR] nested-list setup failed\n");
            ok = 0;
            continue;
        }
        case_ok = !vitte_parser_run(&parser);
        root = vitte_parser_get_node(&parser, vitte_parser_root(&parser));
        tail = find_named_child(
            &parser, root, VITTE_AST_NODE_PROC_DECL, "tail");
        if (root == NULL || tail == NULL ||
            root->child_count != (case_index == 0u ? 4u : 2u) ||
            vitte_parser_diagnostic_count(&parser) <
                (case_index == 0u ? 4u : 1u) ||
            !vitte_parser_validate(&parser)) {
            case_ok = 0;
        }
        if (case_index == 0u) {
            const vitte_ast_node_t *form = find_named_child(
                &parser, root, VITTE_AST_NODE_FORM_DECL, "Box");
            const vitte_ast_node_t *pick = find_named_child(
                &parser, root, VITTE_AST_NODE_PICK_DECL, "Choice");
            const vitte_ast_node_t *procedure = find_named_child(
                &parser, root, VITTE_AST_NODE_PROC_DECL, "generic");
            const vitte_ast_node_t *type = find_kind(
                &parser,
                procedure != NULL ? procedure->id : VITTE_AST_INVALID_ID,
                VITTE_AST_NODE_TYPE_EXPR);

            if (form == NULL || pick == NULL || procedure == NULL ||
                type == NULL || type->child_count != 3u ||
                find_named_child(
                    &parser, pick, VITTE_AST_NODE_VARIANT_DECL,
                    "After") == NULL) {
                case_ok = 0;
            }
        } else {
            const vitte_ast_node_t *procedure = find_named_child(
                &parser, root, VITTE_AST_NODE_PROC_DECL, "choose");
            const vitte_ast_node_t *match = find_kind(
                &parser,
                procedure != NULL ? procedure->id : VITTE_AST_INVALID_ID,
                VITTE_AST_NODE_MATCH_STMT);

            if (match == NULL || match->child_count != 3u ||
                find_kind(
                    &parser,
                    procedure != NULL ? procedure->id : VITTE_AST_INVALID_ID,
                    VITTE_AST_NODE_RETURN_STMT) == NULL) {
                case_ok = 0;
            }
        }
        if (!case_ok) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] nested-list recovery lost later syntax "
                "(case=%zu, root children=%zu, diagnostics=%zu)\n",
                case_index + 1u,
                root != NULL ? root->child_count : 0u,
                vitte_parser_diagnostic_count(&parser));
            ok = 0;
        }
        destroy_frontend(&lexer, &parser);
    }

    return ok;
}

static int
check_delimiter_recovery(void)
{
    static const char source[] =
        "proc broken() {\n"
        "  consume((1 + , 2), 3);\n"
        "  return 4;\n"
        "}\n"
        "proc tail() {}\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *broken;
    const vitte_ast_node_t *call;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] delimiter recovery setup failed\n");
        return 0;
    }
    ok = !vitte_parser_run(&parser);
    root = vitte_parser_get_node(&parser, vitte_parser_root(&parser));
    broken = find_named_child(
        &parser, root, VITTE_AST_NODE_PROC_DECL, "broken");
    call = find_kind(
        &parser,
        broken != NULL ? broken->id : VITTE_AST_INVALID_ID,
        VITTE_AST_NODE_CALL_EXPR);
    if (root == NULL || root->child_count != 2u ||
        find_named_child(
            &parser, root, VITTE_AST_NODE_PROC_DECL, "tail") == NULL ||
        call == NULL || call->child_count != 2u ||
        find_kind(
            &parser,
            broken != NULL ? broken->id : VITTE_AST_INVALID_ID,
            VITTE_AST_NODE_RETURN_STMT) == NULL ||
        vitte_parser_diagnostic_count(&parser) == 0u ||
        !vitte_parser_validate(&parser)) {
        (void)fprintf(stderr, "[GRAMMAR] nested delimiter consumed later code\n");
        ok = 0;
    }
    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
check_import_list_recovery(void)
{
    static const char source[] =
        "use core::{first, , third};\n"
        "export {alpha, , beta};\n"
        "proc tail() {}\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *use_node;
    const vitte_ast_node_t *export_node;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] import recovery setup failed\n");
        return 0;
    }
    ok = !vitte_parser_run(&parser);
    root = vitte_parser_get_node(&parser, vitte_parser_root(&parser));
    use_node = root != NULL && root->child_count > 0u
        ? vitte_parser_get_node(&parser, root->children[0]) : NULL;
    export_node = root != NULL && root->child_count > 1u
        ? vitte_parser_get_node(&parser, root->children[1]) : NULL;
    if (root == NULL || root->child_count != 3u ||
        use_node == NULL || use_node->kind != VITTE_AST_NODE_USE_DECL ||
        use_node->child_count != 3u ||
        export_node == NULL || export_node->kind != VITTE_AST_NODE_USE_DECL ||
        export_node->child_count != 2u ||
        find_named_child(
            &parser, root, VITTE_AST_NODE_PROC_DECL, "tail") == NULL ||
        vitte_parser_diagnostic_count(&parser) < 2u ||
        !vitte_parser_validate(&parser)) {
        (void)fprintf(stderr, "[GRAMMAR] import list recovery lost items\n");
        ok = 0;
    }
    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
expect_parser_diagnostic_at(
    const char *source,
    vitte_parser_error_t expected_error,
    const char *marked_text,
    size_t occurrence)
{
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    const char *marked;
    size_t index;
    size_t begin;
    int found = 0;
    int ok;

    if (source == NULL || marked_text == NULL || marked_text[0] == '\0' ||
        !parse_source(source, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] diagnostic case setup failed\n");
        return 0;
    }
    marked = source;
    for (index = 0u; index <= occurrence; ++index) {
        marked = strstr(marked, marked_text);
        if (marked == NULL) {
            break;
        }
        if (index != occurrence) {
            marked += strlen(marked_text);
        }
    }
    begin = marked != NULL ? (size_t)(marked - source) : SIZE_MAX;
    ok = !vitte_parser_run(&parser);
    for (index = 0u; index < vitte_parser_diagnostic_count(&parser); ++index) {
        const vitte_parser_diagnostic_t *diagnostic =
            vitte_parser_diagnostic_at(&parser, index);
        if (diagnostic != NULL &&
            diagnostic->kind == VITTE_PARSER_DIAGNOSTIC_ERROR &&
            diagnostic->error == expected_error) {
            found = 1;
            if (marked == NULL || !diagnostic->span.valid ||
                diagnostic->span.begin != begin ||
                diagnostic->span.end != begin + strlen(marked_text)) {
                (void)fprintf(
                    stderr,
                    "[GRAMMAR] %s reported wrong span for `%s` "
                    "(expected %zu..%zu, actual %zu..%zu)\n",
                    vitte_parser_error_name(expected_error), source,
                    begin, begin + strlen(marked_text),
                    diagnostic->span.begin, diagnostic->span.end);
                ok = 0;
            }
            break;
        }
    }
    if (!found) {
        (void)fprintf(
            stderr,
            "[GRAMMAR] expected diagnostic %s was not emitted\n",
            vitte_parser_error_name(expected_error));
        ok = 0;
    }
    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
expect_parser_diagnostic_at_eof(
    const char *source,
    vitte_parser_error_t expected_error)
{
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    size_t source_length;
    size_t index;
    int found = 0;
    int ok;

    if (source == NULL || !parse_source(source, &lexer, &parser)) {
        return 0;
    }
    source_length = strlen(source);
    ok = !vitte_parser_run(&parser);
    for (index = 0u; index < vitte_parser_diagnostic_count(&parser); ++index) {
        const vitte_parser_diagnostic_t *diagnostic =
            vitte_parser_diagnostic_at(&parser, index);
        if (diagnostic != NULL && diagnostic->error == expected_error) {
            found = diagnostic->span.valid &&
                diagnostic->span.begin == source_length &&
                diagnostic->span.end == source_length;
            break;
        }
    }
    if (!found) {
        (void)fprintf(stderr,
                      "[GRAMMAR] %s did not underline EOF for `%s`\n",
                      vitte_parser_error_name(expected_error), source);
    }
    destroy_frontend(&lexer, &parser);
    return ok && found;
}

static int
check_module_import_export_diagnostics(void)
{
    static const char missing_module_path[] = "space ;\n";
    static const char missing_module_header[] = "use core;\n";
    static const char missing_export_header[] = "export item;\n";
    static const char malformed_module_path[] = "space core::;\n";
    static const char duplicate_module_header[] =
        "space first;\nspace second;\n";
    static const char misplaced_module_header[] =
        "proc before() {}\nspace late;\n";
    static const char malformed_use[] = "use ;\n";
    static const char invalid_use_alias[] = "use core as ;\n";
    static const char malformed_import_group[] =
        "use core::{first second};\n";
    static const char invalid_nested_glob[] =
        "use core::{outer::{*}};\n";
    static const char invalid_top_level_glob[] =
        "use core::* as alias;\n";
    static const char malformed_export[] = "export ;\n";
    static const char empty_export_group[] = "export {};\n";
    static const char invalid_export_alias[] = "export item as ;\n";
    static const char trailing_use_tokens[] = "use core extra;\n";
    static const char trailing_export_tokens[] = "export item extra;\n";
    static const char valid_nested_groups[] =
        "space imports;\n"
        "use core::{fmt::{Display, Debug}, ops::{Add}};\n"
        "use core::*;\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *nested_use;
    const vitte_ast_node_t *glob_use;
    int ok = 1;

    if (!expect_parser_diagnostic_at_eof(
            missing_module_header,
            VITTE_PARSER_ERROR_MISSING_MODULE_HEADER) ||
        !expect_parser_diagnostic_at_eof(
            missing_export_header,
            VITTE_PARSER_ERROR_MISSING_MODULE_HEADER) ||
        !expect_parser_diagnostic_at(
            missing_module_path,
            VITTE_PARSER_ERROR_INVALID_MODULE_PATH,
            ";", 0u) ||
        !expect_parser_diagnostic_at(
            malformed_module_path,
            VITTE_PARSER_ERROR_INVALID_MODULE_PATH,
            "::", 0u) ||
        !expect_parser_diagnostic_at(
            duplicate_module_header,
            VITTE_PARSER_ERROR_DUPLICATE_MODULE_HEADER,
            "space", 1u) ||
        !expect_parser_diagnostic_at(
            misplaced_module_header,
            VITTE_PARSER_ERROR_MISPLACED_MODULE_HEADER,
            "space", 0u) ||
        !expect_parser_diagnostic_at(
            malformed_use,
            VITTE_PARSER_ERROR_INVALID_USE_DECLARATION,
            ";", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_use_alias,
            VITTE_PARSER_ERROR_INVALID_USE_ALIAS,
            ";", 0u) ||
        !expect_parser_diagnostic_at(
            malformed_import_group,
            VITTE_PARSER_ERROR_INVALID_IMPORT_GROUP,
            "second", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_nested_glob,
            VITTE_PARSER_ERROR_INVALID_GLOB_IMPORT,
            "*", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_top_level_glob,
            VITTE_PARSER_ERROR_INVALID_GLOB_IMPORT,
            "as", 0u) ||
        !expect_parser_diagnostic_at(
            malformed_export,
            VITTE_PARSER_ERROR_INVALID_EXPORT_CLAUSE,
            ";", 0u) ||
        !expect_parser_diagnostic_at(
            empty_export_group,
            VITTE_PARSER_ERROR_EMPTY_EXPORT_GROUP,
            "}", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_export_alias,
            VITTE_PARSER_ERROR_INVALID_EXPORT_ALIAS,
            ";", 0u) ||
        !expect_parser_diagnostic_at(
            trailing_use_tokens,
            VITTE_PARSER_ERROR_TRAILING_TOKENS,
            "extra", 0u) ||
        !expect_parser_diagnostic_at(
            trailing_export_tokens,
            VITTE_PARSER_ERROR_TRAILING_TOKENS,
            "extra", 0u)) {
        return 0;
    }

    if (!parse_source(valid_nested_groups, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] nested import setup failed\n");
        return 0;
    }
    ok = vitte_parser_run(&parser);
    root = vitte_parser_get_node(&parser, vitte_parser_root(&parser));
    nested_use = root != NULL && root->child_count > 1u
        ? vitte_parser_get_node(&parser, root->children[1]) : NULL;
    glob_use = root != NULL && root->child_count > 2u
        ? vitte_parser_get_node(&parser, root->children[2]) : NULL;
    if (!ok || root == NULL || root->child_count != 3u ||
        nested_use == NULL || nested_use->child_count != 4u ||
        glob_use == NULL ||
        (glob_use->flags & VITTE_AST_FLAG_GLOB_IMPORT) == 0u ||
        !vitte_parser_validate(&parser)) {
        (void)fprintf(stderr, "[GRAMMAR] nested imports did not form valid AST paths\n");
        ok = 0;
    }
    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
check_value_type_opaque_form_diagnostics(void)
{
    static const char missing_value_name[] = "const = 1;";
    static const char missing_value_equal[] = "const LIMIT 10;";
    static const char missing_value_expression[] = "const LIMIT = ;";
    static const char missing_value_semicolon[] = "static mut COUNT = 1";
    static const char missing_type_alias[] = "type Alias = ;";
    static const char invalid_type_bound[] = "type Item: Trait + = int;";
    static const char missing_opaque_alias[] = "opaque type Handle = ;";
    static const char invalid_opaque_bound[] = "opaque type Handle: Trait;";
    static const char missing_form_name[] = "form { value: int }";
    static const char malformed_form_field[] = "form Point { value int }";
    static const char duplicate_form_field[] =
        "form Point { value: int, value: bool }";
    static const char missing_form_separator[] =
        "form Point { x: int y: int }";
    static const char missing_form_closer[] = "form Point { x: int";
    static const char valid_declarations[] =
        "const LIMIT: int = 10; static mut COUNT: int = 0; "
        "type Alias = int; type Bound: Trait + Debug; "
        "opaque type Handle; opaque type Raw = int; "
        "form Point { x: int, y: int; }";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    int ok;

    if (!expect_parser_diagnostic_at(
            missing_value_name,
            VITTE_PARSER_ERROR_INVALID_VALUE_DECLARATION,
            "=", 0u) ||
        !expect_parser_diagnostic_at(
            missing_value_equal,
            VITTE_PARSER_ERROR_MISSING_VALUE_INITIALIZER,
            "10", 0u) ||
        !expect_parser_diagnostic_at(
            missing_value_expression,
            VITTE_PARSER_ERROR_MISSING_VALUE_INITIALIZER,
            ";", 0u) ||
        !expect_parser_diagnostic_at_eof(
            missing_value_semicolon,
            VITTE_PARSER_ERROR_INVALID_VALUE_DECLARATION) ||
        !expect_parser_diagnostic_at(
            missing_type_alias,
            VITTE_PARSER_ERROR_INVALID_TYPE_DECLARATION,
            ";", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_type_bound,
            VITTE_PARSER_ERROR_INVALID_TYPE_DECLARATION,
            "=", 0u) ||
        !expect_parser_diagnostic_at(
            missing_opaque_alias,
            VITTE_PARSER_ERROR_INVALID_OPAQUE_DECLARATION,
            ";", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_opaque_bound,
            VITTE_PARSER_ERROR_INVALID_OPAQUE_DECLARATION,
            ":", 0u) ||
        !expect_parser_diagnostic_at(
            missing_form_name,
            VITTE_PARSER_ERROR_INVALID_FORM_DECLARATION,
            "{", 0u) ||
        !expect_parser_diagnostic_at(
            malformed_form_field,
            VITTE_PARSER_ERROR_INVALID_FORM_DECLARATION,
            "int", 1u) ||
        !expect_parser_diagnostic_at(
            duplicate_form_field,
            VITTE_PARSER_ERROR_DUPLICATE_FORM_FIELD,
            "value", 1u) ||
        !expect_parser_diagnostic_at(
            missing_form_separator,
            VITTE_PARSER_ERROR_INVALID_FORM_FIELD_SEPARATOR,
            "y", 0u) ||
        !expect_parser_diagnostic_at_eof(
            missing_form_closer,
            VITTE_PARSER_ERROR_INVALID_FORM_DECLARATION)) {
        return 0;
    }

    if (!parse_source(valid_declarations, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] declaration setup failed\n");
        return 0;
    }
    ok = vitte_parser_run(&parser) && vitte_parser_validate(&parser);
    if (!ok) {
        (void)fprintf(stderr, "[GRAMMAR] valid declarations were rejected\n");
    }
    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
check_pick_trait_impl_procedure_diagnostics(void)
{
    static const char malformed_pick[] = "pick { A }";
    static const char duplicate_pick_variant[] = "pick Choice { Some, Some }";
    static const char invalid_pick_payload[] = "pick Choice { Some(,) }";
    static const char malformed_trait[] = "trait Display";
    static const char malformed_impl[] = "impl Point";
    static const char missing_impl_target[] = "impl Display for {}";
    static const char invalid_impl_for_clause[] =
        "impl Display for Point extra {}";
    static const char malformed_procedure[] = "proc () {}";
    static const char malformed_procedure_parameters[] = "proc f(x int) {}";
    static const char missing_procedure_parameter_closer[] = "proc f(x: int {}";
    static const char invalid_procedure_return[] = "proc f() -> {}";
    static const char duplicate_procedure_modifier[] =
        "async async proc f() {}";
    static const char misplaced_procedure_modifier[] = "async form Point {}";
    static const char invalid_procedure_body[] = "proc f() return 1;";
    static const char invalid_external_procedure_body[] =
        "extern proc f() {}";
    static const char missing_external_procedure_semicolon[] =
        "extern proc f()";
    static const char valid_declarations[] =
        "pick Option<T> { None, Some(T), Empty() } "
        "trait Marker { proc marker(); } "
        "impl Marker for Point { proc marker() {} } "
        "async proc run(value: int) -> int { return value; } "
        "extern proc c_call(value: int);";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    int valid;

    if (!expect_parser_diagnostic_at(
            malformed_pick, VITTE_PARSER_ERROR_INVALID_PICK_DECLARATION,
            "{", 0u) ||
        !expect_parser_diagnostic_at(
            duplicate_pick_variant, VITTE_PARSER_ERROR_DUPLICATE_PICK_VARIANT,
            "Some", 1u) ||
        !expect_parser_diagnostic_at(
            invalid_pick_payload,
            VITTE_PARSER_ERROR_INVALID_PICK_VARIANT_PAYLOAD, ",", 0u) ||
        !expect_parser_diagnostic_at_eof(
            malformed_trait, VITTE_PARSER_ERROR_INVALID_TRAIT_DECLARATION) ||
        !expect_parser_diagnostic_at_eof(
            malformed_impl, VITTE_PARSER_ERROR_INVALID_IMPL_DECLARATION) ||
        !expect_parser_diagnostic_at(
            missing_impl_target, VITTE_PARSER_ERROR_MISSING_IMPL_TARGET_TYPE,
            "{", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_impl_for_clause,
            VITTE_PARSER_ERROR_INVALID_IMPL_FOR_CLAUSE, "extra", 0u) ||
        !expect_parser_diagnostic_at(
            malformed_procedure,
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_DECLARATION, "(", 0u) ||
        !expect_parser_diagnostic_at(
            malformed_procedure_parameters,
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_PARAMETERS, "int", 0u) ||
        !expect_parser_diagnostic_at(
            missing_procedure_parameter_closer,
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_PARAMETERS, "{", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_procedure_return,
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_RETURN_TYPE, "{", 0u) ||
        !expect_parser_diagnostic_at(
            duplicate_procedure_modifier,
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_MODIFIER, "async", 1u) ||
        !expect_parser_diagnostic_at(
            misplaced_procedure_modifier,
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_MODIFIER, "async", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_procedure_body,
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_BODY_FORM, "return", 0u) ||
        !expect_parser_diagnostic_at(
            invalid_external_procedure_body,
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_BODY_FORM, "{", 0u) ||
        !expect_parser_diagnostic_at_eof(
            missing_external_procedure_semicolon,
            VITTE_PARSER_ERROR_INVALID_PROCEDURE_BODY_FORM)) {
        return 0;
    }
    if (!parse_source(valid_declarations, &lexer, &parser)) {
        return 0;
    }
    valid = vitte_parser_run(&parser) && vitte_parser_validate(&parser);
    if (!valid) {
        (void)fprintf(stderr,
                      "[GRAMMAR] valid pick/trait/impl/procedure syntax failed\n");
    }
    destroy_frontend(&lexer, &parser);
    return valid;
}

static int
check_attributes_and_patterns(void)
{
    static const char source[] =
        "#![module(test)]\n"
        "space tests::attributes;\n"
        "#[repr(C)] form Pair<T> where T: Trait { left: T, right: T }\n"
        "impl<T> Pair<T> where T: Trait {\n"
        "  proc get(value: T) -> T { return value; }\n"
        "}\n"
        "#[inline] proc unpack(value: i64) {\n"
        "  let [first, second] = [value, value];\n"
        "  for item in [first, second] { return; }\n"
        "  match value { case 0 => { return; }, case _ => { return; } }\n"
        "}\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *pair;
    const vitte_ast_node_t *implementation;
    const vitte_ast_node_t *unpack;
    const vitte_ast_node_t *pattern;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] attribute setup failed\n");
        return 0;
    }
    ok = vitte_parser_run(&parser);
    root = vitte_parser_get_node(&parser, vitte_parser_root(&parser));
    pair = find_named_child(&parser, root, VITTE_AST_NODE_FORM_DECL, "Pair");
    implementation = find_kind(
        &parser,
        vitte_parser_root(&parser),
        VITTE_AST_NODE_IMPL_DECL);
    unpack = find_named_child(&parser, root, VITTE_AST_NODE_PROC_DECL, "unpack");
    pattern = find_kind(&parser, unpack != NULL ? unpack->id : VITTE_AST_INVALID_ID,
                        VITTE_AST_NODE_PATTERN);
    if (!ok || root == NULL || root->child_count != 4u || pair == NULL ||
        implementation == NULL ||
        unpack == NULL || pattern == NULL ||
        (pair->flags & VITTE_AST_FLAG_HAS_ATTRIBUTES) == 0u ||
        (unpack->flags & VITTE_AST_FLAG_HAS_ATTRIBUTES) == 0u ||
        vitte_parser_diagnostic_count(&parser) != 0u ||
        !vitte_parser_validate(&parser)) {
        (void)fprintf(stderr, "[GRAMMAR] attributes or patterns were not preserved\n");
        ok = 0;
    }
    destroy_frontend(&lexer, &parser);
    return ok;
}

static int
check_advanced_grammar_surface(void)
{
    static const char source[] =
        "space tests::advanced_grammar;\n"
        "#[repr(C)]\n"
        "trait Printable<T: Display + Debug> where T: Display + Debug {\n"
        "  type Item: Display;\n"
        "  const FLAG: bool = true;\n"
        "  proc format<U>(value: U) -> string where U: Display;\n"
        "}\n"
        "impl<T> Printable<T> for Box<T> where T: Display + Debug {\n"
        "  type Item = T;\n"
        "  proc format<U>(value: U) -> string where U: Display { return value; }\n"
        "}\n"
        "#[inline] proc inspect<T>(value: T) where T < 4 {\n"
        "  let Point { x, y: mut z, .. } = value;\n"
        "  let 1..=3 = value;\n"
        "  match value {\n"
        "    case Some(a, b) | None if ready => b,\n"
        "    case Point { x, y } => x,\n"
        "    case _ => value,\n"
        "  }\n"
        "}\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    const vitte_ast_node_t *root;
    const vitte_ast_node_t *trait_node;
    const vitte_ast_node_t *impl_node;
    const vitte_ast_node_t *procedure;
    const vitte_ast_node_t *generic;
    const vitte_ast_node_t *where_node;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] advanced surface setup failed\n");
        return 0;
    }
    ok = vitte_parser_run(&parser);
    root = vitte_parser_get_node(&parser, vitte_parser_root(&parser));
    trait_node = find_kind(&parser, vitte_parser_root(&parser),
                           VITTE_AST_NODE_TRAIT_DECL);
    impl_node = find_kind(&parser, vitte_parser_root(&parser),
                          VITTE_AST_NODE_IMPL_DECL);
    procedure = find_named_child(&parser, root, VITTE_AST_NODE_PROC_DECL,
                                 "inspect");
    generic = find_kind(&parser, procedure != NULL ? procedure->id
                                                    : VITTE_AST_INVALID_ID,
                        VITTE_AST_NODE_GENERIC_PARAM);
    where_node = find_kind(&parser, procedure != NULL ? procedure->id
                                                       : VITTE_AST_INVALID_ID,
                           VITTE_AST_NODE_WHERE_CLAUSE);
    if (!ok || root == NULL || root->child_count != 4u ||
        trait_node == NULL || trait_node->child_count < 4u ||
        impl_node == NULL || impl_node->child_count < 3u ||
        procedure == NULL || generic == NULL || where_node == NULL ||
        find_pattern_flag(&parser, procedure->id,
                          VITTE_AST_FLAG_PATTERN_HEAD) == NULL ||
        find_pattern_flag(&parser, procedure->id,
                          VITTE_AST_FLAG_PATTERN_RANGE) == NULL ||
        find_pattern_flag(&parser, procedure->id,
                          VITTE_AST_FLAG_PATTERN_OR) == NULL ||
        (procedure->flags & VITTE_AST_FLAG_HAS_ATTRIBUTES) == 0u ||
        vitte_parser_diagnostic_count(&parser) != 0u ||
        !vitte_parser_validate(&parser)) {
        (void)fprintf(stderr,
                      "[GRAMMAR] generic/trait/impl/where/pattern surface mismatch\n");
        ok = 0;
    }
    destroy_frontend(&lexer, &parser);
    return ok;
}

static char *
read_file(const char *path)
{
    FILE *file;
    long file_size;
    char *contents;
    size_t size;

    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        (void)fclose(file);
        return NULL;
    }

    file_size = ftell(file);
    if (file_size < 0L || fseek(file, 0L, SEEK_SET) != 0) {
        (void)fclose(file);
        return NULL;
    }

    size = (size_t)file_size;
    if ((long)size != file_size || size == SIZE_MAX) {
        (void)fclose(file);
        return NULL;
    }

    contents = (char *)malloc(size + 1u);
    if (contents == NULL) {
        (void)fclose(file);
        return NULL;
    }

    if (fread(contents, 1u, size, file) != size) {
        free(contents);
        (void)fclose(file);
        return NULL;
    }

    contents[size] = '\0';
    if (fclose(file) != 0) {
        free(contents);
        return NULL;
    }

    return contents;
}

static int
is_expected_parser_negative(const char *path)
{
    static const char suffix[] =
        "export_forms/reexport_group_unsupported.vit";
    size_t path_length;
    size_t suffix_length;

    path_length = strlen(path);
    suffix_length = sizeof(suffix) - 1u;

    return path_length >= suffix_length &&
           strcmp(path + path_length - suffix_length, suffix) == 0;
}

static int
check_fixture(const char *path)
{
    char *source;
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    int expected_negative;
    int parsed;

    source = read_file(path);
    if (source == NULL) {
        (void)fprintf(stderr, "[GRAMMAR] cannot read fixture: %s\n", path);
        return 0;
    }

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(stderr, "[GRAMMAR] lexer/parser initialization failed: %s\n",
                      path);
        free(source);
        return 0;
    }

    expected_negative = is_expected_parser_negative(path);
    parsed = vitte_parser_run(&parser);

    if (parsed == expected_negative) {
        size_t diagnostic_index;

        (void)fprintf(
            stderr,
            "[GRAMMAR] %s: expected %s, parser %s\n",
            path,
            expected_negative ? "rejection" : "acceptance",
            parsed ? "accepted" : "rejected");
        for (diagnostic_index = 0u;
             diagnostic_index < vitte_parser_diagnostic_count(&parser);
             ++diagnostic_index) {
            const vitte_parser_diagnostic_t *diagnostic;

            diagnostic = vitte_parser_diagnostic_at(
                &parser,
                diagnostic_index);
            if (diagnostic != NULL &&
                diagnostic->kind == VITTE_PARSER_DIAGNOSTIC_ERROR) {
                (void)fprintf(
                    stderr,
                    "  %s at %zu:%zu (expected %s, found %s)\n",
                    vitte_parser_error_name(diagnostic->error),
                    diagnostic->span.line,
                    diagnostic->span.column,
                    vitte_token_kind_name(diagnostic->expected),
                    vitte_token_kind_name(diagnostic->found));
                break;
            }
        }
        parsed = 0;
    } else {
        if (!expected_negative && !vitte_parser_validate(&parser)) {
            (void)fprintf(
                stderr,
                "[GRAMMAR] %s: parser accepted an invalid AST\n",
                path);
            parsed = 0;
        } else {
            (void)printf(
                "[GRAMMAR] %s: %s\n",
                path,
                expected_negative ? "expected rejection" : "parsed");
            parsed = 1;
        }
    }

    destroy_frontend(&lexer, &parser);
    free(source);
    return parsed;
}

int
main(int argc, char **argv)
{
    int ok;
    int i;

    ok = check_precedence();
    if (!check_division_is_not_a_path_separator()) {
        ok = 0;
    }
    if (!check_syntax_surface()) {
        ok = 0;
    }
    if (!check_slice_and_cast_syntax()) {
        ok = 0;
    }
    if (!check_invalid_syntax()) {
        ok = 0;
    }
    if (!check_parser_limits_and_reset()) {
        ok = 0;
    }
    if (!check_parser_diagnostic_details()) {
        ok = 0;
    }
    if (!check_recovery()) {
        ok = 0;
    }
    if (!check_nested_list_recovery()) {
        ok = 0;
    }
    if (!check_delimiter_recovery()) {
        ok = 0;
    }
    if (!check_import_list_recovery()) {
        ok = 0;
    }
    if (!check_module_import_export_diagnostics()) {
        ok = 0;
    }
    if (!check_value_type_opaque_form_diagnostics()) {
        ok = 0;
    }
    if (!check_pick_trait_impl_procedure_diagnostics()) {
        ok = 0;
    }
    if (!check_attributes_and_patterns()) {
        ok = 0;
    }
    if (!check_advanced_grammar_surface()) {
        ok = 0;
    }

    for (i = 1; i < argc; ++i) {
        if (!check_fixture(argv[i])) {
            ok = 0;
        }
    }

    if (!ok) {
        return EXIT_FAILURE;
    }

    if (!check_keyword_named_procedure()) {
        return EXIT_FAILURE;
    }

    (void)printf(
        "[GRAMMAR] precedence, invalid syntax and %d fixtures passed\n",
        argc - 1);
    return EXIT_SUCCESS;
}
