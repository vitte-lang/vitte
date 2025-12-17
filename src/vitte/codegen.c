#include "vitte/codegen.h"
#include "vitte/parser_phrase.h"
#include "vitte/desugar_phrase.h"
#include <stdlib.h>
#include <string.h>

static void cg_set_err(vitte_error* err, vitte_error_code code, uint32_t line, uint32_t col, const char* msg) {
    if (!err) return;
    err->code = code;
    err->line = line;
    err->col = col;
    if (msg) {
        strncpy(err->message, msg, sizeof(err->message) - 1);
        err->message[sizeof(err->message) - 1] = '\0';
    }
}

static char* cg_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len);
    out[len] = '\0';
    return out;
}

void vitte_codegen_unit_init(vitte_codegen_unit* unit) {
    if (!unit) return;
    memset(unit, 0, sizeof(*unit));
}

static void free_modules(vitte_codegen_module* mods, size_t count) {
    if (!mods) return;
    for (size_t i = 0; i < count; ++i) {
        free(mods[i].name);
    }
    free(mods);
}

static void free_types(vitte_codegen_type* types, size_t count) {
    if (!types) return;
    for (size_t i = 0; i < count; ++i) {
        free(types[i].name);
    }
    free(types);
}

static void free_functions(vitte_codegen_function* fns, size_t count) {
    if (!fns) return;
    for (size_t i = 0; i < count; ++i) {
        free(fns[i].name);
    }
    free(fns);
}

static void free_entrypoints(vitte_codegen_entrypoint* entries, size_t count) {
    if (!entries) return;
    for (size_t i = 0; i < count; ++i) {
        free(entries[i].module_path);
        free(entries[i].symbol);
    }
    free(entries);
}

void vitte_codegen_unit_reset(vitte_ctx* ctx, vitte_codegen_unit* unit) {
    if (!unit) return;
    free_modules(unit->modules, unit->module_count);
    free_types(unit->types, unit->type_count);
    free_functions(unit->functions, unit->function_count);
    free_entrypoints(unit->entrypoints, unit->entrypoint_count);

    if (unit->core_ast) {
        vitte_ast_free(ctx, unit->core_ast);
    }
    if (unit->phrase_ast) {
        vitte_ast_free(ctx, unit->phrase_ast);
    }

    memset(unit, 0, sizeof(*unit));
}

static int ensure_capacity(void** buf, size_t* cap, size_t count, size_t elem_size) {
    if (count < *cap) return 1;
    size_t new_cap = (*cap == 0) ? 4 : (*cap * 2);
    void* nb = realloc(*buf, new_cap * elem_size);
    if (!nb) return 0;
    *buf = nb;
    *cap = new_cap;
    return 1;
}

static size_t count_fields(const vitte_ast* type_ast) {
    size_t count = 0;
    for (const vitte_ast* child = type_ast ? type_ast->first_child : NULL; child; child = child->next) {
        if (child->kind == VITTE_AST_FIELD_DECL) count++;
    }
    return count;
}

static const vitte_ast* find_block_child(const vitte_ast* node) {
    for (const vitte_ast* child = node ? node->first_child : NULL; child; child = child->next) {
        if (child->kind == VITTE_AST_BLOCK) return child;
    }
    return NULL;
}

static size_t count_block_stmts(const vitte_ast* block) {
    size_t n = 0;
    for (const vitte_ast* stmt = block ? block->first_child : NULL; stmt; stmt = stmt->next) {
        n++;
    }
    return n;
}

static int push_module(vitte_codegen_unit* unit, const vitte_ast* node) {
    if (!ensure_capacity((void**)&unit->modules, &unit->module_cap, unit->module_count + 1, sizeof(*unit->modules))) {
        return 0;
    }
    vitte_codegen_module* mod = &unit->modules[unit->module_count++];
    mod->name = cg_strdup(node->text);
    mod->span = node->span;
    mod->ast = node;
    return mod->name || node->text == NULL;
}

static int push_type(vitte_codegen_unit* unit, const vitte_ast* node, const vitte_ast* module_ast) {
    if (!ensure_capacity((void**)&unit->types, &unit->type_cap, unit->type_count + 1, sizeof(*unit->types))) {
        return 0;
    }
    vitte_codegen_type* ty = &unit->types[unit->type_count++];
    ty->name = cg_strdup(node->text);
    ty->span = node->span;
    ty->field_count = count_fields(node);
    ty->ast = node;
    ty->module_ast = module_ast;
    return ty->name || node->text == NULL;
}

static size_t count_params(const vitte_ast* fn_ast) {
    size_t n = 0;
    for (const vitte_ast* child = fn_ast ? fn_ast->first_child : NULL; child; child = child->next) {
        if (child->kind == VITTE_AST_PARAM) {
            ++n;
        }
    }
    return n;
}

static int push_function(vitte_codegen_unit* unit, const vitte_ast* node, const vitte_ast* module_ast) {
    if (!ensure_capacity((void**)&unit->functions, &unit->function_cap, unit->function_count + 1, sizeof(*unit->functions))) {
        return 0;
    }
    vitte_codegen_function* fn = &unit->functions[unit->function_count++];
    fn->name = cg_strdup(node->text);
    fn->span = node->span;
    fn->ast = node;
    fn->block = find_block_child(node);
    fn->stmt_count = count_block_stmts(fn->block);
    fn->param_count = count_params(node);
    fn->module_ast = module_ast;
    return fn->name || node->text == NULL;
}

static int push_entrypoint(vitte_codegen_unit* unit, const vitte_ast* node) {
    if (!ensure_capacity((void**)&unit->entrypoints, &unit->entrypoint_cap, unit->entrypoint_count + 1, sizeof(*unit->entrypoints))) {
        return 0;
    }
    vitte_codegen_entrypoint* ep = &unit->entrypoints[unit->entrypoint_count++];
    ep->module_path = cg_strdup(node->text);
    ep->symbol = cg_strdup(node->aux_text);
    ep->span = node->span;
    ep->ast = node;
    if ((node->text && !ep->module_path) || (node->aux_text && !ep->symbol)) {
        return 0;
    }
    return 1;
}

static int process_core_ast(vitte_codegen_unit* unit) {
    const vitte_ast* current_module = NULL;
    const vitte_ast* child = unit->core_ast ? unit->core_ast->first_child : NULL;
    while (child) {
        switch (child->kind) {
            case VITTE_AST_MODULE_DECL:
                if (!push_module(unit, child)) return 0;
                current_module = child;
                break;
            case VITTE_AST_TYPE_DECL:
                if (!push_type(unit, child, current_module)) return 0;
                break;
            case VITTE_AST_FN_DECL:
            case VITTE_AST_SCENARIO_DECL:
                if (!push_function(unit, child, current_module)) return 0;
                break;
            case VITTE_AST_PROGRAM_DECL:
                if (!push_entrypoint(unit, child)) return 0;
                break;
            default:
                break;
        }
        child = child->next;
    }
    return 1;
}

vitte_result vitte_codegen_unit_build(vitte_ctx* ctx,
                                      const char* src,
                                      size_t len,
                                      vitte_codegen_unit* out,
                                      vitte_error* err) {
    if (!out) return VITTE_ERR_INTERNAL;
    vitte_codegen_unit_reset(ctx, out);

    vitte_ast* phrase = NULL;
    vitte_result r = vitte_parse_phrase(ctx, src, len, &phrase, err);
    if (r != VITTE_OK) {
        if (phrase) vitte_ast_free(ctx, phrase);
        return r;
    }

    vitte_ast* core = NULL;
    r = vitte_desugar_phrase(ctx, phrase, &core, err);
    if (r != VITTE_OK) {
        if (phrase) vitte_ast_free(ctx, phrase);
        return r;
    }

    out->phrase_ast = phrase;
    out->core_ast = core;

    if (!process_core_ast(out)) {
        vitte_codegen_unit_reset(ctx, out);
        cg_set_err(err, VITTE_ERRC_SYNTAX, 0, 0, "codegen allocation failure");
        return VITTE_ERR_INTERNAL;
    }

    return VITTE_OK;
}
