#ifndef VITTE_BOOTSTRAP_AST_H
#define VITTE_BOOTSTRAP_AST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#include "../api/error.h"
#include "../arena/arena.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_ast_node_kind {
    VITTE_AST_NODE_ERROR = 0,
    VITTE_AST_NODE_MODULE,
    VITTE_AST_NODE_IMPORT_DECL,
    VITTE_AST_NODE_EXPORT_DECL,
    VITTE_AST_NODE_PROC_DECL,
    VITTE_AST_NODE_PARAM_DECL,
    VITTE_AST_NODE_CONST_DECL,
    VITTE_AST_NODE_PICK_DECL,
    VITTE_AST_NODE_PICK_VARIANT,
    VITTE_AST_NODE_FORM_DECL,
    VITTE_AST_NODE_FORM_FIELD,
    VITTE_AST_NODE_BLOCK_STMT,
    VITTE_AST_NODE_GIVE_STMT,
    VITTE_AST_NODE_LET_STMT,
    VITTE_AST_NODE_ASSIGN_STMT,
    VITTE_AST_NODE_EXPR_STMT,
    VITTE_AST_NODE_IF_STMT,
    VITTE_AST_NODE_INTEGER_LITERAL,
    VITTE_AST_NODE_STRING_LITERAL,
    VITTE_AST_NODE_IDENTIFIER,
    VITTE_AST_NODE_BINARY_EXPR,
    VITTE_AST_NODE_CALL_EXPR,
    VITTE_AST_NODE_LIST_EXPR,
    VITTE_AST_NODE_RECORD_EXPR,
    VITTE_AST_NODE_RECORD_FIELD,
    VITTE_AST_NODE_CAST_EXPR,
    VITTE_AST_NODE_INDEX_EXPR,
    VITTE_AST_NODE_TYPE_NAME,
    VITTE_AST_NODE_COUNT
} vitte_ast_node_kind_t;

typedef enum vitte_ast_import_kind {
    VITTE_AST_IMPORT_MODULE = 0,
    VITTE_AST_IMPORT_SYMBOL,
    VITTE_AST_IMPORT_GLOB
} vitte_ast_import_kind_t;

typedef struct vitte_ast_span {
    const char *source_name;
    size_t start_offset;
    size_t end_offset;
    uint32_t start_line;
    uint32_t start_column;
    uint32_t end_line;
    uint32_t end_column;
    bool valid;
} vitte_ast_span_t;

typedef struct vitte_ast_node vitte_ast_node_t;
typedef vitte_ast_node_t vitte_ast_module_t;
typedef vitte_ast_node_t vitte_ast_decl_t;
typedef vitte_ast_node_t vitte_ast_stmt_t;
typedef vitte_ast_node_t vitte_ast_expr_t;
typedef vitte_ast_node_t vitte_ast_type_ref_t;

typedef struct vitte_ast_list {
    vitte_ast_node_t *first;
    vitte_ast_node_t *last;
    size_t count;
} vitte_ast_list_t;

struct vitte_ast_node {
    vitte_ast_node_kind_t kind;
    vitte_ast_span_t span;
    vitte_ast_node_t *next;

    union {
        struct {
            const char *name;
            vitte_ast_list_t imports;
            vitte_ast_list_t exports;
            vitte_ast_list_t declarations;
            bool export_all;
        } module;

        struct {
            const char *path;
            const char *alias;
            bool relative;
            vitte_ast_import_kind_t import_kind;
        } import_decl;

        struct {
            const char *local_name;
            const char *export_name;
        } export_decl;

        struct {
            const char *name;
            bool exported;
            const char *lowered_name;
            vitte_ast_list_t parameters;
            vitte_ast_type_ref_t *return_type;
            vitte_ast_stmt_t *body;
        } proc_decl;

        struct {
            const char *name;
            vitte_ast_type_ref_t *type;
            bool mutable_value;
            bool by_ref;
        } param_decl;

        struct {
            const char *name;
            bool exported;
            const char *lowered_name;
            vitte_ast_type_ref_t *type;
            vitte_ast_expr_t *value;
        } const_decl;

        struct {
            const char *name;
            bool exported;
            vitte_ast_list_t variants;
        } pick_decl;

        struct {
            const char *name;
        } pick_variant;

        struct {
            const char *name;
            bool exported;
            vitte_ast_list_t fields;
        } form_decl;

        struct {
            const char *name;
            vitte_ast_type_ref_t *type;
        } form_field;

        struct {
            vitte_ast_list_t statements;
        } block_stmt;

        struct {
            vitte_ast_expr_t *value;
        } give_stmt;

        struct {
            const char *name;
            vitte_ast_type_ref_t *type;
            vitte_ast_expr_t *value;
            bool mutable_value;
        } let_stmt;

        struct {
            vitte_ast_expr_t *target;
            vitte_ast_expr_t *value;
        } assign_stmt;

        struct {
            vitte_ast_expr_t *value;
        } expr_stmt;

        struct {
            vitte_ast_expr_t *condition;
            vitte_ast_stmt_t *then_branch;
            vitte_ast_stmt_t *else_branch;
        } if_stmt;

        struct {
            int64_t value;
        } integer_literal;

        struct {
            const char *value;
        } string_literal;

        struct {
            const char *name;
            const char *lowered_name;
        } identifier;

        struct {
            const char *operator_text;
            vitte_ast_expr_t *left;
            vitte_ast_expr_t *right;
        } binary_expr;

        struct {
            vitte_ast_expr_t *callee;
            vitte_ast_list_t arguments;
        } call_expr;

        struct {
            vitte_ast_list_t elements;
        } list_expr;

        struct {
            const char *type_name;
            vitte_ast_list_t fields;
        } record_expr;

        struct {
            const char *name;
            vitte_ast_expr_t *value;
        } record_field;

        struct { vitte_ast_expr_t *value; vitte_ast_type_ref_t *type; } cast_expr;
        struct { vitte_ast_expr_t *base; vitte_ast_expr_t *index; } index_expr;

        struct {
            const char *name;
        } type_name;

        struct {
            const char *message;
        } error_node;
    } as;
};

typedef struct vitte_ast {
    bool initialized;
    bool owns_arena;
    vitte_arena_t *arena;
    vitte_arena_t owned_arena;
    vitte_ast_module_t *root;
    size_t node_count;
    vitte_error_t last_error;
} vitte_ast_t;

typedef struct vitte_ast_builder {
    vitte_ast_t *ast;
} vitte_ast_builder_t;

typedef bool (*vitte_ast_visit_fn)(
    vitte_ast_node_t *node,
    void *user
);
typedef bool (*vitte_ast_export_visit_fn)(
    const vitte_ast_decl_t *decl,
    const char *public_name,
    void *user
);

void vitte_ast_span_init(vitte_ast_span_t *span);
bool vitte_ast_span_is_valid(const vitte_ast_span_t *span);
bool vitte_ast_span_merge(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right,
    vitte_ast_span_t *out
);

void vitte_ast_list_init(vitte_ast_list_t *list);
bool vitte_ast_list_append(vitte_ast_list_t *list, vitte_ast_node_t *node);

vitte_status_t vitte_ast_init(vitte_ast_t *ast, vitte_arena_t *arena);
vitte_status_t vitte_ast_init_owned(vitte_ast_t *ast, const vitte_arena_config_t *config);
void vitte_ast_destroy(vitte_ast_t *ast);
bool vitte_ast_is_initialized(const vitte_ast_t *ast);

vitte_ast_node_t *vitte_ast_alloc_node(
    vitte_ast_t *ast,
    vitte_ast_node_kind_t kind,
    vitte_ast_span_t span
);

const vitte_error_t *vitte_ast_last_error(const vitte_ast_t *ast);
void vitte_ast_clear_error(vitte_ast_t *ast);
const char *vitte_ast_node_kind_name(vitte_ast_node_kind_t kind);
bool vitte_ast_node_kind_is_valid(vitte_ast_node_kind_t kind);
const char *vitte_ast_node_label(const vitte_ast_node_t *node);
const char *vitte_ast_decl_name(const vitte_ast_decl_t *decl);
const vitte_ast_decl_t *vitte_ast_module_find_decl(const vitte_ast_module_t *module, const char *name);
const vitte_ast_decl_t *vitte_ast_export_decl_target(const vitte_ast_module_t *module, const vitte_ast_decl_t *export_decl);
bool vitte_ast_module_decl_is_exported(const vitte_ast_module_t *module, const vitte_ast_decl_t *decl);
const vitte_ast_decl_t *vitte_ast_module_find_exported_decl(const vitte_ast_module_t *module, const char *export_name);
size_t vitte_ast_module_visit_exports(const vitte_ast_module_t *module, vitte_ast_export_visit_fn callback, void *user);
void vitte_ast_dump(const vitte_ast_node_t *node, FILE *stream, size_t max_depth);

void vitte_ast_builder_init(vitte_ast_builder_t *builder, vitte_ast_t *ast);
vitte_ast_module_t *vitte_ast_make_module(vitte_ast_builder_t *builder, const char *name, vitte_ast_span_t span);
vitte_ast_decl_t *vitte_ast_make_import_decl(
    vitte_ast_builder_t *builder,
    const char *path,
    const char *alias,
    bool relative,
    vitte_ast_import_kind_t import_kind,
    vitte_ast_span_t span
);
vitte_ast_decl_t *vitte_ast_make_export_decl(
    vitte_ast_builder_t *builder,
    const char *local_name,
    const char *export_name,
    vitte_ast_span_t span
);
vitte_ast_decl_t *vitte_ast_make_proc_decl(vitte_ast_builder_t *builder, const char *name, bool exported, vitte_ast_type_ref_t *return_type, vitte_ast_stmt_t *body, vitte_ast_span_t span);
vitte_ast_node_t *vitte_ast_make_param_decl(vitte_ast_builder_t *builder, const char *name, vitte_ast_type_ref_t *type, bool mutable_value, bool by_ref, vitte_ast_span_t span);
vitte_ast_decl_t *vitte_ast_make_const_decl(vitte_ast_builder_t *builder, const char *name, bool exported, vitte_ast_type_ref_t *type, vitte_ast_expr_t *value, vitte_ast_span_t span);
vitte_ast_decl_t *vitte_ast_make_pick_decl(vitte_ast_builder_t *builder, const char *name, bool exported, vitte_ast_span_t span);
vitte_ast_node_t *vitte_ast_make_pick_variant(vitte_ast_builder_t *builder, const char *name, vitte_ast_span_t span);
vitte_ast_decl_t *vitte_ast_make_form_decl(vitte_ast_builder_t *builder, const char *name, bool exported, vitte_ast_span_t span);
vitte_ast_node_t *vitte_ast_make_form_field(vitte_ast_builder_t *builder, const char *name, vitte_ast_type_ref_t *type, vitte_ast_span_t span);
vitte_ast_stmt_t *vitte_ast_make_block_stmt(vitte_ast_builder_t *builder, vitte_ast_span_t span);
vitte_ast_stmt_t *vitte_ast_make_give_stmt(vitte_ast_builder_t *builder, vitte_ast_expr_t *value, vitte_ast_span_t span);
vitte_ast_stmt_t *vitte_ast_make_let_stmt(vitte_ast_builder_t *builder, const char *name, vitte_ast_type_ref_t *type, vitte_ast_expr_t *value, bool mutable_value, vitte_ast_span_t span);
vitte_ast_stmt_t *vitte_ast_make_assign_stmt(vitte_ast_builder_t *builder, vitte_ast_expr_t *target, vitte_ast_expr_t *value, vitte_ast_span_t span);
vitte_ast_stmt_t *vitte_ast_make_expr_stmt(vitte_ast_builder_t *builder, vitte_ast_expr_t *value, vitte_ast_span_t span);
vitte_ast_stmt_t *vitte_ast_make_if_stmt(vitte_ast_builder_t *builder, vitte_ast_expr_t *condition, vitte_ast_stmt_t *then_branch, vitte_ast_stmt_t *else_branch, vitte_ast_span_t span);
vitte_ast_expr_t *vitte_ast_make_integer_literal(vitte_ast_builder_t *builder, int64_t value, vitte_ast_span_t span);
vitte_ast_expr_t *vitte_ast_make_string_literal(vitte_ast_builder_t *builder, const char *value, vitte_ast_span_t span);
vitte_ast_expr_t *vitte_ast_make_identifier(vitte_ast_builder_t *builder, const char *name, vitte_ast_span_t span);
vitte_ast_expr_t *vitte_ast_make_binary_expr(vitte_ast_builder_t *builder, const char *operator_text, vitte_ast_expr_t *left, vitte_ast_expr_t *right, vitte_ast_span_t span);
vitte_ast_expr_t *vitte_ast_make_call_expr(vitte_ast_builder_t *builder, vitte_ast_expr_t *callee, vitte_ast_span_t span);
vitte_ast_expr_t *vitte_ast_make_list_expr(vitte_ast_builder_t *builder, vitte_ast_span_t span);
vitte_ast_expr_t *vitte_ast_make_record_expr(vitte_ast_builder_t *builder, const char *type_name, vitte_ast_span_t span);
vitte_ast_node_t *vitte_ast_make_record_field(vitte_ast_builder_t *builder, const char *name, vitte_ast_expr_t *value, vitte_ast_span_t span);
vitte_ast_expr_t *vitte_ast_make_cast_expr(vitte_ast_builder_t *builder, vitte_ast_expr_t *value, vitte_ast_type_ref_t *type, vitte_ast_span_t span);
vitte_ast_expr_t *vitte_ast_make_index_expr(vitte_ast_builder_t *builder, vitte_ast_expr_t *base, vitte_ast_expr_t *index, vitte_ast_span_t span);
vitte_ast_type_ref_t *vitte_ast_make_type_name(vitte_ast_builder_t *builder, const char *name, vitte_ast_span_t span);
vitte_ast_node_t *vitte_ast_make_error(vitte_ast_builder_t *builder, const char *message, vitte_ast_span_t span);

bool vitte_ast_module_add_decl(vitte_ast_module_t *module, vitte_ast_decl_t *decl);
bool vitte_ast_module_add_import(vitte_ast_module_t *module, vitte_ast_decl_t *import_decl);
bool vitte_ast_module_add_export(vitte_ast_module_t *module, vitte_ast_decl_t *export_decl);
void vitte_ast_module_set_export_all(vitte_ast_module_t *module, bool enabled);
bool vitte_ast_proc_add_param(vitte_ast_decl_t *proc, vitte_ast_node_t *param);
bool vitte_ast_block_add_stmt(vitte_ast_stmt_t *block, vitte_ast_stmt_t *stmt);
bool vitte_ast_call_add_arg(vitte_ast_expr_t *call, vitte_ast_expr_t *argument);

vitte_status_t vitte_ast_validate(vitte_ast_t *ast);
size_t vitte_ast_visit(vitte_ast_node_t *node, vitte_ast_visit_fn callback, void *user, size_t max_depth);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_AST_H */
