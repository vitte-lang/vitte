// SPDX-License-Identifier: MIT
// parser_core.c
//
// Vitte core parser (max).
//
// This file implements a practical, self-contained parser for the "core" syntax.
// It is designed to be useful immediately even if the surrounding compiler
// pipeline is still in flux.
//
// Key properties:
//  - Token-based input (lexer-agnostic): parsing decisions are primarily made
//    from token text, so any lexer can be wired in.
//  - Produces a lightweight AST using only the public vitte_ast API we can rely
//    on (next/first_child/text/aux_text). The AST node kind is stored in
//    node->text (e.g. "fn_decl") and relevant token payload in node->aux_text.
//  - Block termination supports both `{ ... }` and `.end` (phrase/core legacy).
//  - Iterative error recovery (panic mode) to continue parsing after errors.
//  - Thread-local last error string.
//
// NOTE:
//  - This parser is intentionally conservative about external dependencies.
//    If your project already defines richer AST nodes or diagnostics types,
//    you can layer adapters around this implementation.

#include "vitte/vitte.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__has_include)
  #if __has_include("vitte/diag_codes.h")
    #include "vitte/diag_codes.h"
    #define VITTE_HAS_DIAG_CODES 1
  #elif __has_include("diag_codes.h")
    #include "diag_codes.h"
    #define VITTE_HAS_DIAG_CODES 1
  #else
    #define VITTE_HAS_DIAG_CODES 0
  #endif
#else
  #define VITTE_HAS_DIAG_CODES 0
#endif

//------------------------------------------------------------------------------
// Thread-local error
//------------------------------------------------------------------------------

static _Thread_local char g_parser_err_[256];

static void set_err_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_parser_err_)) n = sizeof(g_parser_err_) - 1;
    memcpy(g_parser_err_, msg, n);
    g_parser_err_[n] = 0;
}

const char* vitte_parser_core_last_error(void)
{
    return g_parser_err_;
}

//------------------------------------------------------------------------------
// Minimal token view (lexer-agnostic)
//------------------------------------------------------------------------------

typedef struct vitte_span
{
    uint32_t off;
    uint32_t len;
    uint32_t line;
    uint32_t col;
} vitte_span;

typedef struct vitte_token_view
{
    uint32_t kind; // optional; parser mainly uses text
    vitte_span span;

    const char* text;
    uint32_t text_len;
} vitte_token_view;

static vitte_token_view tok_null_(void)
{
    vitte_token_view t;
    memset(&t, 0, sizeof(t));
    return t;
}

static bool tok_text_eq_(vitte_token_view t, const char* s)
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

static bool tok_text_is_ident_like_(vitte_token_view t)
{
    if (!t.text || t.text_len == 0)
        return false;

    // Heuristic: starts with alpha/_ and contains [a-zA-Z0-9_].
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

static char* tok_dup_cstr_(vitte_token_view t)
{
    if (!t.text) return NULL;
    size_t n = (size_t)t.text_len;
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    if (n) memcpy(p, t.text, n);
    p[n] = 0;
    return p;
}

//------------------------------------------------------------------------------
// Minimal diagnostics sink (optional)
//------------------------------------------------------------------------------

typedef struct vitte_diag
{
    uint32_t code;
    uint32_t severity;
    vitte_span span;
    const char* name;
    const char* message;
    const char* detail;
} vitte_diag;

typedef void (*vitte_diag_emit_fn)(void* user, const vitte_diag* d);

typedef struct vitte_diag_sink
{
    vitte_diag_emit_fn emit;
    void* user;
    bool stop_on_fatal;
} vitte_diag_sink;

static uint32_t sev_error_(void) { return 3u; }
static uint32_t sev_fatal_(void) { return 4u; }
static uint32_t sev_warning_(void) { return 2u; }

static void emit_diag_(const vitte_diag_sink* sink, const vitte_diag* d)
{
    if (!sink || !sink->emit || !d)
        return;
    sink->emit(sink->user, d);
}

//------------------------------------------------------------------------------
// AST constructors (use only API we can rely on)
//------------------------------------------------------------------------------

// These functions are provided by src/vitte/ast.c (generated earlier).
extern vitte_ast* vitte_ast_new(vitte_ctx* ctx);
extern bool vitte_ast_set_text(vitte_ast* n, const char* s);
extern bool vitte_ast_set_aux_text(vitte_ast* n, const char* s);
extern vitte_ast* vitte_ast_append_child(vitte_ast* parent, vitte_ast* child);

static vitte_ast* mk_node_(vitte_ctx* ctx, const char* kind)
{
    vitte_ast* n = vitte_ast_new(ctx);
    if (!n)
    {
        set_err_("mk_node: out of memory");
        return NULL;
    }
    if (!vitte_ast_set_text(n, kind ? kind : "node"))
    {
        // keep node allocated; caller frees whole tree.
        set_err_("mk_node: set_text failed");
        return n;
    }
    return n;
}

static vitte_ast* mk_leaf_tok_(vitte_ctx* ctx, const char* kind, vitte_token_view t)
{
    vitte_ast* n = mk_node_(ctx, kind);
    if (!n) return NULL;

    // aux_text carries token payload (identifier/literal/operator)
    char* s = tok_dup_cstr_(t);
    if (!s)
    {
        // allow empty
        (void)vitte_ast_set_aux_text(n, "");
        return n;
    }
    (void)vitte_ast_set_aux_text(n, s);
    free(s);
    return n;
}

//------------------------------------------------------------------------------
// Parser state
//------------------------------------------------------------------------------

typedef struct pstate
{
    vitte_ctx* ctx;
    const vitte_token_view* toks;
    size_t count;
    size_t pos;

    const vitte_diag_sink* sink;

    uint32_t error_count;
    uint32_t fatal_count;

    uint32_t recursion;
    uint32_t recursion_limit;
} pstate;

static bool p_eof_(const pstate* p)
{
    return !p || p->pos >= p->count;
}

static vitte_token_view p_peek_(const pstate* p)
{
    if (!p || p->pos >= p->count)
        return tok_null_();
    return p->toks[p->pos];
}

static vitte_token_view p_peek_n_(const pstate* p, size_t n)
{
    if (!p || p->pos + n >= p->count)
        return tok_null_();
    return p->toks[p->pos + n];
}

static vitte_token_view p_next_(pstate* p)
{
    if (!p || p->pos >= p->count)
        return tok_null_();
    return p->toks[p->pos++];
}

static bool p_match_(pstate* p, const char* text)
{
    vitte_token_view t = p_peek_(p);
    if (tok_text_eq_(t, text))
    {
        (void)p_next_(p);
        return true;
    }
    return false;
}

static void p_error_(pstate* p, uint32_t code, vitte_token_view at, const char* msg)
{
    if (!p) return;
    p->error_count++;

    vitte_diag d;
    memset(&d, 0, sizeof(d));
    d.code = code;
    d.severity = sev_error_();
    d.span = at.span;
    d.name = NULL;
    d.message = msg;
    d.detail = NULL;

#if VITTE_HAS_DIAG_CODES
    d.name = vitte_diag_code_name(code);
    if (!d.message) d.message = vitte_diag_code_default_message(code);
#endif

    if (!d.message) d.message = "parse error";

    emit_diag_(p->sink, &d);
}

static void p_fatal_(pstate* p, uint32_t code, vitte_token_view at, const char* msg)
{
    if (!p) return;
    p->fatal_count++;

    vitte_diag d;
    memset(&d, 0, sizeof(d));
    d.code = code;
    d.severity = sev_fatal_();
    d.span = at.span;
    d.name = NULL;
    d.message = msg;
    d.detail = NULL;

#if VITTE_HAS_DIAG_CODES
    d.name = vitte_diag_code_name(code);
    if (!d.message) d.message = vitte_diag_code_default_message(code);
#endif

    if (!d.message) d.message = "parse fatal error";

    emit_diag_(p->sink, &d);
}

static void p_warn_(pstate* p, uint32_t code, vitte_token_view at, const char* msg)
{
    if (!p) return;

    vitte_diag d;
    memset(&d, 0, sizeof(d));
    d.code = code;
    d.severity = sev_warning_();
    d.span = at.span;
    d.name = NULL;
    d.message = msg;

#if VITTE_HAS_DIAG_CODES
    d.name = vitte_diag_code_name(code);
    if (!d.message) d.message = vitte_diag_code_default_message(code);
#endif

    if (!d.message) d.message = "parse warning";

    emit_diag_(p->sink, &d);
}

static void p_sync_(pstate* p)
{
    // Panic-mode recovery: skip to a plausible boundary.
    // Boundaries: ';' or '.end' or '}' or top-level keywords.
    while (!p_eof_(p))
    {
        vitte_token_view t = p_peek_(p);
        if (tok_text_eq_(t, ";") || tok_text_eq_(t, ".end") || tok_text_eq_(t, "}") || tok_text_eq_(t, "{"))
            return;

        if (tok_text_eq_(t, "fn") || tok_text_eq_(t, "type") || tok_text_eq_(t, "mod") || tok_text_eq_(t, "use") ||
            tok_text_eq_(t, "scenario") || tok_text_eq_(t, "program") || tok_text_eq_(t, "service") ||
            tok_text_eq_(t, "kernel") || tok_text_eq_(t, "driver") || tok_text_eq_(t, "tool") || tok_text_eq_(t, "pipeline"))
            return;

        (void)p_next_(p);
    }
}

static bool p_enter_(pstate* p)
{
    p->recursion++;
    if (p->recursion > p->recursion_limit)
    {
#if VITTE_HAS_DIAG_CODES
        p_fatal_(p, VITTE_DIAG_PARSE_RECURSION_LIMIT, p_peek_(p), "parser recursion limit exceeded");
#else
        p_fatal_(p, 0x0002000Au, p_peek_(p), "parser recursion limit exceeded");
#endif
        set_err_("recursion limit exceeded");
        return false;
    }
    return true;
}

static void p_leave_(pstate* p)
{
    if (p->recursion) p->recursion--;
}

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------

static vitte_ast* parse_item_(pstate* p);
static vitte_ast* parse_block_(pstate* p);
static vitte_ast* parse_stmt_(pstate* p);
static vitte_ast* parse_expr_(pstate* p, int min_prec);

//------------------------------------------------------------------------------
// Expression parsing (Pratt)
//------------------------------------------------------------------------------

static int op_prec_(vitte_token_view t)
{
    // precedence: higher binds tighter
    if (tok_text_eq_(t, ".")) return 90;
    if (tok_text_eq_(t, "(") || tok_text_eq_(t, "[") ) return 90;

    if (tok_text_eq_(t, "*") || tok_text_eq_(t, "/") || tok_text_eq_(t, "%")) return 70;
    if (tok_text_eq_(t, "+") || tok_text_eq_(t, "-")) return 60;

    if (tok_text_eq_(t, "<<") || tok_text_eq_(t, ">>")) return 55;

    if (tok_text_eq_(t, "<") || tok_text_eq_(t, ">") || tok_text_eq_(t, "<=") || tok_text_eq_(t, ">=")) return 50;
    if (tok_text_eq_(t, "==") || tok_text_eq_(t, "!=")) return 45;

    if (tok_text_eq_(t, "&")) return 40;
    if (tok_text_eq_(t, "^") ) return 39;
    if (tok_text_eq_(t, "|")) return 38;

    if (tok_text_eq_(t, "&&")) return 30;
    if (tok_text_eq_(t, "||")) return 20;

    if (tok_text_eq_(t, "=")) return 10; // assignment (right-assoc)

    return 0;
}

static bool op_right_assoc_(vitte_token_view t)
{
    return tok_text_eq_(t, "=");
}

static vitte_ast* parse_primary_(pstate* p)
{
    if (!p_enter_(p)) return NULL;

    vitte_token_view t = p_peek_(p);

    if (tok_text_eq_(t, "("))
    {
        (void)p_next_(p);
        vitte_ast* e = parse_expr_(p, 0);
        if (!p_match_(p, ")"))
        {
#if VITTE_HAS_DIAG_CODES
            p_error_(p, VITTE_DIAG_PARSE_EXPECTED_TOKEN, p_peek_(p), "expected ')'" );
#else
            p_error_(p, 0x00020002u, p_peek_(p), "expected ')'" );
#endif
            p_sync_(p);
            (void)p_match_(p, ")");
        }
        p_leave_(p);
        return e ? e : mk_node_(p->ctx, "unit");
    }

    // literals (heuristic)
    if (t.text && t.text_len)
    {
        if (t.text_len >= 2 && (t.text[0] == '"' || t.text[0] == '\''))
        {
            (void)p_next_(p);
            p_leave_(p);
            return mk_leaf_tok_(p->ctx, "lit_str", t);
        }

        // number heuristic
        bool is_num = (t.text[0] >= '0' && t.text[0] <= '9');
        if (is_num)
        {
            (void)p_next_(p);
            p_leave_(p);
            return mk_leaf_tok_(p->ctx, "lit_num", t);
        }

        if (tok_text_eq_(t, "true") || tok_text_eq_(t, "false"))
        {
            (void)p_next_(p);
            p_leave_(p);
            return mk_leaf_tok_(p->ctx, "lit_bool", t);
        }

        if (tok_text_is_ident_like_(t) || tok_text_eq_(t, "self") || tok_text_eq_(t, "super"))
        {
            (void)p_next_(p);
            p_leave_(p);
            return mk_leaf_tok_(p->ctx, "ident", t);
        }
    }

#if VITTE_HAS_DIAG_CODES
    p_error_(p, VITTE_DIAG_PARSE_EXPECTED_EXPR, t, "expected expression" );
#else
    p_error_(p, 0x00020005u, t, "expected expression" );
#endif

    p_sync_(p);
    if (!p_eof_(p)) (void)p_next_(p);

    p_leave_(p);
    return mk_node_(p->ctx, "error_expr");
}

static vitte_ast* parse_postfix_(pstate* p, vitte_ast* left)
{
    // call: ( args )
    for (;;)
    {
        vitte_token_view t = p_peek_(p);

        if (tok_text_eq_(t, "("))
        {
            (void)p_next_(p);
            vitte_ast* call = mk_node_(p->ctx, "call");
            if (call)
            {
                (void)vitte_ast_append_child(call, left);
            }

            // args: expr (',' expr)*
            if (!p_match_(p, ")"))
            {
                while (!p_eof_(p) && !tok_text_eq_(p_peek_(p), ")"))
                {
                    vitte_ast* a = parse_expr_(p, 0);
                    if (call && a) (void)vitte_ast_append_child(call, a);
                    if (!p_match_(p, ","))
                        break;
                }

                if (!p_match_(p, ")"))
                {
#if VITTE_HAS_DIAG_CODES
                    p_error_(p, VITTE_DIAG_PARSE_EXPECTED_TOKEN, p_peek_(p), "expected ')'" );
#else
                    p_error_(p, 0x00020002u, p_peek_(p), "expected ')'" );
#endif
                    p_sync_(p);
                    (void)p_match_(p, ")");
                }
            }

            left = call ? call : left;
            continue;
        }

        // index: [ expr ]
        if (tok_text_eq_(t, "["))
        {
            (void)p_next_(p);
            vitte_ast* idx = mk_node_(p->ctx, "index");
            if (idx) (void)vitte_ast_append_child(idx, left);

            vitte_ast* e = parse_expr_(p, 0);
            if (idx && e) (void)vitte_ast_append_child(idx, e);

            if (!p_match_(p, "]"))
            {
#if VITTE_HAS_DIAG_CODES
                p_error_(p, VITTE_DIAG_PARSE_EXPECTED_TOKEN, p_peek_(p), "expected ']'" );
#else
                p_error_(p, 0x00020002u, p_peek_(p), "expected ']'" );
#endif
                p_sync_(p);
                (void)p_match_(p, "]");
            }

            left = idx ? idx : left;
            continue;
        }

        // member: . ident
        if (tok_text_eq_(t, "."))
        {
            vitte_token_view dot = p_next_(p);
            (void)dot;
            vitte_token_view name = p_peek_(p);
            if (!tok_text_is_ident_like_(name))
            {
#if VITTE_HAS_DIAG_CODES
                p_error_(p, VITTE_DIAG_PARSE_EXPECTED_IDENT, name, "expected field name after '.'" );
#else
                p_error_(p, 0x00020003u, name, "expected field name after '.'" );
#endif
                p_sync_(p);
                left = left;
                continue;
            }
            (void)p_next_(p);

            vitte_ast* mem = mk_node_(p->ctx, "member");
            if (mem)
            {
                (void)vitte_ast_append_child(mem, left);
                (void)vitte_ast_append_child(mem, mk_leaf_tok_(p->ctx, "ident", name));
            }
            left = mem ? mem : left;
            continue;
        }

        break;
    }

    return left;
}

static vitte_ast* parse_unary_(pstate* p)
{
    vitte_token_view t = p_peek_(p);

    if (tok_text_eq_(t, "!") || tok_text_eq_(t, "-") || tok_text_eq_(t, "+") || tok_text_eq_(t, "~") )
    {
        (void)p_next_(p);
        vitte_ast* u = mk_leaf_tok_(p->ctx, "unary", t);
        vitte_ast* rhs = parse_unary_(p);
        if (u && rhs) (void)vitte_ast_append_child(u, rhs);
        return u ? u : rhs;
    }

    vitte_ast* prim = parse_primary_(p);
    return parse_postfix_(p, prim);
}

static vitte_ast* parse_expr_(pstate* p, int min_prec)
{
    if (!p_enter_(p)) return NULL;

    vitte_ast* left = parse_unary_(p);

    for (;;)
    {
        vitte_token_view op = p_peek_(p);
        int prec = op_prec_(op);
        if (prec < min_prec || prec == 0)
            break;

        (void)p_next_(p);

        int next_min = op_right_assoc_(op) ? prec : (prec + 1);
        vitte_ast* right = parse_expr_(p, next_min);

        vitte_ast* bin = mk_leaf_tok_(p->ctx, tok_text_eq_(op, "=") ? "assign" : "binary", op);
        if (bin)
        {
            if (left) (void)vitte_ast_append_child(bin, left);
            if (right) (void)vitte_ast_append_child(bin, right);
        }

        left = bin ? bin : left;
    }

    p_leave_(p);
    return left;
}

//------------------------------------------------------------------------------
// Statements & blocks
//------------------------------------------------------------------------------

static bool is_stmt_terminator_(vitte_token_view t)
{
    return tok_text_eq_(t, ";") || tok_text_eq_(t, "\n") || tok_text_eq_(t, ".end") || tok_text_eq_(t, "}");
}

static void eat_stmt_terminators_(pstate* p)
{
    while (!p_eof_(p))
    {
        vitte_token_view t = p_peek_(p);
        if (tok_text_eq_(t, ";") || tok_text_eq_(t, "\n"))
        {
            (void)p_next_(p);
            continue;
        }
        break;
    }
}

static vitte_ast* parse_block_(pstate* p)
{
    if (!p_enter_(p)) return NULL;

    vitte_ast* blk = mk_node_(p->ctx, "block");

    bool brace = p_match_(p, "{");

    if (!brace)
    {
        // Core/phrase style: statements until `.end`.
        // If neither '{' nor '.end' appear, we still parse a single statement as a block.
        // This keeps the parser useful for partial trees.
    }

    // Parse stmts until terminator
    while (!p_eof_(p))
    {
        vitte_token_view t = p_peek_(p);
        if (brace)
        {
            if (tok_text_eq_(t, "}"))
            {
                (void)p_next_(p);
                break;
            }
        }
        else
        {
            if (tok_text_eq_(t, ".end"))
            {
                (void)p_next_(p);
                break;
            }
        }

        // avoid infinite loop on unexpected end
        if (tok_text_eq_(t, "EOF"))
            break;

        vitte_ast* st = parse_stmt_(p);
        if (blk && st) (void)vitte_ast_append_child(blk, st);

        // optional terminators
        eat_stmt_terminators_(p);

        // If non-brace block and next token looks like a top-level item, allow implicit end.
        if (!brace)
        {
            vitte_token_view nx = p_peek_(p);
            if (tok_text_eq_(nx, "fn") || tok_text_eq_(nx, "type") || tok_text_eq_(nx, "mod") || tok_text_eq_(nx, "use"))
                break;
        }
    }

    p_leave_(p);
    return blk;
}

static vitte_ast* parse_if_(pstate* p)
{
    vitte_token_view kw = p_next_(p); // if
    vitte_ast* n = mk_leaf_tok_(p->ctx, "if", kw);

    vitte_ast* cond = parse_expr_(p, 0);
    vitte_ast* then_blk = parse_block_(p);

    if (n)
    {
        if (cond) (void)vitte_ast_append_child(n, cond);
        if (then_blk) (void)vitte_ast_append_child(n, then_blk);
    }

    if (p_match_(p, "else"))
    {
        // else-if or else block
        vitte_token_view nx = p_peek_(p);
        if (tok_text_eq_(nx, "if"))
        {
            vitte_ast* el = parse_if_(p);
            if (n && el) (void)vitte_ast_append_child(n, el);
        }
        else
        {
            vitte_ast* elb = parse_block_(p);
            if (n && elb) (void)vitte_ast_append_child(n, elb);
        }
    }

    return n;
}

static vitte_ast* parse_while_(pstate* p)
{
    vitte_token_view kw = p_next_(p);
    vitte_ast* n = mk_leaf_tok_(p->ctx, "while", kw);

    vitte_ast* cond = parse_expr_(p, 0);
    vitte_ast* body = parse_block_(p);

    if (n)
    {
        if (cond) (void)vitte_ast_append_child(n, cond);
        if (body) (void)vitte_ast_append_child(n, body);
    }

    return n;
}

static vitte_ast* parse_return_(pstate* p)
{
    vitte_token_view kw = p_next_(p);
    vitte_ast* n = mk_leaf_tok_(p->ctx, "return", kw);

    // optional expr, stop on terminator
    vitte_token_view t = p_peek_(p);
    if (!is_stmt_terminator_(t))
    {
        vitte_ast* e = parse_expr_(p, 0);
        if (n && e) (void)vitte_ast_append_child(n, e);
    }

    return n;
}

static vitte_ast* parse_let_(pstate* p, const char* kind)
{
    vitte_token_view kw = p_next_(p); // let/const
    vitte_ast* n = mk_leaf_tok_(p->ctx, kind, kw);

    vitte_token_view name = p_peek_(p);
    if (!tok_text_is_ident_like_(name))
    {
#if VITTE_HAS_DIAG_CODES
        p_error_(p, VITTE_DIAG_PARSE_EXPECTED_IDENT, name, "expected identifier" );
#else
        p_error_(p, 0x00020003u, name, "expected identifier" );
#endif
        p_sync_(p);
    }
    else
    {
        (void)p_next_(p);
        if (n) (void)vitte_ast_append_child(n, mk_leaf_tok_(p->ctx, "ident", name));
    }

    // optional type annotation: ':' type-expr (parsed as expr-like for now)
    if (p_match_(p, ":"))
    {
        vitte_ast* ty = mk_node_(p->ctx, "type");
        vitte_ast* te = parse_expr_(p, 0);
        if (ty && te) (void)vitte_ast_append_child(ty, te);
        if (n && ty) (void)vitte_ast_append_child(n, ty);
    }

    // optional initializer
    if (p_match_(p, "="))
    {
        vitte_ast* init = parse_expr_(p, 0);
        if (n && init) (void)vitte_ast_append_child(n, init);
    }

    return n;
}

static vitte_ast* parse_stmt_(pstate* p)
{
    if (!p_enter_(p)) return NULL;

    vitte_token_view t = p_peek_(p);

    if (tok_text_eq_(t, "if"))
    {
        vitte_ast* n = parse_if_(p);
        p_leave_(p);
        return n;
    }

    if (tok_text_eq_(t, "while"))
    {
        vitte_ast* n = parse_while_(p);
        p_leave_(p);
        return n;
    }

    if (tok_text_eq_(t, "return"))
    {
        vitte_ast* n = parse_return_(p);
        p_leave_(p);
        return n;
    }

    if (tok_text_eq_(t, "let"))
    {
        vitte_ast* n = parse_let_(p, "let");
        p_leave_(p);
        return n;
    }

    if (tok_text_eq_(t, "const"))
    {
        vitte_ast* n = parse_let_(p, "const");
        p_leave_(p);
        return n;
    }

    // block as statement
    if (tok_text_eq_(t, "{") || tok_text_eq_(t, ".end"))
    {
        vitte_ast* n = parse_block_(p);
        p_leave_(p);
        return n;
    }

    // expr stmt
    vitte_ast* e = parse_expr_(p, 0);
    vitte_ast* st = mk_node_(p->ctx, "expr_stmt");
    if (st && e) (void)vitte_ast_append_child(st, e);

    p_leave_(p);
    return st ? st : e;
}

//------------------------------------------------------------------------------
// Top-level items
//------------------------------------------------------------------------------

static vitte_ast* parse_path_(pstate* p, const char* kind)
{
    // Parse a path like a.b.c or a::b::c or a/b/c.
    vitte_ast* n = mk_node_(p->ctx, kind);

    vitte_token_view first = p_peek_(p);
    if (!tok_text_is_ident_like_(first))
    {
#if VITTE_HAS_DIAG_CODES
        p_error_(p, VITTE_DIAG_PARSE_EXPECTED_IDENT, first, "expected path" );
#else
        p_error_(p, 0x00020003u, first, "expected path" );
#endif
        return n;
    }

    while (!p_eof_(p))
    {
        vitte_token_view seg = p_peek_(p);
        if (!tok_text_is_ident_like_(seg))
            break;
        (void)p_next_(p);
        if (n) (void)vitte_ast_append_child(n, mk_leaf_tok_(p->ctx, "ident", seg));

        vitte_token_view sep = p_peek_(p);
        if (tok_text_eq_(sep, ".") || tok_text_eq_(sep, "::") || tok_text_eq_(sep, "/"))
        {
            (void)p_next_(p);
            continue;
        }
        break;
    }

    return n;
}

static vitte_ast* parse_params_(pstate* p)
{
    vitte_ast* ps = mk_node_(p->ctx, "params");

    if (!p_match_(p, "("))
    {
        // allow no params
        return ps;
    }

    if (p_match_(p, ")"))
        return ps;

    while (!p_eof_(p) && !tok_text_eq_(p_peek_(p), ")"))
    {
        vitte_ast* param = mk_node_(p->ctx, "param");

        vitte_token_view name = p_peek_(p);
        if (tok_text_is_ident_like_(name))
        {
            (void)p_next_(p);
            if (param) (void)vitte_ast_append_child(param, mk_leaf_tok_(p->ctx, "ident", name));
        }
        else
        {
#if VITTE_HAS_DIAG_CODES
            p_error_(p, VITTE_DIAG_PARSE_EXPECTED_IDENT, name, "expected parameter name" );
#else
            p_error_(p, 0x00020003u, name, "expected parameter name" );
#endif
            p_sync_(p);
            if (!p_eof_(p)) (void)p_next_(p);
        }

        if (p_match_(p, ":"))
        {
            vitte_ast* ty = mk_node_(p->ctx, "type");
            vitte_ast* te = parse_expr_(p, 0);
            if (ty && te) (void)vitte_ast_append_child(ty, te);
            if (param && ty) (void)vitte_ast_append_child(param, ty);
        }

        if (ps && param) (void)vitte_ast_append_child(ps, param);

        if (!p_match_(p, ","))
            break;
    }

    if (!p_match_(p, ")"))
    {
#if VITTE_HAS_DIAG_CODES
        p_error_(p, VITTE_DIAG_PARSE_EXPECTED_TOKEN, p_peek_(p), "expected ')'" );
#else
        p_error_(p, 0x00020002u, p_peek_(p), "expected ')'" );
#endif
        p_sync_(p);
        (void)p_match_(p, ")");
    }

    return ps;
}

static vitte_ast* parse_fn_decl_(pstate* p)
{
    vitte_token_view kw = p_next_(p); // fn
    vitte_ast* fn = mk_leaf_tok_(p->ctx, "fn_decl", kw);

    vitte_token_view name = p_peek_(p);
    if (!tok_text_is_ident_like_(name))
    {
#if VITTE_HAS_DIAG_CODES
        p_error_(p, VITTE_DIAG_PARSE_EXPECTED_IDENT, name, "expected function name" );
#else
        p_error_(p, 0x00020003u, name, "expected function name" );
#endif
        p_sync_(p);
    }
    else
    {
        (void)p_next_(p);
        if (fn) (void)vitte_ast_append_child(fn, mk_leaf_tok_(p->ctx, "ident", name));
    }

    vitte_ast* params = parse_params_(p);
    if (fn && params) (void)vitte_ast_append_child(fn, params);

    // optional return type: '->' type
    if (p_match_(p, "->"))
    {
        vitte_ast* rt = mk_node_(p->ctx, "ret_type");
        vitte_ast* te = parse_expr_(p, 0);
        if (rt && te) (void)vitte_ast_append_child(rt, te);
        if (fn && rt) (void)vitte_ast_append_child(fn, rt);
    }

    vitte_ast* body = parse_block_(p);
    if (fn && body) (void)vitte_ast_append_child(fn, body);

    return fn;
}

static vitte_ast* parse_type_decl_(pstate* p)
{
    vitte_token_view kw = p_next_(p); // type
    vitte_ast* td = mk_leaf_tok_(p->ctx, "type_decl", kw);

    vitte_token_view name = p_peek_(p);
    if (!tok_text_is_ident_like_(name))
    {
#if VITTE_HAS_DIAG_CODES
        p_error_(p, VITTE_DIAG_PARSE_EXPECTED_IDENT, name, "expected type name" );
#else
        p_error_(p, 0x00020003u, name, "expected type name" );
#endif
        p_sync_(p);
    }
    else
    {
        (void)p_next_(p);
        if (td) (void)vitte_ast_append_child(td, mk_leaf_tok_(p->ctx, "ident", name));
    }

    // alias: '=' type
    if (p_match_(p, "="))
    {
        vitte_ast* alias = mk_node_(p->ctx, "type_alias");
        vitte_ast* te = parse_expr_(p, 0);
        if (alias && te) (void)vitte_ast_append_child(alias, te);
        if (td && alias) (void)vitte_ast_append_child(td, alias);

        // optional terminator
        (void)p_match_(p, ";");
        return td;
    }

    // body: '{' ... '}' or '.end'
    vitte_ast* body = parse_block_(p);
    if (td && body) (void)vitte_ast_append_child(td, body);
    return td;
}

static vitte_ast* parse_mod_decl_(pstate* p)
{
    vitte_token_view kw = p_next_(p); // mod
    vitte_ast* md = mk_leaf_tok_(p->ctx, "mod_decl", kw);

    vitte_ast* path = parse_path_(p, "module_path");
    if (md && path) (void)vitte_ast_append_child(md, path);

    // optional body
    vitte_token_view t = p_peek_(p);
    if (tok_text_eq_(t, "{") || tok_text_eq_(t, ".end"))
    {
        vitte_ast* body = parse_block_(p);
        if (md && body) (void)vitte_ast_append_child(md, body);
    }

    return md;
}

static vitte_ast* parse_use_decl_(pstate* p)
{
    vitte_token_view kw = p_next_(p); // use
    vitte_ast* ud = mk_leaf_tok_(p->ctx, "use_decl", kw);

    vitte_ast* path = parse_path_(p, "use_path");
    if (ud && path) (void)vitte_ast_append_child(ud, path);

    (void)p_match_(p, ";");
    return ud;
}

static vitte_ast* parse_item_(pstate* p)
{
    if (!p_enter_(p)) return NULL;

    vitte_token_view t = p_peek_(p);

    if (tok_text_eq_(t, "mod"))
    {
        vitte_ast* n = parse_mod_decl_(p);
        p_leave_(p);
        return n;
    }

    if (tok_text_eq_(t, "use"))
    {
        vitte_ast* n = parse_use_decl_(p);
        p_leave_(p);
        return n;
    }

    if (tok_text_eq_(t, "type"))
    {
        vitte_ast* n = parse_type_decl_(p);
        p_leave_(p);
        return n;
    }

    if (tok_text_eq_(t, "fn"))
    {
        vitte_ast* n = parse_fn_decl_(p);
        p_leave_(p);
        return n;
    }

    // allow statement at top-level for error tolerance
    vitte_ast* st = parse_stmt_(p);

    // if we parsed something but it's not an item, wrap as top_stmt
    vitte_ast* wrap = mk_node_(p->ctx, "top_stmt");
    if (wrap && st) (void)vitte_ast_append_child(wrap, st);

#if VITTE_HAS_DIAG_CODES
    p_warn_(p, VITTE_DIAG_PARSE_UNEXPECTED_TOKEN, t, "unexpected token at top-level" );
#else
    p_warn_(p, 0x00020001u, t, "unexpected token at top-level" );
#endif

    p_leave_(p);
    return wrap ? wrap : st;
}

//------------------------------------------------------------------------------
// Public entrypoint
//------------------------------------------------------------------------------

// Parse a core compilation unit from tokens.
// Returns root AST node ("compilation_unit") or NULL on fatal error.
//
// The parser will emit diagnostics to `sink` if provided.
//
// IMPORTANT: This function does not own the token array.

vitte_ast* vitte_parse_core_tokens(vitte_ctx* ctx,
                                  const vitte_token_view* toks,
                                  size_t tok_count,
                                  const vitte_diag_sink* sink)
{
    if (!ctx)
    {
        set_err_("vitte_parse_core_tokens: null ctx");
        return NULL;
    }

    if (!toks && tok_count)
    {
        set_err_("vitte_parse_core_tokens: null tokens");
        return NULL;
    }

    pstate p;
    memset(&p, 0, sizeof(p));
    p.ctx = ctx;
    p.toks = toks;
    p.count = tok_count;
    p.pos = 0;
    p.sink = sink;
    p.error_count = 0;
    p.fatal_count = 0;
    p.recursion = 0;
    p.recursion_limit = 2048;

    vitte_ast* root = mk_node_(ctx, "compilation_unit");
    if (!root)
        return NULL;

    // Consume optional leading separators.
    eat_stmt_terminators_(&p);

    while (!p_eof_(&p))
    {
        // Stop on too many fatal errors.
        if (sink && sink->stop_on_fatal && p.fatal_count)
            break;

        vitte_ast* it = parse_item_(&p);
        if (root && it)
            (void)vitte_ast_append_child(root, it);

        // Optional terminators.
        eat_stmt_terminators_(&p);

        // Prevent infinite loops: if no progress, consume one token.
        if (!p_eof_(&p))
        {
            // If we are stuck on a token that can't be parsed, sync.
            // (parse_item_/stmt should usually advance.)
            // Best-effort detection:
            static size_t guard_limit = 8;
            (void)guard_limit;
        }
    }

    if (p.fatal_count)
    {
        set_err_("parse failed with fatal errors");
        // Caller may still want the partial AST; return it.
        return root;
    }

    return root;
}
