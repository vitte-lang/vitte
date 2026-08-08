#include "ir.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "../builtin/builtin.h"

static void vitte_ir_set_error(vitte_ir_t *ir, vitte_status_t status, const char *code, const char *message, const char *details) {
    if (ir != NULL) {
        vitte_error_set_details(&ir->last_error, status, code, message, details);
    }
}

static void vitte_ir_lowering_set_error(vitte_ir_lowering_t *lowering, vitte_status_t status, const char *code, const char *message, const char *details) {
    if (lowering != NULL) {
        vitte_error_set_details(&lowering->last_error, status, code, message, details);
        vitte_ir_set_error(lowering->ir, status, code, message, details);
    }
}

struct vitte_ir_local_binding {
    const char *name;
    vitte_ir_value_t *value;
    vitte_ir_local_binding_t *next;
};

struct vitte_ir_function_binding {
    const char *name;
    vitte_ir_function_t *function;
    vitte_ir_function_binding_t *next;
};

struct vitte_ir_global_binding {
    const char *name;
    vitte_ir_global_t *global;
    vitte_ir_global_binding_t *next;
};

struct vitte_ir_scope_marker {
    vitte_ir_local_binding_t *locals;
    vitte_ir_scope_marker_t *next;
};

static vitte_ir_type_t *vitte_ir_type_from_builtin(vitte_ir_t *ir, vitte_builtin_type_kind_t kind) {
    switch (kind) {
        case VITTE_BUILTIN_TYPE_VOID:
        case VITTE_BUILTIN_TYPE_NEVER:
            return vitte_ir_make_type(ir, VITTE_IR_TYPE_VOID);
        case VITTE_BUILTIN_TYPE_BOOL:
            return vitte_ir_make_type(ir, VITTE_IR_TYPE_BOOL);
        case VITTE_BUILTIN_TYPE_U8:
        case VITTE_BUILTIN_TYPE_U32:
        case VITTE_BUILTIN_TYPE_U64:
        case VITTE_BUILTIN_TYPE_USIZE:
            return vitte_ir_make_type(ir, VITTE_IR_TYPE_USIZE);
        case VITTE_BUILTIN_TYPE_I64:
            return vitte_ir_make_type(ir, VITTE_IR_TYPE_I64);
        case VITTE_BUILTIN_TYPE_INT:
            return vitte_ir_make_type(ir, VITTE_IR_TYPE_I32);
        case VITTE_BUILTIN_TYPE_STRING:
            return vitte_ir_make_type(ir, VITTE_IR_TYPE_STRING_PTR);
        case VITTE_BUILTIN_TYPE_ERROR:
            return vitte_ir_make_type(ir, VITTE_IR_TYPE_ERROR);
        case VITTE_BUILTIN_TYPE_F32:
        case VITTE_BUILTIN_TYPE_F64:
        case VITTE_BUILTIN_TYPE_COUNT:
        default:
            return vitte_ir_make_type(ir, VITTE_IR_TYPE_UNKNOWN);
    }
}

static vitte_ir_value_t *vitte_ir_make_const_int_value(vitte_ir_t *ir, int64_t value, vitte_ir_type_t *type) {
    vitte_ir_value_t *result;

    if (type == NULL) {
        type = vitte_ir_make_type(ir, VITTE_IR_TYPE_I32);
    }
    result = vitte_ir_make_value(ir, VITTE_IR_VALUE_CONST_INT, type, NULL);
    if (result != NULL) {
        result->as.int_value = value;
    }
    return result;
}

static vitte_ir_value_t *vitte_ir_make_const_string_value(vitte_ir_t *ir, const char *value) {
    vitte_ir_value_t *result = vitte_ir_make_value(ir, VITTE_IR_VALUE_CONST_STRING, vitte_ir_make_type(ir, VITTE_IR_TYPE_STRING_PTR), NULL);
    if (result != NULL) {
        result->as.string_value = value;
    }
    return result;
}

static vitte_ir_value_t *vitte_ir_make_function_ref_value(vitte_ir_t *ir, const char *name, vitte_ir_function_t *function, vitte_ir_type_t *type) {
    vitte_ir_value_t *result = vitte_ir_make_value(ir, VITTE_IR_VALUE_FUNCTION_REF, type != NULL ? type : vitte_ir_make_type(ir, VITTE_IR_TYPE_UNKNOWN), name);
    if (result != NULL) {
        result->as.function = function;
    }
    return result;
}

static bool vitte_ir_scope_push(vitte_ir_lowering_t *lowering) {
    vitte_ir_scope_marker_t *marker;

    if (lowering == NULL || !vitte_ir_is_initialized(lowering->ir)) {
        return false;
    }
    marker = (vitte_ir_scope_marker_t *)vitte_arena_alloc_zeroed(lowering->ir->arena, sizeof(*marker), _Alignof(vitte_ir_scope_marker_t));
    if (marker == NULL) {
        vitte_error_copy(&lowering->last_error, vitte_arena_last_error(lowering->ir->arena));
        vitte_error_copy(&lowering->ir->last_error, vitte_arena_last_error(lowering->ir->arena));
        return false;
    }
    marker->locals = lowering->locals;
    marker->next = lowering->scopes;
    lowering->scopes = marker;
    return true;
}

static void vitte_ir_scope_pop(vitte_ir_lowering_t *lowering) {
    if (lowering == NULL || lowering->scopes == NULL) {
        return;
    }
    lowering->locals = lowering->scopes->locals;
    lowering->scopes = lowering->scopes->next;
}

static bool vitte_ir_bind_local(vitte_ir_lowering_t *lowering, const char *name, vitte_ir_value_t *value) {
    vitte_ir_local_binding_t *binding;

    if (lowering == NULL || name == NULL || value == NULL) {
        return false;
    }
    binding = (vitte_ir_local_binding_t *)vitte_arena_alloc_zeroed(lowering->ir->arena, sizeof(*binding), _Alignof(vitte_ir_local_binding_t));
    if (binding == NULL) {
        vitte_error_copy(&lowering->last_error, vitte_arena_last_error(lowering->ir->arena));
        vitte_error_copy(&lowering->ir->last_error, vitte_arena_last_error(lowering->ir->arena));
        return false;
    }
    binding->name = name;
    binding->value = value;
    binding->next = lowering->locals;
    lowering->locals = binding;
    return true;
}

static vitte_ir_value_t *vitte_ir_lookup_local(const vitte_ir_lowering_t *lowering, const char *name) {
    const vitte_ir_local_binding_t *binding;

    if (lowering == NULL || name == NULL) {
        return NULL;
    }
    for (binding = lowering->locals; binding != NULL; binding = binding->next) {
        if (binding->name != NULL && strcmp(binding->name, name) == 0) {
            return binding->value;
        }
    }
    return NULL;
}

static bool vitte_ir_bind_function(vitte_ir_lowering_t *lowering, const char *name, vitte_ir_function_t *function) {
    vitte_ir_function_binding_t *binding;

    if (lowering == NULL || name == NULL || function == NULL) {
        return false;
    }
    binding = (vitte_ir_function_binding_t *)vitte_arena_alloc_zeroed(lowering->ir->arena, sizeof(*binding), _Alignof(vitte_ir_function_binding_t));
    if (binding == NULL) {
        vitte_error_copy(&lowering->last_error, vitte_arena_last_error(lowering->ir->arena));
        vitte_error_copy(&lowering->ir->last_error, vitte_arena_last_error(lowering->ir->arena));
        return false;
    }
    binding->name = name;
    binding->function = function;
    binding->next = lowering->functions;
    lowering->functions = binding;
    return true;
}

static vitte_ir_function_t *vitte_ir_lookup_function(const vitte_ir_lowering_t *lowering, const char *name) {
    const vitte_ir_function_binding_t *binding;

    if (lowering == NULL || name == NULL) {
        return NULL;
    }
    for (binding = lowering->functions; binding != NULL; binding = binding->next) {
        if (binding->name != NULL && strcmp(binding->name, name) == 0) {
            return binding->function;
        }
    }
    return NULL;
}

static bool vitte_ir_bind_global(vitte_ir_lowering_t *lowering, const char *name, vitte_ir_global_t *global) {
    vitte_ir_global_binding_t *binding;

    if (lowering == NULL || name == NULL || global == NULL) {
        return false;
    }
    binding = (vitte_ir_global_binding_t *)vitte_arena_alloc_zeroed(lowering->ir->arena, sizeof(*binding), _Alignof(vitte_ir_global_binding_t));
    if (binding == NULL) {
        vitte_error_copy(&lowering->last_error, vitte_arena_last_error(lowering->ir->arena));
        vitte_error_copy(&lowering->ir->last_error, vitte_arena_last_error(lowering->ir->arena));
        return false;
    }
    binding->name = name;
    binding->global = global;
    binding->next = lowering->globals;
    lowering->globals = binding;
    return true;
}

static vitte_ir_global_t *vitte_ir_lookup_global(const vitte_ir_lowering_t *lowering, const char *name) {
    const vitte_ir_global_binding_t *binding;

    if (lowering == NULL || name == NULL) {
        return NULL;
    }
    for (binding = lowering->globals; binding != NULL; binding = binding->next) {
        if (binding->name != NULL && strcmp(binding->name, name) == 0) {
            return binding->global;
        }
    }
    return NULL;
}

vitte_status_t vitte_ir_init(vitte_ir_t *ir, vitte_arena_t *arena) {
    if (ir == NULL || !vitte_arena_is_initialized(arena)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(ir, 0, sizeof(*ir));
    ir->initialized = true;
    ir->arena = arena;
    ir->next_value_id = 1u;
    ir->next_block_id = 1u;
    ir->next_function_id = 1u;
    vitte_error_init(&ir->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_ir_init_owned(vitte_ir_t *ir, const vitte_arena_config_t *config) {
    vitte_status_t status;

    if (ir == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(ir, 0, sizeof(*ir));
    status = vitte_arena_init(&ir->owned_arena, config);
    if (status != VITTE_STATUS_OK) {
        vitte_error_init(&ir->last_error);
        vitte_ir_set_error(ir, status, "VITTE_IR_E_ARENA", "failed to initialize IR arena", NULL);
        return status;
    }
    ir->initialized = true;
    ir->owns_arena = true;
    ir->arena = &ir->owned_arena;
    ir->next_value_id = 1u;
    ir->next_block_id = 1u;
    ir->next_function_id = 1u;
    vitte_error_init(&ir->last_error);
    return VITTE_STATUS_OK;
}

void vitte_ir_destroy(vitte_ir_t *ir) {
    if (ir == NULL) {
        return;
    }
    if (ir->owns_arena) {
        vitte_arena_destroy(&ir->owned_arena);
    }
    memset(ir, 0, sizeof(*ir));
}

bool vitte_ir_is_initialized(const vitte_ir_t *ir) {
    return ir != NULL && ir->initialized && vitte_arena_is_initialized(ir->arena);
}

const vitte_error_t *vitte_ir_last_error(const vitte_ir_t *ir) {
    return ir != NULL ? &ir->last_error : vitte_error_last();
}

void vitte_ir_clear_error(vitte_ir_t *ir) {
    if (ir != NULL) {
        vitte_error_reset(&ir->last_error);
    }
}

bool vitte_ir_type_kind_is_valid(vitte_ir_type_kind_t kind) {
    return kind >= VITTE_IR_TYPE_ERROR && kind < VITTE_IR_TYPE_COUNT;
}

bool vitte_ir_value_kind_is_valid(vitte_ir_value_kind_t kind) {
    return kind >= VITTE_IR_VALUE_ERROR && kind < VITTE_IR_VALUE_COUNT;
}

bool vitte_ir_opcode_is_valid(vitte_ir_opcode_t opcode) {
    return opcode >= VITTE_IR_OP_ERROR && opcode < VITTE_IR_OP_COUNT;
}

vitte_ir_type_t *vitte_ir_make_type(vitte_ir_t *ir, vitte_ir_type_kind_t kind) {
    vitte_ir_type_t *type;

    if (!vitte_ir_is_initialized(ir) || !vitte_ir_type_kind_is_valid(kind)) {
        vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_TYPE", "invalid IR type request", NULL);
        return NULL;
    }
    type = (vitte_ir_type_t *)vitte_arena_alloc_zeroed(ir->arena, sizeof(*type), _Alignof(vitte_ir_type_t));
    if (type == NULL) {
        vitte_error_copy(&ir->last_error, vitte_arena_last_error(ir->arena));
        return NULL;
    }
    type->kind = kind;
    return type;
}

vitte_ir_type_t *vitte_ir_type_from_hir(vitte_ir_t *ir, const vitte_hir_node_t *hir_type) {
    const char *name;

    if (hir_type == NULL) {
        return vitte_ir_make_type(ir, VITTE_IR_TYPE_I32);
    }
    if (hir_type->kind != VITTE_HIR_TYPE_NAME) {
        vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_TYPE", "HIR type node expected", vitte_hir_kind_name(hir_type->kind));
        return NULL;
    }
    name = hir_type->as.type_name.name;
    if (name == NULL || strcmp(name, "int") == 0 || strcmp(name, "i32") == 0) {
        return vitte_ir_make_type(ir, VITTE_IR_TYPE_I32);
    }
    if (strcmp(name, "i64") == 0) {
        return vitte_ir_make_type(ir, VITTE_IR_TYPE_I64);
    }
    if (strcmp(name, "usize") == 0 || strcmp(name, "u64") == 0 ||
        strcmp(name, "u32") == 0 || strcmp(name, "u8") == 0) {
        return vitte_ir_make_type(ir, VITTE_IR_TYPE_USIZE);
    }
    if (strcmp(name, "bool") == 0) {
        return vitte_ir_make_type(ir, VITTE_IR_TYPE_BOOL);
    }
    if (strcmp(name, "string") == 0 || strcmp(name, "str") == 0) {
        return vitte_ir_make_type(ir, VITTE_IR_TYPE_STRING_PTR);
    }
    if (strcmp(name, "void") == 0) {
        return vitte_ir_make_type(ir, VITTE_IR_TYPE_VOID);
    }
    return vitte_ir_make_type(ir, VITTE_IR_TYPE_UNKNOWN);
}

const char *vitte_ir_type_name(const vitte_ir_type_t *type) {
    if (type == NULL) {
        return "<null>";
    }
    switch (type->kind) {
        case VITTE_IR_TYPE_ERROR:
            return "error";
        case VITTE_IR_TYPE_VOID:
            return "void";
        case VITTE_IR_TYPE_BOOL:
            return "bool";
        case VITTE_IR_TYPE_I32:
            return "i32";
        case VITTE_IR_TYPE_I64:
            return "i64";
        case VITTE_IR_TYPE_USIZE:
            return "usize";
        case VITTE_IR_TYPE_STRING_PTR:
            return "string*";
        case VITTE_IR_TYPE_UNKNOWN:
            return "unknown";
        case VITTE_IR_TYPE_COUNT:
        default:
            return "invalid";
    }
}

bool vitte_ir_type_equals(const vitte_ir_type_t *left, const vitte_ir_type_t *right) {
    return left != NULL && right != NULL && left->kind == right->kind;
}

const char *vitte_ir_opcode_name(vitte_ir_opcode_t opcode) {
    switch (opcode) {
        case VITTE_IR_OP_ERROR:
            return "error";
        case VITTE_IR_OP_CONST_INT:
            return "const_int";
        case VITTE_IR_OP_CONST_STRING:
            return "const_string";
        case VITTE_IR_OP_LOCAL:
            return "local";
        case VITTE_IR_OP_STORE:
            return "store";
        case VITTE_IR_OP_LOAD:
            return "load";
        case VITTE_IR_OP_BINARY:
            return "binary";
        case VITTE_IR_OP_CALL:
            return "call";
        case VITTE_IR_OP_RETURN:
            return "return";
        case VITTE_IR_OP_BRANCH:
            return "branch";
        case VITTE_IR_OP_COND_BRANCH:
            return "cond_branch";
        case VITTE_IR_OP_UNREACHABLE:
            return "unreachable";
        case VITTE_IR_OP_COUNT:
        default:
            return "invalid";
    }
}

const char *vitte_ir_value_label(const vitte_ir_value_t *value) {
    if (value == NULL) {
        return "<null>";
    }
    if (value->name != NULL) {
        return value->name;
    }
    switch (value->kind) {
        case VITTE_IR_VALUE_CONST_INT:
            return "const_int";
        case VITTE_IR_VALUE_CONST_STRING:
            return "const_string";
        case VITTE_IR_VALUE_INSTRUCTION:
            return "tmp";
        case VITTE_IR_VALUE_LOCAL:
            return "local";
        case VITTE_IR_VALUE_FUNCTION_REF:
            return "function";
        case VITTE_IR_VALUE_ERROR:
        default:
            return "value";
    }
}

static bool vitte_ir_opcode_is_terminator(vitte_ir_opcode_t opcode) {
    return opcode == VITTE_IR_OP_RETURN || opcode == VITTE_IR_OP_BRANCH ||
        opcode == VITTE_IR_OP_COND_BRANCH || opcode == VITTE_IR_OP_UNREACHABLE;
}

void vitte_ir_builder_init(vitte_ir_builder_t *builder, vitte_ir_t *ir) {
    if (builder == NULL) {
        return;
    }
    memset(builder, 0, sizeof(*builder));
    builder->ir = ir;
}

vitte_ir_module_t *vitte_ir_make_module(vitte_ir_builder_t *builder, const char *name) {
    vitte_ir_module_t *module;

    if (builder == NULL || !vitte_ir_is_initialized(builder->ir)) {
        return NULL;
    }
    module = (vitte_ir_module_t *)vitte_arena_alloc_zeroed(builder->ir->arena, sizeof(*module), _Alignof(vitte_ir_module_t));
    if (module == NULL) {
        vitte_error_copy(&builder->ir->last_error, vitte_arena_last_error(builder->ir->arena));
        return NULL;
    }
    module->name = name != NULL ? name : "<module>";
    builder->ir->module = module;
    return module;
}

vitte_ir_global_t *vitte_ir_make_global(vitte_ir_builder_t *builder, const char *name, vitte_ir_type_t *type, const vitte_hir_node_t *source) {
    vitte_ir_global_t *global;

    if (builder == NULL || !vitte_ir_is_initialized(builder->ir) || name == NULL || type == NULL) {
        if (builder != NULL) {
            vitte_ir_set_error(builder->ir, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_GLOBAL", "invalid IR global request", NULL);
        }
        return NULL;
    }
    global = (vitte_ir_global_t *)vitte_arena_alloc_zeroed(builder->ir->arena, sizeof(*global), _Alignof(vitte_ir_global_t));
    if (global == NULL) {
        vitte_error_copy(&builder->ir->last_error, vitte_arena_last_error(builder->ir->arena));
        return NULL;
    }
    global->name = name;
    global->type = type;
    global->source = source;
    return global;
}

bool vitte_ir_module_add_global(vitte_ir_module_t *module, vitte_ir_global_t *global) {
    if (module == NULL || global == NULL) {
        return false;
    }
    global->next = NULL;
    if (module->last_global != NULL) {
        module->last_global->next = global;
    } else {
        module->first_global = global;
    }
    module->last_global = global;
    module->global_count++;
    return true;
}

vitte_ir_function_t *vitte_ir_make_function(vitte_ir_builder_t *builder, const char *name, vitte_ir_type_t *return_type, const vitte_hir_node_t *source) {
    vitte_ir_function_t *function;

    if (builder == NULL || !vitte_ir_is_initialized(builder->ir) || name == NULL || return_type == NULL) {
        if (builder != NULL) {
            vitte_ir_set_error(builder->ir, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_FUNCTION", "invalid IR function request", NULL);
        }
        return NULL;
    }
    if (builder->ir->next_function_id == 0u) {
        vitte_ir_set_error(builder->ir, VITTE_STATUS_ERROR_INTERNAL, "VITTE_IR_E_ID", "IR function id overflow", NULL);
        return NULL;
    }
    function = (vitte_ir_function_t *)vitte_arena_alloc_zeroed(builder->ir->arena, sizeof(*function), _Alignof(vitte_ir_function_t));
    if (function == NULL) {
        vitte_error_copy(&builder->ir->last_error, vitte_arena_last_error(builder->ir->arena));
        return NULL;
    }
    function->id = builder->ir->next_function_id++;
    function->name = name;
    function->return_type = return_type;
    function->source = source;
    builder->ir->function_count++;
    return function;
}

bool vitte_ir_module_add_function(vitte_ir_module_t *module, vitte_ir_function_t *function) {
    if (module == NULL || function == NULL) {
        return false;
    }
    function->next = NULL;
    if (module->last_function != NULL) {
        module->last_function->next = function;
    } else {
        module->first_function = function;
    }
    module->last_function = function;
    module->function_count++;
    return true;
}

vitte_ir_block_t *vitte_ir_make_block(vitte_ir_builder_t *builder, const char *name, const vitte_hir_node_t *source) {
    vitte_ir_block_t *block;

    if (builder == NULL || !vitte_ir_is_initialized(builder->ir)) {
        return NULL;
    }
    if (builder->ir->next_block_id == 0u) {
        vitte_ir_set_error(builder->ir, VITTE_STATUS_ERROR_INTERNAL, "VITTE_IR_E_ID", "IR block id overflow", NULL);
        return NULL;
    }
    block = (vitte_ir_block_t *)vitte_arena_alloc_zeroed(builder->ir->arena, sizeof(*block), _Alignof(vitte_ir_block_t));
    if (block == NULL) {
        vitte_error_copy(&builder->ir->last_error, vitte_arena_last_error(builder->ir->arena));
        return NULL;
    }
    block->id = builder->ir->next_block_id++;
    block->name = name != NULL ? name : "block";
    block->source = source;
    builder->ir->block_count++;
    return block;
}

bool vitte_ir_function_add_block(vitte_ir_function_t *function, vitte_ir_block_t *block) {
    if (function == NULL || block == NULL) {
        return false;
    }
    block->next = NULL;
    if (function->last_block != NULL) {
        function->last_block->next = block;
    } else {
        function->first_block = block;
        function->entry = block;
    }
    function->last_block = block;
    function->block_count++;
    return true;
}

void vitte_ir_builder_position_at_end(vitte_ir_builder_t *builder, vitte_ir_function_t *function, vitte_ir_block_t *block) {
    if (builder == NULL) {
        return;
    }
    builder->function = function;
    builder->block = block;
}

vitte_ir_value_t *vitte_ir_make_value(vitte_ir_t *ir, vitte_ir_value_kind_t kind, vitte_ir_type_t *type, const char *name) {
    vitte_ir_value_t *value;

    if (!vitte_ir_is_initialized(ir) || !vitte_ir_value_kind_is_valid(kind) || type == NULL) {
        vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_VALUE", "invalid IR value request", NULL);
        return NULL;
    }
    if (ir->next_value_id == 0u) {
        vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INTERNAL, "VITTE_IR_E_ID", "IR value id overflow", NULL);
        return NULL;
    }
    value = (vitte_ir_value_t *)vitte_arena_alloc_zeroed(ir->arena, sizeof(*value), _Alignof(vitte_ir_value_t));
    if (value == NULL) {
        vitte_error_copy(&ir->last_error, vitte_arena_last_error(ir->arena));
        return NULL;
    }
    value->id = ir->next_value_id++;
    value->kind = kind;
    value->type = type;
    value->name = name;
    ir->value_count++;
    return value;
}

static bool vitte_ir_append_instruction(vitte_ir_builder_t *builder, vitte_ir_instruction_t *instruction) {
    if (builder == NULL || builder->block == NULL || instruction == NULL) {
        return false;
    }
    instruction->next = NULL;
    if (builder->block->last != NULL) {
        builder->block->last->next = instruction;
    } else {
        builder->block->first = instruction;
    }
    builder->block->last = instruction;
    builder->block->instruction_count++;
    builder->ir->instruction_count++;
    if (vitte_ir_opcode_is_terminator(instruction->opcode)) {
        builder->block->terminated = true;
    }
    return true;
}

vitte_ir_instruction_t *vitte_ir_emit_instruction(vitte_ir_builder_t *builder, vitte_ir_opcode_t opcode, vitte_ir_type_t *type, const vitte_hir_node_t *source) {
    vitte_ir_instruction_t *instruction;

    if (builder == NULL || !vitte_ir_is_initialized(builder->ir) || builder->block == NULL || !vitte_ir_opcode_is_valid(opcode)) {
        if (builder != NULL) {
            vitte_ir_set_error(builder->ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_EMIT", "invalid IR emit state", NULL);
        }
        return NULL;
    }
    if (builder->block->terminated) {
        vitte_ir_set_error(builder->ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_TERMINATOR", "cannot emit after block terminator", builder->block->name);
        return NULL;
    }
    instruction = (vitte_ir_instruction_t *)vitte_arena_alloc_zeroed(builder->ir->arena, sizeof(*instruction), _Alignof(vitte_ir_instruction_t));
    if (instruction == NULL) {
        vitte_error_copy(&builder->ir->last_error, vitte_arena_last_error(builder->ir->arena));
        return NULL;
    }
    instruction->opcode = opcode;
    instruction->type = type;
    instruction->source = source;
    if (!vitte_ir_append_instruction(builder, instruction)) {
        vitte_ir_set_error(builder->ir, VITTE_STATUS_ERROR_INTERNAL, "VITTE_IR_E_EMIT", "failed to append IR instruction", NULL);
        return NULL;
    }
    return instruction;
}

vitte_ir_value_t *vitte_ir_emit_const_int(vitte_ir_builder_t *builder, int64_t value, vitte_ir_type_t *type, const vitte_hir_node_t *source) {
    vitte_ir_instruction_t *instruction;
    vitte_ir_value_t *result;

    if (type == NULL && builder != NULL) {
        type = vitte_ir_make_type(builder->ir, VITTE_IR_TYPE_I32);
    }
    instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_CONST_INT, type, source);
    if (instruction == NULL) {
        return NULL;
    }
    result = vitte_ir_make_value(builder->ir, VITTE_IR_VALUE_CONST_INT, type, NULL);
    if (result == NULL) {
        return NULL;
    }
    result->as.int_value = value;
    result->definition = instruction;
    instruction->result = result;
    return result;
}

vitte_ir_value_t *vitte_ir_emit_const_string(vitte_ir_builder_t *builder, const char *value, const vitte_hir_node_t *source) {
    vitte_ir_type_t *type = builder != NULL ? vitte_ir_make_type(builder->ir, VITTE_IR_TYPE_STRING_PTR) : NULL;
    vitte_ir_instruction_t *instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_CONST_STRING, type, source);
    vitte_ir_value_t *result;

    if (instruction == NULL) {
        return NULL;
    }
    result = vitte_ir_make_value(builder->ir, VITTE_IR_VALUE_CONST_STRING, type, NULL);
    if (result == NULL) {
        return NULL;
    }
    result->as.string_value = value;
    result->definition = instruction;
    instruction->result = result;
    return result;
}

vitte_ir_value_t *vitte_ir_emit_local(vitte_ir_builder_t *builder, const char *name, vitte_ir_type_t *type, const vitte_hir_node_t *source) {
    vitte_ir_instruction_t *instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_LOCAL, type, source);
    vitte_ir_value_t *result;

    if (instruction == NULL || type == NULL) {
        return NULL;
    }
    result = vitte_ir_make_value(builder->ir, VITTE_IR_VALUE_LOCAL, type, name);
    if (result == NULL) {
        return NULL;
    }
    result->definition = instruction;
    instruction->result = result;
    return result;
}

vitte_ir_instruction_t *vitte_ir_emit_store(vitte_ir_builder_t *builder, vitte_ir_value_t *local, vitte_ir_value_t *value, const vitte_hir_node_t *source) {
    vitte_ir_instruction_t *instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_STORE, local != NULL ? local->type : NULL, source);
    if (instruction == NULL || local == NULL || value == NULL) {
        return NULL;
    }
    instruction->operands[0] = local;
    instruction->operands[1] = value;
    instruction->operand_count = 2u;
    return instruction;
}

vitte_ir_value_t *vitte_ir_emit_load(vitte_ir_builder_t *builder, vitte_ir_value_t *local, const vitte_hir_node_t *source) {
    vitte_ir_instruction_t *instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_LOAD, local != NULL ? local->type : NULL, source);
    vitte_ir_value_t *result;

    if (instruction == NULL || local == NULL) {
        return NULL;
    }
    result = vitte_ir_make_value(builder->ir, VITTE_IR_VALUE_INSTRUCTION, local->type, local->name);
    if (result == NULL) {
        return NULL;
    }
    result->definition = instruction;
    instruction->operands[0] = local;
    instruction->operand_count = 1u;
    instruction->result = result;
    return result;
}

vitte_ir_value_t *vitte_ir_emit_binary(vitte_ir_builder_t *builder, const char *operator_text, vitte_ir_value_t *left, vitte_ir_value_t *right, const vitte_hir_node_t *source) {
    vitte_ir_instruction_t *instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_BINARY, left != NULL ? left->type : NULL, source);
    vitte_ir_value_t *result;

    if (instruction == NULL || operator_text == NULL || left == NULL || right == NULL) {
        return NULL;
    }
    result = vitte_ir_make_value(builder->ir, VITTE_IR_VALUE_INSTRUCTION, left->type, NULL);
    if (result == NULL) {
        return NULL;
    }
    result->definition = instruction;
    instruction->operator_text = operator_text;
    instruction->operands[0] = left;
    instruction->operands[1] = right;
    instruction->operand_count = 2u;
    instruction->result = result;
    return result;
}

vitte_ir_value_t *vitte_ir_emit_call(vitte_ir_builder_t *builder, vitte_ir_value_t *callee, vitte_ir_value_t *const *arguments, size_t argument_count, vitte_ir_type_t *return_type, const vitte_hir_node_t *source) {
    vitte_ir_instruction_t *instruction;
    vitte_ir_value_t *result;
    size_t index;

    if (argument_count + 1u > VITTE_IR_MAX_OPERANDS || callee == NULL || return_type == NULL) {
        if (builder != NULL) {
            vitte_ir_set_error(builder->ir, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_CALL", "invalid IR call operands", NULL);
        }
        return NULL;
    }
    instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_CALL, return_type, source);
    if (instruction == NULL) {
        return NULL;
    }
    result = vitte_ir_make_value(builder->ir, VITTE_IR_VALUE_INSTRUCTION, return_type, NULL);
    if (result == NULL) {
        return NULL;
    }
    instruction->operands[0] = callee;
    for (index = 0u; index < argument_count; index++) {
        instruction->operands[index + 1u] = arguments[index];
    }
    instruction->operand_count = argument_count + 1u;
    instruction->result = result;
    result->definition = instruction;
    return result;
}

vitte_ir_instruction_t *vitte_ir_emit_return(vitte_ir_builder_t *builder, vitte_ir_value_t *value, const vitte_hir_node_t *source) {
    vitte_ir_type_t *void_type = NULL;
    vitte_ir_instruction_t *instruction;

    if (builder != NULL && value == NULL) {
        void_type = vitte_ir_make_type(builder->ir, VITTE_IR_TYPE_VOID);
    }
    instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_RETURN, value != NULL ? value->type : void_type, source);
    if (instruction == NULL) {
        return NULL;
    }
    if (value != NULL) {
        instruction->operands[0] = value;
        instruction->operand_count = 1u;
    }
    return instruction;
}

vitte_ir_instruction_t *vitte_ir_emit_branch(vitte_ir_builder_t *builder, vitte_ir_block_t *target, const vitte_hir_node_t *source) {
    vitte_ir_instruction_t *instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_BRANCH, NULL, source);
    if (instruction == NULL || target == NULL) {
        return NULL;
    }
    instruction->target = target;
    return instruction;
}

vitte_ir_instruction_t *vitte_ir_emit_cond_branch(vitte_ir_builder_t *builder, vitte_ir_value_t *condition, vitte_ir_block_t *then_target, vitte_ir_block_t *else_target, const vitte_hir_node_t *source) {
    vitte_ir_instruction_t *instruction = vitte_ir_emit_instruction(builder, VITTE_IR_OP_COND_BRANCH, NULL, source);
    if (instruction == NULL || condition == NULL || then_target == NULL || else_target == NULL) {
        return NULL;
    }
    instruction->operands[0] = condition;
    instruction->operand_count = 1u;
    instruction->target = then_target;
    instruction->else_target = else_target;
    return instruction;
}

vitte_ir_instruction_t *vitte_ir_emit_unreachable(vitte_ir_builder_t *builder, const vitte_hir_node_t *source) {
    return vitte_ir_emit_instruction(builder, VITTE_IR_OP_UNREACHABLE, NULL, source);
}

void vitte_ir_lowering_init(vitte_ir_lowering_t *lowering, vitte_ir_t *ir) {
    if (lowering == NULL) {
        return;
    }
    memset(lowering, 0, sizeof(*lowering));
    lowering->ir = ir;
    lowering->max_depth = VITTE_IR_DEFAULT_MAX_DEPTH;
    vitte_ir_builder_init(&lowering->builder, ir);
    vitte_error_init(&lowering->last_error);
}

const vitte_error_t *vitte_ir_lowering_last_error(const vitte_ir_lowering_t *lowering) {
    return lowering != NULL ? &lowering->last_error : vitte_error_last();
}

static vitte_ir_value_t *vitte_ir_lower_expr(vitte_ir_lowering_t *lowering, const vitte_hir_node_t *node, size_t depth);
static vitte_ir_value_t *vitte_ir_resolve_global_initializer(vitte_ir_lowering_t *lowering, vitte_ir_global_t *global);

static bool vitte_ir_depth_ok(vitte_ir_lowering_t *lowering, size_t depth) {
    if (lowering == NULL || depth > lowering->max_depth) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_DEPTH", "IR lowering exceeded maximum depth", NULL);
        return false;
    }
    return true;
}

static bool vitte_ir_operator_returns_bool(const char *operator_text) {
    return operator_text != NULL &&
        (strcmp(operator_text, "==") == 0 ||
        strcmp(operator_text, "!=") == 0 ||
        strcmp(operator_text, "<") == 0 ||
        strcmp(operator_text, "<=") == 0 ||
        strcmp(operator_text, ">") == 0 ||
        strcmp(operator_text, ">=") == 0 ||
        strcmp(operator_text, "&&") == 0 ||
        strcmp(operator_text, "||") == 0);
}

static vitte_ir_type_t *vitte_ir_binary_result_type(vitte_ir_t *ir, const char *operator_text, const vitte_ir_type_t *left, const vitte_ir_type_t *right) {
    if (vitte_ir_operator_returns_bool(operator_text)) {
        return vitte_ir_make_type(ir, VITTE_IR_TYPE_BOOL);
    }
    if (left != NULL && right != NULL && left->kind == right->kind) {
        return vitte_ir_make_type(ir, left->kind);
    }
    return vitte_ir_make_type(ir, VITTE_IR_TYPE_UNKNOWN);
}

static vitte_ir_value_t *vitte_ir_lower_builtin_constant(vitte_ir_lowering_t *lowering, const char *name) {
    vitte_builtin_registry_t registry;
    const vitte_builtin_constant_t *constant;
    vitte_ir_type_t *type;

    vitte_builtin_registry_init(&registry);
    constant = vitte_builtin_lookup_constant(&registry, name);
    if (constant == NULL) {
        return NULL;
    }
    type = vitte_ir_type_from_builtin(lowering->ir, constant->type);
    if (constant->type == VITTE_BUILTIN_TYPE_BOOL) {
        return vitte_ir_make_const_int_value(lowering->ir, constant->bool_value ? 1 : 0, type);
    }
    if (constant->text_value != NULL) {
        return vitte_ir_make_const_string_value(lowering->ir, constant->text_value);
    }
    return vitte_ir_make_const_int_value(lowering->ir, constant->int_value, type);
}

static vitte_ir_value_t *vitte_ir_lower_builtin_function(vitte_ir_lowering_t *lowering, const char *name) {
    vitte_builtin_registry_t registry;
    const vitte_builtin_function_t *function;

    vitte_builtin_registry_init(&registry);
    function = vitte_builtin_lookup_function(&registry, name);
    if (function == NULL) {
        return NULL;
    }
    return vitte_ir_make_function_ref_value(lowering->ir, name, NULL, vitte_ir_type_from_builtin(lowering->ir, function->return_type));
}

static vitte_ir_value_t *vitte_ir_lower_constant_expr(vitte_ir_lowering_t *lowering, const vitte_hir_node_t *node, size_t depth) {
    if (!vitte_ir_depth_ok(lowering, depth) || node == NULL) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_CONST", "missing constant expression", NULL);
        return NULL;
    }

    switch (node->kind) {
        case VITTE_HIR_INTEGER_LITERAL:
            return vitte_ir_make_const_int_value(lowering->ir, node->as.integer_literal.value, vitte_ir_make_type(lowering->ir, VITTE_IR_TYPE_I32));
        case VITTE_HIR_STRING_LITERAL:
            return vitte_ir_make_const_string_value(lowering->ir, node->as.string_literal.value);
        case VITTE_HIR_VARIABLE: {
            vitte_ir_global_t *global = vitte_ir_lookup_global(lowering, node->as.variable.name);
            vitte_ir_value_t *builtin_constant = vitte_ir_lower_builtin_constant(lowering, node->as.variable.name);
            if (global != NULL) {
                return vitte_ir_resolve_global_initializer(lowering, global);
            }
            if (builtin_constant != NULL) {
                return builtin_constant;
            }
            vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_IR_E_CONST", "constant expression references unsupported symbol", node->as.variable.name);
            return NULL;
        }
        case VITTE_HIR_BINARY_EXPR: {
            vitte_ir_value_t *left = vitte_ir_lower_constant_expr(lowering, node->as.binary_expr.left, depth + 1u);
            vitte_ir_value_t *right = vitte_ir_lower_constant_expr(lowering, node->as.binary_expr.right, depth + 1u);
            vitte_ir_type_t *type;

            if (left == NULL || right == NULL) {
                return NULL;
            }
            if (left->kind != VITTE_IR_VALUE_CONST_INT || right->kind != VITTE_IR_VALUE_CONST_INT) {
                vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_IR_E_CONST", "non-integer constant binary expression is not supported yet", node->as.binary_expr.operator_text);
                return NULL;
            }
            type = vitte_ir_binary_result_type(lowering->ir, node->as.binary_expr.operator_text, left->type, right->type);
            if (strcmp(node->as.binary_expr.operator_text, "+") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, left->as.int_value + right->as.int_value, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, "-") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, left->as.int_value - right->as.int_value, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, "*") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, left->as.int_value * right->as.int_value, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, "/") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, right->as.int_value != 0 ? left->as.int_value / right->as.int_value : 0, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, "%") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, right->as.int_value != 0 ? left->as.int_value % right->as.int_value : 0, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, "==") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, left->as.int_value == right->as.int_value ? 1 : 0, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, "!=") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, left->as.int_value != right->as.int_value ? 1 : 0, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, "<") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, left->as.int_value < right->as.int_value ? 1 : 0, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, "<=") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, left->as.int_value <= right->as.int_value ? 1 : 0, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, ">") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, left->as.int_value > right->as.int_value ? 1 : 0, type);
            }
            if (strcmp(node->as.binary_expr.operator_text, ">=") == 0) {
                return vitte_ir_make_const_int_value(lowering->ir, left->as.int_value >= right->as.int_value ? 1 : 0, type);
            }
            vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_IR_E_CONST", "unsupported constant binary operator", node->as.binary_expr.operator_text);
            return NULL;
        }
        default:
            vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_IR_E_CONST", "unsupported constant expression", vitte_hir_kind_name(node->kind));
            return NULL;
    }
}

static vitte_ir_value_t *vitte_ir_resolve_global_initializer(vitte_ir_lowering_t *lowering, vitte_ir_global_t *global) {
    if (lowering == NULL || global == NULL) {
        return NULL;
    }
    if (global->initialized) {
        return global->initializer;
    }
    if (global->resolving) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_CYCLE", "cycle detected while lowering global constant", global->name);
        return NULL;
    }
    if (global->source == NULL || global->source->kind != VITTE_HIR_CONST_DECL) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_GLOBAL", "global constant source is invalid", global->name);
        return NULL;
    }

    global->resolving = true;
    global->initializer = vitte_ir_lower_constant_expr(lowering, global->source->as.const_decl.value, 1u);
    if (global->initializer == NULL) {
        global->resolving = false;
        return NULL;
    }
    if (global->type == NULL || global->type->kind == VITTE_IR_TYPE_UNKNOWN) {
        global->type = global->initializer->type;
    }
    global->initialized = true;
    global->resolving = false;
    return global->initializer;
}

static vitte_ir_value_t *vitte_ir_lower_expr(vitte_ir_lowering_t *lowering, const vitte_hir_node_t *node, size_t depth) {
    if (!vitte_ir_depth_ok(lowering, depth) || node == NULL) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_EXPR", "missing HIR expression", NULL);
        return NULL;
    }
    switch (node->kind) {
        case VITTE_HIR_INTEGER_LITERAL:
            return vitte_ir_emit_const_int(&lowering->builder, node->as.integer_literal.value, vitte_ir_make_type(lowering->ir, VITTE_IR_TYPE_I32), node);
        case VITTE_HIR_STRING_LITERAL:
            return vitte_ir_emit_const_string(&lowering->builder, node->as.string_literal.value, node);
        case VITTE_HIR_VARIABLE: {
            vitte_ir_value_t *local = vitte_ir_lookup_local(lowering, node->as.variable.name);
            vitte_ir_global_t *global = vitte_ir_lookup_global(lowering, node->as.variable.name);
            vitte_ir_function_t *function = vitte_ir_lookup_function(lowering, node->as.variable.name);
            vitte_ir_value_t *builtin_constant;
            vitte_ir_value_t *builtin_function;

            if (local != NULL) {
                return vitte_ir_emit_load(&lowering->builder, local, node);
            }
            if (global != NULL) {
                return vitte_ir_resolve_global_initializer(lowering, global);
            }
            if (function != NULL) {
                return vitte_ir_make_function_ref_value(lowering->ir, node->as.variable.name, function, function->return_type);
            }
            builtin_constant = vitte_ir_lower_builtin_constant(lowering, node->as.variable.name);
            if (builtin_constant != NULL) {
                return builtin_constant;
            }
            builtin_function = vitte_ir_lower_builtin_function(lowering, node->as.variable.name);
            if (builtin_function != NULL) {
                return builtin_function;
            }
            vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_SYMBOL", "unresolved symbol in IR lowering", node->as.variable.name);
            return NULL;
        }
        case VITTE_HIR_BINARY_EXPR: {
            vitte_ir_value_t *left = vitte_ir_lower_expr(lowering, node->as.binary_expr.left, depth + 1u);
            vitte_ir_value_t *right = vitte_ir_lower_expr(lowering, node->as.binary_expr.right, depth + 1u);
            vitte_ir_value_t *result;
            if (left == NULL || right == NULL) {
                return NULL;
            }
            result = vitte_ir_emit_binary(&lowering->builder, node->as.binary_expr.operator_text, left, right, node);
            if (result != NULL) {
                result->type = vitte_ir_binary_result_type(lowering->ir, node->as.binary_expr.operator_text, left->type, right->type);
                if (result->definition != NULL) {
                    result->definition->type = result->type;
                }
            }
            return result;
        }
        case VITTE_HIR_CALL_EXPR: {
            vitte_ir_value_t *callee = vitte_ir_lower_expr(lowering, node->as.call_expr.callee, depth + 1u);
            vitte_ir_value_t *args[VITTE_IR_MAX_OPERANDS - 1u];
            const vitte_hir_node_t *arg;
            size_t count = 0u;
            vitte_ir_type_t *return_type;
            if (callee == NULL) {
                return NULL;
            }
            for (arg = node->as.call_expr.arguments.first; arg != NULL; arg = arg->next) {
                if (count >= VITTE_IR_MAX_OPERANDS - 1u) {
                    vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_IR_E_CALL", "too many call arguments for bootstrap IR", NULL);
                    return NULL;
                }
                args[count] = vitte_ir_lower_expr(lowering, arg, depth + 1u);
                if (args[count] == NULL) {
                    return NULL;
                }
                count++;
            }
            return_type = callee->kind == VITTE_IR_VALUE_FUNCTION_REF && callee->as.function != NULL ?
                callee->as.function->return_type :
                callee->type != NULL ? callee->type : vitte_ir_make_type(lowering->ir, VITTE_IR_TYPE_UNKNOWN);
            return vitte_ir_emit_call(&lowering->builder, callee, args, count, return_type, node);
        }
        case VITTE_HIR_ERROR:
            return vitte_ir_make_value(lowering->ir, VITTE_IR_VALUE_ERROR, vitte_ir_make_type(lowering->ir, VITTE_IR_TYPE_ERROR), "error");
        default:
            vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_IR_E_EXPR", "unsupported HIR expression for IR lowering", vitte_hir_kind_name(node->kind));
            return NULL;
    }
}

static vitte_status_t vitte_ir_lower_stmt(vitte_ir_lowering_t *lowering, const vitte_hir_node_t *node, size_t depth);

static vitte_status_t vitte_ir_lower_block(vitte_ir_lowering_t *lowering, const vitte_hir_node_t *block, size_t depth) {
    const vitte_hir_node_t *stmt;
    vitte_status_t status = VITTE_STATUS_OK;

    if (!vitte_ir_depth_ok(lowering, depth) || block == NULL || block->kind != VITTE_HIR_BLOCK) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_BLOCK", "expected HIR block", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_ir_scope_push(lowering)) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_IR_E_SCOPE", "failed to push IR scope", NULL);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }
    for (stmt = block->as.block.statements.first; stmt != NULL; stmt = stmt->next) {
        status = vitte_ir_lower_stmt(lowering, stmt, depth + 1u);
        if (status != VITTE_STATUS_OK) {
            break;
        }
        if (lowering->builder.block != NULL && lowering->builder.block->terminated) {
            break;
        }
    }
    if (status == VITTE_STATUS_OK && lowering->builder.block != NULL && !lowering->builder.block->terminated) {
        if (vitte_ir_emit_unreachable(&lowering->builder, block) == NULL) {
            status = VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }
    vitte_ir_scope_pop(lowering);
    return status;
}

static vitte_status_t vitte_ir_lower_stmt(vitte_ir_lowering_t *lowering, const vitte_hir_node_t *node, size_t depth) {
    if (!vitte_ir_depth_ok(lowering, depth) || node == NULL) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_STMT", "missing HIR statement", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    switch (node->kind) {
        case VITTE_HIR_RETURN_STMT: {
            vitte_ir_value_t *value = node->as.return_stmt.value != NULL ?
                vitte_ir_lower_expr(lowering, node->as.return_stmt.value, depth + 1u) : NULL;
            if (node->as.return_stmt.value != NULL && value == NULL) {
                return lowering->last_error.status;
            }
            return vitte_ir_emit_return(&lowering->builder, value, node) != NULL ? VITTE_STATUS_OK : VITTE_STATUS_ERROR_INVALID_STATE;
        }
        case VITTE_HIR_LET_STMT: {
            vitte_ir_value_t *local;
            vitte_ir_value_t *value = node->as.let_stmt.value != NULL ?
                vitte_ir_lower_expr(lowering, node->as.let_stmt.value, depth + 1u) : NULL;
            vitte_ir_type_t *type = node->as.let_stmt.declared_type != NULL ?
                vitte_ir_type_from_hir(lowering->ir, node->as.let_stmt.declared_type) :
                (value != NULL ? value->type : vitte_ir_make_type(lowering->ir, VITTE_IR_TYPE_UNKNOWN));
            if (type == NULL) {
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            local = vitte_ir_emit_local(&lowering->builder, node->as.let_stmt.name, type, node);
            if (local == NULL || !vitte_ir_bind_local(lowering, node->as.let_stmt.name, local)) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            if (node->as.let_stmt.value != NULL &&
                (value == NULL || vitte_ir_emit_store(&lowering->builder, local, value, node) == NULL)) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            return VITTE_STATUS_OK;
        }
        case VITTE_HIR_BLOCK:
            return vitte_ir_lower_block(lowering, node, depth + 1u);
        case VITTE_HIR_IF_STMT: {
            vitte_ir_function_t *function = lowering->builder.function;
            vitte_ir_block_t *then_block;
            vitte_ir_block_t *else_block;
            vitte_ir_block_t *merge_block;
            vitte_ir_value_t *condition = vitte_ir_lower_expr(lowering, node->as.if_stmt.condition, depth + 1u);
            if (function == NULL || lowering->builder.block == NULL || condition == NULL) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            then_block = vitte_ir_make_block(&lowering->builder, "if.then", node->as.if_stmt.then_branch);
            else_block = vitte_ir_make_block(&lowering->builder, "if.else", node->as.if_stmt.else_branch);
            merge_block = vitte_ir_make_block(&lowering->builder, "if.end", node);
            if (then_block == NULL || else_block == NULL || merge_block == NULL ||
                !vitte_ir_function_add_block(function, then_block) ||
                !vitte_ir_function_add_block(function, else_block) ||
                !vitte_ir_function_add_block(function, merge_block) ||
                vitte_ir_emit_cond_branch(&lowering->builder, condition, then_block, else_block, node) == NULL) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            vitte_ir_builder_position_at_end(&lowering->builder, function, then_block);
            if (vitte_ir_lower_stmt(lowering, node->as.if_stmt.then_branch, depth + 1u) != VITTE_STATUS_OK) {
                return lowering->last_error.status;
            }
            if (!then_block->terminated && vitte_ir_emit_branch(&lowering->builder, merge_block, node) == NULL) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            vitte_ir_builder_position_at_end(&lowering->builder, function, else_block);
            if (node->as.if_stmt.else_branch != NULL) {
                if (vitte_ir_lower_stmt(lowering, node->as.if_stmt.else_branch, depth + 1u) != VITTE_STATUS_OK) {
                    return lowering->last_error.status;
                }
            }
            if (!else_block->terminated && vitte_ir_emit_branch(&lowering->builder, merge_block, node) == NULL) {
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            vitte_ir_builder_position_at_end(&lowering->builder, function, merge_block);
            return VITTE_STATUS_OK;
        }
        case VITTE_HIR_ERROR:
            return vitte_ir_emit_unreachable(&lowering->builder, node) != NULL ? VITTE_STATUS_OK : VITTE_STATUS_ERROR_INVALID_STATE;
        default:
            vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_IR_E_STMT", "unsupported HIR statement for IR lowering", vitte_hir_kind_name(node->kind));
            return VITTE_STATUS_ERROR_UNSUPPORTED;
    }
}

static vitte_status_t vitte_ir_predeclare_function(vitte_ir_lowering_t *lowering, const vitte_hir_node_t *hir_function) {
    vitte_ir_type_t *return_type;
    vitte_ir_function_t *function;

    if (hir_function == NULL || hir_function->kind != VITTE_HIR_FUNCTION) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_FUNCTION", "expected HIR function", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    return_type = vitte_ir_type_from_hir(lowering->ir, hir_function->as.function.return_type);
    function = vitte_ir_make_function(&lowering->builder, hir_function->as.function.name, return_type, hir_function);
    if (return_type == NULL || function == NULL ||
        !vitte_ir_module_add_function(lowering->ir->module, function) ||
        !vitte_ir_bind_function(lowering, hir_function->as.function.name, function)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_ir_predeclare_global(vitte_ir_lowering_t *lowering, const vitte_hir_node_t *hir_decl) {
    vitte_ir_type_t *type;
    vitte_ir_global_t *global;

    if (hir_decl == NULL || hir_decl->kind != VITTE_HIR_CONST_DECL) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_GLOBAL", "expected HIR const declaration", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    type = hir_decl->as.const_decl.declared_type != NULL ?
        vitte_ir_type_from_hir(lowering->ir, hir_decl->as.const_decl.declared_type) :
        vitte_ir_make_type(lowering->ir, VITTE_IR_TYPE_UNKNOWN);
    global = vitte_ir_make_global(&lowering->builder, hir_decl->as.const_decl.name, type, hir_decl);
    if (type == NULL || global == NULL ||
        !vitte_ir_module_add_global(lowering->ir->module, global) ||
        !vitte_ir_bind_global(lowering, hir_decl->as.const_decl.name, global)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_ir_lower_function_body(vitte_ir_lowering_t *lowering, const vitte_hir_node_t *hir_function) {
    vitte_ir_function_t *function;
    vitte_ir_block_t *entry;

    if (hir_function == NULL || hir_function->kind != VITTE_HIR_FUNCTION) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_FUNCTION", "expected HIR function", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    function = vitte_ir_lookup_function(lowering, hir_function->as.function.name);
    entry = vitte_ir_make_block(&lowering->builder, "entry", hir_function->as.function.body);
    if (function == NULL || entry == NULL || !vitte_ir_function_add_block(function, entry)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    vitte_ir_builder_position_at_end(&lowering->builder, function, entry);
    return vitte_ir_lower_block(lowering, hir_function->as.function.body, 1u);
}

vitte_status_t vitte_ir_lower_hir_with_options(vitte_ir_lowering_t *lowering, const vitte_hir_t *hir) {
    const vitte_hir_node_t *decl;
    vitte_ir_module_t *module;

    if (lowering == NULL || !vitte_ir_is_initialized(lowering->ir) || hir == NULL ||
        !vitte_hir_is_initialized(hir) || hir->root == NULL) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IR_E_ARGUMENT", "missing HIR or IR for lowering", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    module = vitte_ir_make_module(&lowering->builder, hir->root->as.module.name);
    if (module == NULL) {
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_IR_E_ALLOC", "failed to allocate IR module", NULL);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }
    for (decl = hir->root->as.module.declarations.first; decl != NULL; decl = decl->next) {
        vitte_status_t status;

        if (decl->kind == VITTE_HIR_FUNCTION) {
            status = vitte_ir_predeclare_function(lowering, decl);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            continue;
        }
        if (decl->kind == VITTE_HIR_CONST_DECL) {
            status = vitte_ir_predeclare_global(lowering, decl);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            continue;
        }
        vitte_ir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_IR_E_DECL", "unsupported HIR declaration for IR lowering", vitte_hir_kind_name(decl->kind));
        return VITTE_STATUS_ERROR_UNSUPPORTED;
    }
    for (decl = hir->root->as.module.declarations.first; decl != NULL; decl = decl->next) {
        if (decl->kind == VITTE_HIR_CONST_DECL) {
            vitte_ir_global_t *global = vitte_ir_lookup_global(lowering, decl->as.const_decl.name);
            if (global == NULL || vitte_ir_resolve_global_initializer(lowering, global) == NULL) {
                return lowering->last_error.status;
            }
        }
    }
    for (decl = hir->root->as.module.declarations.first; decl != NULL; decl = decl->next) {
        if (decl->kind == VITTE_HIR_FUNCTION) {
            vitte_status_t status = vitte_ir_lower_function_body(lowering, decl);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
        }
    }
    vitte_error_reset(&lowering->last_error);
    vitte_ir_clear_error(lowering->ir);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_ir_lower_hir(vitte_ir_t *ir, const vitte_hir_t *hir) {
    vitte_ir_lowering_t lowering;
    vitte_ir_lowering_init(&lowering, ir);
    return vitte_ir_lower_hir_with_options(&lowering, hir);
}

static size_t vitte_ir_count_instructions(const vitte_ir_block_t *block) {
    const vitte_ir_instruction_t *instruction;
    size_t count = 0u;
    for (instruction = block != NULL ? block->first : NULL; instruction != NULL; instruction = instruction->next) {
        count++;
    }
    return count;
}

static vitte_status_t vitte_ir_validate_instruction(vitte_ir_t *ir, const vitte_ir_instruction_t *instruction) {
    size_t expected_min = 0u;

    if (instruction == NULL || !vitte_ir_opcode_is_valid(instruction->opcode) || instruction->operand_count > VITTE_IR_MAX_OPERANDS) {
        vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_INSTRUCTION", "invalid IR instruction", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    switch (instruction->opcode) {
        case VITTE_IR_OP_CONST_INT:
        case VITTE_IR_OP_CONST_STRING:
        case VITTE_IR_OP_LOCAL:
            if (instruction->result == NULL || instruction->type == NULL) {
                vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_RESULT", "IR instruction requires result and type", vitte_ir_opcode_name(instruction->opcode));
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            break;
        case VITTE_IR_OP_STORE:
        case VITTE_IR_OP_BINARY:
            expected_min = 2u;
            break;
        case VITTE_IR_OP_LOAD:
        case VITTE_IR_OP_RETURN:
            expected_min = instruction->opcode == VITTE_IR_OP_RETURN ? 0u : 1u;
            break;
        case VITTE_IR_OP_CALL:
            expected_min = 1u;
            break;
        case VITTE_IR_OP_BRANCH:
            if (instruction->target == NULL) {
                vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_BRANCH", "IR branch requires target", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            break;
        case VITTE_IR_OP_COND_BRANCH:
            if (instruction->operand_count != 1u || instruction->target == NULL || instruction->else_target == NULL) {
                vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_BRANCH", "IR conditional branch is incomplete", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            break;
        default:
            break;
    }
    if (instruction->operand_count < expected_min) {
        vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_OPERAND", "IR instruction has too few operands", vitte_ir_opcode_name(instruction->opcode));
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_ir_validate(vitte_ir_t *ir) {
    const vitte_ir_global_t *global;
    const vitte_ir_function_t *function;
    size_t globals = 0u;
    size_t functions = 0u;
    size_t blocks = 0u;
    size_t instructions = 0u;

    if (!vitte_ir_is_initialized(ir) || ir->module == NULL) {
        vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_STATE", "IR module is missing", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    for (global = ir->module->first_global; global != NULL; global = global->next) {
        globals++;
        if (global->name == NULL || global->type == NULL || global->initializer == NULL || !global->initialized) {
            vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_GLOBAL", "invalid IR global", global != NULL ? global->name : NULL);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }
    for (function = ir->module->first_function; function != NULL; function = function->next) {
        const vitte_ir_block_t *block;
        functions++;
        if (function->id == 0u || function->name == NULL || function->return_type == NULL || function->entry == NULL || function->block_count == 0u) {
            vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_FUNCTION", "invalid IR function", NULL);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
        for (block = function->first_block; block != NULL; block = block->next) {
            const vitte_ir_instruction_t *instruction;
            blocks++;
            if (block->id == 0u || block->name == NULL || !block->terminated || vitte_ir_count_instructions(block) != block->instruction_count) {
                vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_BLOCK", "invalid or unterminated IR block", block != NULL ? block->name : NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            for (instruction = block->first; instruction != NULL; instruction = instruction->next) {
                vitte_status_t status = vitte_ir_validate_instruction(ir, instruction);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
                instructions++;
            }
        }
    }
    if (globals != ir->module->global_count ||
        functions != ir->module->function_count ||
        functions != ir->function_count ||
        blocks != ir->block_count ||
        instructions != ir->instruction_count) {
        vitte_ir_set_error(ir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IR_E_COUNT", "IR counts are inconsistent", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    vitte_ir_clear_error(ir);
    return VITTE_STATUS_OK;
}

void vitte_ir_dump(const vitte_ir_t *ir, FILE *stream) {
    const vitte_ir_global_t *global;
    const vitte_ir_function_t *function;

    if (ir == NULL || stream == NULL || ir->module == NULL) {
        return;
    }
    (void)fprintf(stream, "module %s\n", ir->module->name != NULL ? ir->module->name : "<module>");
    for (global = ir->module->first_global; global != NULL; global = global->next) {
        (void)fprintf(stream, "  global %s : %s", global->name != NULL ? global->name : "<global>", vitte_ir_type_name(global->type));
        if (global->initializer != NULL) {
            if (global->initializer->kind == VITTE_IR_VALUE_CONST_INT) {
                (void)fprintf(stream, " = %" PRId64, global->initializer->as.int_value);
            } else if (global->initializer->kind == VITTE_IR_VALUE_CONST_STRING) {
                (void)fprintf(stream, " = \"%s\"", global->initializer->as.string_value != NULL ? global->initializer->as.string_value : "");
            }
        }
        (void)fputc('\n', stream);
    }
    for (function = ir->module->first_function; function != NULL; function = function->next) {
        const vitte_ir_block_t *block;
        (void)fprintf(stream, "  fn #%" PRIu32 " %s -> %s\n", function->id, function->name, vitte_ir_type_name(function->return_type));
        for (block = function->first_block; block != NULL; block = block->next) {
            const vitte_ir_instruction_t *instruction;
            (void)fprintf(stream, "    block #%" PRIu32 " %s%s\n", block->id, block->name, block->terminated ? "" : " unterminated");
            for (instruction = block->first; instruction != NULL; instruction = instruction->next) {
                (void)fprintf(stream, "      %s", vitte_ir_opcode_name(instruction->opcode));
                if (instruction->result != NULL) {
                    (void)fprintf(stream, " %%%" PRIu32 ":%s", instruction->result->id, vitte_ir_type_name(instruction->result->type));
                }
                if (instruction->operator_text != NULL) {
                    (void)fprintf(stream, " %s", instruction->operator_text);
                }
                if (instruction->operand_count > 0u) {
                    size_t index;
                    (void)fputs(" [", stream);
                    for (index = 0u; index < instruction->operand_count; index++) {
                        if (index > 0u) {
                            (void)fputs(", ", stream);
                        }
                        (void)fprintf(stream, "%%%" PRIu32, instruction->operands[index] != NULL ? instruction->operands[index]->id : 0u);
                    }
                    (void)fputs("]", stream);
                }
                if (instruction->target != NULL) {
                    (void)fprintf(stream, " -> block#%" PRIu32, instruction->target->id);
                }
                if (instruction->else_target != NULL) {
                    (void)fprintf(stream, " else block#%" PRIu32, instruction->else_target->id);
                }
                (void)fputc('\n', stream);
            }
        }
    }
}
