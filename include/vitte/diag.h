#ifndef VITTE_DIAG_H
#define VITTE_DIAG_H

#include "vitte.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VITTE_SEV_ERROR = 0,
    VITTE_SEV_WARNING = 1,
    VITTE_SEV_NOTE = 2
} vitte_severity;

typedef enum {
    VITTE_DIAG_LABEL_PRIMARY = 0,
    VITTE_DIAG_LABEL_SECONDARY = 1
} vitte_diag_label_style;

typedef struct {
    vitte_diag_label_style style;
    vitte_span span;
    /* Optional message (owned, may be NULL). */
    char* message;
} vitte_diag_label;

typedef struct {
    vitte_severity severity;
    /* Stable code like "V0001". NUL-terminated. */
    char code[16];
    /* Short message (owned, non-NULL). */
    char* message;

    vitte_diag_label* labels;
    uint32_t labels_len;
    uint32_t labels_cap;

    char** notes;
    uint32_t notes_len;
    uint32_t notes_cap;

    /* Optional help text (owned, may be NULL). */
    char* help;
} vitte_diag;

typedef struct {
    vitte_diag* diags;
    uint32_t len;
    uint32_t cap;
    uint32_t errors;
} vitte_diag_bag;

typedef struct {
    vitte_file_id file_id;
    const char* path; /* optional; used for rendering */
    const char* data;
    uint32_t len;
} vitte_source;

typedef struct {
    int context_lines;         /* default 1 */
    int show_line_numbers;     /* default 1 */
    int show_notes;            /* default 1 */
    int show_help;             /* default 1 */
    int sort_by_location;      /* default 1 */
    int json_one_per_line;     /* default 1 */
    int json_pretty;           /* default 0 */
} vitte_emit_options;

void vitte_emit_options_init(vitte_emit_options* opt);

void vitte_diag_bag_init(vitte_diag_bag* b);
void vitte_diag_bag_free(vitte_diag_bag* b);
int vitte_diag_bag_has_errors(const vitte_diag_bag* b);

vitte_diag* vitte_diag_bag_push(
    vitte_diag_bag* b,
    vitte_severity severity,
    const char* code,
    vitte_span primary_span,
    const char* message
);

int vitte_diag_add_label(vitte_diag* d, vitte_diag_label_style style, vitte_span span, const char* message);
int vitte_diag_add_note(vitte_diag* d, const char* note);
void vitte_diag_set_help(vitte_diag* d, const char* help);

void vitte_diag_bag_sort_by_location(vitte_diag_bag* b);

void vitte_emit_human(FILE* out, const vitte_source* src, vitte_diag_bag* bag, const vitte_emit_options* opt);
void vitte_emit_json(FILE* out, const vitte_source* src, vitte_diag_bag* bag, const vitte_emit_options* opt);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAG_H */

