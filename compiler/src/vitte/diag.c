#include "vitte/diag.h"

#include <stdlib.h>
#include <string.h>

static const char* vitte__severity_name(vitte_severity s) {
    switch (s) {
        case VITTE_SEV_ERROR: return "error";
        case VITTE_SEV_WARNING: return "warning";
        case VITTE_SEV_NOTE: return "note";
        default: return "error";
    }
}

static int vitte__severity_rank(vitte_severity s) {
    switch (s) {
        case VITTE_SEV_ERROR: return 0;
        case VITTE_SEV_WARNING: return 1;
        case VITTE_SEV_NOTE: return 2;
        default: return 3;
    }
}

static char* vitte__strdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char* out = (char*)malloc(n + 1u);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static void* vitte__grow(void* buf, uint32_t* cap, uint32_t need, size_t elem_size) {
    if (need <= *cap) return buf;
    uint32_t new_cap = (*cap == 0) ? 4u : (*cap * 2u);
    while (new_cap < need) new_cap *= 2u;
    void* nb = realloc(buf, (size_t)new_cap * elem_size);
    if (!nb) return NULL;
    *cap = new_cap;
    return nb;
}

void vitte_emit_options_init(vitte_emit_options* opt) {
    if (!opt) return;
    memset(opt, 0, sizeof(*opt));
    opt->context_lines = 1;
    opt->show_line_numbers = 1;
    opt->show_notes = 1;
    opt->show_help = 1;
    opt->sort_by_location = 1;
    opt->json_one_per_line = 1;
    opt->json_pretty = 0;
}

void vitte_diag_bag_init(vitte_diag_bag* b) {
    if (!b) return;
    memset(b, 0, sizeof(*b));
}

static void vitte__diag_free(vitte_diag* d) {
    if (!d) return;
    free(d->message);
    for (uint32_t i = 0; i < d->labels_len; i++) {
        free(d->labels[i].message);
    }
    free(d->labels);
    for (uint32_t i = 0; i < d->notes_len; i++) {
        free(d->notes[i]);
    }
    free(d->notes);
    free(d->help);
    memset(d, 0, sizeof(*d));
}

void vitte_diag_bag_free(vitte_diag_bag* b) {
    if (!b) return;
    for (uint32_t i = 0; i < b->len; i++) {
        vitte__diag_free(&b->diags[i]);
    }
    free(b->diags);
    memset(b, 0, sizeof(*b));
}

int vitte_diag_bag_has_errors(const vitte_diag_bag* b) {
    return b && b->errors != 0;
}

static vitte_diag_label* vitte__diag_primary_label(vitte_diag* d) {
    if (!d || !d->labels || d->labels_len == 0) return NULL;
    for (uint32_t i = 0; i < d->labels_len; i++) {
        if (d->labels[i].style == VITTE_DIAG_LABEL_PRIMARY) return &d->labels[i];
    }
    return &d->labels[0];
}

int vitte_diag_add_label(vitte_diag* d, vitte_diag_label_style style, vitte_span span, const char* message) {
    if (!d) return 0;
    if (style == VITTE_DIAG_LABEL_PRIMARY) {
        vitte_diag_label* existing = vitte__diag_primary_label(d);
        if (existing && existing->style == VITTE_DIAG_LABEL_PRIMARY) return 0;
    }
    uint32_t need = d->labels_len + 1u;
    void* nb = vitte__grow(d->labels, &d->labels_cap, need, sizeof(*d->labels));
    if (!nb) return 0;
    d->labels = (vitte_diag_label*)nb;
    vitte_diag_label* lab = &d->labels[d->labels_len++];
    memset(lab, 0, sizeof(*lab));
    lab->style = style;
    lab->span = span;
    lab->message = message ? vitte__strdup(message) : NULL;
    if (message && !lab->message) return 0;
    return 1;
}

int vitte_diag_add_note(vitte_diag* d, const char* note) {
    if (!d) return 0;
    if (!note) note = "";
    uint32_t need = d->notes_len + 1u;
    void* nb = vitte__grow(d->notes, &d->notes_cap, need, sizeof(*d->notes));
    if (!nb) return 0;
    d->notes = (char**)nb;
    char* s = vitte__strdup(note);
    if (!s) return 0;
    d->notes[d->notes_len++] = s;
    return 1;
}

void vitte_diag_set_help(vitte_diag* d, const char* help) {
    if (!d) return;
    free(d->help);
    d->help = help ? vitte__strdup(help) : NULL;
}

vitte_diag* vitte_diag_bag_push(
    vitte_diag_bag* b,
    vitte_severity severity,
    const char* code,
    vitte_span primary_span,
    const char* message
) {
    if (!b || !message) return NULL;
    uint32_t need = b->len + 1u;
    void* nb = vitte__grow(b->diags, &b->cap, need, sizeof(*b->diags));
    if (!nb) return NULL;
    b->diags = (vitte_diag*)nb;
    vitte_diag* d = &b->diags[b->len++];
    memset(d, 0, sizeof(*d));
    d->severity = severity;
    d->message = vitte__strdup(message);
    if (!d->message) return NULL;
    memset(d->code, 0, sizeof(d->code));
    if (code && code[0]) {
        strncpy(d->code, code, sizeof(d->code) - 1u);
        d->code[sizeof(d->code) - 1u] = '\0';
    }
    if (!vitte_diag_add_label(d, VITTE_DIAG_LABEL_PRIMARY, primary_span, NULL)) return NULL;
    if (severity == VITTE_SEV_ERROR) b->errors++;
    return d;
}

typedef struct {
    uint32_t i;
} vitte__diag_index;

static int vitte__cmp_diag_index(const vitte__diag_index* ia, const vitte__diag_index* ib, const vitte_diag_bag* bag) {
    const vitte_diag* da = &bag->diags[ia->i];
    const vitte_diag* db = &bag->diags[ib->i];
    const vitte_diag_label* la = vitte__diag_primary_label((vitte_diag*)da);
    const vitte_diag_label* lb = vitte__diag_primary_label((vitte_diag*)db);
    vitte_span sa = la ? la->span : vitte_span_make(0u, 0u, 0u);
    vitte_span sb = lb ? lb->span : vitte_span_make(0u, 0u, 0u);
    if (sa.file_id != sb.file_id) return (sa.file_id < sb.file_id) ? -1 : 1;
    if (sa.lo != sb.lo) return (sa.lo < sb.lo) ? -1 : 1;
    if (sa.hi != sb.hi) return (sa.hi < sb.hi) ? -1 : 1;
    int ra = vitte__severity_rank(da->severity);
    int rb = vitte__severity_rank(db->severity);
    if (ra != rb) return (ra < rb) ? -1 : 1;
    int c = strcmp(da->code, db->code);
    if (c != 0) return c;
    if (ia->i != ib->i) return (ia->i < ib->i) ? -1 : 1;
    return 0;
}

void vitte_diag_bag_sort_by_location(vitte_diag_bag* b) {
    if (!b || !b->diags || b->len < 2u) return;
    vitte__diag_index* idx = (vitte__diag_index*)malloc((size_t)b->len * sizeof(*idx));
    if (!idx) return;
    for (uint32_t i = 0; i < b->len; i++) idx[i].i = i;

    /* Stable insertion sort on index array. */
    for (uint32_t i = 1; i < b->len; i++) {
        vitte__diag_index key = idx[i];
        uint32_t j = i;
        while (j > 0u) {
            int cmp = vitte__cmp_diag_index(&key, &idx[j - 1u], b);
            if (cmp >= 0) break;
            idx[j] = idx[j - 1u];
            j--;
        }
        idx[j] = key;
    }

    vitte_diag* tmp = (vitte_diag*)malloc((size_t)b->len * sizeof(*tmp));
    if (!tmp) {
        free(idx);
        return;
    }
    for (uint32_t i = 0; i < b->len; i++) {
        tmp[i] = b->diags[idx[i].i];
        memset(&b->diags[idx[i].i], 0, sizeof(b->diags[idx[i].i]));
    }
    for (uint32_t i = 0; i < b->len; i++) {
        b->diags[i] = tmp[i];
    }
    free(tmp);
    free(idx);
}

/* ------------------------------------------------------------------------- */
/* Source mapping helpers (single file)                                       */
/* ------------------------------------------------------------------------- */

typedef struct {
    uint32_t line; /* 1-based */
    uint32_t col;  /* 1-based */
} vitte__line_col;

static vitte__line_col vitte__line_col_at(const vitte_source* src, uint32_t off) {
    vitte__line_col lc;
    lc.line = 1u;
    lc.col = 1u;
    if (!src || !src->data || src->len == 0) return lc;
    if (off > src->len) off = src->len;
    for (uint32_t i = 0; i < off; i++) {
        char c = src->data[i];
        if (c == '\n') {
            lc.line++;
            lc.col = 1u;
        } else {
            lc.col++;
        }
    }
    return lc;
}

static uint32_t vitte__line_start(const vitte_source* src, uint32_t off) {
    if (!src || !src->data || src->len == 0) return 0u;
    if (off > src->len) off = src->len;
    while (off > 0u) {
        char c = src->data[off - 1u];
        if (c == '\n') break;
        off--;
    }
    return off;
}

static uint32_t vitte__line_end(const vitte_source* src, uint32_t off) {
    if (!src || !src->data || src->len == 0) return 0u;
    if (off > src->len) off = src->len;
    while (off < src->len) {
        char c = src->data[off];
        if (c == '\n' || c == '\r') break;
        off++;
    }
    return off;
}

static void vitte__json_write_escaped(FILE* out, const char* s) {
    if (!out) out = stdout;
    fputc('"', out);
    if (s) {
        for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
            unsigned char c = *p;
            switch (c) {
                case '\"': fputs("\\\"", out); break;
                case '\\': fputs("\\\\", out); break;
                case '\b': fputs("\\b", out); break;
                case '\f': fputs("\\f", out); break;
                case '\n': fputs("\\n", out); break;
                case '\r': fputs("\\r", out); break;
                case '\t': fputs("\\t", out); break;
                default:
                    if (c < 0x20) fprintf(out, "\\u%04x", (unsigned)c);
                    else fputc((int)c, out);
                    break;
            }
        }
    }
    fputc('"', out);
}

static void vitte__emit_human_one(FILE* out, const vitte_source* src, const vitte_diag* d, const vitte_emit_options* opt) {
    if (!out) out = stderr;
    if (!d) return;

    const char* sev = vitte__severity_name(d->severity);
    if (d->code[0]) fprintf(out, "%s[%s]: %s\n", sev, d->code, d->message ? d->message : "");
    else fprintf(out, "%s: %s\n", sev, d->message ? d->message : "");

    const vitte_diag_label* primary = NULL;
    for (uint32_t i = 0; i < d->labels_len; i++) {
        if (d->labels[i].style == VITTE_DIAG_LABEL_PRIMARY) {
            primary = &d->labels[i];
            break;
        }
    }
    if (!primary) return;

    vitte_span sp = primary->span;
    const char* path = (src && src->path) ? src->path : "(unknown)";
    vitte__line_col lc = vitte__line_col_at(src, sp.lo);
    fprintf(out, "  --> %s:%u:%u\n", path, (unsigned)lc.line, (unsigned)lc.col);

    if (!src || !src->data || src->len == 0 || sp.file_id != src->file_id) return;

    vitte_emit_options tmp;
    if (!opt) {
        vitte_emit_options_init(&tmp);
        opt = &tmp;
    }
    uint32_t ctx = (opt->context_lines >= 0) ? (uint32_t)opt->context_lines : 1u;

    /* Compute primary line and render a small context window. */
    uint32_t line0 = 1u;
    for (uint32_t i = 0; i < sp.lo && i < src->len; i++) if (src->data[i] == '\n') line0++;
    uint32_t line_lo = (line0 > ctx) ? (line0 - ctx) : 1u;
    uint32_t line_hi = line0 + ctx;

    /* Separator */
    if (opt->show_line_numbers) fputs(" |\n", out);
    else fputs(" |\n", out);

    uint32_t cur_line = 1u;
    uint32_t i = 0u;
    while (i <= src->len) {
        uint32_t ls = i;
        uint32_t le = vitte__line_end(src, i);
        if (cur_line >= line_lo && cur_line <= line_hi) {
            if (opt->show_line_numbers) fprintf(out, "%u | ", (unsigned)cur_line);
            else fputs("| ", out);
            fwrite(src->data + ls, 1, (size_t)(le - ls), out);
            fputc('\n', out);

            if (cur_line == line0) {
                uint32_t underline_lo = (sp.lo > ls) ? (sp.lo - ls) : 0u;
                uint32_t underline_hi = (sp.hi > ls) ? (sp.hi - ls) : 0u;
                if (underline_hi > (le - ls)) underline_hi = (le - ls);
                uint32_t width = (underline_hi > underline_lo) ? (underline_hi - underline_lo) : 1u;
                if (opt->show_line_numbers) fputs(" | ", out);
                else fputs("| ", out);
                for (uint32_t k = 0; k < underline_lo; k++) fputc(' ', out);
                for (uint32_t k = 0; k < width; k++) fputc('^', out);
                if (primary->message && primary->message[0]) fprintf(out, " %s", primary->message);
                fputc('\n', out);
            }
        }
        i = (le < src->len) ? (le + 1u) : (src->len + 1u);
        cur_line++;
        if (cur_line > line_hi) break;
    }

    /* Secondary labels: print as separate "note" blocks. */
    for (uint32_t li = 0; li < d->labels_len; li++) {
        const vitte_diag_label* lab = &d->labels[li];
        if (lab->style != VITTE_DIAG_LABEL_SECONDARY) continue;
        if (lab->span.file_id != src->file_id) continue;
        vitte__line_col slc = vitte__line_col_at(src, lab->span.lo);
        fprintf(out, "note: %s:%u:%u", path, (unsigned)slc.line, (unsigned)slc.col);
        if (lab->message && lab->message[0]) fprintf(out, ": %s", lab->message);
        fputc('\n', out);

        uint32_t s_ls = vitte__line_start(src, lab->span.lo);
        uint32_t s_le = vitte__line_end(src, lab->span.lo);
        if (opt->show_line_numbers) fprintf(out, "%u | ", (unsigned)slc.line);
        else fputs("| ", out);
        fwrite(src->data + s_ls, 1, (size_t)(s_le - s_ls), out);
        fputc('\n', out);
        if (opt->show_line_numbers) fputs(" | ", out);
        else fputs("| ", out);
        uint32_t col0 = (lab->span.lo > s_ls) ? (lab->span.lo - s_ls) : 0u;
        uint32_t w = (lab->span.hi > lab->span.lo) ? (lab->span.hi - lab->span.lo) : 1u;
        for (uint32_t k = 0; k < col0; k++) fputc(' ', out);
        for (uint32_t k = 0; k < w; k++) fputc('^', out);
        fputc('\n', out);
    }

    if (opt->show_help && d->help && d->help[0]) fprintf(out, "help: %s\n", d->help);
    if (opt->show_notes && d->notes && d->notes_len) {
        for (uint32_t ni = 0; ni < d->notes_len; ni++) {
            fprintf(out, "note: %s\n", d->notes[ni] ? d->notes[ni] : "");
        }
    }
}

void vitte_emit_human(FILE* out, const vitte_source* src, vitte_diag_bag* bag, const vitte_emit_options* opt) {
    if (!out) out = stderr;
    if (!bag || !bag->diags || bag->len == 0) return;
    vitte_emit_options tmp;
    if (!opt) {
        vitte_emit_options_init(&tmp);
        opt = &tmp;
    }
    if (opt->sort_by_location) vitte_diag_bag_sort_by_location(bag);
    for (uint32_t i = 0; i < bag->len; i++) {
        vitte__emit_human_one(out, src, &bag->diags[i], opt);
        if (i + 1u < bag->len) fputc('\n', out);
    }
}

static void vitte__emit_json_diag(FILE* out, const vitte_source* src, const vitte_diag* d) {
    if (!out) out = stdout;
    if (!d) return;
    const vitte_diag_label* primary = NULL;
    for (uint32_t i = 0; i < d->labels_len; i++) {
        if (d->labels[i].style == VITTE_DIAG_LABEL_PRIMARY) {
            primary = &d->labels[i];
            break;
        }
    }
    vitte_span sp = primary ? primary->span : vitte_span_make(0u, 0u, 0u);
    const char* path = (src && src->path) ? src->path : "";
    vitte__line_col lc = vitte__line_col_at(src, sp.lo);

    fputc('{', out);
    fputs("\"severity\":", out);
    vitte__json_write_escaped(out, vitte__severity_name(d->severity));
    fputs(",\"code\":", out);
    vitte__json_write_escaped(out, d->code);
    fputs(",\"message\":", out);
    vitte__json_write_escaped(out, d->message ? d->message : "");
    fputs(",\"file\":", out);
    vitte__json_write_escaped(out, path);
    fprintf(out, ",\"line\":%u,\"col\":%u", (unsigned)lc.line, (unsigned)lc.col);
    fprintf(out, ",\"span\":{\"file_id\":%u,\"lo\":%u,\"hi\":%u}", (unsigned)sp.file_id, (unsigned)sp.lo, (unsigned)sp.hi);

    fputs(",\"labels\":[", out);
    for (uint32_t i = 0; i < d->labels_len; i++) {
        const vitte_diag_label* lab = &d->labels[i];
        if (i) fputc(',', out);
        fputc('{', out);
        fputs("\"style\":", out);
        vitte__json_write_escaped(out, (lab->style == VITTE_DIAG_LABEL_PRIMARY) ? "primary" : "secondary");
        fprintf(out, ",\"span\":{\"file_id\":%u,\"lo\":%u,\"hi\":%u}", (unsigned)lab->span.file_id, (unsigned)lab->span.lo, (unsigned)lab->span.hi);
        fputs(",\"message\":", out);
        vitte__json_write_escaped(out, lab->message ? lab->message : "");
        fputc('}', out);
    }
    fputc(']', out);

    fputs(",\"help\":", out);
    vitte__json_write_escaped(out, d->help ? d->help : "");

    fputs(",\"notes\":[", out);
    for (uint32_t i = 0; i < d->notes_len; i++) {
        if (i) fputc(',', out);
        vitte__json_write_escaped(out, d->notes[i] ? d->notes[i] : "");
    }
    fputc(']', out);
    fputc('}', out);
}

void vitte_emit_json(FILE* out, const vitte_source* src, vitte_diag_bag* bag, const vitte_emit_options* opt) {
    if (!out) out = stdout;
    if (!bag || !bag->diags || bag->len == 0) return;
    vitte_emit_options tmp;
    if (!opt) {
        vitte_emit_options_init(&tmp);
        opt = &tmp;
    }
    if (opt->sort_by_location) vitte_diag_bag_sort_by_location(bag);

    if (!opt->json_one_per_line) fputs("[", out);
    for (uint32_t i = 0; i < bag->len; i++) {
        if (!opt->json_one_per_line) {
            if (i) fputs(",", out);
            if (opt->json_pretty) fputs("\n  ", out);
        }
        vitte__emit_json_diag(out, src, &bag->diags[i]);
        if (opt->json_one_per_line) fputc('\n', out);
    }
    if (!opt->json_one_per_line) {
        if (opt->json_pretty) fputs("\n", out);
        fputs("]\n", out);
    }
}

