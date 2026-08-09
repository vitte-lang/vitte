#include "error.h"

#include <string.h>

static _Thread_local vitte_error_t g_vitte_last_error;

static void vitte_error_copy_details_text(
    vitte_error_t *error,
    const char *details
) {
    size_t length;

    if (error == NULL) {
        return;
    }
    error->details = NULL;
    error->details_storage[0] = '\0';
    if (details == NULL || details[0] == '\0') {
        return;
    }
    length = strlen(details);
    if (length >= sizeof(error->details_storage)) {
        length = sizeof(error->details_storage) - 1u;
    }
    if (length > 0u) {
        (void)memcpy(error->details_storage, details, length);
    }
    error->details_storage[length] = '\0';
    error->details = error->details_storage;
}

void vitte_error_init(vitte_error_t *error) {
    if (error == NULL) {
        return;
    }

    error->status = VITTE_STATUS_OK;
    error->code = "VITTE_OK";
    error->message = "ok";
    error->details = NULL;
    error->details_storage[0] = '\0';
}

void vitte_error_reset(vitte_error_t *error) {
    vitte_error_init(error);
}

bool vitte_error_is_set(const vitte_error_t *error) {
    return error != NULL && error->status != VITTE_STATUS_OK;
}

bool vitte_error_is_ok(const vitte_error_t *error) {
    return error != NULL && error->status == VITTE_STATUS_OK;
}

void vitte_error_set(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message
) {
    vitte_error_set_details(error, status, code, message, NULL);
}

void vitte_error_set_details(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (error == NULL) {
        return;
    }

    error->status = status;
    error->code = code != NULL ? code : vitte_status_name(status);
    error->message = message != NULL ? message : vitte_status_message(status);
    vitte_error_copy_details_text(error, details);

    vitte_error_set_last(error);
}

void vitte_error_copy(
    vitte_error_t *destination,
    const vitte_error_t *source
) {
    if (destination == NULL) {
        return;
    }

    if (source == NULL) {
        vitte_error_reset(destination);
        return;
    }

    destination->status = source->status;
    destination->code = source->code;
    destination->message = source->message;
    vitte_error_copy_details_text(destination, source->details);
}

const char *vitte_status_name(vitte_status_t status) {
    switch (status) {
        case VITTE_STATUS_OK:
            return "VITTE_OK";
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
            return "VITTE_ERROR_UNKNOWN";
    }
}

const char *vitte_status_message(vitte_status_t status) {
    switch (status) {
        case VITTE_STATUS_OK:
            return "ok";
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
            return "internal error";
        default:
            return "unknown error";
    }
}

void vitte_error_set_last(const vitte_error_t *error) {
    if (error == NULL) {
        vitte_error_clear_last();
        return;
    }

    vitte_error_copy(&g_vitte_last_error, error);
}

const vitte_error_t *vitte_error_last(void) {
    if (g_vitte_last_error.code == NULL) {
        vitte_error_init(&g_vitte_last_error);
    }

    return &g_vitte_last_error;
}

void vitte_error_clear_last(void) {
    memset(&g_vitte_last_error, 0, sizeof(g_vitte_last_error));
    vitte_error_init(&g_vitte_last_error);
}
