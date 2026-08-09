#ifndef VITTE_BOOTSTRAP_API_ERROR_H
#define VITTE_BOOTSTRAP_API_ERROR_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_ERROR_DETAILS_CAPACITY ((size_t)4096u)

typedef enum vitte_status {
    VITTE_STATUS_OK = 0,
    VITTE_STATUS_ERROR_INVALID_ARGUMENT,
    VITTE_STATUS_ERROR_INVALID_STATE,
    VITTE_STATUS_ERROR_OUT_OF_MEMORY,
    VITTE_STATUS_ERROR_IO,
    VITTE_STATUS_ERROR_PARSE,
    VITTE_STATUS_ERROR_UNSUPPORTED,
    VITTE_STATUS_ERROR_BACKEND,
    VITTE_STATUS_ERROR_INTERNAL
} vitte_status_t;

typedef struct vitte_error {
    vitte_status_t status;
    const char *code;
    const char *message;
    const char *details;
    char details_storage[VITTE_ERROR_DETAILS_CAPACITY];
} vitte_error_t;

void vitte_error_init(vitte_error_t *error);
void vitte_error_reset(vitte_error_t *error);

bool vitte_error_is_set(const vitte_error_t *error);
bool vitte_error_is_ok(const vitte_error_t *error);

void vitte_error_set(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message
);

void vitte_error_set_details(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
);

void vitte_error_copy(
    vitte_error_t *destination,
    const vitte_error_t *source
);

const char *vitte_status_name(vitte_status_t status);
const char *vitte_status_message(vitte_status_t status);

void vitte_error_set_last(const vitte_error_t *error);
const vitte_error_t *vitte_error_last(void);
void vitte_error_clear_last(void);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_API_ERROR_H */
