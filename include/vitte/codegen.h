#ifndef VITTE_CODEGEN_H
#define VITTE_CODEGEN_H

#include "vitte.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char* name;              /* fully qualified module path (owned copy) */
    vitte_span span;         /* span of the `mod` declaration */
    const vitte_ast* ast;    /* original module AST node for extra metadata */
} vitte_codegen_module;

typedef struct {
    char* name;                      /* short type name */
    vitte_span span;
    size_t field_count;
    const vitte_ast* ast;            /* TYPE_DECL node */
    const vitte_ast* module_ast;     /* owning module declaration (may be NULL) */
} vitte_codegen_type;

typedef struct {
    char* name;                      /* function identifier */
    vitte_span span;
    const vitte_ast* ast;            /* FN_DECL node */
    const vitte_ast* block;          /* lowered block body */
    size_t stmt_count;
    size_t param_count;
    const vitte_ast* module_ast;     /* innermost module describing the scope */
} vitte_codegen_function;

typedef struct {
    char* module_path;
    char* symbol;
    vitte_span span;
    const vitte_ast* ast;
} vitte_codegen_entrypoint;

typedef struct {
    vitte_ast* phrase_ast;
    vitte_ast* core_ast;

    vitte_codegen_module* modules;
    size_t module_count;
    size_t module_cap;

    vitte_codegen_type* types;
    size_t type_count;
    size_t type_cap;

    vitte_codegen_function* functions;
    size_t function_count;
    size_t function_cap;

    vitte_codegen_entrypoint* entrypoints;
    size_t entrypoint_count;
    size_t entrypoint_cap;
} vitte_codegen_unit;

void vitte_codegen_unit_init(vitte_codegen_unit* unit);
void vitte_codegen_unit_reset(vitte_ctx* ctx, vitte_codegen_unit* unit);

vitte_result vitte_codegen_unit_build(vitte_ctx* ctx,
                                      const char* src,
                                      size_t len,
                                      vitte_codegen_unit* out,
                                      vitte_error* err);


/* -------------------------------------------------------------------------
 * Inline helpers
 * ------------------------------------------------------------------------- */

static inline const char*
vitte_codegen_module_name_from_ast(const vitte_ast* module_ast) {
    return (module_ast && module_ast->text) ? module_ast->text : "";
}

static inline const char*
vitte_codegen_module_name(const vitte_codegen_module* mod) {
    return (mod && mod->name) ? mod->name : "";
}

static inline const char*
vitte_codegen_type_module_name(const vitte_codegen_type* ty) {
    return ty ? vitte_codegen_module_name_from_ast(ty->module_ast) : "";
}

static inline const char*
vitte_codegen_function_module_name(const vitte_codegen_function* fn) {
    return fn ? vitte_codegen_module_name_from_ast(fn->module_ast) : "";
}

#ifdef __cplusplus
}
#endif

#endif /* VITTE_CODEGEN_H */
