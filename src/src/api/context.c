/*
 * Vitte Compiler
 * src/api/context.c
 *
 * Central compiler context implementation.
 *
 * The context owns reusable compiler infrastructure and per-compilation
 * state. It is the bridge between the stable API layer and the compiler
 * pipeline.
 *
 * Core responsibilities:
 *
 *   - lifecycle and invariants
 *   - source ownership and registration
 *   - diagnostic ownership
 *   - compilation generations
 *   - per-compilation reset
 *   - source IDs
 *   - statistics
 *   - cancellation/failure state
 *   - deterministic cleanup
 *
 * The context does not implement lexer/parser/sema/backend algorithms.
 */

#include "context.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"


/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

#ifndef VITTE_CONTEXT_MAGIC
#define VITTE_CONTEXT_MAGIC UINT64_C(0x5649545445434F4E)
#endif

#ifndef VITTE_CONTEXT_DEAD_MAGIC
#define VITTE_CONTEXT_DEAD_MAGIC UINT64_C(0x44454144434F4E54)
#endif

#ifndef VITTE_CONTEXT_INITIAL_SOURCE_CAPACITY
#define VITTE_CONTEXT_INITIAL_SOURCE_CAPACITY ((size_t)8u)
#endif

#ifndef VITTE_CONTEXT_INITIAL_DIAGNOSTIC_CAPACITY
#define VITTE_CONTEXT_INITIAL_DIAGNOSTIC_CAPACITY ((size_t)32u)
#endif

#ifndef VITTE_CONTEXT_MAX_SOURCE_COUNT
#define VITTE_CONTEXT_MAX_SOURCE_COUNT ((size_t)1048576u)
#endif

#ifndef VITTE_CONTEXT_MAX_SOURCE_SIZE
#define VITTE_CONTEXT_MAX_SOURCE_SIZE \
    ((size_t)1024u * 1024u * 1024u)
#endif

#ifndef VITTE_CONTEXT_MAX_SOURCE_NAME
#define VITTE_CONTEXT_MAX_SOURCE_NAME ((size_t)32768u)
#endif

/* ========================================================================= */
/* Internal helpers                                                          */
/* ========================================================================= */

static bool
vitte_context_size_add_overflow(
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
vitte_context_size_mul_overflow(
    size_t left,
    size_t right,
    size_t *result)
{
    if (result == NULL) {
        return true;
    }

    if (left != 0u && right > SIZE_MAX / left) {
        return true;
    }

    *result = left * right;
    return false;
}

static void
vitte_context_set_thread_error(
    vitte_status_t status,
    const char *code,
    const char *message)
{
    vitte_error_t error;

    vitte_error_init(&error);
    vitte_error_set(&error, status, code, message);
}

static size_t
vitte_context_bounded_strlen(
    const char *text,
    size_t maximum)
{
    size_t length;

    if (text == NULL) {
        return 0u;
    }

    length = 0u;

    while (length < maximum && text[length] != '\0') {
        ++length;
    }

    return length;
}

static char *
vitte_context_duplicate_string_n(
    const char *text,
    size_t length)
{
    char *copy;
    size_t allocation_size;

    if (text == NULL && length != 0u) {
        return NULL;
    }

    if (vitte_context_size_add_overflow(
            length,
            1u,
            &allocation_size)) {
        return NULL;
    }

    copy = (char *)malloc(allocation_size);

    if (copy == NULL) {
        return NULL;
    }

    if (length != 0u) {
        memcpy(copy, text, length);
    }

    copy[length] = '\0';

    return copy;
}

static char *
vitte_context_duplicate_string(
    const char *text)
{
    size_t length;

    if (text == NULL) {
        return NULL;
    }

    length = strlen(text);

    return vitte_context_duplicate_string_n(
        text,
        length);
}

/* ========================================================================= */
/* Source record                                                             */
/* ========================================================================= */

static void
vitte_context_source_init(
    vitte_context_source_t *source)
{
    if (source == NULL) {
        return;
    }

    memset(source, 0, sizeof(*source));

    source->id = VITTE_SOURCE_ID_NONE;
}

static void
vitte_context_source_destroy(
    vitte_context_source_t *source)
{
    if (source == NULL) {
        return;
    }

    free(source->name);
    free(source->data);

    source->name = NULL;
    source->data = NULL;

    source->length = 0u;
    source->id = VITTE_SOURCE_ID_NONE;
    source->generation = 0u;
    source->hash = 0u;
}

static uint64_t
vitte_context_hash_bytes(
    const void *data,
    size_t length)
{
    const unsigned char *bytes;
    uint64_t hash;
    size_t index;

    /*
     * FNV-1a 64.
     *
     * This is intentionally used for deterministic source identity and
     * change detection, not cryptographic authentication.
     */
    hash = UINT64_C(14695981039346656037);

    bytes = (const unsigned char *)data;

    for (index = 0u; index < length; ++index) {
        hash ^= (uint64_t)bytes[index];
        hash *= UINT64_C(1099511628211);
    }

    return hash;
}

/* ========================================================================= */
/* Context state helpers                                                     */
/* ========================================================================= */

static bool
vitte_context_magic_valid(
    const vitte_context_t *context)
{
    return context != NULL &&
           context->magic == VITTE_CONTEXT_MAGIC;
}

static bool
vitte_context_state_valid(
    vitte_context_state_t state)
{
    switch (state) {
        case VITTE_CONTEXT_STATE_READY:
        case VITTE_CONTEXT_STATE_COMPILING:
        case VITTE_CONTEXT_STATE_FAILED:
            return true;

        case VITTE_CONTEXT_STATE_UNINITIALIZED:
        case VITTE_CONTEXT_STATE_DESTROYED:
        default:
            return false;
    }
}

bool
vitte_context_is_valid(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return false;
    }

    if (!vitte_context_state_valid(context->state)) {
        return false;
    }

    if (context->source_count >
        context->source_capacity) {
        return false;
    }

    if (context->source_capacity != 0u &&
        context->sources == NULL) {
        return false;
    }

    if (context->source_count != 0u &&
        context->sources == NULL) {
        return false;
    }

    if (context->diagnostic_storage == NULL ||
        !context->diagnostics.initialized ||
        context->diagnostics.storage !=
            context->diagnostic_storage) {
        return false;
    }

    return true;
}

bool
vitte_context_is_compiling(
    const vitte_context_t *context)
{
    return vitte_context_magic_valid(context) &&
           context->state ==
               VITTE_CONTEXT_STATE_COMPILING;
}

bool
vitte_context_has_failed(
    const vitte_context_t *context)
{
    return vitte_context_magic_valid(context) &&
           context->state ==
               VITTE_CONTEXT_STATE_FAILED;
}

/* ========================================================================= */
/* Error helpers                                                             */
/* ========================================================================= */

static vitte_status_t
vitte_context_fail(
    vitte_context_t *context,
    vitte_status_t status,
    const char *code,
    const char *message)
{
    if (context != NULL &&
        context->magic == VITTE_CONTEXT_MAGIC) {
        vitte_error_set(
            &context->last_error,
            status,
            code,
            message);
    } else {
        vitte_context_set_thread_error(
            status,
            code,
            message);
    }

    return status;
}

static vitte_status_t
vitte_context_invalid_argument(
    vitte_context_t *context,
    const char *message)
{
    return vitte_context_fail(
        context,
        VITTE_STATUS_ERROR_INVALID_ARGUMENT,
        "VITTE_CONTEXT_INVALID_ARGUMENT",
        message);
}

static vitte_status_t
vitte_context_invalid_state(
    vitte_context_t *context,
    const char *message)
{
    return vitte_context_fail(
        context,
        VITTE_STATUS_ERROR_INVALID_STATE,
        "VITTE_CONTEXT_INVALID_STATE",
        message);
}

static vitte_status_t
vitte_context_out_of_memory(
    vitte_context_t *context)
{
    return vitte_context_fail(
        context,
        VITTE_STATUS_ERROR_OUT_OF_MEMORY,
        "VITTE_CONTEXT_OUT_OF_MEMORY",
        "out of memory");
}

static vitte_status_t
vitte_context_internal_error(
    vitte_context_t *context,
    const char *message)
{
    return vitte_context_fail(
        context,
        VITTE_STATUS_ERROR_INTERNAL,
        "VITTE_CONTEXT_INTERNAL",
        message);
}

/* ========================================================================= */
/* Source capacity                                                           */
/* ========================================================================= */

static vitte_status_t
vitte_context_reserve_sources(
    vitte_context_t *context,
    size_t required)
{
    vitte_context_source_t *sources;
    size_t capacity;
    size_t bytes;
    size_t index;

    if (context == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (required <= context->source_capacity) {
        return VITTE_STATUS_OK;
    }

    if (required > VITTE_CONTEXT_MAX_SOURCE_COUNT) {
        return vitte_context_invalid_argument(
            context,
            "source count exceeds compiler limit");
    }

    capacity = context->source_capacity;

    if (capacity == 0u) {
        capacity =
            VITTE_CONTEXT_INITIAL_SOURCE_CAPACITY;
    }

    while (capacity < required) {
        size_t next;

        if (capacity >
            VITTE_CONTEXT_MAX_SOURCE_COUNT / 2u) {
            capacity =
                VITTE_CONTEXT_MAX_SOURCE_COUNT;
        } else {
            next = capacity * 2u;

            if (next <= capacity) {
                return vitte_context_out_of_memory(
                    context);
            }

            capacity = next;
        }

        if (capacity < required &&
            capacity ==
                VITTE_CONTEXT_MAX_SOURCE_COUNT) {
            return vitte_context_invalid_argument(
                context,
                "source count exceeds compiler limit");
        }
    }

    if (vitte_context_size_mul_overflow(
            capacity,
            sizeof(*sources),
            &bytes)) {
        return vitte_context_out_of_memory(context);
    }

    sources = (vitte_context_source_t *)
        realloc(context->sources, bytes);

    if (sources == NULL) {
        return vitte_context_out_of_memory(context);
    }

    for (index = context->source_capacity;
         index < capacity;
         ++index) {
        vitte_context_source_init(
            &sources[index]);
    }

    context->sources = sources;
    context->source_capacity = capacity;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Source reset                                                              */
/* ========================================================================= */

static void
vitte_context_clear_sources(
    vitte_context_t *context)
{
    size_t index;

    if (context == NULL) {
        return;
    }

    for (index = 0u;
         index < context->source_count;
         ++index) {
        vitte_context_source_destroy(
            &context->sources[index]);
    }

    context->source_count = 0u;
    context->total_source_bytes = 0u;
}

/* ========================================================================= */
/* Diagnostics                                                               */
/* ========================================================================= */

static void
vitte_context_reset_diagnostics(
    vitte_context_t *context)
{
    if (context == NULL) {
        return;
    }

    vitte_diagnostic_bag_reset(
        &context->diagnostics);
}

const vitte_diagnostic_bag_t *
vitte_context_diagnostics(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return NULL;
    }

    return &context->diagnostics;
}

vitte_diagnostic_bag_t *
vitte_context_diagnostics_mut(
    vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return NULL;
    }

    return &context->diagnostics;
}

/* ========================================================================= */
/* Initialization                                                            */
/* ========================================================================= */

vitte_status_t
vitte_context_init(
    vitte_context_t *context)
{
    vitte_status_t status;

    if (context == NULL) {
        vitte_context_set_thread_error(
            VITTE_STATUS_ERROR_INVALID_ARGUMENT,
            "VITTE_CONTEXT_INVALID_ARGUMENT",
            "context is NULL");

        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(context, 0, sizeof(*context));

    context->magic = VITTE_CONTEXT_MAGIC;
    context->state =
        VITTE_CONTEXT_STATE_UNINITIALIZED;

    context->generation = 0u;

    context->next_source_id = 1u;

    context->sources = NULL;
    context->source_count = 0u;
    context->source_capacity = 0u;

    context->total_source_bytes = 0u;

    context->compilation_count = 0u;
    context->successful_compilation_count = 0u;
    context->failed_compilation_count = 0u;

    context->cancel_requested = false;

    vitte_error_init(
        &context->last_error);

    context->diagnostic_storage =
        (vitte_diagnostic_t *)calloc(
            VITTE_CONTEXT_INITIAL_DIAGNOSTIC_CAPACITY,
            sizeof(*context->diagnostic_storage));

    if (context->diagnostic_storage == NULL) {
        vitte_context_out_of_memory(context);
        context->magic = 0u;

        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    status = vitte_diagnostic_bag_init(
        &context->diagnostics,
        context->diagnostic_storage,
        VITTE_CONTEXT_INITIAL_DIAGNOSTIC_CAPACITY);

    if (status != VITTE_STATUS_OK) {
        free(context->diagnostic_storage);
        context->diagnostic_storage = NULL;
        context->magic = 0u;

        return status;
    }

    status = vitte_context_reserve_sources(
        context,
        VITTE_CONTEXT_INITIAL_SOURCE_CAPACITY);

    if (status != VITTE_STATUS_OK) {
        vitte_diagnostic_bag_reset(
            &context->diagnostics);
        free(context->diagnostic_storage);
        context->diagnostic_storage = NULL;

        context->magic = 0u;

        return status;
    }

    context->state =
        VITTE_CONTEXT_STATE_READY;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Destruction                                                               */
/* ========================================================================= */

void
vitte_context_destroy(
    vitte_context_t *context)
{
    size_t index;

    if (context == NULL) {
        return;
    }

    if (context->magic != VITTE_CONTEXT_MAGIC) {
        return;
    }

    /*
     * Destruction during compilation is a programming error.
     *
     * We nevertheless release memory because context destruction is the
     * final ownership boundary.
     */
    context->cancel_requested = true;

    for (index = 0u;
         index < context->source_count;
         ++index) {
        vitte_context_source_destroy(
            &context->sources[index]);
    }

    free(context->sources);

    context->sources = NULL;
    context->source_count = 0u;
    context->source_capacity = 0u;

    vitte_diagnostic_bag_reset(
        &context->diagnostics);
    free(context->diagnostic_storage);
    context->diagnostic_storage = NULL;

    vitte_error_reset(
        &context->last_error);

    context->state =
        VITTE_CONTEXT_STATE_DESTROYED;

    context->magic =
        VITTE_CONTEXT_DEAD_MAGIC;

    context->generation = 0u;
    context->next_source_id = 0u;

    context->total_source_bytes = 0u;

    context->compilation_count = 0u;
    context->successful_compilation_count = 0u;
    context->failed_compilation_count = 0u;

    context->cancel_requested = false;
}

/* ========================================================================= */
/* Compilation lifecycle                                                     */
/* ========================================================================= */

vitte_status_t
vitte_context_begin_compilation(
    vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (context->state ==
        VITTE_CONTEXT_STATE_COMPILING) {
        return vitte_context_invalid_state(
            context,
            "context is already compiling");
    }

    if (context->state !=
            VITTE_CONTEXT_STATE_READY &&
        context->state !=
            VITTE_CONTEXT_STATE_FAILED) {
        return vitte_context_invalid_state(
            context,
            "context cannot begin compilation in current state");
    }

    /*
     * Generation zero is reserved for objects that do not belong to a
     * compilation.
     */
    ++context->generation;

    if (context->generation == 0u) {
        ++context->generation;
    }

    ++context->compilation_count;

    context->state =
        VITTE_CONTEXT_STATE_COMPILING;

    context->cancel_requested = false;

    vitte_error_reset(
        &context->last_error);

    vitte_context_clear_sources(context);
    vitte_context_reset_diagnostics(context);

    return VITTE_STATUS_OK;
}

vitte_status_t
vitte_context_end_compilation(
    vitte_context_t *context)
{
    bool failed;

    if (!vitte_context_magic_valid(context)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (context->state !=
        VITTE_CONTEXT_STATE_COMPILING) {
        return vitte_context_invalid_state(
            context,
            "context is not compiling");
    }

    /*
     * Diagnostics are the final authority for semantic compilation
     * failure. Internal pipeline failures should already have set the
     * context error state.
     */
    failed =
        vitte_diagnostic_error_count(
            &context->diagnostics) != 0u ||
        context->last_error.status !=
            VITTE_STATUS_OK ||
        context->cancel_requested;

    if (failed) {
        context->state =
            VITTE_CONTEXT_STATE_FAILED;

        ++context->failed_compilation_count;
    } else {
        context->state =
            VITTE_CONTEXT_STATE_READY;

        ++context->successful_compilation_count;
    }

    return context->last_error.status;
}

/* ========================================================================= */
/* Compilation abort                                                         */
/* ========================================================================= */

vitte_status_t
vitte_context_abort_compilation(
    vitte_context_t *context,
    vitte_status_t status,
    const char *code,
    const char *message)
{
    if (!vitte_context_magic_valid(context)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (context->state !=
        VITTE_CONTEXT_STATE_COMPILING) {
        return vitte_context_invalid_state(
            context,
            "cannot abort a context that is not compiling");
    }

    if (status == VITTE_STATUS_OK) {
        status = VITTE_STATUS_ERROR_INTERNAL;
    }

    vitte_context_fail(
        context,
        status,
        code != NULL
            ? code
            : "VITTE_CONTEXT_ABORT",
        message != NULL
            ? message
            : "compilation aborted");

    context->state =
        VITTE_CONTEXT_STATE_FAILED;

    ++context->failed_compilation_count;

    return status;
}

/* ========================================================================= */
/* Cancellation                                                              */
/* ========================================================================= */

void
vitte_context_request_cancel(
    vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return;
    }

    context->cancel_requested = true;
}

bool
vitte_context_cancel_requested(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return false;
    }

    return context->cancel_requested;
}

vitte_status_t
vitte_context_check_cancel(
    vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (!context->cancel_requested) {
        return VITTE_STATUS_OK;
    }

    return vitte_context_fail(
        context,
        VITTE_STATUS_ERROR_INVALID_STATE,
        "VITTE_CONTEXT_CANCELLED",
        "compilation cancelled");
}

/* ========================================================================= */
/* Source validation                                                         */
/* ========================================================================= */

static vitte_status_t
vitte_context_validate_source_input(
    vitte_context_t *context,
    const char *name,
    const char *data,
    size_t length)
{
    size_t name_length;

    if (name == NULL || name[0] == '\0') {
        return vitte_context_invalid_argument(
            context,
            "source name is empty");
    }

    name_length =
        vitte_context_bounded_strlen(
            name,
            VITTE_CONTEXT_MAX_SOURCE_NAME);

    if (name_length == 0u ||
        name_length >=
            VITTE_CONTEXT_MAX_SOURCE_NAME) {
        return vitte_context_invalid_argument(
            context,
            "source name exceeds compiler limit");
    }

    if (data == NULL && length != 0u) {
        return vitte_context_invalid_argument(
            context,
            "source data is NULL with non-zero length");
    }

    if (length >
        VITTE_CONTEXT_MAX_SOURCE_SIZE) {
        return vitte_context_invalid_argument(
            context,
            "source exceeds compiler size limit");
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Source registration                                                       */
/* ========================================================================= */

vitte_status_t
vitte_context_add_source(
    vitte_context_t *context,
    const char *name,
    const char *data,
    size_t length,
    vitte_source_id_t *out_source_id)
{
    vitte_context_source_t source;
    vitte_context_source_t *destination;
    vitte_status_t status;
    size_t total_bytes;

    if (out_source_id != NULL) {
        *out_source_id = VITTE_SOURCE_ID_NONE;
    }

    if (!vitte_context_magic_valid(context)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (context->state !=
        VITTE_CONTEXT_STATE_COMPILING) {
        return vitte_context_invalid_state(
            context,
            "sources can only be added during compilation");
    }

    status =
        vitte_context_validate_source_input(
            context,
            name,
            data,
            length);

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    if (context->source_count >=
        VITTE_CONTEXT_MAX_SOURCE_COUNT) {
        return vitte_context_invalid_argument(
            context,
            "maximum source count reached");
    }

    if (vitte_context_size_add_overflow(
            context->total_source_bytes,
            length,
            &total_bytes)) {
        return vitte_context_out_of_memory(context);
    }

    status =
        vitte_context_reserve_sources(
            context,
            context->source_count + 1u);

    if (status != VITTE_STATUS_OK) {
        return status;
    }

    vitte_context_source_init(&source);

    source.name =
        vitte_context_duplicate_string(name);

    if (source.name == NULL) {
        return vitte_context_out_of_memory(context);
    }

    source.data =
        vitte_context_duplicate_string_n(
            data,
            length);

    if (source.data == NULL) {
        vitte_context_source_destroy(&source);

        return vitte_context_out_of_memory(context);
    }

    source.length = length;

    source.id = context->next_source_id++;

    if (source.id == VITTE_SOURCE_ID_NONE) {
        source.id = context->next_source_id++;
    }

    source.generation =
        context->generation;

    source.hash =
        vitte_context_hash_bytes(
            data,
            length);

    destination =
        &context->sources[
            context->source_count];

    *destination = source;

    ++context->source_count;

    context->total_source_bytes =
        total_bytes;

    if (out_source_id != NULL) {
        *out_source_id = source.id;
    }

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Source lookup                                                             */
/* ========================================================================= */

const vitte_context_source_t *
vitte_context_source_at(
    const vitte_context_t *context,
    size_t index)
{
    if (!vitte_context_magic_valid(context)) {
        return NULL;
    }

    if (index >= context->source_count) {
        return NULL;
    }

    return &context->sources[index];
}

vitte_context_source_t *
vitte_context_source_at_mut(
    vitte_context_t *context,
    size_t index)
{
    if (!vitte_context_magic_valid(context)) {
        return NULL;
    }

    if (index >= context->source_count) {
        return NULL;
    }

    return &context->sources[index];
}

const vitte_context_source_t *
vitte_context_find_source(
    const vitte_context_t *context,
    vitte_source_id_t source_id)
{
    size_t index;

    if (!vitte_context_magic_valid(context)) {
        return NULL;
    }

    if (source_id == VITTE_SOURCE_ID_NONE) {
        return NULL;
    }

    for (index = 0u;
         index < context->source_count;
         ++index) {
        if (context->sources[index].id ==
            source_id) {
            return &context->sources[index];
        }
    }

    return NULL;
}

vitte_context_source_t *
vitte_context_find_source_mut(
    vitte_context_t *context,
    vitte_source_id_t source_id)
{
    size_t index;

    if (!vitte_context_magic_valid(context)) {
        return NULL;
    }

    if (source_id == VITTE_SOURCE_ID_NONE) {
        return NULL;
    }

    for (index = 0u;
         index < context->source_count;
         ++index) {
        if (context->sources[index].id ==
            source_id) {
            return &context->sources[index];
        }
    }

    return NULL;
}

const vitte_context_source_t *
vitte_context_find_source_by_name(
    const vitte_context_t *context,
    const char *name)
{
    size_t index;

    if (!vitte_context_magic_valid(context) ||
        name == NULL) {
        return NULL;
    }

    for (index = 0u;
         index < context->source_count;
         ++index) {
        if (context->sources[index].name != NULL &&
            strcmp(
                context->sources[index].name,
                name) == 0) {
            return &context->sources[index];
        }
    }

    return NULL;
}

/* ========================================================================= */
/* Source properties                                                         */
/* ========================================================================= */

size_t
vitte_context_source_count(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return 0u;
    }

    return context->source_count;
}

size_t
vitte_context_total_source_bytes(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return 0u;
    }

    return context->total_source_bytes;
}

const char *
vitte_context_source_name(
    const vitte_context_t *context,
    vitte_source_id_t source_id)
{
    const vitte_context_source_t *source;

    source =
        vitte_context_find_source(
            context,
            source_id);

    return source != NULL
        ? source->name
        : NULL;
}

const char *
vitte_context_source_data(
    const vitte_context_t *context,
    vitte_source_id_t source_id,
    size_t *out_length)
{
    const vitte_context_source_t *source;

    if (out_length != NULL) {
        *out_length = 0u;
    }

    source =
        vitte_context_find_source(
            context,
            source_id);

    if (source == NULL) {
        return NULL;
    }

    if (out_length != NULL) {
        *out_length = source->length;
    }

    return source->data;
}

uint64_t
vitte_context_source_hash(
    const vitte_context_t *context,
    vitte_source_id_t source_id)
{
    const vitte_context_source_t *source;

    source =
        vitte_context_find_source(
            context,
            source_id);

    return source != NULL
        ? source->hash
        : UINT64_C(0);
}

/* ========================================================================= */
/* Generation                                                                */
/* ========================================================================= */

uint64_t
vitte_context_generation(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return UINT64_C(0);
    }

    return context->generation;
}

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

size_t
vitte_context_compilation_count(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return 0u;
    }

    return context->compilation_count;
}

size_t
vitte_context_successful_compilation_count(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return 0u;
    }

    return context->successful_compilation_count;
}

size_t
vitte_context_failed_compilation_count(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return 0u;
    }

    return context->failed_compilation_count;
}

/* ========================================================================= */
/* Error state                                                               */
/* ========================================================================= */

const vitte_error_t *
vitte_context_last_error(
    const vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return vitte_error_last();
    }

    return &context->last_error;
}

void
vitte_context_clear_error(
    vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return;
    }

    vitte_error_reset(
        &context->last_error);
}

/* ========================================================================= */
/* Failure state                                                             */
/* ========================================================================= */

vitte_status_t
vitte_context_set_failure(
    vitte_context_t *context,
    vitte_status_t status,
    const char *code,
    const char *message)
{
    if (!vitte_context_magic_valid(context)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (status == VITTE_STATUS_OK) {
        return vitte_context_invalid_argument(
            context,
            "failure status cannot be VITTE_STATUS_OK");
    }

    vitte_context_fail(
        context,
        status,
        code != NULL
            ? code
            : "VITTE_CONTEXT_FAILURE",
        message != NULL
            ? message
            : "compiler context failure");

    return status;
}

/* ========================================================================= */
/* Reset                                                                     */
/* ========================================================================= */

vitte_status_t
vitte_context_reset(
    vitte_context_t *context)
{
    if (!vitte_context_magic_valid(context)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (context->state ==
        VITTE_CONTEXT_STATE_COMPILING) {
        return vitte_context_invalid_state(
            context,
            "cannot reset context during compilation");
    }

    vitte_context_clear_sources(context);
    vitte_context_reset_diagnostics(context);

    vitte_error_reset(
        &context->last_error);

    context->state =
        VITTE_CONTEXT_STATE_READY;

    context->cancel_requested = false;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Full invariant validation                                                 */
/* ========================================================================= */

bool
vitte_context_validate(
    const vitte_context_t *context)
{
    size_t index;
    size_t calculated_bytes;

    if (!vitte_context_is_valid(context)) {
        return false;
    }

    if (context->source_count >
        VITTE_CONTEXT_MAX_SOURCE_COUNT) {
        return false;
    }

    if (context->successful_compilation_count >
        context->compilation_count) {
        return false;
    }

    if (context->failed_compilation_count >
        context->compilation_count) {
        return false;
    }

    if (context->successful_compilation_count +
            context->failed_compilation_count >
        context->compilation_count) {
        return false;
    }

    calculated_bytes = 0u;

    for (index = 0u;
         index < context->source_count;
         ++index) {
        const vitte_context_source_t *source;
        size_t new_total;
        size_t other;

        source = &context->sources[index];

        if (source->id == VITTE_SOURCE_ID_NONE) {
            return false;
        }

        if (source->name == NULL ||
            source->name[0] == '\0') {
            return false;
        }

        if (source->data == NULL) {
            return false;
        }

        if (source->length >
            VITTE_CONTEXT_MAX_SOURCE_SIZE) {
            return false;
        }

        if (source->data[source->length] != '\0') {
            return false;
        }

        if (source->generation !=
            context->generation) {
            return false;
        }

        if (source->hash !=
            vitte_context_hash_bytes(
                source->data,
                source->length)) {
            return false;
        }

        if (vitte_context_size_add_overflow(
                calculated_bytes,
                source->length,
                &new_total)) {
            return false;
        }

        calculated_bytes = new_total;

        /*
         * Source IDs must be unique inside a compilation.
         */
        for (other = index + 1u;
             other < context->source_count;
             ++other) {
            if (source->id ==
                context->sources[other].id) {
                return false;
            }
        }
    }

    if (calculated_bytes !=
        context->total_source_bytes) {
        return false;
    }

    return true;
}

/* ========================================================================= */
/* Debug state names                                                         */
/* ========================================================================= */

const char *
vitte_context_state_name(
    vitte_context_state_t state)
{
    switch (state) {
        case VITTE_CONTEXT_STATE_UNINITIALIZED:
            return "uninitialized";

        case VITTE_CONTEXT_STATE_READY:
            return "ready";

        case VITTE_CONTEXT_STATE_COMPILING:
            return "compiling";

        case VITTE_CONTEXT_STATE_FAILED:
            return "failed";

        case VITTE_CONTEXT_STATE_DESTROYED:
            return "destroyed";

        default:
            return "unknown";
    }
}

/* ========================================================================= */
/* Memory release                                                            */
/* ========================================================================= */

vitte_status_t
vitte_context_release_unused_memory(
    vitte_context_t *context)
{
    vitte_context_source_t *sources;
    size_t bytes;

    if (!vitte_context_magic_valid(context)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (context->state ==
        VITTE_CONTEXT_STATE_COMPILING) {
        return vitte_context_invalid_state(
            context,
            "cannot release context memory during compilation");
    }

    if (context->source_count == 0u) {
        free(context->sources);

        context->sources = NULL;
        context->source_capacity = 0u;

        return VITTE_STATUS_OK;
    }

    if (context->source_capacity ==
        context->source_count) {
        return VITTE_STATUS_OK;
    }

    if (vitte_context_size_mul_overflow(
            context->source_count,
            sizeof(*sources),
            &bytes)) {
        return vitte_context_internal_error(
            context,
            "source allocation size overflow");
    }

    sources =
        (vitte_context_source_t *)
            realloc(context->sources, bytes);

    /*
     * Failure to shrink an allocation is not a compiler failure.
     */
    if (sources == NULL) {
        return VITTE_STATUS_OK;
    }

    context->sources = sources;
    context->source_capacity =
        context->source_count;

    return VITTE_STATUS_OK;
}

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
