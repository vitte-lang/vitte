#include "vittec/diag/emitter.h"
#include "vittec/diag/source_map.h"
#include "vittec/diag/diagnostic.h"
#include "vittec/diag/severity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static const char* vittec__label_kind_name(vittec_diag_label_kind_t k) {
  switch(k) {
    case VITTEC_DIAG_LABEL_PRIMARY: return "primary";
    case VITTEC_DIAG_LABEL_SECONDARY: return "secondary";
    default: return "unknown";
  }
}

/* ========================================================================
 * Legacy sink emitter (KEEP signature)
 * ======================================================================== */

void vittec_emit_json(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags) {
  /* Original behavior printed one big JSON object. Keep that shape, but:
     - escape strings correctly
     - include path/line/col when source_map is available
  */
  (void)sm;
  if(!diags) return;

  FILE* out = stderr;

  vittec__j_write(out, "{\n  \"diagnostics\": [\n");

  for(uint32_t i = 0; i < diags->len; i++) {
    const vittec_diagnostic_t* d = &diags->diags[i];

    /* location (best-effort) */
    vittec_sv_t path = (sm ? vittec_source_map_file_path(sm, d->span.file_id) : (vittec_sv_t){0});
    vittec_line_col_t lc = (sm ? vittec_source_map_line_col(sm, d->span.file_id, d->span.lo) : (vittec_line_col_t){1u,1u});

    vittec__j_write(out, "    { \"severity\": ");
    vittec__j_escape_cstr(out, vittec__sev_name(d->severity));

    vittec__j_write(out, ", \"message\": ");
    vittec__j_escape_sv(out, d->message);

    vittec__j_write(out, ", \"location\": { \"path\": ");
    vittec__j_escape_sv(out, path);
    vittec__j_write(out, ", \"file_id\": ");
    vittec__j_u32(out, d->span.file_id);
    vittec__j_write(out, ", \"line\": ");
    vittec__j_u32(out, lc.line);
    vittec__j_write(out, ", \"col\": ");
    vittec__j_u32(out, lc.col);

    vittec__j_write(out, ", \"span\": { \"lo\": ");
    vittec__j_u32(out, d->span.lo);
    vittec__j_write(out, ", \"hi\": ");
    vittec__j_u32(out, d->span.hi);
    vittec__j_write(out, " } }");

    vittec__j_write(out, " }");
    vittec__j_write(out, (i + 1u == diags->len) ? "\n" : ",\n");
  }

  vittec__j_write(out, "  ]\n}\n");
}

/* ========================================================================
 * Structured JSON emitters (JSON Lines)
 * ======================================================================== */

static vittec_span_t vittec__primary_span(const vittec_diag_t* d) {
  vittec_span_t sp;
  memset(&sp, 0, sizeof(sp));
  if(!d || !d->labels || d->labels_len == 0) return sp;
  for(uint32_t i=0; i<d->labels_len; i++) {
    if(d->labels[i].kind == VITTEC_DIAG_LABEL_PRIMARY) return d->labels[i].span;
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
      vittec__j_write(out, "\"kind\":"); vittec__j_escape_cstr(out, vittec__label_kind_name(lab->kind));
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
  vittec_emit_json_one_ex(sm, d, stderr, 2u);
}

void vittec_emit_json_one_ex(const vittec_source_map_t* sm, const vittec_diag_t* d, void* out_file, uint32_t context_lines) {
  FILE* out = (FILE*)out_file;
  vittec__emit_diag_one_jsonl(out ? out : stderr, sm, d, context_lines);
  vittec__j_putc(out ? out : stderr, '\n');
}

void vittec_emit_json_bag(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag) {
  vittec_emit_json_bag_ex(sm, bag, stderr, 2u);
}

void vittec_emit_json_bag_ex(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag, void* out_file, uint32_t context_lines) {
  FILE* out = (FILE*)out_file;
  if(!out) out = stderr;
  if(!bag || !bag->diags || bag->len == 0) return;

  for(uint32_t i=0; i<bag->len; i++) {
    vittec__emit_diag_one_jsonl(out, sm, &bag->diags[i], context_lines);
    vittec__j_putc(out, '\n');
  }
}

void vittec_emit_json_sink(const vittec_source_map_t* sm, const vittec_diag_sink_t* sink) {
  if(!sink || !sink->diags || sink->len == 0) return;

  /* Stream minimal JSONL from legacy diagnostics. */
  for(uint32_t i=0; i<sink->len; i++) {
    const vittec_diagnostic_t* d0 = &sink->diags[i];
    vittec_sv_t path = vittec_source_map_file_path(sm, d0->span.file_id);
    vittec_line_col_t lc0 = vittec_source_map_line_col(sm, d0->span.file_id, d0->span.lo);

    vittec__j_putc(stderr, '{');
    vittec__j_write(stderr, "\"severity\":"); vittec__j_escape_cstr(stderr, vittec__sev_name(d0->severity));
    vittec__j_write(stderr, ",\"message\":"); vittec__j_escape_sv(stderr, d0->message);
    vittec__j_write(stderr, ",\"location\":{");
    vittec__j_write(stderr, "\"path\":"); vittec__j_escape_sv(stderr, path);
    vittec__j_write(stderr, ",\"file_id\":"); vittec__j_u32(stderr, d0->span.file_id);
    vittec__j_write(stderr, ",\"line\":"); vittec__j_u32(stderr, lc0.line);
    vittec__j_write(stderr, ",\"col\":"); vittec__j_u32(stderr, lc0.col);
    vittec__j_write(stderr, ",\"span\":{\"lo\":"); vittec__j_u32(stderr, d0->span.lo);
    vittec__j_write(stderr, ",\"hi\":"); vittec__j_u32(stderr, d0->span.hi);
    vittec__j_write(stderr, "}}}");
    vittec__j_putc(stderr, '\n');
  }
}
