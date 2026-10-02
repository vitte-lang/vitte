#ifndef VITTE_SRC_BACKEND_C17_AST_EMITTER_H
#define VITTE_SRC_BACKEND_C17_AST_EMITTER_H

#include "../../parser/parser.h"

#include <stdbool.h>
#include <stdio.h>

/*
 * Options used by the bootstrap AST emitter.  The regular emitter remains
 * source-compatible with its historical API; callers that need native
 * debugging metadata use this small explicit contract.
 */
typedef struct vitte_c17_compile_options {
    bool debug_info;
    bool debug_runtime;
    bool emit_line_directives;
    const char *source_path;
} vitte_c17_compile_options_t;

bool
vitte_c17_emit_ast_with_options(
    FILE *output,
    const vitte_parser_t *parser,
    const vitte_c17_compile_options_t *options);

bool
vitte_c17_emit_ast(
    FILE *output,
    const vitte_parser_t *parser);

bool
vitte_c17_compile_ast(
    const vitte_parser_t *parser,
    const char *output_path,
    bool emit_c,
    bool run,
    int run_argc,
    char **run_argv);

bool
vitte_c17_compile_ast_with_options(
    const vitte_parser_t *parser,
    const char *output_path,
    bool emit_c,
    bool run,
    int run_argc,
    char **run_argv,
    const vitte_c17_compile_options_t *options);

#endif
