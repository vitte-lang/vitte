#ifndef VITTE_BOOTSTRAP_API_VERSION_H
#define VITTE_BOOTSTRAP_API_VERSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_VERSION_MAJOR 0u
#define VITTE_VERSION_MINOR 1u
#define VITTE_VERSION_PATCH 0u
#define VITTE_ABI_VERSION 1u

typedef struct vitte_version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    uint32_t abi;
    const char *string;
    const char *build_kind;
} vitte_version_t;

const vitte_version_t *vitte_version(void);
uint32_t vitte_version_major(void);
uint32_t vitte_version_minor(void);
uint32_t vitte_version_patch(void);
uint32_t vitte_abi_version(void);
const char *vitte_version_string(void);
const char *vitte_build_kind(void);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_API_VERSION_H */
