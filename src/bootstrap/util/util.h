#ifndef VITTE_UTIL_UTIL_H
#define VITTE_UTIL_UTIL_H

/*
 * Vitte Compiler
 * src/util/util.h
 *
 * General-purpose low-level compiler utilities.
 *
 * Goals:
 *   - ISO C17;
 *   - deterministic behavior;
 *   - overflow-safe arithmetic;
 *   - allocation helpers;
 *   - byte/string helpers;
 *   - hashing;
 *   - dynamic byte buffers;
 *   - path-independent generic utilities;
 *   - sorting/searching helpers;
 *   - endian encoding/decoding;
 *   - bit/alignment helpers;
 *   - no dependency on another Vitte subsystem.
 *
 * util.c imports this file as its unique public contract.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* API                                                                       */
/* ========================================================================= */

#define VITTE_UTIL_API_VERSION_MAJOR 1u
#define VITTE_UTIL_API_VERSION_MINOR 0u
#define VITTE_UTIL_API_VERSION_PATCH 0u

#define VITTE_UTIL_API_VERSION \
    ((VITTE_UTIL_API_VERSION_MAJOR * 10000u) + \
     (VITTE_UTIL_API_VERSION_MINOR * 100u) + \
     VITTE_UTIL_API_VERSION_PATCH)

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

#define VITTE_UTIL_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_UTIL_FNV_PRIME \
    UINT64_C(1099511628211)

/* ========================================================================= */
/* Limits/defaults                                                           */
/* ========================================================================= */

#define VITTE_UTIL_DEFAULT_BUFFER_CAPACITY \
    ((size_t)256u)

#define VITTE_UTIL_DEFAULT_VECTOR_CAPACITY \
    ((size_t)16u)

#define VITTE_UTIL_DEFAULT_GROWTH_NUMERATOR \
    ((size_t)2u)

#define VITTE_UTIL_DEFAULT_GROWTH_DENOMINATOR \
    ((size_t)1u)

/* ========================================================================= */
/* Error                                                                     */
/* ========================================================================= */

typedef enum vitte_util_error {
    VITTE_UTIL_ERROR_NONE = 0,

    VITTE_UTIL_ERROR_INVALID_ARGUMENT,
    VITTE_UTIL_ERROR_INVALID_STATE,

    VITTE_UTIL_ERROR_OVERFLOW,
    VITTE_UTIL_ERROR_OUT_OF_MEMORY,

    VITTE_UTIL_ERROR_OUT_OF_RANGE,
    VITTE_UTIL_ERROR_BUFFER_TOO_SMALL,

    VITTE_UTIL_ERROR_INVALID_FORMAT,
    VITTE_UTIL_ERROR_INTERNAL,

    VITTE_UTIL_ERROR_COUNT
} vitte_util_error_t;

/* ========================================================================= */
/* Byte/string views                                                         */
/* ========================================================================= */

typedef struct vitte_util_bytes {
    const unsigned char *data;
    size_t length;
} vitte_util_bytes_t;

typedef struct vitte_util_string_view {
    const char *data;
    size_t length;
} vitte_util_string_view_t;

/* ========================================================================= */
/* Mutable byte buffer                                                       */
/* ========================================================================= */

typedef struct vitte_util_buffer {
    unsigned char *data;
    size_t length;
    size_t capacity;
} vitte_util_buffer_t;

/* ========================================================================= */
/* Generic vector                                                            */
/* ========================================================================= */

typedef struct vitte_util_vector {
    unsigned char *data;

    size_t count;
    size_t capacity;
    size_t element_size;
} vitte_util_vector_t;

/* ========================================================================= */
/* Comparator                                                                */
/* ========================================================================= */

typedef int (*vitte_util_compare_fn)(
    const void *left,
    const void *right,
    void *user_data);

/* ========================================================================= */
/* Error names                                                               */
/* ========================================================================= */

const char *
vitte_util_error_name(
    vitte_util_error_t error);

/* ========================================================================= */
/* Checked arithmetic                                                        */
/* ========================================================================= */

bool
vitte_util_add_size(
    size_t left,
    size_t right,
    size_t *out);

bool
vitte_util_sub_size(
    size_t left,
    size_t right,
    size_t *out);

bool
vitte_util_mul_size(
    size_t left,
    size_t right,
    size_t *out);

bool
vitte_util_add_u64(
    uint64_t left,
    uint64_t right,
    uint64_t *out);

bool
vitte_util_mul_u64(
    uint64_t left,
    uint64_t right,
    uint64_t *out);

/* ========================================================================= */
/* Alignment                                                                 */
/* ========================================================================= */

bool
vitte_util_is_power_of_two_size(
    size_t value);

bool
vitte_util_align_up_size(
    size_t value,
    size_t alignment,
    size_t *out);

bool
vitte_util_align_down_size(
    size_t value,
    size_t alignment,
    size_t *out);

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

bool
vitte_util_next_capacity(
    size_t current,
    size_t required,
    size_t element_size,
    size_t *out_capacity);

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

void *
vitte_util_malloc(
    size_t size);

void *
vitte_util_calloc(
    size_t count,
    size_t element_size);

void *
vitte_util_realloc(
    void *pointer,
    size_t size);

void
vitte_util_free(
    void *pointer);

void *
vitte_util_memdup(
    const void *data,
    size_t length);

char *
vitte_util_strndup(
    const char *data,
    size_t length);

/* ========================================================================= */
/* Memory                                                                    */
/* ========================================================================= */

bool
vitte_util_memory_equal(
    const void *left,
    const void *right,
    size_t length);

int
vitte_util_memory_compare(
    const void *left,
    size_t left_length,
    const void *right,
    size_t right_length);

bool
vitte_util_memory_is_zero(
    const void *data,
    size_t length);

void
vitte_util_memory_zero(
    void *data,
    size_t length);

/* ========================================================================= */
/* String views                                                              */
/* ========================================================================= */

vitte_util_string_view_t
vitte_util_string_view(
    const char *data,
    size_t length);

vitte_util_string_view_t
vitte_util_string_from_cstr(
    const char *string);

bool
vitte_util_string_equal(
    vitte_util_string_view_t left,
    vitte_util_string_view_t right);

bool
vitte_util_string_equal_cstr(
    vitte_util_string_view_t left,
    const char *right);

int
vitte_util_string_compare(
    vitte_util_string_view_t left,
    vitte_util_string_view_t right);

bool
vitte_util_string_starts_with(
    vitte_util_string_view_t string,
    vitte_util_string_view_t prefix);

bool
vitte_util_string_ends_with(
    vitte_util_string_view_t string,
    vitte_util_string_view_t suffix);

bool
vitte_util_string_contains(
    vitte_util_string_view_t string,
    vitte_util_string_view_t needle);

size_t
vitte_util_string_find(
    vitte_util_string_view_t string,
    vitte_util_string_view_t needle);

size_t
vitte_util_string_rfind(
    vitte_util_string_view_t string,
    vitte_util_string_view_t needle);

/* ========================================================================= */
/* ASCII                                                                     */
/* ========================================================================= */

bool
vitte_util_ascii_is_space(
    unsigned char character);

bool
vitte_util_ascii_is_digit(
    unsigned char character);

bool
vitte_util_ascii_is_hex_digit(
    unsigned char character);

bool
vitte_util_ascii_is_alpha(
    unsigned char character);

bool
vitte_util_ascii_is_alnum(
    unsigned char character);

unsigned char
vitte_util_ascii_lower(
    unsigned char character);

unsigned char
vitte_util_ascii_upper(
    unsigned char character);

int
vitte_util_ascii_hex_value(
    unsigned char character);

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

uint64_t
vitte_util_hash_bytes(
    const void *data,
    size_t length);

uint64_t
vitte_util_hash_string(
    const char *data,
    size_t length);

uint64_t
vitte_util_hash_u64(
    uint64_t value);

uint64_t
vitte_util_hash_combine(
    uint64_t hash,
    uint64_t value);

/* ========================================================================= */
/* Buffer lifecycle                                                          */
/* ========================================================================= */

bool
vitte_util_buffer_init(
    vitte_util_buffer_t *buffer);

bool
vitte_util_buffer_init_capacity(
    vitte_util_buffer_t *buffer,
    size_t capacity);

void
vitte_util_buffer_reset(
    vitte_util_buffer_t *buffer);

void
vitte_util_buffer_destroy(
    vitte_util_buffer_t *buffer);

bool
vitte_util_buffer_reserve(
    vitte_util_buffer_t *buffer,
    size_t required_capacity);

bool
vitte_util_buffer_resize(
    vitte_util_buffer_t *buffer,
    size_t new_length);

bool
vitte_util_buffer_shrink_to_fit(
    vitte_util_buffer_t *buffer);

/* ========================================================================= */
/* Buffer append                                                             */
/* ========================================================================= */

bool
vitte_util_buffer_append(
    vitte_util_buffer_t *buffer,
    const void *data,
    size_t length);

bool
vitte_util_buffer_append_byte(
    vitte_util_buffer_t *buffer,
    unsigned char byte);

bool
vitte_util_buffer_append_cstr(
    vitte_util_buffer_t *buffer,
    const char *string);

bool
vitte_util_buffer_append_string(
    vitte_util_buffer_t *buffer,
    const char *string,
    size_t length);

bool
vitte_util_buffer_append_u16_le(
    vitte_util_buffer_t *buffer,
    uint16_t value);

bool
vitte_util_buffer_append_u32_le(
    vitte_util_buffer_t *buffer,
    uint32_t value);

bool
vitte_util_buffer_append_u64_le(
    vitte_util_buffer_t *buffer,
    uint64_t value);

bool
vitte_util_buffer_append_u16_be(
    vitte_util_buffer_t *buffer,
    uint16_t value);

bool
vitte_util_buffer_append_u32_be(
    vitte_util_buffer_t *buffer,
    uint32_t value);

bool
vitte_util_buffer_append_u64_be(
    vitte_util_buffer_t *buffer,
    uint64_t value);

/* ========================================================================= */
/* Generic vector                                                            */
/* ========================================================================= */

bool
vitte_util_vector_init(
    vitte_util_vector_t *vector,
    size_t element_size);

bool
vitte_util_vector_init_capacity(
    vitte_util_vector_t *vector,
    size_t element_size,
    size_t capacity);

void
vitte_util_vector_reset(
    vitte_util_vector_t *vector);

void
vitte_util_vector_destroy(
    vitte_util_vector_t *vector);

bool
vitte_util_vector_reserve(
    vitte_util_vector_t *vector,
    size_t required_capacity);

bool
vitte_util_vector_resize(
    vitte_util_vector_t *vector,
    size_t new_count);

bool
vitte_util_vector_push(
    vitte_util_vector_t *vector,
    const void *element);

bool
vitte_util_vector_pop(
    vitte_util_vector_t *vector,
    void *out_element);

void *
vitte_util_vector_at(
    vitte_util_vector_t *vector,
    size_t index);

const void *
vitte_util_vector_at_const(
    const vitte_util_vector_t *vector,
    size_t index);

bool
vitte_util_vector_insert(
    vitte_util_vector_t *vector,
    size_t index,
    const void *element);

bool
vitte_util_vector_remove(
    vitte_util_vector_t *vector,
    size_t index,
    void *out_element);

bool
vitte_util_vector_shrink_to_fit(
    vitte_util_vector_t *vector);

/* ========================================================================= */
/* Endian                                                                    */
/* ========================================================================= */

uint16_t
vitte_util_read_u16_le(
    const unsigned char bytes[2]);

uint32_t
vitte_util_read_u32_le(
    const unsigned char bytes[4]);

uint64_t
vitte_util_read_u64_le(
    const unsigned char bytes[8]);

uint16_t
vitte_util_read_u16_be(
    const unsigned char bytes[2]);

uint32_t
vitte_util_read_u32_be(
    const unsigned char bytes[4]);

uint64_t
vitte_util_read_u64_be(
    const unsigned char bytes[8]);

void
vitte_util_write_u16_le(
    unsigned char bytes[2],
    uint16_t value);

void
vitte_util_write_u32_le(
    unsigned char bytes[4],
    uint32_t value);

void
vitte_util_write_u64_le(
    unsigned char bytes[8],
    uint64_t value);

void
vitte_util_write_u16_be(
    unsigned char bytes[2],
    uint16_t value);

void
vitte_util_write_u32_be(
    unsigned char bytes[4],
    uint32_t value);

void
vitte_util_write_u64_be(
    unsigned char bytes[8],
    uint64_t value);

/* ========================================================================= */
/* Integer parsing                                                           */
/* ========================================================================= */

bool
vitte_util_parse_u64(
    const char *data,
    size_t length,
    unsigned int base,
    uint64_t *out_value);

bool
vitte_util_parse_i64(
    const char *data,
    size_t length,
    unsigned int base,
    int64_t *out_value);

/* ========================================================================= */
/* Sorting                                                                   */
/* ========================================================================= */

bool
vitte_util_sort(
    void *base,
    size_t count,
    size_t element_size,
    vitte_util_compare_fn compare,
    void *user_data);

const void *
vitte_util_binary_search(
    const void *key,
    const void *base,
    size_t count,
    size_t element_size,
    vitte_util_compare_fn compare,
    void *user_data);

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_util_translation_unit_anchor(void);

/* ========================================================================= */
/* Inline helpers                                                            */
/* ========================================================================= */

static inline bool
vitte_util_error_is_valid(
    vitte_util_error_t error)
{
    return error >= VITTE_UTIL_ERROR_NONE &&
           error < VITTE_UTIL_ERROR_COUNT;
}

static inline bool
vitte_util_error_is_success(
    vitte_util_error_t error)
{
    return error == VITTE_UTIL_ERROR_NONE;
}

static inline size_t
vitte_util_min_size(
    size_t left,
    size_t right)
{
    return left < right ? left : right;
}

static inline size_t
vitte_util_max_size(
    size_t left,
    size_t right)
{
    return left > right ? left : right;
}

static inline uint64_t
vitte_util_min_u64(
    uint64_t left,
    uint64_t right)
{
    return left < right ? left : right;
}

static inline uint64_t
vitte_util_max_u64(
    uint64_t left,
    uint64_t right)
{
    return left > right ? left : right;
}

static inline bool
vitte_util_buffer_is_valid(
    const vitte_util_buffer_t *buffer)
{
    if (buffer == NULL) {
        return false;
    }

    if (buffer->length > buffer->capacity) {
        return false;
    }

    if (buffer->capacity != 0u &&
        buffer->data == NULL) {
        return false;
    }

    return true;
}

static inline bool
vitte_util_buffer_empty(
    const vitte_util_buffer_t *buffer)
{
    return buffer == NULL ||
           buffer->length == 0u;
}

static inline bool
vitte_util_vector_is_valid(
    const vitte_util_vector_t *vector)
{
    if (vector == NULL ||
        vector->element_size == 0u) {
        return false;
    }

    if (vector->count > vector->capacity) {
        return false;
    }

    if (vector->capacity != 0u &&
        vector->data == NULL) {
        return false;
    }

    return true;
}

static inline bool
vitte_util_vector_empty(
    const vitte_util_vector_t *vector)
{
    return vector == NULL ||
           vector->count == 0u;
}

static inline unsigned int
vitte_util_api_version_major(void)
{
    return VITTE_UTIL_API_VERSION_MAJOR;
}

static inline unsigned int
vitte_util_api_version_minor(void)
{
    return VITTE_UTIL_API_VERSION_MINOR;
}

static inline unsigned int
vitte_util_api_version_patch(void)
{
    return VITTE_UTIL_API_VERSION_PATCH;
}

static inline unsigned int
vitte_util_api_version(void)
{
    return VITTE_UTIL_API_VERSION;
}

/* ========================================================================= */
/* Compile-time checks                                                       */
/* ========================================================================= */

#if defined(__cplusplus)

static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte requires 64-bit uint64_t");

static_assert(
    sizeof(uint32_t) == 4u,
    "Vitte requires 32-bit uint32_t");

static_assert(
    sizeof(uint16_t) == 2u,
    "Vitte requires 16-bit uint16_t");

static_assert(
    VITTE_UTIL_ERROR_NONE == 0,
    "VITTE_UTIL_ERROR_NONE must be zero");

#else

_Static_assert(
    sizeof(uint64_t) == 8u,
    "Vitte requires 64-bit uint64_t");

_Static_assert(
    sizeof(uint32_t) == 4u,
    "Vitte requires 32-bit uint32_t");

_Static_assert(
    sizeof(uint16_t) == 2u,
    "Vitte requires 16-bit uint16_t");

_Static_assert(
    VITTE_UTIL_ERROR_NONE == 0,
    "VITTE_UTIL_ERROR_NONE must be zero");

#endif

#ifdef __cplusplus
}
#endif

#endif /* VITTE_UTIL_UTIL_H */
