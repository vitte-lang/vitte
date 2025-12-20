#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vitte/codegen.h"
#include "vitte/diag.h"

static int g_fail = 0;

static void failf(const char* file, int line, const char* msg) {
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, msg);
    g_fail = 1;
}
#define T_ASSERT(x) do { if (!(x)) { failf(__FILE__, __LINE__, #x); return; } } while (0)

static int read_entire_file(const char* path, char** out_data, size_t* out_len) {
    *out_data = NULL;
    *out_len = 0;
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return 0; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    char* buf = (char*)malloc((size_t)n + 1u);
    if (!buf) { fclose(f); return 0; }
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    if (got != (size_t)n) { free(buf); return 0; }
    buf[got] = '\0';
    *out_data = buf;
    *out_len = got;
    return 1;
}

static char* slurp_stream(FILE* f) {
    if (!f) return NULL;
    if (fflush(f) != 0) return NULL;
    long pos = ftell(f);
    if (pos < 0) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) return NULL;
    long n = ftell(f);
    if (n < 0) return NULL;
    if (fseek(f, 0, SEEK_SET) != 0) return NULL;
    char* buf = (char*)malloc((size_t)n + 1u);
    if (!buf) return NULL;
    size_t got = fread(buf, 1, (size_t)n, f);
    buf[got] = '\0';
    (void)fseek(f, pos, SEEK_SET);
    return buf;
}

static void run_case_rel(const char* name, const char* input_rel, const char* human_golden_rel, const char* json_golden_rel) {
    (void)name;
    char input_path[1024];
    char human_golden_path[1024];
    char json_golden_path[1024];

    snprintf(input_path, sizeof(input_path), "%s/%s", VITTE_TEST_ROOT, input_rel);
    snprintf(human_golden_path, sizeof(human_golden_path), "%s/%s", VITTE_TEST_ROOT, human_golden_rel);
    snprintf(json_golden_path, sizeof(json_golden_path), "%s/%s", VITTE_TEST_ROOT, json_golden_rel);

    char* src = NULL;
    size_t src_len = 0;
    if (!read_entire_file(input_path, &src, &src_len)) {
        fprintf(stderr, "cannot read input: %s\n", input_path);
        g_fail = 1;
        return;
    }

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_codegen_unit unit;
    vitte_codegen_unit_init(&unit);

    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);

    vitte_result r = vitte_codegen_unit_build(&ctx, 1u, src, src_len, &unit, &diags);
    T_ASSERT(r != VITTE_OK);
    T_ASSERT(vitte_diag_bag_has_errors(&diags));

    vitte_source vs;
    vs.file_id = 1u;
    vs.path = input_rel;
    vs.data = src;
    vs.len = (uint32_t)src_len;

    vitte_emit_options opt;
    vitte_emit_options_init(&opt);
    opt.context_lines = 0;
    opt.sort_by_location = 1;
    opt.json_one_per_line = 0;
    opt.json_pretty = 0;

    FILE* hf = tmpfile();
    if (!hf) { g_fail = 1; goto cleanup; }
    vitte_emit_human(hf, &vs, &diags, &opt);
    char* human_actual = slurp_stream(hf);
    fclose(hf);
    if (!human_actual) { g_fail = 1; goto cleanup; }

    char* human_expected = NULL;
    size_t human_expected_len = 0;
    if (!read_entire_file(human_golden_path, &human_expected, &human_expected_len)) {
        fprintf(stderr, "cannot read golden (human): %s\n", human_golden_path);
        g_fail = 1;
        free(human_actual);
        goto cleanup;
    }

    if (strcmp(human_actual, human_expected) != 0) {
        fprintf(stderr, "golden mismatch (human): %s\n-- expected:\n%s\n-- actual:\n%s\n", input_path, human_expected, human_actual);
        g_fail = 1;
    }

    FILE* jf = tmpfile();
    if (!jf) {
        g_fail = 1;
        free(human_expected);
        free(human_actual);
        goto cleanup;
    }
    vitte_emit_json(jf, &vs, &diags, &opt);
    char* json_actual = slurp_stream(jf);
    fclose(jf);
    if (!json_actual) {
        g_fail = 1;
        free(human_expected);
        free(human_actual);
        goto cleanup;
    }

    char* json_expected = NULL;
    size_t json_expected_len = 0;
    if (!read_entire_file(json_golden_path, &json_expected, &json_expected_len)) {
        fprintf(stderr, "cannot read golden (json): %s\n", json_golden_path);
        g_fail = 1;
        free(json_actual);
        free(human_expected);
        free(human_actual);
        goto cleanup;
    }
    (void)json_expected_len;

    if (strcmp(json_actual, json_expected) != 0) {
        fprintf(stderr, "golden mismatch (json): %s\n-- expected:\n%s\n-- actual:\n%s\n", input_path, json_expected, json_actual);
        g_fail = 1;
    }

    free(json_expected);
    free(json_actual);
    free(human_expected);
    free(human_actual);

cleanup:
    vitte_diag_bag_free(&diags);
    vitte_codegen_unit_reset(&ctx, &unit);
    vitte_ctx_free(&ctx);
    free(src);
}

int main(void) {
    run_case_rel(
        "unterminated_string",
        "tests/golden/diags/unterminated_string.vitte",
        "tests/golden/diags/unterminated_string.human",
        "tests/golden/diags/unterminated_string.json"
    );
    run_case_rel(
        "missing_end",
        "tests/golden/diags/missing_end.vitte",
        "tests/golden/diags/missing_end.human",
        "tests/golden/diags/missing_end.json"
    );
    run_case_rel(
        "unmatched_end",
        "tests/golden/diags/unmatched_end.vitte",
        "tests/golden/diags/unmatched_end.human",
        "tests/golden/diags/unmatched_end.json"
    );

    if (g_fail) return 1;
    printf("OK: vitte diagnostics golden tests\n");
    return 0;
}
