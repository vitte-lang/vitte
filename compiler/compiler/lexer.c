

// SPDX-License-Identifier: MIT
// lexer.c
//
// Vitte lexer (core + phrase).
//
// Design goals:
//  - Fast single-pass tokenizer over UTF-8 bytes (lexing is byte-oriented).
//  - Stable spans (byte offsets) + line/col tracking.
//  - Support both "core" and "phrase" surface forms:
//      - keywords: mod/use/export/type/struct/union/enum/fn/scn/prog/...
//      - phrase: set/say/do/when/loop/ret
//      - block terminator token: `.end` (single token)
//  - Pragmatic: accept legacy braces `{}` as tokens (parser may ignore/legacy).
//
// Notes:
//  - This file assumes lexer.h defines steel_lexer, steel_token, steel_token_kind,
//    and (optionally) steel_span.
//  - If your lexer.h uses different names, keep this implementation and add
//    thin wrappers in lexer.h.

#include "lexer.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#ifndef STEEL_LEXER_MAX_TOKEN_TEXT
  #define STEEL_LEXER_MAX_TOKEN_TEXT 256
#endif

#ifndef STEEL_LEXER_ENABLE_C_STYLE_COMMENTS
  #define STEEL_LEXER_ENABLE_C_STYLE_COMMENTS 1
#endif

//------------------------------------------------------------------------------
// Helpers: character classes
//------------------------------------------------------------------------------

static inline bool steel_is_alpha_(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline bool steel_is_digit_(unsigned char c)
{
    return (c >= '0' && c <= '9');
}

static inline bool steel_is_ident_start_(unsigned char c)
{
    return steel_is_alpha_(c) || c == '_';
}

static inline bool steel_is_ident_cont_(unsigned char c)
{
    return steel_is_ident_start_(c) || steel_is_digit_(c);
}

static inline int steel_hex_val_(unsigned char c)
{
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return (int)(10 + (c - 'a'));
    if (c >= 'A' && c <= 'F') return (int)(10 + (c - 'A'));
    return -1;
}

//------------------------------------------------------------------------------
// Span helpers (best-effort)
//------------------------------------------------------------------------------

// Many parts of the compiler use a span struct; we fill what we can.
// If lexer.h defines a different layout, keep fields compatible or ignore.

static inline void steel_span_set_(steel_span* sp,
                                  uint32_t file_id,
                                  uint32_t start,
                                  uint32_t end,
                                  uint32_t line,
                                  uint32_t col)
{
    if (!sp) return;
    sp->file_id = file_id;
    sp->start = start;
    sp->end = end;
    sp->line = line;
    sp->col = col;
}

//------------------------------------------------------------------------------
// Internal accessors over steel_lexer
//------------------------------------------------------------------------------

static inline unsigned char steel_lx_peek_(const steel_lexer* lx)
{
    if (!lx) return 0;
    if (lx->pos >= lx->len) return 0;
    return (unsigned char)lx->src[lx->pos];
}

static inline unsigned char steel_lx_peek2_(const steel_lexer* lx)
{
    if (!lx) return 0;
    size_t p = lx->pos + 1;
    if (p >= lx->len) return 0;
    return (unsigned char)lx->src[p];
}

static inline bool steel_lx_eof_(const steel_lexer* lx)
{
    return !lx || lx->pos >= lx->len;
}

static inline unsigned char steel_lx_advance_(steel_lexer* lx)
{
    if (steel_lx_eof_(lx)) return 0;
    unsigned char c = (unsigned char)lx->src[lx->pos++];

    if (c == '\n')
    {
        lx->line += 1;
        lx->col = 1;
    }
    else
    {
        lx->col += 1;
    }

    return c;
}

static inline bool steel_lx_match_(steel_lexer* lx, char want)
{
    if (steel_lx_eof_(lx)) return false;
    if (lx->src[lx->pos] != want) return false;
    (void)steel_lx_advance_(lx);
    return true;
}

//------------------------------------------------------------------------------
// Token construction
//------------------------------------------------------------------------------

static steel_token steel_tok_make_(steel_token_kind kind,
                                  uint32_t file_id,
                                  uint32_t start,
                                  uint32_t end,
                                  uint32_t line,
                                  uint32_t col,
                                  const char* text,
                                  size_t text_len)
{
    steel_token t;
    memset(&t, 0, sizeof(t));

    t.kind = kind;
    steel_span_set_(&t.span, file_id, start, end, line, col);

    t.text = text;
    t.text_len = text_len;

    return t;
}

static steel_token steel_tok_error_(steel_lexer* lx, uint32_t start, uint32_t line, uint32_t col, const char* msg)
{
    // Create an error token; attach msg via text pointer.
    size_t n = msg ? strlen(msg) : 0;
    return steel_tok_make_(STEEL_TOK_ERROR,
                           lx ? lx->file_id : 0,
                           start,
                           lx ? (uint32_t)lx->pos : start,
                           line,
                           col,
                           msg ? msg : "lexer error",
                           n);
}

//------------------------------------------------------------------------------
// Keyword table
//------------------------------------------------------------------------------

typedef struct steel_kw
{
    const char* s;
    steel_token_kind k;
} steel_kw;

static steel_token_kind steel_kw_lookup_(const char* s, size_t n)
{
    // Keep this table stable.
    // Core
    static const steel_kw kws[] = {
        {"mod", STEEL_TOK_KW_MOD},
        {"use", STEEL_TOK_KW_USE},
        {"export", STEEL_TOK_KW_EXPORT},
        {"from", STEEL_TOK_KW_FROM},
        {"as", STEEL_TOK_KW_AS},

        {"type", STEEL_TOK_KW_TYPE},
        {"struct", STEEL_TOK_KW_STRUCT},
        {"union", STEEL_TOK_KW_UNION},
        {"enum", STEEL_TOK_KW_ENUM},

        {"fn", STEEL_TOK_KW_FN},
        {"scn", STEEL_TOK_KW_SCN},
        {"prog", STEEL_TOK_KW_PROG},
        {"program", STEEL_TOK_KW_PROGRAM},
        {"service", STEEL_TOK_KW_SERVICE},
        {"kernel", STEEL_TOK_KW_KERNEL},
        {"driver", STEEL_TOK_KW_DRIVER},
        {"tool", STEEL_TOK_KW_TOOL},
        {"pipeline", STEEL_TOK_KW_PIPELINE},

        {"let", STEEL_TOK_KW_LET},
        {"const", STEEL_TOK_KW_CONST},

        {"if", STEEL_TOK_KW_IF},
        {"elif", STEEL_TOK_KW_ELIF},
        {"else", STEEL_TOK_KW_ELSE},
        {"while", STEEL_TOK_KW_WHILE},
        {"for", STEEL_TOK_KW_FOR},
        {"match", STEEL_TOK_KW_MATCH},
        {"break", STEEL_TOK_KW_BREAK},
        {"continue", STEEL_TOK_KW_CONTINUE},
        {"return", STEEL_TOK_KW_RETURN},

        // Phrase
        {"set", STEEL_TOK_KW_SET},
        {"say", STEEL_TOK_KW_SAY},
        {"do", STEEL_TOK_KW_DO},
        {"when", STEEL_TOK_KW_WHEN},
        {"loop", STEEL_TOK_KW_LOOP},
        {"ret", STEEL_TOK_KW_RET},

        // Literals
        {"true", STEEL_TOK_KW_TRUE},
        {"false", STEEL_TOK_KW_FALSE},
        {"null", STEEL_TOK_KW_NULL},
    };

    for (size_t i = 0; i < sizeof(kws)/sizeof(kws[0]); i++)
    {
        const char* kw = kws[i].s;
        if (strlen(kw) != n) continue;
        if (memcmp(kw, s, n) == 0) return kws[i].k;
    }

    return STEEL_TOK_IDENT;
}

//------------------------------------------------------------------------------
// Whitespace + comments
//------------------------------------------------------------------------------

static void steel_lx_skip_ws_and_comments_(steel_lexer* lx)
{
    for (;;)
    {
        unsigned char c = steel_lx_peek_(lx);

        // whitespace
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            (void)steel_lx_advance_(lx);
            continue;
        }

        // # line comment
        if (c == '#')
        {
            while (!steel_lx_eof_(lx) && steel_lx_peek_(lx) != '\n')
                (void)steel_lx_advance_(lx);
            continue;
        }

#if STEEL_LEXER_ENABLE_C_STYLE_COMMENTS
        // // line comment
        if (c == '/' && steel_lx_peek2_(lx) == '/')
        {
            (void)steel_lx_advance_(lx);
            (void)steel_lx_advance_(lx);
            while (!steel_lx_eof_(lx) && steel_lx_peek_(lx) != '\n')
                (void)steel_lx_advance_(lx);
            continue;
        }

        // /* block comment */
        if (c == '/' && steel_lx_peek2_(lx) == '*')
        {
            (void)steel_lx_advance_(lx);
            (void)steel_lx_advance_(lx);

            while (!steel_lx_eof_(lx))
            {
                unsigned char d = steel_lx_advance_(lx);
                if (d == '*' && steel_lx_peek_(lx) == '/')
                {
                    (void)steel_lx_advance_(lx);
                    break;
                }
            }
            continue;
        }
#endif

        break;
    }
}

//------------------------------------------------------------------------------
// Literal lexing
//------------------------------------------------------------------------------

static steel_token steel_lx_lex_ident_or_kw_(steel_lexer* lx, uint32_t start, uint32_t line, uint32_t col)
{
    while (steel_is_ident_cont_(steel_lx_peek_(lx)))
        (void)steel_lx_advance_(lx);

    uint32_t end = (uint32_t)lx->pos;
    const char* s = lx->src + start;
    size_t n = (size_t)(end - start);

    // Special: `.end` is lexed elsewhere (as a single token). Here we only do normal ident.
    steel_token_kind k = steel_kw_lookup_(s, n);

    return steel_tok_make_(k, lx->file_id, start, end, line, col, s, n);
}

static void steel_strip_underscores_(char* out, size_t* out_n, const char* in, size_t in_n)
{
    size_t w = 0;
    for (size_t i = 0; i < in_n; i++)
        if (in[i] != '_') out[w++] = in[i];
    out[w] = 0;
    *out_n = w;
}

static steel_token steel_lx_lex_number_(steel_lexer* lx, uint32_t start, uint32_t line, uint32_t col)
{
    // Supports:
    //  - decimal ints/floats
    //  - 0x/0b/0o ints
    //  - underscores as separators
    //  - exponent for float

    bool is_float = false;

    // leading digits already present
    if (steel_lx_peek_(lx) == '0')
    {
        unsigned char p1 = steel_lx_peek2_(lx);
        if (p1 == 'x' || p1 == 'X' || p1 == 'b' || p1 == 'B' || p1 == 'o' || p1 == 'O')
        {
            (void)steel_lx_advance_(lx); // '0'
            (void)steel_lx_advance_(lx); // base char

            while (true)
            {
                unsigned char c = steel_lx_peek_(lx);
                if (c == '_') { (void)steel_lx_advance_(lx); continue; }

                if (p1 == 'x' || p1 == 'X')
                {
                    if (steel_hex_val_(c) >= 0) { (void)steel_lx_advance_(lx); continue; }
                }
                else if (p1 == 'b' || p1 == 'B')
                {
                    if (c == '0' || c == '1') { (void)steel_lx_advance_(lx); continue; }
                }
                else
                {
                    if (c >= '0' && c <= '7') { (void)steel_lx_advance_(lx); continue; }
                }

                break;
            }

            uint32_t end = (uint32_t)lx->pos;
            const char* s = lx->src + start;
            size_t n = (size_t)(end - start);

            return steel_tok_make_(STEEL_TOK_INT, lx->file_id, start, end, line, col, s, n);
        }
    }

    // decimal / float
    while (true)
    {
        unsigned char c = steel_lx_peek_(lx);
        if (steel_is_digit_(c) || c == '_')
        {
            (void)steel_lx_advance_(lx);
            continue;
        }
        break;
    }

    // fractional part
    if (steel_lx_peek_(lx) == '.' && steel_is_digit_(steel_lx_peek2_(lx)))
    {
        is_float = true;
        (void)steel_lx_advance_(lx); // '.'
        while (true)
        {
            unsigned char c = steel_lx_peek_(lx);
            if (steel_is_digit_(c) || c == '_') { (void)steel_lx_advance_(lx); continue; }
            break;
        }
    }

    // exponent
    {
        unsigned char c = steel_lx_peek_(lx);
        if (c == 'e' || c == 'E')
        {
            is_float = true;
            (void)steel_lx_advance_(lx);
            if (steel_lx_peek_(lx) == '+' || steel_lx_peek_(lx) == '-')
                (void)steel_lx_advance_(lx);

            // digits
            while (true)
            {
                unsigned char d = steel_lx_peek_(lx);
                if (steel_is_digit_(d) || d == '_') { (void)steel_lx_advance_(lx); continue; }
                break;
            }
        }
    }

    uint32_t end = (uint32_t)lx->pos;
    const char* s = lx->src + start;
    size_t n = (size_t)(end - start);

    return steel_tok_make_(is_float ? STEEL_TOK_FLOAT : STEEL_TOK_INT,
                           lx->file_id, start, end, line, col, s, n);
}

static steel_token steel_lx_lex_string_(steel_lexer* lx, uint32_t start, uint32_t line, uint32_t col, char quote)
{
    // start points at opening quote; consume it
    (void)steel_lx_advance_(lx);

    while (!steel_lx_eof_(lx))
    {
        unsigned char c = steel_lx_advance_(lx);

        if (c == (unsigned char)quote)
        {
            uint32_t end = (uint32_t)lx->pos;
            const char* s = lx->src + start;
            size_t n = (size_t)(end - start);
            return steel_tok_make_(quote == '"' ? STEEL_TOK_STRING : STEEL_TOK_CHAR,
                                   lx->file_id, start, end, line, col, s, n);
        }

        if (c == '\\')
        {
            // escape: consume next char if any
            if (!steel_lx_eof_(lx))
                (void)steel_lx_advance_(lx);
            continue;
        }

        if (c == '\n')
        {
            // Strings cannot span lines in most syntaxes. Keep permissive but error out.
            return steel_tok_error_(lx, start, line, col, "unterminated string literal");
        }
    }

    return steel_tok_error_(lx, start, line, col, "unterminated string literal");
}

// `.end` special-case: consume `.end` and return STEEL_TOK_DOT_END
static steel_token steel_lx_lex_dot_end_(steel_lexer* lx, uint32_t start, uint32_t line, uint32_t col)
{
    // We are at '.'
    (void)steel_lx_advance_(lx); // '.'

    // Expect ident "end"
    if (steel_lx_peek_(lx) == 'e' && lx->pos + 2 < lx->len &&
        lx->src[lx->pos] == 'e' && lx->src[lx->pos + 1] == 'n' && lx->src[lx->pos + 2] == 'd')
    {
        // Ensure boundary
        unsigned char after = 0;
        if (lx->pos + 3 < lx->len) after = (unsigned char)lx->src[lx->pos + 3];

        if (after == 0 || !steel_is_ident_cont_(after))
        {
            lx->pos += 3;
            lx->col += 3;

            uint32_t end = (uint32_t)lx->pos;
            const char* s = lx->src + start;
            size_t n = (size_t)(end - start);
            return steel_tok_make_(STEEL_TOK_DOT_END, lx->file_id, start, end, line, col, s, n);
        }
    }

    // Fallback: just '.' token.
    uint32_t end = (uint32_t)lx->pos;
    return steel_tok_make_(STEEL_TOK_DOT, lx->file_id, start, end, line, col, lx->src + start, 1);
}

//------------------------------------------------------------------------------
// Main lexing routine
//------------------------------------------------------------------------------

static steel_token steel_lx_next_impl_(steel_lexer* lx)
{
    steel_lx_skip_ws_and_comments_(lx);

    uint32_t start = (uint32_t)lx->pos;
    uint32_t line = lx->line;
    uint32_t col = lx->col;

    if (steel_lx_eof_(lx))
    {
        return steel_tok_make_(STEEL_TOK_EOF, lx->file_id, start, start, line, col, lx->src + start, 0);
    }

    unsigned char c = steel_lx_peek_(lx);

    // identifiers / keywords
    if (steel_is_ident_start_(c))
    {
        (void)steel_lx_advance_(lx);
        return steel_lx_lex_ident_or_kw_(lx, start, line, col);
    }

    // numbers
    if (steel_is_digit_(c))
    {
        (void)steel_lx_advance_(lx);
        return steel_lx_lex_number_(lx, start, line, col);
    }

    // strings / chars
    if (c == '"')
        return steel_lx_lex_string_(lx, start, line, col, '"');
    if (c == '\'')
        return steel_lx_lex_string_(lx, start, line, col, '\'');

    // punctuation & operators
    switch (c)
    {
        case '.':
            return steel_lx_lex_dot_end_(lx, start, line, col);

        case ',': (void)steel_lx_advance_(lx); return steel_tok_make_(STEEL_TOK_COMMA, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);
        case ':':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, ':'))
                return steel_tok_make_(STEEL_TOK_COLON_COLON, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_COLON, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case ';': (void)steel_lx_advance_(lx); return steel_tok_make_(STEEL_TOK_SEMI, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '(': (void)steel_lx_advance_(lx); return steel_tok_make_(STEEL_TOK_LPAREN, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);
        case ')': (void)steel_lx_advance_(lx); return steel_tok_make_(STEEL_TOK_RPAREN, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);
        case '[': (void)steel_lx_advance_(lx); return steel_tok_make_(STEEL_TOK_LBRACK, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);
        case ']': (void)steel_lx_advance_(lx); return steel_tok_make_(STEEL_TOK_RBRACK, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '{': (void)steel_lx_advance_(lx); return steel_tok_make_(STEEL_TOK_LBRACE, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);
        case '}': (void)steel_lx_advance_(lx); return steel_tok_make_(STEEL_TOK_RBRACE, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '+':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_PLUS_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_PLUS, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '-':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '>')) return steel_tok_make_(STEEL_TOK_ARROW, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_MINUS_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_MINUS, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '*':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_STAR_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_STAR, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '/':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_SLASH_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_SLASH, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '%':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_PERCENT_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_PERCENT, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '=':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_EQ_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            if (steel_lx_match_(lx, '>')) return steel_tok_make_(STEEL_TOK_FATARROW, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '!':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_BANG_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_BANG, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '<':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_LT_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            if (steel_lx_match_(lx, '<'))
            {
                if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_SHL_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 3);
                return steel_tok_make_(STEEL_TOK_SHL, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            }
            return steel_tok_make_(STEEL_TOK_LT, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '>':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_GT_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            if (steel_lx_match_(lx, '>'))
            {
                if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_SHR_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 3);
                return steel_tok_make_(STEEL_TOK_SHR, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            }
            return steel_tok_make_(STEEL_TOK_GT, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '&':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '&')) return steel_tok_make_(STEEL_TOK_AMP_AMP, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_AMP_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_AMP, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '|':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '|')) return steel_tok_make_(STEEL_TOK_PIPE_PIPE, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_PIPE_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_PIPE, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '^':
            (void)steel_lx_advance_(lx);
            if (steel_lx_match_(lx, '=')) return steel_tok_make_(STEEL_TOK_CARET_EQ, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 2);
            return steel_tok_make_(STEEL_TOK_CARET, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        case '?':
            (void)steel_lx_advance_(lx);
            return steel_tok_make_(STEEL_TOK_QMARK, lx->file_id, start, (uint32_t)lx->pos, line, col, lx->src + start, 1);

        default:
            (void)steel_lx_advance_(lx);
            return steel_tok_error_(lx, start, line, col, "unexpected character");
    }
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_lexer_init(steel_lexer* lx, const char* src, size_t len, uint32_t file_id)
{
    if (!lx) return;
    memset(lx, 0, sizeof(*lx));

    lx->src = src ? src : "";
    lx->len = src ? len : 0;
    lx->pos = 0;

    lx->file_id = file_id;
    lx->line = 1;
    lx->col = 1;

    lx->has_peek = false;
}

steel_token steel_lexer_next(steel_lexer* lx)
{
    if (!lx)
    {
        steel_token t;
        memset(&t, 0, sizeof(t));
        t.kind = STEEL_TOK_EOF;
        return t;
    }

    if (lx->has_peek)
    {
        lx->has_peek = false;
        return lx->peek_tok;
    }

    return steel_lx_next_impl_(lx);
}

steel_token steel_lexer_peek(steel_lexer* lx)
{
    if (!lx)
    {
        steel_token t;
        memset(&t, 0, sizeof(t));
        t.kind = STEEL_TOK_EOF;
        return t;
    }

    if (!lx->has_peek)
    {
        lx->peek_tok = steel_lx_next_impl_(lx);
        lx->has_peek = true;
    }

    return lx->peek_tok;
}

bool steel_lexer_consume(steel_lexer* lx, steel_token_kind want, steel_token* out)
{
    steel_token t = steel_lexer_peek(lx);
    if (t.kind != want) return false;
    t = steel_lexer_next(lx);
    if (out) *out = t;
    return true;
}

const char* steel_token_kind_name(steel_token_kind k)
{
    // Keep this exhaustive for debugging. If lexer.h has fewer tokens,
    // remove unused entries.
    switch (k)
    {
        case STEEL_TOK_EOF: return "eof";
        case STEEL_TOK_ERROR: return "error";

        case STEEL_TOK_IDENT: return "ident";
        case STEEL_TOK_INT: return "int";
        case STEEL_TOK_FLOAT: return "float";
        case STEEL_TOK_STRING: return "string";
        case STEEL_TOK_CHAR: return "char";

        case STEEL_TOK_DOT_END: return ".end";

        case STEEL_TOK_DOT: return ".";
        case STEEL_TOK_COMMA: return ",";
        case STEEL_TOK_COLON: return ":";
        case STEEL_TOK_COLON_COLON: return "::";
        case STEEL_TOK_SEMI: return ";";

        case STEEL_TOK_LPAREN: return "(";
        case STEEL_TOK_RPAREN: return ")";
        case STEEL_TOK_LBRACK: return "[";
        case STEEL_TOK_RBRACK: return "]";
        case STEEL_TOK_LBRACE: return "{";
        case STEEL_TOK_RBRACE: return "}";

        case STEEL_TOK_PLUS: return "+";
        case STEEL_TOK_MINUS: return "-";
        case STEEL_TOK_STAR: return "*";
        case STEEL_TOK_SLASH: return "/";
        case STEEL_TOK_PERCENT: return "%";

        case STEEL_TOK_PLUS_EQ: return "+=";
        case STEEL_TOK_MINUS_EQ: return "-=";
        case STEEL_TOK_STAR_EQ: return "*=";
        case STEEL_TOK_SLASH_EQ: return "/=";
        case STEEL_TOK_PERCENT_EQ: return "%=";

        case STEEL_TOK_EQ: return "=";
        case STEEL_TOK_EQ_EQ: return "==";
        case STEEL_TOK_BANG: return "!";
        case STEEL_TOK_BANG_EQ: return "!=";

        case STEEL_TOK_LT: return "<";
        case STEEL_TOK_LT_EQ: return "<=";
        case STEEL_TOK_GT: return ">";
        case STEEL_TOK_GT_EQ: return ">=";

        case STEEL_TOK_ARROW: return "->";
        case STEEL_TOK_FATARROW: return "=>";

        case STEEL_TOK_AMP: return "&";
        case STEEL_TOK_AMP_AMP: return "&&";
        case STEEL_TOK_AMP_EQ: return "&=";

        case STEEL_TOK_PIPE: return "|";
        case STEEL_TOK_PIPE_PIPE: return "||";
        case STEEL_TOK_PIPE_EQ: return "|=";

        case STEEL_TOK_CARET: return "^";
        case STEEL_TOK_CARET_EQ: return "^=";

        case STEEL_TOK_SHL: return "<<";
        case STEEL_TOK_SHL_EQ: return "<<=";
        case STEEL_TOK_SHR: return ">>";
        case STEEL_TOK_SHR_EQ: return ">>=";

        case STEEL_TOK_QMARK: return "?";

        // Keywords
        case STEEL_TOK_KW_MOD: return "mod";
        case STEEL_TOK_KW_USE: return "use";
        case STEEL_TOK_KW_EXPORT: return "export";
        case STEEL_TOK_KW_FROM: return "from";
        case STEEL_TOK_KW_AS: return "as";

        case STEEL_TOK_KW_TYPE: return "type";
        case STEEL_TOK_KW_STRUCT: return "struct";
        case STEEL_TOK_KW_UNION: return "union";
        case STEEL_TOK_KW_ENUM: return "enum";

        case STEEL_TOK_KW_FN: return "fn";
        case STEEL_TOK_KW_SCN: return "scn";
        case STEEL_TOK_KW_PROG: return "prog";
        case STEEL_TOK_KW_PROGRAM: return "program";
        case STEEL_TOK_KW_SERVICE: return "service";
        case STEEL_TOK_KW_KERNEL: return "kernel";
        case STEEL_TOK_KW_DRIVER: return "driver";
        case STEEL_TOK_KW_TOOL: return "tool";
        case STEEL_TOK_KW_PIPELINE: return "pipeline";

        case STEEL_TOK_KW_LET: return "let";
        case STEEL_TOK_KW_CONST: return "const";

        case STEEL_TOK_KW_IF: return "if";
        case STEEL_TOK_KW_ELIF: return "elif";
        case STEEL_TOK_KW_ELSE: return "else";
        case STEEL_TOK_KW_WHILE: return "while";
        case STEEL_TOK_KW_FOR: return "for";
        case STEEL_TOK_KW_MATCH: return "match";
        case STEEL_TOK_KW_BREAK: return "break";
        case STEEL_TOK_KW_CONTINUE: return "continue";
        case STEEL_TOK_KW_RETURN: return "return";

        case STEEL_TOK_KW_SET: return "set";
        case STEEL_TOK_KW_SAY: return "say";
        case STEEL_TOK_KW_DO: return "do";
        case STEEL_TOK_KW_WHEN: return "when";
        case STEEL_TOK_KW_LOOP: return "loop";
        case STEEL_TOK_KW_RET: return "ret";

        case STEEL_TOK_KW_TRUE: return "true";
        case STEEL_TOK_KW_FALSE: return "false";
        case STEEL_TOK_KW_NULL: return "null";

        default: return "(unknown)";
    }
}