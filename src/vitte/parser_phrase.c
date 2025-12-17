#include "vitte/parser_phrase.h"
#include "vitte/lexer.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    vitte_ctx* ctx;
    vitte_token* tokens;
    size_t count;
    size_t pos;
    vitte_error* err;
} vitte_parser;

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} str_builder;

static void set_err(vitte_error* err, vitte_error_code code, uint32_t line, uint32_t col, const char* msg) {
    if (!err) return;
    err->code = code;
    err->line = line;
    err->col = col;
    if (msg) {
        strncpy(err->message, msg, sizeof(err->message) - 1);
        err->message[sizeof(err->message) - 1] = '\0';
    }
}

static const vitte_token* parser_peek(const vitte_parser* p) {
    if (p->pos >= p->count) return &p->tokens[p->count - 1];
    return &p->tokens[p->pos];
}

static const vitte_token* parser_prev(const vitte_parser* p) {
    if (p->pos == 0) return NULL;
    return &p->tokens[p->pos - 1];
}

static int parser_at_end(const vitte_parser* p) {
    return parser_peek(p)->kind == VITTE_TOK_EOF;
}

static const vitte_token* parser_advance(vitte_parser* p) {
    if (!parser_at_end(p)) {
        p->pos++;
    }
    return parser_prev(p);
}

static int parser_check(const vitte_parser* p, vitte_token_kind kind) {
    if (parser_at_end(p)) return kind == VITTE_TOK_EOF;
    return parser_peek(p)->kind == kind;
}

static int parser_match(vitte_parser* p, vitte_token_kind kind) {
    if (parser_check(p, kind)) {
        parser_advance(p);
        return 1;
    }
    return 0;
}

static vitte_span span_from_tokens(const vitte_token* start, const vitte_token* end) {
    vitte_span span;
    if (start) {
        span.start.line = start->line;
        span.start.col = start->col;
    } else {
        span.start.line = 0;
        span.start.col = 0;
    }
    if (end) {
        span.end.line = end->line;
        span.end.col = end->col + (uint32_t)(end->len ? end->len - 1 : 0);
    } else {
        span.end = span.start;
    }
    return span;
}

static vitte_ast* ast_new(vitte_ast_kind kind, const vitte_token* start, const vitte_token* end) {
    vitte_ast* node = (vitte_ast*)calloc(1, sizeof(vitte_ast));
    if (!node) return NULL;
    node->kind = kind;
    node->span = span_from_tokens(start, end ? end : start);
    return node;
}

static void ast_add_child(vitte_ast* parent, vitte_ast* child) {
    if (!parent || !child) return;
    if (!parent->first_child) {
        parent->first_child = child;
        parent->last_child = child;
    } else {
        parent->last_child->next = child;
        parent->last_child = child;
    }
}

static char* str_dup_range(const char* start, size_t len) {
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

static char* str_dup_token(const vitte_token* tok) {
    if (!tok || !tok->lexeme || tok->len == 0) return NULL;
    return str_dup_range(tok->lexeme, tok->len);
}

static int sb_grow(str_builder* sb, size_t extra) {
    if (sb->len + extra <= sb->cap) return 1;
    size_t new_cap = sb->cap ? sb->cap * 2 : 32;
    while (new_cap < sb->len + extra) {
        new_cap *= 2;
    }
    char* nt = (char*)realloc(sb->data, new_cap);
    if (!nt) return 0;
    sb->data = nt;
    sb->cap = new_cap;
    return 1;
}

static int sb_append(str_builder* sb, const char* data, size_t len) {
    if (!sb_grow(sb, len)) return 0;
    memcpy(sb->data + sb->len, data, len);
    sb->len += len;
    return 1;
}

static char* sb_build(str_builder* sb) {
    if (!sb_grow(sb, 1)) return NULL;
    sb->data[sb->len] = '\0';
    return sb->data;
}

static void skip_newlines(vitte_parser* p) {
    while (parser_match(p, VITTE_TOK_NEWLINE)) {
        continue;
    }
}

static vitte_result parser_expect(vitte_parser* p, vitte_token_kind kind, const char* msg) {
    if (parser_check(p, kind)) {
        parser_advance(p);
        return VITTE_OK;
    }
    const vitte_token* tok = parser_peek(p);
    set_err(p->err, VITTE_ERRC_SYNTAX, tok->line, tok->col, msg);
    return VITTE_ERR_PARSE;
}

static vitte_ast* parse_toplevel(vitte_parser* p);
static vitte_ast* parse_module_line(vitte_parser* p);
static vitte_ast* parse_use_line(vitte_parser* p);
static vitte_ast* parse_type_block(vitte_parser* p);
static vitte_ast* parse_fn_block(vitte_parser* p);
static vitte_ast* parse_scn_block(vitte_parser* p);
static vitte_ast* parse_prog_block(vitte_parser* p);
static vitte_ast* parse_phrase_block(vitte_parser* p, vitte_ast_kind parent_kind);
static vitte_ast* parse_phrase_stmt(vitte_parser* p);
static vitte_ast* parse_stmt_set(vitte_parser* p);
static vitte_ast* parse_stmt_say(vitte_parser* p);
static vitte_ast* parse_stmt_do(vitte_parser* p);
static vitte_ast* parse_stmt_ret(vitte_parser* p);
static vitte_ast* parse_stmt_when(vitte_parser* p);
static vitte_ast* parse_stmt_loop(vitte_parser* p);
static vitte_ast* parse_expression(vitte_parser* p, int min_prec);
static vitte_ast* parse_primary(vitte_parser* p);
static vitte_ast* parse_unary(vitte_parser* p);

typedef struct {
    vitte_token_kind kind;
    int precedence;
    vitte_binary_op op;
} binop_entry;

static const binop_entry g_binops[] = {
    {VITTE_TOK_OR, 1, VITTE_BINOP_OR},
    {VITTE_TOK_AND, 2, VITTE_BINOP_AND},
    {VITTE_TOK_EQEQ, 3, VITTE_BINOP_EQ},
    {VITTE_TOK_NEQ, 3, VITTE_BINOP_NEQ},
    {VITTE_TOK_LT, 4, VITTE_BINOP_LT},
    {VITTE_TOK_LTE, 4, VITTE_BINOP_LTE},
    {VITTE_TOK_GT, 4, VITTE_BINOP_GT},
    {VITTE_TOK_GTE, 4, VITTE_BINOP_GTE},
    {VITTE_TOK_PLUS, 5, VITTE_BINOP_ADD},
    {VITTE_TOK_MINUS, 5, VITTE_BINOP_SUB},
    {VITTE_TOK_STAR, 6, VITTE_BINOP_MUL},
    {VITTE_TOK_SLASH, 6, VITTE_BINOP_DIV},
    {VITTE_TOK_PERCENT, 6, VITTE_BINOP_MOD}
};

static int binop_precedence(vitte_token_kind kind, vitte_binary_op* op_out) {
    for (size_t i = 0; i < sizeof(g_binops) / sizeof(g_binops[0]); ++i) {
        if (g_binops[i].kind == kind) {
            if (op_out) *op_out = g_binops[i].op;
            return g_binops[i].precedence;
        }
    }
    return -1;
}

static vitte_ast* parse_identifier_path(vitte_parser* p, int allow_slash) {
    const vitte_token* first = parser_peek(p);
    if (!parser_check(p, VITTE_TOK_IDENT)) {
        set_err(p->err, VITTE_ERRC_SYNTAX, first->line, first->col, "expected identifier");
        return NULL;
    }
    str_builder sb = {0};
    if (!sb_append(&sb, first->lexeme, first->len)) {
        free(sb.data);
        return NULL;
    }
    parser_advance(p);
    while (1) {
        if (parser_match(p, VITTE_TOK_DOT)) {
            const vitte_token* seg = parser_peek(p);
            if (!parser_check(p, VITTE_TOK_IDENT)) {
                set_err(p->err, VITTE_ERRC_SYNTAX, seg->line, seg->col, "expected identifier segment");
                free(sb.data);
                return NULL;
            }
            if (!sb_append(&sb, ".", 1) || !sb_append(&sb, seg->lexeme, seg->len)) {
                free(sb.data);
                return NULL;
            }
            parser_advance(p);
            continue;
        }
        if (allow_slash && parser_match(p, VITTE_TOK_SLASH)) {
            const vitte_token* seg = parser_peek(p);
            if (!parser_check(p, VITTE_TOK_IDENT)) {
                set_err(p->err, VITTE_ERRC_SYNTAX, seg->line, seg->col, "expected identifier segment");
                free(sb.data);
                return NULL;
            }
            if (!sb_append(&sb, "/", 1) || !sb_append(&sb, seg->lexeme, seg->len)) {
                free(sb.data);
                return NULL;
            }
            parser_advance(p);
            continue;
        }
        break;
    }
    char* combined = sb_build(&sb);
    if (!combined) {
        free(sb.data);
        return NULL;
    }
    const vitte_token* last = parser_prev(p);
    vitte_ast* path = ast_new(VITTE_AST_PATH, first, last);
    if (!path) {
        free(combined);
        return NULL;
    }
    path->text = combined;
    return path;
}

static vitte_result expect_newline(vitte_parser* p) {
    if (parser_match(p, VITTE_TOK_NEWLINE)) {
        skip_newlines(p);
        return VITTE_OK;
    }
    const vitte_token* tok = parser_peek(p);
    set_err(p->err, VITTE_ERRC_SYNTAX, tok->line, tok->col, "expected newline");
    return VITTE_ERR_PARSE;
}

static vitte_ast* parse_type_ref(vitte_parser* p) {
    vitte_ast* path = parse_identifier_path(p, 1);
    if (!path) return NULL;
    path->kind = VITTE_AST_TYPE_REF;
    return path;
}

static vitte_ast* parse_module_path(vitte_parser* p) {
    return parse_identifier_path(p, 1);
}

static vitte_ast* parse_field_line(vitte_parser* p) {
    const vitte_token* start = parser_peek(p);
    if (!parser_match(p, VITTE_TOK_KW_FIELD)) {
        set_err(p->err, VITTE_ERRC_SYNTAX, start->line, start->col, "expected field");
        return NULL;
    }
    const vitte_token* name_tok = parser_peek(p);
    if (!parser_check(p, VITTE_TOK_IDENT)) {
        set_err(p->err, VITTE_ERRC_SYNTAX, name_tok->line, name_tok->col, "expected field name");
        return NULL;
    }
    parser_advance(p);
    if (parser_expect(p, VITTE_TOK_COLON, "expected : after field name") != VITTE_OK) return NULL;
    vitte_ast* ty = parse_type_ref(p);
    if (!ty) return NULL;
    const vitte_token* last = parser_prev(p);
    vitte_ast* field = ast_new(VITTE_AST_FIELD_DECL, start, last);
    if (!field) {
        vitte_ast_free(NULL, ty);
        return NULL;
    }
    field->text = str_dup_token(name_tok);
    ast_add_child(field, ty);
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, field);
        return NULL;
    }
    return field;
}

static vitte_ast* parse_param_list(vitte_parser* p, int* out_error) {
    if (out_error) *out_error = 0;
    vitte_ast* first_param = NULL;
    vitte_ast* last_param = NULL;
    if (parser_match(p, VITTE_TOK_RPAREN)) {
        return NULL;
    }
    while (1) {
        const vitte_token* name_tok = parser_peek(p);
        if (!parser_check(p, VITTE_TOK_IDENT)) {
            set_err(p->err, VITTE_ERRC_SYNTAX, name_tok->line, name_tok->col, "expected parameter name");
            vitte_ast_free(NULL, first_param);
            if (out_error) *out_error = 1;
            return NULL;
        }
        parser_advance(p);
        if (parser_expect(p, VITTE_TOK_COLON, "expected : after parameter name") != VITTE_OK) {
            vitte_ast_free(NULL, first_param);
            if (out_error) *out_error = 1;
            return NULL;
        }
        vitte_ast* ty = parse_type_ref(p);
        if (!ty) {
            vitte_ast_free(NULL, first_param);
            if (out_error) *out_error = 1;
            return NULL;
        }
        vitte_ast* param = ast_new(VITTE_AST_PARAM, name_tok, parser_prev(p));
        if (!param) {
            vitte_ast_free(NULL, ty);
            vitte_ast_free(NULL, first_param);
            if (out_error) *out_error = 1;
            return NULL;
        }
        param->text = str_dup_token(name_tok);
        ast_add_child(param, ty);
        if (!first_param) {
            first_param = param;
            last_param = param;
        } else {
            last_param->next = param;
            last_param = param;
        }
        if (parser_match(p, VITTE_TOK_COMMA)) {
            continue;
        }
        break;
    }
    if (parser_expect(p, VITTE_TOK_RPAREN, "expected ) after parameters") != VITTE_OK) {
        vitte_ast_free(NULL, first_param);
        if (out_error) *out_error = 1;
        return NULL;
    }
    return first_param;
}

static vitte_ast* parse_module_line(vitte_parser* p) {
    const vitte_token* start = parser_prev(p);
    vitte_ast* path = parse_module_path(p);
    if (!path) return NULL;
    const vitte_token* end = parser_prev(p);
    vitte_ast* mod = ast_new(VITTE_AST_MODULE_DECL, start, end);
    if (!mod) {
        vitte_ast_free(NULL, path);
        return NULL;
    }
    mod->text = path->text ? str_dup_range(path->text, strlen(path->text)) : NULL;
    vitte_ast_free(NULL, path);
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, mod);
        return NULL;
    }
    return mod;
}

static vitte_ast* parse_use_line(vitte_parser* p) {
    const vitte_token* start = parser_prev(p);
    vitte_ast* path = parse_module_path(p);
    if (!path) return NULL;
    vitte_ast* use = ast_new(VITTE_AST_USE_DECL, start, parser_prev(p));
    if (!use) {
        vitte_ast_free(NULL, path);
        return NULL;
    }
    use->text = path->text ? str_dup_range(path->text, strlen(path->text)) : NULL;
    vitte_ast_free(NULL, path);
    if (parser_match(p, VITTE_TOK_KW_AS)) {
        const vitte_token* alias_tok = parser_peek(p);
        if (!parser_check(p, VITTE_TOK_IDENT)) {
            set_err(p->err, VITTE_ERRC_SYNTAX, alias_tok->line, alias_tok->col, "expected alias identifier");
            vitte_ast_free(NULL, use);
            return NULL;
        }
        parser_advance(p);
        use->aux_text = str_dup_token(alias_tok);
    }
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, use);
        return NULL;
    }
    return use;
}

static vitte_ast* parse_type_block(vitte_parser* p) {
    const vitte_token* name_tok = parser_peek(p);
    if (!parser_check(p, VITTE_TOK_IDENT)) {
        set_err(p->err, VITTE_ERRC_SYNTAX, name_tok->line, name_tok->col, "expected type name");
        return NULL;
    }
    parser_advance(p);
    vitte_ast* type = ast_new(VITTE_AST_TYPE_DECL, parser_prev(p), parser_prev(p));
    if (!type) return NULL;
    type->text = str_dup_token(name_tok);
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, type);
        return NULL;
    }
    skip_newlines(p);
    while (!parser_check(p, VITTE_TOK_DOTEND) && !parser_at_end(p)) {
        vitte_ast* field = parse_field_line(p);
        if (!field) {
            vitte_ast_free(NULL, type);
            return NULL;
        }
        ast_add_child(type, field);
        skip_newlines(p);
    }
    if (parser_expect(p, VITTE_TOK_DOTEND, "expected .end") != VITTE_OK) {
        vitte_ast_free(NULL, type);
        return NULL;
    }
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, type);
        return NULL;
    }
    return type;
}

static vitte_ast* parse_fn_block(vitte_parser* p) {
    const vitte_token* name_tok = parser_peek(p);
    if (!parser_check(p, VITTE_TOK_IDENT)) {
        set_err(p->err, VITTE_ERRC_SYNTAX, name_tok->line, name_tok->col, "expected function name");
        return NULL;
    }
    parser_advance(p);
    vitte_ast* fn = ast_new(VITTE_AST_FN_DECL, parser_prev(p), parser_prev(p));
    if (!fn) return NULL;
    fn->text = str_dup_token(name_tok);
    vitte_ast* params = NULL;
    if (parser_match(p, VITTE_TOK_LPAREN)) {
        int param_err = 0;
        params = parse_param_list(p, &param_err);
        if (param_err) {
            vitte_ast_free(NULL, params);
            vitte_ast_free(NULL, fn);
            return NULL;
        }
    }
    vitte_ast* ret_type = NULL;
    if (parser_match(p, VITTE_TOK_ARROW)) {
        ret_type = parse_type_ref(p);
        if (!ret_type) {
            vitte_ast_free(NULL, params);
            vitte_ast_free(NULL, fn);
            return NULL;
        }
        ret_type->aux_text = str_dup_range("return", 6);
    }
    if (params) {
        vitte_ast* it = params;
        while (it) {
            vitte_ast* next = it->next;
            it->next = NULL;
            ast_add_child(fn, it);
            it = next;
        }
    }
    if (ret_type) {
        ast_add_child(fn, ret_type);
    }
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, fn);
        return NULL;
    }
    skip_newlines(p);
    vitte_ast* body = parse_phrase_block(p, VITTE_AST_BLOCK);
    if (!body) {
        vitte_ast_free(NULL, fn);
        return NULL;
    }
    ast_add_child(fn, body);
    return fn;
}

static vitte_ast* parse_scn_block(vitte_parser* p) {
    const vitte_token* name_tok = parser_peek(p);
    if (!parser_check(p, VITTE_TOK_IDENT)) {
        set_err(p->err, VITTE_ERRC_SYNTAX, name_tok->line, name_tok->col, "expected scenario name");
        return NULL;
    }
    parser_advance(p);
    vitte_ast* scn = ast_new(VITTE_AST_SCENARIO_DECL, parser_prev(p), parser_prev(p));
    if (!scn) return NULL;
    scn->text = str_dup_token(name_tok);
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, scn);
        return NULL;
    }
    skip_newlines(p);
    vitte_ast* body = parse_phrase_block(p, VITTE_AST_BLOCK);
    if (!body) {
        vitte_ast_free(NULL, scn);
        return NULL;
    }
    ast_add_child(scn, body);
    return scn;
}

static vitte_ast* parse_prog_block(vitte_parser* p) {
    vitte_ast* path = parse_module_path(p);
    if (!path) return NULL;
    if (parser_expect(p, VITTE_TOK_DOT, "expected . before entrypoint") != VITTE_OK) {
        vitte_ast_free(NULL, path);
        return NULL;
    }
    const vitte_token* entry_tok = parser_peek(p);
    if (!parser_check(p, VITTE_TOK_IDENT)) {
        set_err(p->err, VITTE_ERRC_SYNTAX, entry_tok->line, entry_tok->col, "expected entry identifier");
        vitte_ast_free(NULL, path);
        return NULL;
    }
    parser_advance(p);
    vitte_ast* prog = ast_new(VITTE_AST_PROGRAM_DECL, entry_tok, entry_tok);
    if (!prog) {
        vitte_ast_free(NULL, path);
        return NULL;
    }
    prog->text = path->text ? str_dup_range(path->text, strlen(path->text)) : NULL;
    prog->aux_text = str_dup_token(entry_tok);
    vitte_ast_free(NULL, path);
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, prog);
        return NULL;
    }
    skip_newlines(p);
    vitte_ast* body = parse_phrase_block(p, VITTE_AST_BLOCK);
    if (!body) {
        vitte_ast_free(NULL, prog);
        return NULL;
    }
    ast_add_child(prog, body);
    return prog;
}

static vitte_ast* parse_phrase_block(vitte_parser* p, vitte_ast_kind parent_kind) {
    const vitte_token* start = parser_peek(p);
    vitte_ast* block = ast_new(parent_kind, start, start);
    if (!block) return NULL;
    skip_newlines(p);
    while (!parser_check(p, VITTE_TOK_DOTEND) && !parser_at_end(p)) {
        vitte_ast* stmt = parse_phrase_stmt(p);
        if (!stmt) {
            vitte_ast_free(NULL, block);
            return NULL;
        }
        ast_add_child(block, stmt);
        skip_newlines(p);
    }
    if (parser_expect(p, VITTE_TOK_DOTEND, "expected .end") != VITTE_OK) {
        vitte_ast_free(NULL, block);
        return NULL;
    }
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, block);
        return NULL;
    }
    return block;
}

static vitte_ast* parse_stmt_set(vitte_parser* p) {
    const vitte_token* start = parser_prev(p);
    vitte_ast* target = parse_identifier_path(p, 0);
    if (!target) return NULL;
    target->kind = VITTE_AST_EXPR_PATH;
    if (parser_expect(p, VITTE_TOK_EQUAL, "expected =") != VITTE_OK) {
        vitte_ast_free(NULL, target);
        return NULL;
    }
    vitte_ast* expr = parse_expression(p, 0);
    if (!expr) {
        vitte_ast_free(NULL, target);
        return NULL;
    }
    vitte_ast* stmt = ast_new(VITTE_AST_PHR_STMT_SET, start, parser_prev(p));
    if (!stmt) {
        vitte_ast_free(NULL, target);
        vitte_ast_free(NULL, expr);
        return NULL;
    }
    ast_add_child(stmt, target);
    ast_add_child(stmt, expr);
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, stmt);
        return NULL;
    }
    return stmt;
}

static vitte_ast* parse_stmt_say(vitte_parser* p) {
    const vitte_token* start = parser_prev(p);
    vitte_ast* expr = parse_expression(p, 0);
    if (!expr) return NULL;
    vitte_ast* stmt = ast_new(VITTE_AST_PHR_STMT_SAY, start, parser_prev(p));
    if (!stmt) {
        vitte_ast_free(NULL, expr);
        return NULL;
    }
    ast_add_child(stmt, expr);
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, stmt);
        return NULL;
    }
    return stmt;
}

static vitte_ast* parse_stmt_do(vitte_parser* p) {
    const vitte_token* start = parser_prev(p);
    const vitte_token* name_tok = parser_peek(p);
    if (!parser_check(p, VITTE_TOK_IDENT)) {
        set_err(p->err, VITTE_ERRC_SYNTAX, name_tok->line, name_tok->col, "expected identifier after do");
        return NULL;
    }
    parser_advance(p);
    vitte_ast* stmt = ast_new(VITTE_AST_PHR_STMT_DO, start, name_tok);
    if (!stmt) return NULL;
    stmt->text = str_dup_token(name_tok);
    while (!parser_check(p, VITTE_TOK_NEWLINE) && !parser_check(p, VITTE_TOK_DOTEND) && !parser_at_end(p)) {
        vitte_ast* arg = parse_expression(p, 0);
        if (!arg) {
            vitte_ast_free(NULL, stmt);
            return NULL;
        }
        vitte_ast* arg_node = ast_new(VITTE_AST_EXPR_ARG, parser_prev(p), parser_prev(p));
        if (!arg_node) {
            vitte_ast_free(NULL, arg);
            vitte_ast_free(NULL, stmt);
            return NULL;
        }
        ast_add_child(arg_node, arg);
        ast_add_child(stmt, arg_node);
        if (parser_check(p, VITTE_TOK_NEWLINE) || parser_check(p, VITTE_TOK_DOTEND)) break;
    }
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, stmt);
        return NULL;
    }
    return stmt;
}

static vitte_ast* parse_stmt_ret(vitte_parser* p) {
    const vitte_token* start = parser_prev(p);
    vitte_ast* stmt = ast_new(VITTE_AST_PHR_STMT_RET, start, start);
    if (!stmt) return NULL;
    if (!parser_check(p, VITTE_TOK_NEWLINE)) {
        vitte_ast* expr = parse_expression(p, 0);
        if (!expr) {
            vitte_ast_free(NULL, stmt);
            return NULL;
        }
        ast_add_child(stmt, expr);
    }
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, stmt);
        return NULL;
    }
    return stmt;
}

static vitte_ast* parse_when_branch(vitte_parser* p, int is_else) {
    const vitte_token* start = parser_peek(p);
    vitte_ast* branch = ast_new(VITTE_AST_BRANCH, start, start);
    if (!branch) return NULL;
    branch->literal.kind = VITTE_LITERAL_BOOL;
    branch->literal.bool_value = is_else ? 1 : 0;
    vitte_ast* cond = NULL;
    if (!is_else) {
        cond = parse_expression(p, 0);
        if (!cond) {
            vitte_ast_free(NULL, branch);
            return NULL;
        }
        ast_add_child(branch, cond);
    }
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, branch);
        return NULL;
    }
    skip_newlines(p);
    vitte_ast* block = ast_new(VITTE_AST_BLOCK, parser_peek(p), parser_peek(p));
    if (!block) {
        vitte_ast_free(NULL, branch);
        return NULL;
    }
    while (!parser_check(p, VITTE_TOK_DOTEND) &&
           !(parser_check(p, VITTE_TOK_KW_ELSE)) &&
           !parser_at_end(p)) {
        vitte_ast* stmt = parse_phrase_stmt(p);
        if (!stmt) {
            vitte_ast_free(NULL, branch);
            vitte_ast_free(NULL, block);
            return NULL;
        }
        ast_add_child(block, stmt);
        skip_newlines(p);
    }
    ast_add_child(branch, block);
    return branch;
}

static vitte_ast* parse_stmt_when(vitte_parser* p) {
    const vitte_token* start = parser_prev(p);
    vitte_ast* when_stmt = ast_new(VITTE_AST_PHR_STMT_WHEN, start, start);
    if (!when_stmt) return NULL;
    vitte_ast* first_branch = parse_when_branch(p, 0);
    if (!first_branch) {
        vitte_ast_free(NULL, when_stmt);
        return NULL;
    }
    ast_add_child(when_stmt, first_branch);
    while (parser_match(p, VITTE_TOK_KW_ELSE)) {
        if (parser_match(p, VITTE_TOK_KW_WHEN)) {
            vitte_ast* branch = parse_when_branch(p, 0);
            if (!branch) {
                vitte_ast_free(NULL, when_stmt);
                return NULL;
            }
            ast_add_child(when_stmt, branch);
        } else {
            vitte_ast* else_branch = parse_when_branch(p, 1);
            if (!else_branch) {
                vitte_ast_free(NULL, when_stmt);
                return NULL;
            }
            ast_add_child(when_stmt, else_branch);
            break;
        }
    }
    if (parser_expect(p, VITTE_TOK_DOTEND, "expected .end for when") != VITTE_OK) {
        vitte_ast_free(NULL, when_stmt);
        return NULL;
    }
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, when_stmt);
        return NULL;
    }
    return when_stmt;
}

static vitte_ast* parse_stmt_loop(vitte_parser* p) {
    const vitte_token* start = parser_prev(p);
    const vitte_token* name_tok = parser_peek(p);
    if (!parser_check(p, VITTE_TOK_IDENT)) {
        set_err(p->err, VITTE_ERRC_SYNTAX, name_tok->line, name_tok->col, "expected loop variable name");
        return NULL;
    }
    parser_advance(p);
    if (parser_expect(p, VITTE_TOK_KW_FROM, "expected from") != VITTE_OK) return NULL;
    vitte_ast* start_expr = parse_expression(p, 0);
    if (!start_expr) return NULL;
    if (parser_expect(p, VITTE_TOK_KW_TO, "expected to") != VITTE_OK) {
        vitte_ast_free(NULL, start_expr);
        return NULL;
    }
    vitte_ast* end_expr = parse_expression(p, 0);
    if (!end_expr) {
        vitte_ast_free(NULL, start_expr);
        return NULL;
    }
    vitte_ast* step_expr = NULL;
    if (parser_match(p, VITTE_TOK_KW_STEP)) {
        step_expr = parse_expression(p, 0);
        if (!step_expr) {
            vitte_ast_free(NULL, start_expr);
            vitte_ast_free(NULL, end_expr);
            return NULL;
        }
    }
    if (expect_newline(p) != VITTE_OK) {
        vitte_ast_free(NULL, start_expr);
        vitte_ast_free(NULL, end_expr);
        vitte_ast_free(NULL, step_expr);
        return NULL;
    }
    skip_newlines(p);
    vitte_ast* body = parse_phrase_block(p, VITTE_AST_BLOCK);
    if (!body) {
        vitte_ast_free(NULL, start_expr);
        vitte_ast_free(NULL, end_expr);
        vitte_ast_free(NULL, step_expr);
        return NULL;
    }
    vitte_ast* loop = ast_new(VITTE_AST_PHR_STMT_LOOP, start, parser_prev(p));
    if (!loop) {
        vitte_ast_free(NULL, start_expr);
        vitte_ast_free(NULL, end_expr);
        vitte_ast_free(NULL, step_expr);
        vitte_ast_free(NULL, body);
        return NULL;
    }
    loop->text = str_dup_token(name_tok);
    ast_add_child(loop, start_expr);
    ast_add_child(loop, end_expr);
    if (step_expr) ast_add_child(loop, step_expr);
    ast_add_child(loop, body);
    return loop;
}

static vitte_ast* parse_phrase_stmt(vitte_parser* p) {
    const vitte_token* tok = parser_peek(p);
    switch (tok->kind) {
        case VITTE_TOK_KW_SET:
            parser_advance(p);
            return parse_stmt_set(p);
        case VITTE_TOK_KW_SAY:
            parser_advance(p);
            return parse_stmt_say(p);
        case VITTE_TOK_KW_DO:
            parser_advance(p);
            return parse_stmt_do(p);
        case VITTE_TOK_KW_RET:
            parser_advance(p);
            return parse_stmt_ret(p);
        case VITTE_TOK_KW_WHEN:
            parser_advance(p);
            return parse_stmt_when(p);
        case VITTE_TOK_KW_LOOP:
            parser_advance(p);
            return parse_stmt_loop(p);
        default:
            set_err(p->err, VITTE_ERRC_SYNTAX, tok->line, tok->col, "unexpected token in phrase block");
            return NULL;
    }
}

static vitte_ast* parse_primary(vitte_parser* p) {
    const vitte_token* tok = parser_peek(p);
    if (parser_match(p, VITTE_TOK_IDENT)) {
        vitte_ast* ident = ast_new(VITTE_AST_EXPR_IDENT, tok, tok);
        if (!ident) return NULL;
        ident->text = str_dup_token(tok);
        return ident;
    }
    if (parser_match(p, VITTE_TOK_INT) || parser_match(p, VITTE_TOK_FLOAT)) {
        vitte_ast* lit = ast_new(VITTE_AST_EXPR_LITERAL, tok, tok);
        if (!lit) return NULL;
        lit->text = str_dup_token(tok);
        lit->literal.kind = tok->kind == VITTE_TOK_INT ? VITTE_LITERAL_INT : VITTE_LITERAL_FLOAT;
        return lit;
    }
    if (parser_match(p, VITTE_TOK_STRING)) {
        vitte_ast* lit = ast_new(VITTE_AST_EXPR_LITERAL, tok, tok);
        if (!lit) return NULL;
        if (tok->len >= 2) {
            lit->text = str_dup_range(tok->lexeme + 1, tok->len - 2);
        }
        lit->literal.kind = VITTE_LITERAL_STRING;
        return lit;
    }
    if (parser_match(p, VITTE_TOK_TRUE) || parser_match(p, VITTE_TOK_FALSE)) {
        vitte_ast* lit = ast_new(VITTE_AST_EXPR_LITERAL, tok, tok);
        if (!lit) return NULL;
        lit->literal.kind = VITTE_LITERAL_BOOL;
        lit->literal.bool_value = tok->kind == VITTE_TOK_TRUE ? 1 : 0;
        return lit;
    }
    if (parser_match(p, VITTE_TOK_LPAREN)) {
        vitte_ast* expr = parse_expression(p, 0);
        if (!expr) return NULL;
        if (parser_expect(p, VITTE_TOK_RPAREN, "expected )") != VITTE_OK) {
            vitte_ast_free(NULL, expr);
            return NULL;
        }
        return expr;
    }
    set_err(p->err, VITTE_ERRC_SYNTAX, tok->line, tok->col, "expected expression");
    return NULL;
}

static vitte_ast* parse_unary(vitte_parser* p) {
    const vitte_token* tok = parser_peek(p);
    if (parser_match(p, VITTE_TOK_MINUS) || parser_match(p, VITTE_TOK_NOT)) {
        vitte_unary_op op = tok->kind == VITTE_TOK_MINUS ? VITTE_UNOP_NEG : VITTE_UNOP_NOT;
        vitte_ast* rhs = parse_unary(p);
        if (!rhs) return NULL;
        vitte_ast* node = ast_new(VITTE_AST_EXPR_UNARY, tok, parser_prev(p));
        if (!node) {
            vitte_ast_free(NULL, rhs);
            return NULL;
        }
        node->unary_op = op;
        ast_add_child(node, rhs);
        return node;
    }
    return parse_primary(p);
}

static vitte_ast* parse_expression(vitte_parser* p, int min_prec) {
    vitte_ast* lhs = parse_unary(p);
    if (!lhs) return NULL;
    while (1) {
        vitte_binary_op op;
        const vitte_token* tok = parser_peek(p);
        int prec = binop_precedence(tok->kind, &op);
        if (prec < min_prec) break;
        parser_advance(p);
        vitte_ast* rhs = parse_expression(p, prec + 1);
        if (!rhs) {
            vitte_ast_free(NULL, lhs);
            return NULL;
        }
        vitte_ast* node = ast_new(VITTE_AST_EXPR_BINARY, tok, parser_prev(p));
        if (!node) {
            vitte_ast_free(NULL, lhs);
            vitte_ast_free(NULL, rhs);
            return NULL;
        }
        node->binary_op = op;
        ast_add_child(node, lhs);
        ast_add_child(node, rhs);
        lhs = node;
    }
    return lhs;
}

static vitte_ast* parse_toplevel(vitte_parser* p) {
    const vitte_token* tok = parser_peek(p);
    if (parser_match(p, VITTE_TOK_KW_MOD)) {
        return parse_module_line(p);
    }
    if (parser_match(p, VITTE_TOK_KW_USE)) {
        return parse_use_line(p);
    }
    if (parser_match(p, VITTE_TOK_KW_TYPE)) {
        return parse_type_block(p);
    }
    if (parser_match(p, VITTE_TOK_KW_FN)) {
        return parse_fn_block(p);
    }
    if (parser_match(p, VITTE_TOK_KW_SCN)) {
        return parse_scn_block(p);
    }
    if (parser_match(p, VITTE_TOK_KW_PROG)) {
        return parse_prog_block(p);
    }
    set_err(p->err, VITTE_ERRC_SYNTAX, tok->line, tok->col, "unexpected token at top level");
    return NULL;
}

vitte_result vitte_parse_phrase(vitte_ctx* ctx,
                                const char* src,
                                size_t len,
                                vitte_ast** out_ast,
                                vitte_error* err) {
    if (!out_ast) {
        set_err(err, VITTE_ERRC_SYNTAX, 0, 0, "invalid out_ast");
        return VITTE_ERR_PARSE;
    }
    *out_ast = NULL;

    vitte_token* toks = NULL;
    size_t count = 0;
    vitte_result lr = vitte_lex_all(ctx, src, len, &toks, &count, err);
    if (lr != VITTE_OK) {
        free(toks);
        return lr;
    }

    vitte_parser parser = {ctx, toks, count, 0, err};
    vitte_ast* root = ast_new(VITTE_AST_PHR_UNIT, parser_peek(&parser), parser_peek(&parser));
    if (!root) {
        free(toks);
        return VITTE_ERR_INTERNAL;
    }

    skip_newlines(&parser);
    while (!parser_at_end(&parser)) {
        vitte_ast* node = parse_toplevel(&parser);
        if (!node) {
            vitte_ast_free(ctx, root);
            free(toks);
            return VITTE_ERR_PARSE;
        }
        ast_add_child(root, node);
        skip_newlines(&parser);
    }

    *out_ast = root;
    free(toks);
    return VITTE_OK;
}
