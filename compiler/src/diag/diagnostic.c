#include "vittec/diag/diagnostic.h"
#include "vittec/support/assert.h"

#include <stdlib.h>
#include <string.h>

/* ========================================================================
 * Legacy minimal diagnostics sink (KEEP)
 * ======================================================================== */

void vittec_diag_sink_init(vittec_diag_sink_t* s) {
  if(!s) return;
  s->diags = NULL;
  s->len = 0;
  s->cap = 0;
}

void vittec_diag_sink_free(vittec_diag_sink_t* s) {
  if(!s) return;
  if(s->diags) free(s->diags);
  s->diags = NULL;
  s->len = 0;
  s->cap = 0;
}

void vittec_diag_push(vittec_diag_sink_t* s, vittec_severity_t sev, vittec_span_t span, vittec_sv_t msg) {
  if(!s) return;

  if(s->len == s->cap) {
    uint32_t new_cap = s->cap ? s->cap * 2u : 16u;
    vittec_diagnostic_t* p = (vittec_diagnostic_t*)realloc(s->diags, (size_t)new_cap * sizeof(vittec_diagnostic_t));
    VITTEC_ASSERT(p != NULL);
    s->diags = p;
    s->cap = new_cap;
  }

  s->diags[s->len].severity = sev;
  s->diags[s->len].span = span;
  s->diags[s->len].message = msg;
  s->len++;
}

/* ========================================================================
 * Structured diagnostics (bag)
 * ======================================================================== */

static void vittec__diag_zero(vittec_diag_t* d) {
  if(!d) return;
  memset(d, 0, sizeof(*d));
}

static void vittec__diag_free_internals(vittec_diag_t* d) {
  if(!d) return;
  /* note: strings are string-views; only arrays are owned */
  if(d->labels) free(d->labels);
  if(d->notes) free(d->notes);
  d->labels = NULL;
  d->labels_len = 0;
  d->labels_cap = 0;
  d->notes = NULL;
  d->notes_len = 0;
  d->notes_cap = 0;
}

static void vittec__bag_reserve(vittec_diag_bag_t* b, uint32_t add) {
  if(!b) return;
  uint32_t need = b->len + add;
  if(need <= b->cap) return;
  uint32_t new_cap = b->cap ? b->cap : 16u;
  while(new_cap < need) new_cap *= 2u;
  vittec_diag_t* p = (vittec_diag_t*)realloc(b->diags, (size_t)new_cap * sizeof(vittec_diag_t));
  VITTEC_ASSERT(p != NULL);
  b->diags = p;
  b->cap = new_cap;
}

static void vittec__labels_reserve(vittec_diag_t* d, uint32_t add) {
  if(!d) return;
  uint32_t need = d->labels_len + add;
  if(need <= d->labels_cap) return;
  uint32_t new_cap = d->labels_cap ? d->labels_cap : 4u;
  while(new_cap < need) new_cap *= 2u;
  vittec_diag_label_t* p = (vittec_diag_label_t*)realloc(d->labels, (size_t)new_cap * sizeof(vittec_diag_label_t));
  VITTEC_ASSERT(p != NULL);
  d->labels = p;
  d->labels_cap = new_cap;
}

static void vittec__notes_reserve(vittec_diag_t* d, uint32_t add) {
  if(!d) return;
  uint32_t need = d->notes_len + add;
  if(need <= d->notes_cap) return;
  uint32_t new_cap = d->notes_cap ? d->notes_cap : 4u;
  while(new_cap < need) new_cap *= 2u;
  vittec_sv_t* p = (vittec_sv_t*)realloc(d->notes, (size_t)new_cap * sizeof(vittec_sv_t));
  VITTEC_ASSERT(p != NULL);
  d->notes = p;
  d->notes_cap = new_cap;
}

/* Public: init/free bag */

void vittec_diag_bag_init(vittec_diag_bag_t* b) {
  if(!b) return;
  b->diags = NULL;
  b->len = 0;
  b->cap = 0;
  b->error_count = 0;
}

void vittec_diag_bag_free(vittec_diag_bag_t* b) {
  if(!b) return;
  if(b->diags) {
    for(uint32_t i = 0; i < b->len; i++) {
      vittec__diag_free_internals(&b->diags[i]);
    }
    free(b->diags);
  }
  b->diags = NULL;
  b->len = 0;
  b->cap = 0;
  b->error_count = 0;
}

void vittec_diag_bag_clear(vittec_diag_bag_t* b) {
  if(!b) return;
  if(b->diags) {
    for(uint32_t i = 0; i < b->len; i++) {
      vittec__diag_free_internals(&b->diags[i]);
    }
  }
  b->len = 0;
  b->error_count = 0;
}

int vittec_diag_bag_has_errors(const vittec_diag_bag_t* b) {
  return b && (b->error_count != 0u);
}

/* ------------------------------------------------------------------------
 * Structured diag constructors
 * ------------------------------------------------------------------------ */

static vittec_sv_t vittec__sv_empty(void) {
  vittec_sv_t s;
  s.data = NULL;
  s.len = 0;
  return s;
}

static vittec_diag_t vittec__diag_make(vittec_severity_t sev, vittec_span_t sp, vittec_sv_t code, vittec_sv_t msg) {
  vittec_diag_t d;
  vittec__diag_zero(&d);

  d.severity = sev;
  d.code = code;
  d.message = msg;
  d.help = vittec__sv_empty();

  /* Primary label by default. */
  vittec__labels_reserve(&d, 1u);
  d.labels[d.labels_len].kind = VITTEC_DIAG_LABEL_PRIMARY;
  d.labels[d.labels_len].span = sp;
  d.labels[d.labels_len].message = vittec__sv_empty();
  d.labels_len++;

  return d;
}

vittec_diag_t vittec_diag_error(vittec_span_t sp, vittec_sv_t code, vittec_sv_t msg) {
  return vittec__diag_make(VITTEC_SEV_ERROR, sp, code, msg);
}

vittec_diag_t vittec_diag_warning(vittec_span_t sp, vittec_sv_t code, vittec_sv_t msg) {
  return vittec__diag_make(VITTEC_SEV_WARNING, sp, code, msg);
}

vittec_diag_t vittec_diag_note(vittec_span_t sp, vittec_sv_t code, vittec_sv_t msg) {
  return vittec__diag_make(VITTEC_SEV_NOTE, sp, code, msg);
}

/* ------------------------------------------------------------------------
 * Structured diag mutators
 * ------------------------------------------------------------------------ */

void vittec_diag_set_help(vittec_diag_t* d, vittec_sv_t help) {
  if(!d) return;
  d->help = help;
}

void vittec_diag_add_label(vittec_diag_t* d, vittec_diag_label_kind_t kind, vittec_span_t sp, vittec_sv_t msg) {
  if(!d) return;
  vittec__labels_reserve(d, 1u);
  d->labels[d->labels_len].kind = kind;
  d->labels[d->labels_len].span = sp;
  d->labels[d->labels_len].message = msg;
  d->labels_len++;
}

void vittec_diag_add_note(vittec_diag_t* d, vittec_sv_t note) {
  if(!d) return;
  vittec__notes_reserve(d, 1u);
  d->notes[d->notes_len++] = note;
}

/* ------------------------------------------------------------------------
 * Bag push
 * ------------------------------------------------------------------------ */

void vittec_diag_bag_push(vittec_diag_bag_t* b, const vittec_diag_t* d) {
  if(!b || !d) return;

  vittec__bag_reserve(b, 1u);

  /* shallow copy of views, deep copy of arrays */
  vittec_diag_t* out = &b->diags[b->len];
  vittec__diag_zero(out);

  out->severity = d->severity;
  out->code = d->code;
  out->message = d->message;
  out->help = d->help;

  /* labels */
  if(d->labels_len) {
    out->labels = (vittec_diag_label_t*)malloc((size_t)d->labels_len * sizeof(vittec_diag_label_t));
    VITTEC_ASSERT(out->labels != NULL);
    memcpy(out->labels, d->labels, (size_t)d->labels_len * sizeof(vittec_diag_label_t));
    out->labels_len = d->labels_len;
    out->labels_cap = d->labels_len;
  }

  /* notes */
  if(d->notes_len) {
    out->notes = (vittec_sv_t*)malloc((size_t)d->notes_len * sizeof(vittec_sv_t));
    VITTEC_ASSERT(out->notes != NULL);
    memcpy(out->notes, d->notes, (size_t)d->notes_len * sizeof(vittec_sv_t));
    out->notes_len = d->notes_len;
    out->notes_cap = d->notes_len;
  }

  b->len++;
  if(d->severity == VITTEC_SEV_ERROR) b->error_count++;
}

/* Alias kept for callers who expect “add”. */
void vittec_diag_bag_add(vittec_diag_bag_t* b, const vittec_diag_t* d) {
  vittec_diag_bag_push(b, d);
}

/* Convenience: push a minimal error into both modern bag and legacy sink. */
void vittec_diag_report(
  vittec_diag_sink_t* sink,
  vittec_diag_bag_t* bag,
  vittec_severity_t sev,
  vittec_span_t sp,
  vittec_sv_t code,
  vittec_sv_t msg
) {
  if(sink) vittec_diag_push(sink, sev, sp, msg);
  if(bag) {
    vittec_diag_t d = vittec__diag_make(sev, sp, code, msg);
    vittec_diag_bag_push(bag, &d);
    vittec__diag_free_internals(&d);
  }
}
