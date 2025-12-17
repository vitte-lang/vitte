#include "compiler/lexer.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Keyword table */
typedef struct {
    const char *text;
    token_kind_t kind;
} keyword_t;

static const keyword_t KEYWORDS[] = {
    /* Module system */
    {"module", TOK_KEYWORD},
    {"import", TOK_KEYWORD},
    {"export", TOK_KEYWORD},
    {"use", TOK_KEYWORD},
    
    /* Types */
    {"type", TOK_KEYWORD},
    {"struct", TOK_KEYWORD},
    {"enum", TOK_KEYWORD},
    {"union", TOK_KEYWORD},
    
    /* Functions */
    {"fn", TOK_KEYWORD},
    {"scenario", TOK_KEYWORD},
    
    /* Entrypoints */
    {"program", TOK_KEYWORD},
    {"service", TOK_KEYWORD},
    {"kernel", TOK_KEYWORD},
    {"driver", TOK_KEYWORD},
    {"tool", TOK_KEYWORD},
    {"pipeline", TOK_KEYWORD},
    
    /* Statements */
    {"let", TOK_KEYWORD},
    {"const", TOK_KEYWORD},
    {"if", TOK_KEYWORD},
    {"elif", TOK_KEYWORD},
    {"else", TOK_KEYWORD},
    {"while", TOK_KEYWORD},
    {"for", TOK_KEYWORD},
    {"match", TOK_KEYWORD},
    {"break", TOK_KEYWORD},
    {"continue", TOK_KEYWORD},
    {"return", TOK_KEYWORD},
    
    /* Phrase sugar */
    {"set", TOK_KEYWORD},
    {"say", TOK_KEYWORD},
    {"do", TOK_KEYWORD},
    {"ret", TOK_KEYWORD},
    {"when", TOK_KEYWORD},
    {"loop", TOK_KEYWORD},
    
    /* Literals */
    {"true", TOK_KEYWORD},
    {"false", TOK_KEYWORD},
    {"null", TOK_KEYWORD},
    {"end", TOK_KEYWORD},
};

#define KEYWORDS_COUNT (sizeof(KEYWORDS) / sizeof(KEYWORDS[0]))

/* Character classification */
static int is_ident_start(char c) {
    return isalpha(c) || c == '_';
}

static int is_ident_cont(char c) {
    return isalnum(c) || c == '_';
}

static int is_digit(char c) {
    return isdigit(c);
}

static int is_hex_digit(char c) {
    return isxdigit(c);
}

static int is_octal_digit(char c) {
    return c >= '0' && c <= '7';
}

static int is_binary_digit(char c) {
    return c == '0' || c == '1';
}

/* Keyword lookup */
static token_kind_t lookup_keyword(const char *text, uint32_t len) {
    for (uint32_t i = 0; i < KEYWORDS_COUNT; i++) {
        const char *kw = KEYWORDS[i].text;
        if (strlen(kw) == len && strncmp(kw, text, len) == 0) {
            return KEYWORDS[i].kind;
        }
    }
    return TOK_IDENT;
}

lexer_t* lexer_create(const char *input, uint32_t len) {
    lexer_t *lex = (lexer_t *)malloc(sizeof(lexer_t));
    if (!lex) return NULL;
    
    lex->input = input;
    lex->len = len;
    lex->pos = 0;
    lex->line = 1;
    lex->col = 1;
    
    return lex;
}

void lexer_destroy(lexer_t *lex) {
    if (!lex) return;
    free(lex);
}

static void skip_whitespace(lexer_t *lex) {
    while (lex->pos < lex->len) {
        char c = lex->input[lex->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lex->pos++;
            lex->col++;
        } else if (c == '\n') {
            lex->pos++;
            lex->line++;
            lex->col = 1;
        } else {
            break;
        }
    }
}

static token_t make_token(token_kind_t kind, uint32_t line, uint32_t col, uint32_t start_pos, uint32_t len) {
    (void)start_pos;
    token_t tok;
    tok.kind = kind;
    tok.line = line;
    tok.col = col;
    tok.lexeme = NULL;
    tok.len = len;
    return tok;
}

static token_t scan_string(lexer_t *lex, char quote) {
    uint32_t start_line = lex->line;
    uint32_t start_col = lex->col;
    uint32_t start_pos = lex->pos;
    
    lex->pos++;  /* Skip opening quote */
    lex->col++;
    
    while (lex->pos < lex->len) {
        char c = lex->input[lex->pos];
        
        if (c == quote) {
            lex->pos++;
            lex->col++;
            return make_token(TOK_STRING, start_line, start_col, start_pos, lex->pos - start_pos);
        } else if (c == '\\') {
            lex->pos += 2;
            lex->col += 2;
        } else if (c == '\n') {
            lex->line++;
            lex->col = 1;
            lex->pos++;
        } else {
            lex->pos++;
            lex->col++;
        }
    }
    
    return make_token(TOK_ERROR, start_line, start_col, start_pos, lex->pos - start_pos);
}

static token_t scan_number(lexer_t *lex) {
    uint32_t start_line = lex->line;
    uint32_t start_col = lex->col;
    uint32_t start_pos = lex->pos;
    int is_float = 0;
    
    /* Check for hex, octal, binary prefixes */
    if (lex->input[lex->pos] == '0' && lex->pos + 1 < lex->len) {
        char next = lex->input[lex->pos + 1];
        if (next == 'x' || next == 'X') {
            lex->pos += 2;
            lex->col += 2;
            while (lex->pos < lex->len && (is_hex_digit(lex->input[lex->pos]) || lex->input[lex->pos] == '_')) {
                lex->pos++;
                lex->col++;
            }
            return make_token(TOK_INT, start_line, start_col, start_pos, lex->pos - start_pos);
        } else if (next == 'b' || next == 'B') {
            lex->pos += 2;
            lex->col += 2;
            while (lex->pos < lex->len && (is_binary_digit(lex->input[lex->pos]) || lex->input[lex->pos] == '_')) {
                lex->pos++;
                lex->col++;
            }
            return make_token(TOK_INT, start_line, start_col, start_pos, lex->pos - start_pos);
        } else if (next == 'o' || next == 'O') {
            lex->pos += 2;
            lex->col += 2;
            while (lex->pos < lex->len && (is_octal_digit(lex->input[lex->pos]) || lex->input[lex->pos] == '_')) {
                lex->pos++;
                lex->col++;
            }
            return make_token(TOK_INT, start_line, start_col, start_pos, lex->pos - start_pos);
        }
    }
    
    /* Decimal integer or float */
    while (lex->pos < lex->len && (is_digit(lex->input[lex->pos]) || lex->input[lex->pos] == '_')) {
        lex->pos++;
        lex->col++;
    }
    
    /* Check for float */
    if (lex->pos < lex->len && lex->input[lex->pos] == '.') {
        if (lex->pos + 1 < lex->len && is_digit(lex->input[lex->pos + 1])) {
            lex->pos++;
            lex->col++;
            while (lex->pos < lex->len && (is_digit(lex->input[lex->pos]) || lex->input[lex->pos] == '_')) {
                lex->pos++;
                lex->col++;
            }
            is_float = 1;
        }
    }
    
    /* Check for exponent */
    if (lex->pos < lex->len && (lex->input[lex->pos] == 'e' || lex->input[lex->pos] == 'E')) {
        lex->pos++;
        lex->col++;
        if (lex->pos < lex->len && (lex->input[lex->pos] == '+' || lex->input[lex->pos] == '-')) {
            lex->pos++;
            lex->col++;
        }
        while (lex->pos < lex->len && is_digit(lex->input[lex->pos])) {
            lex->pos++;
            lex->col++;
        }
        is_float = 1;
    }
    
    token_kind_t kind = is_float ? TOK_FLOAT : TOK_INT;
    return make_token(kind, start_line, start_col, start_pos, lex->pos - start_pos);
}

static token_t scan_ident(lexer_t *lex) {
    uint32_t start_line = lex->line;
    uint32_t start_col = lex->col;
    uint32_t start_pos = lex->pos;
    
    while (lex->pos < lex->len && is_ident_cont(lex->input[lex->pos])) {
        lex->pos++;
        lex->col++;
    }
    
    uint32_t len = lex->pos - start_pos;
    token_kind_t kind = lookup_keyword(lex->input + start_pos, len);
    
    return make_token(kind, start_line, start_col, start_pos, len);
}

static token_t scan_next(lexer_t *lex) {
    skip_whitespace(lex);
    
    if (lex->pos >= lex->len) {
        return make_token(TOK_EOF, lex->line, lex->col, lex->pos, 0);
    }
    
    char c = lex->input[lex->pos];
    uint32_t start_line = lex->line;
    uint32_t start_col = lex->col;
    uint32_t start_pos = lex->pos;
    
    /* Strings and chars */
    if (c == '"') {
        return scan_string(lex, '"');
    }
    if (c == '\'') {
        return scan_string(lex, '\'');
    }
    
    /* Numbers */
    if (is_digit(c)) {
        return scan_number(lex);
    }
    
    /* Identifiers and keywords */
    if (is_ident_start(c)) {
        return scan_ident(lex);
    }
    
    /* Comments */
    if (c == '/' && lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '/') {
        lex->pos += 2;
        lex->col += 2;
        while (lex->pos < lex->len && lex->input[lex->pos] != '\n') {
            lex->pos++;
            lex->col++;
        }
        return make_token(TOK_STRING, start_line, start_col, start_pos, lex->pos - start_pos);
    }
    
    if (c == '/' && lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '*') {
        lex->pos += 2;
        lex->col += 2;
        while (lex->pos < lex->len) {
            if (lex->input[lex->pos] == '*' && lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '/') {
                lex->pos += 2;
                lex->col += 2;
                break;
            }
            if (lex->input[lex->pos] == '\n') {
                lex->line++;
                lex->col = 1;
            } else {
                lex->col++;
            }
            lex->pos++;
        }
        return make_token(TOK_STRING, start_line, start_col, start_pos, lex->pos - start_pos);
    }
    
    /* Operators and punctuation */
    lex->pos++;
    lex->col++;
    
    if (c == '(') return make_token(TOK_LPAREN, start_line, start_col, start_pos, 1);
    if (c == ')') return make_token(TOK_RPAREN, start_line, start_col, start_pos, 1);
    if (c == '[') return make_token(TOK_LBRACK, start_line, start_col, start_pos, 1);
    if (c == ']') return make_token(TOK_RBRACK, start_line, start_col, start_pos, 1);
    if (c == '{') return make_token(TOK_LBRACE, start_line, start_col, start_pos, 1);
    if (c == '}') return make_token(TOK_RBRACE, start_line, start_col, start_pos, 1);
    if (c == ',') return make_token(TOK_COMMA, start_line, start_col, start_pos, 1);
    if (c == ';') return make_token(TOK_SEMICOLON, start_line, start_col, start_pos, 1);
    if (c == '.') return make_token(TOK_DOT, start_line, start_col, start_pos, 1);
    if (c == ':') return make_token(TOK_COLON, start_line, start_col, start_pos, 1);
    if (c == '+') return make_token(TOK_PLUS, start_line, start_col, start_pos, 1);
    if (c == '-') return make_token(TOK_MINUS, start_line, start_col, start_pos, 1);
    if (c == '*') return make_token(TOK_STAR, start_line, start_col, start_pos, 1);
    if (c == '/') return make_token(TOK_SLASH, start_line, start_col, start_pos, 1);
    if (c == '%') return make_token(TOK_PERCENT, start_line, start_col, start_pos, 1);
    if (c == '=') return make_token(TOK_ASSIGN, start_line, start_col, start_pos, 1);
    if (c == '<') return make_token(TOK_LT, start_line, start_col, start_pos, 1);
    if (c == '>') return make_token(TOK_GT, start_line, start_col, start_pos, 1);
    if (c == '!') return make_token(TOK_BANG, start_line, start_col, start_pos, 1);
    if (c == '&') return make_token(TOK_AMP, start_line, start_col, start_pos, 1);
    if (c == '|') return make_token(TOK_PIPE, start_line, start_col, start_pos, 1);
    if (c == '^') return make_token(TOK_CARET, start_line, start_col, start_pos, 1);
    if (c == '~') return make_token(TOK_TILDE, start_line, start_col, start_pos, 1);
    if (c == '?') return make_token(TOK_QUESTION, start_line, start_col, start_pos, 1);
    
    return make_token(TOK_ERROR, start_line, start_col, start_pos, 1);
}

token_t* lexer_tokenize(lexer_t *lex, uint32_t *out_count) {
    if (!lex || !out_count) return NULL;
    
    uint32_t capacity = 64;
    uint32_t count = 0;
    token_t *tokens = (token_t *)malloc(capacity * sizeof(token_t));
    
    if (!tokens) {
        *out_count = 0;
        return NULL;
    }
    
    while (lex->pos <= lex->len) {
        token_t tok = scan_next(lex);
        
        if (count >= capacity) {
            capacity *= 2;
            token_t *new_tokens = (token_t *)realloc(tokens, capacity * sizeof(token_t));
            if (!new_tokens) {
                free(tokens);
                *out_count = 0;
                return NULL;
            }
            tokens = new_tokens;
        }
        
        tokens[count++] = tok;
        
        if (tok.kind == TOK_EOF) {
            break;
        }
    }
    
    *out_count = count;
    return tokens;
}
