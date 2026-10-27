#include "pointer.h"

#include <limits.h>

uintptr_t
vitte_core_pointer_address(const void *value)
{
    return (uintptr_t)value;
}

void *
vitte_core_pointer_add_bytes(void *value, size_t offset)
{
    uintptr_t address;

    if (value == NULL || offset == 0u) {
        return value;
    }

    address = (uintptr_t)value;
    if ((uintmax_t)offset > (uintmax_t)(UINTPTR_MAX - address)) {
        return NULL;
    }

    return (void *)(address + (uintptr_t)offset);
}

const void *
vitte_core_pointer_add_bytes_const(const void *value, size_t offset)
{
    uintptr_t address;

    if (value == NULL || offset == 0u) {
        return value;
    }

    address = (uintptr_t)value;
    if ((uintmax_t)offset > (uintmax_t)(UINTPTR_MAX - address)) {
        return NULL;
    }

    return (const void *)(address + (uintptr_t)offset);
}

void *
vitte_core_pointer_from_address(uintptr_t value)
{
    return (void *)value;
}
