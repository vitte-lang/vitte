#ifndef VITTE_AST_H
#define VITTE_AST_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    AST_MODULE,
    AST_FUNCTION,
    AST_STRUCT,
    AST_ENUM,
    AST_VAR_DECL,
    AST_CONST_DECL,
    AST_EXPR_STMT,
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_CALL,
    AST_INDEX,
    AST_MEMBER,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_TYPE,
} ast_node_type_t;

typedef enum {
    LITERAL_INT,
    LITERAL_FLOAT,
    LITERAL_STRING,
    LITERAL_BOOL,
    LITERAL_NULL,
} literal_type_t;

typedef struct ast_node {
    ast_node_type_t type;
    int32_t line;
    int32_t column;
    void *data;
    struct ast_node **children;
    uint32_t child_count;
} ast_node_t;

typedef struct {
    char *name;
    ast_node_t **params;
    uint32_t param_count;
    ast_node_t *return_type;
    ast_node_t *body;
    bool is_pub;
} function_decl_t;

typedef struct {
    char *name;
    ast_node_t **fields;
    uint32_t field_count;
} struct_decl_t;

typedef struct {
    int64_t int_value;
    double float_value;
    char *string_value;
    bool bool_value;
} literal_value_t;

typedef struct {
    literal_type_t type;
    literal_value_t value;
} literal_t;

typedef struct {
    char *name;
    ast_node_t *type;
} variable_t;

typedef struct {
    ast_node_t *left;
    ast_node_t *right;
    char *op;
} binary_op_t;

ast_node_t* ast_create_module(void);
ast_node_t* ast_create_function(const char *name, ast_node_t *body);
ast_node_t* ast_create_block(void);
ast_node_t* ast_create_if(ast_node_t *condition, ast_node_t *then_branch, ast_node_t *else_branch);
ast_node_t* ast_create_while(ast_node_t *condition, ast_node_t *body);
ast_node_t* ast_create_literal(literal_type_t type);
ast_node_t* ast_create_identifier(const char *name);
ast_node_t* ast_create_binary_op(ast_node_t *left, const char *op, ast_node_t *right);
ast_node_t* ast_create_call(ast_node_t *func, ast_node_t **args, uint32_t arg_count);

void ast_add_child(ast_node_t *parent, ast_node_t *child);
void ast_free(ast_node_t *node);
void ast_print(ast_node_t *node, uint32_t indent);

#endif
