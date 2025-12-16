// Vitte Compiler - Parser Header
// Syntax analysis and AST construction

#ifndef VITTE_PARSER_H
#define VITTE_PARSER_H

#include "lexer.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// AST Node Types
// ============================================================================

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_VARIABLE,
    AST_ASSIGNMENT,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_CALL,
    AST_IF,
    AST_FOR,
    AST_WHILE,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_BLOCK,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_TYPE,
    AST_STRUCT,
    AST_ENUM,
    AST_TRAIT,
    AST_MATCH,
    AST_ARRAY,
    AST_INDEX,
    AST_MEMBER,
} ast_node_type_t;

// ============================================================================
// AST Node Structure
// ============================================================================

typedef struct ast_node {
    ast_node_type_t type;
    int line;
    int column;
    void *data;
    struct ast_node **children;
    size_t children_count;
    char *annotation;  // Type information, etc.
} ast_node_t;

// ============================================================================
// Parser Structure
// ============================================================================

typedef struct {
    token_t *tokens;
    size_t token_count;
    size_t current;
    int error_count;
    char **errors;
    size_t error_capacity;
} parser_t;

// ============================================================================
// Parser API
// ============================================================================

// Create parser from tokens
parser_t* parser_create(token_t *tokens, size_t token_count);

// Parse program
ast_node_t* parser_parse(parser_t *parser);

// Parse expression
ast_node_t* parser_parse_expression(parser_t *parser);

// Parse statement
ast_node_t* parser_parse_statement(parser_t *parser);

// Get error messages
const char** parser_get_errors(parser_t *parser, size_t *count);

// Free parser
void parser_free(parser_t *parser);

// ============================================================================
// AST Node Management
// ============================================================================

// Create AST node
ast_node_t* ast_node_create(ast_node_type_t type, int line, int column);

// Add child node
void ast_node_add_child(ast_node_t *parent, ast_node_t *child);

// Free AST node
void ast_node_free(ast_node_t *node);

// Print AST
void ast_print(ast_node_t *node, int indent);

// Get node type name
const char* ast_node_type_name(ast_node_type_t type);

#ifdef __cplusplus
}
#endif

#endif // VITTE_PARSER_H
