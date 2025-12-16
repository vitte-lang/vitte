#include "compiler/ast.h"
#include <stdlib.h>
#include <string.h>

ast_module_t* ast_module_create(void) {
    ast_module_t *mod = (ast_module_t *)malloc(sizeof(ast_module_t));
    if (!mod) return NULL;
    
    mod->decl_capacity = 16;
    mod->decl_count = 0;
    mod->decls = (ast_node_t **)calloc(mod->decl_capacity, sizeof(ast_node_t *));
    
    if (!mod->decls) {
        free(mod);
        return NULL;
    }
    
    return mod;
}

void ast_module_destroy(ast_module_t *mod) {
    if (!mod) return;
    free(mod->decls);
    free(mod);
}

void ast_module_add_decl(ast_module_t *mod, ast_node_t *decl) {
    if (!mod || !decl) return;
    
    if (mod->decl_count >= mod->decl_capacity) {
        mod->decl_capacity *= 2;
        mod->decls = (ast_node_t **)realloc(mod->decls, mod->decl_capacity * sizeof(ast_node_t *));
    }
    
    mod->decls[mod->decl_count++] = decl;
}
