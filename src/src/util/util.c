/*
 * Vitte Compiler
 * src/util/util.c
 *
 * General-purpose compiler utilities.
 *
 * Public contract: util.h
 *
 * ISO C17.
 */

#include "util.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Error                                                                     */
/* ========================================================================= */

const char *
vitte_util_error_name(
    vitte_util_error_t error)
{
    switch (error) {
        case VITTE_UTIL_ERROR_NONE:
            return "none";

        case VITTE_UTIL_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case VITTE_UTIL_ERROR_INVALID_STATE:
            return "invalid_state";

        case VITTE_UTIL_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_UTIL_ERROR_OUT_OF_MEMORY:
            return "out_of_memory";

        case VITTE_UTIL_ERROR_OUT_OF_RANGE:
            return "out_of_range";

        case VITTE_UTIL_ERROR_BUFFER_TOO_SMALL:
            return "buffer_too_small";

        case VITTE_UTIL_ERROR_INVALID_FORMAT:
            return "invalid_format";

        case VITTE_UTIL_ERROR_INTERNAL:
            return "internal";

        case VITTE_UTIL_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Checked arithmetic                                                        */
/* ========================================================================= */

bool
vitte_util_add_size(
    size_t left,
    size_t right,
    size_t *out)
{
    if (out == NULL) {
        return false;
    }

    if (left > SIZE_MAX - right) {
        return false;
    }

    *out = left + right;

    return true;
}

bool
vitte_util_sub_size(
    size_t left,
    size_t right,
    size_t *out)
{
    if (out == NULL ||
        right > left) {
        return false;
    }

    *out = left - right;

    return true;
}

bool
vitte_util_mul_size(
    size_t left,
    size_t right,
    size_t *out)
{
    if (out == NULL) {
        return false;
    }

    if (left != 0u &&
        right > SIZE_MAX / left) {
        return false;
    }

    *out = left * right;

    return true;
}

bool
vitte_util_add_u64(
    uint64_t left,
    uint64_t right,
    uint64_t *out)
{
    if (out == NULL) {
        return false;
    }

    if (left > UINT64_MAX - right) {
        return false;
    }

    *out = left + right;

    return true;
}

bool
vitte_util_mul_u64(
    uint64_t left,
    uint64_t right,
    uint64_t *out)
{
    if (out == NULL) {
        return false;
    }

    if (left != UINT64_C(0) &&
        right > UINT64_MAX / left) {
        return false;
    }

    *out = left * right;

    return true;
}

/* ========================================================================= */
/* Alignment                                                                 */
/* ========================================================================= */

bool
vitte_util_is_power_of_two_size(
    size_t value)
{
    return value != 0u &&
           (value & (value - 1u)) == 0u;
}

bool
vitte_util_align_up_size(
    size_t value,
    size_t alignment,
    size_t *out)
{
    size_t mask;

    if (out == NULL ||
        !vitte_util_is_power_of_two_size(
            alignment)) {
        return false;
    }

    mask = alignment - 1u;

    if (value > SIZE_MAX - mask) {
        return false;
    }

    *out = (value + mask) & ~mask;

    return true;
}

bool
vitte_util_align_down_size(
    size_t value,
    size_t alignment,
    size_t *out)
{
    if (out == NULL ||
        !vitte_util_is_power_of_two_size(
            alignment)) {
        return false;
    }

    *out = value & ~(alignment - 1u);

    return true;
}

/* ========================================================================= */
/* Capacity                                                                  */
/* ========================================================================= */

bool
vitte_util_next_capacity(
    size_t current,
    size_t required,
    size_t element_size,
    size_t *out_capacity)
{
    size_t capacity;

    if (out_capacity == NULL ||
        element_size == 0u) {
        return false;
    }

    if (required == 0u) {
        *out_capacity = current;
        return true;
    }

    if (required > SIZE_MAX / element_size) {
        return false;
    }

    if (current >= required) {
        *out_capacity = current;
        return true;
    }

    capacity = current;

    if (capacity == 0u) {
        capacity =
            VITTE_UTIL_DEFAULT_VECTOR_CAPACITY;
    }

    while (capacity < required) {
        size_t next;

        if (capacity > SIZE_MAX / 2u) {
            capacity = required;
            break;
        }

        next = capacity * 2u;

        if (next <= capacity) {
            return false;
        }

        capacity = next;
    }

    if (capacity > SIZE_MAX / element_size) {
        return false;
    }

    *out_capacity = capacity;

    return true;
}

/* ========================================================================= */
/* Allocation                                                                */
/* ========================================================================= */

void *
vitte_util_malloc(
    size_t size)
{
    if (size == 0u) {
        size = 1u;
    }

    return malloc(size);
}

void *
vitte_util_calloc(
    size_t count,
    size_t element_size)
{
    if (count == 0u ||
        element_size == 0u) {
        return calloc(1u, 1u);
    }

    if (count > SIZE_MAX / element_size) {
        return NULL;
    }

    return calloc(
        count,
        element_size);
}

void *
vitte_util_realloc(
    void *pointer,
    size_t size)
{
    if (size == 0u) {
        size = 1u;
    }

    return realloc(
        pointer,
        size);
}

void
vitte_util_free(
    void *pointer)
{
    free(pointer);
}

void *
vitte_util_memdup(
    const void *data,
    size_t length)
{
    void *copy;

    if (length == 0u) {
        return vitte_util_malloc(1u);
    }

    if (data == NULL) {
        return NULL;
    }

    copy = vitte_util_malloc(length);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(
        copy,
        data,
        length);

    return copy;
}

char *
vitte_util_strndup(
    const char *data,
    size_t length)
{
    char *copy;
    size_t allocation_size;

    if (data == NULL &&
        length != 0u) {
        return NULL;
    }

    if (!vitte_util_add_size(
            length,
            1u,
            &allocation_size)) {
        return NULL;
    }

    copy =
        (char *)vitte_util_malloc(
            allocation_size);

    if (copy == NULL) {
        return NULL;
    }

    if (length != 0u) {
        memcpy(
            copy,
            data,
            length);
    }

    copy[length] = '\0';

    return copy;
}

/* ========================================================================= */
/* Memory                                                                    */
/* ========================================================================= */

bool
vitte_util_memory_equal(
    const void *left,
    const void *right,
    size_t length)
{
    if (length == 0u) {
        return true;
    }

    if (left == NULL ||
        right == NULL) {
        return false;
    }

    return memcmp(
               left,
               right,
               length) == 0;
}

int
vitte_util_memory_compare(
    const void *left,
    size_t left_length,
    const void *right,
    size_t right_length)
{
    size_t common;
    int comparison;

    if (left_length != 0u &&
        left == NULL) {
        return -1;
    }

    if (right_length != 0u &&
        right == NULL) {
        return 1;
    }

    common =
        left_length < right_length
            ? left_length
            : right_length;

    comparison = 0;

    if (common != 0u) {
        comparison =
            memcmp(
                left,
                right,
                common);
    }

    if (comparison < 0) {
        return -1;
    }

    if (comparison > 0) {
        return 1;
    }

    if (left_length < right_length) {
        return -1;
    }

    if (left_length > right_length) {
        return 1;
    }

    return 0;
}

bool
vitte_util_memory_is_zero(
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    if (length == 0u) {
        return true;
    }

    if (data == NULL) {
        return false;
    }

    bytes =
        (const unsigned char *)data;

    for (index = 0u;
         index < length;
         ++index) {
        if (bytes[index] != 0u) {
            return false;
        }
    }

    return true;
}

void
vitte_util_memory_zero(
    void *data,
    size_t length)
{
    if (data == NULL ||
        length == 0u) {
        return;
    }

    memset(
        data,
        0,
        length);
}

/* ========================================================================= */
/* String views                                                              */
/* ========================================================================= */

vitte_util_string_view_t
vitte_util_string_view(
    const char *data,
    size_t length)
{
    vitte_util_string_view_t view;

    view.data = data;
    view.length = length;

    return view;
}

vitte_util_string_view_t
vitte_util_string_from_cstr(
    const char *string)
{
    vitte_util_string_view_t view;

    view.data = string;
    view.length =
        string != NULL
            ? strlen(string)
            : 0u;

    return view;
}

bool
vitte_util_string_equal(
    vitte_util_string_view_t left,
    vitte_util_string_view_t right)
{
    if (left.length != right.length) {
        return false;
    }

    return vitte_util_memory_equal(
        left.data,
        right.data,
        left.length);
}

bool
vitte_util_string_equal_cstr(
    vitte_util_string_view_t left,
    const char *right)
{
    return vitte_util_string_equal(
        left,
        vitte_util_string_from_cstr(
            right));
}

int
vitte_util_string_compare(
    vitte_util_string_view_t left,
    vitte_util_string_view_t right)
{
    return vitte_util_memory_compare(
        left.data,
        left.length,
        right.data,
        right.length);
}

bool
vitte_util_string_starts_with(
    vitte_util_string_view_t string,
    vitte_util_string_view_t prefix)
{
    if (prefix.length > string.length) {
        return false;
    }

    return vitte_util_memory_equal(
        string.data,
        prefix.data,
        prefix.length);
}

bool
vitte_util_string_ends_with(
    vitte_util_string_view_t string,
    vitte_util_string_view_t suffix)
{
    if (suffix.length > string.length) {
        return false;
    }

    if (suffix.length == 0u) {
        return true;
    }

    if (string.data == NULL ||
        suffix.data == NULL) {
        return false;
    }

    return vitte_util_memory_equal(
        string.data +
            (string.length - suffix.length),
        suffix.data,
        suffix.length);
}

size_t
vitte_util_string_find(
    vitte_util_string_view_t string,
    vitte_util_string_view_t needle)
{
    size_t index;

    if (needle.length == 0u) {
        return 0u;
    }

    if (string.data == NULL ||
        needle.data == NULL ||
        needle.length > string.length) {
        return SIZE_MAX;
    }

    for (index = 0u;
         index <= string.length - needle.length;
         ++index) {
        if (memcmp(
                string.data + index,
                needle.data,
                needle.length) == 0) {
            return index;
        }
    }

    return SIZE_MAX;
}

size_t
vitte_util_string_rfind(
    vitte_util_string_view_t string,
    vitte_util_string_view_t needle)
{
    size_t index;

    if (needle.length == 0u) {
        return string.length;
    }

    if (string.data == NULL ||
        needle.data == NULL ||
        needle.length > string.length) {
        return SIZE_MAX;
    }

    index =
        string.length -
        needle.length;

    for (;;) {
        if (memcmp(
                string.data + index,
                needle.data,
                needle.length) == 0) {
            return index;
        }

        if (index == 0u) {
            break;
        }

        index--;
    }

    return SIZE_MAX;
}

bool
vitte_util_string_contains(
    vitte_util_string_view_t string,
    vitte_util_string_view_t needle)
{
    return vitte_util_string_find(
               string,
               needle) != SIZE_MAX;
}

/* ========================================================================= */
/* ASCII                                                                     */
/* ========================================================================= */

bool
vitte_util_ascii_is_space(
    unsigned char character)
{
    return character == (unsigned char)' ' ||
           character == (unsigned char)'\t' ||
           character == (unsigned char)'\n' ||
           character == (unsigned char)'\r' ||
           character == (unsigned char)'\v' ||
           character == (unsigned char)'\f';
}

bool
vitte_util_ascii_is_digit(
    unsigned char character)
{
    return character >= (unsigned char)'0' &&
           character <= (unsigned char)'9';
}

bool
vitte_util_ascii_is_hex_digit(
    unsigned char character)
{
    return vitte_util_ascii_is_digit(
               character) ||
           (character >= (unsigned char)'a' &&
            character <= (unsigned char)'f') ||
           (character >= (unsigned char)'A' &&
            character <= (unsigned char)'F');
}

bool
vitte_util_ascii_is_alpha(
    unsigned char character)
{
    return (character >= (unsigned char)'a' &&
            character <= (unsigned char)'z') ||
           (character >= (unsigned char)'A' &&
            character <= (unsigned char)'Z');
}

bool
vitte_util_ascii_is_alnum(
    unsigned char character)
{
    return vitte_util_ascii_is_alpha(
               character) ||
           vitte_util_ascii_is_digit(
               character);
}

unsigned char
vitte_util_ascii_lower(
    unsigned char character)
{
    if (character >= (unsigned char)'A' &&
        character <= (unsigned char)'Z') {
        return (unsigned char)(
            character +
            ((unsigned char)'a' -
             (unsigned char)'A'));
    }

    return character;
}

unsigned char
vitte_util_ascii_upper(
    unsigned char character)
{
    if (character >= (unsigned char)'a' &&
        character <= (unsigned char)'z') {
        return (unsigned char)(
            character -
            ((unsigned char)'a' -
             (unsigned char)'A'));
    }

    return character;
}

int
vitte_util_ascii_hex_value(
    unsigned char character)
{
    if (character >= (unsigned char)'0' &&
        character <= (unsigned char)'9') {
        return (int)(
            character -
            (unsigned char)'0');
    }

    if (character >= (unsigned char)'a' &&
        character <= (unsigned char)'f') {
        return 10 +
               (int)(
                   character -
                   (unsigned char)'a');
    }

    if (character >= (unsigned char)'A' &&
        character <= (unsigned char)'F') {
        return 10 +
               (int)(
                   character -
                   (unsigned char)'A');
    }

    return -1;
}

/* ========================================================================= */
/* Hash                                                                      */
/* ========================================================================= */

uint64_t
vitte_util_hash_bytes(
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    uint64_t hash;
    size_t index;

    hash = VITTE_UTIL_FNV_OFFSET;

    if (length == 0u) {
        return hash;
    }

    if (data == NULL) {
        return UINT64_C(0);
    }

    bytes =
        (const unsigned char *)data;

    for (index = 0u;
         index < length;
         ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_UTIL_FNV_PRIME;
    }

    return hash;
}

uint64_t
vitte_util_hash_string(
    const char *data,
    size_t length)
{
    return vitte_util_hash_bytes(
        data,
        length);
}

uint64_t
vitte_util_hash_u64(
    uint64_t value)
{
    uint64_t hash;
    unsigned int shift;

    hash = VITTE_UTIL_FNV_OFFSET;

    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        hash ^=
            (value >> shift) &
            UINT64_C(0xff);

        hash *= VITTE_UTIL_FNV_PRIME;
    }

    return hash;
}

uint64_t
vitte_util_hash_combine(
    uint64_t hash,
    uint64_t value)
{
    unsigned int shift;

    for (shift = 0u;
         shift < 64u;
         shift += 8u) {
        hash ^=
            (value >> shift) &
            UINT64_C(0xff);

        hash *= VITTE_UTIL_FNV_PRIME;
    }

    return hash;
}

/* ========================================================================= */
/* Buffer                                                                    */
/* ========================================================================= */

bool
vitte_util_buffer_init(
    vitte_util_buffer_t *buffer)
{
    if (buffer == NULL) {
        return false;
    }

    memset(
        buffer,
        0,
        sizeof(*buffer));

    return true;
}

bool
vitte_util_buffer_init_capacity(
    vitte_util_buffer_t *buffer,
    size_t capacity)
{
    if (!vitte_util_buffer_init(
            buffer)) {
        return false;
    }

    if (capacity == 0u) {
        return true;
    }

    buffer->data =
        (unsigned char *)
        vitte_util_malloc(
            capacity);

    if (buffer->data == NULL) {
        return false;
    }

    buffer->capacity = capacity;

    return true;
}

void
vitte_util_buffer_reset(
    vitte_util_buffer_t *buffer)
{
    if (buffer == NULL) {
        return;
    }

    buffer->length = 0u;
}

void
vitte_util_buffer_destroy(
    vitte_util_buffer_t *buffer)
{
    if (buffer == NULL) {
        return;
    }

    vitte_util_free(
        buffer->data);

    memset(
        buffer,
        0,
        sizeof(*buffer));
}

bool
vitte_util_buffer_reserve(
    vitte_util_buffer_t *buffer,
    size_t required_capacity)
{
    size_t capacity;
    unsigned char *new_data;

    if (!vitte_util_buffer_is_valid(
            buffer)) {
        return false;
    }

    if (required_capacity <=
        buffer->capacity) {
        return true;
    }

    if (!vitte_util_next_capacity(
            buffer->capacity,
            required_capacity,
            1u,
            &capacity)) {
        return false;
    }

    new_data =
        (unsigned char *)
        vitte_util_realloc(
            buffer->data,
            capacity);

    if (new_data == NULL) {
        return false;
    }

    buffer->data = new_data;
    buffer->capacity = capacity;

    return true;
}

bool
vitte_util_buffer_resize(
    vitte_util_buffer_t *buffer,
    size_t new_length)
{
    size_t old_length;

    if (!vitte_util_buffer_is_valid(
            buffer)) {
        return false;
    }

    old_length = buffer->length;

    if (new_length >
        buffer->capacity) {
        if (!vitte_util_buffer_reserve(
                buffer,
                new_length)) {
            return false;
        }
    }

    if (new_length > old_length) {
        memset(
            buffer->data + old_length,
            0,
            new_length - old_length);
    }

    buffer->length = new_length;

    return true;
}

bool
vitte_util_buffer_shrink_to_fit(
    vitte_util_buffer_t *buffer)
{
    unsigned char *new_data;

    if (!vitte_util_buffer_is_valid(
            buffer)) {
        return false;
    }

    if (buffer->length ==
        buffer->capacity) {
        return true;
    }

    if (buffer->length == 0u) {
        vitte_util_free(
            buffer->data);

        buffer->data = NULL;
        buffer->capacity = 0u;

        return true;
    }

    new_data =
        (unsigned char *)
        vitte_util_realloc(
            buffer->data,
            buffer->length);

    if (new_data == NULL) {
        return false;
    }

    buffer->data = new_data;
    buffer->capacity = buffer->length;

    return true;
}

bool
vitte_util_buffer_append(
    vitte_util_buffer_t *buffer,
    const void *data,
    size_t length)
{
    size_t required;

    if (!vitte_util_buffer_is_valid(
            buffer)) {
        return false;
    }

    if (length == 0u) {
        return true;
    }

    if (data == NULL) {
        return false;
    }

    if (!vitte_util_add_size(
            buffer->length,
            length,
            &required)) {
        return false;
    }

    if (!vitte_util_buffer_reserve(
            buffer,
            required)) {
        return false;
    }

    /*
     * memmove intentionally permits appending a range that already belongs
     * to the same buffer, provided reserve did not invalidate the caller's
     * source pointer. External callers should not retain buffer pointers
     * across operations that can reallocate.
     */
    memmove(
        buffer->data +
            buffer->length,
        data,
        length);

    buffer->length = required;

    return true;
}

bool
vitte_util_buffer_append_byte(
    vitte_util_buffer_t *buffer,
    unsigned char byte)
{
    return vitte_util_buffer_append(
        buffer,
        &byte,
        1u);
}

bool
vitte_util_buffer_append_cstr(
    vitte_util_buffer_t *buffer,
    const char *string)
{
    if (string == NULL) {
        return false;
    }

    return vitte_util_buffer_append(
        buffer,
        string,
        strlen(string));
}

bool
vitte_util_buffer_append_string(
    vitte_util_buffer_t *buffer,
    const char *string,
    size_t length)
{
    return vitte_util_buffer_append(
        buffer,
        string,
        length);
}

/* ========================================================================= */
/* Endian writers                                                            */
/* ========================================================================= */

void
vitte_util_write_u16_le(
    unsigned char bytes[2],
    uint16_t value)
{
    if (bytes == NULL) {
        return;
    }

    bytes[0] =
        (unsigned char)(
            value & UINT16_C(0xff));

    bytes[1] =
        (unsigned char)(
            (value >> 8u) &
            UINT16_C(0xff));
}

void
vitte_util_write_u32_le(
    unsigned char bytes[4],
    uint32_t value)
{
    unsigned int index;

    if (bytes == NULL) {
        return;
    }

    for (index = 0u;
         index < 4u;
         ++index) {
        bytes[index] =
            (unsigned char)(
                (value >>
                 (index * 8u)) &
                UINT32_C(0xff));
    }
}

void
vitte_util_write_u64_le(
    unsigned char bytes[8],
    uint64_t value)
{
    unsigned int index;

    if (bytes == NULL) {
        return;
    }

    for (index = 0u;
         index < 8u;
         ++index) {
        bytes[index] =
            (unsigned char)(
                (value >>
                 (index * 8u)) &
                UINT64_C(0xff));
    }
}

void
vitte_util_write_u16_be(
    unsigned char bytes[2],
    uint16_t value)
{
    if (bytes == NULL) {
        return;
    }

    bytes[0] =
        (unsigned char)(
            (value >> 8u) &
            UINT16_C(0xff));

    bytes[1] =
        (unsigned char)(
            value &
            UINT16_C(0xff));
}

void
vitte_util_write_u32_be(
    unsigned char bytes[4],
    uint32_t value)
{
    unsigned int index;

    if (bytes == NULL) {
        return;
    }

    for (index = 0u;
         index < 4u;
         ++index) {
        const unsigned int shift =
            (3u - index) * 8u;

        bytes[index] =
            (unsigned char)(
                (value >> shift) &
                UINT32_C(0xff));
    }
}

void
vitte_util_write_u64_be(
    unsigned char bytes[8],
    uint64_t value)
{
    unsigned int index;

    if (bytes == NULL) {
        return;
    }

    for (index = 0u;
         index < 8u;
         ++index) {
        const unsigned int shift =
            (7u - index) * 8u;

        bytes[index] =
            (unsigned char)(
                (value >> shift) &
                UINT64_C(0xff));
    }
}

/* ========================================================================= */
/* Endian readers                                                            */
/* ========================================================================= */

uint16_t
vitte_util_read_u16_le(
    const unsigned char bytes[2])
{
    if (bytes == NULL) {
        return UINT16_C(0);
    }

    return
        (uint16_t)(
            (uint16_t)bytes[0] |
            ((uint16_t)bytes[1] << 8u));
}

uint32_t
vitte_util_read_u32_le(
    const unsigned char bytes[4])
{
    uint32_t value;
    unsigned int index;

    if (bytes == NULL) {
        return UINT32_C(0);
    }

    value = UINT32_C(0);

    for (index = 0u;
         index < 4u;
         ++index) {
        value |=
            (uint32_t)bytes[index] <<
            (index * 8u);
    }

    return value;
}

uint64_t
vitte_util_read_u64_le(
    const unsigned char bytes[8])
{
    uint64_t value;
    unsigned int index;

    if (bytes == NULL) {
        return UINT64_C(0);
    }

    value = UINT64_C(0);

    for (index = 0u;
         index < 8u;
         ++index) {
        value |=
            (uint64_t)bytes[index] <<
            (index * 8u);
    }

    return value;
}

uint16_t
vitte_util_read_u16_be(
    const unsigned char bytes[2])
{
    if (bytes == NULL) {
        return UINT16_C(0);
    }

    return
        (uint16_t)(
            ((uint16_t)bytes[0] << 8u) |
            (uint16_t)bytes[1]);
}

uint32_t
vitte_util_read_u32_be(
    const unsigned char bytes[4])
{
    uint32_t value;
    unsigned int index;

    if (bytes == NULL) {
        return UINT32_C(0);
    }

    value = UINT32_C(0);

    for (index = 0u;
         index < 4u;
         ++index) {
        value =
            (value << 8u) |
            (uint32_t)bytes[index];
    }

    return value;
}

uint64_t
vitte_util_read_u64_be(
    const unsigned char bytes[8])
{
    uint64_t value;
    unsigned int index;

    if (bytes == NULL) {
        return UINT64_C(0);
    }

    value = UINT64_C(0);

    for (index = 0u;
         index < 8u;
         ++index) {
        value =
            (value << 8u) |
            (uint64_t)bytes[index];
    }

    return value;
}

/* ========================================================================= */
/* Buffer endian append                                                      */
/* ========================================================================= */

bool
vitte_util_buffer_append_u16_le(
    vitte_util_buffer_t *buffer,
    uint16_t value)
{
    unsigned char bytes[2];

    vitte_util_write_u16_le(
        bytes,
        value);

    return vitte_util_buffer_append(
        buffer,
        bytes,
        sizeof(bytes));
}

bool
vitte_util_buffer_append_u32_le(
    vitte_util_buffer_t *buffer,
    uint32_t value)
{
    unsigned char bytes[4];

    vitte_util_write_u32_le(
        bytes,
        value);

    return vitte_util_buffer_append(
        buffer,
        bytes,
        sizeof(bytes));
}

bool
vitte_util_buffer_append_u64_le(
    vitte_util_buffer_t *buffer,
    uint64_t value)
{
    unsigned char bytes[8];

    vitte_util_write_u64_le(
        bytes,
        value);

    return vitte_util_buffer_append(
        buffer,
        bytes,
        sizeof(bytes));
}

bool
vitte_util_buffer_append_u16_be(
    vitte_util_buffer_t *buffer,
    uint16_t value)
{
    unsigned char bytes[2];

    vitte_util_write_u16_be(
        bytes,
        value);

    return vitte_util_buffer_append(
        buffer,
        bytes,
        sizeof(bytes));
}

bool
vitte_util_buffer_append_u32_be(
    vitte_util_buffer_t *buffer,
    uint32_t value)
{
    unsigned char bytes[4];

    vitte_util_write_u32_be(
        bytes,
        value);

    return vitte_util_buffer_append(
        buffer,
        bytes,
        sizeof(bytes));
}

bool
vitte_util_buffer_append_u64_be(
    vitte_util_buffer_t *buffer,
    uint64_t value)
{
    unsigned char bytes[8];

    vitte_util_write_u64_be(
        bytes,
        value);

    return vitte_util_buffer_append(
        buffer,
        bytes,
        sizeof(bytes));
}

/* ========================================================================= */
/* Vector                                                                    */
/* ========================================================================= */

bool
vitte_util_vector_init(
    vitte_util_vector_t *vector,
    size_t element_size)
{
    if (vector == NULL ||
        element_size == 0u) {
        return false;
    }

    memset(
        vector,
        0,
        sizeof(*vector));

    vector->element_size =
        element_size;

    return true;
}

bool
vitte_util_vector_init_capacity(
    vitte_util_vector_t *vector,
    size_t element_size,
    size_t capacity)
{
    size_t bytes;

    if (!vitte_util_vector_init(
            vector,
            element_size)) {
        return false;
    }

    if (capacity == 0u) {
        return true;
    }

    if (!vitte_util_mul_size(
            capacity,
            element_size,
            &bytes)) {
        return false;
    }

    vector->data =
        (unsigned char *)
        vitte_util_malloc(bytes);

    if (vector->data == NULL) {
        return false;
    }

    vector->capacity = capacity;

    return true;
}

void
vitte_util_vector_reset(
    vitte_util_vector_t *vector)
{
    if (vector == NULL) {
        return;
    }

    vector->count = 0u;
}

void
vitte_util_vector_destroy(
    vitte_util_vector_t *vector)
{
    if (vector == NULL) {
        return;
    }

    vitte_util_free(
        vector->data);

    memset(
        vector,
        0,
        sizeof(*vector));
}

bool
vitte_util_vector_reserve(
    vitte_util_vector_t *vector,
    size_t required_capacity)
{
    size_t capacity;
    size_t bytes;
    unsigned char *new_data;

    if (!vitte_util_vector_is_valid(
            vector)) {
        return false;
    }

    if (required_capacity <=
        vector->capacity) {
        return true;
    }

    if (!vitte_util_next_capacity(
            vector->capacity,
            required_capacity,
            vector->element_size,
            &capacity)) {
        return false;
    }

    if (!vitte_util_mul_size(
            capacity,
            vector->element_size,
            &bytes)) {
        return false;
    }

    new_data =
        (unsigned char *)
        vitte_util_realloc(
            vector->data,
            bytes);

    if (new_data == NULL) {
        return false;
    }

    vector->data = new_data;
    vector->capacity = capacity;

    return true;
}

bool
vitte_util_vector_resize(
    vitte_util_vector_t *vector,
    size_t new_count)
{
    size_t old_count;
    size_t offset;
    size_t bytes;

    if (!vitte_util_vector_is_valid(
            vector)) {
        return false;
    }

    old_count = vector->count;

    if (new_count >
        vector->capacity) {
        if (!vitte_util_vector_reserve(
                vector,
                new_count)) {
            return false;
        }
    }

    if (new_count > old_count) {
        if (!vitte_util_mul_size(
                old_count,
                vector->element_size,
                &offset) ||
            !vitte_util_mul_size(
                new_count - old_count,
                vector->element_size,
                &bytes)) {
            return false;
        }

        memset(
            vector->data + offset,
            0,
            bytes);
    }

    vector->count = new_count;

    return true;
}

bool
vitte_util_vector_push(
    vitte_util_vector_t *vector,
    const void *element)
{
    size_t new_count;
    size_t offset;

    if (!vitte_util_vector_is_valid(
            vector) ||
        element == NULL) {
        return false;
    }

    if (!vitte_util_add_size(
            vector->count,
            1u,
            &new_count)) {
        return false;
    }

    if (!vitte_util_vector_reserve(
            vector,
            new_count)) {
        return false;
    }

    if (!vitte_util_mul_size(
            vector->count,
            vector->element_size,
            &offset)) {
        return false;
    }

    memcpy(
        vector->data + offset,
        element,
        vector->element_size);

    vector->count = new_count;

    return true;
}

bool
vitte_util_vector_pop(
    vitte_util_vector_t *vector,
    void *out_element)
{
    size_t offset;

    if (!vitte_util_vector_is_valid(
            vector) ||
        vector->count == 0u) {
        return false;
    }

    if (!vitte_util_mul_size(
            vector->count - 1u,
            vector->element_size,
            &offset)) {
        return false;
    }

    if (out_element != NULL) {
        memcpy(
            out_element,
            vector->data + offset,
            vector->element_size);
    }

    memset(
        vector->data + offset,
        0,
        vector->element_size);

    vector->count--;

    return true;
}

void *
vitte_util_vector_at(
    vitte_util_vector_t *vector,
    size_t index)
{
    size_t offset;

    if (!vitte_util_vector_is_valid(
            vector) ||
        index >= vector->count) {
        return NULL;
    }

    if (!vitte_util_mul_size(
            index,
            vector->element_size,
            &offset)) {
        return NULL;
    }

    return vector->data + offset;
}

const void *
vitte_util_vector_at_const(
    const vitte_util_vector_t *vector,
    size_t index)
{
    size_t offset;

    if (!vitte_util_vector_is_valid(
            vector) ||
        index >= vector->count) {
        return NULL;
    }

    if (!vitte_util_mul_size(
            index,
            vector->element_size,
            &offset)) {
        return NULL;
    }

    return vector->data + offset;
}

bool
vitte_util_vector_insert(
    vitte_util_vector_t *vector,
    size_t index,
    const void *element)
{
    size_t new_count;
    size_t offset;
    size_t move_count;
    size_t move_bytes;

    if (!vitte_util_vector_is_valid(
            vector) ||
        element == NULL ||
        index > vector->count) {
        return false;
    }

    if (!vitte_util_add_size(
            vector->count,
            1u,
            &new_count)) {
        return false;
    }

    if (!vitte_util_vector_reserve(
            vector,
            new_count)) {
        return false;
    }

    if (!vitte_util_mul_size(
            index,
            vector->element_size,
            &offset)) {
        return false;
    }

    move_count =
        vector->count - index;

    if (!vitte_util_mul_size(
            move_count,
            vector->element_size,
            &move_bytes)) {
        return false;
    }

    if (move_bytes != 0u) {
        memmove(
            vector->data +
                offset +
                vector->element_size,
            vector->data + offset,
            move_bytes);
    }

    memcpy(
        vector->data + offset,
        element,
        vector->element_size);

    vector->count = new_count;

    return true;
}

bool
vitte_util_vector_remove(
    vitte_util_vector_t *vector,
    size_t index,
    void *out_element)
{
    size_t offset;
    size_t move_count;
    size_t move_bytes;
    size_t final_offset;

    if (!vitte_util_vector_is_valid(
            vector) ||
        index >= vector->count) {
        return false;
    }

    if (!vitte_util_mul_size(
            index,
            vector->element_size,
            &offset)) {
        return false;
    }

    if (out_element != NULL) {
        memcpy(
            out_element,
            vector->data + offset,
            vector->element_size);
    }

    move_count =
        vector->count -
        index -
        1u;

    if (!vitte_util_mul_size(
            move_count,
            vector->element_size,
            &move_bytes)) {
        return false;
    }

    if (move_bytes != 0u) {
        memmove(
            vector->data + offset,
            vector->data +
                offset +
                vector->element_size,
            move_bytes);
    }

    if (!vitte_util_mul_size(
            vector->count - 1u,
            vector->element_size,
            &final_offset)) {
        return false;
    }

    memset(
        vector->data + final_offset,
        0,
        vector->element_size);

    vector->count--;

    return true;
}

bool
vitte_util_vector_shrink_to_fit(
    vitte_util_vector_t *vector)
{
    size_t bytes;
    unsigned char *new_data;

    if (!vitte_util_vector_is_valid(
            vector)) {
        return false;
    }

    if (vector->count ==
        vector->capacity) {
        return true;
    }

    if (vector->count == 0u) {
        vitte_util_free(
            vector->data);

        vector->data = NULL;
        vector->capacity = 0u;

        return true;
    }

    if (!vitte_util_mul_size(
            vector->count,
            vector->element_size,
            &bytes)) {
        return false;
    }

    new_data =
        (unsigned char *)
        vitte_util_realloc(
            vector->data,
            bytes);

    if (new_data == NULL) {
        return false;
    }

    vector->data = new_data;
    vector->capacity = vector->count;

    return true;
}

/* ========================================================================= */
/* Integer parsing                                                           */
/* ========================================================================= */

static int
vitte_util_digit_value(
    unsigned char character)
{
    if (character >= (unsigned char)'0' &&
        character <= (unsigned char)'9') {
        return (int)(
            character -
            (unsigned char)'0');
    }

    if (character >= (unsigned char)'a' &&
        character <= (unsigned char)'z') {
        return 10 +
               (int)(
                   character -
                   (unsigned char)'a');
    }

    if (character >= (unsigned char)'A' &&
        character <= (unsigned char)'Z') {
        return 10 +
               (int)(
                   character -
                   (unsigned char)'A');
    }

    return -1;
}

bool
vitte_util_parse_u64(
    const char *data,
    size_t length,
    unsigned int base,
    uint64_t *out_value)
{
    uint64_t value;
    size_t index;
    bool saw_digit;

    if (data == NULL ||
        out_value == NULL ||
        length == 0u ||
        base < 2u ||
        base > 36u) {
        return false;
    }

    value = UINT64_C(0);
    saw_digit = false;

    for (index = 0u;
         index < length;
         ++index) {
        int digit;

        if (data[index] == '_') {
            /*
             * Separators may only occur between digits.
             */
            if (!saw_digit ||
                index + 1u >= length) {
                return false;
            }

            continue;
        }

        digit =
            vitte_util_digit_value(
                (unsigned char)data[index]);

        if (digit < 0 ||
            (unsigned int)digit >= base) {
            return false;
        }

        if (value >
            (UINT64_MAX -
             (uint64_t)digit) /
                (uint64_t)base) {
            return false;
        }

        value =
            value * (uint64_t)base +
            (uint64_t)digit;

        saw_digit = true;
    }

    if (!saw_digit) {
        return false;
    }

    *out_value = value;

    return true;
}

bool
vitte_util_parse_i64(
    const char *data,
    size_t length,
    unsigned int base,
    int64_t *out_value)
{
    bool negative;
    size_t offset;
    uint64_t magnitude;
    uint64_t limit;

    if (data == NULL ||
        out_value == NULL ||
        length == 0u) {
        return false;
    }

    negative = false;
    offset = 0u;

    if (data[0] == '+' ||
        data[0] == '-') {
        negative =
            data[0] == '-';

        offset = 1u;

        if (offset == length) {
            return false;
        }
    }

    if (!vitte_util_parse_u64(
            data + offset,
            length - offset,
            base,
            &magnitude)) {
        return false;
    }

    if (negative) {
        limit =
            (uint64_t)INT64_MAX +
            UINT64_C(1);

        if (magnitude > limit) {
            return false;
        }

        if (magnitude == limit) {
            *out_value = INT64_MIN;
        } else {
            *out_value =
                -(int64_t)magnitude;
        }
    } else {
        if (magnitude >
            (uint64_t)INT64_MAX) {
            return false;
        }

        *out_value =
            (int64_t)magnitude;
    }

    return true;
}

/* ========================================================================= */
/* Sorting                                                                   */
/* ========================================================================= */

static void
vitte_util_swap_bytes(
    unsigned char *left,
    unsigned char *right,
    size_t size)
{
    size_t index;

    if (left == right) {
        return;
    }

    for (index = 0u;
         index < size;
         ++index) {
        const unsigned char temporary =
            left[index];

        left[index] = right[index];
        right[index] = temporary;
    }
}

static void
vitte_util_insertion_sort(
    unsigned char *base,
    size_t count,
    size_t element_size,
    vitte_util_compare_fn compare,
    void *user_data)
{
    size_t index;

    for (index = 1u;
         index < count;
         ++index) {
        size_t current;

        current = index;

        while (current > 0u) {
            unsigned char *left =
                base +
                ((current - 1u) *
                 element_size);

            unsigned char *right =
                base +
                (current *
                 element_size);

            if (compare(
                    left,
                    right,
                    user_data) <= 0) {
                break;
            }

            vitte_util_swap_bytes(
                left,
                right,
                element_size);

            current--;
        }
    }
}

bool
vitte_util_sort(
    void *base,
    size_t count,
    size_t element_size,
    vitte_util_compare_fn compare,
    void *user_data)
{
    if (element_size == 0u ||
        compare == NULL) {
        return false;
    }

    if (count == 0u ||
        count == 1u) {
        return true;
    }

    if (base == NULL) {
        return false;
    }

    if (count >
        SIZE_MAX / element_size) {
        return false;
    }

    /*
     * Stable, allocation-free insertion sort.
     *
     * Compiler utility collections using this generic helper are generally
     * small. Performance-critical large collections should use specialized
     * typed sorting where appropriate.
     */
    vitte_util_insertion_sort(
        (unsigned char *)base,
        count,
        element_size,
        compare,
        user_data);

    return true;
}

const void *
vitte_util_binary_search(
    const void *key,
    const void *base,
    size_t count,
    size_t element_size,
    vitte_util_compare_fn compare,
    void *user_data)
{
    const unsigned char *bytes;
    size_t low;
    size_t high;

    if (key == NULL ||
        compare == NULL ||
        element_size == 0u) {
        return NULL;
    }

    if (count == 0u) {
        return NULL;
    }

    if (base == NULL ||
        count > SIZE_MAX / element_size) {
        return NULL;
    }

    bytes =
        (const unsigned char *)base;

    low = 0u;
    high = count;

    while (low < high) {
        const size_t middle =
            low +
            ((high - low) / 2u);

        const void *element =
            bytes +
            (middle * element_size);

        const int result =
            compare(
                key,
                element,
                user_data);

        if (result < 0) {
            high = middle;
        } else if (result > 0) {
            low = middle + 1u;
        } else {
            return element;
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_util_translation_unit_anchor(void)
{
}
