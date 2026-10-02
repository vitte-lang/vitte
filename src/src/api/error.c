#include "error.h"

#include <stddef.h>
#include <string.h>

/*
 * ============================================================================
 * Vitte Core — Infrastructure Error Layer
 * ============================================================================
 *
 * This module implements the small, allocation-free error object used by
 * Vitte's infrastructure and public API.
 *
 * IMPORTANT
 * ---------
 *
 * vitte_error_t is deliberately NOT the compiler diagnostic representation.
 *
 * Infrastructure:
 *
 *      vitte_status_t
 *          |
 *          v
 *      vitte_error_t
 *
 * Compiler diagnostics:
 *
 *      lexer/parser/import/sema/HIR/IR/backend
 *          |
 *          v
 *      vitte_diagnostic_t
 *
 * vitte_error_t answers:
 *
 *      "Did this operation fail, and why?"
 *
 * vitte_diagnostic_t answers:
 *
 *      "Where in Vitte source did compilation fail, why did it fail,
 *       what caused it, and how can the user fix it?"
 *
 * Properties of this implementation:
 *
 *   - C17
 *   - no heap allocation
 *   - thread-local last error
 *   - bounded details storage
 *   - self-copy safe
 *   - alias-safe details copying
 *   - deterministic status metadata
 *   - defensive handling of invalid enum values
 *   - no recursive TLS updates
 *   - no dangling details pointer after copy
 *   - stable OK state
 *   - compatible with the current error.h ABI
 */

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
 * C guarantees zero-initialization for thread-local objects.
 *
 * We nevertheless maintain an explicit initialization flag because the
 * canonical OK state contains non-NULL code/message pointers.
 */

static _Thread_local vitte_error_t g_vitte_last_error;
static _Thread_local bool g_vitte_last_error_initialized = false;

/* ========================================================================= */
/* Internal string helpers                                                   */
/* ========================================================================= */

static bool
vitte_error_string_is_empty(
    const char *text
)
{
    return text == NULL || text[0] == '\0';
}

static size_t
vitte_error_strnlen_internal(
    const char *text,
    size_t maximum
)
{
    size_t length = 0u;

    if (text == NULL) {
        return 0u;
    }

    while (length < maximum &&
           text[length] != '\0') {
        ++length;
    }

    return length;
}

static bool
vitte_error_pointer_inside_details(
    const vitte_error_t *error,
    const char *pointer
)
{
    const char *begin;
    const char *end;

    if (error == NULL || pointer == NULL) {
        return false;
    }

    begin = error->details_storage;
    end = error->details_storage +
          sizeof(error->details_storage);

    return pointer >= begin &&
           pointer < end;
}

/* ========================================================================= */
/* Status metadata                                                           */
/* ========================================================================= */

const char *
vitte_status_name(
    vitte_status_t status
)
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

        default:
            return VITTE_ERROR_CODE_UNKNOWN;
    }
}

const char *
vitte_status_message(
    vitte_status_t status
)
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

        default:
            return VITTE_ERROR_MESSAGE_UNKNOWN;
    }
}

/* ========================================================================= */
/* Internal canonical initialization                                         */
/* ========================================================================= */

static void
vitte_error_initialize_ok_raw(
    vitte_error_t *error
)
{
    if (error == NULL) {
        return;
    }

    error->status = VITTE_STATUS_OK;
    error->code = VITTE_ERROR_CODE_OK;
    error->message = VITTE_ERROR_MESSAGE_OK;
    error->details = NULL;
    error->details_storage[0] = '\0';
}

/* ========================================================================= */
/* Details storage                                                           */
/* ========================================================================= */

/*
 * Copy details into destination-owned inline storage.
 *
 * The temporary buffer is intentional.
 *
 * Consider:
 *
 *     error.details = error.details_storage + 3;
 *     vitte_error_set_details(&error, ..., error.details);
 *
 * Clearing destination storage before reading details would destroy the
 * source. The temporary buffer makes this operation alias-safe.
 *
 * The buffer is stack-local, fixed-size and allocation-free.
 */
static void
vitte_error_store_details(
    vitte_error_t *error,
    const char *details
)
{
    char temporary[VITTE_ERROR_DETAILS_CAPACITY];
    size_t length;

    if (error == NULL) {
        return;
    }

    if (vitte_error_string_is_empty(details)) {
        error->details = NULL;
        error->details_storage[0] = '\0';
        return;
    }

    length = vitte_error_strnlen_internal(
        details,
        VITTE_ERROR_DETAILS_CAPACITY - 1u
    );

    if (vitte_error_pointer_inside_details(
            error,
            details)) {
        if (length != 0u) {
            (void)memcpy(
                temporary,
                details,
                length
            );
        }

        temporary[length] = '\0';

        if (length != 0u) {
            (void)memcpy(
                error->details_storage,
                temporary,
                length
            );
        }
    } else {
        if (length != 0u) {
            (void)memcpy(
                error->details_storage,
                details,
                length
            );
        }
    }

    error->details_storage[length] = '\0';
    error->details = error->details_storage;
}

/* ========================================================================= */
/* Internal normalization                                                    */
/* ========================================================================= */

static const char *
vitte_error_normalize_code(
    vitte_status_t status,
    const char *code
)
{
    if (!vitte_error_string_is_empty(code)) {
        return code;
    }

    return vitte_status_name(status);
}

static const char *
vitte_error_normalize_message(
    vitte_status_t status,
    const char *message
)
{
    if (!vitte_error_string_is_empty(message)) {
        return message;
    }

    return vitte_status_message(status);
}

/* ========================================================================= */
/* Raw copy                                                                  */
/* ========================================================================= */

/*
 * Copy without modifying the thread-local last-error object.
 *
 * This is a fundamental invariant:
 *
 *     raw copy != report error
 *
 * Keeping these operations separate prevents:
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
    const vitte_error_t *source
)
{
    const char *source_code;
    const char *source_message;
    char details_copy[VITTE_ERROR_DETAILS_CAPACITY];
    size_t details_length = 0u;
    bool has_details = false;

    if (destination == NULL) {
        return;
    }

    if (source == NULL) {
        vitte_error_initialize_ok_raw(destination);
        return;
    }

    if (destination == source) {
        return;
    }

    /*
     * Capture everything before mutating destination.
     *
     * This also makes the implementation robust if a caller constructed
     * unusual aliases involving destination storage.
     */

    source_code =
        vitte_error_normalize_code(
            source->status,
            source->code
        );

    source_message =
        vitte_error_normalize_message(
            source->status,
            source->message
        );

    if (!vitte_error_string_is_empty(source->details)) {
        details_length =
            vitte_error_strnlen_internal(
                source->details,
                VITTE_ERROR_DETAILS_CAPACITY - 1u
            );

        if (details_length != 0u) {
            (void)memcpy(
                details_copy,
                source->details,
                details_length
            );
        }

        details_copy[details_length] = '\0';
        has_details = true;
    }

    destination->status = source->status;
    destination->code = source_code;
    destination->message = source_message;

    if (has_details) {
        if (details_length != 0u) {
            (void)memcpy(
                destination->details_storage,
                details_copy,
                details_length
            );
        }

        destination->details_storage[details_length] = '\0';
        destination->details =
            destination->details_storage;
    } else {
        destination->details_storage[0] = '\0';
        destination->details = NULL;
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
        &g_vitte_last_error
    );

    g_vitte_last_error_initialized = true;
}

/* ========================================================================= */
/* Public lifetime API                                                       */
/* ========================================================================= */

void
vitte_error_init(
    vitte_error_t *error
)
{
    vitte_error_initialize_ok_raw(error);
}

void
vitte_error_reset(
    vitte_error_t *error
)
{
    vitte_error_initialize_ok_raw(error);
}

/* ========================================================================= */
/* Public state queries                                                      */
/* ========================================================================= */

bool
vitte_error_is_set(
    const vitte_error_t *error
)
{
    return error != NULL &&
           error->status != VITTE_STATUS_OK;
}

bool
vitte_error_is_ok(
    const vitte_error_t *error
)
{
    return error != NULL &&
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
    const char *message
)
{
    vitte_error_set_details(
        error,
        status,
        code,
        message,
        NULL
    );
}

void
vitte_error_set_details(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
)
{
    const char *normalized_code;
    const char *normalized_message;

    if (error == NULL) {
        return;
    }

    /*
     * Resolve fallback metadata before changing the object.
     */
    normalized_code =
        vitte_error_normalize_code(
            status,
            code
        );

    normalized_message =
        vitte_error_normalize_message(
            status,
            message
        );

    /*
     * Store details first because details may alias the current object's
     * inline details storage.
     */
    vitte_error_store_details(
        error,
        details
    );

    error->status = status;
    error->code = normalized_code;
    error->message = normalized_message;

    /*
     * Preserve the established Vitte API behavior:
     *
     * setting an error object also updates the per-thread last error.
     */
    vitte_error_set_last(error);
}

/* ========================================================================= */
/* Public copy                                                               */
/* ========================================================================= */

void
vitte_error_copy(
    vitte_error_t *destination,
    const vitte_error_t *source
)
{
    vitte_error_copy_raw(
        destination,
        source
    );
}

/* ========================================================================= */
/* Thread-local last error API                                               */
/* ========================================================================= */

void
vitte_error_set_last(
    const vitte_error_t *error
)
{
    vitte_error_initialize_tls();

    if (error == NULL) {
        vitte_error_initialize_ok_raw(
            &g_vitte_last_error
        );

        return;
    }

    /*
     * This is legal:
     *
     *     vitte_error_set_last(vitte_error_last());
     */
    if (error == &g_vitte_last_error) {
        return;
    }

    vitte_error_copy_raw(
        &g_vitte_last_error,
        error
    );
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
        &g_vitte_last_error
    );
}

/* ========================================================================= */
/* Internal invariants documented for maintainers                            */
/* ========================================================================= */

/*
 * ERROR OBJECT INVARIANTS
 * -----------------------
 *
 * I1
 *
 *     status == VITTE_STATUS_OK
 *
 * does not require details == NULL from arbitrary external callers, but all
 * constructors in this module create the canonical state:
 *
 *     status  = VITTE_STATUS_OK
 *     code    = "VITTE_OK"
 *     message = "ok"
 *     details = NULL
 *
 *
 * I2
 *
 * If details != NULL for an error produced or copied by this module:
 *
 *     details == details_storage
 *
 *
 * I3
 *
 * details_storage is always NUL-terminated after every public operation.
 *
 *
 * I4
 *
 * Details never exceed:
 *
 *     VITTE_ERROR_DETAILS_CAPACITY - 1
 *
 * bytes.
 *
 *
 * I5
 *
 * This module performs no heap allocation.
 *
 *
 * I6
 *
 * vitte_error_copy() does not change the TLS last error.
 *
 *
 * I7
 *
 * vitte_error_set() and vitte_error_set_details() DO change the TLS last
 * error, preserving existing API semantics.
 *
 *
 * I8
 *
 * vitte_error_set_last() performs a raw copy and therefore cannot recursively
 * update itself.
 *
 *
 * I9
 *
 * Every thread has independent last-error state because
 * g_vitte_last_error is _Thread_local.
 *
 *
 * I10
 *
 * Unknown vitte_status_t values are converted to stable fallback strings
 * rather than causing undefined behavior.
 */

/* ========================================================================= */
/* Diagnostic architecture                                                   */
/* ========================================================================= */

/*
 * Do NOT extend vitte_error_t with source diagnostics merely because another
 * compiler phase needs richer errors.
 *
 * The intended architecture is:
 *
 *     API / filesystem / allocator / driver
 *                    |
 *                    v
 *             vitte_error_t
 *
 *
 *     Vitte source
 *          |
 *          v
 *       lexer
 *          |
 *          v
 *       parser
 *          |
 *          v
 *       AST
 *          |
 *          v
 *       resolver
 *          |
 *          v
 *       semantic analysis
 *          |
 *          v
 *       type checker
 *          |
 *          v
 *       contract checker
 *          |
 *          v
 *       HIR
 *          |
 *          v
 *       IR
 *          |
 *          v
 *       C17 backend
 *          |
 *          v
 *       generated C
 *          |
 *          v
 *       clang/gcc
 *
 * All source-facing failures should eventually converge on:
 *
 *              vitte_diagnostic_t
 *
 * carrying:
 *
 *     stable diagnostic code
 *     severity
 *     phase
 *     category
 *     primary source span
 *     secondary spans
 *     declaration origin
 *     definition origin
 *     value origin
 *     call site
 *     instantiation site
 *     labels
 *     notes
 *     help
 *     fix-its
 *     applicability
 *     causes
 *     provenance
 *     source mapping
 *     cascade/root relation
 *
 *
 * Typical mapping:
 *
 *     VITTE_STATUS_ERROR_PARSE
 *
 *         infrastructure status
 *
 *                    +
 *
 *     E02xx
 *
 *         structured parser diagnostic
 *
 *
 *     VITTE_STATUS_ERROR_BACKEND
 *
 *                    +
 *
 *     E09xx
 *
 *         structured C17/backend diagnostic
 *
 *
 *     VITTE_STATUS_ERROR_INTERNAL
 *
 *                    +
 *
 *     E00xx
 *
 *         structured compiler/internal diagnostic
 */

/* ========================================================================= */
/* Status policy                                                             */
/* ========================================================================= */

/*
 * Current public status model:
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
 * Keep this list deliberately compact.
 *
 * Do NOT create one vitte_status_t value per diagnostic condition.
 *
 * For example:
 *
 *     unknown identifier
 *     mismatched type
 *     invalid contract
 *     unresolved trait
 *     wrong argument count
 *
 * belong to diagnostic codes such as:
 *
 *     E04xx
 *     E05xx
 *     E07xx
 *     E08xx
 *     E06xx
 *
 * rather than exploding the infrastructure status enumeration.
 */

/* ========================================================================= */
/* Ownership model                                                           */
/* ========================================================================= */

/*
 * code:
 *
 *     borrowed
 *
 * message:
 *
 *     borrowed
 *
 * details:
 *
 *     owned when produced by this implementation
 *
 * details_storage:
 *
 *     inline owned storage
 *
 *
 * This means callers should normally pass static or otherwise sufficiently
 * long-lived strings for code/message.
 *
 * Example:
 *
 *     vitte_error_set(
 *         &error,
 *         VITTE_STATUS_ERROR_PARSE,
 *         "VITTE_PARSER_E_EXPECTED_TOKEN",
 *         "expected token"
 *     );
 *
 * Dynamic contextual text belongs in details:
 *
 *     vitte_error_set_details(
 *         &error,
 *         VITTE_STATUS_ERROR_IO,
 *         "VITTE_IO_E_READ",
 *         "unable to read source file",
 *         dynamic_details
 *     );
 *
 * details is copied immediately.
 */

/* ========================================================================= */
/* Thread safety                                                             */
/* ========================================================================= */

/*
 * vitte_error_t itself has no synchronization.
 *
 * Independent error objects may safely be used by independent threads.
 *
 * The global-looking last-error object is actually:
 *
 *     _Thread_local
 *
 * and therefore each thread receives independent state.
 *
 * No mutex is required for vitte_error_last().
 */

/* ========================================================================= */
/* Failure semantics                                                         */
/* ========================================================================= */

/*
 * A function returning vitte_status_t should generally follow:
 *
 *     vitte_status_t function(...)
 *     {
 *         if (...) {
 *             vitte_error_set(...);
 *             return VITTE_STATUS_ERROR_...;
 *         }
 *
 *         return VITTE_STATUS_OK;
 *     }
 *
 * A function returning a pointer may use:
 *
 *     if (...) {
 *         vitte_error_set(...);
 *         return NULL;
 *     }
 *
 * Components with their own context object should normally store:
 *
 *     vitte_error_t last_error;
 *
 * and expose:
 *
 *     component_last_error(...)
 *
 * The TLS last error remains useful for APIs where no context object is
 * available.
 */

/* ========================================================================= */
/* End                                                                       */
/* ========================================================================= */
