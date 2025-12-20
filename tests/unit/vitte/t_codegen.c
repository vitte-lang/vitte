#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vitte/codegen.h"

static int g_fail = 0;

static void assert_true(int cond, const char* msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        g_fail = 1;
    }
}

static void test_codegen_module_type_fn(void) {
    const char* src =
        "mod foo.bar\n"
        "type Vec2\n"
        "  field x: f32\n"
        "  field y: f32\n"
        ".end\n"
        "fn add(a: i32, b: i32) -> i32\n"
        "  set tmp = a + b\n"
        "  ret tmp\n"
        ".end\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);

    vitte_codegen_unit unit;
    vitte_codegen_unit_init(&unit);

    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);

    vitte_result r = vitte_codegen_unit_build(&ctx, 0u, src, strlen(src), &unit, &diags);
    assert_true(r == VITTE_OK, "codegen unit build ok");
    assert_true(!vitte_diag_bag_has_errors(&diags), "no diagnostics");
    assert_true(unit.module_count == 1, "found module");
    assert_true(unit.type_count == 1, "found type");
    assert_true(unit.functions && unit.function_count == 1, "found fn");
    assert_true(unit.types[0].field_count == 2, "type fields count");
    assert_true(unit.functions[0].stmt_count == 2, "fn stmt count");
    assert_true(unit.modules[0].name && strcmp(unit.modules[0].name, "foo.bar") == 0, "module path stored");
    assert_true(unit.functions[0].module_ast == unit.modules[0].ast, "function captures module scope");
    assert_true(unit.types[0].module_ast == unit.modules[0].ast, "type captures module scope");
    assert_true(unit.functions[0].param_count == 2, "param count captured");

    vitte_codegen_unit_reset(&ctx, &unit);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

static void test_codegen_error(void) {
    const char* src =
        "fn bad()\n"
        "  set x = 1\n"
        ".end\n"
        ".end\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);

    vitte_codegen_unit unit;
    vitte_codegen_unit_init(&unit);

    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);

    vitte_result r = vitte_codegen_unit_build(&ctx, 0u, src, strlen(src), &unit, &diags);
    assert_true(r == VITTE_ERR_PARSE, "codegen propagates parse error");
    assert_true(vitte_diag_bag_has_errors(&diags), "diagnostics present");

    vitte_codegen_unit_reset(&ctx, &unit);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

int main(void) {
    test_codegen_module_type_fn();
    test_codegen_error();
    if (g_fail) return 1;
    printf("OK: vitte codegen unit tests\n");
    return 0;
}
