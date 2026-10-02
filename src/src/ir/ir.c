/*
 * Vitte Compiler
 * src/ir/ir.c
 *
 * Core Intermediate Representation implementation.
 *
 * The public IR contract lives in ir.h.
 *
 * Responsibilities:
 *   - context lifecycle;
 *   - string interning;
 *   - structural type interning;
 *   - constants and SSA values;
 *   - functions and parameters;
 *   - basic blocks;
 *   - typed instructions;
 *   - operands;
 *   - phi incoming edges;
 *   - CFG edges;
 *   - predecessor/successor tracking;
 *   - use accounting;
 *   - terminator validation;
 *   - structural verification;
 *   - deterministic fingerprints;
 *   - statistics;
 *   - resource limits.
 *
 * ISO C17.
 */

#include "ir.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(VITTE_IR_INVALID_ID == UINT64_C(0),
               "IR zero ID must remain invalid");

_Static_assert(VITTE_IR_DEFAULT_INITIAL_CAPACITY > 0u,
               "IR initial capacity must be non-zero");

_Static_assert(VITTE_IR_DEFAULT_MAX_TYPES > 0u,
               "IR type limit must be non-zero");

_Static_assert(VITTE_IR_DEFAULT_MAX_VALUES > 0u,
               "IR value limit must be non-zero");

_Static_assert(VITTE_IR_DEFAULT_MAX_INSTRUCTIONS > 0u,
               "IR instruction limit must be non-zero");

_Static_assert(VITTE_IR_DEFAULT_MAX_BLOCKS > 0u,
               "IR block limit must be non-zero");

_Static_assert(VITTE_IR_DEFAULT_MAX_FUNCTIONS > 0u,
               "IR function limit must be non-zero");

/* ========================================================================= */
/* Private declarations                                                      */
/* ========================================================================= */

static bool
vitte_ir_size_add(size_t left, size_t right, size_t *result);

static bool
vitte_ir_size_mul(size_t left, size_t right, size_t *result);

static uint64_t
vitte_ir_u64_add_sat(uint64_t left, uint64_t right);

static uint64_t
vitte_ir_hash_bytes(uint64_t hash, const void *data, size_t length);

static uint64_t
vitte_ir_hash_u64(uint64_t hash, uint64_t value);

static uint64_t
vitte_ir_hash_size(uint64_t hash, size_t value);

static uint64_t
vitte_ir_hash_bool(uint64_t hash, bool value);

static bool
vitte_ir_context_envelope_valid(const vitte_ir_context_t *context);

static bool
vitte_ir_context_mutable(const vitte_ir_context_t *context);

static bool
vitte_ir_fail(vitte_ir_context_t *context, vitte_ir_error_t error);

static void *
vitte_ir_realloc(vitte_ir_context_t *context, void *pointer, size_t bytes);

static bool
vitte_ir_reserve_raw(
    vitte_ir_context_t *context,
    void **storage,
    size_t *capacity,
    size_t required,
    size_t element_size);

static bool
vitte_ir_reserve_types(vitte_ir_context_t *context, size_t required);

static bool
vitte_ir_reserve_values(vitte_ir_context_t *context, size_t required);

static bool
vitte_ir_reserve_instructions(vitte_ir_context_t *context, size_t required);

static bool
vitte_ir_reserve_blocks(vitte_ir_context_t *context, size_t required);

static bool
vitte_ir_reserve_functions(vitte_ir_context_t *context, size_t required);

static bool
vitte_ir_reserve_strings(vitte_ir_context_t *context, size_t required);

static bool
vitte_ir_reserve_value_ids(
    vitte_ir_context_t *context,
    vitte_ir_value_id_t **items,
    size_t *capacity,
    size_t required);

static bool
vitte_ir_reserve_block_ids(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t **items,
    size_t *capacity,
    size_t required);

static bool
vitte_ir_reserve_instruction_ids(
    vitte_ir_context_t *context,
    vitte_ir_instruction_id_t **items,
    size_t *capacity,
    size_t required);

static bool
vitte_ir_reserve_phi_incomings(
    vitte_ir_context_t *context,
    vitte_ir_phi_incoming_t **items,
    size_t *capacity,
    size_t required);

static bool
vitte_ir_string_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_string_id_t id);

static bool
vitte_ir_type_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_type_id_t id);

static bool
vitte_ir_value_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_value_id_t id);

static bool
vitte_ir_instruction_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_instruction_id_t id);

static bool
vitte_ir_block_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id);

static bool
vitte_ir_function_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_function_id_t id);

static vitte_ir_string_id_t
vitte_ir_find_string(
    const vitte_ir_context_t *context,
    const char *data,
    size_t length,
    uint64_t hash);

static bool
vitte_ir_type_equal(
    const vitte_ir_type_t *left,
    const vitte_ir_type_desc_t *right);

static vitte_ir_type_id_t
vitte_ir_find_type(
    const vitte_ir_context_t *context,
    const vitte_ir_type_desc_t *description);

static bool
vitte_ir_type_kind_valid(vitte_ir_type_kind_t kind);

static bool
vitte_ir_value_kind_valid(vitte_ir_value_kind_t kind);

static bool
vitte_ir_opcode_valid(vitte_ir_opcode_t opcode);

static bool
vitte_ir_linkage_valid(vitte_ir_linkage_t linkage);

static bool
vitte_ir_calling_convention_valid(vitte_ir_calling_convention_t convention);

static bool
vitte_ir_block_append_instruction(
    vitte_ir_context_t *context,
    vitte_ir_block_t *block,
    vitte_ir_instruction_id_t instruction);

static bool
vitte_ir_function_append_block(
    vitte_ir_context_t *context,
    vitte_ir_function_t *function,
    vitte_ir_block_id_t block);

static bool
vitte_ir_function_append_parameter(
    vitte_ir_context_t *context,
    vitte_ir_function_t *function,
    vitte_ir_value_id_t value);

static bool
vitte_ir_block_append_successor(
    vitte_ir_context_t *context,
    vitte_ir_block_t *block,
    vitte_ir_block_id_t successor);

static bool
vitte_ir_block_append_predecessor(
    vitte_ir_context_t *context,
    vitte_ir_block_t *block,
    vitte_ir_block_id_t predecessor);

static bool
vitte_ir_instruction_append_operand(
    vitte_ir_context_t *context,
    vitte_ir_instruction_t *instruction,
    vitte_ir_value_id_t value);

static bool
vitte_ir_value_add_use(
    vitte_ir_context_t *context,
    vitte_ir_value_id_t value);

static bool
vitte_ir_opcode_is_terminator_internal(vitte_ir_opcode_t opcode);

static bool
vitte_ir_opcode_produces_value_internal(vitte_ir_opcode_t opcode);

static bool
vitte_ir_validate_type(
    vitte_ir_context_t *context,
    const vitte_ir_type_t *type);

static bool
vitte_ir_validate_value(
    vitte_ir_context_t *context,
    const vitte_ir_value_t *value);

static bool
vitte_ir_validate_instruction(
    vitte_ir_context_t *context,
    const vitte_ir_instruction_t *instruction);

static bool
vitte_ir_validate_block(
    vitte_ir_context_t *context,
    const vitte_ir_block_t *block);

static bool
vitte_ir_validate_function(
    vitte_ir_context_t *context,
    const vitte_ir_function_t *function);

static void
vitte_ir_release_instruction(vitte_ir_instruction_t *instruction);

static void
vitte_ir_release_block(vitte_ir_block_t *block);

static void
vitte_ir_release_function(vitte_ir_function_t *function);

static void
vitte_ir_release_type(vitte_ir_type_t *type);

static void
vitte_ir_release_string(vitte_ir_string_t *string);

static void
vitte_ir_release_storage(vitte_ir_context_t *context);

/* ========================================================================= */
/* Checked arithmetic                                                        */
/* ========================================================================= */

static bool
vitte_ir_size_add(size_t left, size_t right, size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left > SIZE_MAX - right) {
        return false;
    }

    *result = left + right;
    return true;
}

static bool
vitte_ir_size_mul(size_t left, size_t right, size_t *result)
{
    if (result == NULL) {
        return false;
    }

    if (left != 0u && right > SIZE_MAX / left) {
        return false;
    }

    *result = left * right;
    return true;
}

static uint64_t
vitte_ir_u64_add_sat(uint64_t left, uint64_t right)
{
    if (UINT64_MAX - left < right) {
        return UINT64_MAX;
    }

    return left + right;
}

/* ========================================================================= */
/* Hashing                                                                   */
/* ========================================================================= */

static uint64_t
vitte_ir_hash_bytes(uint64_t hash, const void *data, size_t length)
{
    const unsigned char *bytes;
    size_t index;

    if (data == NULL && length != 0u) {
        return hash;
    }

    bytes = (const unsigned char *)data;

    for (index = 0u; index < length; ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_IR_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_ir_hash_u64(uint64_t hash, uint64_t value)
{
    unsigned int shift;

    for (shift = 0u; shift < 64u; shift += 8u) {
        hash ^= (value >> shift) & UINT64_C(0xff);
        hash *= VITTE_IR_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_ir_hash_size(uint64_t hash, size_t value)
{
    const unsigned char *bytes;
    size_t index;

    bytes = (const unsigned char *)&value;

    for (index = 0u; index < sizeof(value); ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_IR_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_ir_hash_bool(uint64_t hash, bool value)
{
    hash ^= value ? UINT64_C(1) : UINT64_C(0);
    hash *= VITTE_IR_FNV_PRIME;

    return hash;
}

/* ========================================================================= */
/* Enum names                                                                */
/* ========================================================================= */

const char *
vitte_ir_error_name(vitte_ir_error_t error)
{
    switch (error) {
        case VITTE_IR_ERROR_NONE: return "none";
        case VITTE_IR_ERROR_INVALID_ARGUMENT: return "invalid_argument";
        case VITTE_IR_ERROR_INVALID_CONTEXT: return "invalid_context";
        case VITTE_IR_ERROR_INVALID_STATE: return "invalid_state";
        case VITTE_IR_ERROR_INVALID_STRING: return "invalid_string";
        case VITTE_IR_ERROR_INVALID_TYPE: return "invalid_type";
        case VITTE_IR_ERROR_INVALID_VALUE: return "invalid_value";
        case VITTE_IR_ERROR_INVALID_INSTRUCTION: return "invalid_instruction";
        case VITTE_IR_ERROR_INVALID_BLOCK: return "invalid_block";
        case VITTE_IR_ERROR_INVALID_FUNCTION: return "invalid_function";
        case VITTE_IR_ERROR_TYPE_MISMATCH: return "type_mismatch";
        case VITTE_IR_ERROR_DUPLICATE: return "duplicate";
        case VITTE_IR_ERROR_TERMINATED_BLOCK: return "terminated_block";
        case VITTE_IR_ERROR_MISSING_TERMINATOR: return "missing_terminator";
        case VITTE_IR_ERROR_INVALID_CFG: return "invalid_cfg";
        case VITTE_IR_ERROR_INVALID_PHI: return "invalid_phi";
        case VITTE_IR_ERROR_LIMIT: return "limit";
        case VITTE_IR_ERROR_OVERFLOW: return "overflow";
        case VITTE_IR_ERROR_OUT_OF_MEMORY: return "out_of_memory";
        case VITTE_IR_ERROR_VALIDATION: return "validation";
        case VITTE_IR_ERROR_CORRUPTION: return "corruption";
        case VITTE_IR_ERROR_UNSUPPORTED: return "unsupported";
        case VITTE_IR_ERROR_INTERNAL: return "internal";
        case VITTE_IR_ERROR_COUNT: return "count";
    }

    return "unknown";
}

const char *
vitte_ir_state_name(vitte_ir_state_t state)
{
    switch (state) {
        case VITTE_IR_STATE_INVALID: return "invalid";
        case VITTE_IR_STATE_BUILDING: return "building";
        case VITTE_IR_STATE_SEALED: return "sealed";
        case VITTE_IR_STATE_FAILED: return "failed";
        case VITTE_IR_STATE_DESTROYED: return "destroyed";
        case VITTE_IR_STATE_COUNT: return "count";
    }

    return "unknown";
}

const char *
vitte_ir_type_kind_name(vitte_ir_type_kind_t kind)
{
    switch (kind) {
        case VITTE_IR_TYPE_INVALID: return "invalid";
        case VITTE_IR_TYPE_VOID: return "void";
        case VITTE_IR_TYPE_BOOL: return "bool";
        case VITTE_IR_TYPE_I8: return "i8";
        case VITTE_IR_TYPE_I16: return "i16";
        case VITTE_IR_TYPE_I32: return "i32";
        case VITTE_IR_TYPE_I64: return "i64";
        case VITTE_IR_TYPE_U8: return "u8";
        case VITTE_IR_TYPE_U16: return "u16";
        case VITTE_IR_TYPE_U32: return "u32";
        case VITTE_IR_TYPE_U64: return "u64";
        case VITTE_IR_TYPE_F32: return "f32";
        case VITTE_IR_TYPE_F64: return "f64";
        case VITTE_IR_TYPE_CHAR: return "char";
        case VITTE_IR_TYPE_POINTER: return "pointer";
        case VITTE_IR_TYPE_ARRAY: return "array";
        case VITTE_IR_TYPE_FUNCTION: return "function";
        case VITTE_IR_TYPE_STRUCT: return "struct";
        case VITTE_IR_TYPE_OPAQUE: return "opaque";
        case VITTE_IR_TYPE_COUNT: return "count";
    }

    return "unknown";
}

const char *
vitte_ir_value_kind_name(vitte_ir_value_kind_t kind)
{
    switch (kind) {
        case VITTE_IR_VALUE_INVALID: return "invalid";
        case VITTE_IR_VALUE_UNDEF: return "undef";
        case VITTE_IR_VALUE_POISON: return "poison";
        case VITTE_IR_VALUE_NULL: return "null";
        case VITTE_IR_VALUE_CONSTANT_BOOL: return "constant_bool";
        case VITTE_IR_VALUE_CONSTANT_INTEGER: return "constant_integer";
        case VITTE_IR_VALUE_CONSTANT_FLOAT: return "constant_float";
        case VITTE_IR_VALUE_PARAMETER: return "parameter";
        case VITTE_IR_VALUE_INSTRUCTION: return "instruction";
        case VITTE_IR_VALUE_FUNCTION: return "function";
        case VITTE_IR_VALUE_COUNT: return "count";
    }

    return "unknown";
}

const char *
vitte_ir_opcode_name(vitte_ir_opcode_t opcode)
{
    switch (opcode) {
        case VITTE_IR_OP_INVALID: return "invalid";
        case VITTE_IR_OP_NOP: return "nop";
        case VITTE_IR_OP_ADD: return "add";
        case VITTE_IR_OP_SUB: return "sub";
        case VITTE_IR_OP_MUL: return "mul";
        case VITTE_IR_OP_SDIV: return "sdiv";
        case VITTE_IR_OP_UDIV: return "udiv";
        case VITTE_IR_OP_SREM: return "srem";
        case VITTE_IR_OP_UREM: return "urem";
        case VITTE_IR_OP_FADD: return "fadd";
        case VITTE_IR_OP_FSUB: return "fsub";
        case VITTE_IR_OP_FMUL: return "fmul";
        case VITTE_IR_OP_FDIV: return "fdiv";
        case VITTE_IR_OP_FREM: return "frem";
        case VITTE_IR_OP_NEG: return "neg";
        case VITTE_IR_OP_FNEG: return "fneg";
        case VITTE_IR_OP_AND: return "and";
        case VITTE_IR_OP_OR: return "or";
        case VITTE_IR_OP_XOR: return "xor";
        case VITTE_IR_OP_NOT: return "not";
        case VITTE_IR_OP_SHL: return "shl";
        case VITTE_IR_OP_LSHR: return "lshr";
        case VITTE_IR_OP_ASHR: return "ashr";
        case VITTE_IR_OP_ICMP_EQ: return "icmp_eq";
        case VITTE_IR_OP_ICMP_NE: return "icmp_ne";
        case VITTE_IR_OP_ICMP_SLT: return "icmp_slt";
        case VITTE_IR_OP_ICMP_SLE: return "icmp_sle";
        case VITTE_IR_OP_ICMP_SGT: return "icmp_sgt";
        case VITTE_IR_OP_ICMP_SGE: return "icmp_sge";
        case VITTE_IR_OP_ICMP_ULT: return "icmp_ult";
        case VITTE_IR_OP_ICMP_ULE: return "icmp_ule";
        case VITTE_IR_OP_ICMP_UGT: return "icmp_ugt";
        case VITTE_IR_OP_ICMP_UGE: return "icmp_uge";
        case VITTE_IR_OP_FCMP_EQ: return "fcmp_eq";
        case VITTE_IR_OP_FCMP_NE: return "fcmp_ne";
        case VITTE_IR_OP_FCMP_LT: return "fcmp_lt";
        case VITTE_IR_OP_FCMP_LE: return "fcmp_le";
        case VITTE_IR_OP_FCMP_GT: return "fcmp_gt";
        case VITTE_IR_OP_FCMP_GE: return "fcmp_ge";
        case VITTE_IR_OP_TRUNC: return "trunc";
        case VITTE_IR_OP_ZEXT: return "zext";
        case VITTE_IR_OP_SEXT: return "sext";
        case VITTE_IR_OP_FP_TRUNC: return "fp_trunc";
        case VITTE_IR_OP_FP_EXT: return "fp_ext";
        case VITTE_IR_OP_SI_TO_FP: return "si_to_fp";
        case VITTE_IR_OP_UI_TO_FP: return "ui_to_fp";
        case VITTE_IR_OP_FP_TO_SI: return "fp_to_si";
        case VITTE_IR_OP_FP_TO_UI: return "fp_to_ui";
        case VITTE_IR_OP_PTR_TO_INT: return "ptr_to_int";
        case VITTE_IR_OP_INT_TO_PTR: return "int_to_ptr";
        case VITTE_IR_OP_BITCAST: return "bitcast";
        case VITTE_IR_OP_ALLOCA: return "alloca";
        case VITTE_IR_OP_LOAD: return "load";
        case VITTE_IR_OP_STORE: return "store";
        case VITTE_IR_OP_GEP: return "gep";
        case VITTE_IR_OP_PHI: return "phi";
        case VITTE_IR_OP_SELECT: return "select";
        case VITTE_IR_OP_CALL: return "call";
        case VITTE_IR_OP_BR: return "br";
        case VITTE_IR_OP_COND_BR: return "cond_br";
        case VITTE_IR_OP_SWITCH: return "switch";
        case VITTE_IR_OP_RETURN: return "return";
        case VITTE_IR_OP_UNREACHABLE: return "unreachable";
        case VITTE_IR_OP_COUNT: return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Enum validation                                                           */
/* ========================================================================= */

static bool
vitte_ir_type_kind_valid(vitte_ir_type_kind_t kind)
{
    return kind > VITTE_IR_TYPE_INVALID &&
           kind < VITTE_IR_TYPE_COUNT;
}

static bool
vitte_ir_value_kind_valid(vitte_ir_value_kind_t kind)
{
    return kind > VITTE_IR_VALUE_INVALID &&
           kind < VITTE_IR_VALUE_COUNT;
}

static bool
vitte_ir_opcode_valid(vitte_ir_opcode_t opcode)
{
    return opcode > VITTE_IR_OP_INVALID &&
           opcode < VITTE_IR_OP_COUNT;
}

static bool
vitte_ir_linkage_valid(vitte_ir_linkage_t linkage)
{
    return linkage >= VITTE_IR_LINKAGE_INTERNAL &&
           linkage < VITTE_IR_LINKAGE_COUNT;
}

static bool
vitte_ir_calling_convention_valid(vitte_ir_calling_convention_t convention)
{
    return convention >= VITTE_IR_CALL_DEFAULT &&
           convention < VITTE_IR_CALL_COUNT;
}

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

static bool
vitte_ir_context_envelope_valid(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return false;
    }

    if (context->state <= VITTE_IR_STATE_INVALID ||
        context->state >= VITTE_IR_STATE_DESTROYED) {
        return false;
    }

    if (context->type_count > context->type_capacity ||
        context->value_count > context->value_capacity ||
        context->instruction_count > context->instruction_capacity ||
        context->block_count > context->block_capacity ||
        context->function_count > context->function_capacity ||
        context->string_count > context->string_capacity) {
        return false;
    }

    if ((context->type_count != 0u && context->types == NULL) ||
        (context->value_count != 0u && context->values == NULL) ||
        (context->instruction_count != 0u && context->instructions == NULL) ||
        (context->block_count != 0u && context->blocks == NULL) ||
        (context->function_count != 0u && context->functions == NULL) ||
        (context->string_count != 0u && context->strings == NULL)) {
        return false;
    }

    return true;
}

bool
vitte_ir_is_valid(const vitte_ir_context_t *context)
{
    return vitte_ir_context_envelope_valid(context);
}

static bool
vitte_ir_context_mutable(const vitte_ir_context_t *context)
{
    return vitte_ir_context_envelope_valid(context) &&
           context->state == VITTE_IR_STATE_BUILDING;
}

static bool
vitte_ir_fail(vitte_ir_context_t *context, vitte_ir_error_t error)
{
    if (context != NULL && context->magic == VITTE_IR_MAGIC) {
        context->last_error = error;
    }

    return false;
}

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

static void *
vitte_ir_realloc(vitte_ir_context_t *context, void *pointer, size_t bytes)
{
    void *result;

    if (bytes == 0u) {
        free(pointer);
        return NULL;
    }

    result = realloc(pointer, bytes);

    if (result == NULL) {
        if (context != NULL) {
            context->stats.allocation_failures =
                vitte_ir_u64_add_sat(
                    context->stats.allocation_failures,
                    UINT64_C(1));

            context->last_error = VITTE_IR_ERROR_OUT_OF_MEMORY;
        }

        return NULL;
    }

    if (context != NULL) {
        if (pointer == NULL) {
            context->stats.allocations =
                vitte_ir_u64_add_sat(
                    context->stats.allocations,
                    UINT64_C(1));
        } else {
            context->stats.reallocations =
                vitte_ir_u64_add_sat(
                    context->stats.reallocations,
                    UINT64_C(1));
        }
    }

    return result;
}

static bool
vitte_ir_reserve_raw(
    vitte_ir_context_t *context,
    void **storage,
    size_t *capacity,
    size_t required,
    size_t element_size)
{
    size_t new_capacity;
    size_t bytes;
    void *new_storage;

    if (context == NULL ||
        storage == NULL ||
        capacity == NULL ||
        element_size == 0u) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_ARGUMENT);
    }

    if (required <= *capacity) {
        return true;
    }

    new_capacity = *capacity;

    if (new_capacity == 0u) {
        new_capacity = VITTE_IR_DEFAULT_INITIAL_CAPACITY;
    }

    while (new_capacity < required) {
        size_t doubled;

        if (!vitte_ir_size_mul(new_capacity, (size_t)2u, &doubled) ||
            doubled <= new_capacity) {
            new_capacity = required;
            break;
        }

        new_capacity = doubled;
    }

    if (!vitte_ir_size_mul(new_capacity, element_size, &bytes)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
    }

    new_storage =
        vitte_ir_realloc(context, *storage, bytes);

    if (new_storage == NULL) {
        return false;
    }

    *storage = new_storage;
    *capacity = new_capacity;

    return true;
}

static bool
vitte_ir_reserve_types(vitte_ir_context_t *context, size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)&context->types,
        &context->type_capacity,
        required,
        sizeof(*context->types));
}

static bool
vitte_ir_reserve_values(vitte_ir_context_t *context, size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)&context->values,
        &context->value_capacity,
        required,
        sizeof(*context->values));
}

static bool
vitte_ir_reserve_instructions(vitte_ir_context_t *context, size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)&context->instructions,
        &context->instruction_capacity,
        required,
        sizeof(*context->instructions));
}

static bool
vitte_ir_reserve_blocks(vitte_ir_context_t *context, size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)&context->blocks,
        &context->block_capacity,
        required,
        sizeof(*context->blocks));
}

static bool
vitte_ir_reserve_functions(vitte_ir_context_t *context, size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)&context->functions,
        &context->function_capacity,
        required,
        sizeof(*context->functions));
}

static bool
vitte_ir_reserve_strings(vitte_ir_context_t *context, size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)&context->strings,
        &context->string_capacity,
        required,
        sizeof(*context->strings));
}

static bool
vitte_ir_reserve_value_ids(
    vitte_ir_context_t *context,
    vitte_ir_value_id_t **items,
    size_t *capacity,
    size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)items,
        capacity,
        required,
        sizeof(**items));
}

static bool
vitte_ir_reserve_block_ids(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t **items,
    size_t *capacity,
    size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)items,
        capacity,
        required,
        sizeof(**items));
}

static bool
vitte_ir_reserve_instruction_ids(
    vitte_ir_context_t *context,
    vitte_ir_instruction_id_t **items,
    size_t *capacity,
    size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)items,
        capacity,
        required,
        sizeof(**items));
}

static bool
vitte_ir_reserve_phi_incomings(
    vitte_ir_context_t *context,
    vitte_ir_phi_incoming_t **items,
    size_t *capacity,
    size_t required)
{
    return vitte_ir_reserve_raw(
        context,
        (void **)items,
        capacity,
        required,
        sizeof(**items));
}

/* ========================================================================= */
/* ID validation                                                             */
/* ========================================================================= */

static bool
vitte_ir_string_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_string_id_t id)
{
    return vitte_ir_context_envelope_valid(context) &&
           id != VITTE_IR_INVALID_ID &&
           id <= (uint64_t)context->string_count;
}

static bool
vitte_ir_type_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_type_id_t id)
{
    return vitte_ir_context_envelope_valid(context) &&
           id != VITTE_IR_INVALID_ID &&
           id <= (uint64_t)context->type_count;
}

static bool
vitte_ir_value_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_value_id_t id)
{
    return vitte_ir_context_envelope_valid(context) &&
           id != VITTE_IR_INVALID_ID &&
           id <= (uint64_t)context->value_count;
}

static bool
vitte_ir_instruction_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_instruction_id_t id)
{
    return vitte_ir_context_envelope_valid(context) &&
           id != VITTE_IR_INVALID_ID &&
           id <= (uint64_t)context->instruction_count;
}

static bool
vitte_ir_block_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id)
{
    return vitte_ir_context_envelope_valid(context) &&
           id != VITTE_IR_INVALID_ID &&
           id <= (uint64_t)context->block_count;
}

static bool
vitte_ir_function_id_exists(
    const vitte_ir_context_t *context,
    vitte_ir_function_id_t id)
{
    return vitte_ir_context_envelope_valid(context) &&
           id != VITTE_IR_INVALID_ID &&
           id <= (uint64_t)context->function_count;
}

/* ========================================================================= */
/* Strings                                                                   */
/* ========================================================================= */

static vitte_ir_string_id_t
vitte_ir_find_string(
    const vitte_ir_context_t *context,
    const char *data,
    size_t length,
    uint64_t hash)
{
    size_t index;

    if (!vitte_ir_context_envelope_valid(context)) {
        return VITTE_IR_INVALID_ID;
    }

    for (index = 0u; index < context->string_count; ++index) {
        const vitte_ir_string_t *string = &context->strings[index];

        if (string->hash == hash &&
            string->length == length &&
            (length == 0u ||
             memcmp(string->data, data, length) == 0)) {
            return string->id;
        }
    }

    return VITTE_IR_INVALID_ID;
}

vitte_ir_string_id_t
vitte_ir_intern_string(
    vitte_ir_context_t *context,
    const char *data,
    size_t length)
{
    uint64_t hash;
    vitte_ir_string_id_t existing;
    size_t required;
    size_t allocation_size;
    size_t new_bytes;
    char *copy;
    vitte_ir_string_t *string;

    if (!vitte_ir_context_mutable(context)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STATE);
        return VITTE_IR_INVALID_ID;
    }

    if (data == NULL && length != 0u) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_ARGUMENT);
        return VITTE_IR_INVALID_ID;
    }

    hash = vitte_ir_hash_bytes(VITTE_IR_FNV_OFFSET, data, length);

    existing =
        vitte_ir_find_string(context, data, length, hash);

    if (existing != VITTE_IR_INVALID_ID) {
        return existing;
    }

    if (!vitte_ir_size_add(context->string_count, 1u, &required) ||
        !vitte_ir_size_add(length, 1u, &allocation_size) ||
        !vitte_ir_size_add(context->string_bytes, length, &new_bytes)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
        return VITTE_IR_INVALID_ID;
    }

    if (new_bytes > context->max_string_bytes) {
        vitte_ir_fail(context, VITTE_IR_ERROR_LIMIT);
        return VITTE_IR_INVALID_ID;
    }

    if (!vitte_ir_reserve_strings(context, required)) {
        return VITTE_IR_INVALID_ID;
    }

    copy = (char *)malloc(allocation_size);

    if (copy == NULL) {
        context->stats.allocation_failures =
            vitte_ir_u64_add_sat(
                context->stats.allocation_failures,
                UINT64_C(1));

        vitte_ir_fail(context, VITTE_IR_ERROR_OUT_OF_MEMORY);
        return VITTE_IR_INVALID_ID;
    }

    context->stats.allocations =
        vitte_ir_u64_add_sat(context->stats.allocations, UINT64_C(1));

    if (length != 0u) {
        memcpy(copy, data, length);
    }

    copy[length] = '\0';

    string = &context->strings[context->string_count];

    memset(string, 0, sizeof(*string));

    string->id =
        (vitte_ir_string_id_t)(context->string_count + 1u);

    string->data = copy;
    string->length = length;
    string->hash = hash;

    context->string_count = required;
    context->string_bytes = new_bytes;

    context->stats.strings_interned =
        vitte_ir_u64_add_sat(
            context->stats.strings_interned,
            UINT64_C(1));

    context->last_error = VITTE_IR_ERROR_NONE;

    return string->id;
}

const vitte_ir_string_t *
vitte_ir_get_string(
    const vitte_ir_context_t *context,
    vitte_ir_string_id_t id)
{
    if (!vitte_ir_string_id_exists(context, id)) {
        return NULL;
    }

    return &context->strings[(size_t)(id - UINT64_C(1))];
}

/* ========================================================================= */
/* Types                                                                     */
/* ========================================================================= */

static bool
vitte_ir_type_equal(
    const vitte_ir_type_t *left,
    const vitte_ir_type_desc_t *right)
{
    size_t index;

    if (left == NULL || right == NULL) {
        return false;
    }

    if (left->kind != right->kind ||
        left->element_type != right->element_type ||
        left->array_length != right->array_length ||
        left->return_type != right->return_type ||
        left->parameter_count != right->parameter_count ||
        left->is_variadic != right->is_variadic ||
        left->name != right->name) {
        return false;
    }

    for (index = 0u; index < left->parameter_count; ++index) {
        if (left->parameter_types[index] !=
            right->parameter_types[index]) {
            return false;
        }
    }

    return true;
}

static vitte_ir_type_id_t
vitte_ir_find_type(
    const vitte_ir_context_t *context,
    const vitte_ir_type_desc_t *description)
{
    size_t index;

    if (!vitte_ir_context_envelope_valid(context) ||
        description == NULL) {
        return VITTE_IR_INVALID_ID;
    }

    for (index = 0u; index < context->type_count; ++index) {
        if (vitte_ir_type_equal(
                &context->types[index],
                description)) {
            return context->types[index].id;
        }
    }

    return VITTE_IR_INVALID_ID;
}

vitte_ir_type_id_t
vitte_ir_intern_type(
    vitte_ir_context_t *context,
    const vitte_ir_type_desc_t *description)
{
    vitte_ir_type_id_t existing;
    vitte_ir_type_t *type;
    size_t required;
    size_t bytes;

    if (!vitte_ir_context_mutable(context)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STATE);
        return VITTE_IR_INVALID_ID;
    }

    if (description == NULL ||
        !vitte_ir_type_kind_valid(description->kind)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
        return VITTE_IR_INVALID_ID;
    }

    if (description->element_type != VITTE_IR_INVALID_ID &&
        !vitte_ir_type_id_exists(context, description->element_type)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
        return VITTE_IR_INVALID_ID;
    }

    if (description->return_type != VITTE_IR_INVALID_ID &&
        !vitte_ir_type_id_exists(context, description->return_type)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
        return VITTE_IR_INVALID_ID;
    }

    if (description->name != VITTE_IR_INVALID_ID &&
        !vitte_ir_string_id_exists(context, description->name)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STRING);
        return VITTE_IR_INVALID_ID;
    }

    if (description->parameter_count != 0u &&
        description->parameter_types == NULL) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_ARGUMENT);
        return VITTE_IR_INVALID_ID;
    }

    {
        size_t index;

        for (index = 0u;
             index < description->parameter_count;
             ++index) {
            if (!vitte_ir_type_id_exists(
                    context,
                    description->parameter_types[index])) {
                vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
                return VITTE_IR_INVALID_ID;
            }
        }
    }

    existing = vitte_ir_find_type(context, description);

    if (existing != VITTE_IR_INVALID_ID) {
        return existing;
    }

    if (context->type_count >= context->max_types) {
        vitte_ir_fail(context, VITTE_IR_ERROR_LIMIT);
        return VITTE_IR_INVALID_ID;
    }

    if (!vitte_ir_size_add(context->type_count, 1u, &required) ||
        !vitte_ir_reserve_types(context, required)) {
        return VITTE_IR_INVALID_ID;
    }

    type = &context->types[context->type_count];
    memset(type, 0, sizeof(*type));

    type->id =
        (vitte_ir_type_id_t)(context->type_count + 1u);

    type->kind = description->kind;
    type->element_type = description->element_type;
    type->array_length = description->array_length;
    type->return_type = description->return_type;
    type->parameter_count = description->parameter_count;
    type->is_variadic = description->is_variadic;
    type->name = description->name;

    if (description->parameter_count != 0u) {
        if (!vitte_ir_size_mul(
                description->parameter_count,
                sizeof(*type->parameter_types),
                &bytes)) {
            vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
            return VITTE_IR_INVALID_ID;
        }

        type->parameter_types =
            (vitte_ir_type_id_t *)malloc(bytes);

        if (type->parameter_types == NULL) {
            context->stats.allocation_failures =
                vitte_ir_u64_add_sat(
                    context->stats.allocation_failures,
                    UINT64_C(1));

            vitte_ir_fail(context, VITTE_IR_ERROR_OUT_OF_MEMORY);
            return VITTE_IR_INVALID_ID;
        }

        context->stats.allocations =
            vitte_ir_u64_add_sat(
                context->stats.allocations,
                UINT64_C(1));

        memcpy(
            type->parameter_types,
            description->parameter_types,
            bytes);
    }

    context->type_count = required;

    context->stats.types_created =
        vitte_ir_u64_add_sat(
            context->stats.types_created,
            UINT64_C(1));

    context->last_error = VITTE_IR_ERROR_NONE;

    return type->id;
}

const vitte_ir_type_t *
vitte_ir_get_type(
    const vitte_ir_context_t *context,
    vitte_ir_type_id_t id)
{
    if (!vitte_ir_type_id_exists(context, id)) {
        return NULL;
    }

    return &context->types[(size_t)(id - UINT64_C(1))];
}

/* ========================================================================= */
/* Values                                                                    */
/* ========================================================================= */

vitte_ir_value_id_t
vitte_ir_add_value(
    vitte_ir_context_t *context,
    const vitte_ir_value_desc_t *description)
{
    vitte_ir_value_t *value;
    size_t required;

    if (!vitte_ir_context_mutable(context)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STATE);
        return VITTE_IR_INVALID_ID;
    }

    if (description == NULL ||
        !vitte_ir_value_kind_valid(description->kind) ||
        !vitte_ir_type_id_exists(context, description->type)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
        return VITTE_IR_INVALID_ID;
    }

    if (description->name != VITTE_IR_INVALID_ID &&
        !vitte_ir_string_id_exists(context, description->name)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STRING);
        return VITTE_IR_INVALID_ID;
    }

    if (context->value_count >= context->max_values) {
        vitte_ir_fail(context, VITTE_IR_ERROR_LIMIT);
        return VITTE_IR_INVALID_ID;
    }

    if (!vitte_ir_size_add(context->value_count, 1u, &required) ||
        !vitte_ir_reserve_values(context, required)) {
        return VITTE_IR_INVALID_ID;
    }

    value = &context->values[context->value_count];

    memset(value, 0, sizeof(*value));

    value->id =
        (vitte_ir_value_id_t)(context->value_count + 1u);

    value->kind = description->kind;
    value->type = description->type;
    value->name = description->name;
    value->span = description->span;

    value->integer_value = description->integer_value;
    value->float_value = description->float_value;
    value->bool_value = description->bool_value;

    value->function = description->function;
    value->instruction = description->instruction;
    value->parameter_index = description->parameter_index;

    context->value_count = required;

    context->stats.values_created =
        vitte_ir_u64_add_sat(
            context->stats.values_created,
            UINT64_C(1));

    context->last_error = VITTE_IR_ERROR_NONE;

    return value->id;
}

vitte_ir_value_id_t
vitte_ir_const_bool(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type,
    bool value)
{
    vitte_ir_value_desc_t description;

    memset(&description, 0, sizeof(description));

    description.kind = VITTE_IR_VALUE_CONSTANT_BOOL;
    description.type = type;
    description.bool_value = value;

    return vitte_ir_add_value(context, &description);
}

vitte_ir_value_id_t
vitte_ir_const_integer(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type,
    uint64_t value)
{
    vitte_ir_value_desc_t description;

    memset(&description, 0, sizeof(description));

    description.kind = VITTE_IR_VALUE_CONSTANT_INTEGER;
    description.type = type;
    description.integer_value = value;

    return vitte_ir_add_value(context, &description);
}

vitte_ir_value_id_t
vitte_ir_const_float(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type,
    double value)
{
    vitte_ir_value_desc_t description;

    if (!isfinite(value)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
        return VITTE_IR_INVALID_ID;
    }

    memset(&description, 0, sizeof(description));

    description.kind = VITTE_IR_VALUE_CONSTANT_FLOAT;
    description.type = type;
    description.float_value = value;

    return vitte_ir_add_value(context, &description);
}

vitte_ir_value_id_t
vitte_ir_null(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type)
{
    vitte_ir_value_desc_t description;

    memset(&description, 0, sizeof(description));

    description.kind = VITTE_IR_VALUE_NULL;
    description.type = type;

    return vitte_ir_add_value(context, &description);
}

vitte_ir_value_id_t
vitte_ir_undef(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type)
{
    vitte_ir_value_desc_t description;

    memset(&description, 0, sizeof(description));

    description.kind = VITTE_IR_VALUE_UNDEF;
    description.type = type;

    return vitte_ir_add_value(context, &description);
}

vitte_ir_value_id_t
vitte_ir_poison(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type)
{
    vitte_ir_value_desc_t description;

    memset(&description, 0, sizeof(description));

    description.kind = VITTE_IR_VALUE_POISON;
    description.type = type;

    return vitte_ir_add_value(context, &description);
}

const vitte_ir_value_t *
vitte_ir_get_value(
    const vitte_ir_context_t *context,
    vitte_ir_value_id_t id)
{
    if (!vitte_ir_value_id_exists(context, id)) {
        return NULL;
    }

    return &context->values[(size_t)(id - UINT64_C(1))];
}

/* ========================================================================= */
/* Functions                                                                 */
/* ========================================================================= */

vitte_ir_function_id_t
vitte_ir_add_function(
    vitte_ir_context_t *context,
    const vitte_ir_function_desc_t *description)
{
    vitte_ir_function_t *function;
    size_t required;

    if (!vitte_ir_context_mutable(context)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STATE);
        return VITTE_IR_INVALID_ID;
    }

    if (description == NULL ||
        !vitte_ir_string_id_exists(context, description->name) ||
        !vitte_ir_type_id_exists(context, description->type) ||
        !vitte_ir_linkage_valid(description->linkage) ||
        !vitte_ir_calling_convention_valid(
            description->calling_convention)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_FUNCTION);
        return VITTE_IR_INVALID_ID;
    }

    if (context->function_count >= context->max_functions) {
        vitte_ir_fail(context, VITTE_IR_ERROR_LIMIT);
        return VITTE_IR_INVALID_ID;
    }

    if (!vitte_ir_size_add(context->function_count, 1u, &required) ||
        !vitte_ir_reserve_functions(context, required)) {
        return VITTE_IR_INVALID_ID;
    }

    function = &context->functions[context->function_count];

    memset(function, 0, sizeof(*function));

    function->id =
        (vitte_ir_function_id_t)(context->function_count + 1u);

    function->name = description->name;
    function->type = description->type;
    function->linkage = description->linkage;
    function->calling_convention = description->calling_convention;
    function->is_declaration = description->is_declaration;
    function->is_variadic = description->is_variadic;
    function->span = description->span;

    context->function_count = required;

    context->stats.functions_created =
        vitte_ir_u64_add_sat(
            context->stats.functions_created,
            UINT64_C(1));

    context->last_error = VITTE_IR_ERROR_NONE;

    return function->id;
}

const vitte_ir_function_t *
vitte_ir_get_function(
    const vitte_ir_context_t *context,
    vitte_ir_function_id_t id)
{
    if (!vitte_ir_function_id_exists(context, id)) {
        return NULL;
    }

    return &context->functions[(size_t)(id - UINT64_C(1))];
}

vitte_ir_function_t *
vitte_ir_get_function_mut(
    vitte_ir_context_t *context,
    vitte_ir_function_id_t id)
{
    if (!vitte_ir_context_mutable(context) ||
        !vitte_ir_function_id_exists(context, id)) {
        return NULL;
    }

    return &context->functions[(size_t)(id - UINT64_C(1))];
}

static bool
vitte_ir_function_append_parameter(
    vitte_ir_context_t *context,
    vitte_ir_function_t *function,
    vitte_ir_value_id_t value)
{
    size_t required;

    if (!vitte_ir_size_add(
            function->parameter_count,
            1u,
            &required)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
    }

    if (!vitte_ir_reserve_value_ids(
            context,
            &function->parameters,
            &function->parameter_capacity,
            required)) {
        return false;
    }

    function->parameters[function->parameter_count] = value;
    function->parameter_count = required;

    return true;
}

vitte_ir_value_id_t
vitte_ir_add_parameter(
    vitte_ir_context_t *context,
    vitte_ir_function_id_t function_id,
    vitte_ir_type_id_t type,
    vitte_ir_string_id_t name,
    vitte_ir_span_t span)
{
    vitte_ir_function_t *function;
    vitte_ir_value_desc_t description;
    vitte_ir_value_id_t value;

    if (!vitte_ir_context_mutable(context) ||
        !vitte_ir_function_id_exists(context, function_id) ||
        !vitte_ir_type_id_exists(context, type)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_ARGUMENT);
        return VITTE_IR_INVALID_ID;
    }

    if (name != VITTE_IR_INVALID_ID &&
        !vitte_ir_string_id_exists(context, name)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STRING);
        return VITTE_IR_INVALID_ID;
    }

    function =
        &context->functions[(size_t)(function_id - UINT64_C(1))];

    memset(&description, 0, sizeof(description));

    description.kind = VITTE_IR_VALUE_PARAMETER;
    description.type = type;
    description.name = name;
    description.span = span;
    description.function = function_id;
    description.parameter_index = function->parameter_count;

    value = vitte_ir_add_value(context, &description);

    if (value == VITTE_IR_INVALID_ID) {
        return value;
    }

    if (!vitte_ir_function_append_parameter(
            context,
            function,
            value)) {
        return VITTE_IR_INVALID_ID;
    }

    return value;
}

/* ========================================================================= */
/* Blocks                                                                    */
/* ========================================================================= */

static bool
vitte_ir_function_append_block(
    vitte_ir_context_t *context,
    vitte_ir_function_t *function,
    vitte_ir_block_id_t block)
{
    size_t required;

    if (!vitte_ir_size_add(function->block_count, 1u, &required)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
    }

    if (!vitte_ir_reserve_block_ids(
            context,
            &function->blocks,
            &function->block_capacity,
            required)) {
        return false;
    }

    function->blocks[function->block_count] = block;
    function->block_count = required;

    if (function->entry_block == VITTE_IR_INVALID_ID) {
        function->entry_block = block;
    }

    return true;
}

vitte_ir_block_id_t
vitte_ir_add_block(
    vitte_ir_context_t *context,
    vitte_ir_function_id_t function_id,
    vitte_ir_string_id_t name,
    vitte_ir_span_t span)
{
    vitte_ir_function_t *function;
    vitte_ir_block_t *block;
    size_t required;

    if (!vitte_ir_context_mutable(context) ||
        !vitte_ir_function_id_exists(context, function_id)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_FUNCTION);
        return VITTE_IR_INVALID_ID;
    }

    if (name != VITTE_IR_INVALID_ID &&
        !vitte_ir_string_id_exists(context, name)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STRING);
        return VITTE_IR_INVALID_ID;
    }

    if (context->block_count >= context->max_blocks) {
        vitte_ir_fail(context, VITTE_IR_ERROR_LIMIT);
        return VITTE_IR_INVALID_ID;
    }

    if (!vitte_ir_size_add(context->block_count, 1u, &required) ||
        !vitte_ir_reserve_blocks(context, required)) {
        return VITTE_IR_INVALID_ID;
    }

    block = &context->blocks[context->block_count];

    memset(block, 0, sizeof(*block));

    block->id =
        (vitte_ir_block_id_t)(context->block_count + 1u);

    block->function = function_id;
    block->name = name;
    block->span = span;

    context->block_count = required;

    function =
        &context->functions[(size_t)(function_id - UINT64_C(1))];

    if (!vitte_ir_function_append_block(
            context,
            function,
            block->id)) {
        --context->block_count;
        memset(block, 0, sizeof(*block));
        return VITTE_IR_INVALID_ID;
    }

    context->stats.blocks_created =
        vitte_ir_u64_add_sat(
            context->stats.blocks_created,
            UINT64_C(1));

    context->last_error = VITTE_IR_ERROR_NONE;

    return block->id;
}

const vitte_ir_block_t *
vitte_ir_get_block(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id)
{
    if (!vitte_ir_block_id_exists(context, id)) {
        return NULL;
    }

    return &context->blocks[(size_t)(id - UINT64_C(1))];
}

vitte_ir_block_t *
vitte_ir_get_block_mut(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t id)
{
    if (!vitte_ir_context_mutable(context) ||
        !vitte_ir_block_id_exists(context, id)) {
        return NULL;
    }

    return &context->blocks[(size_t)(id - UINT64_C(1))];
}

/* ========================================================================= */
/* CFG                                                                       */
/* ========================================================================= */

static bool
vitte_ir_block_append_successor(
    vitte_ir_context_t *context,
    vitte_ir_block_t *block,
    vitte_ir_block_id_t successor)
{
    size_t index;
    size_t required;

    for (index = 0u; index < block->successor_count; ++index) {
        if (block->successors[index] == successor) {
            return true;
        }
    }

    if (!vitte_ir_size_add(block->successor_count, 1u, &required)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
    }

    if (!vitte_ir_reserve_block_ids(
            context,
            &block->successors,
            &block->successor_capacity,
            required)) {
        return false;
    }

    block->successors[block->successor_count] = successor;
    block->successor_count = required;

    return true;
}

static bool
vitte_ir_block_append_predecessor(
    vitte_ir_context_t *context,
    vitte_ir_block_t *block,
    vitte_ir_block_id_t predecessor)
{
    size_t index;
    size_t required;

    for (index = 0u; index < block->predecessor_count; ++index) {
        if (block->predecessors[index] == predecessor) {
            return true;
        }
    }

    if (!vitte_ir_size_add(block->predecessor_count, 1u, &required)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
    }

    if (!vitte_ir_reserve_block_ids(
            context,
            &block->predecessors,
            &block->predecessor_capacity,
            required)) {
        return false;
    }

    block->predecessors[block->predecessor_count] = predecessor;
    block->predecessor_count = required;

    return true;
}

bool
vitte_ir_add_cfg_edge(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t source_id,
    vitte_ir_block_id_t target_id)
{
    vitte_ir_block_t *source;
    vitte_ir_block_t *target;

    if (!vitte_ir_context_mutable(context) ||
        !vitte_ir_block_id_exists(context, source_id) ||
        !vitte_ir_block_id_exists(context, target_id)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_BLOCK);
    }

    source = &context->blocks[(size_t)(source_id - UINT64_C(1))];
    target = &context->blocks[(size_t)(target_id - UINT64_C(1))];

    if (source->function != target->function) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_CFG);
    }

    if (!vitte_ir_block_append_successor(context, source, target_id) ||
        !vitte_ir_block_append_predecessor(context, target, source_id)) {
        return false;
    }

    context->stats.cfg_edges =
        vitte_ir_u64_add_sat(
            context->stats.cfg_edges,
            UINT64_C(1));

    return true;
}

/* ========================================================================= */
/* Instructions                                                              */
/* ========================================================================= */

static bool
vitte_ir_opcode_is_terminator_internal(vitte_ir_opcode_t opcode)
{
    switch (opcode) {
        case VITTE_IR_OP_BR:
        case VITTE_IR_OP_COND_BR:
        case VITTE_IR_OP_SWITCH:
        case VITTE_IR_OP_RETURN:
        case VITTE_IR_OP_UNREACHABLE:
            return true;

        default:
            return false;
    }
}

bool
vitte_ir_opcode_is_terminator(vitte_ir_opcode_t opcode)
{
    return vitte_ir_opcode_is_terminator_internal(opcode);
}

static bool
vitte_ir_opcode_produces_value_internal(vitte_ir_opcode_t opcode)
{
    switch (opcode) {
        case VITTE_IR_OP_INVALID:
        case VITTE_IR_OP_NOP:
        case VITTE_IR_OP_STORE:
        case VITTE_IR_OP_BR:
        case VITTE_IR_OP_COND_BR:
        case VITTE_IR_OP_SWITCH:
        case VITTE_IR_OP_RETURN:
        case VITTE_IR_OP_UNREACHABLE:
        case VITTE_IR_OP_COUNT:
            return false;

        default:
            return true;
    }
}

bool
vitte_ir_opcode_produces_value(vitte_ir_opcode_t opcode)
{
    return vitte_ir_opcode_produces_value_internal(opcode);
}

static bool
vitte_ir_block_append_instruction(
    vitte_ir_context_t *context,
    vitte_ir_block_t *block,
    vitte_ir_instruction_id_t instruction)
{
    size_t required;

    if (!vitte_ir_size_add(block->instruction_count, 1u, &required)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
    }

    if (!vitte_ir_reserve_instruction_ids(
            context,
            &block->instructions,
            &block->instruction_capacity,
            required)) {
        return false;
    }

    block->instructions[block->instruction_count] = instruction;
    block->instruction_count = required;

    return true;
}

static bool
vitte_ir_value_add_use(
    vitte_ir_context_t *context,
    vitte_ir_value_id_t value)
{
    vitte_ir_value_t *entry;

    if (!vitte_ir_value_id_exists(context, value)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
    }

    entry = &context->values[(size_t)(value - UINT64_C(1))];

    entry->use_count =
        vitte_ir_u64_add_sat(entry->use_count, UINT64_C(1));

    return true;
}

vitte_ir_instruction_id_t
vitte_ir_add_instruction(
    vitte_ir_context_t *context,
    const vitte_ir_instruction_desc_t *description)
{
    vitte_ir_block_t *block;
    vitte_ir_instruction_t *instruction;
    size_t required;
    size_t index;

    if (!vitte_ir_context_mutable(context)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STATE);
        return VITTE_IR_INVALID_ID;
    }

    if (description == NULL ||
        !vitte_ir_opcode_valid(description->opcode) ||
        !vitte_ir_block_id_exists(context, description->block)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_INSTRUCTION);
        return VITTE_IR_INVALID_ID;
    }

    if (description->result_type != VITTE_IR_INVALID_ID &&
        !vitte_ir_type_id_exists(context, description->result_type)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
        return VITTE_IR_INVALID_ID;
    }

    if (description->operand_count != 0u &&
        description->operands == NULL) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_ARGUMENT);
        return VITTE_IR_INVALID_ID;
    }

    for (index = 0u; index < description->operand_count; ++index) {
        if (!vitte_ir_value_id_exists(
                context,
                description->operands[index])) {
            vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
            return VITTE_IR_INVALID_ID;
        }
    }

    block =
        &context->blocks[(size_t)(description->block - UINT64_C(1))];

    if (block->terminated) {
        vitte_ir_fail(context, VITTE_IR_ERROR_TERMINATED_BLOCK);
        return VITTE_IR_INVALID_ID;
    }

    if (context->instruction_count >= context->max_instructions) {
        vitte_ir_fail(context, VITTE_IR_ERROR_LIMIT);
        return VITTE_IR_INVALID_ID;
    }

    if (!vitte_ir_size_add(
            context->instruction_count,
            1u,
            &required) ||
        !vitte_ir_reserve_instructions(context, required)) {
        return VITTE_IR_INVALID_ID;
    }

    instruction =
        &context->instructions[context->instruction_count];

    memset(instruction, 0, sizeof(*instruction));

    instruction->id =
        (vitte_ir_instruction_id_t)(
            context->instruction_count + 1u);

    instruction->opcode = description->opcode;
    instruction->block = description->block;
    instruction->result_type = description->result_type;
    instruction->span = description->span;

    context->instruction_count = required;

    for (index = 0u; index < description->operand_count; ++index) {
        if (!vitte_ir_instruction_append_operand(
                context,
                instruction,
                description->operands[index])) {
            return VITTE_IR_INVALID_ID;
        }
    }

    if (!vitte_ir_block_append_instruction(
            context,
            block,
            instruction->id)) {
        return VITTE_IR_INVALID_ID;
    }

    if (vitte_ir_opcode_produces_value_internal(
            description->opcode)) {
        vitte_ir_value_desc_t value_description;

        if (description->result_type == VITTE_IR_INVALID_ID) {
            vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
            return VITTE_IR_INVALID_ID;
        }

        memset(&value_description, 0, sizeof(value_description));

        value_description.kind = VITTE_IR_VALUE_INSTRUCTION;
        value_description.type = description->result_type;
        value_description.name = description->result_name;
        value_description.span = description->span;
        value_description.instruction = instruction->id;

        instruction->result =
            vitte_ir_add_value(context, &value_description);

        if (instruction->result == VITTE_IR_INVALID_ID) {
            return VITTE_IR_INVALID_ID;
        }
    }

    if (vitte_ir_opcode_is_terminator_internal(
            description->opcode)) {
        block->terminated = true;
        block->terminator = instruction->id;
    }

    context->stats.instructions_created =
        vitte_ir_u64_add_sat(
            context->stats.instructions_created,
            UINT64_C(1));

    context->last_error = VITTE_IR_ERROR_NONE;

    return instruction->id;
}

static bool
vitte_ir_instruction_append_operand(
    vitte_ir_context_t *context,
    vitte_ir_instruction_t *instruction,
    vitte_ir_value_id_t value)
{
    size_t required;

    if (instruction == NULL ||
        !vitte_ir_value_id_exists(context, value)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
    }

    if (!vitte_ir_size_add(
            instruction->operand_count,
            1u,
            &required)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
    }

    if (!vitte_ir_reserve_value_ids(
            context,
            &instruction->operands,
            &instruction->operand_capacity,
            required)) {
        return false;
    }

    instruction->operands[instruction->operand_count] = value;
    instruction->operand_count = required;

    return vitte_ir_value_add_use(context, value);
}

const vitte_ir_instruction_t *
vitte_ir_get_instruction(
    const vitte_ir_context_t *context,
    vitte_ir_instruction_id_t id)
{
    if (!vitte_ir_instruction_id_exists(context, id)) {
        return NULL;
    }

    return &context->instructions[(size_t)(id - UINT64_C(1))];
}

vitte_ir_instruction_t *
vitte_ir_get_instruction_mut(
    vitte_ir_context_t *context,
    vitte_ir_instruction_id_t id)
{
    if (!vitte_ir_context_mutable(context) ||
        !vitte_ir_instruction_id_exists(context, id)) {
        return NULL;
    }

    return &context->instructions[(size_t)(id - UINT64_C(1))];
}

vitte_ir_value_id_t
vitte_ir_instruction_result(
    const vitte_ir_context_t *context,
    vitte_ir_instruction_id_t id)
{
    const vitte_ir_instruction_t *instruction;

    instruction = vitte_ir_get_instruction(context, id);

    if (instruction == NULL) {
        return VITTE_IR_INVALID_ID;
    }

    return instruction->result;
}

/* ========================================================================= */
/* Phi                                                                       */
/* ========================================================================= */

bool
vitte_ir_phi_add_incoming(
    vitte_ir_context_t *context,
    vitte_ir_instruction_id_t instruction_id,
    vitte_ir_block_id_t block_id,
    vitte_ir_value_id_t value_id)
{
    vitte_ir_instruction_t *instruction;
    size_t required;
    size_t index;

    if (!vitte_ir_context_mutable(context) ||
        !vitte_ir_instruction_id_exists(context, instruction_id) ||
        !vitte_ir_block_id_exists(context, block_id) ||
        !vitte_ir_value_id_exists(context, value_id)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_ARGUMENT);
    }

    instruction =
        &context->instructions[
            (size_t)(instruction_id - UINT64_C(1))];

    if (instruction->opcode != VITTE_IR_OP_PHI) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_PHI);
    }

    for (index = 0u; index < instruction->phi_count; ++index) {
        if (instruction->phi_incomings[index].block == block_id) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_DUPLICATE);
        }
    }

    if (!vitte_ir_size_add(instruction->phi_count, 1u, &required)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_OVERFLOW);
    }

    if (!vitte_ir_reserve_phi_incomings(
            context,
            &instruction->phi_incomings,
            &instruction->phi_capacity,
            required)) {
        return false;
    }

    instruction->phi_incomings[instruction->phi_count].block = block_id;
    instruction->phi_incomings[instruction->phi_count].value = value_id;

    instruction->phi_count = required;

    if (!vitte_ir_value_add_use(context, value_id)) {
        return false;
    }

    context->stats.phi_incomings =
        vitte_ir_u64_add_sat(
            context->stats.phi_incomings,
            UINT64_C(1));

    return true;
}

/* ========================================================================= */
/* Branch helpers                                                            */
/* ========================================================================= */

vitte_ir_instruction_id_t
vitte_ir_build_br(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t block,
    vitte_ir_block_id_t target,
    vitte_ir_span_t span)
{
    vitte_ir_instruction_desc_t description;
    vitte_ir_instruction_id_t instruction;

    if (!vitte_ir_block_id_exists(context, target)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_BLOCK);
        return VITTE_IR_INVALID_ID;
    }

    memset(&description, 0, sizeof(description));

    description.opcode = VITTE_IR_OP_BR;
    description.block = block;
    description.span = span;

    instruction =
        vitte_ir_add_instruction(context, &description);

    if (instruction == VITTE_IR_INVALID_ID) {
        return instruction;
    }

    context->instructions[
        (size_t)(instruction - UINT64_C(1))].target_true = target;

    if (!vitte_ir_add_cfg_edge(context, block, target)) {
        return VITTE_IR_INVALID_ID;
    }

    return instruction;
}

vitte_ir_instruction_id_t
vitte_ir_build_cond_br(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t block,
    vitte_ir_value_id_t condition,
    vitte_ir_block_id_t true_target,
    vitte_ir_block_id_t false_target,
    vitte_ir_span_t span)
{
    vitte_ir_instruction_desc_t description;
    vitte_ir_value_id_t operands[1];
    vitte_ir_instruction_id_t instruction;
    vitte_ir_instruction_t *entry;

    if (!vitte_ir_value_id_exists(context, condition) ||
        !vitte_ir_block_id_exists(context, true_target) ||
        !vitte_ir_block_id_exists(context, false_target)) {
        vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_ARGUMENT);
        return VITTE_IR_INVALID_ID;
    }

    operands[0] = condition;

    memset(&description, 0, sizeof(description));

    description.opcode = VITTE_IR_OP_COND_BR;
    description.block = block;
    description.operands = operands;
    description.operand_count = 1u;
    description.span = span;

    instruction =
        vitte_ir_add_instruction(context, &description);

    if (instruction == VITTE_IR_INVALID_ID) {
        return instruction;
    }

    entry =
        &context->instructions[
            (size_t)(instruction - UINT64_C(1))];

    entry->target_true = true_target;
    entry->target_false = false_target;

    if (!vitte_ir_add_cfg_edge(context, block, true_target) ||
        !vitte_ir_add_cfg_edge(context, block, false_target)) {
        return VITTE_IR_INVALID_ID;
    }

    return instruction;
}

vitte_ir_instruction_id_t
vitte_ir_build_return(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t block,
    vitte_ir_value_id_t value,
    vitte_ir_span_t span)
{
    vitte_ir_instruction_desc_t description;
    vitte_ir_value_id_t operand;

    memset(&description, 0, sizeof(description));

    description.opcode = VITTE_IR_OP_RETURN;
    description.block = block;
    description.span = span;

    if (value != VITTE_IR_INVALID_ID) {
        if (!vitte_ir_value_id_exists(context, value)) {
            vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
            return VITTE_IR_INVALID_ID;
        }

        operand = value;
        description.operands = &operand;
        description.operand_count = 1u;
    }

    return vitte_ir_add_instruction(context, &description);
}

vitte_ir_instruction_id_t
vitte_ir_build_unreachable(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t block,
    vitte_ir_span_t span)
{
    vitte_ir_instruction_desc_t description;

    memset(&description, 0, sizeof(description));

    description.opcode = VITTE_IR_OP_UNREACHABLE;
    description.block = block;
    description.span = span;

    return vitte_ir_add_instruction(context, &description);
}

/* ========================================================================= */
/* Generic unary/binary builders                                             */
/* ========================================================================= */

vitte_ir_instruction_id_t
vitte_ir_build_unary(
    vitte_ir_context_t *context,
    vitte_ir_opcode_t opcode,
    vitte_ir_block_id_t block,
    vitte_ir_type_id_t result_type,
    vitte_ir_value_id_t operand,
    vitte_ir_span_t span)
{
    vitte_ir_instruction_desc_t description;
    vitte_ir_value_id_t operands[1];

    operands[0] = operand;

    memset(&description, 0, sizeof(description));

    description.opcode = opcode;
    description.block = block;
    description.result_type = result_type;
    description.operands = operands;
    description.operand_count = 1u;
    description.span = span;

    return vitte_ir_add_instruction(context, &description);
}

vitte_ir_instruction_id_t
vitte_ir_build_binary(
    vitte_ir_context_t *context,
    vitte_ir_opcode_t opcode,
    vitte_ir_block_id_t block,
    vitte_ir_type_id_t result_type,
    vitte_ir_value_id_t left,
    vitte_ir_value_id_t right,
    vitte_ir_span_t span)
{
    vitte_ir_instruction_desc_t description;
    vitte_ir_value_id_t operands[2];

    operands[0] = left;
    operands[1] = right;

    memset(&description, 0, sizeof(description));

    description.opcode = opcode;
    description.block = block;
    description.result_type = result_type;
    description.operands = operands;
    description.operand_count = 2u;
    description.span = span;

    return vitte_ir_add_instruction(context, &description);
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

static bool
vitte_ir_validate_type(
    vitte_ir_context_t *context,
    const vitte_ir_type_t *type)
{
    size_t index;

    if (type == NULL ||
        !vitte_ir_type_kind_valid(type->kind)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
    }

    if (type->element_type != VITTE_IR_INVALID_ID &&
        !vitte_ir_type_id_exists(context, type->element_type)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
    }

    if (type->return_type != VITTE_IR_INVALID_ID &&
        !vitte_ir_type_id_exists(context, type->return_type)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
    }

    if (type->name != VITTE_IR_INVALID_ID &&
        !vitte_ir_string_id_exists(context, type->name)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STRING);
    }

    if (type->parameter_count != 0u &&
        type->parameter_types == NULL) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_CORRUPTION);
    }

    for (index = 0u; index < type->parameter_count; ++index) {
        if (!vitte_ir_type_id_exists(
                context,
                type->parameter_types[index])) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_TYPE);
        }
    }

    return true;
}

static bool
vitte_ir_validate_value(
    vitte_ir_context_t *context,
    const vitte_ir_value_t *value)
{
    if (value == NULL ||
        !vitte_ir_value_kind_valid(value->kind) ||
        !vitte_ir_type_id_exists(context, value->type)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
    }

    if (value->name != VITTE_IR_INVALID_ID &&
        !vitte_ir_string_id_exists(context, value->name)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STRING);
    }

    if (value->kind == VITTE_IR_VALUE_PARAMETER &&
        !vitte_ir_function_id_exists(context, value->function)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_FUNCTION);
    }

    if (value->kind == VITTE_IR_VALUE_INSTRUCTION &&
        !vitte_ir_instruction_id_exists(context, value->instruction)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_INSTRUCTION);
    }

    if (value->kind == VITTE_IR_VALUE_CONSTANT_FLOAT &&
        !isfinite(value->float_value)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
    }

    return true;
}

static bool
vitte_ir_validate_instruction(
    vitte_ir_context_t *context,
    const vitte_ir_instruction_t *instruction)
{
    size_t index;

    if (instruction == NULL ||
        !vitte_ir_opcode_valid(instruction->opcode) ||
        !vitte_ir_block_id_exists(context, instruction->block)) {
        return vitte_ir_fail(
            context,
            VITTE_IR_ERROR_INVALID_INSTRUCTION);
    }

    if (instruction->operand_count > instruction->operand_capacity ||
        (instruction->operand_count != 0u &&
         instruction->operands == NULL)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_CORRUPTION);
    }

    for (index = 0u; index < instruction->operand_count; ++index) {
        if (!vitte_ir_value_id_exists(
                context,
                instruction->operands[index])) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
        }
    }

    if (vitte_ir_opcode_produces_value_internal(instruction->opcode)) {
        if (!vitte_ir_value_id_exists(context, instruction->result) ||
            !vitte_ir_type_id_exists(context, instruction->result_type)) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
        }
    } else if (instruction->result != VITTE_IR_INVALID_ID) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_INSTRUCTION);
    }

    if (instruction->opcode == VITTE_IR_OP_PHI) {
        if (instruction->phi_count > instruction->phi_capacity ||
            (instruction->phi_count != 0u &&
             instruction->phi_incomings == NULL)) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_PHI);
        }

        for (index = 0u; index < instruction->phi_count; ++index) {
            if (!vitte_ir_block_id_exists(
                    context,
                    instruction->phi_incomings[index].block) ||
                !vitte_ir_value_id_exists(
                    context,
                    instruction->phi_incomings[index].value)) {
                return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_PHI);
            }
        }
    }

    return true;
}

static bool
vitte_ir_validate_block(
    vitte_ir_context_t *context,
    const vitte_ir_block_t *block)
{
    size_t index;

    if (block == NULL ||
        !vitte_ir_function_id_exists(context, block->function)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_BLOCK);
    }

    if (block->instruction_count > block->instruction_capacity ||
        block->predecessor_count > block->predecessor_capacity ||
        block->successor_count > block->successor_capacity) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_CORRUPTION);
    }

    for (index = 0u; index < block->instruction_count; ++index) {
        const vitte_ir_instruction_t *instruction;

        if (!vitte_ir_instruction_id_exists(
                context,
                block->instructions[index])) {
            return vitte_ir_fail(
                context,
                VITTE_IR_ERROR_INVALID_INSTRUCTION);
        }

        instruction =
            &context->instructions[
                (size_t)(
                    block->instructions[index] -
                    UINT64_C(1))];

        if (instruction->block != block->id) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_CORRUPTION);
        }

        if (index + 1u < block->instruction_count &&
            vitte_ir_opcode_is_terminator_internal(
                instruction->opcode)) {
            return vitte_ir_fail(
                context,
                VITTE_IR_ERROR_TERMINATED_BLOCK);
        }
    }

    if (block->instruction_count != 0u) {
        const vitte_ir_instruction_id_t last =
            block->instructions[block->instruction_count - 1u];

        const vitte_ir_instruction_t *instruction =
            &context->instructions[
                (size_t)(last - UINT64_C(1))];

        if (block->terminated !=
            vitte_ir_opcode_is_terminator_internal(
                instruction->opcode)) {
            return vitte_ir_fail(
                context,
                VITTE_IR_ERROR_MISSING_TERMINATOR);
        }

        if (block->terminated &&
            block->terminator != last) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_CORRUPTION);
        }
    }

    for (index = 0u; index < block->successor_count; ++index) {
        const vitte_ir_block_t *target;
        size_t reverse;
        bool found;

        if (!vitte_ir_block_id_exists(
                context,
                block->successors[index])) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_CFG);
        }

        target =
            &context->blocks[
                (size_t)(
                    block->successors[index] -
                    UINT64_C(1))];

        if (target->function != block->function) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_CFG);
        }

        found = false;

        for (reverse = 0u;
             reverse < target->predecessor_count;
             ++reverse) {
            if (target->predecessors[reverse] == block->id) {
                found = true;
                break;
            }
        }

        if (!found) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_CFG);
        }
    }

    return true;
}

static bool
vitte_ir_validate_function(
    vitte_ir_context_t *context,
    const vitte_ir_function_t *function)
{
    size_t index;

    if (function == NULL ||
        !vitte_ir_string_id_exists(context, function->name) ||
        !vitte_ir_type_id_exists(context, function->type) ||
        !vitte_ir_linkage_valid(function->linkage) ||
        !vitte_ir_calling_convention_valid(
            function->calling_convention)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_FUNCTION);
    }

    if (function->parameter_count > function->parameter_capacity ||
        function->block_count > function->block_capacity) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_CORRUPTION);
    }

    for (index = 0u; index < function->parameter_count; ++index) {
        const vitte_ir_value_t *value;

        if (!vitte_ir_value_id_exists(
                context,
                function->parameters[index])) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_VALUE);
        }

        value =
            &context->values[
                (size_t)(
                    function->parameters[index] -
                    UINT64_C(1))];

        if (value->kind != VITTE_IR_VALUE_PARAMETER ||
            value->function != function->id ||
            value->parameter_index != index) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_CORRUPTION);
        }
    }

    if (!function->is_declaration) {
        if (function->block_count == 0u ||
            !vitte_ir_block_id_exists(
                context,
                function->entry_block)) {
            return vitte_ir_fail(
                context,
                VITTE_IR_ERROR_INVALID_FUNCTION);
        }
    }

    for (index = 0u; index < function->block_count; ++index) {
        const vitte_ir_block_t *block;

        if (!vitte_ir_block_id_exists(
                context,
                function->blocks[index])) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_BLOCK);
        }

        block =
            &context->blocks[
                (size_t)(
                    function->blocks[index] -
                    UINT64_C(1))];

        if (block->function != function->id) {
            return vitte_ir_fail(context, VITTE_IR_ERROR_CORRUPTION);
        }

        if (!function->is_declaration &&
            !block->terminated) {
            return vitte_ir_fail(
                context,
                VITTE_IR_ERROR_MISSING_TERMINATOR);
        }
    }

    return true;
}

bool
vitte_ir_validate(vitte_ir_context_t *context)
{
    size_t index;

    if (!vitte_ir_context_envelope_valid(context)) {
        return false;
    }

    context->stats.validation_runs =
        vitte_ir_u64_add_sat(
            context->stats.validation_runs,
            UINT64_C(1));

    for (index = 0u; index < context->type_count; ++index) {
        if (context->types[index].id !=
                (vitte_ir_type_id_t)(index + 1u) ||
            !vitte_ir_validate_type(
                context,
                &context->types[index])) {
            goto failure;
        }
    }

    for (index = 0u; index < context->value_count; ++index) {
        if (context->values[index].id !=
                (vitte_ir_value_id_t)(index + 1u) ||
            !vitte_ir_validate_value(
                context,
                &context->values[index])) {
            goto failure;
        }
    }

    for (index = 0u;
         index < context->instruction_count;
         ++index) {
        if (context->instructions[index].id !=
                (vitte_ir_instruction_id_t)(index + 1u) ||
            !vitte_ir_validate_instruction(
                context,
                &context->instructions[index])) {
            goto failure;
        }
    }

    for (index = 0u; index < context->block_count; ++index) {
        if (context->blocks[index].id !=
                (vitte_ir_block_id_t)(index + 1u) ||
            !vitte_ir_validate_block(
                context,
                &context->blocks[index])) {
            goto failure;
        }
    }

    for (index = 0u; index < context->function_count; ++index) {
        if (context->functions[index].id !=
                (vitte_ir_function_id_t)(index + 1u) ||
            !vitte_ir_validate_function(
                context,
                &context->functions[index])) {
            goto failure;
        }
    }

    context->last_error = VITTE_IR_ERROR_NONE;
    return true;

failure:

    context->stats.validation_failures =
        vitte_ir_u64_add_sat(
            context->stats.validation_failures,
            UINT64_C(1));

    if (context->last_error == VITTE_IR_ERROR_NONE) {
        context->last_error = VITTE_IR_ERROR_VALIDATION;
    }

    return false;
}

bool
vitte_ir_seal(vitte_ir_context_t *context)
{
    if (!vitte_ir_context_mutable(context)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STATE);
    }

    if (!vitte_ir_validate(context)) {
        context->state = VITTE_IR_STATE_FAILED;
        return false;
    }

    context->state = VITTE_IR_STATE_SEALED;
    context->last_error = VITTE_IR_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_ir_fingerprint(vitte_ir_context_t *context)
{
    uint64_t hash;
    size_t index;

    if (!vitte_ir_context_envelope_valid(context)) {
        return UINT64_C(0);
    }

    context->stats.hash_runs =
        vitte_ir_u64_add_sat(
            context->stats.hash_runs,
            UINT64_C(1));

    hash = VITTE_IR_FNV_OFFSET;

    hash = vitte_ir_hash_u64(
        hash,
        (uint64_t)VITTE_IR_API_VERSION_MAJOR);

    hash = vitte_ir_hash_u64(
        hash,
        (uint64_t)VITTE_IR_API_VERSION_MINOR);

    hash = vitte_ir_hash_u64(
        hash,
        (uint64_t)VITTE_IR_API_VERSION_PATCH);

    hash = vitte_ir_hash_size(hash, context->string_count);
    hash = vitte_ir_hash_size(hash, context->type_count);
    hash = vitte_ir_hash_size(hash, context->value_count);
    hash = vitte_ir_hash_size(hash, context->instruction_count);
    hash = vitte_ir_hash_size(hash, context->block_count);
    hash = vitte_ir_hash_size(hash, context->function_count);

    for (index = 0u; index < context->string_count; ++index) {
        const vitte_ir_string_t *string = &context->strings[index];

        hash = vitte_ir_hash_u64(hash, string->id);
        hash = vitte_ir_hash_size(hash, string->length);
        hash = vitte_ir_hash_bytes(
            hash,
            string->data,
            string->length);
    }

    for (index = 0u; index < context->type_count; ++index) {
        const vitte_ir_type_t *type = &context->types[index];
        size_t parameter;

        hash = vitte_ir_hash_u64(hash, type->id);
        hash = vitte_ir_hash_u64(hash, (uint64_t)type->kind);
        hash = vitte_ir_hash_u64(hash, type->element_type);
        hash = vitte_ir_hash_size(hash, type->array_length);
        hash = vitte_ir_hash_u64(hash, type->return_type);
        hash = vitte_ir_hash_bool(hash, type->is_variadic);
        hash = vitte_ir_hash_u64(hash, type->name);

        for (parameter = 0u;
             parameter < type->parameter_count;
             ++parameter) {
            hash = vitte_ir_hash_u64(
                hash,
                type->parameter_types[parameter]);
        }
    }

    for (index = 0u; index < context->value_count; ++index) {
        const vitte_ir_value_t *value = &context->values[index];

        hash = vitte_ir_hash_u64(hash, value->id);
        hash = vitte_ir_hash_u64(hash, (uint64_t)value->kind);
        hash = vitte_ir_hash_u64(hash, value->type);
        hash = vitte_ir_hash_u64(hash, value->name);
        hash = vitte_ir_hash_u64(hash, value->integer_value);
        hash = vitte_ir_hash_bool(hash, value->bool_value);
        hash = vitte_ir_hash_u64(hash, value->function);
        hash = vitte_ir_hash_u64(hash, value->instruction);
        hash = vitte_ir_hash_size(hash, value->parameter_index);
    }

    for (index = 0u;
         index < context->instruction_count;
         ++index) {
        const vitte_ir_instruction_t *instruction =
            &context->instructions[index];

        size_t operand;
        size_t incoming;

        hash = vitte_ir_hash_u64(hash, instruction->id);
        hash = vitte_ir_hash_u64(
            hash,
            (uint64_t)instruction->opcode);

        hash = vitte_ir_hash_u64(hash, instruction->block);
        hash = vitte_ir_hash_u64(hash, instruction->result);
        hash = vitte_ir_hash_u64(hash, instruction->result_type);

        for (operand = 0u;
             operand < instruction->operand_count;
             ++operand) {
            hash = vitte_ir_hash_u64(
                hash,
                instruction->operands[operand]);
        }

        for (incoming = 0u;
             incoming < instruction->phi_count;
             ++incoming) {
            hash = vitte_ir_hash_u64(
                hash,
                instruction->phi_incomings[incoming].block);

            hash = vitte_ir_hash_u64(
                hash,
                instruction->phi_incomings[incoming].value);
        }

        hash = vitte_ir_hash_u64(hash, instruction->target_true);
        hash = vitte_ir_hash_u64(hash, instruction->target_false);
    }

    for (index = 0u; index < context->block_count; ++index) {
        const vitte_ir_block_t *block = &context->blocks[index];
        size_t item;

        hash = vitte_ir_hash_u64(hash, block->id);
        hash = vitte_ir_hash_u64(hash, block->function);
        hash = vitte_ir_hash_u64(hash, block->name);
        hash = vitte_ir_hash_bool(hash, block->terminated);

        for (item = 0u;
             item < block->instruction_count;
             ++item) {
            hash = vitte_ir_hash_u64(
                hash,
                block->instructions[item]);
        }

        for (item = 0u;
             item < block->successor_count;
             ++item) {
            hash = vitte_ir_hash_u64(
                hash,
                block->successors[item]);
        }
    }

    for (index = 0u; index < context->function_count; ++index) {
        const vitte_ir_function_t *function =
            &context->functions[index];

        size_t item;

        hash = vitte_ir_hash_u64(hash, function->id);
        hash = vitte_ir_hash_u64(hash, function->name);
        hash = vitte_ir_hash_u64(hash, function->type);
        hash = vitte_ir_hash_u64(
            hash,
            (uint64_t)function->linkage);

        hash = vitte_ir_hash_u64(
            hash,
            (uint64_t)function->calling_convention);

        hash = vitte_ir_hash_bool(hash, function->is_declaration);
        hash = vitte_ir_hash_bool(hash, function->is_variadic);
        hash = vitte_ir_hash_u64(hash, function->entry_block);

        for (item = 0u;
             item < function->parameter_count;
             ++item) {
            hash = vitte_ir_hash_u64(
                hash,
                function->parameters[item]);
        }

        for (item = 0u;
             item < function->block_count;
             ++item) {
            hash = vitte_ir_hash_u64(
                hash,
                function->blocks[item]);
        }
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_ir_stats_t
vitte_ir_stats(const vitte_ir_context_t *context)
{
    vitte_ir_stats_t result;

    memset(&result, 0, sizeof(result));

    if (!vitte_ir_context_envelope_valid(context)) {
        return result;
    }

    return context->stats;
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_ir_set_limits(
    vitte_ir_context_t *context,
    size_t max_types,
    size_t max_values,
    size_t max_instructions,
    size_t max_blocks,
    size_t max_functions,
    size_t max_string_bytes)
{
    if (!vitte_ir_context_mutable(context)) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_STATE);
    }

    if (max_types == 0u ||
        max_values == 0u ||
        max_instructions == 0u ||
        max_blocks == 0u ||
        max_functions == 0u ||
        max_string_bytes == 0u) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_INVALID_ARGUMENT);
    }

    if (max_types < context->type_count ||
        max_values < context->value_count ||
        max_instructions < context->instruction_count ||
        max_blocks < context->block_count ||
        max_functions < context->function_count ||
        max_string_bytes < context->string_bytes) {
        return vitte_ir_fail(context, VITTE_IR_ERROR_LIMIT);
    }

    context->max_types = max_types;
    context->max_values = max_values;
    context->max_instructions = max_instructions;
    context->max_blocks = max_blocks;
    context->max_functions = max_functions;
    context->max_string_bytes = max_string_bytes;

    context->last_error = VITTE_IR_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Storage release                                                           */
/* ========================================================================= */

static void
vitte_ir_release_instruction(vitte_ir_instruction_t *instruction)
{
    if (instruction == NULL) {
        return;
    }

    free(instruction->operands);
    free(instruction->phi_incomings);

    memset(instruction, 0, sizeof(*instruction));
}

static void
vitte_ir_release_block(vitte_ir_block_t *block)
{
    if (block == NULL) {
        return;
    }

    free(block->instructions);
    free(block->predecessors);
    free(block->successors);

    memset(block, 0, sizeof(*block));
}

static void
vitte_ir_release_function(vitte_ir_function_t *function)
{
    if (function == NULL) {
        return;
    }

    free(function->parameters);
    free(function->blocks);

    memset(function, 0, sizeof(*function));
}

static void
vitte_ir_release_type(vitte_ir_type_t *type)
{
    if (type == NULL) {
        return;
    }

    free(type->parameter_types);

    memset(type, 0, sizeof(*type));
}

static void
vitte_ir_release_string(vitte_ir_string_t *string)
{
    if (string == NULL) {
        return;
    }

    free(string->data);

    memset(string, 0, sizeof(*string));
}

static void
vitte_ir_release_storage(vitte_ir_context_t *context)
{
    size_t index;

    if (context == NULL) {
        return;
    }

    for (index = 0u; index < context->instruction_count; ++index) {
        vitte_ir_release_instruction(&context->instructions[index]);
    }

    for (index = 0u; index < context->block_count; ++index) {
        vitte_ir_release_block(&context->blocks[index]);
    }

    for (index = 0u; index < context->function_count; ++index) {
        vitte_ir_release_function(&context->functions[index]);
    }

    for (index = 0u; index < context->type_count; ++index) {
        vitte_ir_release_type(&context->types[index]);
    }

    for (index = 0u; index < context->string_count; ++index) {
        vitte_ir_release_string(&context->strings[index]);
    }

    free(context->instructions);
    free(context->blocks);
    free(context->functions);
    free(context->types);
    free(context->values);
    free(context->strings);

    context->instructions = NULL;
    context->blocks = NULL;
    context->functions = NULL;
    context->types = NULL;
    context->values = NULL;
    context->strings = NULL;

    context->instruction_count = 0u;
    context->instruction_capacity = 0u;

    context->block_count = 0u;
    context->block_capacity = 0u;

    context->function_count = 0u;
    context->function_capacity = 0u;

    context->type_count = 0u;
    context->type_capacity = 0u;

    context->value_count = 0u;
    context->value_capacity = 0u;

    context->string_count = 0u;
    context->string_capacity = 0u;
    context->string_bytes = 0u;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_ir_init(vitte_ir_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    memset(context, 0, sizeof(*context));

    context->magic = VITTE_IR_MAGIC;
    context->state = VITTE_IR_STATE_BUILDING;
    context->last_error = VITTE_IR_ERROR_NONE;

    context->max_types = VITTE_IR_DEFAULT_MAX_TYPES;
    context->max_values = VITTE_IR_DEFAULT_MAX_VALUES;
    context->max_instructions = VITTE_IR_DEFAULT_MAX_INSTRUCTIONS;
    context->max_blocks = VITTE_IR_DEFAULT_MAX_BLOCKS;
    context->max_functions = VITTE_IR_DEFAULT_MAX_FUNCTIONS;
    context->max_string_bytes = VITTE_IR_DEFAULT_MAX_STRING_BYTES;

    context->generation = UINT64_C(1);

    return true;
}

bool
vitte_ir_reset(vitte_ir_context_t *context)
{
    size_t max_types;
    size_t max_values;
    size_t max_instructions;
    size_t max_blocks;
    size_t max_functions;
    size_t max_string_bytes;
    uint64_t generation;

    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return false;
    }

    max_types = context->max_types;
    max_values = context->max_values;
    max_instructions = context->max_instructions;
    max_blocks = context->max_blocks;
    max_functions = context->max_functions;
    max_string_bytes = context->max_string_bytes;
    generation = context->generation;

    vitte_ir_release_storage(context);

    memset(&context->stats, 0, sizeof(context->stats));

    context->state = VITTE_IR_STATE_BUILDING;
    context->last_error = VITTE_IR_ERROR_NONE;

    context->max_types = max_types;
    context->max_values = max_values;
    context->max_instructions = max_instructions;
    context->max_blocks = max_blocks;
    context->max_functions = max_functions;
    context->max_string_bytes = max_string_bytes;

    context->generation =
        generation == UINT64_MAX
            ? UINT64_MAX
            : generation + UINT64_C(1);

    return true;
}

void
vitte_ir_destroy(vitte_ir_context_t *context)
{
    if (context == NULL) {
        return;
    }

    if (context->magic == VITTE_IR_MAGIC) {
        vitte_ir_release_storage(context);
    }

    memset(context, 0, sizeof(*context));

    context->magic = VITTE_IR_DEAD_MAGIC;
    context->state = VITTE_IR_STATE_DESTROYED;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_ir_translation_unit_anchor(void)
{
    /*
     * Deliberately empty.
     *
     * Useful when forcing this translation unit into a static archive and
     * for build-system/linkage diagnostics.
     */
}
