#ifndef COMPILER_DIAGNOSTIC_H
#define COMPILER_DIAGNOSTIC_H

#include <stdint.h>

typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE,
    DIAG_INFO,
} diagnostic_level_t;

typedef struct diagnostic {
    diagnostic_level_t level;
    const char *message;
    uint32_t line;
    uint32_t col;
    const char *source_file;
} diagnostic_t;

typedef struct diagnostic_ctx {
    diagnostic_t **diags;
    uint32_t count;
    uint32_t capacity;
} diagnostic_ctx_t;

diagnostic_ctx_t* diagnostic_ctx_create(void);
void diagnostic_ctx_destroy(diagnostic_ctx_t *ctx);

void diagnostic_emit(diagnostic_ctx_t *ctx, diagnostic_level_t level,
                     const char *message, uint32_t line, uint32_t col);

#endif /* COMPILER_DIAGNOSTIC_H */
