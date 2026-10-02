#ifndef VITTE_TYPE_TYPE_H
#define VITTE_TYPE_TYPE_H

/*
 * Vitte Compiler
 * src/type/type.h
 *
 * Canonical public type-system contract.
 *
 * This header is synchronized with type.c.
 *
 * Design:
 *   - stable one-based type IDs;
 *   - canonical builtin types;
 *   - structural type interning;
 *   - pointer/reference/array/slice/range types;
 *   - tuples;
 *   - function types;
 *   - nominal/named types;
 *   - generic parameters;
 *   - dyn trait types;
 *   - deterministic structural hashes;
 *   - compatibility and assignment rules;
 *   - numeric promotion;
 *   - bounded resources;
 *   - validation;
 *   - deterministic fingerprints;
 *   - no dependency on sema.h or symbol.h.
 *
 * ISO C17 / C++ compatible public header.
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

#define VITTE_TYPE_API_VERSION_MAJOR 1u
#define VITTE_TYPE_API_VERSION_MINOR 0u
#define VITTE_TYPE_API_VERSION_PATCH 0u

#define VITTE_TYPE_API_VERSION \
    ((VITTE_TYPE_API_VERSION_MAJOR * 10000u) + \
     (VITTE_TYPE_API_VERSION_MINOR * 100u) + \
     VITTE_TYPE_API_VERSION_PATCH)

/* ========================================================================= */
/* Magic                                                                     */
/* ========================================================================= */

#define VITTE_TYPE_MAGIC \
    UINT64_C(0x5649545454595045)

#define VITTE_TYPE_DEAD_MAGIC \
    UINT64_C(0x4445414454595045)

/* ========================================================================= */
/* Hashing                                                                   */
/* ========================================================================= */

#define VITTE_TYPE_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_TYPE_FNV_PRIME \
    UINT64_C(1099511628211)

/* ========================================================================= */
/* Resource defaults                                                         */
/* ========================================================================= */

#define VITTE_TYPE_DEFAULT_INITIAL_CAPACITY \
    ((size_t)128u)

#define VITTE_TYPE_DEFAULT_INITIAL_PARAMETER_CAPACITY \
    ((size_t)256u)

#define VITTE_TYPE_DEFAULT_MAX_TYPES \
    ((size_t)16777216u)

#define VITTE_TYPE_DEFAULT_MAX_PARAMETERS \
    ((size_t)67108864u)

#define VITTE_TYPE_DEFAULT_MAX_COMPARISON_DEPTH \
    ((size_t)4096u)

/* ========================================================================= */
/* IDs                                                                       */
/* ========================================================================= */

typedef uint64_t vitte_type_id_t;

/*
 * Symbol IDs deliberately remain opaque here.
 *
 * type.h must not include symbol.h because symbol.h may itself need to refer
 * to type IDs. Keeping the representation at uint64_t prevents a dependency
 * cycle between the semantic symbol and type layers.
 */
typedef uint64_t vitte_type_symbol_id_t;

#define VITTE_TYPE_INVALID_ID \
    ((vitte_type_id_t)UINT64_C(0))

#define VITTE_TYPE_INVALID_SYMBOL_ID \
    ((vitte_type_symbol_id_t)UINT64_C(0))

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_type_error {
    VITTE_TYPE_ERROR_NONE = 0,

    VITTE_TYPE_ERROR_INVALID_ARGUMENT,
    VITTE_TYPE_ERROR_INVALID_CONTEXT,
    VITTE_TYPE_ERROR_INVALID_STATE,

    VITTE_TYPE_ERROR_INVALID_TYPE,
    VITTE_TYPE_ERROR_INVALID_TYPE_ID,
    VITTE_TYPE_ERROR_INVALID_KIND,
    VITTE_TYPE_ERROR_INVALID_DESCRIPTOR,

    VITTE_TYPE_ERROR_TYPE_LIMIT,
    VITTE_TYPE_ERROR_PARAMETER_LIMIT,
    VITTE_TYPE_ERROR_COMPARISON_DEPTH,

    VITTE_TYPE_ERROR_OVERFLOW,
    VITTE_TYPE_ERROR_OUT_OF_MEMORY,

    VITTE_TYPE_ERROR_VALIDATION,
    VITTE_TYPE_ERROR_CORRUPTION,

    VITTE_TYPE_ERROR_INTERNAL,

    VITTE_TYPE_ERROR_COUNT
} vitte_type_error_t;

/* ========================================================================= */
/* Context state                                                             */
/* ========================================================================= */

typedef enum vitte_type_state {
    VITTE_TYPE_STATE_INVALID = 0,

    VITTE_TYPE_STATE_READY,
    VITTE_TYPE_STATE_BUILDING,
    VITTE_TYPE_STATE_FROZEN,
    VITTE_TYPE_STATE_FAILED,
    VITTE_TYPE_STATE_DESTROYED,

    VITTE_TYPE_STATE_COUNT
} vitte_type_state_t;

/* ========================================================================= */
/* Type kinds                                                                */
/* ========================================================================= */

typedef enum vitte_type_kind {
    VITTE_TYPE_KIND_INVALID = 0,

    /*
     * Recovery / control types.
     */
    VITTE_TYPE_KIND_ERROR,
    VITTE_TYPE_KIND_VOID,
    VITTE_TYPE_KIND_NEVER,

    /*
     * Primitive scalar types.
     */
    VITTE_TYPE_KIND_BOOL,

    VITTE_TYPE_KIND_I8,
    VITTE_TYPE_KIND_I16,
    VITTE_TYPE_KIND_I32,
    VITTE_TYPE_KIND_I64,
    VITTE_TYPE_KIND_ISIZE,

    VITTE_TYPE_KIND_U8,
    VITTE_TYPE_KIND_U16,
    VITTE_TYPE_KIND_U32,
    VITTE_TYPE_KIND_U64,
    VITTE_TYPE_KIND_USIZE,

    VITTE_TYPE_KIND_F32,
    VITTE_TYPE_KIND_F64,

    VITTE_TYPE_KIND_CHAR,
    VITTE_TYPE_KIND_STRING,

    /*
     * Null literal type.
     */
    VITTE_TYPE_KIND_NULL,

    /*
     * Constructed types.
     */
    VITTE_TYPE_KIND_POINTER,
    VITTE_TYPE_KIND_REFERENCE,
    VITTE_TYPE_KIND_ARRAY,
    VITTE_TYPE_KIND_SLICE,
    VITTE_TYPE_KIND_RANGE,
    VITTE_TYPE_KIND_TUPLE,
    VITTE_TYPE_KIND_FUNCTION,

    /*
     * Nominal declaration type.
     *
     * form/pick/opaque/type declarations are represented by their semantic
     * symbol identity instead of duplicating every language declaration kind
     * in the low-level type system.
     */
    VITTE_TYPE_KIND_NAMED,

    /*
     * Generic type parameter.
     */
    VITTE_TYPE_KIND_GENERIC_PARAMETER,

    /*
     * Dynamic trait object.
     */
    VITTE_TYPE_KIND_DYN,

    VITTE_TYPE_KIND_COUNT
} vitte_type_kind_t;

/* ========================================================================= */
/* Type flags                                                                */
/* ========================================================================= */

typedef uint64_t vitte_type_flags_t;

#define VITTE_TYPE_FLAG_NONE \
    UINT64_C(0)

/*
 * Pointer/reference/slice mutability.
 */
#define VITTE_TYPE_FLAG_MUTABLE \
    (UINT64_C(1) << 0)

/*
 * Function metadata.
 */
#define VITTE_TYPE_FLAG_VARIADIC \
    (UINT64_C(1) << 1)

#define VITTE_TYPE_FLAG_ASYNC \
    (UINT64_C(1) << 2)

#define VITTE_TYPE_FLAG_UNSAFE \
    (UINT64_C(1) << 3)

/*
 * Inclusive range: a..=b.
 */
#define VITTE_TYPE_FLAG_INCLUSIVE \
    (UINT64_C(1) << 4)

/*
 * Compiler-provided primitive/canonical type.
 */
#define VITTE_TYPE_FLAG_BUILTIN \
    (UINT64_C(1) << 5)

#define VITTE_TYPE_FLAG_CANONICAL \
    (UINT64_C(1) << 6)

/*
 * Generic semantic type.
 */
#define VITTE_TYPE_FLAG_GENERIC \
    (UINT64_C(1) << 7)

/*
 * Reserved semantic metadata bits.
 */
#define VITTE_TYPE_FLAG_SYNTHETIC \
    (UINT64_C(1) << 8)

#define VITTE_TYPE_FLAG_INFERRED \
    (UINT64_C(1) << 9)

#define VITTE_TYPE_FLAG_OPAQUE \
    (UINT64_C(1) << 10)

/* ========================================================================= */
/* Type descriptor                                                           */
/* ========================================================================= */

/*
 * Temporary structural description used when interning a type.
 *
 * All pointers are borrowed for the duration of vitte_type_intern().
 * The type context copies data that must outlive the call.
 */
typedef struct vitte_type_descriptor {
    vitte_type_kind_t kind;
    vitte_type_flags_t flags;

    /*
     * Used by:
     *   pointer
     *   reference
     *   array
     *   slice
     *   range
     */
    vitte_type_id_t element_type;

    /*
     * Function return type.
     */
    vitte_type_id_t return_type;

    /*
     * Array element count.
     */
    size_t array_length;

    /*
     * Nominal declaration / generic parameter / trait identity.
     */
    vitte_type_symbol_id_t symbol_id;

    /*
     * Generic parameter position.
     */
    size_t generic_index;

    /*
     * Optional human-readable / nominal name.
     */
    const char *name;
    size_t name_length;

    /*
     * Tuple elements or function parameters.
     */
    const vitte_type_id_t *parameters;
    size_t parameter_count;
} vitte_type_descriptor_t;

/* ========================================================================= */
/* Canonical type                                                            */
/* ========================================================================= */

typedef struct vitte_type {
    /*
     * Stable one-based identity.
     */
    vitte_type_id_t id;

    vitte_type_kind_t kind;
    vitte_type_flags_t flags;

    /*
     * Element type for pointer/reference/array/slice/range.
     */
    vitte_type_id_t element_type;

    /*
     * Function return type.
     */
    vitte_type_id_t return_type;

    /*
     * Array length.
     */
    size_t array_length;

    /*
     * Semantic declaration identity.
     */
    vitte_type_symbol_id_t symbol_id;

    /*
     * Generic parameter index.
     */
    size_t generic_index;

    /*
     * Owned NUL-terminated name.
     *
     * name_length excludes the terminator.
     */
    char *name;
    size_t name_length;

    /*
     * Owned canonical parameter list.
     *
     * Used for:
     *   tuples;
     *   functions.
     */
    vitte_type_id_t *parameters;
    size_t parameter_count;

    /*
     * Offset into context->parameters.
     *
     * The flat parameter arena mirrors the owned parameter list and provides
     * deterministic serialization/introspection.
     */
    size_t parameter_offset;

    /*
     * Structural hash of this type's canonical descriptor.
     */
    uint64_t structural_hash;
} vitte_type_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_type_stats {
    uint64_t types_created;
    uint64_t intern_hits;

    uint64_t allocations;
    uint64_t reallocations;
    uint64_t allocation_failures;

    uint64_t validation_runs;
    uint64_t validation_failures;

    uint64_t hash_runs;

    uint64_t resets;
    uint64_t freezes;
    uint64_t failures;
} vitte_type_stats_t;

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

typedef struct vitte_type_context {
    uint64_t magic;

    vitte_type_state_t state;
    vitte_type_error_t last_error;

    /*
     * Canonical type arena.
     *
     * ID N is stored at:
     *
     *   types[N - 1]
     */
    vitte_type_t *types;
    size_t type_count;
    size_t type_capacity;

    /*
     * Flat canonical parameter arena.
     */
    vitte_type_id_t *parameters;
    size_t parameter_count;
    size_t parameter_capacity;

    /*
     * Resource limits.
     */
    size_t max_types;
    size_t max_parameters;

    /*
     * Canonical builtin IDs.
     */
    vitte_type_id_t type_error;
    vitte_type_id_t type_void;
    vitte_type_id_t type_never;
    vitte_type_id_t type_bool;

    vitte_type_id_t type_i8;
    vitte_type_id_t type_i16;
    vitte_type_id_t type_i32;
    vitte_type_id_t type_i64;
    vitte_type_id_t type_isize;

    vitte_type_id_t type_u8;
    vitte_type_id_t type_u16;
    vitte_type_id_t type_u32;
    vitte_type_id_t type_u64;
    vitte_type_id_t type_usize;

    vitte_type_id_t type_f32;
    vitte_type_id_t type_f64;

    vitte_type_id_t type_char;
    vitte_type_id_t type_string;
    vitte_type_id_t type_null;

    vitte_type_stats_t stats;

    uint64_t generation;
} vitte_type_context_t;

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_type_error_name(
    vitte_type_error_t error);

const char *
vitte_type_state_name(
    vitte_type_state_t state);

const char *
vitte_type_kind_name(
    vitte_type_kind_t kind);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_type_init(
    vitte_type_context_t *context);

bool
vitte_type_reset(
    vitte_type_context_t *context);

void
vitte_type_destroy(
    vitte_type_context_t *context);

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

bool
vitte_type_is_valid(
    const vitte_type_context_t *context);

vitte_type_error_t
vitte_type_last_error(
    const vitte_type_context_t *context);

uint64_t
vitte_type_generation(
    const vitte_type_context_t *context);

bool
vitte_type_set_limits(
    vitte_type_context_t *context,
    size_t max_types,
    size_t max_parameters);

bool
vitte_type_freeze(
    vitte_type_context_t *context);

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

const vitte_type_t *
vitte_type_get(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

/* ========================================================================= */
/* Generic interning                                                         */
/* ========================================================================= */

bool
vitte_type_intern(
    vitte_type_context_t *context,
    const vitte_type_descriptor_t *descriptor,
    vitte_type_id_t *out_type_id);

/* ========================================================================= */
/* Constructed types                                                         */
/* ========================================================================= */

bool
vitte_type_pointer(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    bool is_mutable,
    vitte_type_id_t *out_type_id);

bool
vitte_type_reference(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    bool is_mutable,
    vitte_type_id_t *out_type_id);

bool
vitte_type_array(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    size_t array_length,
    vitte_type_id_t *out_type_id);

bool
vitte_type_slice(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    bool is_mutable,
    vitte_type_id_t *out_type_id);

bool
vitte_type_range(
    vitte_type_context_t *context,
    vitte_type_id_t element_type,
    bool inclusive,
    vitte_type_id_t *out_type_id);

bool
vitte_type_tuple(
    vitte_type_context_t *context,
    const vitte_type_id_t *elements,
    size_t element_count,
    vitte_type_id_t *out_type_id);

bool
vitte_type_function(
    vitte_type_context_t *context,
    const vitte_type_id_t *parameters,
    size_t parameter_count,
    vitte_type_id_t return_type,
    bool variadic,
    bool is_async,
    bool is_unsafe,
    vitte_type_id_t *out_type_id);

bool
vitte_type_named(
    vitte_type_context_t *context,
    uint64_t symbol_id,
    const char *name,
    size_t name_length,
    vitte_type_id_t *out_type_id);

bool
vitte_type_generic_parameter(
    vitte_type_context_t *context,
    uint64_t symbol_id,
    const char *name,
    size_t name_length,
    size_t generic_index,
    vitte_type_id_t *out_type_id);

bool
vitte_type_dyn(
    vitte_type_context_t *context,
    uint64_t trait_symbol_id,
    const char *name,
    size_t name_length,
    vitte_type_id_t *out_type_id);

/* ========================================================================= */
/* Classification                                                            */
/* ========================================================================= */

bool
vitte_type_is_error(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_void(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_never(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_bool(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_integer(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_signed_integer(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_unsigned_integer(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_float(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_numeric(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_scalar(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_pointer_like(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_sequence(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_callable(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

bool
vitte_type_is_aggregate(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

/* ========================================================================= */
/* Equality                                                                  */
/* ========================================================================= */

bool
vitte_type_equal(
    const vitte_type_context_t *context,
    vitte_type_id_t left,
    vitte_type_id_t right);

bool
vitte_type_structural_equal(
    const vitte_type_context_t *context,
    vitte_type_id_t left,
    vitte_type_id_t right);

/* ========================================================================= */
/* Compatibility                                                             */
/* ========================================================================= */

bool
vitte_type_assignable(
    const vitte_type_context_t *context,
    vitte_type_id_t destination_id,
    vitte_type_id_t source_id);

bool
vitte_type_compatible(
    const vitte_type_context_t *context,
    vitte_type_id_t left,
    vitte_type_id_t right);

/* ========================================================================= */
/* Numeric promotion                                                         */
/* ========================================================================= */

vitte_type_id_t
vitte_type_common_numeric(
    const vitte_type_context_t *context,
    vitte_type_id_t left_id,
    vitte_type_id_t right_id);

/* ========================================================================= */
/* Type properties                                                           */
/* ========================================================================= */

vitte_type_id_t
vitte_type_element(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

vitte_type_id_t
vitte_type_return(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

size_t
vitte_type_parameter_count(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id);

vitte_type_id_t
vitte_type_parameter(
    const vitte_type_context_t *context,
    vitte_type_id_t type_id,
    size_t index);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_type_validate(
    vitte_type_context_t *context);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_type_fingerprint(
    vitte_type_context_t *context);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_type_stats_t
vitte_type_stats(
    const vitte_type_context_t *context);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_type_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline API version                                                        */
/* ========================================================================= */

static inline unsigned int
vitte_type_api_version_major(void)
{
    return VITTE_TYPE_API_VERSION_MAJOR;
}

static inline unsigned int
vitte_type_api_version_minor(void)
{
    return VITTE_TYPE_API_VERSION_MINOR;
}

static inline unsigned int
vitte_type_api_version_patch(void)
{
    return VITTE_TYPE_API_VERSION_PATCH;
}

static inline unsigned int
vitte_type_api_version(void)
{
    return VITTE_TYPE_API_VERSION;
}

/* ========================================================================= */
/* Inline ID helpers                                                         */
/* ========================================================================= */

static inline bool
vitte_type_id_is_valid(
    vitte_type_id_t type_id)
{
    return type_id != VITTE_TYPE_INVALID_ID;
}

static inline bool
vitte_type_symbol_id_is_valid(
    vitte_type_symbol_id_t symbol_id)
{
    return symbol_id !=
           VITTE_TYPE_INVALID_SYMBOL_ID;
}

/* ========================================================================= */
/* Inline enum helpers                                                       */
/* ========================================================================= */

static inline bool
vitte_type_error_is_valid(
    vitte_type_error_t error)
{
    return error >=
               VITTE_TYPE_ERROR_NONE &&
           error <
               VITTE_TYPE_ERROR_COUNT;
}

static inline bool
vitte_type_state_is_valid(
    vitte_type_state_t state)
{
    return state >
               VITTE_TYPE_STATE_INVALID &&
           state <
               VITTE_TYPE_STATE_COUNT;
}

static inline bool
vitte_type_kind_is_valid(
    vitte_type_kind_t kind)
{
    return kind >
               VITTE_TYPE_KIND_INVALID &&
           kind <
               VITTE_TYPE_KIND_COUNT;
}

/* ========================================================================= */
/* Inline context state helpers                                              */
/* ========================================================================= */

static inline bool
vitte_type_is_ready(
    const vitte_type_context_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_TYPE_MAGIC &&
           context->state ==
               VITTE_TYPE_STATE_READY;
}

static inline bool
vitte_type_is_building(
    const vitte_type_context_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_TYPE_MAGIC &&
           context->state ==
               VITTE_TYPE_STATE_BUILDING;
}

static inline bool
vitte_type_is_frozen(
    const vitte_type_context_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_TYPE_MAGIC &&
           context->state ==
               VITTE_TYPE_STATE_FROZEN;
}

static inline bool
vitte_type_has_failed(
    const vitte_type_context_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_TYPE_MAGIC &&
           context->state ==
               VITTE_TYPE_STATE_FAILED;
}

static inline bool
vitte_type_is_destroyed(
    const vitte_type_context_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_TYPE_DEAD_MAGIC &&
           context->state ==
               VITTE_TYPE_STATE_DESTROYED;
}

/* ========================================================================= */
/* Inline counts                                                             */
/* ========================================================================= */

static inline size_t
vitte_type_count(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_count
        : 0u;
}

static inline bool
vitte_type_empty(
    const vitte_type_context_t *context)
{
    return context == NULL ||
           context->type_count == 0u;
}

/* ========================================================================= */
/* Inline type-entry helpers                                                 */
/* ========================================================================= */

static inline bool
vitte_type_entry_is_valid(
    const vitte_type_t *type)
{
    return type != NULL &&
           type->id !=
               VITTE_TYPE_INVALID_ID &&
           vitte_type_kind_is_valid(
               type->kind);
}

static inline bool
vitte_type_has_flag(
    const vitte_type_t *type,
    vitte_type_flags_t flag)
{
    return type != NULL &&
           (type->flags & flag) !=
               VITTE_TYPE_FLAG_NONE;
}

static inline bool
vitte_type_is_mutable_entry(
    const vitte_type_t *type)
{
    return vitte_type_has_flag(
        type,
        VITTE_TYPE_FLAG_MUTABLE);
}

static inline bool
vitte_type_is_variadic_entry(
    const vitte_type_t *type)
{
    return vitte_type_has_flag(
        type,
        VITTE_TYPE_FLAG_VARIADIC);
}

static inline bool
vitte_type_is_async_entry(
    const vitte_type_t *type)
{
    return vitte_type_has_flag(
        type,
        VITTE_TYPE_FLAG_ASYNC);
}

static inline bool
vitte_type_is_unsafe_entry(
    const vitte_type_t *type)
{
    return vitte_type_has_flag(
        type,
        VITTE_TYPE_FLAG_UNSAFE);
}

static inline bool
vitte_type_is_builtin_entry(
    const vitte_type_t *type)
{
    return vitte_type_has_flag(
        type,
        VITTE_TYPE_FLAG_BUILTIN);
}

static inline bool
vitte_type_is_canonical_entry(
    const vitte_type_t *type)
{
    return vitte_type_has_flag(
        type,
        VITTE_TYPE_FLAG_CANONICAL);
}

static inline bool
vitte_type_is_generic_entry(
    const vitte_type_t *type)
{
    return vitte_type_has_flag(
        type,
        VITTE_TYPE_FLAG_GENERIC);
}

static inline bool
vitte_type_range_is_inclusive(
    const vitte_type_t *type)
{
    return type != NULL &&
           type->kind ==
               VITTE_TYPE_KIND_RANGE &&
           vitte_type_has_flag(
               type,
               VITTE_TYPE_FLAG_INCLUSIVE);
}

static inline bool
vitte_type_has_element(
    const vitte_type_t *type)
{
    return type != NULL &&
           type->element_type !=
               VITTE_TYPE_INVALID_ID;
}

static inline bool
vitte_type_has_return_type(
    const vitte_type_t *type)
{
    return type != NULL &&
           type->return_type !=
               VITTE_TYPE_INVALID_ID;
}

static inline bool
vitte_type_has_name(
    const vitte_type_t *type)
{
    return type != NULL &&
           type->name != NULL &&
           type->name_length != 0u;
}

static inline bool
vitte_type_has_symbol(
    const vitte_type_t *type)
{
    return type != NULL &&
           type->symbol_id !=
               VITTE_TYPE_INVALID_SYMBOL_ID;
}

static inline bool
vitte_type_has_parameters(
    const vitte_type_t *type)
{
    return type != NULL &&
           type->parameter_count != 0u &&
           type->parameters != NULL;
}

/* ========================================================================= */
/* Inline kind classification                                                */
/* ========================================================================= */

static inline bool
vitte_type_kind_is_signed_integer(
    vitte_type_kind_t kind)
{
    switch (kind) {
        case VITTE_TYPE_KIND_I8:
        case VITTE_TYPE_KIND_I16:
        case VITTE_TYPE_KIND_I32:
        case VITTE_TYPE_KIND_I64:
        case VITTE_TYPE_KIND_ISIZE:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_type_kind_is_unsigned_integer(
    vitte_type_kind_t kind)
{
    switch (kind) {
        case VITTE_TYPE_KIND_U8:
        case VITTE_TYPE_KIND_U16:
        case VITTE_TYPE_KIND_U32:
        case VITTE_TYPE_KIND_U64:
        case VITTE_TYPE_KIND_USIZE:
            return true;

        default:
            return false;
    }
}

static inline bool
vitte_type_kind_is_integer(
    vitte_type_kind_t kind)
{
    return vitte_type_kind_is_signed_integer(
               kind) ||
           vitte_type_kind_is_unsigned_integer(
               kind);
}

static inline bool
vitte_type_kind_is_float(
    vitte_type_kind_t kind)
{
    return kind == VITTE_TYPE_KIND_F32 ||
           kind == VITTE_TYPE_KIND_F64;
}

static inline bool
vitte_type_kind_is_numeric(
    vitte_type_kind_t kind)
{
    return vitte_type_kind_is_integer(
               kind) ||
           vitte_type_kind_is_float(
               kind);
}

static inline bool
vitte_type_kind_is_pointer_like(
    vitte_type_kind_t kind)
{
    return kind ==
               VITTE_TYPE_KIND_POINTER ||
           kind ==
               VITTE_TYPE_KIND_REFERENCE;
}

static inline bool
vitte_type_kind_is_sequence(
    vitte_type_kind_t kind)
{
    return kind ==
               VITTE_TYPE_KIND_ARRAY ||
           kind ==
               VITTE_TYPE_KIND_SLICE ||
           kind ==
               VITTE_TYPE_KIND_STRING;
}

static inline bool
vitte_type_kind_is_aggregate(
    vitte_type_kind_t kind)
{
    switch (kind) {
        case VITTE_TYPE_KIND_ARRAY:
        case VITTE_TYPE_KIND_SLICE:
        case VITTE_TYPE_KIND_TUPLE:
        case VITTE_TYPE_KIND_NAMED:
        case VITTE_TYPE_KIND_DYN:
        case VITTE_TYPE_KIND_STRING:
            return true;

        default:
            return false;
    }
}

/* ========================================================================= */
/* Builtin accessors                                                         */
/* ========================================================================= */

static inline vitte_type_id_t
vitte_type_builtin_error(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_error
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_void(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_void
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_never(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_never
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_bool(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_bool
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_i8(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_i8
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_i16(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_i16
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_i32(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_i32
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_i64(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_i64
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_isize(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_isize
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_u8(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_u8
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_u16(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_u16
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_u32(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_u32
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_u64(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_u64
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_usize(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_usize
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_f32(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_f32
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_f64(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_f64
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_char(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_char
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_string(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_string
        : VITTE_TYPE_INVALID_ID;
}

static inline vitte_type_id_t
vitte_type_builtin_null(
    const vitte_type_context_t *context)
{
    return context != NULL
        ? context->type_null
        : VITTE_TYPE_INVALID_ID;
}

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__cplusplus)

static_assert(
    sizeof(vitte_type_id_t) ==
        sizeof(uint64_t),
    "vitte_type_id_t must be 64-bit");

static_assert(
    sizeof(vitte_type_symbol_id_t) ==
        sizeof(uint64_t),
    "vitte_type_symbol_id_t must be 64-bit");

static_assert(
    VITTE_TYPE_INVALID_ID ==
        UINT64_C(0),
    "invalid type ID must be zero");

static_assert(
    VITTE_TYPE_INVALID_SYMBOL_ID ==
        UINT64_C(0),
    "invalid type symbol ID must be zero");

static_assert(
    VITTE_TYPE_KIND_INVALID == 0,
    "invalid type kind must be zero");

static_assert(
    VITTE_TYPE_STATE_INVALID == 0,
    "invalid type state must be zero");

static_assert(
    VITTE_TYPE_ERROR_NONE == 0,
    "type error NONE must be zero");

#else

_Static_assert(
    sizeof(vitte_type_id_t) ==
        sizeof(uint64_t),
    "vitte_type_id_t must be 64-bit");

_Static_assert(
    sizeof(vitte_type_symbol_id_t) ==
        sizeof(uint64_t),
    "vitte_type_symbol_id_t must be 64-bit");

_Static_assert(
    VITTE_TYPE_INVALID_ID ==
        UINT64_C(0),
    "invalid type ID must be zero");

_Static_assert(
    VITTE_TYPE_INVALID_SYMBOL_ID ==
        UINT64_C(0),
    "invalid type symbol ID must be zero");

_Static_assert(
    VITTE_TYPE_KIND_INVALID == 0,
    "invalid type kind must be zero");

_Static_assert(
    VITTE_TYPE_STATE_INVALID == 0,
    "invalid type state must be zero");

_Static_assert(
    VITTE_TYPE_ERROR_NONE == 0,
    "type error NONE must be zero");

#endif

#ifdef __cplusplus
}
#endif

#endif /* VITTE_TYPE_TYPE_H */
