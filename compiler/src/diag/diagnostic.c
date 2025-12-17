#include "vittec/diag/diagnostic.h"
#include <stdlib.h>
#include <string.h>

void vittec_diag_sink_init(vittec_diag_sink_t* s) {
  if (!s) return;
  s->diags = NULL;
  s->len = 0;
  s->cap = 0;
}

void vittec_diag_sink_free(vittec_diag_sink_t* s) {
  if (!s) return;
  free(s->diags);
  s->diags = NULL;
  s->len = 0;
  s->cap = 0;
}

static void vittec_diag_sink_reserve(vittec_diag_sink_t* s, uint32_t add) {
  if (!s) return;
  uint32_t need = s->len + add;
  if (need <= s->cap) return;
  uint32_t cap = s->cap ? s->cap * 2u : 4u;
  if (cap < need) cap = need;
  vittec_diagnostic_t* p = (vittec_diagnostic_t*)realloc(s->diags, (size_t)cap * sizeof(*p));
  if (!p) return;
  s->diags = p;
  s->cap = cap;
}

void vittec_diag_push(vittec_diag_sink_t* s, vittec_severity_t sev, vittec_span_t span, vittec_sv_t msg) {
  if (!s) return;
  vittec_diag_sink_reserve(s, 1u);
  if (s->len >= s->cap) return;
  vittec_diagnostic_t* d = &s->diags[s->len++];
  d->severity = sev;
  d->span = span;
  d->message = msg;
}

static void vittec_diag_labels_reserve(vittec_diag_t* d, uint32_t add) {
  if (!d) return;
  uint32_t need = d->labels_len + add;
  if (need <= d->labels_cap) return;
  uint32_t cap = d->labels_cap ? d->labels_cap * 2u : 4u;
  if (cap < need) cap = need;
  vittec_diag_label_t* p = (vittec_diag_label_t*)realloc(d->labels, (size_t)cap * sizeof(*p));
  if (!p) return;
  d->labels = p;
  d->labels_cap = cap;
}

static void vittec_diag_notes_reserve(vittec_diag_t* d, uint32_t add) {
  if (!d) return;
  uint32_t need = d->notes_len + add;
  if (need <= d->notes_cap) return;
  uint32_t cap = d->notes_cap ? d->notes_cap * 2u : 4u;
  if (cap < need) cap = need;
  vittec_sv_t* p = (vittec_sv_t*)realloc(d->notes, (size_t)cap * sizeof(*p));
  if (!p) return;
  d->notes = p;
  d->notes_cap = cap;
}

void vittec_diag_init(
  vittec_diag_t* d,
  vittec_severity_t sev,
  vittec_sv_t code,
  vittec_span_t primary_span,
  vittec_sv_t message
) {
  if (!d) return;
  memset(d, 0, sizeof(*d));
  d->severity = sev;
  d->code = code;
  d->message = message;
  vittec_diag_labels_reserve(d, 1u);
  if (d->labels_cap > 0) {
    d->labels[0].style = VITTEC_DIAG_LABEL_PRIMARY;
    d->labels[0].span = primary_span;
    d->labels[0].message = vittec_sv(NULL, 0);
    d->labels_len = 1u;
  }
}

void vittec_diag_free(vittec_diag_t* d) {
  if (!d) return;
  free(d->labels);
  free(d->notes);
  d->labels = NULL;
  d->notes = NULL;
  d->labels_len = d->labels_cap = 0;
  d->notes_len = d->notes_cap = 0;
  d->help = vittec_sv(NULL, 0);
}

int vittec_diag_add_label(vittec_diag_t* d, vittec_diag_label_style_t style, vittec_span_t span, vittec_sv_t msg) {
  if (!d) return 0;
  if (style == VITTEC_DIAG_LABEL_PRIMARY) {
    for (uint32_t i = 0; i < d->labels_len; i++) {
      if (d->labels[i].style == VITTEC_DIAG_LABEL_PRIMARY) return 0;
    }
  }
  vittec_diag_labels_reserve(d, 1u);
  if (d->labels_len >= d->labels_cap) return 0;
  vittec_diag_label_t* lab = &d->labels[d->labels_len++];
  lab->style = style;
  lab->span = span;
  lab->message = msg;
  return 1;
}

int vittec_diag_add_note(vittec_diag_t* d, vittec_sv_t note) {
  if (!d) return 0;
  vittec_diag_notes_reserve(d, 1u);
  if (d->notes_len >= d->notes_cap) return 0;
  d->notes[d->notes_len++] = note;
  return 1;
}

void vittec_diag_set_help(vittec_diag_t* d, vittec_sv_t help) {
  if (!d) return;
  d->help = help;
}

void vittec_diag_bag_init(vittec_diag_bag_t* b) {
  if (!b) return;
  memset(b, 0, sizeof(*b));
}

static void vittec_diag_bag_reserve(vittec_diag_bag_t* b, uint32_t add) {
  if (!b) return;
  uint32_t need = b->len + add;
  if (need <= b->cap) return;
  uint32_t cap = b->cap ? b->cap * 2u : 4u;
  if (cap < need) cap = need;
  vittec_diag_t* p = (vittec_diag_t*)realloc(b->diags, (size_t)cap * sizeof(*p));
  if (!p) return;
  b->diags = p;
  b->cap = cap;
}

void vittec_diag_bag_free(vittec_diag_bag_t* b) {
  if (!b) return;
  for (uint32_t i = 0; i < b->len; i++) {
    vittec_diag_free(&b->diags[i]);
  }
  free(b->diags);
  b->diags = NULL;
  b->len = b->cap = 0;
  b->errors = 0;
}

static void vittec_diag_copy(vittec_diag_t* dst, const vittec_diag_t* src) {
  memset(dst, 0, sizeof(*dst));
  if (!src) return;
  dst->severity = src->severity;
  dst->code = src->code;
  dst->message = src->message;
  dst->help = src->help;
  if (src->labels_len) {
    dst->labels = (vittec_diag_label_t*)malloc((size_t)src->labels_len * sizeof(*dst->labels));
    if (dst->labels) {
      memcpy(dst->labels, src->labels, (size_t)src->labels_len * sizeof(*dst->labels));
      dst->labels_len = src->labels_len;
      dst->labels_cap = src->labels_len;
    }
  }
  if (src->notes_len) {
    dst->notes = (vittec_sv_t*)malloc((size_t)src->notes_len * sizeof(*dst->notes));
    if (dst->notes) {
      memcpy(dst->notes, src->notes, (size_t)src->notes_len * sizeof(*dst->notes));
      dst->notes_len = src->notes_len;
      dst->notes_cap = src->notes_len;
    }
  }
}

int vittec_diag_bag_push(vittec_diag_bag_t* b, const vittec_diag_t* d) {
  if (!b || !d) return 0;
  vittec_diag_bag_reserve(b, 1u);
  if (b->len >= b->cap) return 0;
  vittec_diag_copy(&b->diags[b->len], d);
  if (b->diags[b->len].severity == VITTEC_SEV_ERROR) b->errors++;
  b->len++;
  return 1;
}

vittec_diag_t* vittec_diag_bag_push_new(
  vittec_diag_bag_t* b,
  vittec_severity_t sev,
  vittec_sv_t code,
  vittec_span_t primary_span,
  vittec_sv_t message
) {
  if (!b) return NULL;
  vittec_diag_bag_reserve(b, 1u);
  if (b->len >= b->cap) return NULL;
  vittec_diag_t* out = &b->diags[b->len++];
  vittec_diag_init(out, sev, code, primary_span, message);
  if (sev == VITTEC_SEV_ERROR) b->errors++;
  return out;
}
