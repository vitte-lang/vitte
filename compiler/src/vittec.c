#include "vittec/vittec.h"
#include "vittec/version.h"
#include "vittec/support/fs.h"
#include "vittec/support/assert.h"
#include "vittec/support/log.h"
#include "vittec/diag/source_map.h"
#include "vittec/diag/diagnostic.h"
#include "vittec/diag/emitter.h"
#include "vittec/front/lexer.h"
#include "vittec/front/parser.h"
#include "vittec/back/emit_c.h"
#include "vitte/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct vittec_session {
  vittec_source_map_t sm;
  vittec_diag_sink_t diags;
};

vittec_session_t* vittec_session_new(void) {
  vittec_session_t* s = (vittec_session_t*)calloc(1, sizeof(vittec_session_t));
  VITTEC_ASSERT(s != NULL);
  vittec_sourcemap_init(&s->sm);
  vittec_diag_sink_init(&s->diags);
  return s;
}

void vittec_session_free(vittec_session_t* s) {
  if (!s) return;
  vittec_diag_sink_free(&s->diags);
  vittec_sourcemap_free(&s->sm);
  free(s);
}

static void print_tokens(const char* path, const char* src, uint32_t len, uint32_t file_id, vittec_diag_sink_t* diags) {
  (void)path;
  vittec_lexer_t lx;
  vittec_lexer_init(&lx, src, len, file_id, diags);

  for (;;) {
    vittec_token_t t = vittec_lex_next(&lx);
    if (t.kind == TK_EOF) break;

    const char* k = "TK_UNKNOWN";
    switch (t.kind) {
      case TK_IDENT: k="TK_IDENT"; break;
      case TK_INT: k="TK_INT"; break;
      case TK_FLOAT: k="TK_FLOAT"; break;
      case TK_STRING: k="TK_STRING"; break;
      case TK_DOT: k="TK_DOT"; break;
      case TK_COMMA: k="TK_COMMA"; break;
      case TK_COLON: k="TK_COLON"; break;
      case TK_SEMI: k="TK_SEMI"; break;
      case TK_LPAREN: k="TK_LPAREN"; break;
      case TK_RPAREN: k="TK_RPAREN"; break;
      case TK_LBRACK: k="TK_LBRACK"; break;
      case TK_RBRACK: k="TK_RBRACK"; break;
      case TK_ARROW: k="TK_ARROW"; break;
      case TK_EQ: k="TK_EQ"; break;
      case TK_KW_MODULE: k="TK_KW_MODULE"; break;
      case TK_KW_IMPORT: k="TK_KW_IMPORT"; break;
      case TK_KW_EXPORT: k="TK_KW_EXPORT"; break;
      case TK_KW_STRUCT: k="TK_KW_STRUCT"; break;
      case TK_KW_ENUM: k="TK_KW_ENUM"; break;
      case TK_KW_TYPE: k="TK_KW_TYPE"; break;
      case TK_KW_FN: k="TK_KW_FN"; break;
      case TK_KW_LET: k="TK_KW_LET"; break;
      case TK_KW_CONST: k="TK_KW_CONST"; break;
      case TK_KW_IF: k="TK_KW_IF"; break;
      case TK_KW_ELIF: k="TK_KW_ELIF"; break;
      case TK_KW_ELSE: k="TK_KW_ELSE"; break;
      case TK_KW_WHILE: k="TK_KW_WHILE"; break;
      case TK_KW_FOR: k="TK_KW_FOR"; break;
      case TK_KW_MATCH: k="TK_KW_MATCH"; break;
      case TK_KW_BREAK: k="TK_KW_BREAK"; break;
      case TK_KW_CONTINUE: k="TK_KW_CONTINUE"; break;
      case TK_KW_RET: k="TK_KW_RET"; break;
      case TK_KW_SAY: k="TK_KW_SAY"; break;
      case TK_KW_DO: k="TK_KW_DO"; break;
      case TK_KW_SET: k="TK_KW_SET"; break;
      case TK_KW_LOOP: k="TK_KW_LOOP"; break;
      case TK_KW_WHEN: k="TK_KW_WHEN"; break;
      case TK_KW_END: k="TK_KW_END"; break;
      default: break;
    }

    printf("%s %u..%u `%.*s`\\n", k, t.span.lo, t.span.hi, (int)t.text.len, t.text.data);
  }
}

int vittec_compile(vittec_session_t* s, const vittec_compile_options_t* opt) {
  if (!s || !opt || !opt->input_path) return 2;

  vittec_file_buf_t fb;
  if (vittec_read_entire_file(opt->input_path, &fb) != 0) {
    fprintf(stderr, "error: cannot read input: %s\\n", opt->input_path);
    return 1;
  }

  uint32_t file_id = vittec_sourcemap_add(&s->sm, opt->input_path, fb.data, (uint32_t)fb.len);

  if (opt->emit_kind == VITTEC_EMIT_TOKENS) {
    print_tokens(opt->input_path, fb.data, (uint32_t)fb.len, file_id, &s->diags);
  } else if (opt->emit_kind == VITTEC_EMIT_C) {
    const char* out_path = opt->output_path ? opt->output_path : "out.c";

    vitte_ctx vctx;
    vitte_ctx_init(&vctx);
    vitte_codegen_unit unit;
    vitte_codegen_unit_init(&unit);

    vitte_error verr;
    memset(&verr, 0, sizeof(verr));
    vitte_result build_rc = vitte_codegen_unit_build(&vctx, fb.data, (size_t)fb.len, &unit, &verr);
    if (build_rc != VITTE_OK) {
      const char* msg = verr.message[0] ? verr.message : "vitte codegen unit build failed";
      fprintf(stderr, "error: %s:%u:%u: %s (code %d)\n",
              opt->input_path ? opt->input_path : "(stdin)",
              (unsigned)verr.line,
              (unsigned)verr.col,
              msg,
              (int)verr.code);
      vitte_codegen_unit_reset(&vctx, &unit);
      vitte_ctx_free(&vctx);
      vittec_free_file_buf(&fb);
      return 1;
    }

    if (vittec_emit_c_file(&unit, out_path) != 0) {
      fprintf(stderr, "error: cannot write: %s\n", out_path);
      vitte_codegen_unit_reset(&vctx, &unit);
      vitte_ctx_free(&vctx);
      vittec_free_file_buf(&fb);
      return 1;
    }

    vitte_codegen_unit_reset(&vctx, &unit);
    vitte_ctx_free(&vctx);
  } else {
    /* default: tokens */
    print_tokens(opt->input_path, fb.data, (uint32_t)fb.len, file_id, &s->diags);
  }

  if (s->diags.len) {
    if (opt->json_diagnostics) vittec_emit_json(&s->sm, &s->diags);
    else vittec_emit_human(&s->sm, &s->diags);
    vittec_free_file_buf(&fb);
    return 1;
  }

  vittec_free_file_buf(&fb);
  return 0;
}
