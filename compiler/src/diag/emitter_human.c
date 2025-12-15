

#include "vittec/diag/emitter.h"
#include "vittec/diag/source_map.h"
#include "vittec/diag/diagnostic.h"
#include "vittec/diag/severity.h"
#include "vittec/support/str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * Small helpers (no heavy deps)
 * ======================================================================== */

static const char* vittec__cstr_or(const char* s, const char* fallback) {
  return (s && s[0]) ? s : fallback;
}

static void vittec__fwrite_sv(FILE* out, vittec_sv_t s) {
  if(!out) out = stderr;
  if(s.data && s.len) {
    fwrite(s.data, 1, s.len, out);
  }
}

static void vittec__print_n(FILE* out, char ch, uint32_t n) {
  if(!out) out = stderr;
  for(uint32_t i=0; i<n; i++) fputc((int)ch, out);
}

static uint32_t vittec__u32_min(uint32_t a, uint32_t b) { return a < b ? a : b; }
static uint32_t vittec__u32_max(uint32_t a, uint32_t b) { return a > b ? a : b; }

static vittec_sv_t vittec__sv_empty(void) {
  vittec_sv_t s; s.data = NULL; s.len = 0; return s;
}

static int vittec__sv_is_empty(vittec_sv_t s) {
  return !s.data || s.len == 0;
}

static void vittec__sv_to_cstr(vittec_sv_t sv, char* out, size_t cap) {
  if(!out || cap == 0) return;
  size_t n = sv.len;
  if(n >= cap) n = cap - 1;
  if(sv.data && n) memcpy(out, sv.data, n);
  out[n] = 0;
}

static const char* vittec__severity_name(vittec_severity_t sev) {
  /* Prefer helper from severity.h if present; keep local fallback. */
#ifdef VITTEC_SEVERITY_API_VERSION
  return vittec_severity_name(sev);
#else
  switch(sev) {
    case VITTEC_SEV_ERROR: return "error";
    case VITTEC_SEV_WARNING: return "warning";
    case VITTEC_SEV_NOTE: return "note";
    case VITTEC_SEV_HELP: return "help";
    default: return "unknown";
  }
#endif
}

/* byte-based “display column”; for unicode, plug a grapheme counter later */
static uint32_t vittec__span_width_on_line(uint32_t line_lo, uint32_t line_hi, uint32_t sp_lo, uint32_t sp_hi) {
  if(sp_hi < sp_lo) sp_hi = sp_lo;
  if(sp_lo < line_lo) sp_lo = line_lo;
  if(sp_hi > line_hi) sp_hi = line_hi;
  if(sp_hi <= sp_lo) return 1u;
  return sp_hi - sp_lo;
}

static uint32_t vittec__span_col0(uint32_t line_lo, uint32_t sp_lo) {
  if(sp_lo <= line_lo) return 0u;
  return sp_lo - line_lo;
}

/* ========================================================================
 * Core rendering
 * ======================================================================== */

static void vittec__emit_one(FILE* out, const vittec_source_map_t* sm, const vittec_diag_t* d, uint32_t context_lines) {
  if(!out) out = stderr;
  if(!d) return;

  /* Determine primary span: first primary label if any; else first label; else empty. */
  vittec_span_t primary;
  memset(&primary, 0, sizeof(primary));
  int has_primary = 0;

  if(d->labels && d->labels_len) {
    for(uint32_t i=0; i<d->labels_len; i++) {
      if(d->labels[i].kind == VITTEC_DIAG_LABEL_PRIMARY) {
        primary = d->labels[i].span;
        has_primary = 1;
        break;
      }
    }
    if(!has_primary) {
      primary = d->labels[0].span;
      has_primary = 1;
    }
  }

  vittec_sv_t path = vittec_source_map_file_path(sm, primary.file);
  vittec_line_col_t lc = vittec_source_map_line_col(sm, primary.file, primary.lo);

  char path_c[512];
  vittec__sv_to_cstr(path, path_c, sizeof(path_c));

  /* Header: path:line:col: severity[code]: message */
  fprintf(out, "%s:%u:%u: %s",
    vittec__cstr_or(path_c, "<input>"),
    (unsigned)lc.line,
    (unsigned)lc.col,
    vittec__severity_name(d->severity));

  if(!vittec__sv_is_empty(d->code)) {
    fputc('[', out);
    vittec__fwrite_sv(out, d->code);
    fputc(']', out);
  }

  fputs(": ", out);
  vittec__fwrite_sv(out, d->message);
  fputc('\n', out);

  /* If no source map or invalid file, still print notes/help. */
  const vittec_source_file_t* sf = vittec_source_map_get_file(sm, primary.file);
  if(!sf || !sf->data) {
    /* Notes/help */
    if(!vittec__sv_is_empty(d->help)) {
      fputs("help: ", out);
      vittec__fwrite_sv(out, d->help);
      fputc('\n', out);
    }
    if(d->notes && d->notes_len) {
      for(uint32_t i=0; i<d->notes_len; i++) {
        fputs("note: ", out);
        vittec__fwrite_sv(out, d->notes[i]);
        fputc('\n', out);
      }
    }
    return;
  }

  /* Snippet window around primary span */
  vittec_snippet_t sn;
  if(!vittec_source_map_build_snippet(sm, primary, context_lines, &sn)) {
    /* fallback: still print notes/help */
    if(!vittec__sv_is_empty(d->help)) {
      fputs("help: ", out);
      vittec__fwrite_sv(out, d->help);
      fputc('\n', out);
    }
    if(d->notes && d->notes_len) {
      for(uint32_t i=0; i<d->notes_len; i++) {
        fputs("note: ", out);
        vittec__fwrite_sv(out, d->notes[i]);
        fputc('\n', out);
      }
    }
    return;
  }

  /* Print a “--> path:line:col” line, rustc-style. */
  fprintf(out, "--> %s:%u:%u\n",
    vittec__cstr_or(path_c, "<input>"),
    (unsigned)sn.primary.line,
    (unsigned)sn.primary.col);

  /* Determine line number width for alignment. */
  uint32_t max_line = (sn.line_hi > 0) ? (sn.line_hi) : 1u;
  uint32_t width = 1u;
  while(max_line >= 10u) { width++; max_line /= 10u; }

  /* Render each line in [line_lo, line_hi). */
  for(uint32_t li = sn.line_lo; li < sn.line_hi; li++) {
    vittec_span_t lsp;
    if(!vittec_source_map_line_span(sm, primary.file, li, &lsp)) continue;
    vittec_sv_t ltxt = vittec_source_map_line_text(sm, primary.file, li);

    /* “  n | <text>” */
    fprintf(out, "%*u | ", (int)width, (unsigned)(li + 1u));
    vittec__fwrite_sv(out, ltxt);
    fputc('\n', out);

    /* Underline if this is the primary line. */
    if(primary.lo >= lsp.lo && primary.lo <= lsp.hi) {
      uint32_t col0 = vittec__span_col0(lsp.lo, primary.lo);
      uint32_t w = vittec__span_width_on_line(lsp.lo, lsp.hi, primary.lo, primary.hi);

      /* “    |   ^^^” */
      fprintf(out, "%*s | ", (int)width, "");
      vittec__print_n(out, ' ', col0);
      vittec__print_n(out, '^', w);

      /* label message (primary label text if provided) */
      if(d->labels && d->labels_len) {
        for(uint32_t k=0; k<d->labels_len; k++) {
          if(d->labels[k].kind == VITTEC_DIAG_LABEL_PRIMARY) {
            if(!vittec__sv_is_empty(d->labels[k].message)) {
              fputc(' ', out);
              vittec__fwrite_sv(out, d->labels[k].message);
            }
            break;
          }
        }
      }
      fputc('\n', out);
    }

    /* Secondary labels on this line (render as “-” underlines). */
    if(d->labels && d->labels_len) {
      for(uint32_t k=0; k<d->labels_len; k++) {
        const vittec_diag_label_t* lab = &d->labels[k];
        if(lab->kind != VITTEC_DIAG_LABEL_SECONDARY) continue;
        if(lab->span.file != primary.file) continue;
        if(!(lab->span.lo < lsp.hi && lsp.lo < lab->span.hi)) continue;

        uint32_t lo = vittec__u32_max(lab->span.lo, lsp.lo);
        uint32_t hi = vittec__u32_min(lab->span.hi, lsp.hi);

        uint32_t col0 = vittec__span_col0(lsp.lo, lo);
        uint32_t w = vittec__span_width_on_line(lsp.lo, lsp.hi, lo, hi);

        fprintf(out, "%*s | ", (int)width, "");
        vittec__print_n(out, ' ', col0);
        vittec__print_n(out, '-', w);
        if(!vittec__sv_is_empty(lab->message)) {
          fputc(' ', out);
          vittec__fwrite_sv(out, lab->message);
        }
        fputc('\n', out);
      }
    }
  }

  /* help */
  if(!vittec__sv_is_empty(d->help)) {
    fputs("help: ", out);
    vittec__fwrite_sv(out, d->help);
    fputc('\n', out);
  }

  /* notes */
  if(d->notes && d->notes_len) {
    for(uint32_t i=0; i<d->notes_len; i++) {
      fputs("note: ", out);
      vittec__fwrite_sv(out, d->notes[i]);
      fputc('\n', out);
    }
  }
}

/* ========================================================================
 * Public API
 * ======================================================================== */

void vittec_emit_human_one(const vittec_source_map_t* sm, const vittec_diag_t* d) {
  vittec_emit_human_one_ex(sm, d, stderr, 2u);
}

void vittec_emit_human_one_ex(const vittec_source_map_t* sm, const vittec_diag_t* d, void* out_file, uint32_t context_lines) {
  FILE* out = (FILE*)out_file;
  vittec__emit_one(out ? out : stderr, sm, d, context_lines);
}

void vittec_emit_human_bag(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag) {
  vittec_emit_human_bag_ex(sm, bag, stderr, 2u);
}

void vittec_emit_human_bag_ex(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag, void* out_file, uint32_t context_lines) {
  FILE* out = (FILE*)out_file;
  if(!out) out = stderr;
  if(!bag || !bag->diags || bag->len == 0) return;

  for(uint32_t i=0; i<bag->len; i++) {
    vittec__emit_one(out, sm, &bag->diags[i], context_lines);
    if(i + 1u < bag->len) fputc('\n', out);
  }
}

/* Legacy sink emitter (minimal): prints severity + message + primary span. */
void vittec_emit_human_sink(const vittec_source_map_t* sm, const vittec_diag_sink_t* sink) {
  if(!sink || !sink->diags || sink->len == 0) return;
  for(uint32_t i=0; i<sink->len; i++) {
    vittec_diagnostic_t d0 = sink->diags[i];
    vittec_diag_t d;
    memset(&d, 0, sizeof(d));
    d.severity = d0.severity;
    d.code = vittec__sv_empty();
    d.message = d0.message;
    d.help = vittec__sv_empty();

    /* One primary label. */
    d.labels = (vittec_diag_label_t*)malloc(sizeof(vittec_diag_label_t));
    if(d.labels) {
      d.labels_len = 1;
      d.labels_cap = 1;
      d.labels[0].kind = VITTEC_DIAG_LABEL_PRIMARY;
      d.labels[0].span = d0.span;
      d.labels[0].message = vittec__sv_empty();
    }

    vittec__emit_one(stderr, sm, &d, 2u);

    if(d.labels) free(d.labels);
    if(i + 1u < sink->len) fputc('\n', stderr);
  }
}