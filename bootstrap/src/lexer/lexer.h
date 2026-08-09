#ifndef VITTE_BOOTSTRAP_LEXER_H
#define VITTE_BOOTSTRAP_LEXER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../api/error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_LEXER_DEFAULT_MAX_SOURCE_BYTES ((size_t)64u * 1024u * 1024u)
#define VITTE_LEXER_DEFAULT_TAB_WIDTH ((size_t)4u)

typedef enum vitte_token_kind {
    VITTE_TOKEN_ERROR = 0,
    VITTE_TOKEN_EOF,
    VITTE_TOKEN_WHITESPACE,
    VITTE_TOKEN_COMMENT,
    VITTE_TOKEN_IDENTIFIER,
    VITTE_TOKEN_INTEGER,
    VITTE_TOKEN_STRING,
    VITTE_TOKEN_KW_SPACE,
    VITTE_TOKEN_KW_USE,
    VITTE_TOKEN_KW_EXPORT,
    VITTE_TOKEN_KW_AS,
    VITTE_TOKEN_KW_PROC,
    VITTE_TOKEN_KW_CONST,
    VITTE_TOKEN_KW_LET,
    VITTE_TOKEN_KW_SET,
    VITTE_TOKEN_KW_MUT,
    VITTE_TOKEN_KW_IF,
    VITTE_TOKEN_KW_ELSE,
    VITTE_TOKEN_KW_GIVE,
    VITTE_TOKEN_KW_PICK,
    VITTE_TOKEN_KW_AND,
    VITTE_TOKEN_KW_OR,
    VITTE_TOKEN_KW_NOT,
    VITTE_TOKEN_LPAREN,
    VITTE_TOKEN_RPAREN,
    VITTE_TOKEN_LBRACKET,
    VITTE_TOKEN_RBRACKET,
    VITTE_TOKEN_LBRACE,
    VITTE_TOKEN_RBRACE,
    VITTE_TOKEN_COMMA,
    VITTE_TOKEN_DOT,
    VITTE_TOKEN_COLON,
    VITTE_TOKEN_DOUBLE_COLON,
    VITTE_TOKEN_SEMICOLON,
    VITTE_TOKEN_EQUAL,
    VITTE_TOKEN_EQUAL_EQUAL,
    VITTE_TOKEN_BANG,
    VITTE_TOKEN_BANG_EQUAL,
    VITTE_TOKEN_PLUS,
    VITTE_TOKEN_MINUS,
    VITTE_TOKEN_STAR,
    VITTE_TOKEN_SLASH,
    VITTE_TOKEN_PERCENT,
    VITTE_TOKEN_AMP,
    VITTE_TOKEN_AMP_AMP,
    VITTE_TOKEN_PIPE,
    VITTE_TOKEN_PIPE_PIPE,
    VITTE_TOKEN_CARET,
    VITTE_TOKEN_LESS,
    VITTE_TOKEN_LESS_EQUAL,
    VITTE_TOKEN_SHIFT_LEFT,
    VITTE_TOKEN_GREATER,
    VITTE_TOKEN_GREATER_EQUAL,
    VITTE_TOKEN_SHIFT_RIGHT,
    VITTE_TOKEN_ARROW,
    VITTE_TOKEN_KIND_COUNT
} vitte_token_kind_t;

typedef struct vitte_token {
    vitte_token_kind_t kind;
    const char *source_name;
    const char *lexeme_start;
    size_t lexeme_length;
    size_t start_offset;
    size_t end_offset;
    uint32_t start_line;
    uint32_t start_column;
    uint32_t end_line;
    uint32_t end_column;
    int64_t integer_value;
    bool has_integer_value;
    bool has_escape;
    const char *message;
} vitte_token_t;

typedef struct vitte_lexer_options {
    bool emit_comments;
    bool emit_whitespace;
    bool keywords_enabled;
    size_t max_source_bytes;
    size_t tab_width;
} vitte_lexer_options_t;

typedef struct vitte_lexer_result {
    vitte_status_t status;
    size_t token_count;
    size_t error_count;
    vitte_token_t last_token;
} vitte_lexer_result_t;

typedef struct vitte_lexer_stats {
    size_t token_count;
    size_t error_count;
    size_t line_count;
    size_t bytes_consumed;
    size_t comment_count;
    size_t whitespace_count;
} vitte_lexer_stats_t;

typedef struct vitte_lexer_cursor {
    size_t offset;
    uint32_t line;
    uint32_t column;
} vitte_lexer_cursor_t;

typedef struct vitte_lexer {
    bool initialized;
    const char *source_name;
    const char *source;
    size_t length;
    vitte_lexer_cursor_t cursor;
    vitte_lexer_options_t options;
    vitte_token_t peeked;
    bool has_peeked;
    vitte_lexer_stats_t stats;
    vitte_error_t last_error;
} vitte_lexer_t;

void vitte_token_init(vitte_token_t *token);
void vitte_lexer_options_init(vitte_lexer_options_t *options);
void vitte_lexer_result_init(vitte_lexer_result_t *result);
void vitte_lexer_stats_init(vitte_lexer_stats_t *stats);

vitte_status_t vitte_lexer_init(
    vitte_lexer_t *lexer,
    const char *source_name,
    const char *source,
    size_t length,
    const vitte_lexer_options_t *options
);
void vitte_lexer_reset(vitte_lexer_t *lexer);
bool vitte_lexer_is_initialized(const vitte_lexer_t *lexer);
const vitte_error_t *vitte_lexer_last_error(const vitte_lexer_t *lexer);
const vitte_lexer_stats_t *vitte_lexer_stats(const vitte_lexer_t *lexer);

const char *vitte_token_kind_name(vitte_token_kind_t kind);
bool vitte_token_kind_is_keyword(vitte_token_kind_t kind);

vitte_status_t vitte_lexer_next(vitte_lexer_t *lexer, vitte_token_t *token);
vitte_status_t vitte_lexer_peek(vitte_lexer_t *lexer, vitte_token_t *token);
vitte_status_t vitte_lexer_lex_all(
    vitte_lexer_t *lexer,
    vitte_token_t *storage,
    size_t capacity,
    vitte_lexer_result_t *result
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_LEXER_H */
