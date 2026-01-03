#include "vitte_ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ast_node_t* ast_node_new(ast_node_type_t type, int32_t line, int32_t column) {
    ast_node_t *node = (ast_node_t *)malloc(sizeof(ast_node_t));
    if (!node) return NULL;
    
    node->type = type;
    node->line = line;
    node->column = column;
    node->data = NULL;
    node->children = NULL;
    node->child_count = 0;
    
    return node;
}

ast_node_t* ast_create_module(void) {
    return ast_node_new(AST_MODULE, 0, 0);
}

ast_node_t* ast_create_function(const char *name, ast_node_t *body) {
    ast_node_t *node = ast_node_new(AST_FUNCTION, 0, 0);
    if (!node) return NULL;
    
    function_decl_t *fn = (function_decl_t *)malloc(sizeof(function_decl_t));
    if (!fn) {
        free(node);
        return NULL;
    }
    
    fn->name = (char *)malloc(strlen(name) + 1);
    strcpy(fn->name, name);
    fn->params = NULL;
    fn->param_count = 0;
    fn->return_type = NULL;
    fn->body = body;
    fn->is_pub = false;
    
    node->data = fn;
    if (body) {
        ast_add_child(node, body);
    }
    
    return node;
}

ast_node_t* ast_create_block(void) {
    return ast_node_new(AST_BLOCK, 0, 0);
}

ast_node_t* ast_create_if(ast_node_t *condition, ast_node_t *then_branch, ast_node_t *else_branch) {
    ast_node_t *node = ast_node_new(AST_IF, 0, 0);
    if (!node) return NULL;
    
    ast_add_child(node, condition);
    ast_add_child(node, then_branch);
    if (else_branch) {
        ast_add_child(node, else_branch);
    }
    
    return node;
}

ast_node_t* ast_create_while(ast_node_t *condition, ast_node_t *body) {
    ast_node_t *node = ast_node_new(AST_WHILE, 0, 0);
    if (!node) return NULL;
    
    ast_add_child(node, condition);
    ast_add_child(node, body);
    
    return node;
}

ast_node_t* ast_create_literal(literal_type_t type) {
    ast_node_t *node = ast_node_new(AST_LITERAL, 0, 0);
    if (!node) return NULL;
    
    literal_t *lit = (literal_t *)malloc(sizeof(literal_t));
    if (!lit) {
        free(node);
        return NULL;
    }
    
    lit->type = type;
    memset(&lit->value, 0, sizeof(literal_value_t));
    
    node->data = lit;
    return node;
}

ast_node_t* ast_create_identifier(const char *name) {
    ast_node_t *node = ast_node_new(AST_IDENTIFIER, 0, 0);
    if (!node) return NULL;
    
    char *dup = (char *)malloc(strlen(name) + 1);
    if (!dup) {
        free(node);
        return NULL;
    }
    strcpy(dup, name);
    node->data = dup;
    
    return node;
}

ast_node_t* ast_create_binary_op(ast_node_t *left, const char *op, ast_node_t *right) {
    ast_node_t *node = ast_node_new(AST_BINARY_OP, 0, 0);
    if (!node) return NULL;
    
    binary_op_t *binop = (binary_op_t *)malloc(sizeof(binary_op_t));
    if (!binop) {
        free(node);
        return NULL;
    }
    
    binop->left = left;
    binop->right = right;
    binop->op = (char *)malloc(strlen(op) + 1);
    strcpy(binop->op, op);
    
    node->data = binop;
    ast_add_child(node, left);
    ast_add_child(node, right);
    
    return node;
}

ast_node_t* ast_create_call(ast_node_t *func, ast_node_t **args, uint32_t arg_count) {
    ast_node_t *node = ast_node_new(AST_CALL, 0, 0);
    if (!node) return NULL;
    
    ast_add_child(node, func);
    for (uint32_t i = 0; i < arg_count; i++) {
        ast_add_child(node, args[i]);
    }
    
    return node;
}

void ast_add_child(ast_node_t *parent, ast_node_t *child) {
    if (!parent || !child) return;
    
    parent->children = (ast_node_t **)realloc(parent->children, 
                                              sizeof(ast_node_t *) * (parent->child_count + 1));
    parent->children[parent->child_count] = child;
    parent->child_count++;
}

void ast_free(ast_node_t *node) {
    if (!node) return;
    
    if (node->data) {
        if (node->type == AST_FUNCTION) {
            function_decl_t *fn = (function_decl_t *)node->data;
            if (fn->name) free(fn->name);
            if (fn->params) free(fn->params);
            free(fn);
        } else if (node->type == AST_IDENTIFIER) {
            free((char *)node->data);
        } else if (node->type == AST_BINARY_OP) {
            binary_op_t *binop = (binary_op_t *)node->data;
            if (binop->op) free(binop->op);
            free(binop);
        } else if (node->type == AST_LITERAL) {
            literal_t *lit = (literal_t *)node->data;
            if (lit->type == LITERAL_STRING && lit->value.string_value) {
                free(lit->value.string_value);
            }
            free(lit);
        } else {
            free(node->data);
        }
    }
    
    if (node->children) {
        for (uint32_t i = 0; i < node->child_count; i++) {
            ast_free(node->children[i]);
        }
        free(node->children);
    }
    
    free(node);
}

const char* ast_type_name(ast_node_type_t type) {
    switch (type) {
        case AST_MODULE: return "MODULE";
        case AST_FUNCTION: return "FUNCTION";
        case AST_STRUCT: return "STRUCT";
        case AST_BLOCK: return "BLOCK";
        case AST_IF: return "IF";
        case AST_WHILE: return "WHILE";
        case AST_BINARY_OP: return "BINARY_OP";
        case AST_LITERAL: return "LITERAL";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_CALL: return "CALL";
        default: return "UNKNOWN";
    }
}

void ast_print(ast_node_t *node, uint32_t indent) {
    if (!node) return;
    
    for (uint32_t i = 0; i < indent; i++) printf("  ");
    printf("[%s]", ast_type_name(node->type));
    
    if (node->type == AST_IDENTIFIER) {
        printf(" %s", (char *)node->data);
    } else if (node->type == AST_FUNCTION) {
        function_decl_t *fn = (function_decl_t *)node->data;
        printf(" %s", fn->name);
    }
    
    printf("\n");
    
    for (uint32_t i = 0; i < node->child_count; i++) {
        ast_print(node->children[i], indent + 1);
    }
}
