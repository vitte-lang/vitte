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

int32_t vitte_host_runtime_available(void);
VitteString vitte_host_read_file(VitteString path);
int32_t vitte_host_write_file(VitteString path, VitteString content);
int32_t vitte_host_append_file(VitteString path, VitteString content);
int32_t vitte_host_file_exists(VitteString path);
int32_t vitte_host_is_file(VitteString path);
int32_t vitte_host_is_directory(VitteString path);
int32_t vitte_host_mkdir_all(VitteString path);
int32_t vitte_host_delete_file(VitteString path);
int32_t vitte_host_copy_file(VitteString src, VitteString dst);
int32_t vitte_host_move_file(VitteString src, VitteString dst);
int32_t vitte_host_delete_directory(VitteString path);
VitteSliceString vitte_host_list_directory(VitteString path);
int32_t vitte_host_system(VitteString command);
int32_t vitte_host_emit_llvm_object(VitteString ir_text, VitteString compiler_path, VitteString target_triple, VitteString sysroot_path, VitteString object_path);
int32_t vitte_host_emit_assembly_object(VitteString assembly_text, VitteString assembler_path, VitteString target_triple, VitteString sysroot_path, VitteString object_path, int32_t debug_enabled);
int32_t vitte_host_verify_native_object(VitteString object_path, VitteString target_triple, VitteString expected_symbol, int32_t require_relocations, int32_t require_debug);
int32_t vitte_host_link_executable(VitteString linker_path, VitteString target_triple, VitteString sysroot_path, VitteString object_path, VitteString runtime_source_path, VitteString runtime_include_path, VitteString executable_path);
int32_t vitte_host_run_executable(VitteString executable_path);

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
