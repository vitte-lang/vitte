#ifndef VITTE_BOOTSTRAP_IR_H
#define VITTE_BOOTSTRAP_IR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../api/error.h"
#include "../arena/arena.h"
#include "../hir/hir.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_IR_MAX_OPERANDS ((size_t)32u)
#define VITTE_IR_DEFAULT_MAX_DEPTH ((size_t)256u)

typedef uint32_t vitte_ir_value_id_t;
typedef uint32_t vitte_ir_block_id_t;
typedef uint32_t vitte_ir_function_id_t;

typedef enum vitte_ir_type_kind {
    VITTE_IR_TYPE_ERROR = 0,
    VITTE_IR_TYPE_VOID,
    VITTE_IR_TYPE_BOOL,
    VITTE_IR_TYPE_I32,
    VITTE_IR_TYPE_I64,
    VITTE_IR_TYPE_USIZE,
    VITTE_IR_TYPE_STRING_PTR,
    VITTE_IR_TYPE_UNKNOWN,
    VITTE_IR_TYPE_COUNT
} vitte_ir_type_kind_t;

typedef enum vitte_ir_value_kind {
    VITTE_IR_VALUE_ERROR = 0,
    VITTE_IR_VALUE_CONST_INT,
    VITTE_IR_VALUE_CONST_STRING,
    VITTE_IR_VALUE_PARAMETER,
    VITTE_IR_VALUE_INSTRUCTION,
    VITTE_IR_VALUE_FUNCTION_REF,
    VITTE_IR_VALUE_LOCAL,
    VITTE_IR_VALUE_COUNT
} vitte_ir_value_kind_t;

typedef enum vitte_ir_opcode {
    VITTE_IR_OP_ERROR = 0,
    VITTE_IR_OP_CONST_INT,
    VITTE_IR_OP_CONST_STRING,
    VITTE_IR_OP_LOCAL,
    VITTE_IR_OP_STORE,
    VITTE_IR_OP_LOAD,
    VITTE_IR_OP_CAST,
    VITTE_IR_OP_BINARY,
    VITTE_IR_OP_SELECT,
    VITTE_IR_OP_CALL,
    VITTE_IR_OP_RETURN,
    VITTE_IR_OP_BRANCH,
    VITTE_IR_OP_COND_BRANCH,
    VITTE_IR_OP_UNREACHABLE,
    VITTE_IR_OP_COUNT
} vitte_ir_opcode_t;

typedef struct vitte_ir_type vitte_ir_type_t;
typedef struct vitte_ir_value vitte_ir_value_t;
typedef struct vitte_ir_instruction vitte_ir_instruction_t;
typedef struct vitte_ir_block vitte_ir_block_t;
typedef struct vitte_ir_function vitte_ir_function_t;
typedef struct vitte_ir_global vitte_ir_global_t;
typedef struct vitte_ir_pick_variant vitte_ir_pick_variant_t;
typedef struct vitte_ir_pick vitte_ir_pick_t;
typedef struct vitte_ir_form_field vitte_ir_form_field_t;
typedef struct vitte_ir_form vitte_ir_form_t;
typedef struct vitte_ir_module vitte_ir_module_t;
typedef struct vitte_ir_local_binding vitte_ir_local_binding_t;
typedef struct vitte_ir_function_binding vitte_ir_function_binding_t;
typedef struct vitte_ir_global_binding vitte_ir_global_binding_t;
typedef struct vitte_ir_scope_marker vitte_ir_scope_marker_t;

struct vitte_ir_type {
    vitte_ir_type_kind_t kind;
};

struct vitte_ir_value {
    vitte_ir_value_id_t id;
    vitte_ir_value_kind_t kind;
    vitte_ir_type_t *type;
    const char *name;
    vitte_ir_instruction_t *definition;
    vitte_ir_value_t *next;
    union {
        int64_t int_value;
        const char *string_value;
        vitte_ir_function_t *function;
    } as;
};

struct vitte_ir_instruction {
    vitte_ir_opcode_t opcode;
    vitte_ir_value_t *result;
    vitte_ir_value_t *operands[VITTE_IR_MAX_OPERANDS];
    size_t operand_count;
    vitte_ir_type_t *type;
    const char *operator_text;
    vitte_ir_block_t *target;
    vitte_ir_block_t *else_target;
    const vitte_hir_node_t *source;
    vitte_ir_instruction_t *next;
};

struct vitte_ir_block {
    vitte_ir_block_id_t id;
    const char *name;
    vitte_ir_instruction_t *first;
    vitte_ir_instruction_t *last;
    size_t instruction_count;
    bool terminated;
    const vitte_hir_node_t *source;
    vitte_ir_block_t *next;
};

struct vitte_ir_function {
    vitte_ir_function_id_t id;
    const char *name;
    vitte_ir_type_t *return_type;
    vitte_ir_value_t *first_parameter;
    vitte_ir_value_t *last_parameter;
    size_t parameter_count;
    vitte_ir_block_t *entry;
    vitte_ir_block_t *first_block;
    vitte_ir_block_t *last_block;
    size_t block_count;
    const vitte_hir_node_t *source;
    vitte_ir_function_t *next;
};

struct vitte_ir_global {
    const char *name;
    vitte_ir_type_t *type;
    vitte_ir_value_t *initializer;
    const vitte_hir_node_t *source;
    bool resolving;
    bool initialized;
    vitte_ir_global_t *next;
};

struct vitte_ir_pick_variant {
    const char *name;
    vitte_ir_pick_variant_t *next;
};

struct vitte_ir_pick {
    const char *name;
    vitte_ir_pick_variant_t *first_variant;
    vitte_ir_pick_variant_t *last_variant;
    size_t variant_count;
    const vitte_hir_node_t *source;
    vitte_ir_pick_t *next;
};

struct vitte_ir_form_field {
    const char *name;
    vitte_ir_type_t *type;
    vitte_ir_form_field_t *next;
};

struct vitte_ir_form {
    const char *name;
    vitte_ir_form_field_t *first_field;
    vitte_ir_form_field_t *last_field;
    size_t field_count;
    const vitte_hir_node_t *source;
    vitte_ir_form_t *next;
};

struct vitte_ir_module {
    const char *name;
    vitte_ir_global_t *first_global;
    vitte_ir_global_t *last_global;
    size_t global_count;
    vitte_ir_pick_t *first_pick;
    vitte_ir_pick_t *last_pick;
    size_t pick_count;
    vitte_ir_form_t *first_form;
    vitte_ir_form_t *last_form;
    size_t form_count;
    vitte_ir_function_t *first_function;
    vitte_ir_function_t *last_function;
    size_t function_count;
};

typedef struct vitte_ir {
    bool initialized;
    bool owns_arena;
    vitte_arena_t *arena;
    vitte_arena_t owned_arena;
    vitte_ir_module_t *module;
    vitte_ir_value_id_t next_value_id;
    vitte_ir_block_id_t next_block_id;
    vitte_ir_function_id_t next_function_id;
    size_t value_count;
    size_t block_count;
    size_t function_count;
    size_t instruction_count;
    vitte_error_t last_error;
} vitte_ir_t;

typedef struct vitte_ir_builder {
    vitte_ir_t *ir;
    vitte_ir_function_t *function;
    vitte_ir_block_t *block;
} vitte_ir_builder_t;

typedef struct vitte_ir_lowering {
    vitte_ir_t *ir;
    vitte_ir_builder_t builder;
    size_t max_depth;
    vitte_ir_local_binding_t *locals;
    vitte_ir_function_binding_t *functions;
    vitte_ir_global_binding_t *globals;
    vitte_ir_scope_marker_t *scopes;
    vitte_error_t last_error;
} vitte_ir_lowering_t;

vitte_status_t vitte_ir_init(vitte_ir_t *ir, vitte_arena_t *arena);
vitte_status_t vitte_ir_init_owned(vitte_ir_t *ir, const vitte_arena_config_t *config);
void vitte_ir_destroy(vitte_ir_t *ir);
bool vitte_ir_is_initialized(const vitte_ir_t *ir);
const vitte_error_t *vitte_ir_last_error(const vitte_ir_t *ir);
void vitte_ir_clear_error(vitte_ir_t *ir);

bool vitte_ir_type_kind_is_valid(vitte_ir_type_kind_t kind);
bool vitte_ir_value_kind_is_valid(vitte_ir_value_kind_t kind);
bool vitte_ir_opcode_is_valid(vitte_ir_opcode_t opcode);
const char *vitte_ir_type_name(const vitte_ir_type_t *type);
const char *vitte_ir_opcode_name(vitte_ir_opcode_t opcode);
const char *vitte_ir_value_label(const vitte_ir_value_t *value);
bool vitte_ir_type_equals(const vitte_ir_type_t *left, const vitte_ir_type_t *right);

vitte_ir_type_t *vitte_ir_make_type(vitte_ir_t *ir, vitte_ir_type_kind_t kind);
vitte_ir_type_t *vitte_ir_type_from_hir(vitte_ir_t *ir, const vitte_hir_node_t *hir_type);

void vitte_ir_builder_init(vitte_ir_builder_t *builder, vitte_ir_t *ir);
vitte_ir_module_t *vitte_ir_make_module(vitte_ir_builder_t *builder, const char *name);
vitte_ir_global_t *vitte_ir_make_global(vitte_ir_builder_t *builder, const char *name, vitte_ir_type_t *type, const vitte_hir_node_t *source);
vitte_ir_pick_t *vitte_ir_make_pick(vitte_ir_builder_t *builder, const char *name, const vitte_hir_node_t *source);
vitte_ir_pick_variant_t *vitte_ir_make_pick_variant(vitte_ir_builder_t *builder, const char *name);
vitte_ir_form_t *vitte_ir_make_form(vitte_ir_builder_t *builder, const char *name, const vitte_hir_node_t *source);
vitte_ir_form_field_t *vitte_ir_make_form_field(vitte_ir_builder_t *builder, const char *name, vitte_ir_type_t *type);
bool vitte_ir_module_add_global(vitte_ir_module_t *module, vitte_ir_global_t *global);
bool vitte_ir_module_add_pick(vitte_ir_module_t *module, vitte_ir_pick_t *pick);
bool vitte_ir_pick_add_variant(vitte_ir_pick_t *pick, vitte_ir_pick_variant_t *variant);
bool vitte_ir_module_add_form(vitte_ir_module_t *module, vitte_ir_form_t *form);
bool vitte_ir_form_add_field(vitte_ir_form_t *form, vitte_ir_form_field_t *field);
vitte_ir_function_t *vitte_ir_make_function(vitte_ir_builder_t *builder, const char *name, vitte_ir_type_t *return_type, const vitte_hir_node_t *source);
bool vitte_ir_module_add_function(vitte_ir_module_t *module, vitte_ir_function_t *function);
vitte_ir_value_t *vitte_ir_make_parameter(vitte_ir_builder_t *builder, const char *name, vitte_ir_type_t *type);
bool vitte_ir_function_add_parameter(vitte_ir_function_t *function, vitte_ir_value_t *parameter);
vitte_ir_block_t *vitte_ir_make_block(vitte_ir_builder_t *builder, const char *name, const vitte_hir_node_t *source);
bool vitte_ir_function_add_block(vitte_ir_function_t *function, vitte_ir_block_t *block);
void vitte_ir_builder_position_at_end(vitte_ir_builder_t *builder, vitte_ir_function_t *function, vitte_ir_block_t *block);

vitte_ir_value_t *vitte_ir_make_value(vitte_ir_t *ir, vitte_ir_value_kind_t kind, vitte_ir_type_t *type, const char *name);
vitte_ir_instruction_t *vitte_ir_emit_instruction(
    vitte_ir_builder_t *builder,
    vitte_ir_opcode_t opcode,
    vitte_ir_type_t *type,
    const vitte_hir_node_t *source
);
vitte_ir_value_t *vitte_ir_emit_const_int(vitte_ir_builder_t *builder, int64_t value, vitte_ir_type_t *type, const vitte_hir_node_t *source);
vitte_ir_value_t *vitte_ir_emit_const_string(vitte_ir_builder_t *builder, const char *value, const vitte_hir_node_t *source);
vitte_ir_value_t *vitte_ir_emit_local(vitte_ir_builder_t *builder, const char *name, vitte_ir_type_t *type, const vitte_hir_node_t *source);
vitte_ir_instruction_t *vitte_ir_emit_store(vitte_ir_builder_t *builder, vitte_ir_value_t *local, vitte_ir_value_t *value, const vitte_hir_node_t *source);
vitte_ir_value_t *vitte_ir_emit_load(vitte_ir_builder_t *builder, vitte_ir_value_t *local, const vitte_hir_node_t *source);
vitte_ir_value_t *vitte_ir_emit_cast(vitte_ir_builder_t *builder, vitte_ir_value_t *value, vitte_ir_type_t *target_type, const vitte_hir_node_t *source);
vitte_ir_value_t *vitte_ir_emit_binary(vitte_ir_builder_t *builder, const char *operator_text, vitte_ir_value_t *left, vitte_ir_value_t *right, const vitte_hir_node_t *source);
vitte_ir_value_t *vitte_ir_emit_select(vitte_ir_builder_t *builder, vitte_ir_value_t *condition, vitte_ir_value_t *then_value, vitte_ir_value_t *else_value, const vitte_hir_node_t *source);
vitte_ir_value_t *vitte_ir_emit_call(vitte_ir_builder_t *builder, vitte_ir_value_t *callee, vitte_ir_value_t *const *arguments, size_t argument_count, vitte_ir_type_t *return_type, const vitte_hir_node_t *source);
vitte_ir_instruction_t *vitte_ir_emit_return(vitte_ir_builder_t *builder, vitte_ir_value_t *value, const vitte_hir_node_t *source);
vitte_ir_instruction_t *vitte_ir_emit_branch(vitte_ir_builder_t *builder, vitte_ir_block_t *target, const vitte_hir_node_t *source);
vitte_ir_instruction_t *vitte_ir_emit_cond_branch(vitte_ir_builder_t *builder, vitte_ir_value_t *condition, vitte_ir_block_t *then_target, vitte_ir_block_t *else_target, const vitte_hir_node_t *source);
vitte_ir_instruction_t *vitte_ir_emit_unreachable(vitte_ir_builder_t *builder, const vitte_hir_node_t *source);

void vitte_ir_lowering_init(vitte_ir_lowering_t *lowering, vitte_ir_t *ir);
const vitte_error_t *vitte_ir_lowering_last_error(const vitte_ir_lowering_t *lowering);
vitte_status_t vitte_ir_lower_hir(vitte_ir_t *ir, const vitte_hir_t *hir);
vitte_status_t vitte_ir_lower_hir_with_options(vitte_ir_lowering_t *lowering, const vitte_hir_t *hir);

vitte_status_t vitte_ir_validate(vitte_ir_t *ir);
void vitte_ir_dump(const vitte_ir_t *ir, FILE *stream);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_IR_H */
