#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vitte/codegen.h"
#include "vittec/back/emit_c.h"

static const char* k_demo_src =
    "mod demo.core\n"
    "use runtime/printf as log\n"
    "type Pair\n"
    "  field left: i32\n"
    "  field right: i32\n"
    ".end\n"
    "fn main(val: i32) -> i32\n"
    "  set tmp = val\n"
    "  ret tmp\n"
    ".end\n"
    "prog demo.core.main\n"
    "  ret 0\n"
    ".end\n";

static int g_fail = 0;

static void t_fail(const char* file, int line, const char* msg) {
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, msg);
    g_fail = 1;
}
#define T_ASSERT(x) do { if (!(x)) t_fail(__FILE__, __LINE__, #x); } while (0)

static void test_emit_from_phrase_unit(void) {
    const char* src = k_demo_src;

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_codegen_unit unit;
    vitte_codegen_unit_init(&unit);
    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);

    T_ASSERT(vitte_codegen_unit_build(&ctx, 0u, src, strlen(src), &unit, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));

    vittec_emit_c_options_t opt;
    vittec_emit_c_options_init(&opt);
    opt.emit_header = 0;
    opt.emit_debug_comments = 1;

    vittec_emit_c_buffer_t buf;
    vittec_emit_c_buffer_init(&buf);

    int rc = vittec_emit_c_buffer(&unit, &buf, &opt);
    T_ASSERT(rc == VITTEC_EMIT_C_OK);
    T_ASSERT(buf.c_data && buf.c_size > 0);
    T_ASSERT(buf.h_data == NULL);
    T_ASSERT(strstr(buf.c_data, "module demo.core") != NULL);
    T_ASSERT(strstr(buf.c_data, "typedef struct vitte_demo_core_Pair") != NULL);
    T_ASSERT(strstr(buf.c_data, "demo.core::main params=1") != NULL);
    T_ASSERT(strstr(buf.c_data, "entrypoints: 1") != NULL);
    T_ASSERT(strstr(buf.c_data, "return vitte_demo_core_main") != NULL);

    vittec_emit_c_buffer_reset(&buf);
    vitte_codegen_unit_reset(&ctx, &unit);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

static void test_emit_buffer_with_header(void) {
    const char* src = k_demo_src;

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_codegen_unit unit;
    vitte_codegen_unit_init(&unit);
    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);

    T_ASSERT(vitte_codegen_unit_build(&ctx, 0u, src, strlen(src), &unit, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));

    vittec_emit_c_options_t opt;
    vittec_emit_c_options_init(&opt);
    opt.emit_header = 1;

    vittec_emit_c_buffer_t buf;
    vittec_emit_c_buffer_init(&buf);

    int rc = vittec_emit_c_buffer(&unit, &buf, &opt);
    T_ASSERT(rc == VITTEC_EMIT_C_OK);
    T_ASSERT(buf.c_data && buf.h_data);
    T_ASSERT(strstr(buf.c_data, "#include \"generated.h\"") != NULL);
    T_ASSERT(strstr(buf.h_data, "#ifndef VITTE_GENERATED_H") != NULL);

    vittec_emit_c_buffer_reset(&buf);
    vitte_codegen_unit_reset(&ctx, &unit);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

static void test_emit_rejects_null_unit(void) {
    int rc = vittec_emit_c_file(NULL, "out.c");
    T_ASSERT(rc == VITTEC_EMIT_C_EINVAL);
}

int main(void) {
    test_emit_from_phrase_unit();
    test_emit_buffer_with_header();
    test_emit_rejects_null_unit();
    if (g_fail) return 1;
    printf("OK: vittec emit_c backend tests\n");
    return 0;
}
