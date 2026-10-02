#ifndef VITTE_CONSTANT_FOLD_CONSTANT_H
#define VITTE_CONSTANT_FOLD_CONSTANT_H

/*
 * Vitte Compiler
 * src/constant_fold/constant.h
 *
 * Canonical public API for compile-time constant values and primitive
 * constant folding.
 *
 * This layer represents already-known constant values and provides the
 * primitive operations required by higher constant-evaluation layers.
 *
 * Architecture:
 *
 *     parser / AST
 *          |
 *          v
 *     semantic analysis
 *          |
 *          v
 *     typed AST / HIR / IR
 *          |
 *          v
 *     constant evaluator
 *          |
 *          v
 *     +---------------------+
 *     |     constant.h      |
 *     |     constant.c      |
 *     +---------------------+
 *          |
 *          +-- typed constant values
 *          +-- unary folding
 *          +-- binary folding
 *          +-- checked arithmetic
 *          +-- comparisons
 *          +-- casts
 *          +-- deterministic hashing
 *          +-- validation
 *          +-- statistics
 *
 * This subsystem does NOT own:
 *
 *   - AST nodes
 *   - HIR nodes
 *   - IR nodes
 *   - symbol resolution
 *   - type inference
 *   - diagnostic rendering
 *   - source management
 *   - arbitrary-precision integers
 *   - arbitrary-precision floating point
 *
 * Ownership:
 *
 *   vitte_constant_t itself owns no heap allocation.
 *
 *   String constants are borrowed:
 *
 *       value.as.string.data
 *
 *   must remain valid for as long as the constant is used.
 *
 * Threading:
 *
 *   Pure value operations are reentrant when their input objects are not
 *   concurrently mutated.
 *
 *   Global statistics are process-global and are not synchronized. They are
 *   intended for compiler instrumentation, tests and diagnostics rather than
 *   concurrent mutation from multiple threads.
 *
 * Language:
 *
 *   ISO C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Version                                                                   */
/* ========================================================================= */

#define VITTE_CONSTANT_API_VERSION_MAJOR 1u
#define VITTE_CONSTANT_API_VERSION_MINOR 0u
#define VITTE_CONSTANT_API_VERSION_PATCH 0u

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

#define VITTE_CONSTANT_MAX_INTEGER_BITS 64u

#define VITTE_CONSTANT_MAX_STRING_LENGTH \
    ((size_t)(16u * 1024u * 1024u))

/* ========================================================================= */
/* Deterministic hashing                                                     */
/* ========================================================================= */

#define VITTE_CONSTANT_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_CONSTANT_FNV_PRIME \
    UINT64_C(1099511628211)

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

/*
 * Errors represent failures while manipulating an already-known constant.
 *
 * They intentionally do not contain source locations or diagnostic strings.
 * Higher layers are responsible for attaching source context and mapping these
 * errors into Vitte diagnostic codes.
 */
typedef enum vitte_constant_error {
    VITTE_CONSTANT_ERROR_NONE = 0,

    VITTE_CONSTANT_ERROR_INVALID_ARGUMENT,
    VITTE_CONSTANT_ERROR_INVALID_VALUE,
    VITTE_CONSTANT_ERROR_INVALID_TYPE,
    VITTE_CONSTANT_ERROR_INVALID_OPERATOR,

    VITTE_CONSTANT_ERROR_TYPE_MISMATCH,

    VITTE_CONSTANT_ERROR_OVERFLOW,
    VITTE_CONSTANT_ERROR_UNDERFLOW,

    VITTE_CONSTANT_ERROR_DIVISION_BY_ZERO,
    VITTE_CONSTANT_ERROR_REMAINDER_BY_ZERO,

    VITTE_CONSTANT_ERROR_INVALID_SHIFT,
    VITTE_CONSTANT_ERROR_INVALID_CAST,

    VITTE_CONSTANT_ERROR_NON_FINITE,
    VITTE_CONSTANT_ERROR_STRING_TOO_LONG,

    VITTE_CONSTANT_ERROR_UNSUPPORTED,

    VITTE_CONSTANT_ERROR_COUNT
} vitte_constant_error_t;

/* ========================================================================= */
/* Constant kinds                                                            */
/* ========================================================================= */

/*
 * The primitive constant representation currently supports integer values up
 * to 64 bits.
 *
 * Arbitrary precision values, target-sized integers and semantic aliases
 * should be handled by higher constant-evaluation/type layers until explicitly
 * added to this API.
 */
typedef enum vitte_constant_kind {
    VITTE_CONSTANT_KIND_INVALID = 0,

    VITTE_CONSTANT_KIND_BOOL,

    VITTE_CONSTANT_KIND_I8,
    VITTE_CONSTANT_KIND_I16,
    VITTE_CONSTANT_KIND_I32,
    VITTE_CONSTANT_KIND_I64,

    VITTE_CONSTANT_KIND_U8,
    VITTE_CONSTANT_KIND_U16,
    VITTE_CONSTANT_KIND_U32,
    VITTE_CONSTANT_KIND_U64,

    VITTE_CONSTANT_KIND_F32,
    VITTE_CONSTANT_KIND_F64,

    VITTE_CONSTANT_KIND_CHAR,
    VITTE_CONSTANT_KIND_STRING,

    VITTE_CONSTANT_KIND_NULL,

    /*
     * Poison represents a value whose computation already failed.
     *
     * It is a valid internal constant representation but is not a successful
     * folding result.
     */
    VITTE_CONSTANT_KIND_POISON,

    VITTE_CONSTANT_KIND_COUNT
} vitte_constant_kind_t;

/* ========================================================================= */
/* Unary operators                                                           */
/* ========================================================================= */

typedef enum vitte_constant_unary_operator {
    VITTE_CONSTANT_UNARY_INVALID = 0,

    VITTE_CONSTANT_UNARY_PLUS,
    VITTE_CONSTANT_UNARY_NEGATE,

    VITTE_CONSTANT_UNARY_LOGICAL_NOT,
    VITTE_CONSTANT_UNARY_BITWISE_NOT,

    VITTE_CONSTANT_UNARY_COUNT
} vitte_constant_unary_operator_t;

/* ========================================================================= */
/* Binary operators                                                          */
/* ========================================================================= */

typedef enum vitte_constant_binary_operator {
    VITTE_CONSTANT_BINARY_INVALID = 0,

    VITTE_CONSTANT_BINARY_ADD,
    VITTE_CONSTANT_BINARY_SUBTRACT,
    VITTE_CONSTANT_BINARY_MULTIPLY,
    VITTE_CONSTANT_BINARY_DIVIDE,
    VITTE_CONSTANT_BINARY_REMAINDER,

    VITTE_CONSTANT_BINARY_BIT_AND,
    VITTE_CONSTANT_BINARY_BIT_OR,
    VITTE_CONSTANT_BINARY_BIT_XOR,

    VITTE_CONSTANT_BINARY_SHIFT_LEFT,
    VITTE_CONSTANT_BINARY_SHIFT_RIGHT,

    VITTE_CONSTANT_BINARY_ROTATE_LEFT,
    VITTE_CONSTANT_BINARY_ROTATE_RIGHT,

    VITTE_CONSTANT_BINARY_LOGICAL_AND,
    VITTE_CONSTANT_BINARY_LOGICAL_OR,

    VITTE_CONSTANT_BINARY_EQUAL,
    VITTE_CONSTANT_BINARY_NOT_EQUAL,

    VITTE_CONSTANT_BINARY_LESS,
    VITTE_CONSTANT_BINARY_LESS_EQUAL,
    VITTE_CONSTANT_BINARY_GREATER,
    VITTE_CONSTANT_BINARY_GREATER_EQUAL,

    VITTE_CONSTANT_BINARY_COUNT
} vitte_constant_binary_operator_t;

/* ========================================================================= */
/* Fold status                                                               */
/* ========================================================================= */

/*
 * SUCCESS
 *
 *   The expression was evaluated successfully and result.value contains the
 *   folded constant.
 *
 * NOT_CONSTANT
 *
 *   Reserved for higher evaluation layers where an expression is valid but
 *   cannot be evaluated at compile time.
 *
 * ERROR
 *
 *   Folding failed and result.error contains the reason.
 */
typedef enum vitte_constant_fold_status {
    VITTE_CONSTANT_FOLD_INVALID = 0,

    VITTE_CONSTANT_FOLD_SUCCESS,
    VITTE_CONSTANT_FOLD_NOT_CONSTANT,
    VITTE_CONSTANT_FOLD_ERROR,

    VITTE_CONSTANT_FOLD_COUNT
} vitte_constant_fold_status_t;

/* ========================================================================= */
/* String constant                                                           */
/* ========================================================================= */

/*
 * Binary-safe borrowed byte string.
 *
 * The buffer:
 *
 *   - does not need to be NUL terminated;
 *   - may contain embedded NUL bytes;
 *   - is not copied by this subsystem;
 *   - must remain alive while the constant is used.
 *
 * Empty strings may use:
 *
 *     data   = NULL
 *     length = 0
 */
typedef struct vitte_constant_string {
    const char *data;
    size_t length;
} vitte_constant_string_t;

/* ========================================================================= */
/* Constant value                                                            */
/* ========================================================================= */

typedef struct vitte_constant {
    vitte_constant_kind_t kind;

    union {
        bool boolean;

        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;

        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;

        float f32;
        double f64;

        /*
         * Unicode scalar value.
         *
         * Valid values:
         *
         *     U+0000 .. U+D7FF
         *     U+E000 .. U+10FFFF
         */
        uint32_t character;

        vitte_constant_string_t string;
    } as;
} vitte_constant_t;

/* ========================================================================= */
/* Fold result                                                               */
/* ========================================================================= */

typedef struct vitte_constant_result {
    vitte_constant_fold_status_t status;
    vitte_constant_error_t error;
    vitte_constant_t value;
} vitte_constant_result_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

/*
 * Instrumentation counters.
 *
 * These counters are diagnostic/instrumentation data. They are not part of
 * the semantic result of constant evaluation.
 */
typedef struct vitte_constant_stats {
    uint64_t fold_attempts;
    uint64_t fold_successes;
    uint64_t fold_failures;

    uint64_t unary_folds;
    uint64_t binary_folds;

    uint64_t integer_folds;
    uint64_t floating_folds;
    uint64_t boolean_folds;
    uint64_t comparison_folds;
    uint64_t bitwise_folds;

    uint64_t overflow_errors;
    uint64_t division_by_zero_errors;
    uint64_t invalid_shift_errors;
} vitte_constant_stats_t;

/* ========================================================================= */
/* Name helpers                                                              */
/* ========================================================================= */

/*
 * Returned strings have static storage duration.
 */
const char *
vitte_constant_error_name(vitte_constant_error_t error);

const char *
vitte_constant_kind_name(vitte_constant_kind_t kind);

const char *
vitte_constant_unary_operator_name(vitte_constant_unary_operator_t op);

const char *
vitte_constant_binary_operator_name(vitte_constant_binary_operator_t op);

const char *
vitte_constant_fold_status_name(vitte_constant_fold_status_t status);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate the structural representation of a constant.
 *
 * POISON is considered structurally valid.
 * INVALID and COUNT are not.
 */
bool
vitte_constant_is_valid(const vitte_constant_t *value);

/* ========================================================================= */
/* Classification                                                            */
/* ========================================================================= */

bool
vitte_constant_is_integer(const vitte_constant_t *value);

bool
vitte_constant_is_signed_integer(const vitte_constant_t *value);

bool
vitte_constant_is_unsigned_integer(const vitte_constant_t *value);

bool
vitte_constant_is_float(const vitte_constant_t *value);

bool
vitte_constant_is_numeric(const vitte_constant_t *value);

/* ========================================================================= */
/* Common-value queries                                                      */
/* ========================================================================= */

bool
vitte_constant_is_zero(const vitte_constant_t *value);

bool
vitte_constant_is_one(const vitte_constant_t *value);

/* ========================================================================= */
/* Type properties                                                           */
/* ========================================================================= */

/*
 * Return the primitive representation width for fixed-width numeric kinds.
 *
 * Returns zero for kinds without a numeric representation width.
 */
unsigned
vitte_constant_bit_width(vitte_constant_kind_t kind);

/* ========================================================================= */
/* Structural equality                                                       */
/* ========================================================================= */

/*
 * Equality is kind-sensitive.
 *
 * Examples:
 *
 *     i32(1) != i64(1)
 *     u32(1) != i32(1)
 *     f32(1) != f64(1)
 *
 * String comparison is byte-exact.
 *
 * This function is structural constant equality. Language-level semantic
 * comparison is performed by vitte_constant_fold_binary() after semantic
 * typing has selected the operand kinds.
 */
bool
vitte_constant_equal(
    const vitte_constant_t *left,
    const vitte_constant_t *right);

/* ========================================================================= */
/* Deterministic hashing                                                     */
/* ========================================================================= */

/*
 * Produce a deterministic FNV-1a based hash.
 *
 * The hash includes the constant kind.
 *
 * Integer values are serialized explicitly rather than hashing host structure
 * memory, avoiding padding and host-layout dependencies.
 *
 * Floating values are hashed from their representation bits.
 *
 * The function returns zero for structurally invalid values.
 *
 * The hash is suitable for:
 *
 *   - compiler caches
 *   - constant tables
 *   - deterministic fingerprints
 *   - tests
 *
 * It is not a cryptographic hash.
 */
uint64_t
vitte_constant_hash(const vitte_constant_t *value);

/* ========================================================================= */
/* Unary folding                                                             */
/* ========================================================================= */

/*
 * Fold a primitive unary operation.
 *
 * Arithmetic overflow is diagnosed instead of relying on host C signed
 * overflow.
 *
 * Examples:
 *
 *     +42
 *     -42
 *     !true
 *     ~0xff
 */
vitte_constant_result_t
vitte_constant_fold_unary(
    vitte_constant_unary_operator_t op,
    const vitte_constant_t *operand);

/* ========================================================================= */
/* Binary folding                                                            */
/* ========================================================================= */

/*
 * Fold a primitive binary operation.
 *
 * Arithmetic operands are currently required to have the same primitive kind.
 * Semantic type conversion belongs to the type checker or should be performed
 * explicitly with vitte_constant_cast() before folding.
 *
 * Checked conditions include:
 *
 *   - signed overflow
 *   - unsigned overflow
 *   - unsigned underflow
 *   - division by zero
 *   - remainder by zero
 *   - invalid shift counts
 *   - unsupported operand/operator combinations
 *
 * Signed right shift is implemented explicitly so compile-time behavior does
 * not depend on the host C implementation's treatment of negative right
 * shifts.
 */
vitte_constant_result_t
vitte_constant_fold_binary(
    vitte_constant_binary_operator_t op,
    const vitte_constant_t *left,
    const vitte_constant_t *right);

/* ========================================================================= */
/* Constant casts                                                            */
/* ========================================================================= */

/*
 * Perform a checked primitive constant conversion.
 *
 * Supported families currently include:
 *
 *   integer -> integer
 *   integer -> floating
 *   floating -> integer
 *   floating -> floating
 *   numeric -> bool
 *   bool/numeric/string/null -> bool where truth conversion is defined
 *   integer -> char when the value is a valid Unicode scalar
 *
 * Narrowing integer conversions are checked.
 *
 * Floating-to-integer conversion requires a finite in-range value.
 *
 * This API performs primitive representation conversion. Whether a cast is
 * legal according to Vitte's source-language type system remains the semantic
 * analyzer's responsibility.
 */
vitte_constant_result_t
vitte_constant_cast(
    const vitte_constant_t *value,
    vitte_constant_kind_t destination);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_constant_stats_t
vitte_constant_stats(void);

void
vitte_constant_stats_reset(void);

/* ========================================================================= */
/* Integration anchor                                                        */
/* ========================================================================= */

/*
 * Small translation-unit integration anchor.
 *
 * It performs no semantic compiler work and exists primarily for subsystem
 * integration/link tests.
 */
void
vitte_constant_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline kind queries                                                       */
/* ========================================================================= */

static inline bool
vitte_constant_kind_is_valid(vitte_constant_kind_t kind)
{
    return
        kind > VITTE_CONSTANT_KIND_INVALID &&
        kind < VITTE_CONSTANT_KIND_COUNT;
}

static inline bool
vitte_constant_kind_is_integer_public(vitte_constant_kind_t kind)
{
    return
        kind == VITTE_CONSTANT_KIND_I8 ||
        kind == VITTE_CONSTANT_KIND_I16 ||
        kind == VITTE_CONSTANT_KIND_I32 ||
        kind == VITTE_CONSTANT_KIND_I64 ||
        kind == VITTE_CONSTANT_KIND_U8 ||
        kind == VITTE_CONSTANT_KIND_U16 ||
        kind == VITTE_CONSTANT_KIND_U32 ||
        kind == VITTE_CONSTANT_KIND_U64;
}

static inline bool
vitte_constant_kind_is_signed_integer(vitte_constant_kind_t kind)
{
    return
        kind == VITTE_CONSTANT_KIND_I8 ||
        kind == VITTE_CONSTANT_KIND_I16 ||
        kind == VITTE_CONSTANT_KIND_I32 ||
        kind == VITTE_CONSTANT_KIND_I64;
}

static inline bool
vitte_constant_kind_is_unsigned_integer(vitte_constant_kind_t kind)
{
    return
        kind == VITTE_CONSTANT_KIND_U8 ||
        kind == VITTE_CONSTANT_KIND_U16 ||
        kind == VITTE_CONSTANT_KIND_U32 ||
        kind == VITTE_CONSTANT_KIND_U64;
}

static inline bool
vitte_constant_kind_is_floating(vitte_constant_kind_t kind)
{
    return
        kind == VITTE_CONSTANT_KIND_F32 ||
        kind == VITTE_CONSTANT_KIND_F64;
}

static inline bool
vitte_constant_kind_is_numeric(vitte_constant_kind_t kind)
{
    return
        vitte_constant_kind_is_integer_public(kind) ||
        vitte_constant_kind_is_floating(kind);
}

/* ========================================================================= */
/* Inline result queries                                                     */
/* ========================================================================= */

static inline bool
vitte_constant_result_succeeded(
    const vitte_constant_result_t *result)
{
    return
        result != NULL &&
        result->status == VITTE_CONSTANT_FOLD_SUCCESS &&
        result->error == VITTE_CONSTANT_ERROR_NONE;
}

static inline bool
vitte_constant_result_failed(
    const vitte_constant_result_t *result)
{
    return
        result != NULL &&
        result->status == VITTE_CONSTANT_FOLD_ERROR;
}

static inline bool
vitte_constant_result_is_not_constant(
    const vitte_constant_result_t *result)
{
    return
        result != NULL &&
        result->status == VITTE_CONSTANT_FOLD_NOT_CONSTANT;
}

/* ========================================================================= */
/* Inline constructors                                                       */
/* ========================================================================= */

static inline vitte_constant_t
vitte_constant_make_invalid(void)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_INVALID;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_poison(void)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_POISON;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_null(void)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_NULL;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_bool(bool input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_BOOL;
    value.as.boolean = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_i8(int8_t input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_I8;
    value.as.i8 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_i16(int16_t input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_I16;
    value.as.i16 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_i32(int32_t input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_I32;
    value.as.i32 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_i64(int64_t input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_I64;
    value.as.i64 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_u8(uint8_t input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_U8;
    value.as.u8 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_u16(uint16_t input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_U16;
    value.as.u16 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_u32(uint32_t input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_U32;
    value.as.u32 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_u64(uint64_t input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_U64;
    value.as.u64 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_f32(float input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_F32;
    value.as.f32 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_f64(double input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_F64;
    value.as.f64 = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_char(uint32_t input)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_CHAR;
    value.as.character = input;

    return value;
}

static inline vitte_constant_t
vitte_constant_make_string(
    const char *data,
    size_t length)
{
    vitte_constant_t value = {0};

    value.kind = VITTE_CONSTANT_KIND_STRING;
    value.as.string.data = data;
    value.as.string.length = length;

    return value;
}

/* ========================================================================= */
/* Inline status constructors                                                */
/* ========================================================================= */

static inline vitte_constant_result_t
vitte_constant_make_success(vitte_constant_t value)
{
    vitte_constant_result_t result = {0};

    result.status = VITTE_CONSTANT_FOLD_SUCCESS;
    result.error = VITTE_CONSTANT_ERROR_NONE;
    result.value = value;

    return result;
}

static inline vitte_constant_result_t
vitte_constant_make_error(vitte_constant_error_t error)
{
    vitte_constant_result_t result = {0};

    result.status = VITTE_CONSTANT_FOLD_ERROR;
    result.error = error;
    result.value = vitte_constant_make_poison();

    return result;
}

static inline vitte_constant_result_t
vitte_constant_make_not_constant(void)
{
    vitte_constant_result_t result = {0};

    result.status = VITTE_CONSTANT_FOLD_NOT_CONSTANT;
    result.error = VITTE_CONSTANT_ERROR_NONE;
    result.value = vitte_constant_make_invalid();

    return result;
}

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(
    VITTE_CONSTANT_MAX_INTEGER_BITS == 64u,
    "constant representation currently assumes at most 64-bit integers");

_Static_assert(
    sizeof(int8_t) == 1u,
    "int8_t must occupy one byte");

_Static_assert(
    sizeof(uint8_t) == 1u,
    "uint8_t must occupy one byte");

_Static_assert(
    sizeof(int16_t) == 2u,
    "int16_t must occupy two bytes");

_Static_assert(
    sizeof(uint16_t) == 2u,
    "uint16_t must occupy two bytes");

_Static_assert(
    sizeof(int32_t) == 4u,
    "int32_t must occupy four bytes");

_Static_assert(
    sizeof(uint32_t) == 4u,
    "uint32_t must occupy four bytes");

_Static_assert(
    sizeof(int64_t) == 8u,
    "int64_t must occupy eight bytes");

_Static_assert(
    sizeof(uint64_t) == 8u,
    "uint64_t must occupy eight bytes");

/* ========================================================================= */
/* C++                                                                      */
/* ========================================================================= */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_CONSTANT_FOLD_CONSTANT_H */
