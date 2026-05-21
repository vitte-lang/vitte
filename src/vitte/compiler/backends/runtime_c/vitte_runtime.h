#ifndef VITTE_RUNTIME_H
#define VITTE_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *data;
  size_t len;
} VitteString;

typedef struct {
  int32_t *data;
  size_t len;
} VitteSliceI32;

typedef struct {
  VitteString *data;
  size_t len;
} VitteSliceString;

const char *vitte_c_abi_version(void);
void vitte_builtin_trap(VitteString msg);

int32_t vitte_runtime_panic_boundary_begin(void);
int32_t vitte_runtime_panic_boundary_end(void);
int32_t vitte_runtime_panic_boundary_triggered(void);
int32_t vitte_runtime_panic_boundary_code(void);
int32_t vitte_runtime_panic_boundary_reset(void);

VitteSliceI32 vitte_empty_slice_i32(void);
VitteSliceString vitte_empty_slice_string(void);
VitteSliceI32 vitte_slice_push_i32(VitteSliceI32 base, int32_t value);
VitteSliceString vitte_slice_push_string(VitteSliceString base, VitteString value);
VitteString vitte_string_concat(VitteString a, VitteString b);
VitteString vitte_i32_to_string(int32_t v);
void vitte_set_args(int argc, const char **argv);
VitteSliceString cli_args(void);

#ifdef __cplusplus
}
#endif

#endif
