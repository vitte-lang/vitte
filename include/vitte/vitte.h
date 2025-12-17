#ifndef VITTE_VITTE_H
#define VITTE_VITTE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VITTE_OK = 0,
    VITTE_ERR_LEX = 1,
    VITTE_ERR_PARSE = 2,
    VITTE_ERR_DESUGAR = 3,
    VITTE_ERR_INTERNAL = 99
} vitte_result;

typedef enum {
    VITTE_ERRC_NONE = 0,
    VITTE_ERRC_UNEXPECTED_EOF,
    VITTE_ERRC_UNMATCHED_END,
    VITTE_ERRC_MISSING_END,
    VITTE_ERRC_SYNTAX
} vitte_error_code;

typedef struct {
    vitte_error_code code;
    uint32_t line;
    uint32_t col;
    char message[128];
} vitte_error;

typedef struct {
    int reserved;
} vitte_ctx;

typedef struct {
    uint32_t line;
    uint32_t col;
} vitte_location;

typedef struct {
    vitte_location start;
    vitte_location end;
} vitte_span;

typedef enum {
    VITTE_AST_PHR_UNIT = 1,
    VITTE_AST_CORE_UNIT,
    VITTE_AST_MODULE_DECL,
    VITTE_AST_USE_DECL,
    VITTE_AST_TYPE_DECL,
    VITTE_AST_FIELD_DECL,
    VITTE_AST_FN_DECL,
    VITTE_AST_SCENARIO_DECL,
    VITTE_AST_PROGRAM_DECL,
    VITTE_AST_PARAM,
    VITTE_AST_TYPE_REF,
    VITTE_AST_PATH,
    VITTE_AST_ALIAS,
    VITTE_AST_BLOCK,
    VITTE_AST_BRANCH,
    VITTE_AST_PHR_STMT_SET,
    VITTE_AST_PHR_STMT_SAY,
    VITTE_AST_PHR_STMT_DO,
    VITTE_AST_PHR_STMT_RET,
    VITTE_AST_PHR_STMT_WHEN,
    VITTE_AST_PHR_STMT_LOOP,
    VITTE_AST_CORE_STMT_ASSIGN,
    VITTE_AST_CORE_STMT_EXPR,
    VITTE_AST_CORE_STMT_RETURN,
    VITTE_AST_CORE_STMT_IF,
    VITTE_AST_CORE_STMT_FOR,
    VITTE_AST_EXPR_IDENT,
    VITTE_AST_EXPR_LITERAL,
    VITTE_AST_EXPR_BINARY,
    VITTE_AST_EXPR_UNARY,
    VITTE_AST_EXPR_CALL,
    VITTE_AST_EXPR_ARG,
    VITTE_AST_EXPR_PATH
} vitte_ast_kind;

typedef enum {
    VITTE_LITERAL_NONE = 0,
    VITTE_LITERAL_INT,
    VITTE_LITERAL_FLOAT,
    VITTE_LITERAL_STRING,
    VITTE_LITERAL_BOOL
} vitte_literal_kind;

typedef enum {
    VITTE_BINOP_NONE = 0,
    VITTE_BINOP_ADD,
    VITTE_BINOP_SUB,
    VITTE_BINOP_MUL,
    VITTE_BINOP_DIV,
    VITTE_BINOP_MOD,
    VITTE_BINOP_EQ,
    VITTE_BINOP_NEQ,
    VITTE_BINOP_LT,
    VITTE_BINOP_LTE,
    VITTE_BINOP_GT,
    VITTE_BINOP_GTE,
    VITTE_BINOP_AND,
    VITTE_BINOP_OR
} vitte_binary_op;

typedef enum {
    VITTE_UNOP_NONE = 0,
    VITTE_UNOP_NEG,
    VITTE_UNOP_POS,
    VITTE_UNOP_NOT
} vitte_unary_op;

typedef struct {
    vitte_literal_kind kind;
    int bool_value;
} vitte_literal;

typedef struct vitte_ast {
    vitte_ast_kind kind;
    vitte_span span;
    struct vitte_ast* first_child;
    struct vitte_ast* last_child;
    struct vitte_ast* next;
    char* text;
    char* aux_text;
    vitte_literal literal;
    vitte_binary_op binary_op;
    vitte_unary_op unary_op;
} vitte_ast;

void vitte_ctx_init(vitte_ctx* ctx);
void vitte_ctx_free(vitte_ctx* ctx);
void vitte_ast_free(vitte_ctx* ctx, vitte_ast* ast);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_VITTE_H */
