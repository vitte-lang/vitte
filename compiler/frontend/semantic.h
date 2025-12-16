// Vitte Compiler - Semantic Analyzer Header
// Type checking and semantic analysis

#ifndef VITTE_SEMANTIC_H
#define VITTE_SEMANTIC_H

#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Type System
// ============================================================================

typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_INT,
    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,
    TYPE_UINT,
    TYPE_UINT8,
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_UINT64,
    TYPE_FLOAT,
    TYPE_FLOAT32,
    TYPE_FLOAT64,
    TYPE_STRING,
    TYPE_CHAR,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_STRUCT,
    TYPE_ENUM,
    TYPE_TRAIT,
    TYPE_GENERIC,
    TYPE_UNKNOWN,
    TYPE_ERROR,
} type_kind_t;

typedef struct type_info {
    type_kind_t kind;
    char *name;
    struct type_info *base_type;  // For pointers, arrays
    size_t array_size;             // For arrays
    struct type_info **param_types; // For functions
    size_t param_count;
    struct type_info *return_type; // For functions
    struct type_info **constraints; // For generics
    size_t constraint_count;
} type_info_t;

// ============================================================================
// Symbol Table
// ============================================================================

typedef struct {
    char *name;
    type_info_t *type;
    int is_mutable;
    int is_function;
    int is_parameter;
    int line;
    int column;
} symbol_t;

typedef struct {
    symbol_t *symbols;
    size_t symbol_count;
    size_t symbol_capacity;
    struct scope *parent;
} scope_t;

// ============================================================================
// Semantic Analyzer
// ============================================================================

typedef struct {
    scope_t *current_scope;
    scope_t *global_scope;
    char **errors;
    size_t error_count;
    size_t error_capacity;
    int in_function;
    int in_loop;
} semantic_analyzer_t;

// ============================================================================
// Analyzer API
// ============================================================================

// Create analyzer
semantic_analyzer_t* analyzer_create(void);

// Analyze AST
int analyzer_analyze(semantic_analyzer_t *analyzer, ast_node_t *ast);

// Get type of expression
type_info_t* analyzer_get_type(semantic_analyzer_t *analyzer, 
                               ast_node_t *expr);

// Get error messages
const char** analyzer_get_errors(semantic_analyzer_t *analyzer, size_t *count);

// Free analyzer
void analyzer_free(semantic_analyzer_t *analyzer);

// ============================================================================
// Symbol Table Management
// ============================================================================

// Create scope
scope_t* scope_create(scope_t *parent);

// Enter scope
void analyzer_push_scope(semantic_analyzer_t *analyzer);

// Exit scope
void analyzer_pop_scope(semantic_analyzer_t *analyzer);

// Define symbol
void analyzer_define_symbol(semantic_analyzer_t *analyzer,
                           const char *name, type_info_t *type,
                           int is_mutable, int line, int column);

// Lookup symbol
symbol_t* analyzer_lookup_symbol(semantic_analyzer_t *analyzer,
                                const char *name);

// ============================================================================
// Type Management
// ============================================================================

// Create type
type_info_t* type_create(type_kind_t kind);

// Create pointer type
type_info_t* type_create_pointer(type_info_t *base);

// Create array type
type_info_t* type_create_array(type_info_t *element, size_t size);

// Create function type
type_info_t* type_create_function(type_info_t **param_types, size_t param_count,
                                 type_info_t *return_type);

// Check type compatibility
int type_compatible(type_info_t *a, type_info_t *b);

// Free type
void type_free(type_info_t *type);

// ============================================================================
// Type Printing
// ============================================================================

// Get type name
const char* type_kind_name(type_kind_t kind);

// Print type
void type_print(type_info_t *type);

#ifdef __cplusplus
}
#endif

#endif // VITTE_SEMANTIC_H
