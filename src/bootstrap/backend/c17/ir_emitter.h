#ifndef VITTE_C17_IR_EMITTER_H
#define VITTE_C17_IR_EMITTER_H

/*
 * Vitte Compiler
 * src/bootstrap/backend/c17/ir_emitter.h
 *
 * Deterministic C17 emission for the validated bootstrap IR.
 */

#include "../../ir/ir.h"

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_c17_ir_emit_options {
    bool emit_banner;
    bool emit_debug_comments;
    bool emit_line_directives;
    bool emit_debug_info;
    bool emit_entry_wrapper;
    bool strict;
    const char *source_path;
} vitte_c17_ir_emit_options_t;

typedef enum vitte_c17_ir_compile_result {
    VITTE_C17_IR_COMPILE_SUCCESS = 0,
    VITTE_C17_IR_COMPILE_INVALID_ARGUMENT,
    VITTE_C17_IR_COMPILE_OUTPUT_FAILED,
    VITTE_C17_IR_COMPILE_EMIT_FAILED,
    VITTE_C17_IR_COMPILE_C_COMPILATION_FAILED,
    VITTE_C17_IR_COMPILE_LINK_FAILED,
    VITTE_C17_IR_COMPILE_RUNTIME_FAILED,
    VITTE_C17_IR_COMPILE_UNSUPPORTED
} vitte_c17_ir_compile_result_t;

/* Debug output emits a stable #line directive and vitte-symbol comments.
 * Native compilation adds DWARF flags, disables inlining and preserves frame
 * pointers when emit_debug_info is enabled. */

vitte_c17_ir_emit_options_t
vitte_c17_ir_emit_options_default(void);

/* Emit a complete translation unit from a validated IR context. */
bool
vitte_c17_emit_ir(
    FILE *output,
    const vitte_ir_context_t *ir,
    const vitte_c17_ir_emit_options_t *options);

/* Emit C and invoke the host C compiler for an executable or object file. */
bool
vitte_c17_compile_ir(
    const vitte_ir_context_t *ir,
    const char *output_path,
    bool emit_c,
    bool run,
    int run_argc,
    char **run_argv,
    const vitte_c17_ir_emit_options_t *options);

vitte_c17_ir_compile_result_t
vitte_c17_compile_ir_result(
    const vitte_ir_context_t *ir,
    const char *output_path,
    bool emit_c,
    bool run,
    int run_argc,
    char **run_argv,
    const vitte_c17_ir_emit_options_t *options);

bool
vitte_c17_compile_ir_object(
    const vitte_ir_context_t *ir,
    const char *output_path,
    const vitte_c17_ir_emit_options_t *options);

vitte_c17_ir_compile_result_t
vitte_c17_compile_ir_object_result(
    const vitte_ir_context_t *ir,
    const char *output_path,
    const vitte_c17_ir_emit_options_t *options);

#ifdef __cplusplus
}
#endif

#endif
