#ifndef VITTE_PLATFORM_PLATFORM_H
#define VITTE_PLATFORM_PLATFORM_H

/* Small host boundary used by the bootstrap driver. */

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool
vitte_platform_getcwd(
    char *buffer,
    size_t capacity);

bool
vitte_platform_is_executable(
    const char *path);

char
vitte_platform_path_list_separator(void);

/* Run a compiler tool and wait for it to finish successfully.  The argument
 * array is NULL terminated and remains caller-owned. */
bool
vitte_platform_run(
    const char *const arguments[],
    bool inherit_environment);

#ifdef __cplusplus
}
#endif

#endif
