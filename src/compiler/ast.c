#include "../../include/compiler/ast.h"
#include <stdlib.h>
#include <string.h>

ASTNode *ast_new_ident(const char *name) {
    ASTNode *n = malloc(sizeof(ASTNode));
    n->type = AST_NODE_IDENT;
    n->u.ident = strdup(name ?: "");
    return n;
}

ASTNode *ast_new_number(long v) {
    ASTNode *n = malloc(sizeof(ASTNode));
    n->type = AST_NODE_NUMBER;
    n->u.number_value = v;
    return n;
}

ASTNode *ast_new_string(const char *s) {
    ASTNode *n = malloc(sizeof(ASTNode));
    n->type = AST_NODE_STRING;
    n->u.string_value = strdup(s ?: "");
    return n;
}

ASTNode *ast_new_binary(char op, ASTNode *l, ASTNode *r) {
    ASTNode *n = malloc(sizeof(ASTNode));
    n->type = AST_NODE_BINARY;
    n->u.binary.op = op;
    n->u.binary.left = l;
    n->u.binary.right = r;
    return n;
}

void ast_free(ASTNode *n) {
    if (!n) return;
    switch (n->type) {
        case AST_NODE_IDENT: free(n->u.ident); break;
        case AST_NODE_STRING: free(n->u.string_value); break;
        case AST_NODE_NUMBER: break;
        case AST_NODE_BINARY:
            ast_free(n->u.binary.left);
            ast_free(n->u.binary.right);
            break;
    }
    free(n);
}
