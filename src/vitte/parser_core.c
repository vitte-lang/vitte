#include "steel/vitte/parser_core.h"
#include "steel/diag/diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void bump(vitte_parser* p) { p->cur = vitte_lex_next(&p->lx); }

void vitte_parser_init(vitte_parser* p, const char* src, uint32_t len, uint32_t file_id) {
  p->src = src; p->len = len; p->file_id = file_id;
  vitte_lexer_init(&p->lx, src, len, file_id);
  p->had_error = 0;
  bump(p);
}

static int accept(vitte_parser* p, vitte_tok_kind k) {
  if (p->cur.kind == k) { bump(p); return 1; }
  return 0;
}

static int expect(vitte_parser* p, vitte_tok_kind k, const char* msg) {
  if (accept(p, k)) return 1;
  steel_diag_emit(STEEL_DIAG_ERROR, p->cur.span, msg);
  p->had_error = 1;
  return 0;
}

static ast_ident parse_ident(vitte_parser* p, const char* what) {
  ast_ident id = {0};
  if (p->cur.kind != TK_IDENT) {
    steel_diag_emit(STEEL_DIAG_ERROR, p->cur.span, what);
    p->had_error = 1;
    return id;
  }
  id.name_ptr = p->cur.lexeme;
  id.name_len = p->cur.len;
  id.name_span = p->cur.span;
  bump(p);
  return id;
}

static void parse_module_decl(vitte_parser* p, ast_program* out) {
  /* module <ident> */
  expect(p, TK_KW_MODULE, "expected 'module'");
  out->module_path = parse_ident(p, "expected module identifier");
  out->has_module = 1;
}

static void parse_end(vitte_parser* p) {
  /* .end */
  expect(p, TK_DOT, "expected '.' before end");
  ast_ident e = parse_ident(p, "expected 'end' after '.'");
  if (e.name_len == 3 && strncmp(e.name_ptr, "end", 3) == 0) return;
  steel_diag_emit(STEEL_DIAG_ERROR, e.name_span, "expected '.end'");
  p->had_error = 1;
}

static void parse_main_fn(vitte_parser* p, ast_program* out) {
  expect(p, TK_KW_FN, "expected 'fn'");
  out->main_fn.name = parse_ident(p, "expected function name");
  expect(p, TK_LPAREN, "expected '('");
  expect(p, TK_RPAREN, "expected ')'");
  expect(p, TK_ARROW, "expected '->'");
  /* type name (ignore), expect ident */
  (void)parse_ident(p, "expected return type identifier");

  /* body: only 'return <int> ; .end' */
  expect(p, TK_KW_RETURN, "expected 'return'");
  if (p->cur.kind != TK_INT) {
    steel_diag_emit(STEEL_DIAG_ERROR, p->cur.span, "expected integer literal after return (minimal subset)");
    p->had_error = 1;
  } else {
    out->main_fn.ret.ret_value.value = p->cur.int_val;
    out->main_fn.ret.ret_value.span = p->cur.span;
    bump(p);
  }
  /* optional ; */
  accept(p, TK_SEMI);
  parse_end(p);
}

int vitte_parse_program(vitte_parser* p, ast_program* out) {
  memset(out, 0, sizeof(*out));

  if (p->cur.kind == TK_KW_MODULE) {
    parse_module_decl(p, out);
  }

  /* Accept optional separators/newlines are skipped by lexer; next should be fn */
  if (p->cur.kind != TK_KW_FN) {
    steel_diag_emit(STEEL_DIAG_ERROR, p->cur.span, "expected 'fn' (only main supported in scaffold)");
    p->had_error = 1;
  } else {
    parse_main_fn(p, out);
  }

  if (p->had_error) return -1;
  return 0;
}
