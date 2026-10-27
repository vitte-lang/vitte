#include "backend/c17/ir_emitter.h"
#include "backend/c17/backend.h"
#include "hir/hir.h"
#include "hir/lower.h"
#include "ir/ir.h"
#include "ir/lower.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct source_map_capture {
    size_t count;
    uint32_t file_id;
    size_t begin;
    size_t end;
    size_t generated_line;
    size_t generated_column;
    size_t generated_offset;
} source_map_capture_t;

static void
capture_source_map(
    void *user_data,
    uint32_t source_file_id,
    size_t source_begin,
    size_t source_end,
    size_t generated_line,
    size_t generated_column,
    size_t generated_offset)
{
    source_map_capture_t *capture =
        (source_map_capture_t *)user_data;
    assert(capture != NULL);
    capture->count++;
    capture->file_id = source_file_id;
    capture->begin = source_begin;
    capture->end = source_end;
    capture->generated_line = generated_line;
    capture->generated_column = generated_column;
    capture->generated_offset = generated_offset;
}

static void
test_source_map_positions(void)
{
    vitte_c17_backend_t backend;
    vitte_c17_backend_config_t config =
        vitte_c17_backend_config_default();
    vitte_c17_target_t target = vitte_c17_target_default();
    vitte_c17_memory_writer_t writer;
    source_map_capture_t capture = {0};

    vitte_c17_memory_writer_init(&writer);
    assert(vitte_c17_backend_init(
        &backend,
        &config,
        &target,
        vitte_c17_writer_from_memory(&writer)));
    vitte_c17_backend_set_source_map_handler(
        &backend,
        capture_source_map,
        &capture);
    assert(vitte_c17_backend_begin(&backend));
    vitte_c17_backend_set_source_location(&backend, 9u, 12u, 18u);
    assert(vitte_c17_backend_write(&backend, "int x;\n"));
    vitte_c17_backend_mark_source(&backend);
    assert(vitte_c17_backend_finish(&backend));
    assert(capture.count == 1u);
    assert(capture.file_id == 9u);
    assert(capture.begin == 12u && capture.end == 18u);
    assert(capture.generated_line == 7u);
    assert(capture.generated_column == 0u);
    assert(capture.generated_offset == 120u);
    vitte_c17_backend_destroy(&backend);
    vitte_c17_memory_writer_destroy(&writer);
}

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *data;
    if (file == NULL || fseek(file, 0L, SEEK_END) != 0) return NULL;
    length = ftell(file);
    if (length < 0L || fseek(file, 0L, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    data = (char *)malloc((size_t)length + 1u);
    if (data != NULL && fread(data, 1u, (size_t)length, file) == (size_t)length) {
        data[(size_t)length] = '\0';
    } else {
        free(data);
        data = NULL;
    }
    fclose(file);
    return data;
}

int main(void)
{
    const char source[] =
        "space backend::smoke;\n"
        "const baseline = 40;\n"
        "export space lib { export proc helper() -> i64 { return 1; } }\n"
        "proc message() -> string { return \"hello\"; }\n"
        "proc main() -> i64 { let answer: i64 = baseline + lib::helper(); if true { message(); } return answer + 2; }\n";
    const char c_path[] = "/tmp/vitte-bootstrap-ir-backend.c";
    const char object_path[] = "/tmp/vitte-bootstrap-ir-backend.o";
    const char executable_path[] = "/tmp/vitte-bootstrap-ir-backend";
    vitte_lexer_t lexer;
    vitte_parser_t parser;
    vitte_hir_t hir;
    vitte_ir_context_t ir;
    vitte_c17_ir_emit_options_t options;
    FILE *output;
    char *generated;

    test_source_map_positions();

    assert(vitte_lexer_init(&lexer, source, sizeof(source) - 1u, 1u));
    assert(vitte_lexer_run(&lexer));
    assert(vitte_parser_init(&parser, lexer.tokens, lexer.token_count));
    assert(vitte_parser_run(&parser));
    assert(vitte_parser_validate(&parser));
    assert(vitte_hir_init(&hir));
    assert(vitte_hir_lower_ast(&hir, &parser));
    assert(hir.state == VITTE_HIR_STATE_VALIDATED);
    assert(vitte_hir_validate(&hir));
    assert(vitte_ir_init(&ir));
    assert(vitte_ir_lower_hir(&ir, &hir));
    assert(ir.state == VITTE_IR_STATE_SEALED);
    assert(vitte_ir_validate(&ir));

    options = vitte_c17_ir_emit_options_default();
    options.emit_debug_comments = true;
    options.emit_line_directives = true;
    options.emit_debug_info = true;
    options.source_path = "backend/smoke.vit";
    output = fopen(c_path, "wb");
    assert(output != NULL);
    assert(vitte_c17_emit_ir(output, &ir, &options));
    assert(fclose(output) == 0);
    generated = read_file(c_path);
    assert(generated != NULL);
    assert(strstr(generated, "vitte_fn_main_") != NULL);
    assert(strstr(generated, "hello") != NULL);
    assert(strstr(generated, "vitte:1:") != NULL);
    assert(strstr(generated, "#line 1 \"backend/smoke.vit\"") != NULL);
    assert(strstr(generated, "vitte-symbol: main -> vitte_fn_main_") != NULL);
    free(generated);
    assert(vitte_c17_compile_ir_object(&ir, object_path, &options));
    assert(vitte_c17_compile_ir(&ir, executable_path, false, false, 0, NULL, &options));

    remove(c_path);
    remove(object_path);
    remove(executable_path);
    vitte_ir_destroy(&ir);
    vitte_hir_destroy(&hir);
    vitte_parser_destroy(&parser);
    vitte_lexer_destroy(&lexer);
    return 0;
}
