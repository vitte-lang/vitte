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

static const char* keyword_name(vittec_keyword_t kw) {
  switch (kw) {
    case VITTEC_KW_MODULE: return "kw_module";
    case VITTEC_KW_IMPORT: return "kw_import";
    case VITTEC_KW_EXPORT: return "kw_export";
    case VITTEC_KW_USE: return "kw_use";
    case VITTEC_KW_TYPE: return "kw_type";
    case VITTEC_KW_STRUCT: return "kw_struct";
    case VITTEC_KW_ENUM: return "kw_enum";
    case VITTEC_KW_UNION: return "kw_union";
    case VITTEC_KW_FN: return "kw_fn";
    case VITTEC_KW_SCN: return "kw_scn";
    case VITTEC_KW_SCENARIO: return "kw_scenario";
    case VITTEC_KW_PROGRAM: return "kw_program";
    case VITTEC_KW_SERVICE: return "kw_service";
    case VITTEC_KW_KERNEL: return "kw_kernel";
    case VITTEC_KW_DRIVER: return "kw_driver";
    case VITTEC_KW_TOOL: return "kw_tool";
    case VITTEC_KW_PIPELINE: return "kw_pipeline";
    case VITTEC_KW_LET: return "kw_let";
    case VITTEC_KW_CONST: return "kw_const";
    case VITTEC_KW_IF: return "kw_if";
    case VITTEC_KW_ELIF: return "kw_elif";
    case VITTEC_KW_ELSE: return "kw_else";
    case VITTEC_KW_WHILE: return "kw_while";
    case VITTEC_KW_FOR: return "kw_for";
    case VITTEC_KW_MATCH: return "kw_match";
    case VITTEC_KW_BREAK: return "kw_break";
    case VITTEC_KW_CONTINUE: return "kw_continue";
    case VITTEC_KW_RETURN: return "kw_return";
    case VITTEC_KW_RET: return "kw_ret";
    case VITTEC_KW_SET: return "kw_set";
    case VITTEC_KW_SAY: return "kw_say";
    case VITTEC_KW_DO: return "kw_do";
    case VITTEC_KW_WHEN: return "kw_when";
    case VITTEC_KW_LOOP: return "kw_loop";
    case VITTEC_KW_TRUE: return "kw_true";
    case VITTEC_KW_FALSE: return "kw_false";
    case VITTEC_KW_NULL: return "kw_null";
    case VITTEC_KW_END: return "kw_end";
    default: return "kw_unknown";
  }
}

static const char* punct_name(vittec_punct_t punct) {
  switch (punct) {
    case VITTEC_PUNCT_LPAREN: return "punct_lparen";
    case VITTEC_PUNCT_RPAREN: return "punct_rparen";
    case VITTEC_PUNCT_LBRACK: return "punct_lbrack";
    case VITTEC_PUNCT_RBRACK: return "punct_rbrack";
    case VITTEC_PUNCT_COMMA: return "punct_comma";
    case VITTEC_PUNCT_DOT: return "punct_dot";
    case VITTEC_PUNCT_COLON: return "punct_colon";
    case VITTEC_PUNCT_SEMI: return "punct_semi";
    case VITTEC_PUNCT_EQ: return "punct_eq";
    case VITTEC_PUNCT_PLUS: return "punct_plus";
    case VITTEC_PUNCT_MINUS: return "punct_minus";
    case VITTEC_PUNCT_STAR: return "punct_star";
    case VITTEC_PUNCT_SLASH: return "punct_slash";
    case VITTEC_PUNCT_PERCENT: return "punct_percent";
    case VITTEC_PUNCT_AMP: return "punct_amp";
    case VITTEC_PUNCT_PIPE: return "punct_pipe";
    case VITTEC_PUNCT_CARET: return "punct_caret";
    case VITTEC_PUNCT_TILDE: return "punct_tilde";
    case VITTEC_PUNCT_BANG: return "punct_bang";
    case VITTEC_PUNCT_LT: return "punct_lt";
    case VITTEC_PUNCT_GT: return "punct_gt";
    case VITTEC_PUNCT_LE: return "punct_le";
    case VITTEC_PUNCT_GE: return "punct_ge";
    case VITTEC_PUNCT_EQEQ: return "punct_eqeq";
    case VITTEC_PUNCT_NE: return "punct_ne";
    case VITTEC_PUNCT_ANDAND: return "punct_andand";
    case VITTEC_PUNCT_OROR: return "punct_oror";
    case VITTEC_PUNCT_ARROW: return "punct_arrow";
    case VITTEC_PUNCT_FATARROW: return "punct_fatarrow";
    case VITTEC_PUNCT_QUESTION: return "punct_question";
    default: return "punct_unknown";
  }
}

vittec_session_t* vittec_session_new(void) {
  vittec_session_t* s = (vittec_session_t*)calloc(1, sizeof(vittec_session_t));
  VITTEC_ASSERT(s != NULL);
  vittec_source_map_init(&s->sm);
  vittec_diag_sink_init(&s->diags);
  return s;
}

void vittec_session_free(vittec_session_t* s) {
  if (!s) return;
  vittec_diag_sink_free(&s->diags);
  vittec_source_map_free(&s->sm);
  free(s);
}

static void print_tokens(const char* path, const char* src, uint32_t len, vittec_file_id_t file_id, vittec_diag_sink_t* diags) {
  (void)path;
  vittec_lexer_t lx;
  vittec_lexer_init(&lx, src, len, file_id, diags);

  for (;;) {
    vittec_token_t t = vittec_lex_next(&lx);
    if (t.kind == VITTEC_TOK_EOF) break;

    const char* kind_name = vittec_token_kind_name(t.kind);
    char detail[64];
    detail[0] = '\0';
    if (t.kind == VITTEC_TOK_KEYWORD) {
      snprintf(detail, sizeof(detail), " (%s)", keyword_name(t.as.kw));
    } else if (t.kind == VITTEC_TOK_PUNCT) {
      snprintf(detail, sizeof(detail), " (%s)", punct_name(t.as.punct));
    }

    printf("%s%s %u..%u `%.*s`\n",
           kind_name ? kind_name : "unknown",
           detail,
           t.span.lo,
           t.span.hi,
           (int)t.text.len,
           t.text.data ? t.text.data : "");
  }
}

int vittec_compile(vittec_session_t* s, const vittec_compile_options_t* opt) {
  if (!s || !opt || !opt->input_path) return 2;

  vittec_file_buf_t fb;
  if (vittec_read_entire_file(opt->input_path, &fb) != 0) {
    fprintf(stderr, "error: cannot read input: %s\\n", opt->input_path);
    return 1;
  }

  vittec_file_id_t file_id = 0;
  vittec_sv_t path_sv = vittec_sv(opt->input_path, (uint64_t)strlen(opt->input_path));
  int sm_err = vittec_source_map_add_memory(&s->sm, path_sv, fb.data, (size_t)fb.len, 1, &file_id);
  if (sm_err != VITTEC_SM_OK) {
    fprintf(stderr, "error: cannot register source: %s\n", opt->input_path);
    vittec_free_file_buf(&fb);
    return 1;
  }

  if (opt->emit_kind == VITTEC_EMIT_TOKENS) {
    print_tokens(opt->input_path, fb.data, (uint32_t)fb.len, file_id, &s->diags);
  } else if (opt->emit_kind == VITTEC_EMIT_C) {
    const char* out_path = opt->output_path ? opt->output_path : "out.c";

    vitte_ctx vctx;
    vitte_ctx_init(&vctx);
    vitte_codegen_unit unit;
    vitte_codegen_unit_init(&unit);

    vitte_diag_bag lbag;
    vitte_diag_bag_init(&lbag);
    vitte_result build_rc = vitte_codegen_unit_build(&vctx, (vitte_file_id)file_id, fb.data, (size_t)fb.len, &unit, &lbag);
    if (build_rc != VITTE_OK) {
      vittec_diag_bag_t bag;
      vittec_diag_bag_init(&bag);

      /* Convert libvitte diagnostics into vittec diagnostics for rendering. */
      for (uint32_t i = 0; i < lbag.len; i++) {
        const vitte_diag* ld = &lbag.diags[i];
        vittec_severity_t sev = VITTEC_SEV_ERROR;
        if (ld->severity == VITTE_SEV_WARNING) sev = VITTEC_SEV_WARNING;
        else if (ld->severity == VITTE_SEV_NOTE) sev = VITTEC_SEV_NOTE;

        vittec_sv_t code = vittec_sv(ld->code, (uint64_t)strlen(ld->code));
        vittec_sv_t message = vittec_sv(ld->message ? ld->message : "", (uint64_t)strlen(ld->message ? ld->message : ""));

        vitte_span primary = vitte_span_make(0u, 0u, 0u);
        for (uint32_t li = 0; li < ld->labels_len; li++) {
          if (ld->labels[li].style == VITTE_DIAG_LABEL_PRIMARY) {
            primary = ld->labels[li].span;
            break;
          }
        }
        vittec_span_t sp = vittec_span((vittec_file_id_t)primary.file_id, primary.lo, primary.hi);

        vittec_diag_t* d = vittec_diag_bag_push_new(&bag, sev, code, sp, message);
        if (!d) continue;

        for (uint32_t li = 0; li < ld->labels_len; li++) {
          const vitte_diag_label* lab = &ld->labels[li];
          if (lab->style != VITTE_DIAG_LABEL_SECONDARY) continue;
          vittec_span_t lsp = vittec_span((vittec_file_id_t)lab->span.file_id, lab->span.lo, lab->span.hi);
          vittec_diag_add_label(d, VITTEC_DIAG_LABEL_SECONDARY, lsp, vittec_sv(lab->message ? lab->message : "", (uint64_t)strlen(lab->message ? lab->message : "")));
        }

        if (ld->help && ld->help[0]) vittec_diag_set_help(d, vittec_sv(ld->help, (uint64_t)strlen(ld->help)));
        for (uint32_t ni = 0; ni < ld->notes_len; ni++) {
          const char* note = ld->notes[ni] ? ld->notes[ni] : "";
          vittec_diag_add_note(d, vittec_sv(note, (uint64_t)strlen(note)));
        }
      }

      vittec_emit_options_t emit_opt;
      vittec_emit_options_init(&emit_opt);
      emit_opt.sort_by_location = 1;
      if (opt->json_diagnostics) vittec_emit_json_bag_ex(&s->sm, &bag, &emit_opt);
      else vittec_emit_human_bag_ex(&s->sm, &bag, &emit_opt);

      vittec_diag_bag_free(&bag);
      vitte_diag_bag_free(&lbag);

      vitte_codegen_unit_reset(&vctx, &unit);
      vitte_ctx_free(&vctx);
      vittec_free_file_buf(&fb);
      return 1;
    }
    vitte_diag_bag_free(&lbag);

    vittec_emit_c_options_t emit_opt;
    vittec_emit_c_options_init(&emit_opt);
    emit_opt.mode = VITTEC_EMIT_C_MODE_FULL;

    if (vittec_emit_c_file_ex(&unit, out_path, &emit_opt) != 0) {
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
