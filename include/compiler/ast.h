#ifndef STEEL_AST_H
#define STEEL_AST_H

typedef enum {
    AST_NODE_IDENT,
    AST_NODE_NUMBER,
    AST_NODE_STRING,
    AST_NODE_BINARY,
} ASTNodeType;

typedef struct ASTNode ASTNode;

struct ASTNode {
    ASTNodeType type;
    union {
        char *ident;
        char *string_value;
        long number_value;
        struct {
            char op;
            ASTNode *left;
            ASTNode *right;
        } binary;
    } u;
};

ASTNode *ast_new_ident(const char *name);
ASTNode *ast_new_number(long v);
ASTNode *ast_new_string(const char *s);
ASTNode *ast_new_binary(char op, ASTNode *l, ASTNode *r);
void ast_free(ASTNode *n);

#endif
