#ifndef VITTE_BOOTSTRAP_POINTER_POINTER_H
#define VITTE_BOOTSTRAP_POINTER_POINTER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uintptr_t
vitte_core_pointer_address(const void *value);

void *
vitte_core_pointer_add_bytes(void *value, size_t offset);

const void *
vitte_core_pointer_add_bytes_const(const void *value, size_t offset);

void *
vitte_core_pointer_from_address(uintptr_t value);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_POINTER_POINTER_H */
