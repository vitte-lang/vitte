#ifndef VITTE_COMPILER_H
#define VITTE_COMPILER_H

#include "vitte_lexer.h"
#include "vitte_ast.h"
#include <stdio.h>

typedef struct {
    const char *source_file;
    const char *output_file;
    FILE *output;
    int32_t line;
    int32_t column;
    char error_buffer[512];
    int error_count;
    int warning_count;
} compiler_context_t;

typedef struct {
    lexer_t *lexer;
    token_t current_token;
    token_t peek_token;
    compiler_context_t *context;
    char error_buffer[512];
} parser_t;

typedef struct {
    compiler_context_t *context;
    ast_node_t *root;
    FILE *output;
    int instruction_count;
    char current_indent[256];
} codegen_t;

compiler_context_t* compiler_create(const char *input_file, const char *output_file);
void compiler_free(compiler_context_t *ctx);
int compiler_compile(compiler_context_t *ctx);

parser_t* parser_create(lexer_t *lexer, compiler_context_t *ctx);
void parser_free(parser_t *parser);
ast_node_t* parser_parse_module(parser_t *parser);
ast_node_t* parser_parse_function(parser_t *parser);
ast_node_t* parser_parse_block(parser_t *parser);
ast_node_t* parser_parse_statement(parser_t *parser);
ast_node_t* parser_parse_expression(parser_t *parser);

codegen_t* codegen_create(compiler_context_t *ctx, FILE *output);
void codegen_free(codegen_t *gen);
int codegen_generate(codegen_t *gen, ast_node_t *ast);
void codegen_emit_instruction(codegen_t *gen, const char *format, ...);

void compiler_error(compiler_context_t *ctx, int32_t line, int32_t col, const char *msg);
void compiler_warning(compiler_context_t *ctx, int32_t line, int32_t col, const char *msg);

#endif
