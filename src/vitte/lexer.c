#include "vitte/lexer.h"
#include "diag_codes.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    vitte_token_kind kind;
    const char* word;
} vitte_keyword;

static const vitte_keyword g_keywords[] = {
    {VITTE_TOK_AND, "and"},
    {VITTE_TOK_OR, "or"},
    {VITTE_TOK_NOT, "not"},
    {VITTE_TOK_KW_MOD, "mod"},
    {VITTE_TOK_KW_USE, "use"},
    {VITTE_TOK_KW_TYPE, "type"},
    {VITTE_TOK_KW_FIELD, "field"},
    {VITTE_TOK_KW_FN, "fn"},
    {VITTE_TOK_KW_SCN, "scn"},
    {VITTE_TOK_KW_PROG, "prog"},
    {VITTE_TOK_KW_SET, "set"},
    {VITTE_TOK_KW_SAY, "say"},
    {VITTE_TOK_KW_DO, "do"},
    {VITTE_TOK_KW_RET, "ret"},
    {VITTE_TOK_KW_WHEN, "when"},
    {VITTE_TOK_KW_ELSE, "else"},
    {VITTE_TOK_KW_LOOP, "loop"},
    {VITTE_TOK_KW_FROM, "from"},
    {VITTE_TOK_KW_TO, "to"},
    {VITTE_TOK_KW_STEP, "step"},
    {VITTE_TOK_KW_AS, "as"},
    {VITTE_TOK_KW_IN, "in"},
    {VITTE_TOK_KW_PROGRAM, "program"},
    {VITTE_TOK_KW_SERVICE, "service"},
    {VITTE_TOK_KW_KERNEL, "kernel"},
    {VITTE_TOK_KW_DRIVER, "driver"},
    {VITTE_TOK_KW_TOOL, "tool"},
    {VITTE_TOK_KW_PIPELINE, "pipeline"},
    {VITTE_TOK_KW_SCENARIO, "scenario"}
};

static int is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static int is_ident_continue(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

static vitte_token_kind keyword_lookup(const char* start, size_t len) {
    for (size_t i = 0; i < sizeof(g_keywords) / sizeof(g_keywords[0]); ++i) {
        const vitte_keyword* kw = &g_keywords[i];
        if (strlen(kw->word) == len && strncmp(kw->word, start, len) == 0) {
            return kw->kind;
        }
    }
    if (len == 4 && strncmp(start, "true", 4) == 0) return VITTE_TOK_TRUE;
    if (len == 5 && strncmp(start, "false", 5) == 0) return VITTE_TOK_FALSE;
    if (len == 3 && strncmp(start, "nil", 3) == 0) return VITTE_TOK_NIL;
    return VITTE_TOK_IDENT;
}

typedef struct {
    vitte_token* data;
    size_t count;
    size_t cap;
} token_vec;

static int reserve_tokens(token_vec* vec, size_t extra) {
    if (vec->count + extra <= vec->cap) return 1;
    size_t new_cap = vec->cap ? vec->cap : 64;
    while (new_cap < vec->count + extra) {
        new_cap *= 2;
    }
    vitte_token* nt = (vitte_token*)realloc(vec->data, new_cap * sizeof(vitte_token));
    if (!nt) return 0;
    vec->data = nt;
    vec->cap = new_cap;
    return 1;
}

static void push_token(token_vec* vec, vitte_token_kind kind, const char* lexeme, size_t len, vitte_file_id file_id, uint32_t lo, uint32_t hi) {
    vec->data[vec->count].kind = kind;
    vec->data[vec->count].lexeme = lexeme;
    vec->data[vec->count].len = len;
    vec->data[vec->count].span = vitte_span_make(file_id, lo, hi);
    vec->count++;
}

static int match_word(const char* cur, const char* end, const char* word, size_t len) {
    if ((size_t)(end - cur) < len) return 0;
    return strncmp(cur, word, len) == 0;
}

static int is_word_boundary(char c) {
    return c == '\0' ||
           isspace((unsigned char)c) ||
           c == '.' || c == ',' || c == ':' || c == '(' || c == ')' ||
           c == '{' || c == '}' || c == '[' || c == ']' ||
           c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '=' || c == '!' || c == '<' || c == '>' ||
           c == '"' || c == '\'' || c == '#';
}

typedef struct {
    const char* src;
    const char* cur;
    const char* end;
    vitte_file_id file_id;
    uint32_t off;
    vitte_ctx* ctx;
} lexer_state;

static void advance_char(lexer_state* st) {
    st->off++;
    st->cur++;
}

static void skip_comment(lexer_state* st) {
    while (st->cur < st->end && *st->cur != '\n') {
        advance_char(st);
    }
}

static vitte_result emit_error(vitte_diag_bag* diags, vitte_file_id file_id, uint32_t lo, uint32_t hi, vitte_error_code code, const char* msg) {
    if (hi < lo) hi = lo;
    vitte_diag* d = vitte_diag_bag_push(diags, VITTE_SEV_ERROR, vitte_errc_code(code), vitte_span_make(file_id, lo, hi), msg ? msg : "");
    if (d) {
        const char* help = vitte_errc_help(code);
        if (help && help[0]) vitte_diag_set_help(d, help);
    }
    return VITTE_ERR_LEX;
}

static vitte_result lex_string_literal(lexer_state* st,
                                       token_vec* vec,
                                       vitte_diag_bag* diags) {
    const char quote = *st->cur;
    const char* begin = st->cur;
    uint32_t lo = st->off;
    advance_char(st); /* skip quote */
    while (st->cur < st->end) {
        char c = *st->cur;
        if (c == '\\') {
            advance_char(st);
            if (st->cur >= st->end) break;
            advance_char(st);
            continue;
        }
        if (c == quote) {
            advance_char(st);
            size_t len = (size_t)(st->cur - begin);
            push_token(vec, VITTE_TOK_STRING, begin, len, st->file_id, lo, lo + (uint32_t)len);
            return VITTE_OK;
        }
        if (c == '\n' || c == '\r') {
            return emit_error(diags, st->file_id, lo, lo + 1u, VITTE_ERRC_SYNTAX, "unterminated string literal");
        }
        advance_char(st);
    }
    return emit_error(diags, st->file_id, lo, lo + 1u, VITTE_ERRC_UNEXPECTED_EOF, "unterminated string literal");
}

static vitte_result lex_number_literal(lexer_state* st,
                                       token_vec* vec,
                                       vitte_diag_bag* diags) {
    (void)diags;
    uint32_t lo = st->off;
    const char* start = st->cur;
    int has_dot = 0;
    while (st->cur < st->end && isdigit((unsigned char)*st->cur)) {
        advance_char(st);
    }
    if (st->cur < st->end && *st->cur == '.' && st->cur + 1 < st->end && isdigit((unsigned char)st->cur[1])) {
        has_dot = 1;
        advance_char(st); /* consume '.' */
        while (st->cur < st->end && isdigit((unsigned char)*st->cur)) {
            advance_char(st);
        }
    }
    size_t len = (size_t)(st->cur - start);
    push_token(vec, has_dot ? VITTE_TOK_FLOAT : VITTE_TOK_INT, start, len, st->file_id, lo, lo + (uint32_t)len);
    return VITTE_OK;
}

static vitte_result lex_ident_or_keyword(lexer_state* st,
                                         token_vec* vec,
                                         vitte_diag_bag* diags) {
    (void)diags;
    uint32_t lo = st->off;
    const char* start = st->cur;
    while (st->cur < st->end && is_ident_continue(*st->cur)) {
        advance_char(st);
    }
    size_t len = (size_t)(st->cur - start);
    vitte_token_kind kind = keyword_lookup(start, len);
    push_token(vec, kind, start, len, st->file_id, lo, lo + (uint32_t)len);
    return VITTE_OK;
}

static vitte_result lex_dot(lexer_state* st,
                            token_vec* vec) {
    uint32_t lo = st->off;
    const char* start = st->cur;
    if (st->cur + 4 <= st->end && match_word(st->cur, st->end, ".end", 4)) {
        const char next = (st->cur + 4) >= st->end ? '\0' : st->cur[4];
        if (is_word_boundary(next)) {
            st->cur += 4;
            st->off += 4u;
            push_token(vec, VITTE_TOK_DOTEND, start, 4, st->file_id, lo, lo + 4u);
            return VITTE_OK;
        }
    }
    advance_char(st);
    push_token(vec, VITTE_TOK_DOT, start, 1, st->file_id, lo, lo + 1u);
    return VITTE_OK;
}

static vitte_result lex_newline(lexer_state* st,
                                token_vec* vec) {
    uint32_t lo = st->off;
    const char* start = st->cur;
    advance_char(st);
    if (*start == '\r' && st->cur < st->end && *st->cur == '\n') {
        advance_char(st);
    }
    push_token(vec, VITTE_TOK_NEWLINE, start, 1, st->file_id, lo, lo + 1u);
    return VITTE_OK;
}

static vitte_result lex_simple_token(lexer_state* st,
                                     token_vec* vec,
                                     vitte_token_kind kind,
                                     size_t width) {
    uint32_t lo = st->off;
    const char* start = st->cur;
    for (size_t i = 0; i < width; ++i) advance_char(st);
    push_token(vec, kind, start, width, st->file_id, lo, lo + (uint32_t)width);
    return VITTE_OK;
}

vitte_result vitte_lex_all(vitte_ctx* ctx,
                           vitte_file_id file_id,
                           const char* src,
                           size_t len,
                           vitte_token** out_tokens,
                           size_t* out_count,
                           vitte_diag_bag* diags) {
    (void)ctx;
    if (!src || !out_tokens || !out_count) {
        return emit_error(diags, file_id, 0u, 0u, VITTE_ERRC_SYNTAX, "invalid arguments");
    }

    lexer_state st;
    st.src = src;
    st.cur = src;
    st.end = src + len;
    st.file_id = file_id;
    st.off = 0u;
    st.ctx = ctx;

    token_vec vec = {0};
    if (!reserve_tokens(&vec, 64)) {
        return VITTE_ERR_INTERNAL;
    }

    while (st.cur < st.end) {
        char c = *st.cur;
        if (c == ' ' || c == '\t' || c == '\f' || c == '\v') {
            advance_char(&st);
            continue;
        }
        if (c == '\r' || c == '\n') {
            if (!reserve_tokens(&vec, 1)) goto oom;
            lex_newline(&st, &vec);
            continue;
        }
        if (c == '#') {
            skip_comment(&st);
            continue;
        }
        if (c == '/' && st.cur + 1 < st.end && st.cur[1] == '/') {
            skip_comment(&st);
            continue;
        }
        if (isdigit((unsigned char)c)) {
            if (!reserve_tokens(&vec, 1)) goto oom;
            vitte_result r = lex_number_literal(&st, &vec, diags);
            if (r != VITTE_OK) goto fail;
            continue;
        }
        if (is_ident_start(c)) {
            if (!reserve_tokens(&vec, 1)) goto oom;
            vitte_result r = lex_ident_or_keyword(&st, &vec, diags);
            if (r != VITTE_OK) goto fail;
            continue;
        }
        if (c == '"' || c == '\'') {
            if (!reserve_tokens(&vec, 1)) goto oom;
            vitte_result r = lex_string_literal(&st, &vec, diags);
            if (r != VITTE_OK) goto fail;
            continue;
        }

        if (!reserve_tokens(&vec, 1)) goto oom;
        switch (c) {
            case '.':
                if (lex_dot(&st, &vec) != VITTE_OK) goto fail;
                break;
            case ',':
                lex_simple_token(&st, &vec, VITTE_TOK_COMMA, 1);
                break;
            case ':':
                if (st.cur + 1 < st.end && st.cur[1] == ':') {
                    lex_simple_token(&st, &vec, VITTE_TOK_DOUBLECOLON, 2);
                } else {
                    lex_simple_token(&st, &vec, VITTE_TOK_COLON, 1);
                }
                break;
            case ';':
                lex_simple_token(&st, &vec, VITTE_TOK_SEMICOLON, 1);
                break;
            case '(':
                lex_simple_token(&st, &vec, VITTE_TOK_LPAREN, 1);
                break;
            case ')':
                lex_simple_token(&st, &vec, VITTE_TOK_RPAREN, 1);
                break;
            case '{':
                lex_simple_token(&st, &vec, VITTE_TOK_LBRACE, 1);
                break;
            case '}':
                lex_simple_token(&st, &vec, VITTE_TOK_RBRACE, 1);
                break;
            case '[':
                lex_simple_token(&st, &vec, VITTE_TOK_LBRACKET, 1);
                break;
            case ']':
                lex_simple_token(&st, &vec, VITTE_TOK_RBRACKET, 1);
                break;
            case '+':
                lex_simple_token(&st, &vec, VITTE_TOK_PLUS, 1);
                break;
            case '-':
                if (st.cur + 1 < st.end && st.cur[1] == '>') {
                    lex_simple_token(&st, &vec, VITTE_TOK_ARROW, 2);
                } else {
                    lex_simple_token(&st, &vec, VITTE_TOK_MINUS, 1);
                }
                break;
            case '*':
                lex_simple_token(&st, &vec, VITTE_TOK_STAR, 1);
                break;
            case '/':
                lex_simple_token(&st, &vec, VITTE_TOK_SLASH, 1);
                break;
            case '%':
                lex_simple_token(&st, &vec, VITTE_TOK_PERCENT, 1);
                break;
            case '=':
                if (st.cur + 1 < st.end && st.cur[1] == '=') {
                    lex_simple_token(&st, &vec, VITTE_TOK_EQEQ, 2);
                } else {
                    lex_simple_token(&st, &vec, VITTE_TOK_EQUAL, 1);
                }
                break;
            case '!':
                if (st.cur + 1 < st.end && st.cur[1] == '=') {
                    lex_simple_token(&st, &vec, VITTE_TOK_NEQ, 2);
                } else {
                    goto invalid_char;
                }
                break;
            case '<':
                if (st.cur + 1 < st.end && st.cur[1] == '=') {
                    lex_simple_token(&st, &vec, VITTE_TOK_LTE, 2);
                } else {
                    lex_simple_token(&st, &vec, VITTE_TOK_LT, 1);
                }
                break;
            case '>':
                if (st.cur + 1 < st.end && st.cur[1] == '=') {
                    lex_simple_token(&st, &vec, VITTE_TOK_GTE, 2);
                } else {
                    lex_simple_token(&st, &vec, VITTE_TOK_GT, 1);
                }
                break;
            default:
            invalid_char:
                (void)emit_error(diags, st.file_id, st.off, st.off + 1u, VITTE_ERRC_SYNTAX, "invalid character");
                goto fail;
        }
    }

    if (!reserve_tokens(&vec, 1)) goto oom;
    push_token(&vec, VITTE_TOK_EOF, st.cur, 0, st.file_id, st.off, st.off);
    *out_tokens = vec.data;
    *out_count = vec.count;
    return VITTE_OK;

oom:
    free(vec.data);
    return VITTE_ERR_INTERNAL;
fail:
    free(vec.data);
    return VITTE_ERR_LEX;
}
