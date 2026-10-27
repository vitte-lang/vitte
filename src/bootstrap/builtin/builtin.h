#ifndef VITTE_BUILTIN_BUILTIN_H
#define VITTE_BUILTIN_BUILTIN_H

/*
 * Vitte Compiler
 * builtin/builtin.h
 *
 * Canonical compiler builtin registry API.
 *
 * This header describes compiler-known builtin operations independently
 * from parser, semantic analysis, IR and backend implementation details.
 *
 * C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_BUILTIN_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_BUILTIN_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_BUILTIN_VARIADIC_ARITY \
    ((size_t)-1)

/* ========================================================================= */
/* Category                                                                  */
/* ========================================================================= */

typedef enum vitte_builtin_category {
    VITTE_BUILTIN_CATEGORY_INVALID = 0,

    VITTE_BUILTIN_CATEGORY_MEMORY,
    VITTE_BUILTIN_CATEGORY_POINTER,
    VITTE_BUILTIN_CATEGORY_TYPE,
    VITTE_BUILTIN_CATEGORY_INTEGER,
    VITTE_BUILTIN_CATEGORY_FLOAT,
    VITTE_BUILTIN_CATEGORY_BIT,
    VITTE_BUILTIN_CATEGORY_ATOMIC,
    VITTE_BUILTIN_CATEGORY_CONTROL,
    VITTE_BUILTIN_CATEGORY_DEBUG,
    VITTE_BUILTIN_CATEGORY_COMPILER,

    VITTE_BUILTIN_CATEGORY_COUNT
} vitte_builtin_category_t;

/* ========================================================================= */
/* Flags                                                                     */
/* ========================================================================= */

typedef enum vitte_builtin_flag {
    VITTE_BUILTIN_FLAG_NONE =
        0u,

    /*
     * The operation has no externally observable side effects.
     */
    VITTE_BUILTIN_FLAG_PURE =
        1u << 0,

    /*
     * The operation can potentially be evaluated at compile time.
     *
     * CONST implies PURE.
     */
    VITTE_BUILTIN_FLAG_CONST =
        1u << 1,

    /*
     * The operation requires an unsafe context or equivalent semantic
     * permission.
     */
    VITTE_BUILTIN_FLAG_UNSAFE =
        1u << 2,

    /*
     * Successful execution never returns to the caller.
     */
    VITTE_BUILTIN_FLAG_NORETURN =
        1u << 3,

    /*
     * The operation can read memory observable by the program.
     */
    VITTE_BUILTIN_FLAG_MEMORY_READ =
        1u << 4,

    /*
     * The operation can modify memory observable by the program.
     */
    VITTE_BUILTIN_FLAG_MEMORY_WRITE =
        1u << 5,

    /*
     * The operation has atomic semantics.
     */
    VITTE_BUILTIN_FLAG_ATOMIC =
        1u << 6,

    /*
     * The operation depends on target properties.
     */
    VITTE_BUILTIN_FLAG_TARGET_DEPENDENT =
        1u << 7,

    /*
     * The operation is intended to be available during compile-time
     * evaluation.
     */
    VITTE_BUILTIN_FLAG_COMPTIME =
        1u << 8,

    /*
     * The compiler should lower this operation directly instead of treating
     * it as an ordinary Vitte function call.
     */
    VITTE_BUILTIN_FLAG_LOWER_DIRECTLY =
        1u << 9
} vitte_builtin_flag_t;

/* ========================================================================= */
/* Builtin IDs                                                               */
/* ========================================================================= */

typedef enum vitte_builtin_id {
    VITTE_BUILTIN_INVALID = 0,

    /* --------------------------------------------------------------------- */
    /* Memory                                                                */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_MEMCPY,
    VITTE_BUILTIN_MEMMOVE,
    VITTE_BUILTIN_MEMSET,
    VITTE_BUILTIN_MEMCMP,

    /* --------------------------------------------------------------------- */
    /* Pointer                                                               */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_PTR_OFFSET,
    VITTE_BUILTIN_PTR_DIFF,
    VITTE_BUILTIN_PTR_IS_NULL,

    /* --------------------------------------------------------------------- */
    /* Type                                                                  */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_SIZEOF,
    VITTE_BUILTIN_ALIGNOF,
    VITTE_BUILTIN_OFFSETOF,

    /* --------------------------------------------------------------------- */
    /* Checked integer operations                                            */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_ADD_OVERFLOW,
    VITTE_BUILTIN_SUB_OVERFLOW,
    VITTE_BUILTIN_MUL_OVERFLOW,

    /* --------------------------------------------------------------------- */
    /* Bit operations                                                        */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_CLZ,
    VITTE_BUILTIN_CTZ,
    VITTE_BUILTIN_POPCOUNT,
    VITTE_BUILTIN_BSWAP,
    VITTE_BUILTIN_ROTL,
    VITTE_BUILTIN_ROTR,

    /* --------------------------------------------------------------------- */
    /* Floating-point classification                                         */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_ISNAN,
    VITTE_BUILTIN_ISINF,
    VITTE_BUILTIN_ISFINITE,

    /* --------------------------------------------------------------------- */
    /* Atomics                                                               */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_ATOMIC_LOAD,
    VITTE_BUILTIN_ATOMIC_STORE,
    VITTE_BUILTIN_ATOMIC_EXCHANGE,
    VITTE_BUILTIN_ATOMIC_COMPARE_EXCHANGE,
    VITTE_BUILTIN_ATOMIC_FETCH_ADD,
    VITTE_BUILTIN_ATOMIC_FETCH_SUB,
    VITTE_BUILTIN_ATOMIC_FENCE,

    /* --------------------------------------------------------------------- */
    /* Control                                                               */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_UNREACHABLE,
    VITTE_BUILTIN_TRAP,

    /* --------------------------------------------------------------------- */
    /* Compiler                                                              */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_ASSUME,

    /* --------------------------------------------------------------------- */
    /* Debug                                                                 */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_DEBUG_BREAK,

    /* --------------------------------------------------------------------- */
    /* Compiler/target queries                                               */
    /* --------------------------------------------------------------------- */

    VITTE_BUILTIN_COMPILER_VERSION,
    VITTE_BUILTIN_TARGET_POINTER_BITS,
    VITTE_BUILTIN_TARGET_ENDIAN,

    /*
     * Sentinel.
     *
     * Not a real builtin.
     */
    VITTE_BUILTIN_COUNT
} vitte_builtin_id_t;

/* ========================================================================= */
/* Descriptor                                                                */
/* ========================================================================= */

typedef struct vitte_builtin_descriptor {
    /*
     * Stable semantic identifier.
     */
    vitte_builtin_id_t id;

    /*
     * Canonical source/compiler name.
     *
     * Owned by the builtin registry.
     */
    const char *name;

    /*
     * Human-readable description.
     *
     * Owned by the builtin registry.
     */
    const char *description;

    /*
     * Semantic category.
     */
    vitte_builtin_category_t category;

    /*
     * Minimum accepted number of arguments.
     */
    size_t minimum_arity;

    /*
     * Maximum accepted number of arguments.
     *
     * VITTE_BUILTIN_VARIADIC_ARITY means no finite upper bound.
     */
    size_t maximum_arity;

    /*
     * Bitwise combination of vitte_builtin_flag_t.
     */
    uint32_t flags;
} vitte_builtin_descriptor_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_builtin_stats {
    size_t builtin_count;

    size_t pure_count;
    size_t const_count;
    size_t unsafe_count;
    size_t noreturn_count;

    size_t memory_read_count;
    size_t memory_write_count;

    size_t atomic_count;

    size_t target_dependent_count;
    size_t comptime_count;
    size_t directly_lowered_count;

    /*
     * Stable registry fingerprint.
     *
     * The implementation hashes explicit integer encodings rather than raw
     * host object representations so this value is deterministic across
     * host endianness.
     */
    uint64_t registry_hash;
} vitte_builtin_stats_t;

/* ========================================================================= */
/* Visitor                                                                   */
/* ========================================================================= */

typedef bool
(*vitte_builtin_visit_fn)(
    const vitte_builtin_descriptor_t *descriptor,
    void *user_data);

/* ========================================================================= */
/* Category API                                                              */
/* ========================================================================= */

/*
 * Return a stable textual representation of a builtin category.
 *
 * The returned string has static storage duration.
 */
const char *
vitte_builtin_category_name(
    vitte_builtin_category_t category);

/* ========================================================================= */
/* Registry API                                                              */
/* ========================================================================= */

/*
 * Return the number of real builtin descriptors.
 *
 * VITTE_BUILTIN_INVALID and VITTE_BUILTIN_COUNT are not counted.
 */
size_t
vitte_builtin_count(void);

/*
 * Return a descriptor by deterministic registry index.
 *
 * Returns NULL if index is out of range.
 *
 * The returned pointer is owned by the registry and remains valid for the
 * lifetime of the process.
 */
const vitte_builtin_descriptor_t *
vitte_builtin_at(
    size_t index);

/*
 * Lookup a builtin by semantic ID.
 *
 * Returns NULL for:
 *   VITTE_BUILTIN_INVALID
 *   VITTE_BUILTIN_COUNT
 *   unknown/out-of-range values
 */
const vitte_builtin_descriptor_t *
vitte_builtin_find_by_id(
    vitte_builtin_id_t id);

/*
 * Lookup a builtin by exact canonical name.
 *
 * Matching is byte-exact and case-sensitive.
 *
 * Returns NULL when:
 *   name == NULL
 *   name is empty
 *   no builtin matches
 */
const vitte_builtin_descriptor_t *
vitte_builtin_find(
    const char *name);

/*
 * Return true if an exact canonical builtin name exists.
 */
bool
vitte_builtin_exists(
    const char *name);

/* ========================================================================= */
/* ID API                                                                    */
/* ========================================================================= */

/*
 * Return the canonical builtin name corresponding to an ID.
 *
 * Special values:
 *
 *   VITTE_BUILTIN_INVALID -> "invalid"
 *   VITTE_BUILTIN_COUNT   -> "count"
 *
 * Invalid numeric values return "invalid".
 *
 * The returned string has static storage duration.
 */
const char *
vitte_builtin_id_name(
    vitte_builtin_id_t id);

/* ========================================================================= */
/* Descriptor queries                                                        */
/* ========================================================================= */

/*
 * Test a descriptor flag.
 *
 * Returns false for a NULL descriptor.
 */
bool
vitte_builtin_has_flag(
    const vitte_builtin_descriptor_t *descriptor,
    vitte_builtin_flag_t flag);

/*
 * Return true if the builtin accepts exactly this argument count.
 */
bool
vitte_builtin_accepts_arity(
    const vitte_builtin_descriptor_t *descriptor,
    size_t arity);

/*
 * Return true if the builtin has no finite maximum arity.
 */
bool
vitte_builtin_is_variadic(
    const vitte_builtin_descriptor_t *descriptor);

/*
 * Semantic convenience predicates.
 */
bool
vitte_builtin_is_pure(
    const vitte_builtin_descriptor_t *descriptor);

bool
vitte_builtin_is_const(
    const vitte_builtin_descriptor_t *descriptor);

bool
vitte_builtin_is_unsafe(
    const vitte_builtin_descriptor_t *descriptor);

bool
vitte_builtin_is_noreturn(
    const vitte_builtin_descriptor_t *descriptor);

bool
vitte_builtin_reads_memory(
    const vitte_builtin_descriptor_t *descriptor);

bool
vitte_builtin_writes_memory(
    const vitte_builtin_descriptor_t *descriptor);

bool
vitte_builtin_is_atomic(
    const vitte_builtin_descriptor_t *descriptor);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate all static registry invariants.
 *
 * Checks include:
 *
 *   - descriptor validity
 *   - ID ranges
 *   - category ranges
 *   - non-empty names
 *   - non-empty descriptions
 *   - valid arity ranges
 *   - unique IDs
 *   - unique names
 *   - every semantic builtin ID represented exactly once
 *   - CONST implies PURE
 *   - ATOMIC implies ATOMIC category
 *
 * This operation does not allocate memory.
 */
bool
vitte_builtin_registry_validate(void);

/* ========================================================================= */
/* Deterministic registry fingerprint                                        */
/* ========================================================================= */

/*
 * Compute a deterministic FNV-1a fingerprint of the complete builtin
 * registry.
 *
 * Included properties:
 *
 *   ID
 *   name
 *   description
 *   category
 *   minimum arity
 *   maximum arity
 *   flags
 *
 * Integer fields use a defined byte encoding and therefore do not depend on
 * host endianness.
 */
uint64_t
vitte_builtin_registry_hash(void);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

/*
 * Compute registry statistics.
 *
 * No allocation is performed.
 */
vitte_builtin_stats_t
vitte_builtin_stats(void);

/*
 * Count real builtins belonging to a category.
 *
 * INVALID and COUNT categories return zero.
 */
size_t
vitte_builtin_count_category(
    vitte_builtin_category_t category);

/*
 * Count descriptors containing the requested flag.
 */
size_t
vitte_builtin_count_flag(
    vitte_builtin_flag_t flag);

/* ========================================================================= */
/* Iteration                                                                 */
/* ========================================================================= */

/*
 * Visit every builtin in deterministic registry order.
 *
 * Iteration stops immediately when the visitor returns false.
 *
 * Returns:
 *
 *   false - visitor == NULL or visitor requested termination
 *   true  - every descriptor was visited
 */
bool
vitte_builtin_visit(
    vitte_builtin_visit_fn visitor,
    void *user_data);

/* ========================================================================= */
/* Inline helpers                                                            */
/* ========================================================================= */

static inline bool
vitte_builtin_id_is_valid(
    vitte_builtin_id_t id)
{
    return
        id > VITTE_BUILTIN_INVALID &&
        id < VITTE_BUILTIN_COUNT;
}

static inline bool
vitte_builtin_category_is_valid(
    vitte_builtin_category_t category)
{
    return
        category >
            VITTE_BUILTIN_CATEGORY_INVALID &&
        category <
            VITTE_BUILTIN_CATEGORY_COUNT;
}

static inline bool
vitte_builtin_is_memory_operation(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->category ==
            VITTE_BUILTIN_CATEGORY_MEMORY;
}

static inline bool
vitte_builtin_is_pointer_operation(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->category ==
            VITTE_BUILTIN_CATEGORY_POINTER;
}

static inline bool
vitte_builtin_is_type_operation(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->category ==
            VITTE_BUILTIN_CATEGORY_TYPE;
}

static inline bool
vitte_builtin_is_integer_operation(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->category ==
            VITTE_BUILTIN_CATEGORY_INTEGER;
}

static inline bool
vitte_builtin_is_float_operation(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->category ==
            VITTE_BUILTIN_CATEGORY_FLOAT;
}

static inline bool
vitte_builtin_is_bit_operation(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->category ==
            VITTE_BUILTIN_CATEGORY_BIT;
}

static inline bool
vitte_builtin_is_control_operation(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->category ==
            VITTE_BUILTIN_CATEGORY_CONTROL;
}

static inline bool
vitte_builtin_is_compiler_operation(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->category ==
            VITTE_BUILTIN_CATEGORY_COMPILER;
}

static inline bool
vitte_builtin_is_target_dependent(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_TARGET_DEPENDENT);
}

static inline bool
vitte_builtin_is_comptime(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_COMPTIME);
}

static inline bool
vitte_builtin_lowers_directly(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_LOWER_DIRECTLY);
}

static inline bool
vitte_builtin_touches_memory(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_reads_memory(
            descriptor) ||
        vitte_builtin_writes_memory(
            descriptor);
}

static inline bool
vitte_builtin_has_fixed_arity(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->maximum_arity !=
            VITTE_BUILTIN_VARIADIC_ARITY &&
        descriptor->minimum_arity ==
            descriptor->maximum_arity;
}

static inline size_t
vitte_builtin_fixed_arity(
    const vitte_builtin_descriptor_t *descriptor)
{
    if (!vitte_builtin_has_fixed_arity(
            descriptor)) {
        return 0u;
    }

    return descriptor->minimum_arity;
}

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BUILTIN_BUILTIN_H */
