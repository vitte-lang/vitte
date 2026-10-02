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

static int
check_precedence(void)
{
    static const char source[] =
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
        "  give result;\n"
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
        "form Buffer { values: [i64] }\n"
        "proc convert(value: f64) -> i64 {\n"
        "  give value as i64;\n"
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
        "proc null() -> int { give 0; }\n"
        "proc declared() -> int;\n"
        "proc use_null() -> int { give null(); }\n"
        "proc use_null_literal() { set value = null; }\n";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    int ok;

    if (!parse_source(source, &lexer, &parser)) {
        (void)fprintf(
            stderr,
            "[GRAMMAR] failed to initialize keyword-name test\n");
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
                "[GRAMMAR] keyword-name source rejected: %s at %zu:%zu\n",
                vitte_parser_error_name(diagnostic->error),
                diagnostic->span.line,
                diagnostic->span.column);
        }
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
        }

        destroy_frontend(&lexer, &parser);
    }

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
        (void)printf(
            "[GRAMMAR] %s: %s\n",
            path,
            expected_negative ? "expected rejection" : "parsed");
        parsed = 1;
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
    if (!check_syntax_surface()) {
        ok = 0;
    }
    if (!check_slice_and_cast_syntax()) {
        ok = 0;
    }
    if (!check_invalid_syntax()) {
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
