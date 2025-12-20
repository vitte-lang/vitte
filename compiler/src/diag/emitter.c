#include "vittec/diag/emitter.h"
#include "vittec/diag/diagnostic.h"
#include "vittec/diag/source_map.h"
#include "vittec/diag/severity.h"
#include "vittec/support/str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static vittec_sv_t vittec__sv_empty(void) {
  vittec_sv_t s;
  s.data = NULL;
  s.len = 0;
  return s;
}

static int vittec__sv_is_empty(vittec_sv_t s) {
  return !s.data || s.len == 0;
}

static void vittec__fwrite_sv(FILE* out, vittec_sv_t s) {
  if (!out) out = stderr;
  if (s.data && s.len) {
    (void)fwrite(s.data, 1, (size_t)s.len, out);
  }
}

static void vittec__json_write_escaped(FILE* out, vittec_sv_t s) {
  if (!out) out = stdout;
  fputc('"', out);
  if (s.data && s.len) {
    for (uint64_t i = 0; i < s.len; i++) {
      unsigned char c = (unsigned char)s.data[i];
      switch (c) {
        case '\"': fputs("\\\"", out); break;
        case '\\': fputs("\\\\", out); break;
        case '\b': fputs("\\b", out); break;
        case '\f': fputs("\\f", out); break;
        case '\n': fputs("\\n", out); break;
        case '\r': fputs("\\r", out); break;
        case '\t': fputs("\\t", out); break;
        default:
          if (c < 0x20) {
            fprintf(out, "\\u%04x", (unsigned)c);
          } else {
            fputc((int)c, out);
          }
          break;
      }
    }
  }
  fputc('"', out);
}

static uint32_t vittec__u32_digits(uint32_t v) {
  uint32_t d = 1;
  while (v >= 10u) {
    v /= 10u;
    d++;
  }
  return d;
}

static uint32_t vittec__u32_min(uint32_t a, uint32_t b) { return a < b ? a : b; }
static uint32_t vittec__u32_max(uint32_t a, uint32_t b) { return a > b ? a : b; }

static void vittec__print_n(FILE* out, char ch, uint32_t n) {
  if (!out) out = stderr;
  for (uint32_t i = 0; i < n; i++) fputc((int)ch, out);
}

static void vittec_emit_options_normalize(vittec_emit_options_t* tmp, const vittec_emit_options_t* opt) {
  vittec_emit_options_init(tmp);
  tmp->size = sizeof(*tmp);
  if (!opt) return;
  /* Treat size==0 as "same size as struct"; forward-compat: copy known fields. */
  (void)opt->size;
  tmp->use_color = opt->use_color;
  tmp->context_lines = opt->context_lines;
  tmp->show_line_numbers = opt->show_line_numbers;
  tmp->show_notes = opt->show_notes;
  tmp->show_help = opt->show_help;
  tmp->sort_by_location = opt->sort_by_location;
  tmp->json_pretty = opt->json_pretty;
  tmp->json_one_per_line = opt->json_one_per_line;
  tmp->out_stream = opt->out_stream;
}

void vittec_emit_options_init(vittec_emit_options_t* opt) {
  if (!opt) return;
  memset(opt, 0, sizeof(*opt));
  opt->size = sizeof(*opt);
  opt->use_color = -1;
  opt->context_lines = 1;
  opt->show_line_numbers = 1;
  opt->show_notes = 1;
  opt->show_help = 1;
  opt->sort_by_location = 1;
  opt->json_pretty = 0;
  opt->json_one_per_line = 1;
  opt->out_stream = NULL;
}

static const vittec_diag_label_t* vittec__primary_label(const vittec_diag_t* d) {
  if (!d || !d->labels || d->labels_len == 0) return NULL;
  for (uint32_t i = 0; i < d->labels_len; i++) {
    if (d->labels[i].style == VITTEC_DIAG_LABEL_PRIMARY) return &d->labels[i];
  }
  return &d->labels[0];
}

static void vittec__emit_human_one(FILE* out, const vittec_source_map_t* sm, const vittec_diag_t* d, const vittec_emit_options_t* opt) {
  if (!out) out = stderr;
  if (!d) return;

  const char* sev = vittec_severity_name(d->severity);
  if (!vittec__sv_is_empty(d->code)) {
    fputs(sev, out);
    fputc('[', out);
    vittec__fwrite_sv(out, d->code);
    fputs("]: ", out);
    vittec__fwrite_sv(out, d->message);
    fputc('\n', out);
  } else {
    fputs(sev, out);
    fputs(": ", out);
    vittec__fwrite_sv(out, d->message);
    fputc('\n', out);
  }

  const vittec_diag_label_t* primary = vittec__primary_label(d);
  if (!primary || !sm) return;

  vittec_span_t sp = primary->span;
  vittec_snippet_t sn;
  uint32_t ctx = (opt && opt->context_lines >= 0) ? (uint32_t)opt->context_lines : 1u;
  if (!vittec_source_map_build_snippet(sm, sp, ctx, &sn)) return;

  vittec_sv_t path = vittec_source_map_file_path(sm, sp.file);
  char path_buf[256];
  if (path.data && path.len < sizeof(path_buf)) {
    memcpy(path_buf, path.data, (size_t)path.len);
    path_buf[path.len] = '\0';
  } else {
    snprintf(path_buf, sizeof(path_buf), "%s", path.data ? "(path-too-long)" : "(unknown)");
  }

  fprintf(out, "  --> %s:%u:%u\n", path_buf, (unsigned)sn.primary.line, (unsigned)sn.primary.col);

  const int show_lines = !opt || opt->show_line_numbers != 0;
  uint32_t line_no_width = 1;
  if (show_lines) {
    uint32_t last_line_no = sn.line_hi; /* 1-based display => index+1, but hi is count; this is safe upper bound */
    line_no_width = vittec__u32_digits(last_line_no);
  }

  /* Separator line like rustc: " |" */
  if (show_lines) {
    vittec__print_n(out, ' ', line_no_width);
    fputs(" |\n", out);
  } else {
    fputs(" |\n", out);
  }

  for (uint32_t li = sn.line_lo; li < sn.line_hi; li++) {
    vittec_span_t line_sp;
    if (!vittec_source_map_line_span(sm, sp.file, li, &line_sp)) continue;
    vittec_sv_t line_txt = vittec_source_map_line_text(sm, sp.file, li);

    if (show_lines) fprintf(out, "%*u | ", (int)line_no_width, (unsigned)(li + 1u));
    else fputs("| ", out);
    vittec__fwrite_sv(out, line_txt);
    fputc('\n', out);

    /* underline only on primary line */
    uint32_t primary_idx0 = sn.primary.line ? (sn.primary.line - 1u) : li;
    if (li != primary_idx0) continue;

    uint32_t underline_lo = vittec__u32_max(sp.lo, line_sp.lo);
    uint32_t underline_hi = vittec__u32_min(sp.hi, line_sp.hi);
    uint32_t width = (underline_hi > underline_lo) ? (underline_hi - underline_lo) : 1u;
    uint32_t col0 = (underline_lo > line_sp.lo) ? (underline_lo - line_sp.lo) : 0u;

    if (show_lines) {
      vittec__print_n(out, ' ', line_no_width);
      fputs(" | ", out);
    } else {
      fputs("| ", out);
    }

    vittec__print_n(out, ' ', col0);
    vittec__print_n(out, '^', width);
    if (primary->message.data && primary->message.len) {
      fputc(' ', out);
      vittec__fwrite_sv(out, primary->message);
    }
    fputc('\n', out);
  }

  /* Secondary labels: emit as rustc-like location notes. */
  if (d->labels && d->labels_len) {
    for (uint32_t i = 0; i < d->labels_len; i++) {
      const vittec_diag_label_t* lab = &d->labels[i];
      if (lab->style != VITTEC_DIAG_LABEL_SECONDARY) continue;
      if (!sm) continue;
      vittec_sv_t lab_path = vittec_source_map_file_path(sm, lab->span.file);
      char lab_path_buf[256];
      if (lab_path.data && lab_path.len < sizeof(lab_path_buf)) {
        memcpy(lab_path_buf, lab_path.data, (size_t)lab_path.len);
        lab_path_buf[lab_path.len] = '\0';
      } else {
        snprintf(lab_path_buf, sizeof(lab_path_buf), "%s", lab_path.data ? "(path-too-long)" : "(unknown)");
      }
      vittec_line_col_t llc = vittec_source_map_line_col(sm, lab->span.file, lab->span.lo);
      fprintf(out, "  = note: %s:%u:%u", lab_path_buf, (unsigned)llc.line, (unsigned)llc.col);
      if (lab->message.data && lab->message.len) {
        fputs(": ", out);
        vittec__fwrite_sv(out, lab->message);
      }
      fputc('\n', out);
    }
  }

  if (opt ? opt->show_help : 1) {
    if (!vittec__sv_is_empty(d->help)) {
      fputs("help: ", out);
      vittec__fwrite_sv(out, d->help);
      fputc('\n', out);
    }
  }

  if (opt ? opt->show_notes : 1) {
    if (d->notes && d->notes_len) {
      for (uint32_t i = 0; i < d->notes_len; i++) {
        fputs("note: ", out);
        vittec__fwrite_sv(out, d->notes[i]);
        fputc('\n', out);
      }
    }
  }
}

typedef struct vittec__diag_index {
  uint32_t i;
} vittec__diag_index_t;

static int vittec__cmp_diag_index_bag(const void* a, const void* b, void* ctx) {
  const vittec__diag_index_t* ia = (const vittec__diag_index_t*)a;
  const vittec__diag_index_t* ib = (const vittec__diag_index_t*)b;
  const vittec_diag_bag_t* bag = (const vittec_diag_bag_t*)ctx;
  const vittec_diag_t* da = &bag->diags[ia->i];
  const vittec_diag_t* db = &bag->diags[ib->i];
  const vittec_diag_label_t* la = vittec__primary_label(da);
  const vittec_diag_label_t* lb = vittec__primary_label(db);
  vittec_span_t sa = la ? la->span : vittec_span(0, 0, 0);
  vittec_span_t sb = lb ? lb->span : vittec_span(0, 0, 0);
  if (sa.file != sb.file) return (sa.file < sb.file) ? -1 : 1;
  if (sa.lo != sb.lo) return (sa.lo < sb.lo) ? -1 : 1;
  int ra = vittec_severity_rank(da->severity);
  int rb = vittec_severity_rank(db->severity);
  if (ra != rb) return (ra < rb) ? -1 : 1;
  /* stable: original index */
  if (ia->i != ib->i) return (ia->i < ib->i) ? -1 : 1;
  return 0;
}

static void vittec__stable_sort_bag(vittec__diag_index_t* idx, uint32_t n, const vittec_diag_bag_t* bag) {
  /* Portable, stable insertion sort (diag lists are expected to be small). */
  for (uint32_t i = 1; i < n; i++) {
    vittec__diag_index_t key = idx[i];
    uint32_t j = i;
    while (j > 0) {
      int cmp = vittec__cmp_diag_index_bag(&key, &idx[j - 1u], (void*)bag);
      if (cmp >= 0) break;
      idx[j] = idx[j - 1u];
      j--;
    }
    idx[j] = key;
  }
}

static void vittec__emit_json_diag(FILE* out, const vittec_source_map_t* sm, const vittec_diag_t* d) {
  if (!out) out = stdout;
  if (!d) return;

  const vittec_diag_label_t* primary = vittec__primary_label(d);
  vittec_span_t sp = primary ? primary->span : vittec_span(0, 0, 0);
  vittec_sv_t path = sm ? vittec_source_map_file_path(sm, sp.file) : vittec__sv_empty();
  vittec_line_col_t lc = sm ? vittec_source_map_line_col(sm, sp.file, sp.lo) : (vittec_line_col_t){0u, 0u};

  fputc('{', out);
  fputs("\"severity\":", out);
  vittec__json_write_escaped(out, vittec_sv(vittec_severity_name(d->severity), (uint64_t)strlen(vittec_severity_name(d->severity))));
  fputs(",\"code\":", out);
  vittec__json_write_escaped(out, d->code);
  fputs(",\"message\":", out);
  vittec__json_write_escaped(out, d->message);
  fputs(",\"file\":", out);
  vittec__json_write_escaped(out, path);
  fprintf(out, ",\"line\":%u,\"col\":%u", (unsigned)lc.line, (unsigned)lc.col);
  fprintf(out, ",\"span\":{\"lo\":%u,\"hi\":%u}", (unsigned)sp.lo, (unsigned)sp.hi);

  fputs(",\"labels\":[", out);
  if (d->labels && d->labels_len) {
    for (uint32_t i = 0; i < d->labels_len; i++) {
      const vittec_diag_label_t* lab = &d->labels[i];
      if (i) fputc(',', out);
      fputc('{', out);
      fputs("\"style\":", out);
      vittec__json_write_escaped(out, vittec_sv(
        lab->style == VITTEC_DIAG_LABEL_PRIMARY ? "primary" : "secondary",
        (uint64_t)strlen(lab->style == VITTEC_DIAG_LABEL_PRIMARY ? "primary" : "secondary")));
      fprintf(out, ",\"span\":{\"lo\":%u,\"hi\":%u}", (unsigned)lab->span.lo, (unsigned)lab->span.hi);
      fputs(",\"message\":", out);
      vittec__json_write_escaped(out, lab->message);
      fputc('}', out);
    }
  }
  fputc(']', out);

  fputs(",\"help\":", out);
  vittec__json_write_escaped(out, d->help);

  fputs(",\"notes\":[", out);
  if (d->notes && d->notes_len) {
    for (uint32_t i = 0; i < d->notes_len; i++) {
      if (i) fputc(',', out);
      vittec__json_write_escaped(out, d->notes[i]);
    }
  }
  fputc(']', out);

  fputc('}', out);
}

void vittec_emit_human(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags) {
  vittec_emit_human_ex(sm, diags, NULL);
}

void vittec_emit_json(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags) {
  vittec_emit_json_ex(sm, diags, NULL);
}

void vittec_emit_human_ex(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags, const vittec_emit_options_t* opt) {
  vittec_emit_options_t o;
  vittec_emit_options_normalize(&o, opt);
  FILE* out = (FILE*)(o.out_stream ? o.out_stream : stderr);

  if (!diags || !diags->diags || diags->len == 0) return;
  for (uint32_t i = 0; i < diags->len; i++) {
    const vittec_diagnostic_t* d0 = &diags->diags[i];
    vittec_diag_t d;
    memset(&d, 0, sizeof(d));
    d.severity = d0->severity;
    d.code = vittec__sv_empty();
    d.message = d0->message;
    d.help = vittec__sv_empty();

    vittec_diag_init(&d, d0->severity, vittec__sv_empty(), d0->span, d0->message);

    vittec__emit_human_one(out, sm, &d, &o);
    vittec_diag_free(&d);

    if (i + 1u < diags->len) fputc('\n', out);
  }
}

void vittec_emit_json_ex(const vittec_source_map_t* sm, const vittec_diag_sink_t* diags, const vittec_emit_options_t* opt) {
  vittec_emit_options_t o;
  vittec_emit_options_normalize(&o, opt);
  FILE* out = (FILE*)(o.out_stream ? o.out_stream : stdout);

  if (!diags || !diags->diags || diags->len == 0) return;

  if (!o.json_one_per_line) fputs("[", out);
  for (uint32_t i = 0; i < diags->len; i++) {
    const vittec_diagnostic_t* d0 = &diags->diags[i];
    vittec_diag_t d;
    memset(&d, 0, sizeof(d));
    vittec_diag_init(&d, d0->severity, vittec__sv_empty(), d0->span, d0->message);

    if (!o.json_one_per_line) {
      if (i) fputs(",", out);
      if (o.json_pretty) fputs("\n  ", out);
    }
    vittec__emit_json_diag(out, sm, &d);
    if (o.json_one_per_line) fputc('\n', out);
    vittec_diag_free(&d);
  }
  if (!o.json_one_per_line) {
    if (o.json_pretty) fputs("\n", out);
    fputs("]\n", out);
  }
}

void vittec_emit_human_bag(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag) {
  vittec_emit_human_bag_ex(sm, bag, NULL);
}

void vittec_emit_json_bag(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag) {
  vittec_emit_json_bag_ex(sm, bag, NULL);
}

void vittec_emit_human_bag_ex(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag, const vittec_emit_options_t* opt) {
  vittec_emit_options_t o;
  vittec_emit_options_normalize(&o, opt);
  FILE* out = (FILE*)(o.out_stream ? o.out_stream : stderr);

  if (!bag || !bag->diags || bag->len == 0) return;

  vittec__diag_index_t* idx = NULL;
  if (o.sort_by_location) {
    idx = (vittec__diag_index_t*)malloc((size_t)bag->len * sizeof(*idx));
    if (idx) {
      for (uint32_t i = 0; i < bag->len; i++) idx[i].i = i;
      vittec__stable_sort_bag(idx, bag->len, bag);
    }
  }

  for (uint32_t k = 0; k < bag->len; k++) {
    uint32_t i = idx ? idx[k].i : k;
    vittec__emit_human_one(out, sm, &bag->diags[i], &o);
    if (k + 1u < bag->len) fputc('\n', out);
  }

  free(idx);
}

void vittec_emit_json_bag_ex(const vittec_source_map_t* sm, const vittec_diag_bag_t* bag, const vittec_emit_options_t* opt) {
  vittec_emit_options_t o;
  vittec_emit_options_normalize(&o, opt);
  FILE* out = (FILE*)(o.out_stream ? o.out_stream : stdout);

  if (!bag || !bag->diags || bag->len == 0) return;

  vittec__diag_index_t* idx = NULL;
  if (o.sort_by_location) {
    idx = (vittec__diag_index_t*)malloc((size_t)bag->len * sizeof(*idx));
    if (idx) {
      for (uint32_t i = 0; i < bag->len; i++) idx[i].i = i;
      vittec__stable_sort_bag(idx, bag->len, bag);
    }
  }

  if (!o.json_one_per_line) fputs("[", out);
  for (uint32_t k = 0; k < bag->len; k++) {
    uint32_t i = idx ? idx[k].i : k;
    if (!o.json_one_per_line) {
      if (k) fputs(",", out);
      if (o.json_pretty) fputs("\n  ", out);
    }
    vittec__emit_json_diag(out, sm, &bag->diags[i]);
    if (o.json_one_per_line) fputc('\n', out);
  }
  if (!o.json_one_per_line) {
    if (o.json_pretty) fputs("\n", out);
    fputs("]\n", out);
  }

  free(idx);
}
