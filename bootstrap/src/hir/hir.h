#ifndef VITTE_BOOTSTRAP_HIR_H
#define VITTE_BOOTSTRAP_HIR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../api/error.h"
#include "../arena/arena.h"
#include "../ast/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_HIR_DEFAULT_MAX_DEPTH ((size_t)256u)

typedef uint32_t vitte_hir_node_id_t;

typedef enum vitte_hir_kind {
    VITTE_HIR_ERROR = 0,
    VITTE_HIR_MODULE,
    VITTE_HIR_FUNCTION,
    VITTE_HIR_CONST_DECL,
    VITTE_HIR_PICK_DECL,
    VITTE_HIR_PICK_VARIANT,
    VITTE_HIR_FORM_DECL,
    VITTE_HIR_FORM_FIELD,
    VITTE_HIR_LIST_EXPR,
    VITTE_HIR_RECORD_EXPR,
    VITTE_HIR_RECORD_FIELD,
    VITTE_HIR_BLOCK,
    VITTE_HIR_RETURN_STMT,
    VITTE_HIR_LET_STMT,
    VITTE_HIR_ASSIGN_STMT,
    VITTE_HIR_EXPR_STMT,
    VITTE_HIR_IF_STMT,
    VITTE_HIR_WHILE_STMT,
    VITTE_HIR_INTEGER_LITERAL,
    VITTE_HIR_STRING_LITERAL,
    VITTE_HIR_VARIABLE,
    VITTE_HIR_BINARY_EXPR,
    VITTE_HIR_CALL_EXPR,
    VITTE_HIR_IF_EXPR,
    VITTE_HIR_BLOCK_EXPR,
    VITTE_HIR_TYPE_NAME,
    VITTE_HIR_KIND_COUNT
} vitte_hir_kind_t;

typedef struct vitte_hir_node vitte_hir_node_t;
typedef vitte_hir_node_t vitte_hir_module_t;
typedef vitte_hir_node_t vitte_hir_decl_t;
typedef vitte_hir_node_t vitte_hir_function_t;
typedef vitte_hir_node_t vitte_hir_block_t;
typedef vitte_hir_node_t vitte_hir_stmt_t;
typedef vitte_hir_node_t vitte_hir_expr_t;
typedef vitte_hir_node_t vitte_hir_type_t;

typedef struct vitte_hir_list {
    vitte_hir_node_t *first;
    vitte_hir_node_t *last;
    size_t count;
} vitte_hir_list_t;

struct vitte_hir_node {
    vitte_hir_node_id_t id;
    vitte_hir_kind_t kind;
    vitte_hir_node_t *next;
    const vitte_ast_node_t *source;
    vitte_hir_type_t *type;

    union {
        struct {
            const char *name;
            vitte_hir_list_t declarations;
        } module;

        struct {
            const char *name;
            const char *source_name;
            const char *lowered_name;
            vitte_hir_list_t parameters;
            vitte_hir_type_t *return_type;
            vitte_hir_block_t *body;
        } function;

        struct {
            const char *name;
            const char *source_name;
            const char *lowered_name;
            vitte_hir_type_t *declared_type;
            vitte_hir_expr_t *value;
        } const_decl;

        struct {
            const char *name;
            vitte_hir_list_t variants;
        } pick_decl;

        struct {
            const char *name;
        } pick_variant;

        struct {
            const char *name;
            vitte_hir_list_t fields;
        } form_decl;

        struct {
            const char *name;
            vitte_hir_type_t *type;
        } form_field;

        struct { vitte_hir_list_t elements; } list_expr;
        struct { const char *type_name; vitte_hir_list_t fields; } record_expr;
        struct { const char *name; vitte_hir_expr_t *value; } record_field;

        struct {
            vitte_hir_list_t statements;
        } block;

        struct {
            vitte_hir_expr_t *value;
        } return_stmt;

        struct {
            const char *name;
            vitte_hir_type_t *declared_type;
            vitte_hir_expr_t *value;
        } let_stmt;

        struct {
            const char *name;
            vitte_hir_expr_t *value;
        } assign_stmt;

        struct {
            vitte_hir_expr_t *value;
        } expr_stmt;

        struct {
            vitte_hir_expr_t *condition;
            vitte_hir_stmt_t *then_branch;
            vitte_hir_stmt_t *else_branch;
        } if_stmt;
        struct { vitte_hir_expr_t *condition; vitte_hir_stmt_t *body; } while_stmt;

        struct {
            int64_t value;
        } integer_literal;

        struct {
            const char *value;
        } string_literal;

        struct {
            const char *name;
            const char *source_name;
            const char *lowered_name;
        } variable;

        struct {
            const char *operator_text;
            vitte_hir_expr_t *left;
            vitte_hir_expr_t *right;
        } binary_expr;

        struct {
            vitte_hir_expr_t *callee;
            vitte_hir_list_t arguments;
        } call_expr;

        struct { vitte_hir_expr_t *condition; vitte_hir_expr_t *then_value; vitte_hir_expr_t *else_value; } if_expr;
        struct { vitte_hir_list_t statements; vitte_hir_expr_t *value; } block_expr;

        struct {
            const char *name;
        } type_name;

        struct {
            const char *message;
        } error_node;
    } as;
};

typedef struct vitte_hir {
    bool initialized;
    bool owns_arena;
    vitte_arena_t *arena;
    vitte_arena_t owned_arena;
    vitte_hir_module_t *root;
    vitte_hir_node_id_t next_id;
    size_t node_count;
    vitte_error_t last_error;
} vitte_hir_t;

typedef struct vitte_hir_builder {
    vitte_hir_t *hir;
} vitte_hir_builder_t;

typedef struct vitte_hir_lowering {
    vitte_hir_t *hir;
    size_t max_depth;
    vitte_error_t last_error;
} vitte_hir_lowering_t;

typedef bool (*vitte_hir_visit_fn)(vitte_hir_node_t *node, void *user);

void vitte_hir_list_init(vitte_hir_list_t *list);
bool vitte_hir_list_append(vitte_hir_list_t *list, vitte_hir_node_t *node);

vitte_status_t vitte_hir_init(vitte_hir_t *hir, vitte_arena_t *arena);
vitte_status_t vitte_hir_init_owned(vitte_hir_t *hir, const vitte_arena_config_t *config);
void vitte_hir_destroy(vitte_hir_t *hir);
bool vitte_hir_is_initialized(const vitte_hir_t *hir);
const vitte_error_t *vitte_hir_last_error(const vitte_hir_t *hir);
void vitte_hir_clear_error(vitte_hir_t *hir);

vitte_hir_node_t *vitte_hir_alloc_node(
    vitte_hir_t *hir,
    vitte_hir_kind_t kind,
    const vitte_ast_node_t *source
);

bool vitte_hir_kind_is_valid(vitte_hir_kind_t kind);
const char *vitte_hir_kind_name(vitte_hir_kind_t kind);
const char *vitte_hir_node_label(const vitte_hir_node_t *node);
size_t vitte_hir_node_count(const vitte_hir_t *hir);
size_t vitte_hir_function_count(const vitte_hir_t *hir);

void vitte_hir_builder_init(vitte_hir_builder_t *builder, vitte_hir_t *hir);
vitte_hir_module_t *vitte_hir_make_module(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source);
vitte_hir_function_t *vitte_hir_make_function(vitte_hir_builder_t *builder, const char *name, vitte_hir_type_t *return_type, vitte_hir_block_t *body, const vitte_ast_node_t *source);
vitte_hir_decl_t *vitte_hir_make_const(vitte_hir_builder_t *builder, const char *name, vitte_hir_type_t *type, vitte_hir_expr_t *value, const vitte_ast_node_t *source);
vitte_hir_decl_t *vitte_hir_make_pick(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source);
vitte_hir_node_t *vitte_hir_make_pick_variant(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source);
vitte_hir_decl_t *vitte_hir_make_form(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source);
vitte_hir_node_t *vitte_hir_make_form_field(vitte_hir_builder_t *builder, const char *name, vitte_hir_type_t *type, const vitte_ast_node_t *source);
vitte_hir_expr_t *vitte_hir_make_list(vitte_hir_builder_t *builder, const vitte_ast_node_t *source);
vitte_hir_expr_t *vitte_hir_make_record(vitte_hir_builder_t *builder, const char *type_name, const vitte_ast_node_t *source);
vitte_hir_node_t *vitte_hir_make_record_field(vitte_hir_builder_t *builder, const char *name, vitte_hir_expr_t *value, const vitte_ast_node_t *source);
vitte_hir_block_t *vitte_hir_make_block(vitte_hir_builder_t *builder, const vitte_ast_node_t *source);
vitte_hir_stmt_t *vitte_hir_make_return(vitte_hir_builder_t *builder, vitte_hir_expr_t *value, const vitte_ast_node_t *source);
vitte_hir_stmt_t *vitte_hir_make_let(vitte_hir_builder_t *builder, const char *name, vitte_hir_type_t *type, vitte_hir_expr_t *value, const vitte_ast_node_t *source);
vitte_hir_stmt_t *vitte_hir_make_assign(vitte_hir_builder_t *builder, const char *name, vitte_hir_expr_t *value, const vitte_ast_node_t *source);
vitte_hir_stmt_t *vitte_hir_make_expr_stmt(vitte_hir_builder_t *builder, vitte_hir_expr_t *value, const vitte_ast_node_t *source);
vitte_hir_stmt_t *vitte_hir_make_if(vitte_hir_builder_t *builder, vitte_hir_expr_t *condition, vitte_hir_stmt_t *then_branch, vitte_hir_stmt_t *else_branch, const vitte_ast_node_t *source);
vitte_hir_expr_t *vitte_hir_make_integer_literal(vitte_hir_builder_t *builder, int64_t value, const vitte_ast_node_t *source);
vitte_hir_expr_t *vitte_hir_make_string_literal(vitte_hir_builder_t *builder, const char *value, const vitte_ast_node_t *source);
vitte_hir_expr_t *vitte_hir_make_variable(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source);
vitte_hir_expr_t *vitte_hir_make_binary(vitte_hir_builder_t *builder, const char *operator_text, vitte_hir_expr_t *left, vitte_hir_expr_t *right, const vitte_ast_node_t *source);
vitte_hir_expr_t *vitte_hir_make_call(vitte_hir_builder_t *builder, vitte_hir_expr_t *callee, const vitte_ast_node_t *source);
vitte_hir_expr_t *vitte_hir_make_if_expr(vitte_hir_builder_t *builder, vitte_hir_expr_t *condition, vitte_hir_expr_t *then_value, vitte_hir_expr_t *else_value, const vitte_ast_node_t *source);
vitte_hir_stmt_t *vitte_hir_make_while(vitte_hir_builder_t *builder, vitte_hir_expr_t *condition, vitte_hir_stmt_t *body, const vitte_ast_node_t *source);
vitte_hir_expr_t *vitte_hir_make_block_expr(vitte_hir_builder_t *builder, vitte_hir_list_t statements, vitte_hir_expr_t *value, const vitte_ast_node_t *source);
vitte_hir_type_t *vitte_hir_make_type_name(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source);
vitte_hir_node_t *vitte_hir_make_error(vitte_hir_builder_t *builder, const char *message, const vitte_ast_node_t *source);

bool vitte_hir_module_add_decl(vitte_hir_module_t *module, vitte_hir_decl_t *decl);
bool vitte_hir_module_add_function(vitte_hir_module_t *module, vitte_hir_function_t *function);
bool vitte_hir_module_add_const(vitte_hir_module_t *module, vitte_hir_decl_t *decl);
bool vitte_hir_function_add_param(vitte_hir_function_t *function, vitte_hir_node_t *param);
bool vitte_hir_block_add_stmt(vitte_hir_block_t *block, vitte_hir_stmt_t *stmt);
bool vitte_hir_call_add_arg(vitte_hir_expr_t *call, vitte_hir_expr_t *argument);

void vitte_hir_lowering_init(vitte_hir_lowering_t *lowering, vitte_hir_t *hir);
const vitte_error_t *vitte_hir_lowering_last_error(const vitte_hir_lowering_t *lowering);
vitte_status_t vitte_hir_lower_ast(vitte_hir_t *hir, const vitte_ast_t *ast);
vitte_status_t vitte_hir_lower_ast_with_options(
    vitte_hir_lowering_t *lowering,
    const vitte_ast_t *ast
);

vitte_status_t vitte_hir_validate(vitte_hir_t *hir);
size_t vitte_hir_visit(vitte_hir_node_t *node, vitte_hir_visit_fn callback, void *user, size_t max_depth);
void vitte_hir_dump(const vitte_hir_node_t *node, FILE *stream, size_t max_depth);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_HIR_H */
