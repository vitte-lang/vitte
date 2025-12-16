// Vitte Compiler - Semantic Analyzer Implementation
// Type checking and semantic analysis

#include "semantic.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// Helper Functions
// ============================================================================

static char* semantic_strdup(const char *str) {
    if (!str) return NULL;
    char *copy = malloc(strlen(str) + 1);
    if (copy) strcpy(copy, str);
    return copy;
}

static void add_error(semantic_analyzer_t *analyzer, const char *message) {
    if (analyzer->error_count >= analyzer->error_capacity) {
        analyzer->error_capacity *= 2;
        char **new_errors = realloc(analyzer->errors,
            sizeof(char*) * analyzer->error_capacity);
        if (!new_errors) return;
        analyzer->errors = new_errors;
    }
    
    analyzer->errors[analyzer->error_count++] = semantic_strdup(message);
}

// ============================================================================
// Type Management
// ============================================================================

type_info_t* type_create(type_kind_t kind) {
    type_info_t *type = malloc(sizeof(type_info_t));
    if (!type) return NULL;
    
    type->kind = kind;
    type->name = NULL;
    type->base_type = NULL;
    type->array_size = 0;
    type->param_types = NULL;
    type->param_count = 0;
    type->return_type = NULL;
    type->constraints = NULL;
    type->constraint_count = 0;
    
    return type;
}

type_info_t* type_create_pointer(type_info_t *base) {
    if (!base) return NULL;
    
    type_info_t *type = type_create(TYPE_POINTER);
    if (!type) return NULL;
    
    type->base_type = base;
    return type;
}

type_info_t* type_create_array(type_info_t *element, size_t size) {
    if (!element) return NULL;
    
    type_info_t *type = type_create(TYPE_ARRAY);
    if (!type) return NULL;
    
    type->base_type = element;
    type->array_size = size;
    return type;
}

type_info_t* type_create_function(type_info_t **param_types, size_t param_count,
                                 type_info_t *return_type) {
    type_info_t *type = type_create(TYPE_FUNCTION);
    if (!type) return NULL;
    
    type->param_types = malloc(sizeof(type_info_t*) * param_count);
    if (!type->param_types && param_count > 0) {
        free(type);
        return NULL;
    }
    
    for (size_t i = 0; i < param_count; i++) {
        type->param_types[i] = param_types[i];
    }
    type->param_count = param_count;
    type->return_type = return_type;
    
    return type;
}

int type_compatible(type_info_t *a, type_info_t *b) {
    if (!a || !b) return 0;
    
    if (a->kind != b->kind) return 0;
    
    switch (a->kind) {
        case TYPE_POINTER:
        case TYPE_ARRAY:
            return type_compatible(a->base_type, b->base_type);
        
        case TYPE_FUNCTION:
            if (a->param_count != b->param_count) return 0;
            for (size_t i = 0; i < a->param_count; i++) {
                if (!type_compatible(a->param_types[i], b->param_types[i])) {
                    return 0;
                }
            }
            return type_compatible(a->return_type, b->return_type);
        
        default:
            return 1;
    }
}

void type_free(type_info_t *type) {
    if (!type) return;
    
    free(type->name);
    
    if (type->kind == TYPE_POINTER || type->kind == TYPE_ARRAY) {
        type_free(type->base_type);
    }
    
    if (type->kind == TYPE_FUNCTION) {
        for (size_t i = 0; i < type->param_count; i++) {
            type_free(type->param_types[i]);
        }
        free(type->param_types);
        type_free(type->return_type);
    }
    
    for (size_t i = 0; i < type->constraint_count; i++) {
        type_free(type->constraints[i]);
    }
    free(type->constraints);
    
    free(type);
}

const char* type_kind_name(type_kind_t kind) {
    switch (kind) {
        case TYPE_VOID: return "void";
        case TYPE_BOOL: return "bool";
        case TYPE_INT: return "int";
        case TYPE_INT8: return "i8";
        case TYPE_INT16: return "i16";
        case TYPE_INT32: return "i32";
        case TYPE_INT64: return "i64";
        case TYPE_UINT: return "uint";
        case TYPE_UINT8: return "u8";
        case TYPE_UINT16: return "u16";
        case TYPE_UINT32: return "u32";
        case TYPE_UINT64: return "u64";
        case TYPE_FLOAT: return "float";
        case TYPE_FLOAT32: return "f32";
        case TYPE_FLOAT64: return "f64";
        case TYPE_STRING: return "string";
        case TYPE_CHAR: return "char";
        case TYPE_POINTER: return "pointer";
        case TYPE_ARRAY: return "array";
        case TYPE_FUNCTION: return "function";
        case TYPE_STRUCT: return "struct";
        case TYPE_ENUM: return "enum";
        case TYPE_TRAIT: return "trait";
        case TYPE_GENERIC: return "generic";
        case TYPE_UNKNOWN: return "unknown";
        case TYPE_ERROR: return "error";
        default: return "?";
    }
}

void type_print(type_info_t *type) {
    if (!type) return;
    
    if (type->name) {
        printf("%s", type->name);
    } else {
        printf("%s", type_kind_name(type->kind));
    }
    
    if (type->kind == TYPE_POINTER) {
        printf("*");
        type_print(type->base_type);
    } else if (type->kind == TYPE_ARRAY) {
        printf("[%zu]", type->array_size);
        type_print(type->base_type);
    }
}

// ============================================================================
// Symbol Table
// ============================================================================

scope_t* scope_create(scope_t *parent) {
    scope_t *scope = malloc(sizeof(scope_t));
    if (!scope) return NULL;
    
    scope->symbols = NULL;
    scope->symbol_count = 0;
    scope->symbol_capacity = 16;
    scope->parent = parent;
    
    scope->symbols = malloc(sizeof(symbol_t) * scope->symbol_capacity);
    if (!scope->symbols) {
        free(scope);
        return NULL;
    }
    
    return scope;
}

static void scope_free(scope_t *scope) {
    if (!scope) return;
    
    for (size_t i = 0; i < scope->symbol_count; i++) {
        free(scope->symbols[i].name);
        type_free(scope->symbols[i].type);
    }
    
    free(scope->symbols);
    free(scope);
}

// ============================================================================
// Analyzer Implementation
// ============================================================================

semantic_analyzer_t* analyzer_create(void) {
    semantic_analyzer_t *analyzer = malloc(sizeof(semantic_analyzer_t));
    if (!analyzer) return NULL;
    
    analyzer->global_scope = scope_create(NULL);
    analyzer->current_scope = analyzer->global_scope;
    analyzer->error_count = 0;
    analyzer->error_capacity = 32;
    analyzer->errors = malloc(sizeof(char*) * analyzer->error_capacity);
    analyzer->in_function = 0;
    analyzer->in_loop = 0;
    
    if (!analyzer->errors || !analyzer->global_scope) {
        free(analyzer->errors);
        free(analyzer);
        return NULL;
    }
    
    return analyzer;
}

void analyzer_push_scope(semantic_analyzer_t *analyzer) {
    if (!analyzer) return;
    
    scope_t *new_scope = scope_create(analyzer->current_scope);
    if (new_scope) {
        analyzer->current_scope = new_scope;
    }
}

void analyzer_pop_scope(semantic_analyzer_t *analyzer) {
    if (!analyzer || !analyzer->current_scope->parent) return;
    
    scope_t *old_scope = analyzer->current_scope;
    analyzer->current_scope = analyzer->current_scope->parent;
    scope_free(old_scope);
}

void analyzer_define_symbol(semantic_analyzer_t *analyzer,
                           const char *name, type_info_t *type,
                           int is_mutable, int line, int column) {
    if (!analyzer || !analyzer->current_scope || !name) return;
    
    scope_t *scope = analyzer->current_scope;
    
    if (scope->symbol_count >= scope->symbol_capacity) {
        scope->symbol_capacity *= 2;
        symbol_t *new_symbols = realloc(scope->symbols,
            sizeof(symbol_t) * scope->symbol_capacity);
        if (!new_symbols) return;
        scope->symbols = new_symbols;
    }
    
    scope->symbols[scope->symbol_count].name = semantic_strdup(name);
    scope->symbols[scope->symbol_count].type = type;
    scope->symbols[scope->symbol_count].is_mutable = is_mutable;
    scope->symbols[scope->symbol_count].is_function = 0;
    scope->symbols[scope->symbol_count].is_parameter = 0;
    scope->symbols[scope->symbol_count].line = line;
    scope->symbols[scope->symbol_count].column = column;
    
    scope->symbol_count++;
}

symbol_t* analyzer_lookup_symbol(semantic_analyzer_t *analyzer,
                                const char *name) {
    if (!analyzer || !name) return NULL;
    
    scope_t *scope = analyzer->current_scope;
    
    while (scope) {
        for (size_t i = 0; i < scope->symbol_count; i++) {
            if (strcmp(scope->symbols[i].name, name) == 0) {
                return &scope->symbols[i];
            }
        }
        scope = scope->parent;
    }
    
    return NULL;
}

// ============================================================================
// Analysis Functions
// ============================================================================

static type_info_t* analyze_node(semantic_analyzer_t *analyzer,
                                ast_node_t *node);

static type_info_t* analyze_node(semantic_analyzer_t *analyzer,
                                ast_node_t *node) {
    if (!analyzer || !node) return type_create(TYPE_ERROR);
    
    switch (node->type) {
        case AST_LITERAL: {
            return type_create(TYPE_INT);
        }
        
        case AST_IDENTIFIER: {
            symbol_t *sym = analyzer_lookup_symbol(analyzer, "temp");
            if (sym) {
                return sym->type;
            }
            add_error(analyzer, "Undefined identifier");
            return type_create(TYPE_ERROR);
        }
        
        case AST_BINARY_OP: {
            if (node->children_count < 2) {
                return type_create(TYPE_ERROR);
            }
            analyze_node(analyzer, node->children[0]);
            analyze_node(analyzer, node->children[1]);
            return type_create(TYPE_INT);
        }
        
        case AST_UNARY_OP: {
            if (node->children_count < 1) {
                return type_create(TYPE_ERROR);
            }
            return analyze_node(analyzer, node->children[0]);
        }
        
        case AST_FUNCTION: {
            analyzer->in_function = 1;
            analyzer_push_scope(analyzer);
            
            for (size_t i = 0; i < node->children_count; i++) {
                analyze_node(analyzer, node->children[i]);
            }
            
            analyzer_pop_scope(analyzer);
            analyzer->in_function = 0;
            return type_create(TYPE_VOID);
        }
        
        case AST_WHILE:
        case AST_FOR: {
            int old_in_loop = analyzer->in_loop;
            analyzer->in_loop = 1;
            
            for (size_t i = 0; i < node->children_count; i++) {
                analyze_node(analyzer, node->children[i]);
            }
            
            analyzer->in_loop = old_in_loop;
            return type_create(TYPE_VOID);
        }
        
        case AST_RETURN: {
            if (!analyzer->in_function) {
                add_error(analyzer, "Return outside function");
            }
            if (node->children_count > 0) {
                return analyze_node(analyzer, node->children[0]);
            }
            return type_create(TYPE_VOID);
        }
        
        case AST_BREAK:
        case AST_CONTINUE: {
            if (!analyzer->in_loop) {
                add_error(analyzer, "Break/Continue outside loop");
            }
            return type_create(TYPE_VOID);
        }
        
        case AST_BLOCK: {
            analyzer_push_scope(analyzer);
            
            for (size_t i = 0; i < node->children_count; i++) {
                analyze_node(analyzer, node->children[i]);
            }
            
            analyzer_pop_scope(analyzer);
            return type_create(TYPE_VOID);
        }
        
        default:
            return type_create(TYPE_UNKNOWN);
    }
}

int analyzer_analyze(semantic_analyzer_t *analyzer, ast_node_t *ast) {
    if (!analyzer || !ast) return -1;
    
    for (size_t i = 0; i < ast->children_count; i++) {
        analyze_node(analyzer, ast->children[i]);
    }
    
    return analyzer->error_count > 0 ? -1 : 0;
}

type_info_t* analyzer_get_type(semantic_analyzer_t *analyzer,
                               ast_node_t *expr) {
    if (!analyzer || !expr) return type_create(TYPE_ERROR);
    
    return analyze_node(analyzer, expr);
}

const char** analyzer_get_errors(semantic_analyzer_t *analyzer, size_t *count) {
    if (!analyzer) {
        *count = 0;
        return NULL;
    }
    *count = analyzer->error_count;
    return (const char**)analyzer->errors;
}

void analyzer_free(semantic_analyzer_t *analyzer) {
    if (!analyzer) return;
    
    scope_free(analyzer->global_scope);
    
    for (size_t i = 0; i < analyzer->error_count; i++) {
        free(analyzer->errors[i]);
    }
    free(analyzer->errors);
    
    free(analyzer);
}
