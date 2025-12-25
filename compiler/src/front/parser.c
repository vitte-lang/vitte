// parser.c
// -----------------------------------------------------------------------------
// vittec front-end bootstrap parser (top-level index)
//
// This matches the intent of `syntax/vitte.ebnf`:
//   - top-level: module/use/export/type/fn/scn/entrypoint + empty statements
//   - blocks: delimited by `.end` (lexer emits keyword END with FROM_DOT_END)
//
// The bootstrap parser is intentionally shallow: it records spans and names and
// can optionally capture token-span for bodies.
// -----------------------------------------------------------------------------

#include "vittec/front/parser.h"
#include "vittec/front/lexer.h"
#include "vittec/front/token.h"

#include <stdlib.h>
#include <string.h>

static int mul_overflow_size(size_t a, size_t b, size_t* out) {
  if (!out) return 1;
  if (a == 0 || b == 0) {
    *out = 0;
    return 0;
  }
  if (a > (SIZE_MAX / b))
    return 1;
  *out = a * b;
  return 0;
}

static void* d_alloc(vittec_alloc_t a, size_t n) {
  if (a.alloc) return a.alloc(a.user, n);
  return malloc(n);
}

static void* d_realloc(vittec_alloc_t a, void* p, size_t n) {
  if (a.realloc) return a.realloc(a.user, p, n);
  return realloc(p, n);
}

static void d_free(vittec_alloc_t a, void* p) {
  if (a.free) a.free(a.user, p);
  else free(p);
}

void vittec_parse_options_init(vittec_parse_options_t* opt) {
  if (!opt) return;
  memset(opt, 0, sizeof(*opt));
  opt->size = sizeof(*opt);
  opt->flags = VITTEC_PARSE_RECOVER | VITTEC_PARSE_SKIP_UNKNOWN_TOP;
}

void vittec_parse_unit_init(vittec_parse_unit_t* u) {
  if (!u) return;
  memset(u, 0, sizeof(*u));
}

void vittec_parse_unit_free(vittec_parse_unit_t* u) {
  if (!u) return;
  d_free(u->alloc, u->items);
  d_free(u->alloc, u->imports);
  d_free(u->alloc, u->exports);
  d_free(u->alloc, u->fns);
  memset(u, 0, sizeof(*u));
}

static int ensure_cap(void** buf, uint32_t* cap, uint32_t want, size_t elem, vittec_alloc_t a) {
  if (want <= *cap) return 1;
  uint32_t new_cap = (*cap == 0u) ? 8u : *cap;
  while (new_cap < want) {
    if (new_cap > (UINT32_MAX / 2u))
      return 0;
    new_cap *= 2u;
  }
  size_t bytes = 0;
  if (mul_overflow_size((size_t)new_cap, elem, &bytes))
    return 0;
  void* nb = d_realloc(a, *buf, bytes);
  if (!nb) return 0;
  *buf = nb;
  *cap = new_cap;
  return 1;
}

static void skip_stmt_end(vittec_lexer_t* lx) {
  if (!lx) return;
  for (;;) {
    vittec_token_t t = vittec_lex_peek(lx);
    if (t.kind == VITTEC_TOK_NEWLINE) {
      (void)vittec_lex_next(lx);
      continue;
    }
    if (t.kind == VITTEC_TOK_PUNCT && t.as.punct == VITTEC_PUNCT_SEMI) {
      (void)vittec_lex_next(lx);
      continue;
    }
    break;
  }
}

static vittec_token_t expect_ident(vittec_lexer_t* lx, int* ok) {
  vittec_token_t t = vittec_lex_next(lx);
  if (t.kind != VITTEC_TOK_IDENT) {
    if (ok) *ok = 0;
    return t;
  }
  if (ok) *ok = 1;
  return t;
}

static vittec_span_t span_join(vittec_span_t a, vittec_span_t b) {
  if (a.file_id != b.file_id) return a;
  vittec_span_t out = a;
  if (b.lo < out.lo) out.lo = b.lo;
  if (b.hi > out.hi) out.hi = b.hi;
  return out;
}

static int push_item(vittec_parse_unit_t* u, vittec_top_kind_t kind, vittec_span_t sp, vittec_sv_t kw) {
  if (!ensure_cap((void**)&u->items, &u->items_cap, u->items_len + 1u, sizeof(*u->items), u->alloc)) return 0;
  vittec_top_item_t* it = &u->items[u->items_len++];
  it->kind = kind;
  it->span = sp;
  it->keyword = kw;
  return 1;
}

static int push_path(vittec_path_t** arr, uint32_t* len, uint32_t* cap, vittec_alloc_t a, vittec_sv_t text, vittec_span_t sp) {
  if (!ensure_cap((void**)arr, cap, *len + 1u, sizeof(**arr), a)) return 0;
  vittec_path_t* p = &(*arr)[(*len)++];
  p->text = text;
  p->span = sp;
  return 1;
}

static int push_fn(vittec_parse_unit_t* u, vittec_fn_decl_t d) {
  if (!ensure_cap((void**)&u->fns, &u->fns_cap, u->fns_len + 1u, sizeof(*u->fns), u->alloc)) return 0;
  u->fns[u->fns_len++] = d;
  return 1;
}

static int token_is_dot_end(vittec_token_t t) {
  return t.kind == VITTEC_TOK_KEYWORD && t.as.kw == VITTEC_KW_END && (t.flags & VITTEC_TOKF_FROM_DOT_END);
}

static vittec_span_t skip_block(vittec_lexer_t* lx) {
  vittec_span_t start = vittec_span_with_file_id(lx ? lx->file_id : 0, 0, 0);
  vittec_span_t end = start;
  int started = 0;
  for (;;) {
    vittec_token_t t = vittec_lex_next(lx);
    if (!started) {
      start = t.span;
      end = t.span;
      started = 1;
    } else {
      end = t.span;
    }

    if (t.kind == VITTEC_TOK_EOF) break;
    if (token_is_dot_end(t)) break;
  }
  return span_join(start, end);
}

static int parse_module_decl(vittec_lexer_t* lx, vittec_parse_unit_t* u) {
  vittec_token_t kw = vittec_lex_next(lx); // "module" keyword token kind uses KW_MODULE
  (void)kw;
  int ok = 0;
  vittec_token_t name = expect_ident(lx, &ok);
  if (ok) {
    u->module_name = name.text;
    u->module_span = name.span;
  }
  skip_stmt_end(lx);
  return push_item(u, VITTEC_TOP_MODULE, span_join(kw.span, name.span), kw.text);
}

static int parse_use_like(vittec_lexer_t* lx, vittec_parse_unit_t* u) {
  vittec_token_t kw = vittec_lex_next(lx); // "use"
  vittec_span_t start = kw.span;
  // Capture a best-effort "path": Ident ('.'|'/') Ident ...
  vittec_token_t first = vittec_lex_next(lx);
  vittec_span_t sp = first.span;
  if (first.kind == VITTEC_TOK_IDENT) {
    for (;;) {
      vittec_token_t p = vittec_lex_peek(lx);
      if (p.kind == VITTEC_TOK_PUNCT && p.as.punct == VITTEC_PUNCT_DOT) {
        (void)vittec_lex_next(lx);
        vittec_token_t seg = vittec_lex_next(lx);
        sp = span_join(sp, seg.span);
        continue;
      }
      break;
    }
    (void)push_path(&u->imports, &u->imports_len, &u->imports_cap, u->alloc, first.text, sp);
  }
  skip_stmt_end(lx);
  return push_item(u, VITTEC_TOP_IMPORT, span_join(start, sp), kw.text);
}

static int parse_export_decl(vittec_lexer_t* lx, vittec_parse_unit_t* u) {
  vittec_token_t kw = vittec_lex_next(lx); // "export"
  vittec_span_t sp = kw.span;
  // Best-effort: consume until stmt end.
  for (;;) {
    vittec_token_t t = vittec_lex_peek(lx);
    if (t.kind == VITTEC_TOK_EOF) break;
    if (t.kind == VITTEC_TOK_NEWLINE) break;
    if (t.kind == VITTEC_TOK_PUNCT && t.as.punct == VITTEC_PUNCT_SEMI) break;
    t = vittec_lex_next(lx);
    sp = span_join(sp, t.span);
  }
  skip_stmt_end(lx);
  return push_item(u, VITTEC_TOP_EXPORT, sp, kw.text);
}

static int parse_fn_like(vittec_lexer_t* lx, vittec_parse_unit_t* u, const vittec_parse_options_t* opt, vittec_top_kind_t kind, vittec_token_t kw) {
  int ok = 0;
  vittec_token_t name = expect_ident(lx, &ok);
  vittec_fn_decl_t d;
  memset(&d, 0, sizeof(d));
  d.name = name.text;
  d.name_span = name.span;
  d.sig_span = span_join(kw.span, name.span);
  d.is_main = ok && vittec_sv_eq_lit(name.text, "main", 4);

  // Consume tokens until newline (signature end) then skip body until `.end`.
  for (;;) {
    vittec_token_t t = vittec_lex_next(lx);
    d.sig_span = span_join(d.sig_span, t.span);
    if (t.kind == VITTEC_TOK_EOF) break;
    if (t.kind == VITTEC_TOK_NEWLINE) break;
  }

  vittec_span_t body = vittec_span_with_file_id(kw.span.file_id, d.sig_span.hi, d.sig_span.hi);
  vittec_span_t body_sp = skip_block(lx);
  if (body_sp.hi > body_sp.lo) {
    body = body_sp;
    d.has_body = 1u;
  } else {
    d.has_body = 0u;
  }

  if (opt && (opt->flags & VITTEC_PARSE_CAPTURE_BODIES)) {
    d.body_span = body;
  } else {
    d.body_span = vittec_span_with_file_id(kw.span.file_id, 0u, 0u);
  }
  if (d.is_main) u->has_main = 1;

  if (kind == VITTEC_TOP_FN || kind == VITTEC_TOP_SCENARIO) {
    if (!push_fn(u, d)) return 0;
  }

  if (!push_item(u, kind, span_join(kw.span, body), kw.text)) return 0;
  skip_stmt_end(lx);
  return 1;
}

int vittec_parse_unit_ex(vittec_lexer_t* lx, vittec_parse_unit_t* out, const vittec_parse_options_t* opt) {
  if (!lx || !out) return 1;

  vittec_parse_options_t tmp;
  if (!opt) {
    vittec_parse_options_init(&tmp);
    opt = &tmp;
  }

  vittec_parse_unit_free(out);
  vittec_parse_unit_init(out);
  out->alloc = opt->alloc;

  skip_stmt_end(lx);

  uint32_t top_count = 0;
  uint32_t stall_count = 0;
  for (;;) {
    vittec_token_t t = vittec_lex_peek(lx);
    if (t.kind == VITTEC_TOK_EOF) break;

    uint32_t before_lo = t.span.lo;
    if (opt->max_top_items && top_count >= opt->max_top_items) {
      // Budget reached: stop producing more items, but ensure we still advance
      // the lexer at least once to guarantee forward progress on repeated calls.
      (void)vittec_lex_next(lx);
      skip_stmt_end(lx);
      break;
    }

    if (t.kind == VITTEC_TOK_NEWLINE || (t.kind == VITTEC_TOK_PUNCT && t.as.punct == VITTEC_PUNCT_SEMI)) {
      skip_stmt_end(lx);
      continue;
    }

    if (token_is_dot_end(t)) {
      // Unmatched `.end` at top-level; skip it.
      (void)vittec_lex_next(lx);
      skip_stmt_end(lx);
      continue;
    }

    if (t.kind == VITTEC_TOK_KEYWORD) {
      switch (t.as.kw) {
        case VITTEC_KW_MODULE:
          if (!parse_module_decl(lx, out)) return 1;
          top_count++;
          continue;
        case VITTEC_KW_USE:
          if (!parse_use_like(lx, out)) return 1;
          top_count++;
          continue;
        case VITTEC_KW_EXPORT:
          if (!parse_export_decl(lx, out)) return 1;
          top_count++;
          continue;
        case VITTEC_KW_FN: {
          vittec_token_t kw = vittec_lex_next(lx);
          if (!parse_fn_like(lx, out, opt, VITTEC_TOP_FN, kw)) return 1;
          top_count++;
          continue;
        }
        case VITTEC_KW_SCN:
        case VITTEC_KW_SCENARIO: {
          vittec_token_t kw = vittec_lex_next(lx);
          if (!parse_fn_like(lx, out, opt, VITTEC_TOP_SCENARIO, kw)) return 1;
          top_count++;
          continue;
        }
        case VITTEC_KW_PROG:
        case VITTEC_KW_PROGRAM:
        case VITTEC_KW_SERVICE:
        case VITTEC_KW_KERNEL:
        case VITTEC_KW_DRIVER:
        case VITTEC_KW_TOOL:
        case VITTEC_KW_PIPELINE: {
          vittec_token_t kw = vittec_lex_next(lx);
          if (!parse_fn_like(lx, out, opt, VITTEC_TOP_ENTRY, kw)) return 1;
          top_count++;
          continue;
        }
        default:
          break;
      }
    }

    if (opt->flags & VITTEC_PARSE_SKIP_UNKNOWN_TOP) {
      // Skip unknown top-level item until newline/stmt-end.
      for (;;) {
        vittec_token_t s = vittec_lex_peek(lx);
        if (s.kind == VITTEC_TOK_EOF) break;
        if (s.kind == VITTEC_TOK_NEWLINE) break;
        if (s.kind == VITTEC_TOK_PUNCT && s.as.punct == VITTEC_PUNCT_SEMI) break;
        (void)vittec_lex_next(lx);
      }
      skip_stmt_end(lx);

      // Safety: if the lexer did not advance, force-consume one token to guarantee
      // progress on pathological inputs/lexers.
      t = vittec_lex_peek(lx);
      if (t.kind != VITTEC_TOK_EOF && t.span.lo == before_lo) {
        stall_count++;
        (void)vittec_lex_next(lx);
        skip_stmt_end(lx);
        if (stall_count > 256u) break;
      } else {
        stall_count = 0;
      }
      continue;
    }

    return 1;
  }

  return 0;
}

int vittec_parse_unit(vittec_lexer_t* lx, vittec_parse_unit_t* out) {
  return vittec_parse_unit_ex(lx, out, NULL);
}
