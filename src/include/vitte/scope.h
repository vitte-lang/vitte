#ifndef VITTE_INCLUDE_VITTE_SCOPE_H_H
#define VITTE_INCLUDE_VITTE_SCOPE_H_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_include_vitte_scope_h_module {
    const char *name;
    const char *category;
    const char *purpose;
    uint32_t version;
} vitte_include_vitte_scope_h_module_t;

const vitte_include_vitte_scope_h_module_t *vitte_include_vitte_scope_h_module(void);
const char *vitte_include_vitte_scope_h_name(void);
const char *vitte_include_vitte_scope_h_category(void);
const char *vitte_include_vitte_scope_h_purpose(void);
uint32_t vitte_include_vitte_scope_h_checksum(const char *text, size_t length);
bool vitte_include_vitte_scope_h_self_test(void);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_INCLUDE_VITTE_SCOPE_H_H */
