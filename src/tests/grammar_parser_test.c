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
check_attributes_and_patterns(void)
{
    static const char source[] =
        "#![module(test)]\n"
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
    if (!ok || root == NULL || root->child_count != 3u || pair == NULL ||
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
    if (!ok || root == NULL || root->child_count != 3u ||
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
