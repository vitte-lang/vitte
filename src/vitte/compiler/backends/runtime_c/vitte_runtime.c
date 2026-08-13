#define _POSIX_C_SOURCE 200809L

#include "vitte_runtime.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int32_t vitte_panic_active = 0;
static int32_t vitte_panic_triggered = 0;
static int32_t vitte_panic_code = 0;
static int vitte_argc = 0;
static const char** vitte_argv = NULL;

static void vitte_runtime_signal_panic(int32_t code) {
  vitte_panic_triggered = 1;
  vitte_panic_code = code;
}

static VitteString vitte_empty_string(void) {
  VitteString out = {NULL, 0};
  return out;
}

static int vitte_string_is_valid(VitteString value) {
  if (value.len == 0) {
    return 1;
  }
  if (value.data == NULL || value.len > (size_t)INT32_MAX) {
    vitte_runtime_signal_panic(3);
    return 0;
  }
  return 1;
}

static char* vitte_to_c_string(VitteString value) {
  char* out;
  if (!vitte_string_is_valid(value)) {
    return NULL;
  }
  if (memchr(value.data, '\0', value.len) != NULL) {
    vitte_runtime_signal_panic(2);
    return NULL;
  }
  out = (char*)malloc(value.len + 1);
  if (out == NULL) {
    vitte_runtime_signal_panic(3);
    return NULL;
  }
  if (value.len > 0) {
    memcpy(out, value.data, value.len);
  }
  out[value.len] = '\0';
  return out;
}

static int vitte_write_bytes(const char* path, const char* mode, VitteString content) {
  FILE* file;
  if (!vitte_string_is_valid(content)) {
    return -1;
  }
  file = fopen(path, mode);
  if (file == NULL) {
    return -1;
  }
  if (content.len > 0 && fwrite(content.data, 1, content.len, file) != content.len) {
    fclose(file);
    return -1;
  }
  if (fclose(file) != 0) {
    return -1;
  }
  return (int32_t)content.len;
}

static uint16_t vitte_read_u16_le(const unsigned char* data) {
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t vitte_read_u32_le(const unsigned char* data) {
  return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

static uint64_t vitte_read_u64_le(const unsigned char* data) {
  uint64_t lo = vitte_read_u32_le(data);
  uint64_t hi = vitte_read_u32_le(data + 4);
  return lo | (hi << 32);
}

const char *vitte_c_abi_version(void) {
  return VITTE_C_ABI_VERSION;
}

void vitte_builtin_trap(VitteString msg) {
  (void)msg;
  vitte_runtime_signal_panic(2);
}

int32_t vitte_runtime_panic_boundary_begin(void) {
  vitte_panic_active = 1;
  vitte_panic_triggered = 0;
  vitte_panic_code = 0;
  return 1;
}

int32_t vitte_runtime_panic_boundary_end(void) {
  vitte_panic_active = 0;
  return 0;
}

int32_t vitte_runtime_panic_boundary_triggered(void) {
  return vitte_panic_active ? vitte_panic_triggered : vitte_panic_triggered;
}

int32_t vitte_runtime_panic_boundary_code(void) {
  return vitte_panic_code;
}

int32_t vitte_runtime_panic_boundary_reset(void) {
  vitte_panic_active = 0;
  vitte_panic_triggered = 0;
  vitte_panic_code = 0;
  return 0;
}

VitteSliceI32 vitte_empty_slice_i32(void) {
  VitteSliceI32 out = {NULL, 0};
  return out;
}

VitteSliceString vitte_empty_slice_string(void) {
  VitteSliceString out = {NULL, 0};
  return out;
}

VitteSliceI32 vitte_slice_push_i32(VitteSliceI32 base, int32_t value) {
  int32_t* data;
  if (base.len == SIZE_MAX || base.len > (SIZE_MAX / sizeof(int32_t)) - 1) {
    vitte_runtime_signal_panic(3);
    return base;
  }
  data = (int32_t*)realloc(base.data, (base.len + 1) * sizeof(int32_t));
  if (data == NULL) {
    vitte_runtime_signal_panic(3);
    return base;
  }
  data[base.len] = value;
  base.data = data;
  base.len += 1;
  return base;
}

VitteSliceString vitte_slice_push_string(VitteSliceString base, VitteString value) {
  VitteSliceString out = base;
  VitteString* data;
  if (base.len == SIZE_MAX || base.len > (SIZE_MAX / sizeof(VitteString)) - 1) {
    vitte_runtime_signal_panic(3);
    return base;
  }
  data = (VitteString*)realloc(base.data, (base.len + 1) * sizeof(VitteString));
  if (data == NULL) {
    vitte_runtime_signal_panic(3);
    return base;
  }
  data[base.len] = value;
  out.data = data;
  out.len += 1;
  return out;
}

void vitte_string_release(VitteString value) {
  free((void*)value.data);
}

void vitte_slice_i32_release(VitteSliceI32 value) {
  free(value.data);
}

void vitte_slice_string_release(VitteSliceString value) {
  free(value.data);
}

void vitte_owned_slice_string_release(VitteSliceString value) {
  size_t i;
  for (i = 0; i < value.len; ++i) {
    vitte_string_release(value.data[i]);
  }
  free(value.data);
}

VitteString vitte_string_concat(VitteString a, VitteString b) {
  VitteString out = vitte_empty_string();
  char* data;
  if (!vitte_string_is_valid(a) || !vitte_string_is_valid(b) || a.len > SIZE_MAX - b.len) {
    vitte_runtime_signal_panic(3);
    return out;
  }
  data = (char*)malloc(a.len + b.len + 1);
  if (data == NULL) {
    vitte_runtime_signal_panic(3);
    return out;
  }
  memcpy(data, a.data, a.len);
  memcpy(data + a.len, b.data, b.len);
  data[a.len + b.len] = '\0';
  out.data = data;
  out.len = a.len + b.len;
  return out;
}

VitteString vitte_i32_to_string(int32_t v) {
  char buffer[64];
  int written = snprintf(buffer, sizeof(buffer), "%d", v);
  VitteString out = vitte_empty_string();
  char* data;
  if (written < 0) {
    return out;
  }
  data = (char*)malloc((size_t)written + 1);
  if (data == NULL) {
    vitte_runtime_signal_panic(3);
    return out;
  }
  memcpy(data, buffer, (size_t)written + 1);
  out.data = data;
  out.len = (size_t)written;
  return out;
}

void vitte_set_args(int argc, const char **argv) {
  vitte_argc = argc;
  vitte_argv = argv;
}

VitteSliceString cli_args(void) {
  VitteSliceString out = vitte_empty_slice_string();
  int i;
  if (vitte_argc <= 0 || vitte_argv == NULL) {
    return out;
  }
  out.data = (VitteString*)calloc((size_t)vitte_argc, sizeof(VitteString));
  if (out.data == NULL) {
    vitte_runtime_signal_panic(3);
    return out;
  }
  out.len = (size_t)vitte_argc;
  for (i = 0; i < vitte_argc; ++i) {
    out.data[i].data = vitte_argv[i];
    out.data[i].len = strlen(vitte_argv[i]);
  }
  return out;
}

int32_t vitte_host_runtime_available(void) {
  return 1;
}

VitteString vitte_host_read_file(VitteString path) {
  VitteString out = vitte_empty_string();
  char* path_c = vitte_to_c_string(path);
  FILE* file;
  long size;
  char* data;
  if (path_c == NULL) {
    return out;
  }
  file = fopen(path_c, "rb");
  free(path_c);
  if (file == NULL) {
    return out;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return out;
  }
  size = ftell(file);
  if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return out;
  }
  data = (char*)malloc((size_t)size + 1);
  if (data == NULL) {
    fclose(file);
    vitte_runtime_signal_panic(3);
    return out;
  }
  if ((size_t)size > 0 && fread(data, 1, (size_t)size, file) != (size_t)size) {
    free(data);
    fclose(file);
    return out;
  }
  fclose(file);
  data[size] = '\0';
  out.data = data;
  out.len = (size_t)size;
  return out;
}

int32_t vitte_host_write_file(VitteString path, VitteString content) {
  char* path_c = vitte_to_c_string(path);
  int32_t result;
  if (path_c == NULL) {
    return -1;
  }
  result = vitte_write_bytes(path_c, "wb", content);
  free(path_c);
  return result;
}

int32_t vitte_host_append_file(VitteString path, VitteString content) {
  char* path_c = vitte_to_c_string(path);
  int32_t result;
  if (path_c == NULL) {
    return -1;
  }
  result = vitte_write_bytes(path_c, "ab", content);
  free(path_c);
  return result;
}

int32_t vitte_host_file_exists(VitteString path) {
  char* path_c = vitte_to_c_string(path);
  int ok;
  if (path_c == NULL) {
    return 0;
  }
  ok = access(path_c, F_OK) == 0;
  free(path_c);
  return ok ? 1 : 0;
}

int32_t vitte_host_is_file(VitteString path) {
  char* path_c = vitte_to_c_string(path);
  struct stat info;
  int ok;
  if (path_c == NULL) {
    return 0;
  }
  ok = stat(path_c, &info) == 0 && S_ISREG(info.st_mode);
  free(path_c);
  return ok ? 1 : 0;
}

int32_t vitte_host_is_directory(VitteString path) {
  char* path_c = vitte_to_c_string(path);
  struct stat info;
  int ok;
  if (path_c == NULL) {
    return 0;
  }
  ok = stat(path_c, &info) == 0 && S_ISDIR(info.st_mode);
  free(path_c);
  return ok ? 1 : 0;
}

int32_t vitte_host_mkdir_all(VitteString path) {
  char* path_c = vitte_to_c_string(path);
  char* cursor;
  if (path_c == NULL) {
    return -1;
  }
  for (cursor = path_c + 1; *cursor != '\0'; ++cursor) {
    if (*cursor == '/') {
      *cursor = '\0';
      if (mkdir(path_c, 0777) != 0 && errno != EEXIST) {
        free(path_c);
        return -1;
      }
      *cursor = '/';
    }
  }
  if (mkdir(path_c, 0777) != 0 && errno != EEXIST) {
    free(path_c);
    return -1;
  }
  free(path_c);
  return 0;
}

int32_t vitte_host_delete_file(VitteString path) {
  char* path_c = vitte_to_c_string(path);
  int result;
  if (path_c == NULL) {
    return -1;
  }
  result = unlink(path_c);
  free(path_c);
  return result == 0 ? 0 : -1;
}

int32_t vitte_host_copy_file(VitteString src, VitteString dst) {
  char* src_c = vitte_to_c_string(src);
  char* dst_c = vitte_to_c_string(dst);
  char tmp_path[PATH_MAX];
  struct stat info;
  FILE* in;
  FILE* out;
  char buffer[8192];
  size_t read_count;
  int result = -1;
  if (src_c == NULL || dst_c == NULL) {
    free(src_c);
    free(dst_c);
    return -1;
  }
  if (strcmp(src_c, dst_c) == 0 || stat(src_c, &info) != 0) {
    free(src_c);
    free(dst_c);
    return -1;
  }
  snprintf(tmp_path, sizeof(tmp_path), "%s.tmp.%ld", dst_c, (long)getpid());
  in = fopen(src_c, "rb");
  out = fopen(tmp_path, "wb");
  if (in != NULL && out != NULL) {
    while ((read_count = fread(buffer, 1, sizeof(buffer), in)) > 0) {
      if (fwrite(buffer, 1, read_count, out) != read_count) {
        break;
      }
    }
    if (ferror(in) == 0 && ferror(out) == 0 && fclose(out) == 0) {
      out = NULL;
      chmod(tmp_path, info.st_mode & 0777);
      if (rename(tmp_path, dst_c) == 0) {
        result = 0;
      }
    }
  }
  if (in != NULL) {
    fclose(in);
  }
  if (out != NULL) {
    fclose(out);
  }
  if (result != 0) {
    unlink(tmp_path);
  }
  free(src_c);
  free(dst_c);
  return result;
}

int32_t vitte_host_move_file(VitteString src, VitteString dst) {
  char* src_c = vitte_to_c_string(src);
  char* dst_c = vitte_to_c_string(dst);
  int result;
  if (src_c == NULL || dst_c == NULL) {
    free(src_c);
    free(dst_c);
    return -1;
  }
  result = rename(src_c, dst_c);
  free(src_c);
  free(dst_c);
  return result == 0 ? 0 : -1;
}

int32_t vitte_host_delete_directory(VitteString path) {
  char* path_c = vitte_to_c_string(path);
  int result;
  if (path_c == NULL) {
    return -1;
  }
  result = rmdir(path_c);
  free(path_c);
  return result == 0 ? 0 : -1;
}

static int vitte_compare_string(const void* left, const void* right) {
  const VitteString* a = (const VitteString*)left;
  const VitteString* b = (const VitteString*)right;
  size_t min_len = a->len < b->len ? a->len : b->len;
  int cmp = memcmp(a->data, b->data, min_len);
  if (cmp != 0) {
    return cmp;
  }
  return (a->len > b->len) - (a->len < b->len);
}

VitteSliceString vitte_host_list_directory(VitteString path) {
  VitteSliceString out = vitte_empty_slice_string();
  char* path_c = vitte_to_c_string(path);
  DIR* dir;
  struct dirent* entry;
  if (path_c == NULL) {
    return out;
  }
  dir = opendir(path_c);
  free(path_c);
  if (dir == NULL) {
    return out;
  }
  while ((entry = readdir(dir)) != NULL) {
    VitteString item;
    char* data;
    size_t len;
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    len = strlen(entry->d_name);
    data = (char*)malloc(len + 1);
    if (data == NULL) {
      vitte_runtime_signal_panic(3);
      continue;
    }
    memcpy(data, entry->d_name, len + 1);
    item.data = data;
    item.len = len;
    out = vitte_slice_push_string(out, item);
  }
  closedir(dir);
  qsort(out.data, out.len, sizeof(VitteString), vitte_compare_string);
  return out;
}

int32_t vitte_host_system(VitteString command) {
  char* command_c = vitte_to_c_string(command);
  int status;
  if (command_c == NULL) {
    return -1;
  }
  status = system(command_c);
  free(command_c);
  if (status == -1) {
    return -1;
  }
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  return -1;
}

static char* vitte_sidecar_path(const char* object_c, const char* suffix) {
  const char* slash = strrchr(object_c, '/');
  size_t dir_len = slash == NULL ? 1 : (size_t)(slash - object_c + 1);
  const char* name = slash == NULL ? "" : object_c;
  char* out = (char*)malloc(dir_len + strlen(".vitte-native-input") + strlen(suffix) + 1);
  if (out == NULL) {
    return NULL;
  }
  if (slash == NULL) {
    strcpy(out, ".");
    strcat(out, "/.vitte-native-input");
  } else {
    memcpy(out, object_c, dir_len);
    out[dir_len] = '\0';
    strcat(out, ".vitte-native-input");
  }
  (void)name;
  strcat(out, suffix);
  return out;
}

static char* vitte_object_sidecar_path(const char* object_c, const char* suffix) {
  char* out = (char*)malloc(strlen(object_c) + strlen(suffix) + 1);
  if (out == NULL) {
    return NULL;
  }
  strcpy(out, object_c);
  strcat(out, suffix);
  return out;
}

static int vitte_run_argv(char* const argv[]) {
  pid_t child = fork();
  int status;
  if (child < 0) {
    return -1;
  }
  if (child == 0) {
    execvp(argv[0], argv);
    _exit(127);
  }
  if (waitpid(child, &status, 0) < 0) {
    return -1;
  }
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    return 0;
  }
  return -1;
}

static int vitte_append_target_argv(char** argv, int arg_index, const char* target_c, const char* sysroot_c) {
  if (target_c != NULL && target_c[0] != '\0') {
    argv[arg_index++] = "-target";
    argv[arg_index++] = (char*)target_c;
  }
  if (sysroot_c != NULL && sysroot_c[0] != '\0') {
    argv[arg_index++] = "--sysroot";
    argv[arg_index++] = (char*)sysroot_c;
  }
  return arg_index;
}

int32_t vitte_host_emit_llvm_object(VitteString ir_text, VitteString compiler_path, VitteString target_triple, VitteString sysroot_path, VitteString object_path) {
  char* compiler_c = vitte_to_c_string(compiler_path);
  char* target_c = vitte_to_c_string(target_triple);
  char* sysroot_c = vitte_to_c_string(sysroot_path);
  char* object_c = vitte_to_c_string(object_path);
  char* llvm_path;
  char* argv[16];
  int arg_index = 0;
  int result = -1;
  if (compiler_c == NULL || target_c == NULL || sysroot_c == NULL || object_c == NULL || target_c[0] == '\0') {
    goto done;
  }
  llvm_path = vitte_object_sidecar_path(object_c, ".ll");
  if (llvm_path == NULL) {
    goto done;
  }
  if (vitte_write_bytes(llvm_path, "wb", ir_text) < 0) {
    free(llvm_path);
    goto done;
  }
  remove(object_c);
  setenv("SOURCE_DATE_EPOCH", "0", 1);
  argv[arg_index++] = compiler_c;
  arg_index = vitte_append_target_argv(argv, arg_index, target_c, sysroot_c);
  argv[arg_index++] = "-Wno-override-module";
  argv[arg_index++] = "-c";
  argv[arg_index++] = llvm_path;
  argv[arg_index++] = "-o";
  argv[arg_index++] = object_c;
  argv[arg_index] = NULL;
  if (vitte_run_argv(argv) == 0) {
    result = 0;
  }
  remove(llvm_path);
  free(llvm_path);
done:
  free(compiler_c);
  free(target_c);
  free(sysroot_c);
  free(object_c);
  return result;
}

int32_t vitte_host_emit_assembly_object(VitteString assembly_text, VitteString assembler_path, VitteString target_triple, VitteString sysroot_path, VitteString object_path, int32_t debug_enabled) {
  char* assembler_c = vitte_to_c_string(assembler_path);
  char* target_c = vitte_to_c_string(target_triple);
  char* sysroot_c = vitte_to_c_string(sysroot_path);
  char* object_c = vitte_to_c_string(object_path);
  char* assembly_path;
  char* argv[20];
  int arg_index = 0;
  int result = -1;
  if (assembler_c == NULL || target_c == NULL || sysroot_c == NULL || object_c == NULL || target_c[0] == '\0') {
    goto done;
  }
  if (debug_enabled) {
    assembly_path = vitte_sidecar_path(object_c, ".s");
  } else {
    assembly_path = vitte_object_sidecar_path(object_c, ".s");
  }
  if (assembly_path == NULL) {
    goto done;
  }
  if (vitte_write_bytes(assembly_path, "wb", assembly_text) < 0) {
    free(assembly_path);
    goto done;
  }
  remove(object_c);
  setenv("SOURCE_DATE_EPOCH", "0", 1);
  argv[arg_index++] = assembler_c;
  arg_index = vitte_append_target_argv(argv, arg_index, target_c, sysroot_c);
  if (debug_enabled) {
    argv[arg_index++] = "-g";
  }
  argv[arg_index++] = "-x";
  argv[arg_index++] = "assembler";
  argv[arg_index++] = "-c";
  argv[arg_index++] = assembly_path;
  argv[arg_index++] = "-o";
  argv[arg_index++] = object_c;
  argv[arg_index] = NULL;
  if (vitte_run_argv(argv) == 0) {
    result = 0;
  }
  remove(assembly_path);
  free(assembly_path);
done:
  free(assembler_c);
  free(target_c);
  free(sysroot_c);
  free(object_c);
  return result;
}

typedef struct {
  uint32_t name_offset;
  uint32_t type;
  uint64_t offset;
  uint64_t size;
  uint32_t link;
  uint64_t entry_size;
  const char* name;
} VitteElfSection;

static int vitte_expected_machine(const char* target_c) {
  if (strcmp(target_c, "x86_64-unknown-linux-gnu") == 0) {
    return 62;
  }
  if (strcmp(target_c, "aarch64-unknown-linux-gnu") == 0) {
    return 183;
  }
  return -1;
}

static const char* vitte_elf_string(const unsigned char* data, size_t size, uint64_t table_offset, uint64_t offset) {
  if (table_offset >= size || table_offset + offset >= size) {
    return "";
  }
  return (const char*)(data + table_offset + offset);
}

int32_t vitte_host_verify_native_object(VitteString object_path, VitteString target_triple, VitteString expected_symbol, int32_t require_relocations, int32_t require_debug) {
  char* object_c = vitte_to_c_string(object_path);
  char* target_c = vitte_to_c_string(target_triple);
  char* expected_c = vitte_to_c_string(expected_symbol);
  FILE* file = NULL;
  unsigned char* data = NULL;
  long raw_size;
  size_t size;
  uint64_t section_offset;
  uint16_t section_entry_size;
  uint16_t section_count;
  uint16_t section_name_index;
  VitteElfSection* sections = NULL;
  uint64_t section_name_offset;
  uint64_t section_name_size;
  int expected_machine;
  int saw_text = 0;
  int saw_symbol_table = 0;
  int saw_expected_symbol = 0;
  int saw_relocation = 0;
  int saw_relocation_to_external = 0;
  int saw_eh_frame = 0;
  int saw_debug_info = 0;
  int saw_debug_line = 0;
  int result = 1;
  uint16_t object_machine;
  uint16_t object_type;
  uint16_t i;
  if (object_c == NULL || target_c == NULL || expected_c == NULL) {
    goto done;
  }
  expected_machine = vitte_expected_machine(target_c);
  if (expected_machine < 0) {
    result = 6;
    goto done;
  }
  file = fopen(object_c, "rb");
  if (file == NULL) {
    goto done;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    goto done;
  }
  raw_size = ftell(file);
  if (raw_size < 64 || fseek(file, 0, SEEK_SET) != 0) {
    goto done;
  }
  size = (size_t)raw_size;
  data = (unsigned char*)malloc(size);
  if (data == NULL || fread(data, 1, size, file) != size) {
    goto done;
  }
  if (memcmp(data, "\177ELF", 4) != 0 || data[4] != 2 || data[5] != 1) {
    goto done;
  }
  object_type = vitte_read_u16_le(data + 16);
  if (vitte_read_u16_le(data + 16) != 1) {
    goto done;
  }
  object_machine = vitte_read_u16_le(data + 18);
  if (vitte_read_u16_le(data + 18) != expected_machine) {
    result = 6;
    goto done;
  }
  (void)object_type;
  (void)object_machine;
  section_offset = vitte_read_u64_le(data + 40);
  section_entry_size = vitte_read_u16_le(data + 58);
  section_count = vitte_read_u16_le(data + 60);
  section_name_index = vitte_read_u16_le(data + 62);
  if (section_entry_size < 64 || section_name_index >= section_count || section_offset + ((uint64_t)section_count * section_entry_size) > size) {
    goto done;
  }
  sections = (VitteElfSection*)calloc(section_count, sizeof(VitteElfSection));
  if (sections == NULL) {
    goto done;
  }
  for (i = 0; i < section_count; ++i) {
    uint64_t base = section_offset + ((uint64_t)i * section_entry_size);
    sections[i].name_offset = vitte_read_u32_le(data + base);
    sections[i].type = vitte_read_u32_le(data + base + 4);
    sections[i].offset = vitte_read_u64_le(data + base + 24);
    sections[i].size = vitte_read_u64_le(data + base + 32);
    sections[i].link = vitte_read_u32_le(data + base + 40);
    sections[i].entry_size = vitte_read_u64_le(data + base + 56);
  }
  section_name_offset = sections[section_name_index].offset;
  section_name_size = sections[section_name_index].size;
  if (section_name_offset + section_name_size > size) {
    goto done;
  }
  for (i = 0; i < section_count; ++i) {
    const char* name;
    sections[i].name = vitte_elf_string(data, size, section_name_offset, sections[i].name_offset);
    name = sections[i].name;
    if (strcmp(name, ".text") == 0) {
      saw_text = 1;
    }
    if (strcmp(name, ".eh_frame") == 0) {
      saw_eh_frame = 1;
    }
    if (strcmp(name, ".debug_info") == 0) {
      saw_debug_info = 1;
    }
    if (strcmp(name, ".debug_line") == 0) {
      saw_debug_line = 1;
    }
  }
  for (i = 0; i < section_count; ++i) {
    uint64_t count;
    uint64_t j;
    uint64_t strings_offset;
    if (sections[i].type != 2 || sections[i].entry_size < 24 || sections[i].link >= section_count) {
      continue;
    }
    saw_symbol_table = 1;
    strings_offset = sections[sections[i].link].offset;
    count = sections[i].size / sections[i].entry_size;
    for (j = 0; j < count; ++j) {
      uint64_t base = sections[i].offset + (j * sections[i].entry_size);
      uint32_t name_offset;
      uint16_t section_index;
      const char* name;
      if (base + 24 > size) {
        continue;
      }
      name_offset = vitte_read_u32_le(data + base);
      section_index = vitte_read_u16_le(data + base + 6);
      name = vitte_elf_string(data, size, strings_offset, name_offset);
      if (strcmp(name, expected_c) == 0 && section_index != 0) {
        saw_expected_symbol = 1;
      }
    }
  }
  for (i = 0; i < section_count; ++i) {
    uint64_t relocation_count;
    if (!((sections[i].type == 4 || sections[i].type == 9) && sections[i].entry_size >= 16)) {
      continue;
    }
    uint64_t size = sections[i].size;
    uint64_t entry_size = sections[i].entry_size;
    relocation_count = 0;
    relocation_count += size / entry_size;
    if (relocation_count > 0) {
      saw_relocation = 1;
      saw_relocation_to_external = 1;
    }
  }
  if (!saw_text) {
    result = 7;
    goto done;
  }
  if (!saw_symbol_table) {
    result = 8;
    goto done;
  }
  if (!saw_expected_symbol) {
    result = 11;
    goto done;
  }
  if (require_relocations && (!saw_relocation || !saw_relocation_to_external)) {
    result = 12;
    goto done;
  }
  if (!saw_eh_frame) {
    result = 13;
    goto done;
  }
  if (require_debug && (!saw_debug_info || !saw_debug_line)) {
    result = 14;
    goto done;
  }
  result = 0;
done:
  if (file != NULL) {
    fclose(file);
  }
  free(data);
  free(sections);
  free(object_c);
  free(target_c);
  free(expected_c);
  return result;
}

int32_t vitte_host_link_executable(VitteString linker_path, VitteString target_triple, VitteString sysroot_path, VitteString object_path, VitteString runtime_source_path, VitteString runtime_include_path, VitteString executable_path) {
  char* linker_c = vitte_to_c_string(linker_path);
  char* target_c = vitte_to_c_string(target_triple);
  char* sysroot_c = vitte_to_c_string(sysroot_path);
  char* object_c = vitte_to_c_string(object_path);
  char* runtime_c = vitte_to_c_string(runtime_source_path);
  char* include_c = vitte_to_c_string(runtime_include_path);
  char* executable_c = vitte_to_c_string(executable_path);
  char* argv[20];
  int arg_index = 0;
  int result = -1;
  if (linker_c == NULL || target_c == NULL || sysroot_c == NULL || object_c == NULL || runtime_c == NULL || include_c == NULL || executable_c == NULL || target_c[0] == '\0') {
    goto done;
  }
  remove(executable_c);
  setenv("SOURCE_DATE_EPOCH", "0", 1);
  argv[arg_index++] = linker_c;
  arg_index = vitte_append_target_argv(argv, arg_index, target_c, sysroot_c);
  argv[arg_index++] = object_c;
  argv[arg_index++] = runtime_c;
  argv[arg_index++] = "-I";
  argv[arg_index++] = include_c;
  argv[arg_index++] = "-o";
  argv[arg_index++] = executable_c;
  argv[arg_index] = NULL;
  if (vitte_run_argv(argv) == 0) {
    result = 0;
  }
done:
  free(linker_c);
  free(target_c);
  free(sysroot_c);
  free(object_c);
  free(runtime_c);
  free(include_c);
  free(executable_c);
  return result;
}

int32_t vitte_host_run_executable(VitteString executable_path) {
  char* executable_c = vitte_to_c_string(executable_path);
  char* argv[2];
  int result;
  if (executable_c == NULL) {
    return -1;
  }
  argv[0] = executable_c;
  argv[1] = NULL;
  result = vitte_run_argv(argv);
  free(executable_c);
  return result;
}
