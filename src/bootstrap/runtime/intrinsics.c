#include <stddef.h>
#include <stdlib.h>

/* Host implementation for the core memory intrinsics emitted by C17. */
void *
native_allocate(size_t size)
{
    /* Keep zero-sized allocations well-defined and freeable. */
    return malloc(size == 0u ? 1u : size);
}

void
native_deallocate(void *pointer)
{
    free(pointer);
}
