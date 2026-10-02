/*
 * Vitte Compiler
 * src/arena/arena.c
 *
 * High-level arena interface.
 *
 * allocator.c implements the low-level block allocator.
 * This module provides the compiler-facing arena abstraction:
 *
 *   - configuration
 *   - memory limits
 *   - allocation accounting
 *   - aligned allocation
 *   - zeroed allocation
 *   - arrays
 *   - object duplication
 *   - strings
 *   - formatted strings
 *   - checkpoints
 *   - rollback
 *   - reset
 *   - trimming
 *   - statistics
 *   - validation
 *
 * Intended users:
 *
 *   lexer
 *   parser
 *   AST
 *   semantic analysis
 *   HIR
 *   IR
 *   diagnostics
 *   temporary compiler passes
 */

#include "arena.h"

#include "allocator.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_ARENA_CONTEXT_MAGIC
#define VITTE_ARENA_CONTEXT_MAGIC \
    UINT64_C(0x5649545445414358)
#endif

#ifndef VITTE_ARENA_CONTEXT_DEAD_MAGIC
#define VITTE_ARENA_CONTEXT_DEAD_MAGIC \
    UINT64_C(0x4445414441435458)
#endif

#ifndef VITTE_ARENA_DEFAULT_LIMIT
#define VITTE_ARENA_DEFAULT_LIMIT SIZE_MAX
#endif

#ifndef VITTE_ARENA_DEFAULT_STRING_LIMIT
#define VITTE_ARENA_DEFAULT_STRING_LIMIT \
    ((size_t)64u * 1024u * 1024u)
#endif

#ifndef VITTE_ARENA_MAX_FORMATTED_STRING
#define VITTE_ARENA_MAX_FORMATTED_STRING \
    ((size_t)64u * 1024u * 1024u)
#endif

/* ========================================================================= */
/* Arithmetic                                                                */
/* ========================================================================= */

static bool
vitte_arena_context_add_overflow(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (left > SIZE_MAX - right) {
        return true;
    }

    *result = left + right;
    return false;
}

static bool
vitte_arena_context_mul_overflow(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (left != 0u &&
        right > SIZE_MAX / left) {
        return true;
    }

    *result = left * right;
    return false;
}

static bool
vitte_arena_context_is_power_of_two(
    size_t value)
{
    return value != 0u &&
           (value & (value - 1u)) == 0u;
}

/* ========================================================================= */
/* Validation helpers                                                        */
/* ========================================================================= */

static bool
vitte_arena_context_magic_valid(
    const vitte_arena_context_t *context)
{
    return context != NULL &&
           context->magic ==
               VITTE_ARENA_CONTEXT_MAGIC;
}

static bool
vitte_arena_context_operational(
    const vitte_arena_context_t *context)
{
    return
        vitte_arena_context_magic_valid(context) &&
        context->state ==
            VITTE_ARENA_STATE_READY;
}

/* ========================================================================= */
/* Error handling                                                            */
/* ========================================================================= */

static void
vitte_arena_context_set_error(
    vitte_arena_context_t *context,
    vitte_arena_context_error_t error)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return;
    }

    context->last_error = error;
}

static void
vitte_arena_context_import_allocator_error(
    vitte_arena_context_t *context)
{
    vitte_arena_error_t error;

    if (!vitte_arena_context_magic_valid(
            context)) {
        return;
    }

    error =
        vitte_arena_last_error(
            &context->allocator);

    switch (error) {
        case VITTE_ARENA_ERROR_NONE:
            context->last_error =
                VITTE_ARENA_CONTEXT_ERROR_NONE;
            break;

        case VITTE_ARENA_ERROR_INVALID_ALIGNMENT:
            context->last_error =
                VITTE_ARENA_CONTEXT_ERROR_INVALID_ALIGNMENT;
            break;

        case VITTE_ARENA_ERROR_OVERFLOW:
            context->last_error =
                VITTE_ARENA_CONTEXT_ERROR_OVERFLOW;
            break;

        case VITTE_ARENA_ERROR_OUT_OF_MEMORY:
            context->last_error =
                VITTE_ARENA_CONTEXT_ERROR_OUT_OF_MEMORY;
            break;

        case VITTE_ARENA_ERROR_INVALID_MARK:
            context->last_error =
                VITTE_ARENA_CONTEXT_ERROR_INVALID_CHECKPOINT;
            break;

        case VITTE_ARENA_ERROR_CORRUPTION:
            context->last_error =
                VITTE_ARENA_CONTEXT_ERROR_CORRUPTION;
            break;

        case VITTE_ARENA_ERROR_INVALID_ARENA:
        case VITTE_ARENA_ERROR_INVALID_ARGUMENT:
        default:
            context->last_error =
                VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT;
            break;
    }
}

vitte_arena_context_error_t
vitte_arena_context_last_error(
    const vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return
            VITTE_ARENA_CONTEXT_ERROR_INVALID_CONTEXT;
    }

    return context->last_error;
}

void
vitte_arena_context_clear_error(
    vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return;
    }

    context->last_error =
        VITTE_ARENA_CONTEXT_ERROR_NONE;

    vitte_arena_clear_error(
        &context->allocator);
}

const char *
vitte_arena_context_error_name(
    vitte_arena_context_error_t error)
{
    switch (error) {
        case VITTE_ARENA_CONTEXT_ERROR_NONE:
            return "none";

        case VITTE_ARENA_CONTEXT_ERROR_INVALID_CONTEXT:
            return "invalid-context";

        case VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT:
            return "invalid-argument";

        case VITTE_ARENA_CONTEXT_ERROR_INVALID_ALIGNMENT:
            return "invalid-alignment";

        case VITTE_ARENA_CONTEXT_ERROR_OVERFLOW:
            return "overflow";

        case VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED:
            return "limit-exceeded";

        case VITTE_ARENA_CONTEXT_ERROR_OUT_OF_MEMORY:
            return "out-of-memory";

        case VITTE_ARENA_CONTEXT_ERROR_INVALID_CHECKPOINT:
            return "invalid-checkpoint";

        case VITTE_ARENA_CONTEXT_ERROR_CORRUPTION:
            return "corruption";

        case VITTE_ARENA_CONTEXT_ERROR_INVALID_STATE:
            return "invalid-state";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_arena_config_t
vitte_arena_config_default(void)
{
    vitte_arena_config_t config;

    memset(&config, 0, sizeof(config));

    config.block_size =
        VITTE_ARENA_DEFAULT_BLOCK_SIZE;

    config.memory_limit =
        VITTE_ARENA_DEFAULT_LIMIT;

    config.string_limit =
        VITTE_ARENA_DEFAULT_STRING_LIMIT;

    config.retain_blocks_on_reset = true;

    return config;
}

bool
vitte_arena_config_validate(
    const vitte_arena_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->block_size != 0u &&
        config->block_size <
            VITTE_ARENA_MIN_BLOCK_SIZE) {
        return false;
    }

    if (config->string_limit == 0u) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_arena_context_init(
    vitte_arena_context_t *context,
    const vitte_arena_config_t *config)
{
    vitte_arena_config_t effective;

    if (context == NULL) {
        return false;
    }

    memset(context, 0, sizeof(*context));

    if (config == NULL) {
        effective =
            vitte_arena_config_default();
    } else {
        effective = *config;
    }

    if (effective.block_size == 0u) {
        effective.block_size =
            VITTE_ARENA_DEFAULT_BLOCK_SIZE;
    }

    if (effective.memory_limit == 0u) {
        effective.memory_limit =
            VITTE_ARENA_DEFAULT_LIMIT;
    }

    if (effective.string_limit == 0u) {
        effective.string_limit =
            VITTE_ARENA_DEFAULT_STRING_LIMIT;
    }

    if (!vitte_arena_config_validate(
            &effective)) {
        return false;
    }

    context->magic =
        VITTE_ARENA_CONTEXT_MAGIC;

    context->state =
        VITTE_ARENA_STATE_UNINITIALIZED;

    context->config = effective;

    context->last_error =
        VITTE_ARENA_CONTEXT_ERROR_NONE;

    if (!vitte_arena_init(
            &context->allocator,
            effective.block_size)) {
        context->state =
            VITTE_ARENA_STATE_FAILED;

        context->last_error =
            VITTE_ARENA_CONTEXT_ERROR_OUT_OF_MEMORY;

        return false;
    }

    context->generation =
        vitte_arena_generation(
            &context->allocator);

    context->allocation_count = 0u;
    context->requested_bytes = 0u;

    context->lifetime_allocation_count = 0u;
    context->lifetime_requested_bytes = 0u;

    context->peak_requested_bytes = 0u;

    context->state =
        VITTE_ARENA_STATE_READY;

    return true;
}

bool
vitte_arena_context_init_default(
    vitte_arena_context_t *context)
{
    vitte_arena_config_t config;

    config =
        vitte_arena_config_default();

    return vitte_arena_context_init(
        context,
        &config);
}

void
vitte_arena_context_destroy(
    vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return;
    }

    context->state =
        VITTE_ARENA_STATE_DESTROYED;

    vitte_arena_destroy(
        &context->allocator);

    context->magic =
        VITTE_ARENA_CONTEXT_DEAD_MAGIC;

    context->generation = 0u;

    context->allocation_count = 0u;
    context->requested_bytes = 0u;

    context->last_error =
        VITTE_ARENA_CONTEXT_ERROR_INVALID_CONTEXT;
}

/* ========================================================================= */
/* Limits                                                                    */
/* ========================================================================= */

static bool
vitte_arena_context_check_limit(
    vitte_arena_context_t *context,
    size_t additional)
{
    size_t total;

    if (!vitte_arena_context_operational(
            context)) {
        return false;
    }

    if (vitte_arena_context_add_overflow(
            context->requested_bytes,
            additional,
            &total)) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_OVERFLOW);

        return false;
    }

    if (context->config.memory_limit !=
            SIZE_MAX &&
        total >
            context->config.memory_limit) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED);

        return false;
    }

    return true;
}

/* ========================================================================= */
/* Accounting                                                                */
/* ========================================================================= */

static void
vitte_arena_context_record_allocation(
    vitte_arena_context_t *context,
    size_t requested)
{
    size_t value;

    if (context == NULL) {
        return;
    }

    if (context->allocation_count !=
        SIZE_MAX) {
        ++context->allocation_count;
    }

    if (context->lifetime_allocation_count !=
        SIZE_MAX) {
        ++context->lifetime_allocation_count;
    }

    if (!vitte_arena_context_add_overflow(
            context->requested_bytes,
            requested,
            &value)) {
        context->requested_bytes = value;
    } else {
        context->requested_bytes = SIZE_MAX;
    }

    if (!vitte_arena_context_add_overflow(
            context->lifetime_requested_bytes,
            requested,
            &value)) {
        context->lifetime_requested_bytes =
            value;
    } else {
        context->lifetime_requested_bytes =
            SIZE_MAX;
    }

    if (context->requested_bytes >
        context->peak_requested_bytes) {
        context->peak_requested_bytes =
            context->requested_bytes;
    }
}

/* ========================================================================= */
/* Core allocation                                                           */
/* ========================================================================= */

void *
vitte_arena_context_alloc_aligned(
    vitte_arena_context_t *context,
    size_t size,
    size_t alignment)
{
    void *memory;

    if (!vitte_arena_context_operational(
            context)) {
        if (vitte_arena_context_magic_valid(
                context)) {
            vitte_arena_context_set_error(
                context,
                VITTE_ARENA_CONTEXT_ERROR_INVALID_STATE);
        }

        return NULL;
    }

    context->last_error =
        VITTE_ARENA_CONTEXT_ERROR_NONE;

    if (alignment != 0u &&
        !vitte_arena_context_is_power_of_two(
            alignment)) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_INVALID_ALIGNMENT);

        return NULL;
    }

    if (!vitte_arena_context_check_limit(
            context,
            size)) {
        return NULL;
    }

    memory =
        vitte_arena_alloc_aligned(
            &context->allocator,
            size,
            alignment);

    if (memory == NULL) {
        vitte_arena_context_import_allocator_error(
            context);

        return NULL;
    }

    vitte_arena_context_record_allocation(
        context,
        size);

    return memory;
}

void *
vitte_arena_context_alloc(
    vitte_arena_context_t *context,
    size_t size)
{
    return vitte_arena_context_alloc_aligned(
        context,
        size,
        0u);
}

/* ========================================================================= */
/* Array allocation                                                          */
/* ========================================================================= */

void *
vitte_arena_context_alloc_array(
    vitte_arena_context_t *context,
    size_t count,
    size_t element_size,
    size_t alignment)
{
    size_t total;

    if (!vitte_arena_context_operational(
            context)) {
        return NULL;
    }

    if (vitte_arena_context_mul_overflow(
            count,
            element_size,
            &total)) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_OVERFLOW);

        return NULL;
    }

    return vitte_arena_context_alloc_aligned(
        context,
        total,
        alignment);
}

void *
vitte_arena_context_calloc(
    vitte_arena_context_t *context,
    size_t count,
    size_t element_size)
{
    void *memory;
    size_t total;

    if (!vitte_arena_context_operational(
            context)) {
        return NULL;
    }

    if (vitte_arena_context_mul_overflow(
            count,
            element_size,
            &total)) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_OVERFLOW);

        return NULL;
    }

    memory =
        vitte_arena_context_alloc(
            context,
            total);

    if (memory == NULL) {
        return NULL;
    }

    if (total != 0u) {
        memset(memory, 0, total);
    }

    return memory;
}

void *
vitte_arena_context_calloc_aligned(
    vitte_arena_context_t *context,
    size_t count,
    size_t element_size,
    size_t alignment)
{
    void *memory;
    size_t total;

    if (!vitte_arena_context_operational(
            context)) {
        return NULL;
    }

    if (vitte_arena_context_mul_overflow(
            count,
            element_size,
            &total)) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_OVERFLOW);

        return NULL;
    }

    memory =
        vitte_arena_context_alloc_aligned(
            context,
            total,
            alignment);

    if (memory == NULL) {
        return NULL;
    }

    if (total != 0u) {
        memset(memory, 0, total);
    }

    return memory;
}

/* ========================================================================= */
/* Duplication                                                               */
/* ========================================================================= */

void *
vitte_arena_context_memdup(
    vitte_arena_context_t *context,
    const void *data,
    size_t size)
{
    void *copy;

    if (!vitte_arena_context_operational(
            context)) {
        return NULL;
    }

    if (data == NULL && size != 0u) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    copy =
        vitte_arena_context_alloc(
            context,
            size);

    if (copy == NULL) {
        return NULL;
    }

    if (size != 0u) {
        memcpy(copy, data, size);
    }

    return copy;
}

/* ========================================================================= */
/* Strings                                                                   */
/* ========================================================================= */

char *
vitte_arena_context_strndup(
    vitte_arena_context_t *context,
    const char *text,
    size_t length)
{
    char *copy;
    size_t allocation_size;

    if (!vitte_arena_context_operational(
            context)) {
        return NULL;
    }

    if (text == NULL && length != 0u) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    if (length >
        context->config.string_limit) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED);

        return NULL;
    }

    if (vitte_arena_context_add_overflow(
            length,
            1u,
            &allocation_size)) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_OVERFLOW);

        return NULL;
    }

    copy = (char *)
        vitte_arena_context_alloc_aligned(
            context,
            allocation_size,
            _Alignof(char));

    if (copy == NULL) {
        return NULL;
    }

    if (length != 0u) {
        memcpy(copy, text, length);
    }

    copy[length] = '\0';

    return copy;
}

char *
vitte_arena_context_strdup(
    vitte_arena_context_t *context,
    const char *text)
{
    size_t length;

    if (!vitte_arena_context_operational(
            context)) {
        return NULL;
    }

    if (text == NULL) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    /*
     * Bounded scan prevents an untrusted string from causing an effectively
     * unbounded strlen().
     */
    length = 0u;

    while (length <=
           context->config.string_limit) {
        if (text[length] == '\0') {
            return
                vitte_arena_context_strndup(
                    context,
                    text,
                    length);
        }

        ++length;
    }

    vitte_arena_context_set_error(
        context,
        VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED);

    return NULL;
}

/* ========================================================================= */
/* Formatted strings                                                         */
/* ========================================================================= */

char *
vitte_arena_context_vprintf(
    vitte_arena_context_t *context,
    const char *format,
    va_list arguments)
{
    va_list copy;
    int required;
    size_t length;
    size_t allocation_size;
    char *buffer;
    int written;

    if (!vitte_arena_context_operational(
            context)) {
        return NULL;
    }

    if (format == NULL) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    va_copy(copy, arguments);

    required =
        vsnprintf(
            NULL,
            0u,
            format,
            copy);

    va_end(copy);

    if (required < 0) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT);

        return NULL;
    }

    length = (size_t)required;

    if (length >
            context->config.string_limit ||
        length >
            VITTE_ARENA_MAX_FORMATTED_STRING) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED);

        return NULL;
    }

    if (vitte_arena_context_add_overflow(
            length,
            1u,
            &allocation_size)) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_OVERFLOW);

        return NULL;
    }

    buffer = (char *)
        vitte_arena_context_alloc_aligned(
            context,
            allocation_size,
            _Alignof(char));

    if (buffer == NULL) {
        return NULL;
    }

    va_copy(copy, arguments);

    written =
        vsnprintf(
            buffer,
            allocation_size,
            format,
            copy);

    va_end(copy);

    if (written < 0 ||
        (size_t)written != length) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_CORRUPTION);

        return NULL;
    }

    return buffer;
}

char *
vitte_arena_context_printf(
    vitte_arena_context_t *context,
    const char *format,
    ...)
{
    va_list arguments;
    char *result;

    va_start(arguments, format);

    result =
        vitte_arena_context_vprintf(
            context,
            format,
            arguments);

    va_end(arguments);

    return result;
}

/* ========================================================================= */
/* Checkpoints                                                               */
/* ========================================================================= */

vitte_arena_checkpoint_t
vitte_arena_context_checkpoint(
    const vitte_arena_context_t *context)
{
    vitte_arena_checkpoint_t checkpoint;

    memset(
        &checkpoint,
        0,
        sizeof(checkpoint));

    if (!vitte_arena_context_operational(
            context)) {
        return checkpoint;
    }

    checkpoint.context = context;

    checkpoint.mark =
        vitte_arena_mark(
            &context->allocator);

    checkpoint.generation =
        context->generation;

    checkpoint.allocation_count =
        context->allocation_count;

    checkpoint.requested_bytes =
        context->requested_bytes;

    return checkpoint;
}

bool
vitte_arena_context_rollback(
    vitte_arena_context_t *context,
    vitte_arena_checkpoint_t checkpoint)
{
    if (!vitte_arena_context_operational(
            context)) {
        return false;
    }

    if (checkpoint.context != context ||
        checkpoint.generation !=
            context->generation) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_INVALID_CHECKPOINT);

        return false;
    }

    if (!vitte_arena_rewind(
            &context->allocator,
            checkpoint.mark)) {
        vitte_arena_context_import_allocator_error(
            context);

        return false;
    }

    context->allocation_count =
        checkpoint.allocation_count;

    context->requested_bytes =
        checkpoint.requested_bytes;

    context->last_error =
        VITTE_ARENA_CONTEXT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

void
vitte_arena_context_reset(
    vitte_arena_context_t *context)
{
    if (!vitte_arena_context_operational(
            context)) {
        return;
    }

    if (context->config.retain_blocks_on_reset) {
        vitte_arena_reset(
            &context->allocator);
    } else {
        vitte_arena_reset_and_trim(
            &context->allocator);
    }

    context->generation =
        vitte_arena_generation(
            &context->allocator);

    context->allocation_count = 0u;
    context->requested_bytes = 0u;

    context->last_error =
        VITTE_ARENA_CONTEXT_ERROR_NONE;
}

void
vitte_arena_context_trim(
    vitte_arena_context_t *context)
{
    if (!vitte_arena_context_operational(
            context)) {
        return;
    }

    vitte_arena_trim(
        &context->allocator);
}

void
vitte_arena_context_reset_and_trim(
    vitte_arena_context_t *context)
{
    if (!vitte_arena_context_operational(
            context)) {
        return;
    }

    vitte_arena_reset_and_trim(
        &context->allocator);

    context->generation =
        vitte_arena_generation(
            &context->allocator);

    context->allocation_count = 0u;
    context->requested_bytes = 0u;

    context->last_error =
        VITTE_ARENA_CONTEXT_ERROR_NONE;
}

/* ========================================================================= */
/* Ownership                                                                 */
/* ========================================================================= */

bool
vitte_arena_context_contains(
    const vitte_arena_context_t *context,
    const void *pointer)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return false;
    }

    return vitte_arena_contains(
        &context->allocator,
        pointer);
}

bool
vitte_arena_context_contains_live(
    const vitte_arena_context_t *context,
    const void *pointer)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return false;
    }

    return vitte_arena_contains_live(
        &context->allocator,
        pointer);
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_arena_context_stats_t
vitte_arena_context_stats(
    const vitte_arena_context_t *context)
{
    vitte_arena_context_stats_t result;
    vitte_arena_stats_t allocator_stats;

    memset(&result, 0, sizeof(result));

    if (!vitte_arena_context_magic_valid(
            context)) {
        return result;
    }

    allocator_stats =
        vitte_arena_stats(
            &context->allocator);

    result.block_size =
        allocator_stats.block_size;

    result.block_count =
        allocator_stats.block_count;

    result.peak_block_count =
        allocator_stats.peak_block_count;

    result.reserved_bytes =
        allocator_stats.reserved_bytes;

    result.peak_reserved_bytes =
        allocator_stats.peak_reserved_bytes;

    result.used_bytes =
        allocator_stats.used_bytes;

    result.peak_used_bytes =
        allocator_stats.peak_used_bytes;

    result.allocator_requested_bytes =
        allocator_stats.requested_bytes;

    result.unused_bytes =
        allocator_stats.unused_bytes;

    result.padding_bytes =
        allocator_stats.padding_bytes;

    result.requested_bytes =
        context->requested_bytes;

    result.peak_requested_bytes =
        context->peak_requested_bytes;

    result.allocation_count =
        context->allocation_count;

    result.lifetime_requested_bytes =
        context->lifetime_requested_bytes;

    result.lifetime_allocation_count =
        context->lifetime_allocation_count;

    result.generation =
        context->generation;

    result.memory_limit =
        context->config.memory_limit;

    result.string_limit =
        context->config.string_limit;

    return result;
}

size_t
vitte_arena_context_requested_bytes(
    const vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return 0u;
    }

    return context->requested_bytes;
}

size_t
vitte_arena_context_allocation_count(
    const vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return 0u;
    }

    return context->allocation_count;
}

size_t
vitte_arena_context_reserved_bytes(
    const vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return 0u;
    }

    return vitte_arena_reserved_bytes(
        &context->allocator);
}

size_t
vitte_arena_context_used_bytes(
    const vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return 0u;
    }

    return vitte_arena_used_bytes(
        &context->allocator);
}

uint64_t
vitte_arena_context_generation(
    const vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return 0u;
    }

    return context->generation;
}

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

bool
vitte_arena_context_is_valid(
    const vitte_arena_context_t *context)
{
    return
        vitte_arena_context_magic_valid(
            context) &&
        context->state ==
            VITTE_ARENA_STATE_READY;
}

const char *
vitte_arena_state_name(
    vitte_arena_state_t state)
{
    switch (state) {
        case VITTE_ARENA_STATE_UNINITIALIZED:
            return "uninitialized";

        case VITTE_ARENA_STATE_READY:
            return "ready";

        case VITTE_ARENA_STATE_FAILED:
            return "failed";

        case VITTE_ARENA_STATE_DESTROYED:
            return "destroyed";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Memory limit                                                              */
/* ========================================================================= */

size_t
vitte_arena_context_memory_limit(
    const vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return 0u;
    }

    return context->config.memory_limit;
}

bool
vitte_arena_context_set_memory_limit(
    vitte_arena_context_t *context,
    size_t limit)
{
    if (!vitte_arena_context_operational(
            context)) {
        return false;
    }

    if (limit == 0u) {
        limit = SIZE_MAX;
    }

    if (limit != SIZE_MAX &&
        context->requested_bytes > limit) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_LIMIT_EXCEEDED);

        return false;
    }

    context->config.memory_limit =
        limit;

    context->last_error =
        VITTE_ARENA_CONTEXT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* String limit                                                              */
/* ========================================================================= */

size_t
vitte_arena_context_string_limit(
    const vitte_arena_context_t *context)
{
    if (!vitte_arena_context_magic_valid(
            context)) {
        return 0u;
    }

    return context->config.string_limit;
}

bool
vitte_arena_context_set_string_limit(
    vitte_arena_context_t *context,
    size_t limit)
{
    if (!vitte_arena_context_operational(
            context)) {
        return false;
    }

    if (limit == 0u) {
        vitte_arena_context_set_error(
            context,
            VITTE_ARENA_CONTEXT_ERROR_INVALID_ARGUMENT);

        return false;
    }

    context->config.string_limit = limit;

    context->last_error =
        VITTE_ARENA_CONTEXT_ERROR_NONE;

    return true;
}

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_arena_context_validate(
    const vitte_arena_context_t *context)
{
    vitte_arena_stats_t stats;

    if (!vitte_arena_context_magic_valid(
            context)) {
        return false;
    }

    if (context->state !=
            VITTE_ARENA_STATE_READY &&
        context->state !=
            VITTE_ARENA_STATE_FAILED) {
        return false;
    }

    if (!vitte_arena_config_validate(
            &context->config)) {
        return false;
    }

    if (!vitte_arena_validate(
            &context->allocator)) {
        return false;
    }

    if (context->generation !=
        vitte_arena_generation(
            &context->allocator)) {
        return false;
    }

    if (context->requested_bytes >
        context->peak_requested_bytes) {
        return false;
    }

    if (context->allocation_count >
        context->lifetime_allocation_count) {
        return false;
    }

    if (context->requested_bytes >
        context->lifetime_requested_bytes) {
        return false;
    }

    if (context->config.memory_limit !=
            SIZE_MAX &&
        context->requested_bytes >
            context->config.memory_limit) {
        return false;
    }

    stats =
        vitte_arena_stats(
            &context->allocator);

    if (stats.used_bytes >
        stats.reserved_bytes) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
