#ifndef VITTE_LEXER_H
#define VITTE_LEXER_H

#include "vitte.h"
#include "vitte/diag.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VITTE_TOK_EOF = 0,
    VITTE_TOK_NEWLINE,
    VITTE_TOK_IDENT,
    VITTE_TOK_INT,
    VITTE_TOK_FLOAT,
    VITTE_TOK_STRING,
    VITTE_TOK_TRUE,
    VITTE_TOK_FALSE,
    VITTE_TOK_NIL,
    VITTE_TOK_DOT,
    VITTE_TOK_DOTEND,
    VITTE_TOK_COMMA,
    VITTE_TOK_COLON,
    VITTE_TOK_DOUBLECOLON,
    VITTE_TOK_SEMICOLON,
    VITTE_TOK_LPAREN,
    VITTE_TOK_RPAREN,
    VITTE_TOK_LBRACE,
    VITTE_TOK_RBRACE,
    VITTE_TOK_LBRACKET,
    VITTE_TOK_RBRACKET,
    VITTE_TOK_ARROW,
    VITTE_TOK_EQUAL,
    VITTE_TOK_EQEQ,
    VITTE_TOK_NEQ,
    VITTE_TOK_PLUS,
    VITTE_TOK_MINUS,
    VITTE_TOK_STAR,
    VITTE_TOK_SLASH,
    VITTE_TOK_PERCENT,
    VITTE_TOK_LT,
    VITTE_TOK_LTE,
    VITTE_TOK_GT,
    VITTE_TOK_GTE,
    VITTE_TOK_AND,
    VITTE_TOK_OR,
    VITTE_TOK_NOT,
    VITTE_TOK_KW_MOD,
    VITTE_TOK_KW_USE,
    VITTE_TOK_KW_TYPE,
    VITTE_TOK_KW_FIELD,
    VITTE_TOK_KW_FN,
    VITTE_TOK_KW_SCN,
    VITTE_TOK_KW_PROG,
    VITTE_TOK_KW_SET,
    VITTE_TOK_KW_SAY,
    VITTE_TOK_KW_DO,
    VITTE_TOK_KW_RET,
    VITTE_TOK_KW_WHEN,
    VITTE_TOK_KW_ELSE,
    VITTE_TOK_KW_LOOP,
    VITTE_TOK_KW_FROM,
    VITTE_TOK_KW_TO,
    VITTE_TOK_KW_STEP,
    VITTE_TOK_KW_AS,
    VITTE_TOK_KW_IN,
    VITTE_TOK_KW_PROGRAM,
    VITTE_TOK_KW_SERVICE,
    VITTE_TOK_KW_KERNEL,
    VITTE_TOK_KW_DRIVER,
    VITTE_TOK_KW_TOOL,
    VITTE_TOK_KW_PIPELINE,
    VITTE_TOK_KW_SCENARIO
} vitte_token_kind;

typedef struct {
    vitte_token_kind kind;
    const char* lexeme;
    size_t len;
    vitte_span span;
} vitte_token;

/* Surface API kept minimal for now. */
vitte_result vitte_lex_all(vitte_ctx* ctx,
                           vitte_file_id file_id,
                           const char* src,
                           size_t len,
                           vitte_token** out_tokens,
                           size_t* out_count,
                           vitte_diag_bag* diags);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_LEXER_H */
