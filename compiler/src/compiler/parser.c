

// SPDX-License-Identifier: MIT
// parser.c
//
// Vitte parser (core + phrase) -> AST.
//
// Scope:
//  - Pragmatic recursive-descent / precedence-climbing parser.
//  - Produces a generic AST using the compiler's AST store (steel_ast).
//  - Supports `.end` delimited blocks (preferred) and `{}` (legacy).
//
// This parser is intentionally tolerant:
//  - It recovers on errors by synchronizing on `;`, `.end`, `}`, and toplevel keywords.
//  - It relies primarily on token kinds (from lexer.h) but also accepts minimal
//    convention based parsing when kinds are unknown.
//
// Pairing expectations:
//  - parser.h forward-declares `steel_parser` and declares the functions below.
//  - lexer.h defines steel_lexer/steel_token/steel_token_kind and token fields:
//      token.kind, token.span, token.text, token.text_len
//  - ast.h declares the AST API used here:
//      steel_ast_new, steel_ast_set_name, steel_ast_set_text, steel_ast_add_kid
//

#include "parser.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lexer.h"
#include "ast.h"

//------------------------------------------------------------------------------
// AST kinds (internal)
//------------------------------------------------------------------------------

typedef enum steel_ast_kind_internal
{
    ASTK_FILE = 1,

    // toplevel
    ASTK_MOD,
    ASTK_USE,
    ASTK_EXPORT,

    ASTK_TYPE_ALIAS,
    ASTK_STRUCT,
    ASTK_UNION,
    ASTK_ENUM,

    ASTK_FN,
    ASTK_SCN,
    ASTK_ENTRY,

    // misc containers
    ASTK_PATH,
    ASTK_PARAMS,
    ASTK_PARAM,
    ASTK_FIELDS,
    ASTK_FIELD,
    ASTK_ENUM_BODY,
    ASTK_ENUM_ENTRY,

    ASTK_BLOCK,

    // stmts
    ASTK_STMT_LET,
    ASTK_STMT_CONST,
    ASTK_STMT_SET,
    ASTK_STMT_SAY,
    ASTK_STMT_DO,
    ASTK_STMT_IF,
    ASTK_STMT_ELIF,
    ASTK_STMT_ELSE,
    ASTK_STMT_WHILE,
    ASTK_STMT_FOR,
    ASTK_STMT_MATCH,
    ASTK_MATCH_ARM,
    ASTK_STMT_BREAK,
    ASTK_STMT_CONTINUE,
    ASTK_STMT_RETURN,
    ASTK_STMT_EXPR,

    // expr
    ASTK_IDENT,
    ASTK_INT,
    ASTK_FLOAT,
    ASTK_STRING,
    ASTK_CHAR,

    ASTK_UNARY,
    ASTK_BINARY,
    ASTK_CALL,
    ASTK_MEMBER,
    ASTK_INDEX,

    // patterns
    ASTK_PAT_WILDCARD,
    ASTK_PAT_IDENT,
    ASTK_PAT_LITERAL,
    ASTK_PAT_PATH,
    ASTK_PAT_TUPLE,

    ASTK_ERROR,

} steel_ast_kind_internal;

static inline steel_span steel_span_from_tok_(steel_token t)
{
    return t.span;
}

//------------------------------------------------------------------------------
// Parser state
//------------------------------------------------------------------------------

struct steel_parser
{
    steel_ast* ast;
    steel_lexer lx;

    steel_token cur;

    bool had_error;
    char last_error[256];
};

//------------------------------------------------------------------------------
// Forward decls
//------------------------------------------------------------------------------

static steel_ast_id parse_toplevel_(steel_parser* p);
static steel_ast_id parse_decl_type_(steel_parser* p, bool allow_leading_type_kw);
static steel_ast_id parse_decl_fn_like_(steel_parser* p, steel_token_kind kw_kind);
static steel_ast_id parse_entry_(steel_parser* p);

static steel_ast_id parse_block_(steel_parser* p);
static steel_ast_id parse_stmt_(steel_parser* p);

static steel_ast_id parse_expr_(steel_parser* p);
static steel_ast_id parse_expr_prec_(steel_parser* p, int min_prec);
static steel_ast_id parse_unary_(steel_parser* p);
static steel_ast_id parse_postfix_(steel_parser* p);
static steel_ast_id parse_primary_(steel_parser* p);

static steel_ast_id parse_path_(steel_parser* p);
static steel_ast_id parse_type_expr_(steel_parser* p);

static steel_ast_id parse_pattern_(steel_parser* p);

//------------------------------------------------------------------------------
// Diagnostics / recovery
//------------------------------------------------------------------------------

static void steel_parser_set_error_(steel_parser* p, const char* msg)
{
    p->had_error = true;
    if (!msg) msg = "parse error";
    snprintf(p->last_error, sizeof(p->last_error), "%s", msg);
}

static void steel_parser_advance_(steel_parser* p)
{
    p->cur = steel_lexer_next(&p->lx);
}

static bool steel_parser_match_(steel_parser* p, steel_token_kind k)
{
    if (p->cur.kind != k) return false;
    steel_parser_advance_(p);
    return true;
}

static bool steel_parser_expect_(steel_parser* p, steel_token_kind k, const char* what)
{
    if (p->cur.kind == k)
    {
        steel_parser_advance_(p);
        return true;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "expected %s", what ? what : "token");
    steel_parser_set_error_(p, buf);
    return false;
}

static bool steel_tok_is_toplevel_kw_(steel_token_kind k)
{
    switch (k)
    {
        case STEEL_TOK_KW_MOD:
        case STEEL_TOK_KW_USE:
        case STEEL_TOK_KW_EXPORT:
        case STEEL_TOK_KW_TYPE:
        case STEEL_TOK_KW_STRUCT:
        case STEEL_TOK_KW_UNION:
        case STEEL_TOK_KW_ENUM:
        case STEEL_TOK_KW_FN:
        case STEEL_TOK_KW_SCN:
        case STEEL_TOK_KW_PROG:
        case STEEL_TOK_KW_PROGRAM:
        case STEEL_TOK_KW_SERVICE:
        case STEEL_TOK_KW_KERNEL:
        case STEEL_TOK_KW_DRIVER:
        case STEEL_TOK_KW_TOOL:
        case STEEL_TOK_KW_PIPELINE:
            return true;
        default:
            return false;
    }
}

static void steel_parser_sync_(steel_parser* p)
{
    // Synchronize to a safe boundary.
    while (p->cur.kind != STEEL_TOK_EOF)
    {
        if (p->cur.kind == STEEL_TOK_SEMI || p->cur.kind == STEEL_TOK_DOT_END || p->cur.kind == STEEL_TOK_RBRACE)
        {
            steel_parser_advance_(p);
            return;
        }

        if (steel_tok_is_toplevel_kw_(p->cur.kind))
            return;

        steel_parser_advance_(p);
    }
}

//------------------------------------------------------------------------------
// AST helpers
//------------------------------------------------------------------------------

static steel_ast_id ast_new_(steel_parser* p, steel_ast_kind kind, steel_token at)
{
    return steel_ast_new(p->ast, kind, steel_span_from_tok_(at));
}

static void ast_set_text_tok_(steel_parser* p, steel_ast_id id, steel_token t)
{
    if (!id) return;
    if (t.text && t.text_len)
    {
        (void)steel_ast_set_text(p->ast, id, "");
        (void)steel_ast_set_text(p->ast, id, steel_ast_intern_n(p->ast, t.text, t.text_len));
    }
}

static void ast_set_text_cstr_(steel_parser* p, steel_ast_id id, const char* s)
{
    if (!id) return;
    (void)steel_ast_set_text(p->ast, id, s ? s : "");
}

static void ast_set_name_tok_(steel_parser* p, steel_ast_id id, steel_token t)
{
    if (!id) return;
    if (t.text && t.text_len)
    {
        (void)steel_ast_set_name(p->ast, id, "");
        (void)steel_ast_set_name(p->ast, id, steel_ast_intern_n(p->ast, t.text, t.text_len));
    }
}

static void ast_add_(steel_parser* p, steel_ast_id parent, steel_ast_id child)
{
    if (parent && child)
        (void)steel_ast_add_kid(p->ast, parent, child);
}

static steel_ast_id ast_error_(steel_parser* p, steel_token at, const char* msg)
{
    steel_parser_set_error_(p, msg);
    steel_ast_id e = ast_new_(p, ASTK_ERROR, at);
    ast_set_text_cstr_(p, e, msg);
    return e;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_parser_init(steel_parser* p, steel_ast* ast, const char* src, size_t len, uint32_t file_id)
{
    memset(p, 0, sizeof(*p));
    p->ast = ast;
    steel_lexer_init(&p->lx, src, len, file_id);
    p->cur = steel_lexer_next(&p->lx);
    p->had_error = false;
    p->last_error[0] = 0;
}

bool steel_parser_had_error(const steel_parser* p)
{
    return p ? p->had_error : true;
}

const char* steel_parser_last_error(const steel_parser* p)
{
    if (!p) return "(parser=null)";
    return p->last_error[0] ? p->last_error : "";
}

steel_ast_id steel_parser_parse_file(steel_parser* p)
{
    if (!p || !p->ast) return 0;

    steel_token at = p->cur;
    steel_ast_id root = ast_new_(p, ASTK_FILE, at);
    ast_set_text_cstr_(p, root, "file");

    while (p->cur.kind != STEEL_TOK_EOF)
    {
        steel_ast_id d = parse_toplevel_(p);
        if (d) ast_add_(p, root, d);

        // Optional semicolon between toplevel decls.
        (void)steel_parser_match_(p, STEEL_TOK_SEMI);

        // If we got stuck, recover.
        if (p->had_error)
            steel_parser_sync_(p);
    }

    return root;
}

// Convenience one-shot parse
steel_ast_id steel_parse(steel_ast* ast, const char* src, size_t len, uint32_t file_id)
{
    steel_parser p;
    steel_parser_init(&p, ast, src, len, file_id);
    return steel_parser_parse_file(&p);
}

//------------------------------------------------------------------------------
// Toplevel parsing
//------------------------------------------------------------------------------

static steel_ast_id parse_mod_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id n = ast_new_(p, ASTK_MOD, kw);
    ast_set_text_cstr_(p, n, "mod");

    steel_ast_id path = parse_path_(p);
    ast_add_(p, n, path);
    return n;
}

static steel_ast_id parse_use_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id n = ast_new_(p, ASTK_USE, kw);
    ast_set_text_cstr_(p, n, "use");

    steel_ast_id path = parse_path_(p);
    ast_add_(p, n, path);

    if (steel_parser_match_(p, STEEL_TOK_KW_AS))
    {
        if (p->cur.kind == STEEL_TOK_IDENT)
        {
            steel_token alias = p->cur;
            steel_parser_advance_(p);
            steel_ast_id a = ast_new_(p, ASTK_IDENT, alias);
            ast_set_text_tok_(p, a, alias);
            ast_add_(p, n, a);
        }
        else
        {
            ast_add_(p, n, ast_error_(p, p->cur, "expected alias identifier"));
        }
    }

    return n;
}

static steel_ast_id parse_export_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id n = ast_new_(p, ASTK_EXPORT, kw);
    ast_set_text_cstr_(p, n, "export");

    // Minimal: export [all|ident(,ident)*] (from path)?
    if (p->cur.kind == STEEL_TOK_IDENT)
    {
        while (p->cur.kind == STEEL_TOK_IDENT)
        {
            steel_token id = p->cur;
            steel_parser_advance_(p);
            steel_ast_id x = ast_new_(p, ASTK_IDENT, id);
            ast_set_text_tok_(p, x, id);
            ast_add_(p, n, x);
            if (!steel_parser_match_(p, STEEL_TOK_COMMA)) break;
        }
    }
    else if (p->cur.kind == STEEL_TOK_IDENT && p->cur.text_len == 3 && memcmp(p->cur.text, "all", 3) == 0)
    {
        // handled above anyway
    }

    if (steel_parser_match_(p, STEEL_TOK_KW_FROM))
    {
        steel_ast_id path = parse_path_(p);
        ast_add_(p, n, path);
    }

    return n;
}

static steel_ast_id parse_toplevel_(steel_parser* p)
{
    switch (p->cur.kind)
    {
        case STEEL_TOK_KW_MOD: return parse_mod_(p);
        case STEEL_TOK_KW_USE: return parse_use_(p);
        case STEEL_TOK_KW_EXPORT: return parse_export_(p);

        case STEEL_TOK_KW_TYPE: return parse_decl_type_(p, true);
        case STEEL_TOK_KW_STRUCT: return parse_decl_type_(p, false);
        case STEEL_TOK_KW_UNION: return parse_decl_type_(p, false);
        case STEEL_TOK_KW_ENUM: return parse_decl_type_(p, false);

        case STEEL_TOK_KW_FN: return parse_decl_fn_like_(p, STEEL_TOK_KW_FN);
        case STEEL_TOK_KW_SCN: return parse_decl_fn_like_(p, STEEL_TOK_KW_SCN);

        case STEEL_TOK_KW_PROG:
        case STEEL_TOK_KW_PROGRAM:
        case STEEL_TOK_KW_SERVICE:
        case STEEL_TOK_KW_KERNEL:
        case STEEL_TOK_KW_DRIVER:
        case STEEL_TOK_KW_TOOL:
        case STEEL_TOK_KW_PIPELINE:
            return parse_entry_(p);

        default:
        {
            // tolerate stray statements at toplevel
            steel_ast_id s = parse_stmt_(p);
            if (s) return s;
            return ast_error_(p, p->cur, "unexpected token at toplevel");
        }
    }
}

//------------------------------------------------------------------------------
// Paths / type expressions
//------------------------------------------------------------------------------

static bool tok_is_path_sep_(steel_token_kind k)
{
    return k == STEEL_TOK_DOT || k == STEEL_TOK_COLON_COLON || k == STEEL_TOK_SLASH;
}

static steel_ast_id parse_path_(steel_parser* p)
{
    steel_token at = p->cur;
    steel_ast_id n = ast_new_(p, ASTK_PATH, at);
    ast_set_text_cstr_(p, n, "path");

    if (p->cur.kind != STEEL_TOK_IDENT)
    {
        ast_add_(p, n, ast_error_(p, p->cur, "expected path identifier"));
        return n;
    }

    while (true)
    {
        if (p->cur.kind != STEEL_TOK_IDENT) break;
        steel_token seg = p->cur;
        steel_parser_advance_(p);

        steel_ast_id s = ast_new_(p, ASTK_IDENT, seg);
        ast_set_text_tok_(p, s, seg);
        ast_add_(p, n, s);

        if (!tok_is_path_sep_(p->cur.kind)) break;
        steel_parser_advance_(p);
    }

    return n;
}

static steel_ast_id parse_type_expr_(steel_parser* p)
{
    // For now, reuse path parsing (TypeName + generics later)
    return parse_path_(p);
}

//------------------------------------------------------------------------------
// Type declarations
//------------------------------------------------------------------------------

static steel_ast_id parse_fields_block_(steel_parser* p)
{
    steel_token at = p->cur;
    steel_ast_id fb = ast_new_(p, ASTK_FIELDS, at);
    ast_set_text_cstr_(p, fb, "fields");

    // Parse until `.end` or `}`
    while (p->cur.kind != STEEL_TOK_EOF && p->cur.kind != STEEL_TOK_DOT_END && p->cur.kind != STEEL_TOK_RBRACE)
    {
        if (p->cur.kind != STEEL_TOK_IDENT)
        {
            ast_add_(p, fb, ast_error_(p, p->cur, "expected field name"));
            steel_parser_sync_(p);
            continue;
        }

        steel_token name = p->cur;
        steel_parser_advance_(p);

        steel_ast_id field = ast_new_(p, ASTK_FIELD, name);
        ast_set_name_tok_(p, field, name);

        (void)steel_parser_expect_(p, STEEL_TOK_COLON, "':'");
        steel_ast_id ty = parse_type_expr_(p);
        ast_add_(p, field, ty);

        // Optional trailing comma/semicolon
        (void)steel_parser_match_(p, STEEL_TOK_COMMA);
        (void)steel_parser_match_(p, STEEL_TOK_SEMI);

        ast_add_(p, fb, field);
    }

    // Consume terminator if present
    if (p->cur.kind == STEEL_TOK_DOT_END) steel_parser_advance_(p);
    else if (p->cur.kind == STEEL_TOK_RBRACE) steel_parser_advance_(p);

    return fb;
}

static steel_ast_id parse_enum_body_(steel_parser* p)
{
    steel_token at = p->cur;
    steel_ast_id eb = ast_new_(p, ASTK_ENUM_BODY, at);
    ast_set_text_cstr_(p, eb, "enum_body");

    while (p->cur.kind != STEEL_TOK_EOF && p->cur.kind != STEEL_TOK_DOT_END && p->cur.kind != STEEL_TOK_RBRACE)
    {
        if (p->cur.kind != STEEL_TOK_IDENT)
        {
            ast_add_(p, eb, ast_error_(p, p->cur, "expected enum entry"));
            steel_parser_sync_(p);
            continue;
        }

        steel_token name = p->cur;
        steel_parser_advance_(p);

        steel_ast_id en = ast_new_(p, ASTK_ENUM_ENTRY, name);
        ast_set_name_tok_(p, en, name);

        // Optional payload: ( TypeExpr )
        if (steel_parser_match_(p, STEEL_TOK_LPAREN))
        {
            steel_ast_id ty = parse_type_expr_(p);
            ast_add_(p, en, ty);
            (void)steel_parser_expect_(p, STEEL_TOK_RPAREN, "')'");
        }

        (void)steel_parser_match_(p, STEEL_TOK_COMMA);
        (void)steel_parser_match_(p, STEEL_TOK_SEMI);

        ast_add_(p, eb, en);
    }

    if (p->cur.kind == STEEL_TOK_DOT_END) steel_parser_advance_(p);
    else if (p->cur.kind == STEEL_TOK_RBRACE) steel_parser_advance_(p);

    return eb;
}

static steel_ast_id parse_decl_type_(steel_parser* p, bool allow_leading_type_kw)
{
    steel_token at = p->cur;

    // Forms:
    //  - type Name = TypeExpr
    //  - type Name struct ... .end
    //  - struct Name ... .end

    if (allow_leading_type_kw)
        steel_parser_advance_(p); // consume 'type'

    // If we are at struct/union/enum directly (no 'type'), capture kind.
    steel_token_kind k = at.kind;
    if (!allow_leading_type_kw)
    {
        k = p->cur.kind;
        at = p->cur;
        steel_parser_advance_(p);
    }

    if (p->cur.kind != STEEL_TOK_IDENT)
        return ast_error_(p, p->cur, "expected type name");

    steel_token name = p->cur;
    steel_parser_advance_(p);

    // Decide whether this is alias or struct/union/enum.
    if (allow_leading_type_kw)
    {
        if (p->cur.kind == STEEL_TOK_EQ)
        {
            steel_parser_advance_(p);
            steel_ast_id n = ast_new_(p, ASTK_TYPE_ALIAS, at);
            ast_set_name_tok_(p, n, name);
            steel_ast_id rhs = parse_type_expr_(p);
            ast_add_(p, n, rhs);
            return n;
        }

        // If next keyword describes a body, use that.
        if (p->cur.kind == STEEL_TOK_KW_STRUCT || p->cur.kind == STEEL_TOK_KW_UNION || p->cur.kind == STEEL_TOK_KW_ENUM)
        {
            k = p->cur.kind;
            at = p->cur;
            steel_parser_advance_(p);
        }
        else
        {
            // default alias to path (type Name OtherType)
            steel_ast_id n = ast_new_(p, ASTK_TYPE_ALIAS, at);
            ast_set_name_tok_(p, n, name);
            steel_ast_id rhs = parse_type_expr_(p);
            ast_add_(p, n, rhs);
            return n;
        }
    }

    if (k == STEEL_TOK_KW_STRUCT)
    {
        steel_ast_id n = ast_new_(p, ASTK_STRUCT, at);
        ast_set_name_tok_(p, n, name);
        // body: fields until .end
        steel_ast_id body;
        if (steel_parser_match_(p, STEEL_TOK_LBRACE))
            body = parse_fields_block_(p);
        else
            body = parse_fields_block_(p);
        ast_add_(p, n, body);
        return n;
    }

    if (k == STEEL_TOK_KW_UNION)
    {
        steel_ast_id n = ast_new_(p, ASTK_UNION, at);
        ast_set_name_tok_(p, n, name);
        steel_ast_id body;
        if (steel_parser_match_(p, STEEL_TOK_LBRACE))
            body = parse_fields_block_(p);
        else
            body = parse_fields_block_(p);
        ast_add_(p, n, body);
        return n;
    }

    if (k == STEEL_TOK_KW_ENUM)
    {
        steel_ast_id n = ast_new_(p, ASTK_ENUM, at);
        ast_set_name_tok_(p, n, name);
        steel_ast_id body;
        if (steel_parser_match_(p, STEEL_TOK_LBRACE))
            body = parse_enum_body_(p);
        else
            body = parse_enum_body_(p);
        ast_add_(p, n, body);
        return n;
    }

    return ast_error_(p, at, "unknown type declaration form");
}

//------------------------------------------------------------------------------
// Function / scenario / entry parsing
//------------------------------------------------------------------------------

static steel_ast_id parse_params_(steel_parser* p)
{
    steel_token at = p->cur;
    steel_ast_id ps = ast_new_(p, ASTK_PARAMS, at);
    ast_set_text_cstr_(p, ps, "params");

    if (!steel_parser_expect_(p, STEEL_TOK_LPAREN, "'('") )
        return ps;

    if (p->cur.kind != STEEL_TOK_RPAREN)
    {
        while (true)
        {
            steel_token p_at = p->cur;
            steel_ast_id param = ast_new_(p, ASTK_PARAM, p_at);

            if (p->cur.kind != STEEL_TOK_IDENT)
            {
                ast_add_(p, param, ast_error_(p, p->cur, "expected parameter name"));
            }
            else
            {
                steel_token nm = p->cur;
                steel_parser_advance_(p);
                ast_set_name_tok_(p, param, nm);

                if (steel_parser_match_(p, STEEL_TOK_COLON))
                {
                    steel_ast_id ty = parse_type_expr_(p);
                    ast_add_(p, param, ty);
                }
            }

            ast_add_(p, ps, param);

            if (!steel_parser_match_(p, STEEL_TOK_COMMA))
                break;
        }
    }

    (void)steel_parser_expect_(p, STEEL_TOK_RPAREN, "')'");
    return ps;
}

static steel_ast_id parse_decl_fn_like_(steel_parser* p, steel_token_kind kw_kind)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_kind k = (kw_kind == STEEL_TOK_KW_SCN) ? ASTK_SCN : ASTK_FN;
    steel_ast_id fn = ast_new_(p, k, kw);
    ast_set_text_cstr_(p, fn, (k == ASTK_SCN) ? "scn" : "fn");

    if (p->cur.kind != STEEL_TOK_IDENT)
    {
        ast_add_(p, fn, ast_error_(p, p->cur, "expected function name"));
    }
    else
    {
        steel_token name = p->cur;
        steel_parser_advance_(p);
        ast_set_name_tok_(p, fn, name);
    }

    steel_ast_id params = parse_params_(p);
    ast_add_(p, fn, params);

    // Optional return type
    if (steel_parser_match_(p, STEEL_TOK_ARROW))
    {
        steel_ast_id ret = parse_type_expr_(p);
        ast_add_(p, fn, ret);
    }

    steel_ast_id body = parse_block_(p);
    ast_add_(p, fn, body);

    return fn;
}

static steel_ast_id parse_entry_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id e = ast_new_(p, ASTK_ENTRY, kw);
    ast_set_text_tok_(p, e, kw);

    // entrypoint kind name/path
    if (p->cur.kind == STEEL_TOK_IDENT)
    {
        steel_ast_id path = parse_path_(p);
        ast_add_(p, e, path);
    }

    steel_ast_id body = parse_block_(p);
    ast_add_(p, e, body);

    return e;
}

//------------------------------------------------------------------------------
// Block parsing
//------------------------------------------------------------------------------

static bool tok_starts_stmt_(steel_token_kind k)
{
    switch (k)
    {
        case STEEL_TOK_KW_LET:
        case STEEL_TOK_KW_CONST:
        case STEEL_TOK_KW_SET:
        case STEEL_TOK_KW_SAY:
        case STEEL_TOK_KW_DO:
        case STEEL_TOK_KW_IF:
        case STEEL_TOK_KW_WHILE:
        case STEEL_TOK_KW_FOR:
        case STEEL_TOK_KW_MATCH:
        case STEEL_TOK_KW_BREAK:
        case STEEL_TOK_KW_CONTINUE:
        case STEEL_TOK_KW_RETURN:
        case STEEL_TOK_KW_RET:
            return true;
        default:
            return false;
    }
}

static steel_ast_id parse_block_(steel_parser* p)
{
    steel_token at = p->cur;
    steel_ast_id b = ast_new_(p, ASTK_BLOCK, at);
    ast_set_text_cstr_(p, b, "block");

    bool brace = false;
    if (steel_parser_match_(p, STEEL_TOK_LBRACE))
        brace = true;

    while (p->cur.kind != STEEL_TOK_EOF)
    {
        if (brace)
        {
            if (p->cur.kind == STEEL_TOK_RBRACE)
            {
                steel_parser_advance_(p);
                break;
            }
        }
        else
        {
            if (p->cur.kind == STEEL_TOK_DOT_END)
            {
                steel_parser_advance_(p);
                break;
            }
        }

        // Allow stray semicolons
        if (steel_parser_match_(p, STEEL_TOK_SEMI))
            continue;

        steel_ast_id s = parse_stmt_(p);
        if (s) ast_add_(p, b, s);

        // Statement separator best-effort
        (void)steel_parser_match_(p, STEEL_TOK_SEMI);

        if (p->had_error)
            steel_parser_sync_(p);
    }

    return b;
}

//------------------------------------------------------------------------------
// Statement parsing
//------------------------------------------------------------------------------

static steel_ast_id parse_stmt_let_like_(steel_parser* p, bool is_const)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id s = ast_new_(p, is_const ? ASTK_STMT_CONST : ASTK_STMT_LET, kw);
    ast_set_text_tok_(p, s, kw);

    if (p->cur.kind != STEEL_TOK_IDENT)
    {
        ast_add_(p, s, ast_error_(p, p->cur, "expected identifier"));
        return s;
    }

    steel_token name = p->cur;
    steel_parser_advance_(p);

    steel_ast_id id = ast_new_(p, ASTK_IDENT, name);
    ast_set_text_tok_(p, id, name);
    ast_add_(p, s, id);

    // optional type annotation
    if (steel_parser_match_(p, STEEL_TOK_COLON))
    {
        steel_ast_id ty = parse_type_expr_(p);
        ast_add_(p, s, ty);
    }

    // optional initializer
    if (steel_parser_match_(p, STEEL_TOK_EQ))
    {
        steel_ast_id v = parse_expr_(p);
        ast_add_(p, s, v);
    }

    return s;
}

static steel_ast_id parse_stmt_set_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id s = ast_new_(p, ASTK_STMT_SET, kw);
    ast_set_text_tok_(p, s, kw);

    // lhs
    steel_ast_id lhs = parse_expr_(p);
    ast_add_(p, s, lhs);

    if (steel_parser_match_(p, STEEL_TOK_EQ))
    {
        steel_ast_id rhs = parse_expr_(p);
        ast_add_(p, s, rhs);
    }
    else
    {
        // phrase shorthand: `set x expr`
        steel_ast_id rhs = parse_expr_(p);
        ast_add_(p, s, rhs);
    }

    return s;
}

static steel_ast_id parse_stmt_say_do_(steel_parser* p, bool is_say)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id s = ast_new_(p, is_say ? ASTK_STMT_SAY : ASTK_STMT_DO, kw);
    ast_set_text_tok_(p, s, kw);

    steel_ast_id e = parse_expr_(p);
    ast_add_(p, s, e);
    return s;
}

static steel_ast_id parse_stmt_return_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id s = ast_new_(p, ASTK_STMT_RETURN, kw);
    ast_set_text_tok_(p, s, kw);

    // optional expr if next token doesn't look like end-of-stmt
    if (p->cur.kind != STEEL_TOK_SEMI && p->cur.kind != STEEL_TOK_DOT_END && p->cur.kind != STEEL_TOK_RBRACE)
    {
        if (!tok_starts_stmt_(p->cur.kind))
        {
            steel_ast_id e = parse_expr_(p);
            ast_add_(p, s, e);
        }
    }

    return s;
}

static steel_ast_id parse_stmt_if_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id s = ast_new_(p, ASTK_STMT_IF, kw);
    ast_set_text_tok_(p, s, kw);

    steel_ast_id cond = parse_expr_(p);
    ast_add_(p, s, cond);

    steel_ast_id then_b = parse_block_(p);
    ast_add_(p, s, then_b);

    while (p->cur.kind == STEEL_TOK_KW_ELIF)
    {
        steel_token ek = p->cur;
        steel_parser_advance_(p);
        steel_ast_id elif = ast_new_(p, ASTK_STMT_ELIF, ek);
        ast_set_text_tok_(p, elif, ek);

        steel_ast_id ec = parse_expr_(p);
        ast_add_(p, elif, ec);
        steel_ast_id eb = parse_block_(p);
        ast_add_(p, elif, eb);

        ast_add_(p, s, elif);
    }

    if (p->cur.kind == STEEL_TOK_KW_ELSE)
    {
        steel_token ek = p->cur;
        steel_parser_advance_(p);
        steel_ast_id els = ast_new_(p, ASTK_STMT_ELSE, ek);
        ast_set_text_tok_(p, els, ek);

        steel_ast_id eb = parse_block_(p);
        ast_add_(p, els, eb);
        ast_add_(p, s, els);
    }

    return s;
}

static steel_ast_id parse_stmt_while_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id s = ast_new_(p, ASTK_STMT_WHILE, kw);
    ast_set_text_tok_(p, s, kw);

    steel_ast_id cond = parse_expr_(p);
    ast_add_(p, s, cond);

    steel_ast_id body = parse_block_(p);
    ast_add_(p, s, body);

    return s;
}

static steel_ast_id parse_stmt_for_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id s = ast_new_(p, ASTK_STMT_FOR, kw);
    ast_set_text_tok_(p, s, kw);

    // Minimal: `for ident in expr block` (in is lexed as ident unless keyword exists)
    if (p->cur.kind == STEEL_TOK_IDENT)
    {
        steel_token it = p->cur;
        steel_parser_advance_(p);
        steel_ast_id id = ast_new_(p, ASTK_IDENT, it);
        ast_set_text_tok_(p, id, it);
        ast_add_(p, s, id);
    }

    // optional 'in'
    if (p->cur.kind == STEEL_TOK_IDENT && p->cur.text_len == 2 && memcmp(p->cur.text, "in", 2) == 0)
        steel_parser_advance_(p);

    steel_ast_id it_expr = parse_expr_(p);
    ast_add_(p, s, it_expr);

    steel_ast_id body = parse_block_(p);
    ast_add_(p, s, body);

    return s;
}

static steel_ast_id parse_stmt_match_(steel_parser* p)
{
    steel_token kw = p->cur;
    steel_parser_advance_(p);

    steel_ast_id s = ast_new_(p, ASTK_STMT_MATCH, kw);
    ast_set_text_tok_(p, s, kw);

    steel_ast_id scrut = parse_expr_(p);
    ast_add_(p, s, scrut);

    bool brace = false;
    if (steel_parser_match_(p, STEEL_TOK_LBRACE)) brace = true;

    // arms: pattern => block
    while (p->cur.kind != STEEL_TOK_EOF)
    {
        if ((!brace && p->cur.kind == STEEL_TOK_DOT_END) || (brace && p->cur.kind == STEEL_TOK_RBRACE))
        {
            steel_parser_advance_(p);
            break;
        }

        steel_ast_id arm = ast_new_(p, ASTK_MATCH_ARM, p->cur);
        ast_set_text_cstr_(p, arm, "arm");

        steel_ast_id pat = parse_pattern_(p);
        ast_add_(p, arm, pat);

        (void)steel_parser_expect_(p, STEEL_TOK_FATARROW, "'=>'");

        steel_ast_id body = parse_block_(p);
        ast_add_(p, arm, body);

        ast_add_(p, s, arm);

        (void)steel_parser_match_(p, STEEL_TOK_COMMA);
        (void)steel_parser_match_(p, STEEL_TOK_SEMI);

        if (p->had_error)
            steel_parser_sync_(p);
    }

    return s;
}

static steel_ast_id parse_stmt_(steel_parser* p)
{
    switch (p->cur.kind)
    {
        case STEEL_TOK_KW_LET: return parse_stmt_let_like_(p, false);
        case STEEL_TOK_KW_CONST: return parse_stmt_let_like_(p, true);

        case STEEL_TOK_KW_SET: return parse_stmt_set_(p);
        case STEEL_TOK_KW_SAY: return parse_stmt_say_do_(p, true);
        case STEEL_TOK_KW_DO: return parse_stmt_say_do_(p, false);

        case STEEL_TOK_KW_IF: return parse_stmt_if_(p);
        case STEEL_TOK_KW_WHILE: return parse_stmt_while_(p);
        case STEEL_TOK_KW_FOR: return parse_stmt_for_(p);
        case STEEL_TOK_KW_MATCH: return parse_stmt_match_(p);

        case STEEL_TOK_KW_BREAK:
        {
            steel_token kw = p->cur;
            steel_parser_advance_(p);
            steel_ast_id s = ast_new_(p, ASTK_STMT_BREAK, kw);
            ast_set_text_tok_(p, s, kw);
            return s;
        }

        case STEEL_TOK_KW_CONTINUE:
        {
            steel_token kw = p->cur;
            steel_parser_advance_(p);
            steel_ast_id s = ast_new_(p, ASTK_STMT_CONTINUE, kw);
            ast_set_text_tok_(p, s, kw);
            return s;
        }

        case STEEL_TOK_KW_RETURN:
        case STEEL_TOK_KW_RET:
            return parse_stmt_return_(p);

        default:
        {
            steel_token at = p->cur;
            steel_ast_id s = ast_new_(p, ASTK_STMT_EXPR, at);
            ast_set_text_cstr_(p, s, "expr_stmt");
            steel_ast_id e = parse_expr_(p);
            ast_add_(p, s, e);
            return s;
        }
    }
}

//------------------------------------------------------------------------------
// Expression parsing (precedence climbing)
//------------------------------------------------------------------------------

static int tok_prec_(steel_token_kind k)
{
    switch (k)
    {
        case STEEL_TOK_PIPE_PIPE: return 1;
        case STEEL_TOK_AMP_AMP: return 2;

        case STEEL_TOK_EQ_EQ:
        case STEEL_TOK_BANG_EQ:
            return 3;

        case STEEL_TOK_LT:
        case STEEL_TOK_LT_EQ:
        case STEEL_TOK_GT:
        case STEEL_TOK_GT_EQ:
            return 4;

        case STEEL_TOK_PLUS:
        case STEEL_TOK_MINUS:
            return 5;

        case STEEL_TOK_STAR:
        case STEEL_TOK_SLASH:
        case STEEL_TOK_PERCENT:
            return 6;

        default:
            return 0;
    }
}

static bool tok_is_binop_(steel_token_kind k)
{
    return tok_prec_(k) != 0;
}

static const char* tok_text_short_(steel_token t)
{
    // Safe debug name; token may not have NUL terminated text.
    static char buf[64];
    size_t n = t.text_len;
    if (!t.text || n == 0) return "";
    if (n >= sizeof(buf)) n = sizeof(buf) - 1;
    memcpy(buf, t.text, n);
    buf[n] = 0;
    return buf;
}

static steel_ast_id parse_expr_(steel_parser* p)
{
    return parse_expr_prec_(p, 1);
}

static steel_ast_id parse_expr_prec_(steel_parser* p, int min_prec)
{
    steel_ast_id lhs = parse_unary_(p);

    while (tok_is_binop_(p->cur.kind) && tok_prec_(p->cur.kind) >= min_prec)
    {
        steel_token op = p->cur;
        int prec = tok_prec_(op.kind);
        steel_parser_advance_(p);

        // Left associative: rhs with prec+1
        steel_ast_id rhs = parse_expr_prec_(p, prec + 1);

        steel_ast_id bin = ast_new_(p, ASTK_BINARY, op);
        ast_set_text_cstr_(p, bin, tok_text_short_(op));
        ast_add_(p, bin, lhs);
        ast_add_(p, bin, rhs);

        lhs = bin;
    }

    return lhs;
}

static steel_ast_id parse_unary_(steel_parser* p)
{
    if (p->cur.kind == STEEL_TOK_BANG || p->cur.kind == STEEL_TOK_MINUS)
    {
        steel_token op = p->cur;
        steel_parser_advance_(p);
        steel_ast_id x = parse_unary_(p);

        steel_ast_id un = ast_new_(p, ASTK_UNARY, op);
        ast_set_text_cstr_(p, un, tok_text_short_(op));
        ast_add_(p, un, x);
        return un;
    }

    return parse_postfix_(p);
}

static steel_ast_id parse_postfix_(steel_parser* p)
{
    steel_ast_id e = parse_primary_(p);

    for (;;)
    {
        if (p->cur.kind == STEEL_TOK_LPAREN)
        {
            steel_token at = p->cur;
            steel_ast_id call = ast_new_(p, ASTK_CALL, at);
            ast_set_text_cstr_(p, call, "call");

            ast_add_(p, call, e);

            (void)steel_parser_expect_(p, STEEL_TOK_LPAREN, "'('");
            if (p->cur.kind != STEEL_TOK_RPAREN)
            {
                while (true)
                {
                    steel_ast_id arg = parse_expr_(p);
                    ast_add_(p, call, arg);
                    if (!steel_parser_match_(p, STEEL_TOK_COMMA)) break;
                }
            }
            (void)steel_parser_expect_(p, STEEL_TOK_RPAREN, "')'");

            e = call;
            continue;
        }

        if (p->cur.kind == STEEL_TOK_LBRACK)
        {
            steel_token at = p->cur;
            steel_parser_advance_(p);

            steel_ast_id idx = ast_new_(p, ASTK_INDEX, at);
            ast_set_text_cstr_(p, idx, "index");
            ast_add_(p, idx, e);

            steel_ast_id ix = parse_expr_(p);
            ast_add_(p, idx, ix);
            (void)steel_parser_expect_(p, STEEL_TOK_RBRACK, "']'");

            e = idx;
            continue;
        }

        if (p->cur.kind == STEEL_TOK_DOT)
        {
            steel_token at = p->cur;
            steel_parser_advance_(p);

            steel_ast_id mem = ast_new_(p, ASTK_MEMBER, at);
            ast_set_text_cstr_(p, mem, ".");
            ast_add_(p, mem, e);

            if (p->cur.kind == STEEL_TOK_IDENT)
            {
                steel_token nm = p->cur;
                steel_parser_advance_(p);
                steel_ast_id id = ast_new_(p, ASTK_IDENT, nm);
                ast_set_text_tok_(p, id, nm);
                ast_add_(p, mem, id);
            }
            else
            {
                ast_add_(p, mem, ast_error_(p, p->cur, "expected member name"));
            }

            e = mem;
            continue;
        }

        break;
    }

    return e;
}

static steel_ast_id parse_primary_(steel_parser* p)
{
    steel_token t = p->cur;

    switch (t.kind)
    {
        case STEEL_TOK_IDENT:
        {
            steel_parser_advance_(p);
            steel_ast_id id = ast_new_(p, ASTK_IDENT, t);
            ast_set_text_tok_(p, id, t);
            return id;
        }

        case STEEL_TOK_INT:
        {
            steel_parser_advance_(p);
            steel_ast_id n = ast_new_(p, ASTK_INT, t);
            ast_set_text_tok_(p, n, t);
            return n;
        }

        case STEEL_TOK_FLOAT:
        {
            steel_parser_advance_(p);
            steel_ast_id n = ast_new_(p, ASTK_FLOAT, t);
            ast_set_text_tok_(p, n, t);
            return n;
        }

        case STEEL_TOK_STRING:
        {
            steel_parser_advance_(p);
            steel_ast_id n = ast_new_(p, ASTK_STRING, t);
            ast_set_text_tok_(p, n, t);
            return n;
        }

        case STEEL_TOK_CHAR:
        {
            steel_parser_advance_(p);
            steel_ast_id n = ast_new_(p, ASTK_CHAR, t);
            ast_set_text_tok_(p, n, t);
            return n;
        }

        case STEEL_TOK_KW_TRUE:
        case STEEL_TOK_KW_FALSE:
        case STEEL_TOK_KW_NULL:
        {
            steel_parser_advance_(p);
            steel_ast_id n = ast_new_(p, ASTK_IDENT, t);
            ast_set_text_tok_(p, n, t);
            return n;
        }

        case STEEL_TOK_LPAREN:
        {
            steel_parser_advance_(p);
            steel_ast_id e = parse_expr_(p);
            (void)steel_parser_expect_(p, STEEL_TOK_RPAREN, "')'");
            return e;
        }

        default:
            steel_parser_set_error_(p, "expected expression");
            steel_parser_advance_(p);
            return ast_error_(p, t, "expected expression");
    }
}

//------------------------------------------------------------------------------
// Pattern parsing (match)
//------------------------------------------------------------------------------

static steel_ast_id parse_pattern_(steel_parser* p)
{
    steel_token t = p->cur;

    // wildcard `_`
    if (t.kind == STEEL_TOK_IDENT && t.text_len == 1 && t.text && t.text[0] == '_')
    {
        steel_parser_advance_(p);
        steel_ast_id n = ast_new_(p, ASTK_PAT_WILDCARD, t);
        ast_set_text_tok_(p, n, t);
        return n;
    }

    // tuple pattern
    if (t.kind == STEEL_TOK_LPAREN)
    {
        steel_parser_advance_(p);
        steel_ast_id n = ast_new_(p, ASTK_PAT_TUPLE, t);
        ast_set_text_cstr_(p, n, "tuple");

        if (p->cur.kind != STEEL_TOK_RPAREN)
        {
            while (true)
            {
                steel_ast_id pe = parse_pattern_(p);
                ast_add_(p, n, pe);
                if (!steel_parser_match_(p, STEEL_TOK_COMMA)) break;
            }
        }

        (void)steel_parser_expect_(p, STEEL_TOK_RPAREN, "')'");
        return n;
    }

    // literal patterns: int/float/string/char
    if (t.kind == STEEL_TOK_INT || t.kind == STEEL_TOK_FLOAT || t.kind == STEEL_TOK_STRING || t.kind == STEEL_TOK_CHAR)
    {
        steel_parser_advance_(p);
        steel_ast_id n = ast_new_(p, ASTK_PAT_LITERAL, t);
        ast_set_text_tok_(p, n, t);
        return n;
    }

    // path pattern: Ident (:: Ident)*
    if (t.kind == STEEL_TOK_IDENT)
    {
        steel_ast_id path = parse_path_(p);
        // if multi-segment treat as path pattern
        steel_ast_id n = ast_new_(p, ASTK_PAT_PATH, t);
        ast_set_text_cstr_(p, n, "pat_path");
        ast_add_(p, n, path);
        return n;
    }

    steel_parser_set_error_(p, "expected pattern");
    steel_parser_advance_(p);
    return ast_error_(p, t, "expected pattern");
}