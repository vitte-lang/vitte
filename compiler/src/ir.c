#include "compiler/ir.h"
#include <stdlib.h>

ir_module_t* ir_module_create(void) {
    ir_module_t *mod = (ir_module_t *)malloc(sizeof(ir_module_t));
    if (!mod) return NULL;
    
    mod->func_capacity = 16;
    mod->func_count = 0;
    mod->funcs = (ir_func_t **)calloc(mod->func_capacity, sizeof(ir_func_t *));
    
    if (!mod->funcs) {
        free(mod);
        return NULL;
    }
    
    return mod;
}

void ir_module_destroy(ir_module_t *mod) {
    if (!mod) return;
    free(mod->funcs);
    free(mod);
}

void ir_module_add_func(ir_module_t *mod, ir_func_t *func) {
    if (!mod || !func) return;
    
    if (mod->func_count >= mod->func_capacity) {
        mod->func_capacity *= 2;
        mod->funcs = (ir_func_t **)realloc(mod->funcs, mod->func_capacity * sizeof(ir_func_t *));
    }
    
    mod->funcs[mod->func_count++] = func;
}
