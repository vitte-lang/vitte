/*
 * Vitte Compiler
 * src/source/source.c
 *
 * Canonical source-management implementation.
 *
 * Public contract: source.h
 *
 * Responsibilities:
 *   - source context lifecycle;
 *   - immutable source registration;
 *   - source ownership/copying;
 *   - stable source IDs;
 *   - path/name storage;
 *   - UTF-8 aware line indexing;
 *   - byte-offset -> line/column conversion;
 *   - line/column -> byte-offset conversion;
 *   - source spans;
 *   - line extraction;
 *   - source slicing;
 *   - CR/LF/CRLF handling;
 *   - deterministic lookup;
 *   - validation;
 *   - deterministic fingerprints;
 *   - statistics;
 *   - bounded allocations and overflow protection.
 *
 * ISO C17.
 */

#include "source.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Internal constants                                                        */
/* ========================================================================= */

#define VITTE_SOURCE_UTF8_REPLACEMENT UINT32_C(0xfffd)

/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

static bool
vitte_source_context_valid(
    const vitte_source_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    if (context->magic != VITTE_SOURCE_MAGIC) {
        return false;
    }

    if (context->state <= VITTE_SOURCE_STATE_INVALID ||
        context->state >= VITTE_SOURCE_STATE_DESTROYED) {
        return false;
    }

    return true;
}

static bool
vitte_source_fail(
    vitte_source_context_t *context,
    vitte_source_error_t error)
{
    if (context != NULL) {
        context->last_error = error;

        if (error != VITTE_SOURCE_ERROR_NONE) {
            context->state = VITTE_SOURCE_STATE_FAILED;
        }
    }

    return false;
}

static bool
vitte_source_size_add(
    size_t left,
    size_t right,
    size_t *result)
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
vitte_source_size_mul(
    size_t left,
    size_t right,
    size_t *result)
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
vitte_source_hash_bytes(
    uint64_t hash,
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    size_t index;

    if (data == NULL && length != 0u) {
        return hash;
    }

    bytes = (const unsigned char *)data;

    for (index = 0u; index < length; ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= VITTE_SOURCE_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_source_hash_u64(
    uint64_t hash,
    uint64_t value)
{
    unsigned int shift;

    for (shift = 0u; shift < 64u; shift += 8u) {
        const unsigned char byte =
            (unsigned char)((value >> shift) & UINT64_C(0xff));

        hash ^= (uint64_t)byte;
        hash *= VITTE_SOURCE_FNV_PRIME;
    }

    return hash;
}

static uint64_t
vitte_source_hash_size(
    uint64_t hash,
    size_t value)
{
    return vitte_source_hash_u64(hash, (uint64_t)value);
}

static char *
vitte_source_duplicate_bytes(
    vitte_source_context_t *context,
    const char *data,
    size_t length,
    bool nul_terminate)
{
    char *copy;
    size_t allocation_size;

    if (data == NULL && length != 0u) {
        return NULL;
    }

    allocation_size = length;

    if (nul_terminate) {
        if (!vitte_source_size_add(length, 1u, &allocation_size)) {
            if (context != NULL) {
                context->last_error = VITTE_SOURCE_ERROR_OVERFLOW;
            }
            return NULL;
        }
    }

    if (allocation_size == 0u) {
        allocation_size = 1u;
    }

    copy = (char *)malloc(allocation_size);

    if (copy == NULL) {
        if (context != NULL) {
            context->stats.allocation_failures++;
            context->last_error = VITTE_SOURCE_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }

    if (context != NULL) {
        context->stats.allocations++;
    }

    if (length != 0u) {
        memcpy(copy, data, length);
    }

    if (nul_terminate) {
        copy[length] = '\0';
    }

    return copy;
}

static void
vitte_source_entry_clear(
    vitte_source_entry_t *source)
{
    if (source == NULL) {
        return;
    }

    free(source->name);
    free(source->path);
    free(source->data);
    free(source->line_offsets);

    memset(source, 0, sizeof(*source));
}

static bool
vitte_source_reserve_sources(
    vitte_source_context_t *context,
    size_t required)
{
    vitte_source_entry_t *new_sources;
    size_t new_capacity;
    size_t allocation_size;

    if (context == NULL) {
        return false;
    }

    if (required <= context->source_capacity) {
        return true;
    }

    if (required > context->max_sources) {
        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_SOURCE_LIMIT);
    }

    new_capacity = context->source_capacity;

    if (new_capacity == 0u) {
        new_capacity = VITTE_SOURCE_DEFAULT_INITIAL_CAPACITY;
    }

    while (new_capacity < required) {
        size_t doubled;

        if (!vitte_source_size_mul(new_capacity, 2u, &doubled)) {
            new_capacity = required;
            break;
        }

        new_capacity = doubled;

        if (new_capacity > context->max_sources) {
            new_capacity = context->max_sources;
            break;
        }
    }

    if (new_capacity < required) {
        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_SOURCE_LIMIT);
    }

    if (!vitte_source_size_mul(
            new_capacity,
            sizeof(*new_sources),
            &allocation_size)) {
        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_OVERFLOW);
    }

    new_sources = (vitte_source_entry_t *)realloc(
        context->sources,
        allocation_size);

    if (new_sources == NULL) {
        context->stats.allocation_failures++;

        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_OUT_OF_MEMORY);
    }

    if (context->sources == NULL) {
        context->stats.allocations++;
    } else {
        context->stats.reallocations++;
    }

    if (new_capacity > context->source_capacity) {
        const size_t old_capacity = context->source_capacity;
        const size_t added = new_capacity - old_capacity;

        memset(
            new_sources + old_capacity,
            0,
            added * sizeof(*new_sources));
    }

    context->sources = new_sources;
    context->source_capacity = new_capacity;

    return true;
}

static bool
vitte_source_utf8_continuation(
    unsigned char byte)
{
    return (byte & 0xc0u) == 0x80u;
}

static bool
vitte_source_utf8_decode(
    const char *data,
    size_t length,
    size_t offset,
    uint32_t *codepoint,
    size_t *width)
{
    unsigned char first;

    if (data == NULL ||
        codepoint == NULL ||
        width == NULL ||
        offset >= length) {
        return false;
    }

    first = (unsigned char)data[offset];

    if (first <= 0x7fu) {
        *codepoint = (uint32_t)first;
        *width = 1u;
        return true;
    }

    if (first >= 0xc2u && first <= 0xdfu) {
        unsigned char second;

        if (offset + 1u >= length) {
            return false;
        }

        second = (unsigned char)data[offset + 1u];

        if (!vitte_source_utf8_continuation(second)) {
            return false;
        }

        *codepoint =
            ((uint32_t)(first & 0x1fu) << 6u) |
            (uint32_t)(second & 0x3fu);

        *width = 2u;
        return true;
    }

    if (first >= 0xe0u && first <= 0xefu) {
        unsigned char second;
        unsigned char third;
        uint32_t value;

        if (offset + 2u >= length) {
            return false;
        }

        second = (unsigned char)data[offset + 1u];
        third = (unsigned char)data[offset + 2u];

        if (!vitte_source_utf8_continuation(second) ||
            !vitte_source_utf8_continuation(third)) {
            return false;
        }

        if (first == 0xe0u && second < 0xa0u) {
            return false;
        }

        if (first == 0xedu && second >= 0xa0u) {
            return false;
        }

        value =
            ((uint32_t)(first & 0x0fu) << 12u) |
            ((uint32_t)(second & 0x3fu) << 6u) |
            (uint32_t)(third & 0x3fu);

        if (value >= UINT32_C(0xd800) &&
            value <= UINT32_C(0xdfff)) {
            return false;
        }

        *codepoint = value;
        *width = 3u;
        return true;
    }

    if (first >= 0xf0u && first <= 0xf4u) {
        unsigned char second;
        unsigned char third;
        unsigned char fourth;
        uint32_t value;

        if (offset + 3u >= length) {
            return false;
        }

        second = (unsigned char)data[offset + 1u];
        third = (unsigned char)data[offset + 2u];
        fourth = (unsigned char)data[offset + 3u];

        if (!vitte_source_utf8_continuation(second) ||
            !vitte_source_utf8_continuation(third) ||
            !vitte_source_utf8_continuation(fourth)) {
            return false;
        }

        if (first == 0xf0u && second < 0x90u) {
            return false;
        }

        if (first == 0xf4u && second > 0x8fu) {
            return false;
        }

        value =
            ((uint32_t)(first & 0x07u) << 18u) |
            ((uint32_t)(second & 0x3fu) << 12u) |
            ((uint32_t)(third & 0x3fu) << 6u) |
            (uint32_t)(fourth & 0x3fu);

        if (value > UINT32_C(0x10ffff)) {
            return false;
        }

        *codepoint = value;
        *width = 4u;
        return true;
    }

    return false;
}

static bool
vitte_source_validate_utf8_bytes(
    const char *data,
    size_t length)
{
    size_t offset;

    if (data == NULL && length != 0u) {
        return false;
    }

    offset = 0u;

    while (offset < length) {
        uint32_t codepoint;
        size_t width;

        if (!vitte_source_utf8_decode(
                data,
                length,
                offset,
                &codepoint,
                &width)) {
            return false;
        }

        (void)codepoint;

        if (width == 0u || width > length - offset) {
            return false;
        }

        offset += width;
    }

    return true;
}

static bool
vitte_source_build_line_index(
    vitte_source_context_t *context,
    vitte_source_entry_t *source)
{
    size_t line_count;
    size_t offset;
    size_t *offsets;
    size_t allocation_size;
    size_t line_index;

    if (context == NULL || source == NULL) {
        return false;
    }

    /*
     * Even an empty source has one logical line.
     */
    line_count = 1u;
    offset = 0u;

    while (offset < source->length) {
        const unsigned char byte =
            (unsigned char)source->data[offset];

        if (byte == (unsigned char)'\r') {
            line_count++;

            if (offset + 1u < source->length &&
                source->data[offset + 1u] == '\n') {
                offset += 2u;
            } else {
                offset++;
            }

            continue;
        }

        if (byte == (unsigned char)'\n') {
            line_count++;
            offset++;
            continue;
        }

        {
            uint32_t codepoint;
            size_t width;

            if (!vitte_source_utf8_decode(
                    source->data,
                    source->length,
                    offset,
                    &codepoint,
                    &width)) {
                return vitte_source_fail(
                    context,
                    VITTE_SOURCE_ERROR_INVALID_UTF8);
            }

            (void)codepoint;
            offset += width;
        }
    }

    if (!vitte_source_size_mul(
            line_count,
            sizeof(*offsets),
            &allocation_size)) {
        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_OVERFLOW);
    }

    offsets = (size_t *)malloc(allocation_size);

    if (offsets == NULL) {
        context->stats.allocation_failures++;

        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_OUT_OF_MEMORY);
    }

    context->stats.allocations++;

    offsets[0] = 0u;

    line_index = 1u;
    offset = 0u;

    while (offset < source->length) {
        const unsigned char byte =
            (unsigned char)source->data[offset];

        if (byte == (unsigned char)'\r') {
            if (offset + 1u < source->length &&
                source->data[offset + 1u] == '\n') {
                offset += 2u;
            } else {
                offset++;
            }

            if (line_index < line_count) {
                offsets[line_index++] = offset;
            }

            continue;
        }

        if (byte == (unsigned char)'\n') {
            offset++;

            if (line_index < line_count) {
                offsets[line_index++] = offset;
            }

            continue;
        }

        {
            uint32_t codepoint;
            size_t width;

            if (!vitte_source_utf8_decode(
                    source->data,
                    source->length,
                    offset,
                    &codepoint,
                    &width)) {
                free(offsets);

                return vitte_source_fail(
                    context,
                    VITTE_SOURCE_ERROR_INVALID_UTF8);
            }

            (void)codepoint;
            offset += width;
        }
    }

    if (line_index != line_count) {
        free(offsets);

        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_CORRUPTION);
    }

    source->line_offsets = offsets;
    source->line_count = line_count;

    return true;
}

static size_t
vitte_source_line_index_for_offset(
    const vitte_source_entry_t *source,
    size_t offset)
{
    size_t low;
    size_t high;

    if (source == NULL ||
        source->line_offsets == NULL ||
        source->line_count == 0u) {
        return 0u;
    }

    low = 0u;
    high = source->line_count;

    while (low + 1u < high) {
        const size_t middle =
            low + ((high - low) / 2u);

        if (source->line_offsets[middle] <= offset) {
            low = middle;
        } else {
            high = middle;
        }
    }

    return low;
}

static size_t
vitte_source_line_content_end(
    const vitte_source_entry_t *source,
    size_t line_index)
{
    size_t end;

    if (source == NULL ||
        line_index >= source->line_count) {
        return 0u;
    }

    if (line_index + 1u < source->line_count) {
        end = source->line_offsets[line_index + 1u];
    } else {
        end = source->length;
    }

    if (end > source->line_offsets[line_index]) {
        if (source->data[end - 1u] == '\n') {
            end--;

            if (end > source->line_offsets[line_index] &&
                source->data[end - 1u] == '\r') {
                end--;
            }
        } else if (source->data[end - 1u] == '\r') {
            end--;
        }
    }

    return end;
}

static bool
vitte_source_compute_column(
    const vitte_source_entry_t *source,
    size_t line_start,
    size_t offset,
    size_t *column)
{
    size_t cursor;
    size_t result;

    if (source == NULL ||
        column == NULL ||
        line_start > offset ||
        offset > source->length) {
        return false;
    }

    cursor = line_start;
    result = 1u;

    while (cursor < offset) {
        uint32_t codepoint;
        size_t width;

        if (source->data[cursor] == '\r' ||
            source->data[cursor] == '\n') {
            break;
        }

        if (!vitte_source_utf8_decode(
                source->data,
                source->length,
                cursor,
                &codepoint,
                &width)) {
            return false;
        }

        (void)codepoint;

        if (width > offset - cursor) {
            return false;
        }

        cursor += width;
        result++;
    }

    if (cursor != offset) {
        return false;
    }

    *column = result;
    return true;
}

static const vitte_source_entry_t *
vitte_source_get_internal(
    const vitte_source_context_t *context,
    vitte_source_id_t source_id)
{
    size_t index;

    if (!vitte_source_context_valid(context)) {
        return NULL;
    }

    if (source_id == VITTE_SOURCE_INVALID_ID) {
        return NULL;
    }

    index = (size_t)(source_id - UINT32_C(1));

    if (index >= context->source_count) {
        return NULL;
    }

    if (context->sources[index].id != source_id) {
        return NULL;
    }

    return &context->sources[index];
}

/* ========================================================================= */
/* Names                                                                     */
/* ========================================================================= */

const char *
vitte_source_error_name(
    vitte_source_error_t error)
{
    switch (error) {
        case VITTE_SOURCE_ERROR_NONE:
            return "none";

        case VITTE_SOURCE_ERROR_INVALID_ARGUMENT:
            return "invalid_argument";

        case VITTE_SOURCE_ERROR_INVALID_CONTEXT:
            return "invalid_context";

        case VITTE_SOURCE_ERROR_INVALID_STATE:
            return "invalid_state";

        case VITTE_SOURCE_ERROR_INVALID_SOURCE:
            return "invalid_source";

        case VITTE_SOURCE_ERROR_INVALID_SOURCE_ID:
            return "invalid_source_id";

        case VITTE_SOURCE_ERROR_INVALID_UTF8:
            return "invalid_utf8";

        case VITTE_SOURCE_ERROR_INVALID_OFFSET:
            return "invalid_offset";

        case VITTE_SOURCE_ERROR_INVALID_LINE:
            return "invalid_line";

        case VITTE_SOURCE_ERROR_INVALID_COLUMN:
            return "invalid_column";

        case VITTE_SOURCE_ERROR_INVALID_SPAN:
            return "invalid_span";

        case VITTE_SOURCE_ERROR_DUPLICATE_PATH:
            return "duplicate_path";

        case VITTE_SOURCE_ERROR_SOURCE_LIMIT:
            return "source_limit";

        case VITTE_SOURCE_ERROR_SOURCE_TOO_LARGE:
            return "source_too_large";

        case VITTE_SOURCE_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_SOURCE_ERROR_OUT_OF_MEMORY:
            return "out_of_memory";

        case VITTE_SOURCE_ERROR_VALIDATION:
            return "validation";

        case VITTE_SOURCE_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_SOURCE_ERROR_INTERNAL:
            return "internal";

        case VITTE_SOURCE_ERROR_COUNT:
            return "count";
    }

    return "unknown";
}

const char *
vitte_source_state_name(
    vitte_source_state_t state)
{
    switch (state) {
        case VITTE_SOURCE_STATE_INVALID:
            return "invalid";

        case VITTE_SOURCE_STATE_READY:
            return "ready";

        case VITTE_SOURCE_STATE_FAILED:
            return "failed";

        case VITTE_SOURCE_STATE_DESTROYED:
            return "destroyed";

        case VITTE_SOURCE_STATE_COUNT:
            return "count";
    }

    return "unknown";
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_source_init(
    vitte_source_context_t *context)
{
    if (context == NULL) {
        return false;
    }

    memset(context, 0, sizeof(*context));

    context->magic = VITTE_SOURCE_MAGIC;
    context->state = VITTE_SOURCE_STATE_READY;
    context->last_error = VITTE_SOURCE_ERROR_NONE;

    context->max_sources =
        VITTE_SOURCE_DEFAULT_MAX_SOURCES;

    context->max_source_bytes =
        VITTE_SOURCE_DEFAULT_MAX_SOURCE_BYTES;

    context->generation = UINT64_C(1);

    return true;
}

bool
vitte_source_reset(
    vitte_source_context_t *context)
{
    size_t index;

    if (!vitte_source_context_valid(context)) {
        return false;
    }

    for (index = 0u;
         index < context->source_count;
         ++index) {
        vitte_source_entry_clear(
            &context->sources[index]);
    }

    context->source_count = 0u;

    context->state = VITTE_SOURCE_STATE_READY;
    context->last_error = VITTE_SOURCE_ERROR_NONE;

    context->stats.resets++;

    if (context->generation != UINT64_MAX) {
        context->generation++;
    }

    return true;
}

void
vitte_source_destroy(
    vitte_source_context_t *context)
{
    size_t index;

    if (context == NULL) {
        return;
    }

    if (context->magic == VITTE_SOURCE_DEAD_MAGIC &&
        context->state == VITTE_SOURCE_STATE_DESTROYED) {
        return;
    }

    if (context->magic == VITTE_SOURCE_MAGIC) {
        for (index = 0u;
             index < context->source_count;
             ++index) {
            vitte_source_entry_clear(
                &context->sources[index]);
        }

        free(context->sources);
    }

    memset(context, 0, sizeof(*context));

    context->magic = VITTE_SOURCE_DEAD_MAGIC;
    context->state = VITTE_SOURCE_STATE_DESTROYED;
}

/* ========================================================================= */
/* Context                                                                   */
/* ========================================================================= */

bool
vitte_source_is_valid(
    const vitte_source_context_t *context)
{
    return vitte_source_context_valid(context);
}

vitte_source_error_t
vitte_source_last_error(
    const vitte_source_context_t *context)
{
    if (!vitte_source_context_valid(context)) {
        return VITTE_SOURCE_ERROR_INVALID_CONTEXT;
    }

    return context->last_error;
}

uint64_t
vitte_source_generation(
    const vitte_source_context_t *context)
{
    if (!vitte_source_context_valid(context)) {
        return UINT64_C(0);
    }

    return context->generation;
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

bool
vitte_source_set_limits(
    vitte_source_context_t *context,
    size_t max_sources,
    size_t max_source_bytes)
{
    if (!vitte_source_context_valid(context)) {
        return false;
    }

    if (context->state != VITTE_SOURCE_STATE_READY) {
        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_INVALID_STATE);
    }

    if (max_sources == 0u ||
        max_source_bytes == 0u ||
        max_sources < context->source_count) {
        context->last_error =
            VITTE_SOURCE_ERROR_INVALID_ARGUMENT;
        return false;
    }

    context->max_sources = max_sources;
    context->max_source_bytes = max_source_bytes;
    context->last_error = VITTE_SOURCE_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Registration                                                              */
/* ========================================================================= */

bool
vitte_source_add(
    vitte_source_context_t *context,
    const char *name,
    const char *path,
    const char *data,
    size_t length,
    vitte_source_id_t *out_source_id)
{
    vitte_source_entry_t source;
    size_t name_length;
    size_t path_length;
    size_t index;

    if (!vitte_source_context_valid(context)) {
        return false;
    }

    if (context->state != VITTE_SOURCE_STATE_READY) {
        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_INVALID_STATE);
    }

    if (data == NULL && length != 0u) {
        context->last_error =
            VITTE_SOURCE_ERROR_INVALID_ARGUMENT;
        return false;
    }

    if (name == NULL) {
        name = "";
    }

    if (path == NULL) {
        path = "";
    }

    if (length > context->max_source_bytes) {
        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_SOURCE_TOO_LARGE);
    }

    if (context->source_count >= context->max_sources) {
        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_SOURCE_LIMIT);
    }

    if (context->source_count >=
        (size_t)UINT32_MAX - 1u) {
        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_SOURCE_LIMIT);
    }

    name_length = strlen(name);
    path_length = strlen(path);

    /*
     * A non-empty canonical path must be unique.
     */
    if (path_length != 0u) {
        for (index = 0u;
             index < context->source_count;
             ++index) {
            const vitte_source_entry_t *existing =
                &context->sources[index];

            if (existing->path != NULL &&
                existing->path_length == path_length &&
                memcmp(
                    existing->path,
                    path,
                    path_length) == 0) {
                context->last_error =
                    VITTE_SOURCE_ERROR_DUPLICATE_PATH;
                return false;
            }
        }
    }

    if (!vitte_source_validate_utf8_bytes(data, length)) {
        context->last_error =
            VITTE_SOURCE_ERROR_INVALID_UTF8;
        return false;
    }

    memset(&source, 0, sizeof(source));

    source.id =
        (vitte_source_id_t)(
            context->source_count + 1u);

    source.name = vitte_source_duplicate_bytes(
        context,
        name,
        name_length,
        true);

    if (source.name == NULL) {
        return vitte_source_fail(
            context,
            context->last_error != VITTE_SOURCE_ERROR_NONE
                ? context->last_error
                : VITTE_SOURCE_ERROR_OUT_OF_MEMORY);
    }

    source.name_length = name_length;

    source.path = vitte_source_duplicate_bytes(
        context,
        path,
        path_length,
        true);

    if (source.path == NULL) {
        vitte_source_entry_clear(&source);

        return vitte_source_fail(
            context,
            context->last_error != VITTE_SOURCE_ERROR_NONE
                ? context->last_error
                : VITTE_SOURCE_ERROR_OUT_OF_MEMORY);
    }

    source.path_length = path_length;

    source.data = vitte_source_duplicate_bytes(
        context,
        data,
        length,
        true);

    if (source.data == NULL) {
        vitte_source_entry_clear(&source);

        return vitte_source_fail(
            context,
            context->last_error != VITTE_SOURCE_ERROR_NONE
                ? context->last_error
                : VITTE_SOURCE_ERROR_OUT_OF_MEMORY);
    }

    source.length = length;

    if (!vitte_source_build_line_index(
            context,
            &source)) {
        vitte_source_entry_clear(&source);
        return false;
    }

    if (!vitte_source_reserve_sources(
            context,
            context->source_count + 1u)) {
        vitte_source_entry_clear(&source);
        return false;
    }

    context->sources[context->source_count] = source;
    context->source_count++;

    context->stats.sources_added++;
    context->stats.bytes_added += (uint64_t)length;
    context->stats.lines_indexed +=
        (uint64_t)source.line_count;

    context->last_error = VITTE_SOURCE_ERROR_NONE;

    if (out_source_id != NULL) {
        *out_source_id = source.id;
    }

    return true;
}

bool
vitte_source_add_cstr(
    vitte_source_context_t *context,
    const char *name,
    const char *path,
    const char *data,
    vitte_source_id_t *out_source_id)
{
    if (data == NULL) {
        return vitte_source_add(
            context,
            name,
            path,
            "",
            0u,
            out_source_id);
    }

    return vitte_source_add(
        context,
        name,
        path,
        data,
        strlen(data),
        out_source_id);
}

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

const vitte_source_entry_t *
vitte_source_get(
    const vitte_source_context_t *context,
    vitte_source_id_t source_id)
{
    return vitte_source_get_internal(
        context,
        source_id);
}

const vitte_source_entry_t *
vitte_source_find_path(
    const vitte_source_context_t *context,
    const char *path)
{
    size_t path_length;
    size_t index;

    if (!vitte_source_context_valid(context) ||
        path == NULL) {
        return NULL;
    }

    path_length = strlen(path);

    for (index = 0u;
         index < context->source_count;
         ++index) {
        const vitte_source_entry_t *source =
            &context->sources[index];

        if (source->path_length == path_length &&
            source->path != NULL &&
            memcmp(
                source->path,
                path,
                path_length) == 0) {
            return source;
        }
    }

    return NULL;
}

const vitte_source_entry_t *
vitte_source_find_name(
    const vitte_source_context_t *context,
    const char *name)
{
    size_t name_length;
    size_t index;

    if (!vitte_source_context_valid(context) ||
        name == NULL) {
        return NULL;
    }

    name_length = strlen(name);

    for (index = 0u;
         index < context->source_count;
         ++index) {
        const vitte_source_entry_t *source =
            &context->sources[index];

        if (source->name_length == name_length &&
            source->name != NULL &&
            memcmp(
                source->name,
                name,
                name_length) == 0) {
            return source;
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Location conversion                                                       */
/* ========================================================================= */

bool
vitte_source_location_from_offset(
    const vitte_source_context_t *context,
    vitte_source_id_t source_id,
    size_t offset,
    vitte_source_location_t *location)
{
    const vitte_source_entry_t *source;
    size_t line_index;
    size_t column;

    if (location == NULL) {
        return false;
    }

    memset(location, 0, sizeof(*location));

    source = vitte_source_get_internal(
        context,
        source_id);

    if (source == NULL) {
        return false;
    }

    if (offset > source->length) {
        return false;
    }

    line_index =
        vitte_source_line_index_for_offset(
            source,
            offset);

    if (line_index >= source->line_count) {
        return false;
    }

    if (!vitte_source_compute_column(
            source,
            source->line_offsets[line_index],
            offset,
            &column)) {
        return false;
    }

    location->source_id = source_id;
    location->offset = offset;
    location->line = line_index + 1u;
    location->column = column;
    location->valid = true;

    return true;
}

bool
vitte_source_offset_from_location(
    const vitte_source_context_t *context,
    vitte_source_id_t source_id,
    size_t line,
    size_t column,
    size_t *offset)
{
    const vitte_source_entry_t *source;
    size_t line_index;
    size_t cursor;
    size_t current_column;
    size_t content_end;

    if (offset == NULL) {
        return false;
    }

    source = vitte_source_get_internal(
        context,
        source_id);

    if (source == NULL ||
        line == 0u ||
        column == 0u) {
        return false;
    }

    line_index = line - 1u;

    if (line_index >= source->line_count) {
        return false;
    }

    cursor = source->line_offsets[line_index];
    content_end =
        vitte_source_line_content_end(
            source,
            line_index);

    current_column = 1u;

    while (current_column < column &&
           cursor < content_end) {
        uint32_t codepoint;
        size_t width;

        if (!vitte_source_utf8_decode(
                source->data,
                source->length,
                cursor,
                &codepoint,
                &width)) {
            return false;
        }

        (void)codepoint;

        cursor += width;
        current_column++;
    }

    /*
     * column == codepoint_count + 1 addresses the position immediately
     * after the last character on the line.
     */
    if (current_column != column) {
        return false;
    }

    *offset = cursor;
    return true;
}

/* ========================================================================= */
/* Line access                                                               */
/* ========================================================================= */

bool
vitte_source_get_line(
    const vitte_source_context_t *context,
    vitte_source_id_t source_id,
    size_t line,
    vitte_source_slice_t *slice)
{
    const vitte_source_entry_t *source;
    size_t line_index;
    size_t begin;
    size_t end;

    if (slice == NULL) {
        return false;
    }

    memset(slice, 0, sizeof(*slice));

    source = vitte_source_get_internal(
        context,
        source_id);

    if (source == NULL || line == 0u) {
        return false;
    }

    line_index = line - 1u;

    if (line_index >= source->line_count) {
        return false;
    }

    begin = source->line_offsets[line_index];
    end = vitte_source_line_content_end(
        source,
        line_index);

    if (end < begin ||
        end > source->length) {
        return false;
    }

    slice->data = source->data + begin;
    slice->length = end - begin;
    slice->source_id = source_id;
    slice->begin = begin;
    slice->end = end;
    slice->valid = true;

    return true;
}

/* ========================================================================= */
/* Slicing                                                                   */
/* ========================================================================= */

bool
vitte_source_slice(
    const vitte_source_context_t *context,
    vitte_source_id_t source_id,
    size_t begin,
    size_t end,
    vitte_source_slice_t *slice)
{
    const vitte_source_entry_t *source;

    if (slice == NULL) {
        return false;
    }

    memset(slice, 0, sizeof(*slice));

    source = vitte_source_get_internal(
        context,
        source_id);

    if (source == NULL) {
        return false;
    }

    if (begin > end ||
        end > source->length) {
        return false;
    }

    slice->data = source->data + begin;
    slice->length = end - begin;
    slice->source_id = source_id;
    slice->begin = begin;
    slice->end = end;
    slice->valid = true;

    return true;
}

/* ========================================================================= */
/* Spans                                                                     */
/* ========================================================================= */

bool
vitte_source_make_span(
    const vitte_source_context_t *context,
    vitte_source_id_t source_id,
    size_t begin,
    size_t end,
    vitte_source_span_t *span)
{
    const vitte_source_entry_t *source;

    if (span == NULL) {
        return false;
    }

    memset(span, 0, sizeof(*span));

    source = vitte_source_get_internal(
        context,
        source_id);

    if (source == NULL) {
        return false;
    }

    if (begin > end ||
        end > source->length) {
        return false;
    }

    span->source_id = source_id;
    span->begin = begin;
    span->end = end;
    span->valid = true;

    return true;
}

bool
vitte_source_span_locations(
    const vitte_source_context_t *context,
    vitte_source_span_t span,
    vitte_source_location_t *begin_location,
    vitte_source_location_t *end_location)
{
    if (!span.valid ||
        begin_location == NULL ||
        end_location == NULL) {
        return false;
    }

    if (!vitte_source_location_from_offset(
            context,
            span.source_id,
            span.begin,
            begin_location)) {
        return false;
    }

    if (!vitte_source_location_from_offset(
            context,
            span.source_id,
            span.end,
            end_location)) {
        return false;
    }

    return true;
}

bool
vitte_source_span_slice(
    const vitte_source_context_t *context,
    vitte_source_span_t span,
    vitte_source_slice_t *slice)
{
    if (!span.valid) {
        return false;
    }

    return vitte_source_slice(
        context,
        span.source_id,
        span.begin,
        span.end,
        slice);
}

/* ========================================================================= */
/* UTF-8                                                                     */
/* ========================================================================= */

bool
vitte_source_validate_utf8(
    const vitte_source_context_t *context,
    vitte_source_id_t source_id)
{
    const vitte_source_entry_t *source;

    source = vitte_source_get_internal(
        context,
        source_id);

    if (source == NULL) {
        return false;
    }

    return vitte_source_validate_utf8_bytes(
        source->data,
        source->length);
}

bool
vitte_source_codepoint_at(
    const vitte_source_context_t *context,
    vitte_source_id_t source_id,
    size_t offset,
    uint32_t *codepoint,
    size_t *width)
{
    const vitte_source_entry_t *source;

    source = vitte_source_get_internal(
        context,
        source_id);

    if (source == NULL ||
        codepoint == NULL ||
        width == NULL ||
        offset >= source->length) {
        return false;
    }

    return vitte_source_utf8_decode(
        source->data,
        source->length,
        offset,
        codepoint,
        width);
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_source_validate(
    vitte_source_context_t *context)
{
    size_t index;

    if (!vitte_source_context_valid(context)) {
        return false;
    }

    context->stats.validation_runs++;

    if (context->source_count >
        context->source_capacity) {
        context->stats.validation_failures++;

        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_VALIDATION);
    }

    if (context->source_count >
        context->max_sources) {
        context->stats.validation_failures++;

        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_VALIDATION);
    }

    if (context->source_capacity != 0u &&
        context->sources == NULL) {
        context->stats.validation_failures++;

        return vitte_source_fail(
            context,
            VITTE_SOURCE_ERROR_VALIDATION);
    }

    for (index = 0u;
         index < context->source_count;
         ++index) {
        const vitte_source_entry_t *source =
            &context->sources[index];

        size_t line_index;

        if (source->id !=
            (vitte_source_id_t)(index + 1u)) {
            context->stats.validation_failures++;

            return vitte_source_fail(
                context,
                VITTE_SOURCE_ERROR_VALIDATION);
        }

        if (source->name == NULL ||
            source->path == NULL ||
            source->data == NULL) {
            context->stats.validation_failures++;

            return vitte_source_fail(
                context,
                VITTE_SOURCE_ERROR_VALIDATION);
        }

        if (source->name[source->name_length] != '\0' ||
            source->path[source->path_length] != '\0' ||
            source->data[source->length] != '\0') {
            context->stats.validation_failures++;

            return vitte_source_fail(
                context,
                VITTE_SOURCE_ERROR_VALIDATION);
        }

        if (source->length >
            context->max_source_bytes) {
            context->stats.validation_failures++;

            return vitte_source_fail(
                context,
                VITTE_SOURCE_ERROR_VALIDATION);
        }

        if (source->line_count == 0u ||
            source->line_offsets == NULL ||
            source->line_offsets[0] != 0u) {
            context->stats.validation_failures++;

            return vitte_source_fail(
                context,
                VITTE_SOURCE_ERROR_VALIDATION);
        }

        for (line_index = 0u;
             line_index < source->line_count;
             ++line_index) {
            if (source->line_offsets[line_index] >
                source->length) {
                context->stats.validation_failures++;

                return vitte_source_fail(
                    context,
                    VITTE_SOURCE_ERROR_VALIDATION);
            }

            if (line_index != 0u &&
                source->line_offsets[line_index] <=
                    source->line_offsets[line_index - 1u]) {
                context->stats.validation_failures++;

                return vitte_source_fail(
                    context,
                    VITTE_SOURCE_ERROR_VALIDATION);
            }
        }

        if (!vitte_source_validate_utf8_bytes(
                source->data,
                source->length)) {
            context->stats.validation_failures++;

            return vitte_source_fail(
                context,
                VITTE_SOURCE_ERROR_VALIDATION);
        }
    }

    /*
     * Verify canonical path uniqueness.
     */
    for (index = 0u;
         index < context->source_count;
         ++index) {
        size_t other;

        const vitte_source_entry_t *left =
            &context->sources[index];

        if (left->path_length == 0u) {
            continue;
        }

        for (other = index + 1u;
             other < context->source_count;
             ++other) {
            const vitte_source_entry_t *right =
                &context->sources[other];

            if (left->path_length ==
                    right->path_length &&
                memcmp(
                    left->path,
                    right->path,
                    left->path_length) == 0) {
                context->stats.validation_failures++;

                return vitte_source_fail(
                    context,
                    VITTE_SOURCE_ERROR_VALIDATION);
            }
        }
    }

    context->last_error =
        VITTE_SOURCE_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

uint64_t
vitte_source_fingerprint(
    vitte_source_context_t *context)
{
    uint64_t hash;
    size_t index;

    if (!vitte_source_context_valid(context)) {
        return UINT64_C(0);
    }

    context->stats.hash_runs++;

    hash = VITTE_SOURCE_FNV_OFFSET;

    hash = vitte_source_hash_size(
        hash,
        context->source_count);

    for (index = 0u;
         index < context->source_count;
         ++index) {
        const vitte_source_entry_t *source =
            &context->sources[index];

        /*
         * Deliberately hash canonical source content/metadata rather than
         * pointer addresses or allocator-dependent state.
         */
        hash = vitte_source_hash_u64(
            hash,
            (uint64_t)source->id);

        hash = vitte_source_hash_size(
            hash,
            source->name_length);

        hash = vitte_source_hash_bytes(
            hash,
            source->name,
            source->name_length);

        hash = vitte_source_hash_size(
            hash,
            source->path_length);

        hash = vitte_source_hash_bytes(
            hash,
            source->path,
            source->path_length);

        hash = vitte_source_hash_size(
            hash,
            source->length);

        hash = vitte_source_hash_bytes(
            hash,
            source->data,
            source->length);

        hash = vitte_source_hash_size(
            hash,
            source->line_count);
    }

    return hash;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_source_stats_t
vitte_source_stats(
    const vitte_source_context_t *context)
{
    vitte_source_stats_t empty;

    memset(&empty, 0, sizeof(empty));

    if (!vitte_source_context_valid(context)) {
        return empty;
    }

    return context->stats;
}

/* ========================================================================= */
/* Translation-unit anchor                                                   */
/* ========================================================================= */

void
vitte_source_translation_unit_anchor(void)
{
    /*
     * Deliberately empty.
     *
     * Gives build systems and link tests a stable externally-visible symbol
     * for this translation unit.
     */
}
