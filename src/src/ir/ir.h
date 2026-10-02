#ifndef VITTE_IR_IR_H
#define VITTE_IR_IR_H

/*
 * Vitte Compiler
 * src/ir/ir.h
 *
 * Core Intermediate Representation
 * ================================
 *
 * Public contract for Vitte's typed compiler IR.
 *
 * Design:
 *
 *   HIR
 *    |
 *    v
 *   IR
 *    |
 *    +---- verification
 *    +---- CFG analysis
 *    +---- optimization
 *    +---- constant propagation
 *    +---- dead-code elimination
 *    +---- backend lowering
 *    |
 *    v
 *   C17 / native backend
 *
 * Properties:
 *
 *   - typed values;
 *   - SSA-compatible instruction results;
 *   - explicit functions;
 *   - explicit basic blocks;
 *   - explicit CFG;
 *   - predecessor/successor lists;
 *   - phi nodes;
 *   - explicit terminators;
 *   - integer/floating-point operations;
 *   - comparisons;
 *   - casts;
 *   - memory operations;
 *   - calls;
 *   - deterministic IDs;
 *   - deterministic fingerprints;
 *   - structural validation;
 *   - resource limits;
 *   - source provenance;
 *   - statistics.
 *
 * IDs are 1-based.
 *
 *     0 == invalid / absent
 *
 * The context owns all memory referenced by its IR objects.
 *
 * Objects remain valid until reset() or destroy().
 *
 * A successful seal() makes the IR immutable through the public mutation API.
 *
 * ISO C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

#define VITTE_IR_API_VERSION_MAJOR 1u
#define VITTE_IR_API_VERSION_MINOR 0u
#define VITTE_IR_API_VERSION_PATCH 0u

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_IR_MAGIC \
    UINT64_C(0x5649545445495221)

#define VITTE_IR_DEAD_MAGIC \
    UINT64_C(0x4445414449522121)

#define VITTE_IR_INVALID_ID \
    UINT64_C(0)

#define VITTE_IR_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_IR_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_IR_DEFAULT_INITIAL_CAPACITY \
    ((size_t)16u)

#define VITTE_IR_DEFAULT_MAX_TYPES \
    ((size_t)1048576u)

#define VITTE_IR_DEFAULT_MAX_VALUES \
    ((size_t)16777216u)

#define VITTE_IR_DEFAULT_MAX_INSTRUCTIONS \
    ((size_t)16777216u)

#define VITTE_IR_DEFAULT_MAX_BLOCKS \
    ((size_t)4194304u)

#define VITTE_IR_DEFAULT_MAX_FUNCTIONS \
    ((size_t)1048576u)

#define VITTE_IR_DEFAULT_MAX_STRING_BYTES \
    ((size_t)(256u * 1024u * 1024u))

/* ========================================================================= */
/* IDs                                                                       */
/* ========================================================================= */

typedef uint64_t vitte_ir_id_t;

typedef uint64_t vitte_ir_string_id_t;
typedef uint64_t vitte_ir_type_id_t;
typedef uint64_t vitte_ir_value_id_t;
typedef uint64_t vitte_ir_instruction_id_t;
typedef uint64_t vitte_ir_block_id_t;
typedef uint64_t vitte_ir_function_id_t;

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_ir_error {
    VITTE_IR_ERROR_NONE = 0,

    VITTE_IR_ERROR_INVALID_ARGUMENT,
    VITTE_IR_ERROR_INVALID_CONTEXT,
    VITTE_IR_ERROR_INVALID_STATE,

    VITTE_IR_ERROR_INVALID_STRING,
    VITTE_IR_ERROR_INVALID_TYPE,
    VITTE_IR_ERROR_INVALID_VALUE,
    VITTE_IR_ERROR_INVALID_INSTRUCTION,
    VITTE_IR_ERROR_INVALID_BLOCK,
    VITTE_IR_ERROR_INVALID_FUNCTION,

    VITTE_IR_ERROR_TYPE_MISMATCH,
    VITTE_IR_ERROR_DUPLICATE,

    VITTE_IR_ERROR_TERMINATED_BLOCK,
    VITTE_IR_ERROR_MISSING_TERMINATOR,

    VITTE_IR_ERROR_INVALID_CFG,
    VITTE_IR_ERROR_INVALID_PHI,

    VITTE_IR_ERROR_LIMIT,
    VITTE_IR_ERROR_OVERFLOW,
    VITTE_IR_ERROR_OUT_OF_MEMORY,

    VITTE_IR_ERROR_VALIDATION,
    VITTE_IR_ERROR_CORRUPTION,

    VITTE_IR_ERROR_UNSUPPORTED,
    VITTE_IR_ERROR_INTERNAL,

    VITTE_IR_ERROR_COUNT
} vitte_ir_error_t;

/* ========================================================================= */
/* Context state                                                             */
/* ========================================================================= */

typedef enum vitte_ir_state {
    VITTE_IR_STATE_INVALID = 0,

    /*
     * IR objects may be created and modified.
     */
    VITTE_IR_STATE_BUILDING,

    /*
     * Validation succeeded and mutation through the API is disabled.
     */
    VITTE_IR_STATE_SEALED,

    /*
     * A sealing/verification operation failed.
     */
    VITTE_IR_STATE_FAILED,

    /*
     * Storage has been released.
     */
    VITTE_IR_STATE_DESTROYED,

    VITTE_IR_STATE_COUNT
} vitte_ir_state_t;

/* ========================================================================= */
/* Type kinds                                                                */
/* ========================================================================= */

typedef enum vitte_ir_type_kind {
    VITTE_IR_TYPE_INVALID = 0,

    VITTE_IR_TYPE_VOID,

    VITTE_IR_TYPE_BOOL,

    VITTE_IR_TYPE_I8,
    VITTE_IR_TYPE_I16,
    VITTE_IR_TYPE_I32,
    VITTE_IR_TYPE_I64,

    VITTE_IR_TYPE_U8,
    VITTE_IR_TYPE_U16,
    VITTE_IR_TYPE_U32,
    VITTE_IR_TYPE_U64,

    VITTE_IR_TYPE_F32,
    VITTE_IR_TYPE_F64,

    VITTE_IR_TYPE_CHAR,

    /*
     * element_type = pointee.
     */
    VITTE_IR_TYPE_POINTER,

    /*
     * element_type + array_length.
     */
    VITTE_IR_TYPE_ARRAY,

    /*
     * parameter_types + return_type + is_variadic.
     */
    VITTE_IR_TYPE_FUNCTION,

    /*
     * Named aggregate.
     *
     * API v1 keeps aggregate lowering intentionally compact.
     */
    VITTE_IR_TYPE_STRUCT,

    /*
     * Backend-visible but structurally unspecified type.
     */
    VITTE_IR_TYPE_OPAQUE,

    VITTE_IR_TYPE_COUNT
} vitte_ir_type_kind_t;

/* ========================================================================= */
/* Value kinds                                                               */
/* ========================================================================= */

typedef enum vitte_ir_value_kind {
    VITTE_IR_VALUE_INVALID = 0,

    VITTE_IR_VALUE_UNDEF,
    VITTE_IR_VALUE_POISON,
    VITTE_IR_VALUE_NULL,

    VITTE_IR_VALUE_CONSTANT_BOOL,
    VITTE_IR_VALUE_CONSTANT_INTEGER,
    VITTE_IR_VALUE_CONSTANT_FLOAT,

    VITTE_IR_VALUE_PARAMETER,

    /*
     * SSA result produced by an instruction.
     */
    VITTE_IR_VALUE_INSTRUCTION,

    /*
     * Reserved representation for function values.
     */
    VITTE_IR_VALUE_FUNCTION,

    VITTE_IR_VALUE_COUNT
} vitte_ir_value_kind_t;

/* ========================================================================= */
/* Linkage                                                                   */
/* ========================================================================= */

typedef enum vitte_ir_linkage {
    VITTE_IR_LINKAGE_INTERNAL = 0,

    VITTE_IR_LINKAGE_EXTERNAL,
    VITTE_IR_LINKAGE_WEAK,
    VITTE_IR_LINKAGE_PRIVATE,

    VITTE_IR_LINKAGE_COUNT
} vitte_ir_linkage_t;

/* ========================================================================= */
/* Calling conventions                                                       */
/* ========================================================================= */

typedef enum vitte_ir_calling_convention {
    VITTE_IR_CALL_DEFAULT = 0,

    VITTE_IR_CALL_C,
    VITTE_IR_CALL_SYSV64,
    VITTE_IR_CALL_WIN64,

    VITTE_IR_CALL_COUNT
} vitte_ir_calling_convention_t;

/* ========================================================================= */
/* Opcodes                                                                   */
/* ========================================================================= */

typedef enum vitte_ir_opcode {
    VITTE_IR_OP_INVALID = 0,

    /* --------------------------------------------------------------------- */
    /* Structural                                                            */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_NOP,

    /* --------------------------------------------------------------------- */
    /* Integer arithmetic                                                    */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_ADD,
    VITTE_IR_OP_SUB,
    VITTE_IR_OP_MUL,

    VITTE_IR_OP_SDIV,
    VITTE_IR_OP_UDIV,

    VITTE_IR_OP_SREM,
    VITTE_IR_OP_UREM,

    /* --------------------------------------------------------------------- */
    /* Floating-point arithmetic                                             */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_FADD,
    VITTE_IR_OP_FSUB,
    VITTE_IR_OP_FMUL,
    VITTE_IR_OP_FDIV,
    VITTE_IR_OP_FREM,

    /* --------------------------------------------------------------------- */
    /* Unary                                                                 */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_NEG,
    VITTE_IR_OP_FNEG,

    /* --------------------------------------------------------------------- */
    /* Bitwise                                                               */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_AND,
    VITTE_IR_OP_OR,
    VITTE_IR_OP_XOR,
    VITTE_IR_OP_NOT,

    VITTE_IR_OP_SHL,
    VITTE_IR_OP_LSHR,
    VITTE_IR_OP_ASHR,

    /* --------------------------------------------------------------------- */
    /* Integer comparison                                                    */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_ICMP_EQ,
    VITTE_IR_OP_ICMP_NE,

    VITTE_IR_OP_ICMP_SLT,
    VITTE_IR_OP_ICMP_SLE,
    VITTE_IR_OP_ICMP_SGT,
    VITTE_IR_OP_ICMP_SGE,

    VITTE_IR_OP_ICMP_ULT,
    VITTE_IR_OP_ICMP_ULE,
    VITTE_IR_OP_ICMP_UGT,
    VITTE_IR_OP_ICMP_UGE,

    /* --------------------------------------------------------------------- */
    /* Floating-point comparison                                             */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_FCMP_EQ,
    VITTE_IR_OP_FCMP_NE,
    VITTE_IR_OP_FCMP_LT,
    VITTE_IR_OP_FCMP_LE,
    VITTE_IR_OP_FCMP_GT,
    VITTE_IR_OP_FCMP_GE,

    /* --------------------------------------------------------------------- */
    /* Conversions                                                           */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_TRUNC,
    VITTE_IR_OP_ZEXT,
    VITTE_IR_OP_SEXT,

    VITTE_IR_OP_FP_TRUNC,
    VITTE_IR_OP_FP_EXT,

    VITTE_IR_OP_SI_TO_FP,
    VITTE_IR_OP_UI_TO_FP,

    VITTE_IR_OP_FP_TO_SI,
    VITTE_IR_OP_FP_TO_UI,

    VITTE_IR_OP_PTR_TO_INT,
    VITTE_IR_OP_INT_TO_PTR,

    VITTE_IR_OP_BITCAST,

    /* --------------------------------------------------------------------- */
    /* Memory                                                                */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_ALLOCA,
    VITTE_IR_OP_LOAD,
    VITTE_IR_OP_STORE,

    /*
     * Address calculation / get-element-pointer.
     */
    VITTE_IR_OP_GEP,

    /* --------------------------------------------------------------------- */
    /* SSA                                                                   */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_PHI,
    VITTE_IR_OP_SELECT,

    /* --------------------------------------------------------------------- */
    /* Calls                                                                 */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_CALL,

    /* --------------------------------------------------------------------- */
    /* Control flow                                                          */
    /* --------------------------------------------------------------------- */

    VITTE_IR_OP_BR,
    VITTE_IR_OP_COND_BR,
    VITTE_IR_OP_SWITCH,

    VITTE_IR_OP_RETURN,
    VITTE_IR_OP_UNREACHABLE,

    VITTE_IR_OP_COUNT
} vitte_ir_opcode_t;

/* ========================================================================= */
/* Source span                                                               */
/* ========================================================================= */

/*
 * Half-open byte range:
 *
 *     [begin, end)
 *
 * file_id is owned/interpreted by Vitte's source manager.
 */
typedef struct vitte_ir_span {
    uint32_t file_id;

    size_t begin;
    size_t end;

    bool valid;
} vitte_ir_span_t;

/* ========================================================================= */
/* Interned string                                                           */
/* ========================================================================= */

typedef struct vitte_ir_string {
    vitte_ir_string_id_t id;

    /*
     * Owned and NUL-terminated.
     *
     * length excludes the trailing NUL.
     */
    char *data;

    size_t length;

    uint64_t hash;
} vitte_ir_string_t;

/* ========================================================================= */
/* Type descriptor                                                           */
/* ========================================================================= */

/*
 * Temporary input descriptor for vitte_ir_intern_type().
 *
 * parameter_types is borrowed only for the duration of the call.
 */
typedef struct vitte_ir_type_desc {
    vitte_ir_type_kind_t kind;

    /*
     * Pointer/array element type.
     */
    vitte_ir_type_id_t element_type;

    /*
     * Array length.
     */
    size_t array_length;

    /*
     * Function parameters.
     */
    const vitte_ir_type_id_t *parameter_types;
    size_t parameter_count;

    /*
     * Function return type.
     */
    vitte_ir_type_id_t return_type;

    bool is_variadic;

    /*
     * Optional name for named types.
     */
    vitte_ir_string_id_t name;
} vitte_ir_type_desc_t;

/* ========================================================================= */
/* Type                                                                      */
/* ========================================================================= */

typedef struct vitte_ir_type {
    vitte_ir_type_id_t id;

    vitte_ir_type_kind_t kind;

    vitte_ir_type_id_t element_type;

    size_t array_length;

    /*
     * Owned by the IR context.
     */
    vitte_ir_type_id_t *parameter_types;
    size_t parameter_count;

    vitte_ir_type_id_t return_type;

    bool is_variadic;

    vitte_ir_string_id_t name;
} vitte_ir_type_t;

/* ========================================================================= */
/* Value descriptor                                                          */
/* ========================================================================= */

typedef struct vitte_ir_value_desc {
    vitte_ir_value_kind_t kind;

    vitte_ir_type_id_t type;

    vitte_ir_string_id_t name;

    vitte_ir_span_t span;

    uint64_t integer_value;
    double float_value;
    bool bool_value;

    /*
     * Parameter/function ownership.
     */
    vitte_ir_function_id_t function;

    /*
     * Producer when kind == INSTRUCTION.
     */
    vitte_ir_instruction_id_t instruction;

    /*
     * Parameter ordinal.
     */
    size_t parameter_index;
} vitte_ir_value_desc_t;

/* ========================================================================= */
/* Value                                                                     */
/* ========================================================================= */

typedef struct vitte_ir_value {
    vitte_ir_value_id_t id;

    vitte_ir_value_kind_t kind;

    vitte_ir_type_id_t type;

    vitte_ir_string_id_t name;

    vitte_ir_span_t span;

    uint64_t integer_value;
    double float_value;
    bool bool_value;

    vitte_ir_function_id_t function;
    vitte_ir_instruction_id_t instruction;

    size_t parameter_index;

    /*
     * Number of recorded IR uses.
     *
     * Saturates at UINT64_MAX.
     */
    uint64_t use_count;
} vitte_ir_value_t;

/* ========================================================================= */
/* Phi incoming edge                                                         */
/* ========================================================================= */

typedef struct vitte_ir_phi_incoming {
    vitte_ir_block_id_t block;
    vitte_ir_value_id_t value;
} vitte_ir_phi_incoming_t;

/* ========================================================================= */
/* Instruction descriptor                                                    */
/* ========================================================================= */

/*
 * Input descriptor for vitte_ir_add_instruction().
 *
 * operands is borrowed during the call and copied into context-owned storage.
 */
typedef struct vitte_ir_instruction_desc {
    vitte_ir_opcode_t opcode;

    vitte_ir_block_id_t block;

    /*
     * Zero for instructions that do not produce a value.
     */
    vitte_ir_type_id_t result_type;

    /*
     * Optional SSA/debug name for the result value.
     */
    vitte_ir_string_id_t result_name;

    const vitte_ir_value_id_t *operands;
    size_t operand_count;

    vitte_ir_span_t span;
} vitte_ir_instruction_desc_t;

/* ========================================================================= */
/* Instruction                                                               */
/* ========================================================================= */

typedef struct vitte_ir_instruction {
    vitte_ir_instruction_id_t id;

    vitte_ir_opcode_t opcode;

    vitte_ir_block_id_t block;

    /*
     * SSA result.
     *
     * Zero for non-value-producing instructions.
     */
    vitte_ir_value_id_t result;
    vitte_ir_type_id_t result_type;

    /*
     * Generic operands.
     */
    vitte_ir_value_id_t *operands;
    size_t operand_count;
    size_t operand_capacity;

    /*
     * PHI-specific incoming edges.
     */
    vitte_ir_phi_incoming_t *phi_incomings;
    size_t phi_count;
    size_t phi_capacity;

    /*
     * Branch targets.
     *
     * BR:
     *     target_true
     *
     * COND_BR:
     *     target_true
     *     target_false
     */
    vitte_ir_block_id_t target_true;
    vitte_ir_block_id_t target_false;

    vitte_ir_span_t span;
} vitte_ir_instruction_t;

/* ========================================================================= */
/* Basic block                                                               */
/* ========================================================================= */

typedef struct vitte_ir_block {
    vitte_ir_block_id_t id;

    vitte_ir_function_id_t function;

    vitte_ir_string_id_t name;

    vitte_ir_span_t span;

    /*
     * Instruction order is semantically significant.
     */
    vitte_ir_instruction_id_t *instructions;
    size_t instruction_count;
    size_t instruction_capacity;

    /*
     * Explicit CFG.
     */
    vitte_ir_block_id_t *predecessors;
    size_t predecessor_count;
    size_t predecessor_capacity;

    vitte_ir_block_id_t *successors;
    size_t successor_count;
    size_t successor_capacity;

    bool terminated;

    /*
     * Last instruction when terminated == true.
     */
    vitte_ir_instruction_id_t terminator;
} vitte_ir_block_t;

/* ========================================================================= */
/* Function descriptor                                                       */
/* ========================================================================= */

typedef struct vitte_ir_function_desc {
    vitte_ir_string_id_t name;

    /*
     * Must refer to a FUNCTION type.
     */
    vitte_ir_type_id_t type;

    vitte_ir_linkage_t linkage;

    vitte_ir_calling_convention_t calling_convention;

    bool is_declaration;
    bool is_variadic;

    vitte_ir_span_t span;
} vitte_ir_function_desc_t;

/* ========================================================================= */
/* Function                                                                  */
/* ========================================================================= */

typedef struct vitte_ir_function {
    vitte_ir_function_id_t id;

    vitte_ir_string_id_t name;

    vitte_ir_type_id_t type;

    vitte_ir_linkage_t linkage;

    vitte_ir_calling_convention_t calling_convention;

    bool is_declaration;
    bool is_variadic;

    vitte_ir_span_t span;

    /*
     * Parameter SSA values in declaration order.
     */
    vitte_ir_value_id_t *parameters;
    size_t parameter_count;
    size_t parameter_capacity;

    /*
     * Function-owned blocks in creation order.
     */
    vitte_ir_block_id_t *blocks;
    size_t block_count;
    size_t block_capacity;

    vitte_ir_block_id_t entry_block;
} vitte_ir_function_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_ir_stats {
    uint64_t strings_interned;

    uint64_t types_created;
    uint64_t values_created;
    uint64_t instructions_created;
    uint64_t blocks_created;
    uint64_t functions_created;

    uint64_t cfg_edges;
    uint64_t phi_incomings;

    uint64_t validation_runs;
    uint64_t validation_failures;

    uint64_t hash_runs;

    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;
} vitte_ir_stats_t;

/* ========================================================================= */
/* IR context                                                                */
/* ========================================================================= */

/*
 * Public API-v1 representation.
 *
 * The representation is visible so the compiler may stack-allocate contexts
 * and inspect them in low-level debugging tools.
 *
 * Normal compiler passes should prefer the public accessor API.
 */
typedef struct vitte_ir_context {
    uint64_t magic;

    vitte_ir_state_t state;
    vitte_ir_error_t last_error;

    /* --------------------------------------------------------------------- */
    /* Strings                                                               */
    /* --------------------------------------------------------------------- */

    vitte_ir_string_t *strings;

    size_t string_count;
    size_t string_capacity;
    size_t string_bytes;

    /* --------------------------------------------------------------------- */
    /* Types                                                                 */
    /* --------------------------------------------------------------------- */

    vitte_ir_type_t *types;

    size_t type_count;
    size_t type_capacity;

    /* --------------------------------------------------------------------- */
    /* Values                                                                */
    /* --------------------------------------------------------------------- */

    vitte_ir_value_t *values;

    size_t value_count;
    size_t value_capacity;

    /* --------------------------------------------------------------------- */
    /* Instructions                                                          */
    /* --------------------------------------------------------------------- */

    vitte_ir_instruction_t *instructions;

    size_t instruction_count;
    size_t instruction_capacity;

    /* --------------------------------------------------------------------- */
    /* Blocks                                                                */
    /* --------------------------------------------------------------------- */

    vitte_ir_block_t *blocks;

    size_t block_count;
    size_t block_capacity;

    /* --------------------------------------------------------------------- */
    /* Functions                                                             */
    /* --------------------------------------------------------------------- */

    vitte_ir_function_t *functions;

    size_t function_count;
    size_t function_capacity;

    /* --------------------------------------------------------------------- */
    /* Limits                                                                */
    /* --------------------------------------------------------------------- */

    size_t max_types;
    size_t max_values;
    size_t max_instructions;
    size_t max_blocks;
    size_t max_functions;
    size_t max_string_bytes;

    /* --------------------------------------------------------------------- */
    /* Statistics                                                            */
    /* --------------------------------------------------------------------- */

    vitte_ir_stats_t stats;

    /*
     * Incremented by reset().
     *
     * IDs from an older generation must be considered stale.
     */
    uint64_t generation;
} vitte_ir_context_t;

/* ========================================================================= */
/* Enum names                                                                */
/* ========================================================================= */

const char *
vitte_ir_error_name(
    vitte_ir_error_t error);

const char *
vitte_ir_state_name(
    vitte_ir_state_t state);

const char *
vitte_ir_type_kind_name(
    vitte_ir_type_kind_t kind);

const char *
vitte_ir_value_kind_name(
    vitte_ir_value_kind_t kind);

const char *
vitte_ir_opcode_name(
    vitte_ir_opcode_t opcode);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_ir_init(
    vitte_ir_context_t *context);

bool
vitte_ir_reset(
    vitte_ir_context_t *context);

void
vitte_ir_destroy(
    vitte_ir_context_t *context);

bool
vitte_ir_is_valid(
    const vitte_ir_context_t *context);

/* ========================================================================= */
/* Strings                                                                   */
/* ========================================================================= */

vitte_ir_string_id_t
vitte_ir_intern_string(
    vitte_ir_context_t *context,
    const char *data,
    size_t length);

const vitte_ir_string_t *
vitte_ir_get_string(
    const vitte_ir_context_t *context,
    vitte_ir_string_id_t id);

/* ========================================================================= */
/* Types                                                                     */
/* ========================================================================= */

/*
 * Structurally intern a type.
 *
 * Equivalent type descriptors produce the same ID.
 *
 * Dynamic descriptor arrays are copied.
 */
vitte_ir_type_id_t
vitte_ir_intern_type(
    vitte_ir_context_t *context,
    const vitte_ir_type_desc_t *description);

const vitte_ir_type_t *
vitte_ir_get_type(
    const vitte_ir_context_t *context,
    vitte_ir_type_id_t id);

/* ========================================================================= */
/* Values                                                                    */
/* ========================================================================= */

vitte_ir_value_id_t
vitte_ir_add_value(
    vitte_ir_context_t *context,
    const vitte_ir_value_desc_t *description);

vitte_ir_value_id_t
vitte_ir_const_bool(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type,
    bool value);

vitte_ir_value_id_t
vitte_ir_const_integer(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type,
    uint64_t value);

vitte_ir_value_id_t
vitte_ir_const_float(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type,
    double value);

vitte_ir_value_id_t
vitte_ir_null(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type);

vitte_ir_value_id_t
vitte_ir_undef(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type);

vitte_ir_value_id_t
vitte_ir_poison(
    vitte_ir_context_t *context,
    vitte_ir_type_id_t type);

const vitte_ir_value_t *
vitte_ir_get_value(
    const vitte_ir_context_t *context,
    vitte_ir_value_id_t id);

/* ========================================================================= */
/* Functions                                                                 */
/* ========================================================================= */

vitte_ir_function_id_t
vitte_ir_add_function(
    vitte_ir_context_t *context,
    const vitte_ir_function_desc_t *description);

const vitte_ir_function_t *
vitte_ir_get_function(
    const vitte_ir_context_t *context,
    vitte_ir_function_id_t id);

vitte_ir_function_t *
vitte_ir_get_function_mut(
    vitte_ir_context_t *context,
    vitte_ir_function_id_t id);

vitte_ir_value_id_t
vitte_ir_add_parameter(
    vitte_ir_context_t *context,
    vitte_ir_function_id_t function,
    vitte_ir_type_id_t type,
    vitte_ir_string_id_t name,
    vitte_ir_span_t span);

/* ========================================================================= */
/* Basic blocks                                                              */
/* ========================================================================= */

vitte_ir_block_id_t
vitte_ir_add_block(
    vitte_ir_context_t *context,
    vitte_ir_function_id_t function,
    vitte_ir_string_id_t name,
    vitte_ir_span_t span);

const vitte_ir_block_t *
vitte_ir_get_block(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id);

vitte_ir_block_t *
vitte_ir_get_block_mut(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t id);

/* ========================================================================= */
/* CFG                                                                       */
/* ========================================================================= */

bool
vitte_ir_add_cfg_edge(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t source,
    vitte_ir_block_id_t target);

/* ========================================================================= */
/* Instructions                                                              */
/* ========================================================================= */

vitte_ir_instruction_id_t
vitte_ir_add_instruction(
    vitte_ir_context_t *context,
    const vitte_ir_instruction_desc_t *description);

const vitte_ir_instruction_t *
vitte_ir_get_instruction(
    const vitte_ir_context_t *context,
    vitte_ir_instruction_id_t id);

vitte_ir_instruction_t *
vitte_ir_get_instruction_mut(
    vitte_ir_context_t *context,
    vitte_ir_instruction_id_t id);

vitte_ir_value_id_t
vitte_ir_instruction_result(
    const vitte_ir_context_t *context,
    vitte_ir_instruction_id_t id);

bool
vitte_ir_opcode_is_terminator(
    vitte_ir_opcode_t opcode);

bool
vitte_ir_opcode_produces_value(
    vitte_ir_opcode_t opcode);

/* ========================================================================= */
/* PHI                                                                       */
/* ========================================================================= */

bool
vitte_ir_phi_add_incoming(
    vitte_ir_context_t *context,
    vitte_ir_instruction_id_t instruction,
    vitte_ir_block_id_t block,
    vitte_ir_value_id_t value);

/* ========================================================================= */
/* Instruction builders                                                      */
/* ========================================================================= */

vitte_ir_instruction_id_t
vitte_ir_build_unary(
    vitte_ir_context_t *context,
    vitte_ir_opcode_t opcode,
    vitte_ir_block_id_t block,
    vitte_ir_type_id_t result_type,
    vitte_ir_value_id_t operand,
    vitte_ir_span_t span);

vitte_ir_instruction_id_t
vitte_ir_build_binary(
    vitte_ir_context_t *context,
    vitte_ir_opcode_t opcode,
    vitte_ir_block_id_t block,
    vitte_ir_type_id_t result_type,
    vitte_ir_value_id_t left,
    vitte_ir_value_id_t right,
    vitte_ir_span_t span);

vitte_ir_instruction_id_t
vitte_ir_build_br(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t block,
    vitte_ir_block_id_t target,
    vitte_ir_span_t span);

vitte_ir_instruction_id_t
vitte_ir_build_cond_br(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t block,
    vitte_ir_value_id_t condition,
    vitte_ir_block_id_t true_target,
    vitte_ir_block_id_t false_target,
    vitte_ir_span_t span);

vitte_ir_instruction_id_t
vitte_ir_build_return(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t block,
    vitte_ir_value_id_t value,
    vitte_ir_span_t span);

vitte_ir_instruction_id_t
vitte_ir_build_unreachable(
    vitte_ir_context_t *context,
    vitte_ir_block_id_t block,
    vitte_ir_span_t span);

/* ========================================================================= */
/* Validation / sealing                                                      */
/* ========================================================================= */

/*
 * Validate all cross references and structural invariants.
 *
 * Validation itself does not seal the context.
 */
bool
vitte_ir_validate(
    vitte_ir_context_t *context);

/*
 * Validate and transition BUILDING -> SEALED.
 *
 * On validation failure the context transitions to FAILED.
 */
bool
vitte_ir_seal(
    vitte_ir_context_t *context);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Deterministic structural fingerprint.
 *
 * Intended for:
 *
 *   - regression tests;
 *   - incremental compilation;
 *   - cache bookkeeping;
 *   - reproducibility checks.
 *
 * This is not a cryptographic hash.
 */
uint64_t
vitte_ir_fingerprint(
    vitte_ir_context_t *context);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_ir_stats_t
vitte_ir_stats(
    const vitte_ir_context_t *context);

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
    size_t max_string_bytes);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_ir_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline ID helpers                                                         */
/* ========================================================================= */

static inline bool
vitte_ir_id_is_valid(vitte_ir_id_t id)
{
    return id != VITTE_IR_INVALID_ID;
}

static inline bool
vitte_ir_string_id_is_valid(vitte_ir_string_id_t id)
{
    return id != VITTE_IR_INVALID_ID;
}

static inline bool
vitte_ir_type_id_is_valid(vitte_ir_type_id_t id)
{
    return id != VITTE_IR_INVALID_ID;
}

static inline bool
vitte_ir_value_id_is_valid(vitte_ir_value_id_t id)
{
    return id != VITTE_IR_INVALID_ID;
}

static inline bool
vitte_ir_instruction_id_is_valid(vitte_ir_instruction_id_t id)
{
    return id != VITTE_IR_INVALID_ID;
}

static inline bool
vitte_ir_block_id_is_valid(vitte_ir_block_id_t id)
{
    return id != VITTE_IR_INVALID_ID;
}

static inline bool
vitte_ir_function_id_is_valid(vitte_ir_function_id_t id)
{
    return id != VITTE_IR_INVALID_ID;
}

/* ========================================================================= */
/* Inline span helpers                                                       */
/* ========================================================================= */

static inline vitte_ir_span_t
vitte_ir_span_invalid(void)
{
    vitte_ir_span_t span;

    span.file_id = 0u;
    span.begin = 0u;
    span.end = 0u;
    span.valid = false;

    return span;
}

static inline vitte_ir_span_t
vitte_ir_span_make(
    uint32_t file_id,
    size_t begin,
    size_t end)
{
    vitte_ir_span_t span;

    span.file_id = file_id;
    span.begin = begin;
    span.end = end;
    span.valid = begin <= end;

    return span;
}

static inline size_t
vitte_ir_span_length(const vitte_ir_span_t *span)
{
    if (span == NULL ||
        !span->valid ||
        span->end < span->begin) {
        return 0u;
    }

    return span->end - span->begin;
}

/* ========================================================================= */
/* Inline context helpers                                                    */
/* ========================================================================= */

static inline vitte_ir_state_t
vitte_ir_state(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return VITTE_IR_STATE_INVALID;
    }

    return context->state;
}

static inline vitte_ir_error_t
vitte_ir_last_error(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return VITTE_IR_ERROR_INVALID_CONTEXT;
    }

    return context->last_error;
}

static inline bool
vitte_ir_is_building(const vitte_ir_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_IR_MAGIC &&
           context->state == VITTE_IR_STATE_BUILDING;
}

static inline bool
vitte_ir_is_sealed(const vitte_ir_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_IR_MAGIC &&
           context->state == VITTE_IR_STATE_SEALED;
}

static inline bool
vitte_ir_has_failed(const vitte_ir_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_IR_MAGIC &&
           context->state == VITTE_IR_STATE_FAILED;
}

static inline uint64_t
vitte_ir_generation(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return UINT64_C(0);
    }

    return context->generation;
}

/* ========================================================================= */
/* Inline counts                                                             */
/* ========================================================================= */

static inline size_t
vitte_ir_string_count(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return 0u;
    }

    return context->string_count;
}

static inline size_t
vitte_ir_type_count(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return 0u;
    }

    return context->type_count;
}

static inline size_t
vitte_ir_value_count(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return 0u;
    }

    return context->value_count;
}

static inline size_t
vitte_ir_instruction_count(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return 0u;
    }

    return context->instruction_count;
}

static inline size_t
vitte_ir_block_count(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return 0u;
    }

    return context->block_count;
}

static inline size_t
vitte_ir_function_count(const vitte_ir_context_t *context)
{
    if (context == NULL ||
        context->magic != VITTE_IR_MAGIC) {
        return 0u;
    }

    return context->function_count;
}

/* ========================================================================= */
/* Inline type classification                                                */
/* ========================================================================= */

static inline bool
vitte_ir_type_kind_is_signed_integer(vitte_ir_type_kind_t kind)
{
    return kind == VITTE_IR_TYPE_I8 ||
           kind == VITTE_IR_TYPE_I16 ||
           kind == VITTE_IR_TYPE_I32 ||
           kind == VITTE_IR_TYPE_I64;
}

static inline bool
vitte_ir_type_kind_is_unsigned_integer(vitte_ir_type_kind_t kind)
{
    return kind == VITTE_IR_TYPE_U8 ||
           kind == VITTE_IR_TYPE_U16 ||
           kind == VITTE_IR_TYPE_U32 ||
           kind == VITTE_IR_TYPE_U64;
}

static inline bool
vitte_ir_type_kind_is_integer(vitte_ir_type_kind_t kind)
{
    return vitte_ir_type_kind_is_signed_integer(kind) ||
           vitte_ir_type_kind_is_unsigned_integer(kind);
}

static inline bool
vitte_ir_type_kind_is_float(vitte_ir_type_kind_t kind)
{
    return kind == VITTE_IR_TYPE_F32 ||
           kind == VITTE_IR_TYPE_F64;
}

static inline bool
vitte_ir_type_kind_is_numeric(vitte_ir_type_kind_t kind)
{
    return vitte_ir_type_kind_is_integer(kind) ||
           vitte_ir_type_kind_is_float(kind);
}

static inline bool
vitte_ir_type_kind_is_scalar(vitte_ir_type_kind_t kind)
{
    return kind == VITTE_IR_TYPE_BOOL ||
           kind == VITTE_IR_TYPE_CHAR ||
           vitte_ir_type_kind_is_numeric(kind) ||
           kind == VITTE_IR_TYPE_POINTER;
}

/* ========================================================================= */
/* Inline descriptor constructors                                            */
/* ========================================================================= */

static inline vitte_ir_type_desc_t
vitte_ir_type_desc_empty(void)
{
    vitte_ir_type_desc_t description = {0};

    description.kind = VITTE_IR_TYPE_INVALID;

    return description;
}

static inline vitte_ir_value_desc_t
vitte_ir_value_desc_empty(void)
{
    vitte_ir_value_desc_t description = {0};

    description.kind = VITTE_IR_VALUE_INVALID;
    description.span = vitte_ir_span_invalid();

    return description;
}

static inline vitte_ir_instruction_desc_t
vitte_ir_instruction_desc_empty(void)
{
    vitte_ir_instruction_desc_t description = {0};

    description.opcode = VITTE_IR_OP_INVALID;
    description.span = vitte_ir_span_invalid();

    return description;
}

static inline vitte_ir_function_desc_t
vitte_ir_function_desc_empty(void)
{
    vitte_ir_function_desc_t description = {0};

    description.linkage = VITTE_IR_LINKAGE_INTERNAL;
    description.calling_convention = VITTE_IR_CALL_DEFAULT;
    description.span = vitte_ir_span_invalid();

    return description;
}

/* ========================================================================= */
/* Inline object queries                                                     */
/* ========================================================================= */

static inline const char *
vitte_ir_string_data(
    const vitte_ir_context_t *context,
    vitte_ir_string_id_t id)
{
    const vitte_ir_string_t *string;

    string = vitte_ir_get_string(context, id);

    return string != NULL ? string->data : NULL;
}

static inline size_t
vitte_ir_string_length(
    const vitte_ir_context_t *context,
    vitte_ir_string_id_t id)
{
    const vitte_ir_string_t *string;

    string = vitte_ir_get_string(context, id);

    return string != NULL ? string->length : 0u;
}

static inline vitte_ir_type_id_t
vitte_ir_value_type(
    const vitte_ir_context_t *context,
    vitte_ir_value_id_t id)
{
    const vitte_ir_value_t *value;

    value = vitte_ir_get_value(context, id);

    return value != NULL
        ? value->type
        : VITTE_IR_INVALID_ID;
}

static inline uint64_t
vitte_ir_value_use_count(
    const vitte_ir_context_t *context,
    vitte_ir_value_id_t id)
{
    const vitte_ir_value_t *value;

    value = vitte_ir_get_value(context, id);

    return value != NULL
        ? value->use_count
        : UINT64_C(0);
}

static inline bool
vitte_ir_block_is_terminated(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id)
{
    const vitte_ir_block_t *block;

    block = vitte_ir_get_block(context, id);

    return block != NULL && block->terminated;
}

static inline size_t
vitte_ir_block_instruction_count(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id)
{
    const vitte_ir_block_t *block;

    block = vitte_ir_get_block(context, id);

    return block != NULL
        ? block->instruction_count
        : 0u;
}

static inline size_t
vitte_ir_block_predecessor_count(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id)
{
    const vitte_ir_block_t *block;

    block = vitte_ir_get_block(context, id);

    return block != NULL
        ? block->predecessor_count
        : 0u;
}

static inline size_t
vitte_ir_block_successor_count(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id)
{
    const vitte_ir_block_t *block;

    block = vitte_ir_get_block(context, id);

    return block != NULL
        ? block->successor_count
        : 0u;
}

static inline size_t
vitte_ir_function_parameter_count(
    const vitte_ir_context_t *context,
    vitte_ir_function_id_t id)
{
    const vitte_ir_function_t *function;

    function = vitte_ir_get_function(context, id);

    return function != NULL
        ? function->parameter_count
        : 0u;
}

static inline size_t
vitte_ir_function_block_count(
    const vitte_ir_context_t *context,
    vitte_ir_function_id_t id)
{
    const vitte_ir_function_t *function;

    function = vitte_ir_get_function(context, id);

    return function != NULL
        ? function->block_count
        : 0u;
}

/* ========================================================================= */
/* Indexed access helpers                                                    */
/* ========================================================================= */

static inline vitte_ir_value_id_t
vitte_ir_function_parameter_at(
    const vitte_ir_context_t *context,
    vitte_ir_function_id_t id,
    size_t index)
{
    const vitte_ir_function_t *function;

    function = vitte_ir_get_function(context, id);

    if (function == NULL ||
        index >= function->parameter_count ||
        function->parameters == NULL) {
        return VITTE_IR_INVALID_ID;
    }

    return function->parameters[index];
}

static inline vitte_ir_block_id_t
vitte_ir_function_block_at(
    const vitte_ir_context_t *context,
    vitte_ir_function_id_t id,
    size_t index)
{
    const vitte_ir_function_t *function;

    function = vitte_ir_get_function(context, id);

    if (function == NULL ||
        index >= function->block_count ||
        function->blocks == NULL) {
        return VITTE_IR_INVALID_ID;
    }

    return function->blocks[index];
}

static inline vitte_ir_instruction_id_t
vitte_ir_block_instruction_at(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id,
    size_t index)
{
    const vitte_ir_block_t *block;

    block = vitte_ir_get_block(context, id);

    if (block == NULL ||
        index >= block->instruction_count ||
        block->instructions == NULL) {
        return VITTE_IR_INVALID_ID;
    }

    return block->instructions[index];
}

static inline vitte_ir_block_id_t
vitte_ir_block_predecessor_at(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id,
    size_t index)
{
    const vitte_ir_block_t *block;

    block = vitte_ir_get_block(context, id);

    if (block == NULL ||
        index >= block->predecessor_count ||
        block->predecessors == NULL) {
        return VITTE_IR_INVALID_ID;
    }

    return block->predecessors[index];
}

static inline vitte_ir_block_id_t
vitte_ir_block_successor_at(
    const vitte_ir_context_t *context,
    vitte_ir_block_id_t id,
    size_t index)
{
    const vitte_ir_block_t *block;

    block = vitte_ir_get_block(context, id);

    if (block == NULL ||
        index >= block->successor_count ||
        block->successors == NULL) {
        return VITTE_IR_INVALID_ID;
    }

    return block->successors[index];
}

static inline vitte_ir_value_id_t
vitte_ir_instruction_operand_at(
    const vitte_ir_context_t *context,
    vitte_ir_instruction_id_t id,
    size_t index)
{
    const vitte_ir_instruction_t *instruction;

    instruction = vitte_ir_get_instruction(context, id);

    if (instruction == NULL ||
        index >= instruction->operand_count ||
        instruction->operands == NULL) {
        return VITTE_IR_INVALID_ID;
    }

    return instruction->operands[index];
}

/* ========================================================================= */
/* Compile-time contract                                                     */
/* ========================================================================= */

#if defined(__cplusplus)

#define VITTE_IR_STATIC_ASSERT(condition, message) \
    static_assert((condition), message)

#else

#define VITTE_IR_STATIC_ASSERT(condition, message) \
    _Static_assert((condition), message)

#endif

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_INVALID_ID == UINT64_C(0),
    "IR zero ID must remain invalid");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_DEFAULT_INITIAL_CAPACITY > 0u,
    "IR initial capacity must be non-zero");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_DEFAULT_MAX_TYPES > 0u,
    "IR type limit must be non-zero");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_DEFAULT_MAX_VALUES > 0u,
    "IR value limit must be non-zero");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_DEFAULT_MAX_INSTRUCTIONS > 0u,
    "IR instruction limit must be non-zero");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_DEFAULT_MAX_BLOCKS > 0u,
    "IR block limit must be non-zero");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_DEFAULT_MAX_FUNCTIONS > 0u,
    "IR function limit must be non-zero");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_DEFAULT_MAX_STRING_BYTES > 0u,
    "IR string-byte limit must be non-zero");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_ERROR_COUNT > VITTE_IR_ERROR_INTERNAL,
    "IR error enum invariant");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_STATE_COUNT > VITTE_IR_STATE_DESTROYED,
    "IR state enum invariant");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_TYPE_COUNT > VITTE_IR_TYPE_OPAQUE,
    "IR type enum invariant");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_VALUE_COUNT > VITTE_IR_VALUE_FUNCTION,
    "IR value enum invariant");

VITTE_IR_STATIC_ASSERT(
    VITTE_IR_OP_COUNT > VITTE_IR_OP_UNREACHABLE,
    "IR opcode enum invariant");

#undef VITTE_IR_STATIC_ASSERT

#ifdef __cplusplus
}
#endif

#endif /* VITTE_IR_IR_H */
