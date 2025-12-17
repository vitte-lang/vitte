#include <stdio.h>
#include <string.h>

#include "vitte/parser_phrase.h"
#include "vitte/lexer.h"

static int g_fail = 0;

static void t_fail(const char* file, int line, const char* msg) {
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, msg);
    g_fail = 1;
}
#define T_ASSERT(x) do { if (!(x)) t_fail(__FILE__, __LINE__, #x); } while (0)

static void test_parse_module_and_use(void) {
    const char* src =
        "mod demo.core\n"
        "use runtime/io as rt\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_ast* ast = NULL;
    vitte_error err;
    memset(&err, 0, sizeof(err));

    T_ASSERT(vitte_parse_phrase(&ctx, src, strlen(src), &ast, &err) == VITTE_OK);
    const vitte_ast* mod = ast->first_child;
    T_ASSERT(mod && mod->kind == VITTE_AST_MODULE_DECL);
    T_ASSERT(mod->text && strcmp(mod->text, "demo.core") == 0);
    const vitte_ast* use = mod ? mod->next : NULL;
    T_ASSERT(use && use->kind == VITTE_AST_USE_DECL);
    T_ASSERT(use->text && strcmp(use->text, "runtime/io") == 0);
    T_ASSERT(use->aux_text && strcmp(use->aux_text, "rt") == 0);

    vitte_ast_free(&ctx, ast);
    vitte_ctx_free(&ctx);
}

static void test_type_block_requires_end(void) {
    const char* src =
        "type Pair\n"
        "  field first: i32\n"
        "  field second: i32\n"
        ".end\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_ast* ast = NULL;
    vitte_error err;
    memset(&err, 0, sizeof(err));

    T_ASSERT(vitte_parse_phrase(&ctx, src, strlen(src), &ast, &err) == VITTE_OK);
    const vitte_ast* type_decl = ast->first_child;
    T_ASSERT(type_decl && type_decl->kind == VITTE_AST_TYPE_DECL);
    T_ASSERT(type_decl->first_child && type_decl->first_child->kind == VITTE_AST_FIELD_DECL);
    T_ASSERT(type_decl->first_child->next && type_decl->first_child->next->kind == VITTE_AST_FIELD_DECL);

    vitte_ast_free(&ctx, ast);

    const char* missing_end =
        "type Broken\n"
        "  field x: i32\n";

    ast = NULL;
    memset(&err, 0, sizeof(err));
    T_ASSERT(vitte_parse_phrase(&ctx, missing_end, strlen(missing_end), &ast, &err) == VITTE_ERR_PARSE);
    T_ASSERT(err.code != VITTE_ERRC_NONE);

    if (ast) vitte_ast_free(&ctx, ast);
    vitte_ctx_free(&ctx);
}

static void test_parser_reports_malformed_loop_and_set(void) {
    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_ast* ast = NULL;
    vitte_error err;
    memset(&err, 0, sizeof(err));

    const char* bad_loop =
        "fn bad()\n"
        "  loop i from 0\n"
        ".end\n";
    T_ASSERT(vitte_parse_phrase(&ctx, bad_loop, strlen(bad_loop), &ast, &err) == VITTE_ERR_PARSE);
    T_ASSERT(err.code != VITTE_ERRC_NONE);
    if (ast) {
        vitte_ast_free(&ctx, ast);
        ast = NULL;
    }

    const char* bad_set =
        "fn broken()\n"
        "  set value\n"
        ".end\n";
    memset(&err, 0, sizeof(err));
    T_ASSERT(vitte_parse_phrase(&ctx, bad_set, strlen(bad_set), &ast, &err) == VITTE_ERR_PARSE);
    T_ASSERT(err.code != VITTE_ERRC_NONE);

    if (ast) vitte_ast_free(&ctx, ast);
    vitte_ctx_free(&ctx);
}

int main(void) {
    test_parse_module_and_use();
    test_type_block_requires_end();
    test_parser_reports_malformed_loop_and_set();
    if (g_fail) return 1;
    printf("OK: vitte parser phrase tests\n");
    return 0;
}
