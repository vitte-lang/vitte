
#include "vitte/runtime/runtime.h"
#include <stdlib.h>
#include <string.h>

static vitte_panic_hook_t g_hook = 0;

vitte_u32 vitte_runtime_init(void) { return 0; }
vitte_u32 vitte_runtime_shutdown(void) { return 0; }

void vitte_panic_set_hook(vitte_panic_hook_t hook) { g_hook = hook; }

void* vitte_alloc(vitte_u64 size, vitte_u64 align) {
  (void)align;
  void* p = malloc((size_t)size);
  return p;
}

void vitte_free(void* ptr, vitte_u64 size, vitte_u64 align) {
  (void)size; (void)align;
  free(ptr);
}

vitte_u32 vitte_runtime_version_major(void) { return VITTE_ABI_VERSION_MAJOR; }
vitte_u32 vitte_runtime_version_minor(void) { return VITTE_ABI_VERSION_MINOR; }
vitte_u32 vitte_runtime_version_patch(void) { return VITTE_ABI_VERSION_PATCH; }
