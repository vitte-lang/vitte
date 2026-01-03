#include "vitte_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

compiler_context_t* compiler_create(const char *input_file, const char *output_file) {
    compiler_context_t *ctx = (compiler_context_t *)malloc(sizeof(compiler_context_t));
    if (!ctx) return NULL;
    
    ctx->source_file = input_file;
    ctx->output_file = output_file;
    ctx->output = fopen(output_file, "w");
    if (!ctx->output) {
        free(ctx);
        return NULL;
    }
    
    ctx->line = 1;
    ctx->column = 1;
    ctx->error_buffer[0] = '\0';
    ctx->error_count = 0;
    ctx->warning_count = 0;
    
    return ctx;
}

void compiler_free(compiler_context_t *ctx) {
    if (!ctx) return;
    if (ctx->output) fclose(ctx->output);
    free(ctx);
}

void compiler_error(compiler_context_t *ctx, int32_t line, int32_t col, const char *msg) {
    ctx->error_count++;
    fprintf(stderr, "[ERROR] %s:%d:%d: %s\n", ctx->source_file, line, col, msg);
}

void compiler_warning(compiler_context_t *ctx, int32_t line, int32_t col, const char *msg) {
    ctx->warning_count++;
    fprintf(stderr, "[WARN] %s:%d:%d: %s\n", ctx->source_file, line, col, msg);
}

parser_t* parser_create(lexer_t *lexer, compiler_context_t *ctx) {
    parser_t *parser = (parser_t *)malloc(sizeof(parser_t));
    if (!parser) return NULL;
    
    parser->lexer = lexer;
    parser->context = ctx;
    parser->error_buffer[0] = '\0';
    
    parser->current_token = lexer_next_token(lexer);
    parser->peek_token = lexer_next_token(lexer);
    
    return parser;
}

void parser_free(parser_t *parser) {
    if (parser) free(parser);
}

static void parser_advance(parser_t *parser) {
    parser->current_token = parser->peek_token;
    parser->peek_token = lexer_next_token(parser->lexer);
}

static int parser_match(parser_t *parser, token_type_t type) {
    if (parser->current_token.type == type) {
        parser_advance(parser);
        return 1;
    }
    return 0;
}

static int parser_check(parser_t *parser, token_type_t type) {
    return parser->current_token.type == type;
}

ast_node_t* parser_parse_module(parser_t *parser) {
    ast_node_t *module = ast_create_module();
    
    while (!parser_check(parser, TOKEN_EOF)) {
        if (parser_check(parser, TOKEN_KW_FN)) {
            ast_node_t *func = parser_parse_function(parser);
            if (func) {
                ast_add_child(module, func);
            }
        } else {
            parser_advance(parser);
        }
    }
    
    return module;
}

ast_node_t* parser_parse_function(parser_t *parser) {
    if (!parser_match(parser, TOKEN_KW_FN)) {
        return NULL;
    }
    
    if (!parser_check(parser, TOKEN_IDENTIFIER)) {
        compiler_error(parser->context, parser->current_token.line, 
                      parser->current_token.column, "Expected function name");
        return NULL;
    }
    
    const char *name = parser->current_token.lexeme;
    parser_advance(parser);
    
    if (!parser_match(parser, TOKEN_LPAREN)) {
        compiler_error(parser->context, parser->current_token.line,
                      parser->current_token.column, "Expected '('");
        return NULL;
    }
    
    while (!parser_check(parser, TOKEN_RPAREN) && !parser_check(parser, TOKEN_EOF)) {
        parser_advance(parser);
    }
    
    if (!parser_match(parser, TOKEN_RPAREN)) {
        compiler_error(parser->context, parser->current_token.line,
                      parser->current_token.column, "Expected ')'");
        return NULL;
    }
    
    if (!parser_match(parser, TOKEN_LBRACE)) {
        compiler_error(parser->context, parser->current_token.line,
                      parser->current_token.column, "Expected '{'");
        return NULL;
    }
    
    ast_node_t *body = parser_parse_block(parser);
    
    if (!parser_match(parser, TOKEN_RBRACE)) {
        compiler_error(parser->context, parser->current_token.line,
                      parser->current_token.column, "Expected '}'");
        ast_free(body);
        return NULL;
    }
    
    return ast_create_function(name, body);
}

ast_node_t* parser_parse_block(parser_t *parser) {
    ast_node_t *block = ast_create_block();
    
    while (!parser_check(parser, TOKEN_RBRACE) && !parser_check(parser, TOKEN_EOF)) {
        ast_node_t *stmt = parser_parse_statement(parser);
        if (stmt) {
            ast_add_child(block, stmt);
        }
    }
    
    return block;
}

ast_node_t* parser_parse_statement(parser_t *parser) {
    if (parser_check(parser, TOKEN_KW_LET)) {
        parser_advance(parser);
        if (parser_check(parser, TOKEN_IDENTIFIER)) {
            parser_advance(parser);
        }
        if (parser_match(parser, TOKEN_ASSIGN)) {
            ast_node_t *expr = parser_parse_expression(parser);
            parser_match(parser, TOKEN_SEMICOLON);
            return expr;
        }
    }
    
    if (parser_check(parser, TOKEN_KW_RETURN)) {
        parser_advance(parser);
        ast_node_t *expr = parser_parse_expression(parser);
        parser_match(parser, TOKEN_SEMICOLON);
        return expr;
    }
    
    ast_node_t *expr = parser_parse_expression(parser);
    parser_match(parser, TOKEN_SEMICOLON);
    return expr;
}

ast_node_t* parser_parse_expression(parser_t *parser) {
    if (parser_check(parser, TOKEN_NUMBER)) {
        ast_node_t *lit = ast_create_literal(LITERAL_INT);
        literal_t *lit_data = (literal_t *)lit->data;
        lit_data->value.int_value = (int64_t)strtoll(parser->current_token.lexeme, NULL, 10);
        parser_advance(parser);
        return lit;
    }
    
    if (parser_check(parser, TOKEN_STRING)) {
        ast_node_t *lit = ast_create_literal(LITERAL_STRING);
        parser_advance(parser);
        return lit;
    }
    
    if (parser_check(parser, TOKEN_IDENTIFIER)) {
        ast_node_t *id = ast_create_identifier(parser->current_token.lexeme);
        parser_advance(parser);
        return id;
    }
    
    parser_advance(parser);
    return NULL;
}

codegen_t* codegen_create(compiler_context_t *ctx, FILE *output) {
    codegen_t *gen = (codegen_t *)malloc(sizeof(codegen_t));
    if (!gen) return NULL;
    
    gen->context = ctx;
    gen->output = output;
    gen->root = NULL;
    gen->instruction_count = 0;
    gen->current_indent[0] = '\0';
    
    return gen;
}

void codegen_free(codegen_t *gen) {
    if (gen) free(gen);
}

void codegen_emit_instruction(codegen_t *gen, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    fprintf(gen->output, "%s", gen->current_indent);
    vfprintf(gen->output, format, args);
    fprintf(gen->output, "\n");
    
    gen->instruction_count++;
    va_end(args);
}

static void codegen_emit_node(codegen_t *gen, ast_node_t *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_FUNCTION: {
            function_decl_t *fn = (function_decl_t *)node->data;
            codegen_emit_instruction(gen, "fn %s()", fn->name);
            codegen_emit_instruction(gen, "{");
            strcat(gen->current_indent, "  ");
            for (uint32_t i = 0; i < node->child_count; i++) {
                codegen_emit_node(gen, node->children[i]);
            }
            gen->current_indent[strlen(gen->current_indent) - 2] = '\0';
            codegen_emit_instruction(gen, "}");
            break;
        }
        case AST_BLOCK:
            for (uint32_t i = 0; i < node->child_count; i++) {
                codegen_emit_node(gen, node->children[i]);
            }
            break;
        case AST_LITERAL:
            codegen_emit_instruction(gen, "push <value>");
            break;
        case AST_IDENTIFIER: {
            char *name = (char *)node->data;
            codegen_emit_instruction(gen, "load %%rax, [%s]", name);
            break;
        }
        case AST_CALL: {
            if (node->child_count > 0) {
                codegen_emit_node(gen, node->children[0]);
            }
            codegen_emit_instruction(gen, "call");
            break;
        }
        default:
            break;
    }
}

int codegen_generate(codegen_t *gen, ast_node_t *ast) {
    if (!ast) return -1;
    
    gen->root = ast;
    
    fprintf(gen->output, ";; Generated Vitte code\n");
    fprintf(gen->output, ";; Instructions: TODO\n\n");
    
    codegen_emit_node(gen, ast);
    
    fprintf(gen->output, "\n;; Total instructions: %d\n", gen->instruction_count);
    
    return 0;
}

int compiler_compile(compiler_context_t *ctx) {
    FILE *input = fopen(ctx->source_file, "r");
    if (!input) {
        compiler_error(ctx, 0, 0, "Cannot open input file");
        return 1;
    }
    
    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    fseek(input, 0, SEEK_SET);
    
    char *source = (char *)malloc(size + 1);
    if (!source) {
        compiler_error(ctx, 0, 0, "Out of memory");
        fclose(input);
        return 1;
    }
    
    fread(source, 1, size, input);
    source[size] = '\0';
    fclose(input);
    
    lexer_t *lexer = lexer_create(source);
    if (!lexer) {
        compiler_error(ctx, 0, 0, "Lexer creation failed");
        free(source);
        return 1;
    }
    
    parser_t *parser = parser_create(lexer, ctx);
    if (!parser) {
        compiler_error(ctx, 0, 0, "Parser creation failed");
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    ast_node_t *ast = parser_parse_module(parser);
    
    codegen_t *codegen = codegen_create(ctx, ctx->output);
    if (!codegen) {
        compiler_error(ctx, 0, 0, "Codegen creation failed");
        parser_free(parser);
        lexer_free(lexer);
        free(source);
        return 1;
    }
    
    codegen_generate(codegen, ast);
    
    fprintf(ctx->output, "\n;; Compilation summary:\n");
    fprintf(ctx->output, ";; Errors: %d\n", ctx->error_count);
    fprintf(ctx->output, ";; Warnings: %d\n", ctx->warning_count);
    fprintf(ctx->output, ";; Instructions: %d\n", codegen->instruction_count);
    
    ast_free(ast);
    codegen_free(codegen);
    parser_free(parser);
    lexer_free(lexer);
    free(source);
    
    return ctx->error_count == 0 ? 0 : 1;
}
