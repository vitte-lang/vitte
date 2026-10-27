/*
 * Vitte Compiler
 * src/api/error.c
 *
 * Canonical infrastructure error implementation.
 *
 * This module provides Vitte's small, allocation-free infrastructure error
 * object. It is intentionally separate from the structured compiler
 * diagnostic subsystem.
 *
 * Infrastructure failures:
 *
 *     vitte_status_t + vitte_error_t
 *
 * Source-facing compiler failures:
 *
 *     vitte_diagnostic_t
 *
 * Design goals:
 *
 *   - ISO C17
 *   - no heap allocation
 *   - deterministic status metadata
 *   - canonical OK state
 *   - bounded inline details storage
 *   - guaranteed NUL termination
 *   - alias-safe details updates
 *   - self-copy safety
 *   - movement/copy-safe details rebinding
 *   - thread-local last-error state
 *   - no recursive TLS updates
 *   - defensive handling of malformed objects
 *   - defensive handling of unknown status values
 *   - explicit normalization
 *   - stable public behavior
 */

#include "error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(
    VITTE_ERROR_DETAILS_CAPACITY >= 2u,
    "VITTE_ERROR_DETAILS_CAPACITY must be at least 2"
);

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

static const char VITTE_ERROR_CODE_OK[] =
    "VITTE_OK";

static const char VITTE_ERROR_MESSAGE_OK[] =
    "ok";

static const char VITTE_ERROR_CODE_UNKNOWN[] =
    "VITTE_ERROR_UNKNOWN";

static const char VITTE_ERROR_MESSAGE_UNKNOWN[] =
    "unknown error";

/* ========================================================================= */
/* Thread-local last error                                                   */
/* ========================================================================= */

/*
 * _Thread_local gives every thread an independent last-error object.
 *
 * C guarantees zero-initialization, but the canonical Vitte OK object
 * contains non-NULL metadata pointers. Initialization is therefore performed
 * lazily.
 */

static _Thread_local vitte_error_t g_vitte_last_error;
static _Thread_local bool g_vitte_last_error_initialized = false;

/* ========================================================================= */
/* Internal string helpers                                                   */
/* ========================================================================= */

static bool
vitte_error_string_is_empty(
    const char *text)
{
    return
        text == NULL ||
        text[0] == '\0';
}

static size_t
vitte_error_strnlen_internal(
    const char *text,
    size_t maximum)
{
    size_t length;

    if (text == NULL) {
        return 0u;
    }

    length = 0u;

    while (length < maximum &&
           text[length] != '\0') {
        ++length;
    }

    return length;
}

/* ========================================================================= */
/* Status validation                                                         */
/* ========================================================================= */

static bool
vitte_status_is_known_internal(
    vitte_status_t status)
{
    switch (status) {
        case VITTE_STATUS_OK:
        case VITTE_STATUS_ERROR_INVALID_ARGUMENT:
        case VITTE_STATUS_ERROR_INVALID_STATE:
        case VITTE_STATUS_ERROR_OUT_OF_MEMORY:
        case VITTE_STATUS_ERROR_IO:
        case VITTE_STATUS_ERROR_PARSE:
        case VITTE_STATUS_ERROR_UNSUPPORTED:
        case VITTE_STATUS_ERROR_BACKEND:
        case VITTE_STATUS_ERROR_INTERNAL:
            return true;
    }

    return false;
}

/* ========================================================================= */
/* Status metadata                                                           */
/* ========================================================================= */

const char *
vitte_status_name(
    vitte_status_t status)
{
    switch (status) {
        case VITTE_STATUS_OK:
            return VITTE_ERROR_CODE_OK;

        case VITTE_STATUS_ERROR_INVALID_ARGUMENT:
            return "VITTE_ERROR_INVALID_ARGUMENT";

        case VITTE_STATUS_ERROR_INVALID_STATE:
            return "VITTE_ERROR_INVALID_STATE";

        case VITTE_STATUS_ERROR_OUT_OF_MEMORY:
            return "VITTE_ERROR_OUT_OF_MEMORY";

        case VITTE_STATUS_ERROR_IO:
            return "VITTE_ERROR_IO";

        case VITTE_STATUS_ERROR_PARSE:
            return "VITTE_ERROR_PARSE";

        case VITTE_STATUS_ERROR_UNSUPPORTED:
            return "VITTE_ERROR_UNSUPPORTED";

        case VITTE_STATUS_ERROR_BACKEND:
            return "VITTE_ERROR_BACKEND";

        case VITTE_STATUS_ERROR_INTERNAL:
            return "VITTE_ERROR_INTERNAL";
    }

    return VITTE_ERROR_CODE_UNKNOWN;
}

const char *
vitte_status_message(
    vitte_status_t status)
{
    switch (status) {
        case VITTE_STATUS_OK:
            return VITTE_ERROR_MESSAGE_OK;

        case VITTE_STATUS_ERROR_INVALID_ARGUMENT:
            return "invalid argument";

        case VITTE_STATUS_ERROR_INVALID_STATE:
            return "invalid state";

        case VITTE_STATUS_ERROR_OUT_OF_MEMORY:
            return "out of memory";

        case VITTE_STATUS_ERROR_IO:
            return "I/O error";

        case VITTE_STATUS_ERROR_PARSE:
            return "parse error";

        case VITTE_STATUS_ERROR_UNSUPPORTED:
            return "unsupported operation";

        case VITTE_STATUS_ERROR_BACKEND:
            return "backend error";

        case VITTE_STATUS_ERROR_INTERNAL:
            return "internal compiler error";
    }

    return VITTE_ERROR_MESSAGE_UNKNOWN;
}

/* ========================================================================= */
/* Metadata normalization                                                    */
/* ========================================================================= */

static const char *
vitte_error_normalize_code(
    vitte_status_t status,
    const char *code)
{
    if (!vitte_error_string_is_empty(code)) {
        return code;
    }

    return vitte_status_name(status);
}

static const char *
vitte_error_normalize_message(
    vitte_status_t status,
    const char *message)
{
    if (!vitte_error_string_is_empty(message)) {
        return message;
    }

    return vitte_status_message(status);
}

/* ========================================================================= */
/* Canonical initialization                                                  */
/* ========================================================================= */

static void
vitte_error_initialize_ok_raw(
    vitte_error_t *error)
{
    if (error == NULL) {
        return;
    }

    error->status =
        VITTE_STATUS_OK;

    error->code =
        VITTE_ERROR_CODE_OK;

    error->message =
        VITTE_ERROR_MESSAGE_OK;

    error->details =
        NULL;

    error->details_storage[0] =
        '\0';
}

/* ========================================================================= */
/* Pointer/storage relationships                                             */
/* ========================================================================= */

static bool
vitte_error_pointer_inside_details(
    const vitte_error_t *error,
    const char *pointer)
{
    uintptr_t pointer_address;
    uintptr_t begin_address;
    uintptr_t end_address;

    if (error == NULL ||
        pointer == NULL) {
        return false;
    }

    /*
     * Relational comparison between unrelated C pointers is not generally
     * defined. Convert to uintptr_t for this storage-membership test.
     */

    pointer_address =
        (uintptr_t)(const void *)pointer;

    begin_address =
        (uintptr_t)(const void *)
            error->details_storage;

    end_address =
        begin_address +
        sizeof(error->details_storage);

    if (end_address < begin_address) {
        return false;
    }

    return
        pointer_address >= begin_address &&
        pointer_address < end_address;
}

/* ========================================================================= */
/* Details handling                                                          */
/* ========================================================================= */

static size_t
vitte_error_details_length_internal(
    const char *details)
{
    return
        vitte_error_strnlen_internal(
            details,
            VITTE_ERROR_DETAILS_CAPACITY - 1u);
}

/*
 * Store contextual details in object-owned inline storage.
 *
 * A temporary buffer is used whenever the source aliases the destination
 * object's storage. This makes operations such as:
 *
 *     error.details = error.details_storage + 3;
 *     vitte_error_set_details(..., error.details);
 *
 * well-defined.
 */
static void
vitte_error_store_details(
    vitte_error_t *error,
    const char *details)
{
    char temporary[VITTE_ERROR_DETAILS_CAPACITY];
    size_t length;

    if (error == NULL) {
        return;
    }

    if (vitte_error_string_is_empty(details)) {
        error->details_storage[0] = '\0';
        error->details = NULL;
        return;
    }

    length =
        vitte_error_details_length_internal(details);

    if (vitte_error_pointer_inside_details(
            error,
            details)) {
        if (length != 0u) {
            (void)memmove(
                temporary,
                details,
                length);
        }

        temporary[length] = '\0';

        if (length != 0u) {
            (void)memcpy(
                error->details_storage,
                temporary,
                length);
        }
    } else {
        if (length != 0u) {
            (void)memcpy(
                error->details_storage,
                details,
                length);
        }
    }

    error->details_storage[length] =
        '\0';

    error->details =
        error->details_storage;
}

/* ========================================================================= */
/* Details rebinding                                                         */
/* ========================================================================= */

/*
 * vitte_error_t contains a pointer into its own inline storage.
 *
 * A raw structure copy therefore leaves details pointing into the source
 * object. Public copy operations repair that relationship.
 */
static void
vitte_error_rebind_details(
    vitte_error_t *error)
{
    if (error == NULL) {
        return;
    }

    if (error->details_storage[0] == '\0') {
        error->details = NULL;
        return;
    }

    error->details =
        error->details_storage;
}

/* ========================================================================= */
/* Internal validation                                                       */
/* ========================================================================= */

static bool
vitte_error_details_storage_is_terminated(
    const vitte_error_t *error)
{
    if (error == NULL) {
        return false;
    }

    return
        vitte_error_strnlen_internal(
            error->details_storage,
            sizeof(error->details_storage)) <
        sizeof(error->details_storage);
}

static bool
vitte_error_object_is_canonical_internal(
    const vitte_error_t *error)
{
    if (error == NULL) {
        return false;
    }

    if (!vitte_status_is_known_internal(
            error->status)) {
        return false;
    }

    if (error->code == NULL ||
        error->message == NULL) {
        return false;
    }

    if (!vitte_error_details_storage_is_terminated(
            error)) {
        return false;
    }

    if (error->details == NULL) {
        if (error->details_storage[0] != '\0') {
            return false;
        }
    } else {
        if (error->details !=
            error->details_storage) {
            return false;
        }

        if (error->details_storage[0] == '\0') {
            return false;
        }
    }

    if (error->status ==
        VITTE_STATUS_OK) {
        if (error->details != NULL) {
            return false;
        }
    }

    return true;
}

/* ========================================================================= */
/* Raw copy                                                                  */
/* ========================================================================= */

/*
 * Copy without touching thread-local last-error state.
 *
 * This separation is fundamental:
 *
 *     copy != report
 *
 * Otherwise:
 *
 *     set_last
 *       -> copy
 *          -> set_last
 *             -> copy
 *                -> ...
 */
static void
vitte_error_copy_raw(
    vitte_error_t *destination,
    const vitte_error_t *source)
{
    vitte_status_t status;
    const char *code;
    const char *message;

    char details_copy[VITTE_ERROR_DETAILS_CAPACITY];

    size_t details_length;
    bool has_details;

    if (destination == NULL) {
        return;
    }

    if (source == NULL) {
        vitte_error_initialize_ok_raw(
            destination);
        return;
    }

    if (destination == source) {
        /*
         * Rebind in case the object was previously moved using a raw
         * structure assignment.
         */
        vitte_error_rebind_details(
            destination);
        return;
    }

    /*
     * Capture all externally referenced source data before modifying the
     * destination.
     */

    status =
        source->status;

    code =
        vitte_error_normalize_code(
            status,
            source->code);

    message =
        vitte_error_normalize_message(
            status,
            source->message);

    details_length = 0u;
    has_details = false;

    if (!vitte_error_string_is_empty(
            source->details)) {
        details_length =
            vitte_error_details_length_internal(
                source->details);

        if (details_length != 0u) {
            (void)memmove(
                details_copy,
                source->details,
                details_length);
        }

        details_copy[details_length] =
            '\0';

        has_details = true;
    }

    destination->status =
        status;

    destination->code =
        code;

    destination->message =
        message;

    if (has_details) {
        if (details_length != 0u) {
            (void)memcpy(
                destination->details_storage,
                details_copy,
                details_length);
        }

        destination->details_storage[
            details_length] = '\0';

        destination->details =
            destination->details_storage;
    } else {
        destination->details_storage[0] =
            '\0';

        destination->details =
            NULL;
    }
}

/* ========================================================================= */
/* TLS initialization                                                        */
/* ========================================================================= */

static void
vitte_error_initialize_tls(void)
{
    if (g_vitte_last_error_initialized) {
        return;
    }

    vitte_error_initialize_ok_raw(
        &g_vitte_last_error);

    g_vitte_last_error_initialized =
        true;
}

/* ========================================================================= */
/* Public lifetime API                                                       */
/* ========================================================================= */

void
vitte_error_init(
    vitte_error_t *error)
{
    vitte_error_initialize_ok_raw(
        error);
}

void
vitte_error_reset(
    vitte_error_t *error)
{
    vitte_error_initialize_ok_raw(
        error);
}

/* ========================================================================= */
/* Public state queries                                                      */
/* ========================================================================= */

bool
vitte_error_is_set(
    const vitte_error_t *error)
{
    return
        error != NULL &&
        error->status != VITTE_STATUS_OK;
}

bool
vitte_error_is_ok(
    const vitte_error_t *error)
{
    return
        error != NULL &&
        error->status == VITTE_STATUS_OK;
}

/* ========================================================================= */
/* Public setters                                                            */
/* ========================================================================= */

void
vitte_error_set(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message)
{
    vitte_error_set_details(
        error,
        status,
        code,
        message,
        NULL);
}

void
vitte_error_set_details(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details)
{
    const char *normalized_code;
    const char *normalized_message;

    char code_copy[VITTE_ERROR_DETAILS_CAPACITY];
    char message_copy[VITTE_ERROR_DETAILS_CAPACITY];

    size_t code_length;
    size_t message_length;

    bool code_aliases_details;
    bool message_aliases_details;

    if (error == NULL) {
        return;
    }

    /*
     * code/message are borrowed by the error object.
     *
     * If either pointer currently points into details_storage, writing the new
     * details would invalidate it. Preserve its textual value long enough to
     * choose a safe fallback.
     *
     * Dynamic code/message strings should not normally live in
     * details_storage; this path exists for defensive alias safety.
     */

    code_aliases_details =
        vitte_error_pointer_inside_details(
            error,
            code);

    message_aliases_details =
        vitte_error_pointer_inside_details(
            error,
            message);

    code_length = 0u;
    message_length = 0u;

    if (code_aliases_details &&
        !vitte_error_string_is_empty(code)) {
        code_length =
            vitte_error_strnlen_internal(
                code,
                sizeof(code_copy) - 1u);

        if (code_length != 0u) {
            (void)memmove(
                code_copy,
                code,
                code_length);
        }

        code_copy[code_length] =
            '\0';
    }

    if (message_aliases_details &&
        !vitte_error_string_is_empty(message)) {
        message_length =
            vitte_error_strnlen_internal(
                message,
                sizeof(message_copy) - 1u);

        if (message_length != 0u) {
            (void)memmove(
                message_copy,
                message,
                message_length);
        }

        message_copy[message_length] =
            '\0';
    }

    normalized_code =
        vitte_error_normalize_code(
            status,
            code);

    normalized_message =
        vitte_error_normalize_message(
            status,
            message);

    /*
     * Because code/message are borrowed, pointers into details_storage cannot
     * safely remain metadata after details_storage is rewritten.
     *
     * Use canonical status metadata instead. This avoids creating metadata
     * pointers whose contents change underneath the object.
     */
    if (code_aliases_details) {
        normalized_code =
            vitte_status_name(status);

        (void)code_copy;
        (void)code_length;
    }

    if (message_aliases_details) {
        normalized_message =
            vitte_status_message(status);

        (void)message_copy;
        (void)message_length;
    }

    /*
     * Details are stored before metadata assignment because the supplied
     * details pointer itself may alias current inline storage.
     */
    vitte_error_store_details(
        error,
        details);

    error->status =
        status;

    error->code =
        normalized_code;

    error->message =
        normalized_message;

    /*
     * Preserve established Vitte API semantics: assigning an infrastructure
     * error also publishes it as the current thread's last error.
     */
    vitte_error_set_last(
        error);
}

/* ========================================================================= */
/* Public copy                                                               */
/* ========================================================================= */

void
vitte_error_copy(
    vitte_error_t *destination,
    const vitte_error_t *source)
{
    vitte_error_copy_raw(
        destination,
        source);
}

/* ========================================================================= */
/* Thread-local last-error API                                               */
/* ========================================================================= */

void
vitte_error_set_last(
    const vitte_error_t *error)
{
    vitte_error_initialize_tls();

    if (error == NULL) {
        vitte_error_initialize_ok_raw(
            &g_vitte_last_error);

        return;
    }

    /*
     * Legal and intentionally a no-op:
     *
     *     vitte_error_set_last(vitte_error_last());
     */
    if (error ==
        &g_vitte_last_error) {
        return;
    }

    vitte_error_copy_raw(
        &g_vitte_last_error,
        error);
}

const vitte_error_t *
vitte_error_last(void)
{
    vitte_error_initialize_tls();

    return &g_vitte_last_error;
}

void
vitte_error_clear_last(void)
{
    vitte_error_initialize_tls();

    vitte_error_initialize_ok_raw(
        &g_vitte_last_error);
}

/* ========================================================================= */
/* Public metadata helpers                                                   */
/* ========================================================================= */

bool
vitte_status_is_ok(
    vitte_status_t status)
{
    return
        status == VITTE_STATUS_OK;
}

bool
vitte_status_is_error(
    vitte_status_t status)
{
    return
        status != VITTE_STATUS_OK;
}

bool
vitte_status_is_known(
    vitte_status_t status)
{
    return
        vitte_status_is_known_internal(
            status);
}

/* ========================================================================= */
/* Public error validation                                                   */
/* ========================================================================= */

bool
vitte_error_validate(
    const vitte_error_t *error)
{
    return
        vitte_error_object_is_canonical_internal(
            error);
}

/* ========================================================================= */
/* Public details helpers                                                    */
/* ========================================================================= */

bool
vitte_error_has_details(
    const vitte_error_t *error)
{
    return
        error != NULL &&
        error->details != NULL &&
        error->details[0] != '\0';
}

size_t
vitte_error_details_length(
    const vitte_error_t *error)
{
    if (!vitte_error_has_details(
            error)) {
        return 0u;
    }

    return
        vitte_error_strnlen_internal(
            error->details,
            VITTE_ERROR_DETAILS_CAPACITY - 1u);
}

/* ========================================================================= */
/* Public normalization                                                      */
/* ========================================================================= */

/*
 * Repair an error object into the canonical representation expected by this
 * module.
 *
 * This is useful after:
 *
 *     - deserialization;
 *     - foreign API population;
 *     - raw structure movement;
 *     - partially initialized external objects.
 *
 * It deliberately does not update TLS state.
 */
void
vitte_error_normalize(
    vitte_error_t *error)
{
    vitte_status_t status;
    const char *code;
    const char *message;

    char details_copy[VITTE_ERROR_DETAILS_CAPACITY];
    size_t details_length;
    bool has_details;

    if (error == NULL) {
        return;
    }

    status =
        error->status;

    code =
        vitte_error_normalize_code(
            status,
            error->code);

    message =
        vitte_error_normalize_message(
            status,
            error->message);

    details_length = 0u;
    has_details = false;

    /*
     * Prefer details when it already points to this object's inline storage.
     * Otherwise only trust an external details pointer enough to perform the
     * bounded copy expected by the public ABI.
     */
    if (!vitte_error_string_is_empty(
            error->details)) {
        details_length =
            vitte_error_details_length_internal(
                error->details);

        if (details_length != 0u) {
            (void)memmove(
                details_copy,
                error->details,
                details_length);
        }

        details_copy[details_length] =
            '\0';

        has_details = true;
    }

    error->status =
        status;

    error->code =
        code;

    error->message =
        message;

    if (status ==
        VITTE_STATUS_OK) {
        error->details_storage[0] =
            '\0';

        error->details =
            NULL;

        return;
    }

    if (has_details) {
        if (details_length != 0u) {
            (void)memcpy(
                error->details_storage,
                details_copy,
                details_length);
        }

        error->details_storage[
            details_length] = '\0';

        error->details =
            error->details_storage;
    } else {
        error->details_storage[0] =
            '\0';

        error->details =
            NULL;
    }
}

/* ========================================================================= */
/* Architecture guarantees                                                   */
/* ========================================================================= */

/*
 * ERROR OBJECT INVARIANTS
 * -----------------------
 *
 * Objects produced by this module obey:
 *
 * I1 — Canonical OK state
 *
 *     status  == VITTE_STATUS_OK
 *     code    != NULL
 *     message != NULL
 *     details == NULL
 *     details_storage[0] == '\0'
 *
 *
 * I2 — Owned details
 *
 *     details != NULL
 *
 * implies:
 *
 *     details == details_storage
 *
 *
 * I3 — Termination
 *
 * details_storage always contains a terminating NUL byte.
 *
 *
 * I4 — Bounded storage
 *
 * At most:
 *
 *     VITTE_ERROR_DETAILS_CAPACITY - 1
 *
 * detail bytes are retained.
 *
 *
 * I5 — Allocation freedom
 *
 * No operation in this module performs heap allocation.
 *
 *
 * I6 — Copy/report separation
 *
 * vitte_error_copy() never modifies TLS last-error state.
 *
 *
 * I7 — Setter reporting
 *
 * vitte_error_set() and vitte_error_set_details() publish the resulting
 * object as the calling thread's last error.
 *
 *
 * I8 — Non-recursive TLS publication
 *
 * vitte_error_set_last() uses the raw copy primitive and therefore cannot
 * recursively invoke itself.
 *
 *
 * I9 — Thread isolation
 *
 * Every thread owns an independent g_vitte_last_error.
 *
 *
 * I10 — Unknown status safety
 *
 * Unknown vitte_status_t values map to stable fallback metadata rather than
 * indexing tables or invoking undefined behavior.
 *
 *
 * I11 — Self-copy safety
 *
 *     vitte_error_copy(&error, &error)
 *
 * is legal.
 *
 *
 * I12 — Details alias safety
 *
 * Details may point into the destination's current details_storage while a
 * setter is executing.
 *
 *
 * I13 — Movement repair
 *
 * Public copy/normalization operations rebind details to destination-owned
 * storage.
 */

/* ========================================================================= */
/* Diagnostic architecture                                                   */
/* ========================================================================= */

/*
 * vitte_error_t must remain a compact infrastructure abstraction.
 *
 * Do not extend it with:
 *
 *     source spans
 *     labels
 *     fix-its
 *     expected/found types
 *     source excerpts
 *     declaration locations
 *     causal chains
 *     diagnostic severity
 *     parser recovery information
 *     SARIF/LSP metadata
 *
 * Those belong to:
 *
 *     vitte_diagnostic_t
 *
 *
 * Intended layering:
 *
 *     allocator/filesystem/API/driver
 *                  |
 *                  v
 *          vitte_status_t
 *          vitte_error_t
 *
 *
 *     source
 *       |
 *       v
 *     lexer
 *       |
 *       v
 *     parser
 *       |
 *       v
 *     AST
 *       |
 *       v
 *     import/name resolution
 *       |
 *       v
 *     semantic/type analysis
 *       |
 *       v
 *     contracts/constant evaluation
 *       |
 *       v
 *     HIR
 *       |
 *       v
 *     IR
 *       |
 *       v
 *     C17 backend
 *       |
 *       v
 *     external compiler/linker
 *       |
 *       v
 *     vitte_diagnostic_t
 *
 *
 * A phase may therefore return a compact infrastructure status while also
 * emitting one or more structured diagnostics.
 *
 * Example:
 *
 *     VITTE_STATUS_ERROR_PARSE
 *
 * describes the control-flow failure class.
 *
 *     E02xx
 *
 * describes the exact parser problem.
 */

/* ========================================================================= */
/* Status policy                                                             */
/* ========================================================================= */

/*
 * vitte_status_t is intentionally coarse-grained.
 *
 * Current status families:
 *
 *     VITTE_STATUS_OK
 *
 *     VITTE_STATUS_ERROR_INVALID_ARGUMENT
 *     VITTE_STATUS_ERROR_INVALID_STATE
 *     VITTE_STATUS_ERROR_OUT_OF_MEMORY
 *     VITTE_STATUS_ERROR_IO
 *     VITTE_STATUS_ERROR_PARSE
 *     VITTE_STATUS_ERROR_UNSUPPORTED
 *     VITTE_STATUS_ERROR_BACKEND
 *     VITTE_STATUS_ERROR_INTERNAL
 *
 * Do not create infrastructure statuses for every source-level condition.
 *
 * Examples such as:
 *
 *     unknown identifier
 *     unknown type
 *     type mismatch
 *     wrong arity
 *     invalid contract
 *     failed constant evaluation
 *
 * belong to the diagnostic registry and stable E/W/N/H diagnostic codes.
 */

/* ========================================================================= */
/* Ownership model                                                           */
/* ========================================================================= */

/*
 * status
 *     value
 *
 * code
 *     borrowed immutable string
 *
 * message
 *     borrowed immutable string
 *
 * details
 *     NULL or pointer to details_storage for canonical objects
 *
 * details_storage
 *     inline object-owned storage
 *
 *
 * Dynamic contextual information belongs in details.
 *
 * code/message should normally be static strings or strings whose lifetime
 * exceeds that of the error object.
 *
 * Example:
 *
 *     vitte_error_set_details(
 *         &error,
 *         VITTE_STATUS_ERROR_IO,
 *         "VITTE_IO_E_READ",
 *         "unable to read source file",
 *         path_details);
 *
 * path_details is copied immediately.
 */

/* ========================================================================= */
/* Thread-safety model                                                       */
/* ========================================================================= */

/*
 * Independent vitte_error_t objects require no synchronization.
 *
 * The last-error facility is _Thread_local:
 *
 *     thread A -> independent last error
 *     thread B -> independent last error
 *     thread C -> independent last error
 *
 * No process-global mutex is required.
 *
 * Sharing the same ordinary vitte_error_t object between threads still
 * requires external synchronization.
 */

/* ========================================================================= */
/* Failure semantics                                                         */
/* ========================================================================= */

/*
 * Typical infrastructure function:
 *
 *     vitte_status_t
 *     operation(...)
 *     {
 *         if (invalid) {
 *             vitte_error_set(
 *                 &context->last_error,
 *                 VITTE_STATUS_ERROR_INVALID_ARGUMENT,
 *                 "VITTE_API_E_INVALID_ARGUMENT",
 *                 "invalid argument");
 *
 *             return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
 *         }
 *
 *         return VITTE_STATUS_OK;
 *     }
 *
 *
 * Source-facing compiler phases should additionally emit structured
 * diagnostics through the diagnostic engine.
 */

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
