#include "vittec/diag/emitter.h"
#include "vittec/diag/source_map.h"
#include "vittec/diag/diagnostic.h"
#include "vittec/diag/severity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward decls */
void vittec_emit_json_one_ex(const vittec_source_map_t* sm, const vittec_diag_t* d, const vittec_emit_options_t* opt);

/* ========================================================================
 * Minimal JSON writer (no deps)
 * ======================================================================== */

static void vittec__j_putc(FILE* out, int c) {
  if(!out) out = stderr;
  fputc(c, out);
}

static void vittec__j_write(FILE* out, const char* s) {
  if(!out) out = stderr;
  if(!s) return;
  fputs(s, out);
}

static void vittec__j_u32(FILE* out, uint32_t v) {
  if(!out) out = stderr;
  fprintf(out, "%u", (unsigned)v);
}

static void vittec__j_escape_sv(FILE* out, vittec_sv_t s) {
  if(!out) out = stderr;
  vittec__j_putc(out, '"');

  const unsigned char* p = (const unsigned char*)s.data;
  size_t n = (size_t)s.len;
  for(size_t i=0; i<n; i++) {
    unsigned char c = p ? p[i] : 0u;
    switch(c) {
      case '\\': vittec__j_write(out, "\\\\"); break;
      case '"':  vittec__j_write(out, "\\\""); break;
      case '\b': vittec__j_write(out, "\\b"); break;
      case '\f': vittec__j_write(out, "\\f"); break;
      case '\n': vittec__j_write(out, "\\n"); break;
      case '\r': vittec__j_write(out, "\\r"); break;
      case '\t': vittec__j_write(out, "\\t"); break;
      default:
        if(c < 0x20u) {
          static const char* H = "0123456789abcdef";
          char u[7];
          u[0] = '\\'; u[1] = 'u'; u[2] = '0'; u[3] = '0';
          u[4] = H[(c >> 4) & 0xF];
          u[5] = H[c & 0xF];
          u[6] = 0;
          vittec__j_write(out, u);
        } else {
          vittec__j_putc(out, (int)c);
        }
        break;
    }
  }

  vittec__j_putc(out, '"');
}

static void vittec__j_escape_cstr(FILE* out, const char* s) {
  vittec_sv_t sv;
  sv.data = s;
  sv.len = s ? strlen(s) : 0u;
  vittec__j_escape_sv(out, sv);
}

static const char* vittec__sev_name(vittec_severity_t sev) {
#ifdef VITTEC_SEVERITY_API_VERSION
  return vittec_severity_name(sev);
#else
  switch(sev) {
    case VITTEC_SEV_ERROR:   return "error";
    case VITTEC_SEV_WARNING: return "warning";
    case VITTEC_SEV_NOTE:    return "note";
    case VITTEC_SEV_HELP:    return "help";
    default:                 return "unknown";
  }
#endif
}

static const char* vittec__label_style_name(vittec_diag_label_style_t k) {
  switch(k) {
    case VITTEC_DIAG_LABEL_PRIMARY: return "primary";
    case VITTEC_DIAG_LABEL_SECONDARY: return "secondary";
    default: return "unknown";
  }
}

static FILE* vittec__opt_out(const vittec_emit_options_t* opt) {
  return (opt && opt->out_stream) ? (FILE*)opt->out_stream : stderr;
}

/* ========================================================================
 * Structured JSON emitters (JSON Lines)
 * ======================================================================== */

static vittec_span_t vittec__primary_span(const vittec_diag_t* d) {
  vittec_span_t sp;
  memset(&sp, 0, sizeof(sp));
  if(!d || !d->labels || d->labels_len == 0) return sp;
  for(uint32_t i=0; i<d->labels_len; i++) {
    if(d->labels[i].style == VITTEC_DIAG_LABEL_PRIMARY) return d->labels[i].span;
  }
  return d->labels[0].span;
}

static void vittec__emit_diag_one_jsonl(FILE* out, const vittec_source_map_t* sm, const vittec_diag_t* d, uint32_t context_lines) {
  (void)context_lines;
  if(!out) out = stderr;
  if(!d) return;

  vittec_span_t primary = vittec__primary_span(d);
  vittec_sv_t path = vittec_source_map_file_path(sm, primary.file);
  vittec_line_col_t lc0 = vittec_source_map_line_col(sm, primary.file, primary.lo);

  vittec__j_putc(out, '{');

  vittec__j_write(out, "\"severity\":");
  vittec__j_escape_cstr(out, vittec__sev_name(d->severity));

  if(d->code.data && d->code.len) {
    vittec__j_write(out, ",\"code\":");
    vittec__j_escape_sv(out, d->code);
  }

  vittec__j_write(out, ",\"message\":");
  vittec__j_escape_sv(out, d->message);

  if(d->help.data && d->help.len) {
    vittec__j_write(out, ",\"help\":");
    vittec__j_escape_sv(out, d->help);
  }

  vittec__j_write(out, ",\"location\":{");
  vittec__j_write(out, "\"path\":"); vittec__j_escape_sv(out, path);
  vittec__j_write(out, ",\"file_id\":"); vittec__j_u32(out, primary.file);
  vittec__j_write(out, ",\"span\":{\"lo\":"); vittec__j_u32(out, primary.lo);
  vittec__j_write(out, ",\"hi\":"); vittec__j_u32(out, primary.hi);
  vittec__j_write(out, "}");
  vittec__j_write(out, ",\"line\":"); vittec__j_u32(out, lc0.line);
  vittec__j_write(out, ",\"col\":"); vittec__j_u32(out, lc0.col);
  vittec__j_putc(out, '}');

  vittec__j_write(out, ",\"labels\":[");
  if(d->labels && d->labels_len) {
    for(uint32_t i=0; i<d->labels_len; i++) {
      const vittec_diag_label_t* lab = &d->labels[i];
      vittec_sv_t p = vittec_source_map_file_path(sm, lab->span.file);
      vittec_line_col_t lc = vittec_source_map_line_col(sm, lab->span.file, lab->span.lo);
      if(i) vittec__j_putc(out, ',');
      vittec__j_putc(out, '{');
      vittec__j_write(out, "\"kind\":"); vittec__j_escape_cstr(out, vittec__label_style_name(lab->style));
      vittec__j_write(out, ",\"path\":"); vittec__j_escape_sv(out, p);
      vittec__j_write(out, ",\"file_id\":"); vittec__j_u32(out, lab->span.file);
      vittec__j_write(out, ",\"lo\":"); vittec__j_u32(out, lab->span.lo);
      vittec__j_write(out, ",\"hi\":"); vittec__j_u32(out, lab->span.hi);
      vittec__j_write(out, ",\"line\":"); vittec__j_u32(out, lc.line);
      vittec__j_write(out, ",\"col\":"); vittec__j_u32(out, lc.col);
      if(lab->message.data && lab->message.len) {
        vittec__j_write(out, ",\"message\":"); vittec__j_escape_sv(out, lab->message);
      }
      vittec__j_putc(out, '}');
    }
  }
  vittec__j_putc(out, ']');

  vittec__j_write(out, ",\"notes\":[");
  if(d->notes && d->notes_len) {
    for(uint32_t i=0; i<d->notes_len; i++) {
      if(i) vittec__j_putc(out, ',');
      vittec__j_escape_sv(out, d->notes[i]);
    }
  }
  vittec__j_putc(out, ']');

  vittec__j_putc(out, '}');
}

void vittec_emit_json_one(const vittec_source_map_t* sm, const vittec_diag_t* d) {
  vittec_emit_json_one_ex(sm, d, NULL);
}

void vittec_emit_json_one_ex(const vittec_source_map_t* sm, const vittec_diag_t* d, const vittec_emit_options_t* opt) {
  FILE* out = vittec__opt_out(opt);
  vittec__emit_diag_one_jsonl(out, sm, d, 0u);
  vittec__j_putc(out, '\n');
}

void vittec_emit_json_bag(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag) {
  vittec_emit_json_bag_ex(sm, bag, NULL);
}

void vittec_emit_json_bag_ex(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag, const vittec_emit_options_t* opt) {
  FILE* out = vittec__opt_out(opt);
  if(!bag || !bag->diags || bag->len == 0) return;

  for(uint32_t i=0; i<bag->len; i++) {
    vittec__emit_diag_one_jsonl(out, sm, &bag->diags[i], 0u);
    vittec__j_putc(out, '\n');
  }
}

void vittec_emit_json(const vittec_source_map_t* sm, const vittec_diag_sink_t* sink) {
  vittec_emit_json_ex(sm, sink, NULL);
}

void vittec_emit_json_ex(const vittec_source_map_t* sm, const vittec_diag_sink_t* sink, const vittec_emit_options_t* opt) {
  if(!sink || !sink->diags || sink->len == 0) return;
  FILE* out = vittec__opt_out(opt);

  /* Stream minimal JSONL from legacy diagnostics. */
  for(uint32_t i=0; i<sink->len; i++) {
    const vittec_diagnostic_t* d0 = &sink->diags[i];
    vittec_sv_t path = vittec_source_map_file_path(sm, d0->span.file_id);
    vittec_line_col_t lc0 = vittec_source_map_line_col(sm, d0->span.file_id, d0->span.lo);

    vittec__j_putc(out, '{');
    vittec__j_write(out, "\"severity\":"); vittec__j_escape_cstr(out, vittec__sev_name(d0->severity));
    vittec__j_write(out, ",\"message\":"); vittec__j_escape_sv(out, d0->message);
    vittec__j_write(out, ",\"location\":{");
    vittec__j_write(out, "\"path\":"); vittec__j_escape_sv(out, path);
    vittec__j_write(out, ",\"file_id\":"); vittec__j_u32(out, d0->span.file_id);
    vittec__j_write(out, ",\"line\":"); vittec__j_u32(out, lc0.line);
    vittec__j_write(out, ",\"col\":"); vittec__j_u32(out, lc0.col);
    vittec__j_write(out, ",\"span\":{\"lo\":"); vittec__j_u32(out, d0->span.lo);
    vittec__j_write(out, ",\"hi\":"); vittec__j_u32(out, d0->span.hi);
    vittec__j_write(out, "}}}");
    vittec__j_putc(out, '\n');
  }
}
