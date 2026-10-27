#ifndef VITTE_AST_SPAN_H
#define VITTE_AST_SPAN_H

/*
 * Shared source range contract for AST and diagnostics.
 *
 * This header intentionally does not include the AST node model. The
 * bootstrap parser has its own AST representation, while diagnostics only
 * need source coordinates.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_ast_span {
    uint32_t file_id;
    size_t begin;
    size_t end;

    /* Optional rich coordinates used by the diagnostic subsystem. */
    uint64_t source_id;
    const char *source_name;
    size_t start_offset;
    size_t end_offset;
    unsigned start_line;
    unsigned start_column;
    unsigned end_line;
    unsigned end_column;
    bool valid;
} vitte_ast_span_t;

vitte_ast_span_t
vitte_ast_span_make(
    uint32_t file_id,
    size_t begin,
    size_t end);

bool
vitte_ast_span_is_valid(
    vitte_ast_span_t span);

size_t
vitte_ast_span_length(
    vitte_ast_span_t span);

vitte_ast_span_t
vitte_ast_span_join(
    vitte_ast_span_t left,
    vitte_ast_span_t right);

#ifdef __cplusplus
}
#endif

#endif
