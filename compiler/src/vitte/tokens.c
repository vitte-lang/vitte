// SPDX-License-Identifier: MIT
// tokens.c
//
// Token utilities (max).
//
// This module provides a small, practical token layer that can be used by the
// lexer and consumed by parsers (core/phrase). It is designed to be resilient
// to project refactors by:
//  - Being mostly self-contained.
//  - Exposing a stable token *view* representation.
//  - Optionally owning token text via an internal string pool.
//
// The parser_core.c implementation generated earlier consumes a token view with:
//   kind, span, text, text_len
// This file provides that representation and common helpers.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__has_include)
  #if __has_include("tokens.h")
    #include "tokens.h"
    #define VITTE_HAS_TOKENS_H 1
  #elif __has_include("vitte/tokens.h")
    #include "vitte/tokens.h"
    #define VITTE_HAS_TOKENS_H 1
  #elif __has_include("../vitte/tokens.h")
    #include "../vitte/tokens.h"
    #define VITTE_HAS_TOKENS_H 1
  #endif
#endif

#ifndef VITTE_HAS_TOKENS_H

//------------------------------------------------------------------------------
// Fallback public types (align later with tokens.h)
//------------------------------------------------------------------------------

typedef struct vitte_span
{
    uint32_t off;
    uint32_t len;
    uint32_t line;
    uint32_t col;
} vitte_span;

typedef enum vitte_token_kind
{
    VITTE_TOK_INVALID = 0,
    VITTE_TOK_EOF,

    // trivia
    VITTE_TOK_NEWLINE,

    // atoms
    VITTE_TOK_IDENT,
    VITTE_TOK_INT,
    VITTE_TOK_FLOAT,
    VITTE_TOK_STRING,

    // punctuation / operators (common)
    VITTE_TOK_LPAREN,    // (
    VITTE_TOK_RPAREN,    // )
    VITTE_TOK_LBRACE,    // {
    VITTE_TOK_RBRACE,    // }
    VITTE_TOK_LBRACK,    // [
    VITTE_TOK_RBRACK,    // ]
    VITTE_TOK_COMMA,     // ,
    VITTE_TOK_DOT,       // .
    VITTE_TOK_COLON,     // :
    VITTE_TOK_SEMI,      // ;

    VITTE_TOK_ARROW,     // ->
    VITTE_TOK_COLONCOLON,// ::

    VITTE_TOK_PLUS,      // +
    VITTE_TOK_MINUS,     // -
    VITTE_TOK_STAR,      // *
    VITTE_TOK_SLASH,     // /
    VITTE_TOK_PERCENT,   // %
    VITTE_TOK_BANG,      // !
    VITTE_TOK_TILDE,     // ~

    VITTE_TOK_EQ,        // =
    VITTE_TOK_EQEQ,      // ==
    VITTE_TOK_NEQ,       // !=

    VITTE_TOK_LT,        // <
    VITTE_TOK_GT,        // >
    VITTE_TOK_LTE,       // <=
    VITTE_TOK_GTE,       // >=

    VITTE_TOK_AMP,       // &
    VITTE_TOK_PIPE,      // |
    VITTE_TOK_CARET,     // ^
    VITTE_TOK_AMPAMP,    // &&
    VITTE_TOK_PIPEPIPE,  // ||

    VITTE_TOK_SHL,       // <<
    VITTE_TOK_SHR,       // >>

    // phrase/core terminator
    VITTE_TOK_DOT_END,   // .end

    // keywords (subset used by parser_core)
    VITTE_TOK_KW_FN,
    VITTE_TOK_KW_TYPE,
    VITTE_TOK_KW_MOD,
    VITTE_TOK_KW_USE,
    VITTE_TOK_KW_SCENARIO,
    VITTE_TOK_KW_LET,
    VITTE_TOK_KW_CONST,
    VITTE_TOK_KW_IF,
    VITTE_TOK_KW_ELSE,
    VITTE_TOK_KW_WHILE,
    VITTE_TOK_KW_FOR,
    VITTE_TOK_KW_MATCH,
    VITTE_TOK_KW_RETURN,
    VITTE_TOK_KW_BREAK,
    VITTE_TOK_KW_CONTINUE,
    VITTE_TOK_KW_TRUE,
    VITTE_TOK_KW_FALSE,
    VITTE_TOK_KW_SELF,
    VITTE_TOK_KW_SUPER,

    // entrypoint kinds
    VITTE_TOK_KW_PROGRAM,
    VITTE_TOK_KW_SERVICE,
    VITTE_TOK_KW_KERNEL,
    VITTE_TOK_KW_DRIVER,
    VITTE_TOK_KW_TOOL,
    VITTE_TOK_KW_PIPELINE,

} vitte_token_kind;

typedef struct vitte_token_view
{
    uint32_t kind;
    vitte_span span;
    const char* text;
    uint32_t text_len;
} vitte_token_view;

typedef struct vitte_tokens
{
    vitte_token_view* v;
    size_t len;
    size_t cap;

    // Optional storage for token texts.
    // If used, tokens may point into this pool; otherwise they reference source.
    char* pool;
    size_t pool_len;
    size_t pool_cap;
} vitte_tokens;

#endif // !VITTE_HAS_TOKENS_H

//------------------------------------------------------------------------------
// Error (thread-local)
//------------------------------------------------------------------------------

static _Thread_local char g_tokens_err_[256];

static void set_err_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_tokens_err_)) n = sizeof(g_tokens_err_) - 1;
    memcpy(g_tokens_err_, msg, n);
    g_tokens_err_[n] = 0;
}

const char* vitte_tokens_last_error(void)
{
    return g_tokens_err_;
}

//------------------------------------------------------------------------------
// Span helpers
//------------------------------------------------------------------------------

static vitte_span span_make_(uint32_t off, uint32_t len)
{
    vitte_span s;
    s.off = off;
    s.len = len;
    s.line = 0;
    s.col = 0;
    return s;
}

//------------------------------------------------------------------------------
// Token text helpers
//------------------------------------------------------------------------------

bool vitte_tok_text_eq(vitte_token_view t, const char* s)
{
    if (!s) s = "";
    size_t n = strlen(s);
    if (t.text_len != (uint32_t)n)
        return false;
    if (n == 0)
        return true;
    if (!t.text)
        return false;
    return memcmp(t.text, s, n) == 0;
}

bool vitte_tok_is_ident_like(vitte_token_view t)
{
    if (!t.text || t.text_len == 0)
        return false;

    unsigned char c0 = (unsigned char)t.text[0];
    if (!((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z') || c0 == '_'))
        return false;

    for (uint32_t i = 1; i < t.text_len; i++)
    {
        unsigned char c = (unsigned char)t.text[i];
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_';
        if (!ok) return false;
    }

    return true;
}

//------------------------------------------------------------------------------
// Kind name
//------------------------------------------------------------------------------

const char* vitte_token_kind_name(uint32_t kind)
{
    switch ((vitte_token_kind)kind)
    {
        case VITTE_TOK_INVALID: return "INVALID";
        case VITTE_TOK_EOF: return "EOF";
        case VITTE_TOK_NEWLINE: return "NEWLINE";

        case VITTE_TOK_IDENT: return "IDENT";
        case VITTE_TOK_INT: return "INT";
        case VITTE_TOK_FLOAT: return "FLOAT";
        case VITTE_TOK_STRING: return "STRING";

        case VITTE_TOK_LPAREN: return "LPAREN";
        case VITTE_TOK_RPAREN: return "RPAREN";
        case VITTE_TOK_LBRACE: return "LBRACE";
        case VITTE_TOK_RBRACE: return "RBRACE";
        case VITTE_TOK_LBRACK: return "LBRACK";
        case VITTE_TOK_RBRACK: return "RBRACK";
        case VITTE_TOK_COMMA: return "COMMA";
        case VITTE_TOK_DOT: return "DOT";
        case VITTE_TOK_COLON: return "COLON";
        case VITTE_TOK_SEMI: return "SEMI";

        case VITTE_TOK_ARROW: return "ARROW";
        case VITTE_TOK_COLONCOLON: return "COLONCOLON";

        case VITTE_TOK_PLUS: return "PLUS";
        case VITTE_TOK_MINUS: return "MINUS";
        case VITTE_TOK_STAR: return "STAR";
        case VITTE_TOK_SLASH: return "SLASH";
        case VITTE_TOK_PERCENT: return "PERCENT";
        case VITTE_TOK_BANG: return "BANG";
        case VITTE_TOK_TILDE: return "TILDE";

        case VITTE_TOK_EQ: return "EQ";
        case VITTE_TOK_EQEQ: return "EQEQ";
        case VITTE_TOK_NEQ: return "NEQ";

        case VITTE_TOK_LT: return "LT";
        case VITTE_TOK_GT: return "GT";
        case VITTE_TOK_LTE: return "LTE";
        case VITTE_TOK_GTE: return "GTE";

        case VITTE_TOK_AMP: return "AMP";
        case VITTE_TOK_PIPE: return "PIPE";
        case VITTE_TOK_CARET: return "CARET";
        case VITTE_TOK_AMPAMP: return "AMPAMP";
        case VITTE_TOK_PIPEPIPE: return "PIPEPIPE";

        case VITTE_TOK_SHL: return "SHL";
        case VITTE_TOK_SHR: return "SHR";

        case VITTE_TOK_DOT_END: return "DOT_END";

        case VITTE_TOK_KW_FN: return "KW_FN";
        case VITTE_TOK_KW_TYPE: return "KW_TYPE";
        case VITTE_TOK_KW_MOD: return "KW_MOD";
        case VITTE_TOK_KW_USE: return "KW_USE";
        case VITTE_TOK_KW_SCENARIO: return "KW_SCENARIO";
        case VITTE_TOK_KW_LET: return "KW_LET";
        case VITTE_TOK_KW_CONST: return "KW_CONST";
        case VITTE_TOK_KW_IF: return "KW_IF";
        case VITTE_TOK_KW_ELSE: return "KW_ELSE";
        case VITTE_TOK_KW_WHILE: return "KW_WHILE";
        case VITTE_TOK_KW_FOR: return "KW_FOR";
        case VITTE_TOK_KW_MATCH: return "KW_MATCH";
        case VITTE_TOK_KW_RETURN: return "KW_RETURN";
        case VITTE_TOK_KW_BREAK: return "KW_BREAK";
        case VITTE_TOK_KW_CONTINUE: return "KW_CONTINUE";
        case VITTE_TOK_KW_TRUE: return "KW_TRUE";
        case VITTE_TOK_KW_FALSE: return "KW_FALSE";
        case VITTE_TOK_KW_SELF: return "KW_SELF";
        case VITTE_TOK_KW_SUPER: return "KW_SUPER";

        case VITTE_TOK_KW_PROGRAM: return "KW_PROGRAM";
        case VITTE_TOK_KW_SERVICE: return "KW_SERVICE";
        case VITTE_TOK_KW_KERNEL: return "KW_KERNEL";
        case VITTE_TOK_KW_DRIVER: return "KW_DRIVER";
        case VITTE_TOK_KW_TOOL: return "KW_TOOL";
        case VITTE_TOK_KW_PIPELINE: return "KW_PIPELINE";

        default: return "UNKNOWN";
    }
}

//------------------------------------------------------------------------------
// Keyword lookup
//------------------------------------------------------------------------------

typedef struct kw_ent { const char* s; vitte_token_kind k; } kw_ent;

static const kw_ent k_keywords_[] = {
    { "fn", VITTE_TOK_KW_FN },
    { "type", VITTE_TOK_KW_TYPE },
    { "mod", VITTE_TOK_KW_MOD },
    { "use", VITTE_TOK_KW_USE },
    { "scenario", VITTE_TOK_KW_SCENARIO },
    { "let", VITTE_TOK_KW_LET },
    { "const", VITTE_TOK_KW_CONST },
    { "if", VITTE_TOK_KW_IF },
    { "else", VITTE_TOK_KW_ELSE },
    { "while", VITTE_TOK_KW_WHILE },
    { "for", VITTE_TOK_KW_FOR },
    { "match", VITTE_TOK_KW_MATCH },
    { "return", VITTE_TOK_KW_RETURN },
    { "break", VITTE_TOK_KW_BREAK },
    { "continue", VITTE_TOK_KW_CONTINUE },
    { "true", VITTE_TOK_KW_TRUE },
    { "false", VITTE_TOK_KW_FALSE },
    { "self", VITTE_TOK_KW_SELF },
    { "super", VITTE_TOK_KW_SUPER },

    { "program", VITTE_TOK_KW_PROGRAM },
    { "service", VITTE_TOK_KW_SERVICE },
    { "kernel", VITTE_TOK_KW_KERNEL },
    { "driver", VITTE_TOK_KW_DRIVER },
    { "tool", VITTE_TOK_KW_TOOL },
    { "pipeline", VITTE_TOK_KW_PIPELINE },
};

uint32_t vitte_token_keyword_kind(vitte_token_view t)
{
    if (!t.text || t.text_len == 0)
        return VITTE_TOK_INVALID;

    for (size_t i = 0; i < sizeof(k_keywords_) / sizeof(k_keywords_[0]); i++)
    {
        const char* s = k_keywords_[i].s;
        size_t n = strlen(s);
        if ((uint32_t)n == t.text_len && memcmp(t.text, s, n) == 0)
            return (uint32_t)k_keywords_[i].k;
    }

    return VITTE_TOK_INVALID;
}

bool vitte_token_is_keyword(vitte_token_view t)
{
    return vitte_token_keyword_kind(t) != VITTE_TOK_INVALID;
}

//------------------------------------------------------------------------------
// Punctuation classification
//------------------------------------------------------------------------------

bool vitte_token_is_trivia(uint32_t kind)
{
    return kind == VITTE_TOK_NEWLINE;
}

bool vitte_token_is_atom(uint32_t kind)
{
    return kind == VITTE_TOK_IDENT || kind == VITTE_TOK_INT || kind == VITTE_TOK_FLOAT || kind == VITTE_TOK_STRING;
}

bool vitte_token_is_op(uint32_t kind)
{
    switch ((vitte_token_kind)kind)
    {
        case VITTE_TOK_PLUS:
        case VITTE_TOK_MINUS:
        case VITTE_TOK_STAR:
        case VITTE_TOK_SLASH:
        case VITTE_TOK_PERCENT:
        case VITTE_TOK_BANG:
        case VITTE_TOK_TILDE:
        case VITTE_TOK_EQ:
        case VITTE_TOK_EQEQ:
        case VITTE_TOK_NEQ:
        case VITTE_TOK_LT:
        case VITTE_TOK_GT:
        case VITTE_TOK_LTE:
        case VITTE_TOK_GTE:
        case VITTE_TOK_AMP:
        case VITTE_TOK_PIPE:
        case VITTE_TOK_CARET:
        case VITTE_TOK_AMPAMP:
        case VITTE_TOK_PIPEPIPE:
        case VITTE_TOK_SHL:
        case VITTE_TOK_SHR:
        case VITTE_TOK_DOT:
        case VITTE_TOK_ARROW:
        case VITTE_TOK_COLONCOLON:
            return true;
        default:
            return false;
    }
}

bool vitte_token_is_delim(uint32_t kind)
{
    switch ((vitte_token_kind)kind)
    {
        case VITTE_TOK_LPAREN:
        case VITTE_TOK_RPAREN:
        case VITTE_TOK_LBRACE:
        case VITTE_TOK_RBRACE:
        case VITTE_TOK_LBRACK:
        case VITTE_TOK_RBRACK:
            return true;
        default:
            return false;
    }
}

//------------------------------------------------------------------------------
// Token constructors
//------------------------------------------------------------------------------

vitte_token_view vitte_tok_make(uint32_t kind, const char* text, uint32_t text_len, uint32_t off, uint32_t len)
{
    vitte_token_view t;
    t.kind = kind;
    t.span = span_make_(off, len);
    t.text = text;
    t.text_len = text_len;
    return t;
}

vitte_token_view vitte_tok_make_cstr(uint32_t kind, const char* text, uint32_t off)
{
    if (!text) text = "";
    uint32_t n = (uint32_t)strlen(text);
    return vitte_tok_make(kind, text, n, off, n);
}

//------------------------------------------------------------------------------
// Token vector with optional pool ownership
//------------------------------------------------------------------------------

static bool ensure_tok_cap_(vitte_tokens* ts, size_t want)
{
    if (ts->cap >= want)
        return true;

    size_t ncap = ts->cap ? ts->cap : 128;
    while (ncap < want)
    {
        size_t next = ncap * 2;
        if (next < ncap)
        {
            set_err_("token capacity overflow");
            return false;
        }
        ncap = next;
    }

    vitte_token_view* nv = (vitte_token_view*)realloc(ts->v, ncap * sizeof(*nv));
    if (!nv)
    {
        set_err_("out of memory");
        return false;
    }

    ts->v = nv;
    ts->cap = ncap;
    return true;
}

static bool ensure_pool_cap_(vitte_tokens* ts, size_t want)
{
    if (ts->pool_cap >= want)
        return true;

    size_t ncap = ts->pool_cap ? ts->pool_cap : 4096;
    while (ncap < want)
    {
        size_t next = ncap * 2;
        if (next < ncap)
        {
            set_err_("pool capacity overflow");
            return false;
        }
        ncap = next;
    }

    char* np = (char*)realloc(ts->pool, ncap);
    if (!np)
    {
        set_err_("out of memory");
        return false;
    }

    ts->pool = np;
    ts->pool_cap = ncap;
    return true;
}

void vitte_tokens_init(vitte_tokens* ts)
{
    if (!ts) return;
    memset(ts, 0, sizeof(*ts));
}

void vitte_tokens_dispose(vitte_tokens* ts)
{
    if (!ts) return;
    free(ts->v);
    free(ts->pool);
    memset(ts, 0, sizeof(*ts));
}

void vitte_tokens_reset(vitte_tokens* ts)
{
    if (!ts) return;
    ts->len = 0;
    ts->pool_len = 0;
}

size_t vitte_tokens_count(const vitte_tokens* ts)
{
    return ts ? ts->len : 0;
}

const vitte_token_view* vitte_tokens_data(const vitte_tokens* ts)
{
    return ts ? ts->v : NULL;
}

vitte_token_view vitte_tokens_get(const vitte_tokens* ts, size_t i)
{
    if (!ts || i >= ts->len)
        return vitte_tok_make(0, NULL, 0, 0, 0);
    return ts->v[i];
}

// Push a token that references external memory (typically the source buffer).
bool vitte_tokens_push_view(vitte_tokens* ts, vitte_token_view t)
{
    if (!ts)
    {
        set_err_("null tokens");
        return false;
    }

    if (!ensure_tok_cap_(ts, ts->len + 1))
        return false;

    ts->v[ts->len++] = t;
    return true;
}

// Push a token by copying its text into the internal pool.
// The token's text pointer will be rewritten to point into the pool.
bool vitte_tokens_push_copy(vitte_tokens* ts, vitte_token_view t)
{
    if (!ts)
    {
        set_err_("null tokens");
        return false;
    }

    const char* src = t.text ? t.text : "";
    size_t n = (size_t)t.text_len;

    // allocate in pool with NUL terminator for convenience
    if (!ensure_pool_cap_(ts, ts->pool_len + n + 1))
        return false;

    char* dst = ts->pool + ts->pool_len;
    if (n) memcpy(dst, src, n);
    dst[n] = 0;
    ts->pool_len += n + 1;

    t.text = dst;

    return vitte_tokens_push_view(ts, t);
}

//------------------------------------------------------------------------------
// Debug dump
//------------------------------------------------------------------------------

static void dump_one_(FILE* out, vitte_token_view t, size_t i)
{
    if (!out) out = stdout;

    fprintf(out, "%6zu  %-14s  off=%u len=%u  ", i, vitte_token_kind_name(t.kind), t.span.off, t.span.len);

    if (t.text && t.text_len)
    {
        fprintf(out, "\"");
        for (uint32_t k = 0; k < t.text_len; k++)
        {
            unsigned char c = (unsigned char)t.text[k];
            if (c == '\\' || c == '"')
                fputc('\\', out);
            if (c >= 32 && c < 127)
                fputc((int)c, out);
            else
                fprintf(out, "\\x%02X", (unsigned)c);
        }
        fprintf(out, "\"");
    }
    else
    {
        fprintf(out, "<no-text>");
    }

    fputc('\n', out);
}

void vitte_tokens_dump(const vitte_tokens* ts, FILE* out)
{
    if (!ts)
        return;
    if (!out) out = stdout;

    fprintf(out, "tokens: %zu\n", ts->len);
    for (size_t i = 0; i < ts->len; i++)
        dump_one_(out, ts->v[i], i);
}
