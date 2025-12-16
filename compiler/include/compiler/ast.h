#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <stdint.h>

typedef enum {
    AST_NODE_MODULE,
    AST_NODE_DECL,
    AST_NODE_FUNC,
    AST_NODE_VAR,
    AST_NODE_CONST,
    AST_NODE_STRUCT,
    AST_NODE_ENUM,
    AST_NODE_ALIAS,
} ast_node_kind_t;

typedef struct ast_node {
    ast_node_kind_t kind;
    uint32_t line;
    uint32_t col;
    void *data;
} ast_node_t;

typedef struct ast_module {
    ast_node_t **decls;
    uint32_t decl_count;
    uint32_t decl_capacity;
} ast_module_t;

ast_module_t* ast_module_create(void);
void ast_module_destroy(ast_module_t *mod);
void ast_module_add_decl(ast_module_t *mod, ast_node_t *decl);

#endif /* COMPILER_AST_H */
