#ifndef VITTE_SDK_RUNTIME_RUNTIME_H
#define VITTE_SDK_RUNTIME_RUNTIME_H




#include "vitte/abi/abi_types.h"
#include "vitte/abi/abi_result.h"
#include "vitte/abi/abi_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime lifecycle */
vitte_u32 vitte_runtime_init(void);
vitte_u32 vitte_runtime_shutdown(void);

/* Panic hook */
typedef void (*vitte_panic_hook_t)(vitte_string_t msg);
void vitte_panic_set_hook(vitte_panic_hook_t hook);

/* Allocator (SDK-level). The runtime may override, but ABI stays. */
void* vitte_alloc(vitte_u64 size, vitte_u64 align);
void  vitte_free(void* ptr, vitte_u64 size, vitte_u64 align);

/* Utilities */
vitte_u32 vitte_runtime_version_major(void);
vitte_u32 vitte_runtime_version_minor(void);
vitte_u32 vitte_runtime_version_patch(void);

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* VITTE_SDK_RUNTIME_RUNTIME_H */
