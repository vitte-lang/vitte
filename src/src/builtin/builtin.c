/*
 * Vitte Compiler
 * builtin/builtin.c
 *
 * Canonical builtin registry.
 *
 * Responsibilities:
 *   - define compiler-known builtin operations
 *   - provide deterministic builtin metadata
 *   - classify builtins by semantic domain
 *   - describe arity and semantic properties
 *   - provide name/id lookup
 *   - validate registry invariants
 *   - expose stable hashing and statistics
 *
 * This file intentionally contains no parser, type-checker, IR, backend,
 * allocator, diagnostic renderer, or target-specific implementation logic.
 *
 * C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#define VITTE_BUILTIN_FNV_OFFSET UINT64_C(14695981039346656037)
#define VITTE_BUILTIN_FNV_PRIME  UINT64_C(1099511628211)

#define VITTE_BUILTIN_VARIADIC_ARITY ((size_t)-1)

/* ========================================================================= */
/* Public-style model                                                        */
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

typedef enum vitte_builtin_flag {
    VITTE_BUILTIN_FLAG_NONE =
        0u,

    VITTE_BUILTIN_FLAG_PURE =
        1u << 0,

    VITTE_BUILTIN_FLAG_CONST =
        1u << 1,

    VITTE_BUILTIN_FLAG_UNSAFE =
        1u << 2,

    VITTE_BUILTIN_FLAG_NORETURN =
        1u << 3,

    VITTE_BUILTIN_FLAG_MEMORY_READ =
        1u << 4,

    VITTE_BUILTIN_FLAG_MEMORY_WRITE =
        1u << 5,

    VITTE_BUILTIN_FLAG_ATOMIC =
        1u << 6,

    VITTE_BUILTIN_FLAG_TARGET_DEPENDENT =
        1u << 7,

    VITTE_BUILTIN_FLAG_COMPTIME =
        1u << 8,

    VITTE_BUILTIN_FLAG_LOWER_DIRECTLY =
        1u << 9
} vitte_builtin_flag_t;

typedef enum vitte_builtin_id {
    VITTE_BUILTIN_INVALID = 0,

    /* Memory. */
    VITTE_BUILTIN_MEMCPY,
    VITTE_BUILTIN_MEMMOVE,
    VITTE_BUILTIN_MEMSET,
    VITTE_BUILTIN_MEMCMP,

    /* Pointer / object model. */
    VITTE_BUILTIN_PTR_OFFSET,
    VITTE_BUILTIN_PTR_DIFF,
    VITTE_BUILTIN_PTR_IS_NULL,

    /* Type introspection. */
    VITTE_BUILTIN_SIZEOF,
    VITTE_BUILTIN_ALIGNOF,
    VITTE_BUILTIN_OFFSETOF,

    /* Integer arithmetic helpers. */
    VITTE_BUILTIN_ADD_OVERFLOW,
    VITTE_BUILTIN_SUB_OVERFLOW,
    VITTE_BUILTIN_MUL_OVERFLOW,

    /* Integer bit operations. */
    VITTE_BUILTIN_CLZ,
    VITTE_BUILTIN_CTZ,
    VITTE_BUILTIN_POPCOUNT,
    VITTE_BUILTIN_BSWAP,
    VITTE_BUILTIN_ROTL,
    VITTE_BUILTIN_ROTR,

    /* Floating point. */
    VITTE_BUILTIN_ISNAN,
    VITTE_BUILTIN_ISINF,
    VITTE_BUILTIN_ISFINITE,

    /* Atomics. */
    VITTE_BUILTIN_ATOMIC_LOAD,
    VITTE_BUILTIN_ATOMIC_STORE,
    VITTE_BUILTIN_ATOMIC_EXCHANGE,
    VITTE_BUILTIN_ATOMIC_COMPARE_EXCHANGE,
    VITTE_BUILTIN_ATOMIC_FETCH_ADD,
    VITTE_BUILTIN_ATOMIC_FETCH_SUB,
    VITTE_BUILTIN_ATOMIC_FENCE,

    /* Control. */
    VITTE_BUILTIN_UNREACHABLE,
    VITTE_BUILTIN_TRAP,
    VITTE_BUILTIN_ASSUME,

    /* Debugging. */
    VITTE_BUILTIN_DEBUG_BREAK,

    /* Compiler queries. */
    VITTE_BUILTIN_COMPILER_VERSION,
    VITTE_BUILTIN_TARGET_POINTER_BITS,
    VITTE_BUILTIN_TARGET_ENDIAN,

    VITTE_BUILTIN_COUNT
} vitte_builtin_id_t;

typedef struct vitte_builtin_descriptor {
    vitte_builtin_id_t id;

    const char *name;
    const char *description;

    vitte_builtin_category_t category;

    size_t minimum_arity;
    size_t maximum_arity;

    uint32_t flags;
} vitte_builtin_descriptor_t;

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

    uint64_t registry_hash;
} vitte_builtin_stats_t;

/* ========================================================================= */
/* Descriptor helpers                                                        */
/* ========================================================================= */

#define VITTE_FLAGS_PURE \
    (VITTE_BUILTIN_FLAG_PURE | \
     VITTE_BUILTIN_FLAG_CONST)

#define VITTE_FLAGS_QUERY \
    (VITTE_BUILTIN_FLAG_PURE | \
     VITTE_BUILTIN_FLAG_CONST | \
     VITTE_BUILTIN_FLAG_COMPTIME | \
     VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)

#define VITTE_FLAGS_TARGET_QUERY \
    (VITTE_FLAGS_QUERY | \
     VITTE_BUILTIN_FLAG_TARGET_DEPENDENT)

#define VITTE_FLAGS_MEMORY_READ \
    (VITTE_BUILTIN_FLAG_MEMORY_READ | \
     VITTE_BUILTIN_FLAG_UNSAFE)

#define VITTE_FLAGS_MEMORY_WRITE \
    (VITTE_BUILTIN_FLAG_MEMORY_WRITE | \
     VITTE_BUILTIN_FLAG_UNSAFE)

#define VITTE_FLAGS_ATOMIC_READ \
    (VITTE_BUILTIN_FLAG_ATOMIC | \
     VITTE_BUILTIN_FLAG_MEMORY_READ | \
     VITTE_BUILTIN_FLAG_UNSAFE | \
     VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)

#define VITTE_FLAGS_ATOMIC_WRITE \
    (VITTE_BUILTIN_FLAG_ATOMIC | \
     VITTE_BUILTIN_FLAG_MEMORY_WRITE | \
     VITTE_BUILTIN_FLAG_UNSAFE | \
     VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)

#define VITTE_FLAGS_ATOMIC_RW \
    (VITTE_BUILTIN_FLAG_ATOMIC | \
     VITTE_BUILTIN_FLAG_MEMORY_READ | \
     VITTE_BUILTIN_FLAG_MEMORY_WRITE | \
     VITTE_BUILTIN_FLAG_UNSAFE | \
     VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)

#define VITTE_BUILTIN_ENTRY( \
    builtin_id, \
    builtin_name, \
    builtin_description, \
    builtin_category, \
    min_arity, \
    max_arity, \
    builtin_flags) \
    { \
        (builtin_id), \
        (builtin_name), \
        (builtin_description), \
        (builtin_category), \
        (min_arity), \
        (max_arity), \
        (uint32_t)(builtin_flags) \
    }

/* ========================================================================= */
/* Canonical registry                                                        */
/* ========================================================================= */

static const vitte_builtin_descriptor_t
vitte_builtin_registry[] = {
    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MEMCPY,
        "memcpy",
        "Copy a non-overlapping memory region.",
        VITTE_BUILTIN_CATEGORY_MEMORY,
        3u,
        3u,
        VITTE_FLAGS_MEMORY_READ |
        VITTE_BUILTIN_FLAG_MEMORY_WRITE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MEMMOVE,
        "memmove",
        "Copy a potentially overlapping memory region.",
        VITTE_BUILTIN_CATEGORY_MEMORY,
        3u,
        3u,
        VITTE_FLAGS_MEMORY_READ |
        VITTE_BUILTIN_FLAG_MEMORY_WRITE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MEMSET,
        "memset",
        "Fill a memory region.",
        VITTE_BUILTIN_CATEGORY_MEMORY,
        3u,
        3u,
        VITTE_FLAGS_MEMORY_WRITE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MEMCMP,
        "memcmp",
        "Compare two memory regions.",
        VITTE_BUILTIN_CATEGORY_MEMORY,
        3u,
        3u,
        VITTE_FLAGS_MEMORY_READ),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_PTR_OFFSET,
        "ptr_offset",
        "Offset a pointer by an element displacement.",
        VITTE_BUILTIN_CATEGORY_POINTER,
        2u,
        2u,
        VITTE_BUILTIN_FLAG_UNSAFE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_PTR_DIFF,
        "ptr_diff",
        "Compute the displacement between compatible pointers.",
        VITTE_BUILTIN_CATEGORY_POINTER,
        2u,
        2u,
        VITTE_BUILTIN_FLAG_UNSAFE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_PTR_IS_NULL,
        "ptr_is_null",
        "Test whether a pointer is null.",
        VITTE_BUILTIN_CATEGORY_POINTER,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_SIZEOF,
        "sizeof",
        "Return the storage size of a type or value.",
        VITTE_BUILTIN_CATEGORY_TYPE,
        1u,
        1u,
        VITTE_FLAGS_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ALIGNOF,
        "alignof",
        "Return the required alignment of a type.",
        VITTE_BUILTIN_CATEGORY_TYPE,
        1u,
        1u,
        VITTE_FLAGS_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_OFFSETOF,
        "offsetof",
        "Return a field offset inside an aggregate type.",
        VITTE_BUILTIN_CATEGORY_TYPE,
        2u,
        2u,
        VITTE_FLAGS_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ADD_OVERFLOW,
        "add_overflow",
        "Perform integer addition and report overflow.",
        VITTE_BUILTIN_CATEGORY_INTEGER,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_SUB_OVERFLOW,
        "sub_overflow",
        "Perform integer subtraction and report overflow.",
        VITTE_BUILTIN_CATEGORY_INTEGER,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MUL_OVERFLOW,
        "mul_overflow",
        "Perform integer multiplication and report overflow.",
        VITTE_BUILTIN_CATEGORY_INTEGER,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_CLZ,
        "clz",
        "Count leading zero bits.",
        VITTE_BUILTIN_CATEGORY_BIT,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_CTZ,
        "ctz",
        "Count trailing zero bits.",
        VITTE_BUILTIN_CATEGORY_BIT,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_POPCOUNT,
        "popcount",
        "Count set bits.",
        VITTE_BUILTIN_CATEGORY_BIT,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_BSWAP,
        "bswap",
        "Reverse byte order.",
        VITTE_BUILTIN_CATEGORY_BIT,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ROTL,
        "rotl",
        "Rotate bits left.",
        VITTE_BUILTIN_CATEGORY_BIT,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ROTR,
        "rotr",
        "Rotate bits right.",
        VITTE_BUILTIN_CATEGORY_BIT,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ISNAN,
        "isnan",
        "Test whether a floating-point value is NaN.",
        VITTE_BUILTIN_CATEGORY_FLOAT,
        1u,
        1u,
        VITTE_FLAGS_PURE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ISINF,
        "isinf",
        "Test whether a floating-point value is infinite.",
        VITTE_BUILTIN_CATEGORY_FLOAT,
        1u,
        1u,
        VITTE_FLAGS_PURE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ISFINITE,
        "isfinite",
        "Test whether a floating-point value is finite.",
        VITTE_BUILTIN_CATEGORY_FLOAT,
        1u,
        1u,
        VITTE_FLAGS_PURE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_LOAD,
        "atomic_load",
        "Atomically load a value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        2u,
        2u,
        VITTE_FLAGS_ATOMIC_READ),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_STORE,
        "atomic_store",
        "Atomically store a value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        3u,
        3u,
        VITTE_FLAGS_ATOMIC_WRITE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_EXCHANGE,
        "atomic_exchange",
        "Atomically replace a value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        3u,
        3u,
        VITTE_FLAGS_ATOMIC_RW),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_COMPARE_EXCHANGE,
        "atomic_compare_exchange",
        "Atomically compare and conditionally exchange a value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        5u,
        5u,
        VITTE_FLAGS_ATOMIC_RW),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_FETCH_ADD,
        "atomic_fetch_add",
        "Atomically add and return the previous value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        3u,
        3u,
        VITTE_FLAGS_ATOMIC_RW),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_FETCH_SUB,
        "atomic_fetch_sub",
        "Atomically subtract and return the previous value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        3u,
        3u,
        VITTE_FLAGS_ATOMIC_RW),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_FENCE,
        "atomic_fence",
        "Emit an atomic memory fence.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        1u,
        1u,
        VITTE_BUILTIN_FLAG_ATOMIC |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_UNREACHABLE,
        "unreachable",
        "Mark the current control-flow path unreachable.",
        VITTE_BUILTIN_CATEGORY_CONTROL,
        0u,
        0u,
        VITTE_BUILTIN_FLAG_NORETURN |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_TRAP,
        "trap",
        "Terminate execution through a compiler trap.",
        VITTE_BUILTIN_CATEGORY_CONTROL,
        0u,
        0u,
        VITTE_BUILTIN_FLAG_NORETURN |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ASSUME,
        "assume",
        "Tell the optimizer that a condition is true.",
        VITTE_BUILTIN_CATEGORY_COMPILER,
        1u,
        1u,
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_DEBUG_BREAK,
        "debug_break",
        "Request a debugger breakpoint.",
        VITTE_BUILTIN_CATEGORY_DEBUG,
        0u,
        0u,
        VITTE_BUILTIN_FLAG_TARGET_DEPENDENT |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_COMPILER_VERSION,
        "compiler_version",
        "Return compiler version information.",
        VITTE_BUILTIN_CATEGORY_COMPILER,
        0u,
        0u,
        VITTE_FLAGS_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_TARGET_POINTER_BITS,
        "target_pointer_bits",
        "Return the target pointer width.",
        VITTE_BUILTIN_CATEGORY_COMPILER,
        0u,
        0u,
        VITTE_FLAGS_TARGET_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_TARGET_ENDIAN,
        "target_endian",
        "Return the target byte order.",
        VITTE_BUILTIN_CATEGORY_COMPILER,
        0u,
        0u,
        VITTE_FLAGS_TARGET_QUERY)
};

/* ========================================================================= */
/* Registry size                                                             */
/* ========================================================================= */

static size_t
vitte_builtin_registry_count_internal(void)
{
    return
        sizeof(vitte_builtin_registry) /
        sizeof(vitte_builtin_registry[0]);
}

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

static uint64_t
vitte_builtin_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    bytes = (const unsigned char *)data;

    for (index = 0u;
         index < length;
         ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_BUILTIN_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_builtin_hash_string(
    uint64_t hash,
    const char *text)
{
    if (text == NULL) {
        static const unsigned char null_marker = 0xffu;

        return
            vitte_builtin_hash_bytes(
                hash,
                &null_marker,
                sizeof(null_marker));
    }

    return
        vitte_builtin_hash_bytes(
            hash,
            text,
            strlen(text) + 1u);
}

static uint64_t
vitte_builtin_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned shift;

    /*
     * Hash an explicit little-endian representation rather than native
     * object bytes. Registry fingerprints therefore remain host-independent.
     */
    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        unsigned char byte;

        byte =
            (unsigned char)(
                (value >> shift) &
                UINT64_C(0xff));

        hash =
            vitte_builtin_hash_bytes(
                hash,
                &byte,
                1u);
    }

    return hash;
}

/* ========================================================================= */
/* Category                                                                  */
/* ========================================================================= */

const char *
vitte_builtin_category_name(
    vitte_builtin_category_t category)
{
    switch (category) {
        case VITTE_BUILTIN_CATEGORY_INVALID:
            return "invalid";

        case VITTE_BUILTIN_CATEGORY_MEMORY:
            return "memory";

        case VITTE_BUILTIN_CATEGORY_POINTER:
            return "pointer";

        case VITTE_BUILTIN_CATEGORY_TYPE:
            return "type";

        case VITTE_BUILTIN_CATEGORY_INTEGER:
            return "integer";

        case VITTE_BUILTIN_CATEGORY_FLOAT:
            return "float";

        case VITTE_BUILTIN_CATEGORY_BIT:
            return "bit";

        case VITTE_BUILTIN_CATEGORY_ATOMIC:
            return "atomic";

        case VITTE_BUILTIN_CATEGORY_CONTROL:
            return "control";

        case VITTE_BUILTIN_CATEGORY_DEBUG:
            return "debug";

        case VITTE_BUILTIN_CATEGORY_COMPILER:
            return "compiler";

        case VITTE_BUILTIN_CATEGORY_COUNT:
            return "count";
    }

    return "invalid";
}

/* ========================================================================= */
/* Basic registry API                                                        */
/* ========================================================================= */

size_t
vitte_builtin_count(void)
{
    return
        vitte_builtin_registry_count_internal();
}

const vitte_builtin_descriptor_t *
vitte_builtin_at(
    size_t index)
{
    if (index >=
        vitte_builtin_registry_count_internal()) {
        return NULL;
    }

    return &vitte_builtin_registry[index];
}

const vitte_builtin_descriptor_t *
vitte_builtin_find_by_id(
    vitte_builtin_id_t id)
{
    size_t index;

    if (id <= VITTE_BUILTIN_INVALID ||
        id >= VITTE_BUILTIN_COUNT) {
        return NULL;
    }

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        if (vitte_builtin_registry[index].id ==
            id) {
            return &vitte_builtin_registry[index];
        }
    }

    return NULL;
}

const vitte_builtin_descriptor_t *
vitte_builtin_find(
    const char *name)
{
    size_t index;

    if (name == NULL ||
        name[0] == '\0') {
        return NULL;
    }

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        const vitte_builtin_descriptor_t *descriptor;

        descriptor =
            &vitte_builtin_registry[index];

        if (strcmp(
                descriptor->name,
                name) == 0) {
            return descriptor;
        }
    }

    return NULL;
}

bool
vitte_builtin_exists(
    const char *name)
{
    return
        vitte_builtin_find(name) != NULL;
}

/* ========================================================================= */
/* Descriptor queries                                                        */
/* ========================================================================= */

bool
vitte_builtin_has_flag(
    const vitte_builtin_descriptor_t *descriptor,
    vitte_builtin_flag_t flag)
{
    if (descriptor == NULL) {
        return false;
    }

    return
        (descriptor->flags &
         (uint32_t)flag) != 0u;
}

bool
vitte_builtin_accepts_arity(
    const vitte_builtin_descriptor_t *descriptor,
    size_t arity)
{
    if (descriptor == NULL) {
        return false;
    }

    if (arity <
        descriptor->minimum_arity) {
        return false;
    }

    if (descriptor->maximum_arity ==
        VITTE_BUILTIN_VARIADIC_ARITY) {
        return true;
    }

    return
        arity <=
        descriptor->maximum_arity;
}

bool
vitte_builtin_is_variadic(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->maximum_arity ==
            VITTE_BUILTIN_VARIADIC_ARITY;
}

bool
vitte_builtin_is_pure(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_PURE);
}

bool
vitte_builtin_is_const(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_CONST);
}

bool
vitte_builtin_is_unsafe(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_UNSAFE);
}

bool
vitte_builtin_is_noreturn(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_NORETURN);
}

bool
vitte_builtin_reads_memory(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_MEMORY_READ);
}

bool
vitte_builtin_writes_memory(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_MEMORY_WRITE);
}

bool
vitte_builtin_is_atomic(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_ATOMIC);
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

static bool
vitte_builtin_descriptor_valid(
    const vitte_builtin_descriptor_t *descriptor)
{
    if (descriptor == NULL) {
        return false;
    }

    if (descriptor->id <=
            VITTE_BUILTIN_INVALID ||
        descriptor->id >=
            VITTE_BUILTIN_COUNT) {
        return false;
    }

    if (descriptor->name == NULL ||
        descriptor->name[0] == '\0') {
        return false;
    }

    if (descriptor->description == NULL ||
        descriptor->description[0] == '\0') {
        return false;
    }

    if (descriptor->category <=
            VITTE_BUILTIN_CATEGORY_INVALID ||
        descriptor->category >=
            VITTE_BUILTIN_CATEGORY_COUNT) {
        return false;
    }

    if (descriptor->maximum_arity !=
            VITTE_BUILTIN_VARIADIC_ARITY &&
        descriptor->minimum_arity >
            descriptor->maximum_arity) {
        return false;
    }

    if ((descriptor->flags &
         VITTE_BUILTIN_FLAG_CONST) != 0u &&
        (descriptor->flags &
         VITTE_BUILTIN_FLAG_PURE) == 0u) {
        return false;
    }

    if ((descriptor->flags &
         VITTE_BUILTIN_FLAG_ATOMIC) != 0u &&
        descriptor->category !=
            VITTE_BUILTIN_CATEGORY_ATOMIC) {
        return false;
    }

    return true;
}

bool
vitte_builtin_registry_validate(void)
{
    size_t count;
    size_t left;

    count =
        vitte_builtin_registry_count_internal();

    if (count !=
        (size_t)VITTE_BUILTIN_COUNT - 1u) {
        return false;
    }

    for (left = 0u;
         left < count;
         ++left) {
        size_t right;

        if (!vitte_builtin_descriptor_valid(
                &vitte_builtin_registry[left])) {
            return false;
        }

        for (right = left + 1u;
             right < count;
             ++right) {
            if (vitte_builtin_registry[left].id ==
                vitte_builtin_registry[right].id) {
                return false;
            }

            if (strcmp(
                    vitte_builtin_registry[left].name,
                    vitte_builtin_registry[right].name) == 0) {
                return false;
            }
        }
    }

    /*
     * Ensure every semantic ID is represented exactly once.
     */
    for (left = 1u;
         left < (size_t)VITTE_BUILTIN_COUNT;
         ++left) {
        size_t matches;
        size_t index;

        matches = 0u;

        for (index = 0u;
             index < count;
             ++index) {
            if ((size_t)vitte_builtin_registry[index].id ==
                left) {
                ++matches;
            }
        }

        if (matches != 1u) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Registry fingerprint                                                      */
/* ========================================================================= */

uint64_t
vitte_builtin_registry_hash(void)
{
    uint64_t hash;
    size_t index;

    hash = VITTE_BUILTIN_FNV_OFFSET;

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        const vitte_builtin_descriptor_t *descriptor;

        descriptor =
            &vitte_builtin_registry[index];

        hash =
            vitte_builtin_hash_u64(
                hash,
                (uint64_t)descriptor->id);

        hash =
            vitte_builtin_hash_string(
                hash,
                descriptor->name);

        hash =
            vitte_builtin_hash_string(
                hash,
                descriptor->description);

        hash =
            vitte_builtin_hash_u64(
                hash,
                (uint64_t)descriptor->category);

        hash =
            vitte_builtin_hash_u64(
                hash,
                (uint64_t)descriptor->minimum_arity);

        hash =
            vitte_builtin_hash_u64(
                hash,
                descriptor->maximum_arity ==
                        VITTE_BUILTIN_VARIADIC_ARITY
                    ? UINT64_MAX
                    : (uint64_t)descriptor->maximum_arity);

        hash =
            vitte_builtin_hash_u64(
                hash,
                (uint64_t)descriptor->flags);
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_builtin_stats_t
vitte_builtin_stats(void)
{
    vitte_builtin_stats_t stats;
    size_t index;

    memset(
        &stats,
        0,
        sizeof(stats));

    stats.builtin_count =
        vitte_builtin_registry_count_internal();

    for (index = 0u;
         index < stats.builtin_count;
         ++index) {
        const vitte_builtin_descriptor_t *descriptor;

        descriptor =
            &vitte_builtin_registry[index];

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_PURE)) {
            ++stats.pure_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_CONST)) {
            ++stats.const_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_UNSAFE)) {
            ++stats.unsafe_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_NORETURN)) {
            ++stats.noreturn_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_MEMORY_READ)) {
            ++stats.memory_read_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_MEMORY_WRITE)) {
            ++stats.memory_write_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_ATOMIC)) {
            ++stats.atomic_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_TARGET_DEPENDENT)) {
            ++stats.target_dependent_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_COMPTIME)) {
            ++stats.comptime_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)) {
            ++stats.directly_lowered_count;
        }
    }

    stats.registry_hash =
        vitte_builtin_registry_hash();

    return stats;
}

/* ========================================================================= */
/* ID helpers                                                                */
/* ========================================================================= */

const char *
vitte_builtin_id_name(
    vitte_builtin_id_t id)
{
    const vitte_builtin_descriptor_t *descriptor;

    if (id == VITTE_BUILTIN_INVALID) {
        return "invalid";
    }

    if (id == VITTE_BUILTIN_COUNT) {
        return "count";
    }

    descriptor =
        vitte_builtin_find_by_id(id);

    if (descriptor == NULL) {
        return "invalid";
    }

    return descriptor->name;
}

/* ========================================================================= */
/* Category statistics                                                       */
/* ========================================================================= */

size_t
vitte_builtin_count_category(
    vitte_builtin_category_t category)
{
    size_t count;
    size_t index;

    if (category <=
            VITTE_BUILTIN_CATEGORY_INVALID ||
        category >=
            VITTE_BUILTIN_CATEGORY_COUNT) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        if (vitte_builtin_registry[index].category ==
            category) {
            ++count;
        }
    }

    return count;
}

/* ========================================================================= */
/* Flag statistics                                                           */
/* ========================================================================= */

size_t
vitte_builtin_count_flag(
    vitte_builtin_flag_t flag)
{
    size_t count;
    size_t index;

    count = 0u;

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        if (vitte_builtin_has_flag(
                &vitte_builtin_registry[index],
                flag)) {
            ++count;
        }
    }

    return count;
}

/* ========================================================================= */
/* Deterministic iteration                                                   */
/* ========================================================================= */

typedef bool
(*vitte_builtin_visit_fn)(
    const vitte_builtin_descriptor_t *descriptor,
    void *user_data);

bool
vitte_builtin_visit(
    vitte_builtin_visit_fn visitor,
    void *user_data)
{
    size_t index;

    if (visitor == NULL) {
        return false;
    }

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        if (!visitor(
                &vitte_builtin_registry[index],
                user_data)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
#if 0 /* Accidentally concatenated duplicate translation unit. */
/*
 * Vitte Compiler
 * builtin/builtin.c
 *
 * Canonical builtin registry.
 *
 * Responsibilities:
 *   - define compiler-known builtin operations
 *   - provide deterministic builtin metadata
 *   - classify builtins by semantic domain
 *   - describe arity and semantic properties
 *   - provide name/id lookup
 *   - validate registry invariants
 *   - expose stable hashing and statistics
 *
 * This file intentionally contains no parser, type-checker, IR, backend,
 * allocator, diagnostic renderer, or target-specific implementation logic.
 *
 * C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#define VITTE_BUILTIN_FNV_OFFSET UINT64_C(14695981039346656037)
#define VITTE_BUILTIN_FNV_PRIME  UINT64_C(1099511628211)

#define VITTE_BUILTIN_VARIADIC_ARITY ((size_t)-1)

/* ========================================================================= */
/* Public-style model                                                        */
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

typedef enum vitte_builtin_flag {
    VITTE_BUILTIN_FLAG_NONE =
        0u,

    VITTE_BUILTIN_FLAG_PURE =
        1u << 0,

    VITTE_BUILTIN_FLAG_CONST =
        1u << 1,

    VITTE_BUILTIN_FLAG_UNSAFE =
        1u << 2,

    VITTE_BUILTIN_FLAG_NORETURN =
        1u << 3,

    VITTE_BUILTIN_FLAG_MEMORY_READ =
        1u << 4,

    VITTE_BUILTIN_FLAG_MEMORY_WRITE =
        1u << 5,

    VITTE_BUILTIN_FLAG_ATOMIC =
        1u << 6,

    VITTE_BUILTIN_FLAG_TARGET_DEPENDENT =
        1u << 7,

    VITTE_BUILTIN_FLAG_COMPTIME =
        1u << 8,

    VITTE_BUILTIN_FLAG_LOWER_DIRECTLY =
        1u << 9
} vitte_builtin_flag_t;

typedef enum vitte_builtin_id {
    VITTE_BUILTIN_INVALID = 0,

    /* Memory. */
    VITTE_BUILTIN_MEMCPY,
    VITTE_BUILTIN_MEMMOVE,
    VITTE_BUILTIN_MEMSET,
    VITTE_BUILTIN_MEMCMP,

    /* Pointer / object model. */
    VITTE_BUILTIN_PTR_OFFSET,
    VITTE_BUILTIN_PTR_DIFF,
    VITTE_BUILTIN_PTR_IS_NULL,

    /* Type introspection. */
    VITTE_BUILTIN_SIZEOF,
    VITTE_BUILTIN_ALIGNOF,
    VITTE_BUILTIN_OFFSETOF,

    /* Integer arithmetic helpers. */
    VITTE_BUILTIN_ADD_OVERFLOW,
    VITTE_BUILTIN_SUB_OVERFLOW,
    VITTE_BUILTIN_MUL_OVERFLOW,

    /* Integer bit operations. */
    VITTE_BUILTIN_CLZ,
    VITTE_BUILTIN_CTZ,
    VITTE_BUILTIN_POPCOUNT,
    VITTE_BUILTIN_BSWAP,
    VITTE_BUILTIN_ROTL,
    VITTE_BUILTIN_ROTR,

    /* Floating point. */
    VITTE_BUILTIN_ISNAN,
    VITTE_BUILTIN_ISINF,
    VITTE_BUILTIN_ISFINITE,

    /* Atomics. */
    VITTE_BUILTIN_ATOMIC_LOAD,
    VITTE_BUILTIN_ATOMIC_STORE,
    VITTE_BUILTIN_ATOMIC_EXCHANGE,
    VITTE_BUILTIN_ATOMIC_COMPARE_EXCHANGE,
    VITTE_BUILTIN_ATOMIC_FETCH_ADD,
    VITTE_BUILTIN_ATOMIC_FETCH_SUB,
    VITTE_BUILTIN_ATOMIC_FENCE,

    /* Control. */
    VITTE_BUILTIN_UNREACHABLE,
    VITTE_BUILTIN_TRAP,
    VITTE_BUILTIN_ASSUME,

    /* Debugging. */
    VITTE_BUILTIN_DEBUG_BREAK,

    /* Compiler queries. */
    VITTE_BUILTIN_COMPILER_VERSION,
    VITTE_BUILTIN_TARGET_POINTER_BITS,
    VITTE_BUILTIN_TARGET_ENDIAN,

    VITTE_BUILTIN_COUNT
} vitte_builtin_id_t;

typedef struct vitte_builtin_descriptor {
    vitte_builtin_id_t id;

    const char *name;
    const char *description;

    vitte_builtin_category_t category;

    size_t minimum_arity;
    size_t maximum_arity;

    uint32_t flags;
} vitte_builtin_descriptor_t;

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

    uint64_t registry_hash;
} vitte_builtin_stats_t;

/* ========================================================================= */
/* Descriptor helpers                                                        */
/* ========================================================================= */

#define VITTE_FLAGS_PURE \
    (VITTE_BUILTIN_FLAG_PURE | \
     VITTE_BUILTIN_FLAG_CONST)

#define VITTE_FLAGS_QUERY \
    (VITTE_BUILTIN_FLAG_PURE | \
     VITTE_BUILTIN_FLAG_CONST | \
     VITTE_BUILTIN_FLAG_COMPTIME | \
     VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)

#define VITTE_FLAGS_TARGET_QUERY \
    (VITTE_FLAGS_QUERY | \
     VITTE_BUILTIN_FLAG_TARGET_DEPENDENT)

#define VITTE_FLAGS_MEMORY_READ \
    (VITTE_BUILTIN_FLAG_MEMORY_READ | \
     VITTE_BUILTIN_FLAG_UNSAFE)

#define VITTE_FLAGS_MEMORY_WRITE \
    (VITTE_BUILTIN_FLAG_MEMORY_WRITE | \
     VITTE_BUILTIN_FLAG_UNSAFE)

#define VITTE_FLAGS_ATOMIC_READ \
    (VITTE_BUILTIN_FLAG_ATOMIC | \
     VITTE_BUILTIN_FLAG_MEMORY_READ | \
     VITTE_BUILTIN_FLAG_UNSAFE | \
     VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)

#define VITTE_FLAGS_ATOMIC_WRITE \
    (VITTE_BUILTIN_FLAG_ATOMIC | \
     VITTE_BUILTIN_FLAG_MEMORY_WRITE | \
     VITTE_BUILTIN_FLAG_UNSAFE | \
     VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)

#define VITTE_FLAGS_ATOMIC_RW \
    (VITTE_BUILTIN_FLAG_ATOMIC | \
     VITTE_BUILTIN_FLAG_MEMORY_READ | \
     VITTE_BUILTIN_FLAG_MEMORY_WRITE | \
     VITTE_BUILTIN_FLAG_UNSAFE | \
     VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)

#define VITTE_BUILTIN_ENTRY( \
    builtin_id, \
    builtin_name, \
    builtin_description, \
    builtin_category, \
    min_arity, \
    max_arity, \
    builtin_flags) \
    { \
        (builtin_id), \
        (builtin_name), \
        (builtin_description), \
        (builtin_category), \
        (min_arity), \
        (max_arity), \
        (uint32_t)(builtin_flags) \
    }

/* ========================================================================= */
/* Canonical registry                                                        */
/* ========================================================================= */

static const vitte_builtin_descriptor_t
vitte_builtin_registry[] = {
    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MEMCPY,
        "memcpy",
        "Copy a non-overlapping memory region.",
        VITTE_BUILTIN_CATEGORY_MEMORY,
        3u,
        3u,
        VITTE_FLAGS_MEMORY_READ |
        VITTE_BUILTIN_FLAG_MEMORY_WRITE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MEMMOVE,
        "memmove",
        "Copy a potentially overlapping memory region.",
        VITTE_BUILTIN_CATEGORY_MEMORY,
        3u,
        3u,
        VITTE_FLAGS_MEMORY_READ |
        VITTE_BUILTIN_FLAG_MEMORY_WRITE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MEMSET,
        "memset",
        "Fill a memory region.",
        VITTE_BUILTIN_CATEGORY_MEMORY,
        3u,
        3u,
        VITTE_FLAGS_MEMORY_WRITE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MEMCMP,
        "memcmp",
        "Compare two memory regions.",
        VITTE_BUILTIN_CATEGORY_MEMORY,
        3u,
        3u,
        VITTE_FLAGS_MEMORY_READ),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_PTR_OFFSET,
        "ptr_offset",
        "Offset a pointer by an element displacement.",
        VITTE_BUILTIN_CATEGORY_POINTER,
        2u,
        2u,
        VITTE_BUILTIN_FLAG_UNSAFE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_PTR_DIFF,
        "ptr_diff",
        "Compute the displacement between compatible pointers.",
        VITTE_BUILTIN_CATEGORY_POINTER,
        2u,
        2u,
        VITTE_BUILTIN_FLAG_UNSAFE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_PTR_IS_NULL,
        "ptr_is_null",
        "Test whether a pointer is null.",
        VITTE_BUILTIN_CATEGORY_POINTER,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_SIZEOF,
        "sizeof",
        "Return the storage size of a type or value.",
        VITTE_BUILTIN_CATEGORY_TYPE,
        1u,
        1u,
        VITTE_FLAGS_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ALIGNOF,
        "alignof",
        "Return the required alignment of a type.",
        VITTE_BUILTIN_CATEGORY_TYPE,
        1u,
        1u,
        VITTE_FLAGS_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_OFFSETOF,
        "offsetof",
        "Return a field offset inside an aggregate type.",
        VITTE_BUILTIN_CATEGORY_TYPE,
        2u,
        2u,
        VITTE_FLAGS_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ADD_OVERFLOW,
        "add_overflow",
        "Perform integer addition and report overflow.",
        VITTE_BUILTIN_CATEGORY_INTEGER,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_SUB_OVERFLOW,
        "sub_overflow",
        "Perform integer subtraction and report overflow.",
        VITTE_BUILTIN_CATEGORY_INTEGER,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_MUL_OVERFLOW,
        "mul_overflow",
        "Perform integer multiplication and report overflow.",
        VITTE_BUILTIN_CATEGORY_INTEGER,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_CLZ,
        "clz",
        "Count leading zero bits.",
        VITTE_BUILTIN_CATEGORY_BIT,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_CTZ,
        "ctz",
        "Count trailing zero bits.",
        VITTE_BUILTIN_CATEGORY_BIT,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_POPCOUNT,
        "popcount",
        "Count set bits.",
        VITTE_BUILTIN_CATEGORY_BIT,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_BSWAP,
        "bswap",
        "Reverse byte order.",
        VITTE_BUILTIN_CATEGORY_BIT,
        1u,
        1u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ROTL,
        "rotl",
        "Rotate bits left.",
        VITTE_BUILTIN_CATEGORY_BIT,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ROTR,
        "rotr",
        "Rotate bits right.",
        VITTE_BUILTIN_CATEGORY_BIT,
        2u,
        2u,
        VITTE_FLAGS_PURE |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ISNAN,
        "isnan",
        "Test whether a floating-point value is NaN.",
        VITTE_BUILTIN_CATEGORY_FLOAT,
        1u,
        1u,
        VITTE_FLAGS_PURE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ISINF,
        "isinf",
        "Test whether a floating-point value is infinite.",
        VITTE_BUILTIN_CATEGORY_FLOAT,
        1u,
        1u,
        VITTE_FLAGS_PURE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ISFINITE,
        "isfinite",
        "Test whether a floating-point value is finite.",
        VITTE_BUILTIN_CATEGORY_FLOAT,
        1u,
        1u,
        VITTE_FLAGS_PURE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_LOAD,
        "atomic_load",
        "Atomically load a value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        2u,
        2u,
        VITTE_FLAGS_ATOMIC_READ),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_STORE,
        "atomic_store",
        "Atomically store a value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        3u,
        3u,
        VITTE_FLAGS_ATOMIC_WRITE),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_EXCHANGE,
        "atomic_exchange",
        "Atomically replace a value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        3u,
        3u,
        VITTE_FLAGS_ATOMIC_RW),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_COMPARE_EXCHANGE,
        "atomic_compare_exchange",
        "Atomically compare and conditionally exchange a value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        5u,
        5u,
        VITTE_FLAGS_ATOMIC_RW),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_FETCH_ADD,
        "atomic_fetch_add",
        "Atomically add and return the previous value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        3u,
        3u,
        VITTE_FLAGS_ATOMIC_RW),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_FETCH_SUB,
        "atomic_fetch_sub",
        "Atomically subtract and return the previous value.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        3u,
        3u,
        VITTE_FLAGS_ATOMIC_RW),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ATOMIC_FENCE,
        "atomic_fence",
        "Emit an atomic memory fence.",
        VITTE_BUILTIN_CATEGORY_ATOMIC,
        1u,
        1u,
        VITTE_BUILTIN_FLAG_ATOMIC |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_UNREACHABLE,
        "unreachable",
        "Mark the current control-flow path unreachable.",
        VITTE_BUILTIN_CATEGORY_CONTROL,
        0u,
        0u,
        VITTE_BUILTIN_FLAG_NORETURN |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_TRAP,
        "trap",
        "Terminate execution through a compiler trap.",
        VITTE_BUILTIN_CATEGORY_CONTROL,
        0u,
        0u,
        VITTE_BUILTIN_FLAG_NORETURN |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_ASSUME,
        "assume",
        "Tell the optimizer that a condition is true.",
        VITTE_BUILTIN_CATEGORY_COMPILER,
        1u,
        1u,
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_DEBUG_BREAK,
        "debug_break",
        "Request a debugger breakpoint.",
        VITTE_BUILTIN_CATEGORY_DEBUG,
        0u,
        0u,
        VITTE_BUILTIN_FLAG_TARGET_DEPENDENT |
        VITTE_BUILTIN_FLAG_LOWER_DIRECTLY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_COMPILER_VERSION,
        "compiler_version",
        "Return compiler version information.",
        VITTE_BUILTIN_CATEGORY_COMPILER,
        0u,
        0u,
        VITTE_FLAGS_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_TARGET_POINTER_BITS,
        "target_pointer_bits",
        "Return the target pointer width.",
        VITTE_BUILTIN_CATEGORY_COMPILER,
        0u,
        0u,
        VITTE_FLAGS_TARGET_QUERY),

    VITTE_BUILTIN_ENTRY(
        VITTE_BUILTIN_TARGET_ENDIAN,
        "target_endian",
        "Return the target byte order.",
        VITTE_BUILTIN_CATEGORY_COMPILER,
        0u,
        0u,
        VITTE_FLAGS_TARGET_QUERY)
};

/* ========================================================================= */
/* Registry size                                                             */
/* ========================================================================= */

static size_t
vitte_builtin_registry_count_internal(void)
{
    return
        sizeof(vitte_builtin_registry) /
        sizeof(vitte_builtin_registry[0]);
}

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

static uint64_t
vitte_builtin_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    bytes = (const unsigned char *)data;

    for (index = 0u;
         index < length;
         ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_BUILTIN_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_builtin_hash_string(
    uint64_t hash,
    const char *text)
{
    if (text == NULL) {
        static const unsigned char null_marker = 0xffu;

        return
            vitte_builtin_hash_bytes(
                hash,
                &null_marker,
                sizeof(null_marker));
    }

    return
        vitte_builtin_hash_bytes(
            hash,
            text,
            strlen(text) + 1u);
}

static uint64_t
vitte_builtin_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned shift;

    /*
     * Hash an explicit little-endian representation rather than native
     * object bytes. Registry fingerprints therefore remain host-independent.
     */
    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        unsigned char byte;

        byte =
            (unsigned char)(
                (value >> shift) &
                UINT64_C(0xff));

        hash =
            vitte_builtin_hash_bytes(
                hash,
                &byte,
                1u);
    }

    return hash;
}

/* ========================================================================= */
/* Category                                                                  */
/* ========================================================================= */

const char *
vitte_builtin_category_name(
    vitte_builtin_category_t category)
{
    switch (category) {
        case VITTE_BUILTIN_CATEGORY_INVALID:
            return "invalid";

        case VITTE_BUILTIN_CATEGORY_MEMORY:
            return "memory";

        case VITTE_BUILTIN_CATEGORY_POINTER:
            return "pointer";

        case VITTE_BUILTIN_CATEGORY_TYPE:
            return "type";

        case VITTE_BUILTIN_CATEGORY_INTEGER:
            return "integer";

        case VITTE_BUILTIN_CATEGORY_FLOAT:
            return "float";

        case VITTE_BUILTIN_CATEGORY_BIT:
            return "bit";

        case VITTE_BUILTIN_CATEGORY_ATOMIC:
            return "atomic";

        case VITTE_BUILTIN_CATEGORY_CONTROL:
            return "control";

        case VITTE_BUILTIN_CATEGORY_DEBUG:
            return "debug";

        case VITTE_BUILTIN_CATEGORY_COMPILER:
            return "compiler";

        case VITTE_BUILTIN_CATEGORY_COUNT:
            return "count";
    }

    return "invalid";
}

/* ========================================================================= */
/* Basic registry API                                                        */
/* ========================================================================= */

size_t
vitte_builtin_count(void)
{
    return
        vitte_builtin_registry_count_internal();
}

const vitte_builtin_descriptor_t *
vitte_builtin_at(
    size_t index)
{
    if (index >=
        vitte_builtin_registry_count_internal()) {
        return NULL;
    }

    return &vitte_builtin_registry[index];
}

const vitte_builtin_descriptor_t *
vitte_builtin_find_by_id(
    vitte_builtin_id_t id)
{
    size_t index;

    if (id <= VITTE_BUILTIN_INVALID ||
        id >= VITTE_BUILTIN_COUNT) {
        return NULL;
    }

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        if (vitte_builtin_registry[index].id ==
            id) {
            return &vitte_builtin_registry[index];
        }
    }

    return NULL;
}

const vitte_builtin_descriptor_t *
vitte_builtin_find(
    const char *name)
{
    size_t index;

    if (name == NULL ||
        name[0] == '\0') {
        return NULL;
    }

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        const vitte_builtin_descriptor_t *descriptor;

        descriptor =
            &vitte_builtin_registry[index];

        if (strcmp(
                descriptor->name,
                name) == 0) {
            return descriptor;
        }
    }

    return NULL;
}

bool
vitte_builtin_exists(
    const char *name)
{
    return
        vitte_builtin_find(name) != NULL;
}

/* ========================================================================= */
/* Descriptor queries                                                        */
/* ========================================================================= */

bool
vitte_builtin_has_flag(
    const vitte_builtin_descriptor_t *descriptor,
    vitte_builtin_flag_t flag)
{
    if (descriptor == NULL) {
        return false;
    }

    return
        (descriptor->flags &
         (uint32_t)flag) != 0u;
}

bool
vitte_builtin_accepts_arity(
    const vitte_builtin_descriptor_t *descriptor,
    size_t arity)
{
    if (descriptor == NULL) {
        return false;
    }

    if (arity <
        descriptor->minimum_arity) {
        return false;
    }

    if (descriptor->maximum_arity ==
        VITTE_BUILTIN_VARIADIC_ARITY) {
        return true;
    }

    return
        arity <=
        descriptor->maximum_arity;
}

bool
vitte_builtin_is_variadic(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        descriptor != NULL &&
        descriptor->maximum_arity ==
            VITTE_BUILTIN_VARIADIC_ARITY;
}

bool
vitte_builtin_is_pure(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_PURE);
}

bool
vitte_builtin_is_const(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_CONST);
}

bool
vitte_builtin_is_unsafe(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_UNSAFE);
}

bool
vitte_builtin_is_noreturn(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_NORETURN);
}

bool
vitte_builtin_reads_memory(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_MEMORY_READ);
}

bool
vitte_builtin_writes_memory(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_MEMORY_WRITE);
}

bool
vitte_builtin_is_atomic(
    const vitte_builtin_descriptor_t *descriptor)
{
    return
        vitte_builtin_has_flag(
            descriptor,
            VITTE_BUILTIN_FLAG_ATOMIC);
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

static bool
vitte_builtin_descriptor_valid(
    const vitte_builtin_descriptor_t *descriptor)
{
    if (descriptor == NULL) {
        return false;
    }

    if (descriptor->id <=
            VITTE_BUILTIN_INVALID ||
        descriptor->id >=
            VITTE_BUILTIN_COUNT) {
        return false;
    }

    if (descriptor->name == NULL ||
        descriptor->name[0] == '\0') {
        return false;
    }

    if (descriptor->description == NULL ||
        descriptor->description[0] == '\0') {
        return false;
    }

    if (descriptor->category <=
            VITTE_BUILTIN_CATEGORY_INVALID ||
        descriptor->category >=
            VITTE_BUILTIN_CATEGORY_COUNT) {
        return false;
    }

    if (descriptor->maximum_arity !=
            VITTE_BUILTIN_VARIADIC_ARITY &&
        descriptor->minimum_arity >
            descriptor->maximum_arity) {
        return false;
    }

    if ((descriptor->flags &
         VITTE_BUILTIN_FLAG_CONST) != 0u &&
        (descriptor->flags &
         VITTE_BUILTIN_FLAG_PURE) == 0u) {
        return false;
    }

    if ((descriptor->flags &
         VITTE_BUILTIN_FLAG_ATOMIC) != 0u &&
        descriptor->category !=
            VITTE_BUILTIN_CATEGORY_ATOMIC) {
        return false;
    }

    return true;
}

bool
vitte_builtin_registry_validate(void)
{
    size_t count;
    size_t left;

    count =
        vitte_builtin_registry_count_internal();

    if (count !=
        (size_t)VITTE_BUILTIN_COUNT - 1u) {
        return false;
    }

    for (left = 0u;
         left < count;
         ++left) {
        size_t right;

        if (!vitte_builtin_descriptor_valid(
                &vitte_builtin_registry[left])) {
            return false;
        }

        for (right = left + 1u;
             right < count;
             ++right) {
            if (vitte_builtin_registry[left].id ==
                vitte_builtin_registry[right].id) {
                return false;
            }

            if (strcmp(
                    vitte_builtin_registry[left].name,
                    vitte_builtin_registry[right].name) == 0) {
                return false;
            }
        }
    }

    /*
     * Ensure every semantic ID is represented exactly once.
     */
    for (left = 1u;
         left < (size_t)VITTE_BUILTIN_COUNT;
         ++left) {
        size_t matches;
        size_t index;

        matches = 0u;

        for (index = 0u;
             index < count;
             ++index) {
            if ((size_t)vitte_builtin_registry[index].id ==
                left) {
                ++matches;
            }
        }

        if (matches != 1u) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Registry fingerprint                                                      */
/* ========================================================================= */

uint64_t
vitte_builtin_registry_hash(void)
{
    uint64_t hash;
    size_t index;

    hash = VITTE_BUILTIN_FNV_OFFSET;

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        const vitte_builtin_descriptor_t *descriptor;

        descriptor =
            &vitte_builtin_registry[index];

        hash =
            vitte_builtin_hash_u64(
                hash,
                (uint64_t)descriptor->id);

        hash =
            vitte_builtin_hash_string(
                hash,
                descriptor->name);

        hash =
            vitte_builtin_hash_string(
                hash,
                descriptor->description);

        hash =
            vitte_builtin_hash_u64(
                hash,
                (uint64_t)descriptor->category);

        hash =
            vitte_builtin_hash_u64(
                hash,
                (uint64_t)descriptor->minimum_arity);

        hash =
            vitte_builtin_hash_u64(
                hash,
                descriptor->maximum_arity ==
                        VITTE_BUILTIN_VARIADIC_ARITY
                    ? UINT64_MAX
                    : (uint64_t)descriptor->maximum_arity);

        hash =
            vitte_builtin_hash_u64(
                hash,
                (uint64_t)descriptor->flags);
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_builtin_stats_t
vitte_builtin_stats(void)
{
    vitte_builtin_stats_t stats;
    size_t index;

    memset(
        &stats,
        0,
        sizeof(stats));

    stats.builtin_count =
        vitte_builtin_registry_count_internal();

    for (index = 0u;
         index < stats.builtin_count;
         ++index) {
        const vitte_builtin_descriptor_t *descriptor;

        descriptor =
            &vitte_builtin_registry[index];

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_PURE)) {
            ++stats.pure_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_CONST)) {
            ++stats.const_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_UNSAFE)) {
            ++stats.unsafe_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_NORETURN)) {
            ++stats.noreturn_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_MEMORY_READ)) {
            ++stats.memory_read_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_MEMORY_WRITE)) {
            ++stats.memory_write_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_ATOMIC)) {
            ++stats.atomic_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_TARGET_DEPENDENT)) {
            ++stats.target_dependent_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_COMPTIME)) {
            ++stats.comptime_count;
        }

        if (vitte_builtin_has_flag(
                descriptor,
                VITTE_BUILTIN_FLAG_LOWER_DIRECTLY)) {
            ++stats.directly_lowered_count;
        }
    }

    stats.registry_hash =
        vitte_builtin_registry_hash();

    return stats;
}

/* ========================================================================= */
/* ID helpers                                                                */
/* ========================================================================= */

const char *
vitte_builtin_id_name(
    vitte_builtin_id_t id)
{
    const vitte_builtin_descriptor_t *descriptor;

    if (id == VITTE_BUILTIN_INVALID) {
        return "invalid";
    }

    if (id == VITTE_BUILTIN_COUNT) {
        return "count";
    }

    descriptor =
        vitte_builtin_find_by_id(id);

    if (descriptor == NULL) {
        return "invalid";
    }

    return descriptor->name;
}

/* ========================================================================= */
/* Category statistics                                                       */
/* ========================================================================= */

size_t
vitte_builtin_count_category(
    vitte_builtin_category_t category)
{
    size_t count;
    size_t index;

    if (category <=
            VITTE_BUILTIN_CATEGORY_INVALID ||
        category >=
            VITTE_BUILTIN_CATEGORY_COUNT) {
        return 0u;
    }

    count = 0u;

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        if (vitte_builtin_registry[index].category ==
            category) {
            ++count;
        }
    }

    return count;
}

/* ========================================================================= */
/* Flag statistics                                                           */
/* ========================================================================= */

size_t
vitte_builtin_count_flag(
    vitte_builtin_flag_t flag)
{
    size_t count;
    size_t index;

    count = 0u;

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        if (vitte_builtin_has_flag(
                &vitte_builtin_registry[index],
                flag)) {
            ++count;
        }
    }

    return count;
}

/* ========================================================================= */
/* Deterministic iteration                                                   */
/* ========================================================================= */

typedef bool
(*vitte_builtin_visit_fn)(
    const vitte_builtin_descriptor_t *descriptor,
    void *user_data);

bool
vitte_builtin_visit(
    vitte_builtin_visit_fn visitor,
    void *user_data)
{
    size_t index;

    if (visitor == NULL) {
        return false;
    }

    for (index = 0u;
         index < vitte_builtin_registry_count_internal();
         ++index) {
        if (!visitor(
                &vitte_builtin_registry[index],
                user_data)) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
#endif
