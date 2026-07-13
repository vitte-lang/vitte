#define _POSIX_C_SOURCE 200809L

#include "vitte_runtime.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static _Thread_local int32_t g_boundary_depth = 0;
static _Thread_local int32_t g_panic_triggered = 0;
static _Thread_local int32_t g_panic_code = 0;

static const char *g_abi_version = "1.0.0";
static const char **g_argv = NULL;
static int g_argc = 0;

static void vitte_note_panic(int32_t code) {
  g_panic_triggered = 1;
  g_panic_code = code;
}

const char *vitte_c_abi_version(void) {
  return g_abi_version;
}

int32_t vitte_runtime_panic_boundary_begin(void) {
  g_boundary_depth += 1;
  if (g_boundary_depth == 1) {
    g_panic_triggered = 0;
    g_panic_code = 0;
  }
  return g_boundary_depth;
}

int32_t vitte_runtime_panic_boundary_end(void) {
  if (g_boundary_depth > 0) {
    g_boundary_depth -= 1;
  }
  return g_boundary_depth;
}

int32_t vitte_runtime_panic_boundary_triggered(void) {
  return g_panic_triggered;
}

int32_t vitte_runtime_panic_boundary_code(void) {
  return g_panic_code;
}

int32_t vitte_runtime_panic_boundary_reset(void) {
  g_boundary_depth = 0;
  g_panic_triggered = 0;
  g_panic_code = 0;
  return 0;
}

static char *vitte_string_to_c(VitteString input) {
  char *out = (char *)malloc(input.len + 1);
  if (out == NULL) {
    vitte_note_panic(3);
    return NULL;
  }
  if (input.data != NULL && input.len > 0) {
    memcpy(out, input.data, input.len);
  }
  out[input.len] = '\0';
  return out;
}

static VitteString vitte_string_from_cstr(const char *input) {
  VitteString empty = {NULL, 0};
  size_t len = 0;
  char *buf = NULL;
  if (input == NULL) {
    return empty;
  }
  len = strlen(input);
  buf = (char *)malloc(len + 1);
  if (buf == NULL) {
    vitte_note_panic(3);
    return empty;
  }
  memcpy(buf, input, len + 1);
  empty.data = buf;
  empty.len = len;
  return empty;
}

int32_t vitte_host_runtime_available(void) {
  return 1;
}

VitteString vitte_host_read_file(VitteString path) {
  VitteString empty = {NULL, 0};
  char *native_path = vitte_string_to_c(path);
  if (native_path == NULL) {
    return empty;
  }
  FILE *file = fopen(native_path, "rb");
  free(native_path);
  if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
    if (file != NULL) {
      fclose(file);
    }
    return empty;
  }
  long size = ftell(file);
  if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return empty;
  }
  char *buf = (char *)malloc((size_t)size + 1);
  if (buf == NULL) {
    fclose(file);
    vitte_note_panic(3);
    return empty;
  }
  size_t count = fread(buf, 1, (size_t)size, file);
  fclose(file);
  buf[count] = '\0';
  VitteString out = {buf, count};
  return out;
}

static int32_t vitte_host_write_mode(VitteString path, VitteString content, const char *mode) {
  char *native_path = vitte_string_to_c(path);
  if (native_path == NULL) {
    return -1;
  }
  FILE *file = fopen(native_path, mode);
  free(native_path);
  if (file == NULL) {
    return -1;
  }
  size_t wrote = fwrite(content.data, 1, content.len, file);
  int close_result = fclose(file);
  if (wrote != content.len || close_result != 0) {
    return -1;
  }
  return (int32_t)wrote;
}

int32_t vitte_host_write_file(VitteString path, VitteString content) {
  return vitte_host_write_mode(path, content, "wb");
}

int32_t vitte_host_append_file(VitteString path, VitteString content) {
  return vitte_host_write_mode(path, content, "ab");
}

static int32_t vitte_host_stat_mode(VitteString path, int want_file, int want_directory) {
  char *native_path = vitte_string_to_c(path);
  if (native_path == NULL) {
    return 0;
  }
  struct stat info;
  int ok = stat(native_path, &info) == 0;
  free(native_path);
  if (!ok) {
    return 0;
  }
  if (want_file) {
    return S_ISREG(info.st_mode) ? 1 : 0;
  }
  if (want_directory) {
    return S_ISDIR(info.st_mode) ? 1 : 0;
  }
  return 1;
}

int32_t vitte_host_file_exists(VitteString path) {
  return vitte_host_stat_mode(path, 0, 0);
}

int32_t vitte_host_is_file(VitteString path) {
  return vitte_host_stat_mode(path, 1, 0);
}

int32_t vitte_host_is_directory(VitteString path) {
  return vitte_host_stat_mode(path, 0, 1);
}

int32_t vitte_host_mkdir_all(VitteString path) {
  char *native_path = vitte_string_to_c(path);
  if (native_path == NULL) {
    return -1;
  }
  for (char *cursor = native_path + 1; *cursor != '\0'; cursor += 1) {
    if (*cursor == '/') {
      *cursor = '\0';
      if (mkdir(native_path, 0777) != 0) {
        struct stat parent;
        if (stat(native_path, &parent) != 0 || !S_ISDIR(parent.st_mode)) {
          free(native_path);
          return -1;
        }
      }
      *cursor = '/';
    }
  }
  if (mkdir(native_path, 0777) != 0) {
    struct stat final_dir;
    if (stat(native_path, &final_dir) != 0 || !S_ISDIR(final_dir.st_mode)) {
      free(native_path);
      return -1;
    }
  }
  free(native_path);
  return 0;
}

int32_t vitte_host_delete_file(VitteString path) {
  char *native_path = vitte_string_to_c(path);
  int32_t result = -1;
  if (native_path == NULL) {
    return -1;
  }
  result = remove(native_path) == 0 ? 0 : -1;
  free(native_path);
  return result;
}

int32_t vitte_host_copy_file(VitteString src, VitteString dst) {
  char *src_path = vitte_string_to_c(src);
  char *dst_path = vitte_string_to_c(dst);
  FILE *src_file = NULL;
  FILE *dst_file = NULL;
  char buffer[4096];
  size_t read_count = 0;
  int32_t result = -1;
  if (src_path == NULL || dst_path == NULL) {
    free(src_path);
    free(dst_path);
    return -1;
  }
  src_file = fopen(src_path, "rb");
  dst_file = fopen(dst_path, "wb");
  if (src_file == NULL || dst_file == NULL) {
    goto cleanup_copy;
  }
  while ((read_count = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
    if (fwrite(buffer, 1, read_count, dst_file) != read_count) {
      goto cleanup_copy;
    }
  }
  if (ferror(src_file) != 0 || fclose(dst_file) != 0) {
    dst_file = NULL;
    goto cleanup_copy;
  }
  dst_file = NULL;
  result = 0;

cleanup_copy:
  if (src_file != NULL) {
    fclose(src_file);
  }
  if (dst_file != NULL) {
    fclose(dst_file);
  }
  free(src_path);
  free(dst_path);
  return result;
}

int32_t vitte_host_move_file(VitteString src, VitteString dst) {
  char *src_path = vitte_string_to_c(src);
  char *dst_path = vitte_string_to_c(dst);
  int32_t result = -1;
  if (src_path == NULL || dst_path == NULL) {
    free(src_path);
    free(dst_path);
    return -1;
  }
  result = rename(src_path, dst_path) == 0 ? 0 : -1;
  free(src_path);
  free(dst_path);
  return result;
}

int32_t vitte_host_delete_directory(VitteString path) {
  char *native_path = vitte_string_to_c(path);
  int32_t result = -1;
  if (native_path == NULL) {
    return -1;
  }
  result = rmdir(native_path) == 0 ? 0 : -1;
  free(native_path);
  return result;
}

VitteSliceString vitte_host_list_directory(VitteString path) {
  VitteSliceString out = {0};
  char *native_path = vitte_string_to_c(path);
  DIR *dir = NULL;
  struct dirent *entry = NULL;
  if (native_path == NULL) {
    return out;
  }
  dir = opendir(native_path);
  free(native_path);
  if (dir == NULL) {
    return out;
  }
  while ((entry = readdir(dir)) != NULL) {
    VitteString item;
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    item = vitte_string_from_cstr(entry->d_name);
    if (item.data == NULL && item.len == 0) {
      closedir(dir);
      return out;
    }
    out = vitte_slice_push_string(out, item);
  }
  closedir(dir);
  return out;
}

int32_t vitte_host_system(VitteString command) {
  char *native_command = vitte_string_to_c(command);
  if (native_command == NULL) {
    return -1;
  }
  int result = system(native_command);
  free(native_command);
  if (result < 0) {
    return -1;
  }
  if (WIFEXITED(result)) {
    return (int32_t)WEXITSTATUS(result);
  }
  return -1;
}

static int32_t vitte_run_argv(char *const argv[]) {
  pid_t pid = fork();
  int status = 0;
  if (pid < 0) {
    return -1;
  }
  if (pid == 0) {
    setenv("SOURCE_DATE_EPOCH", "0", 1);
    setenv("ZERO_AR_DATE", "1", 1);
    execvp(argv[0], argv);
    _exit(127);
  }
  if (waitpid(pid, &status, 0) < 0) {
    return -1;
  }
  if (WIFEXITED(status)) {
    return (int32_t)WEXITSTATUS(status);
  }
  return -1;
}

int32_t vitte_host_emit_llvm_object(VitteString ir_text, VitteString object_path) {
  char *object_c = vitte_string_to_c(object_path);
  char *ir_path = NULL;
  FILE *stream = NULL;
  char *argv[7];
  int32_t result = -1;
  if (object_c == NULL) {
    return -1;
  }
  ir_path = (char *)malloc(strlen(object_c) + sizeof(".ll"));
  if (ir_path == NULL) {
    free(object_c);
    return -1;
  }
  strcpy(ir_path, object_c);
  strcat(ir_path, ".ll");
  remove(object_c);
  stream = fopen(ir_path, "wb");
  if (stream != NULL &&
      (ir_text.len == 0 || fwrite(ir_text.data, 1, ir_text.len, stream) == ir_text.len) &&
      fclose(stream) == 0) {
    stream = NULL;
    argv[0] = "clang";
    argv[1] = "-Wno-override-module";
    argv[2] = "-c";
    argv[3] = ir_path;
    argv[4] = "-o";
    argv[5] = object_c;
    argv[6] = NULL;
    result = vitte_run_argv(argv);
  }
  if (stream != NULL) {
    fclose(stream);
  }
  free(ir_path);
  free(object_c);
  return result;
}

int32_t vitte_host_emit_assembly_object(VitteString assembly_text, VitteString target_triple, VitteString object_path, int32_t debug_enabled) {
  char *object_c = vitte_string_to_c(object_path);
  char *target_c = vitte_string_to_c(target_triple);
  char *assembly_path = NULL;
  FILE *stream = NULL;
  char *argv[12];
  int arg_index = 0;
  int32_t result = -1;
  if (object_c == NULL || target_c == NULL || target_c[0] == '\0') {
    free(object_c);
    free(target_c);
    return -1;
  }
  assembly_path = (char *)malloc(strlen(object_c) + sizeof(".s"));
  if (assembly_path == NULL) {
    free(object_c);
    free(target_c);
    return -1;
  }
  strcpy(assembly_path, object_c);
  strcat(assembly_path, ".s");
  remove(object_c);
  stream = fopen(assembly_path, "wb");
  if (stream != NULL &&
      (assembly_text.len == 0 || fwrite(assembly_text.data, 1, assembly_text.len, stream) == assembly_text.len) &&
      fclose(stream) == 0) {
    stream = NULL;
    argv[arg_index++] = "clang";
    argv[arg_index++] = "-target";
    argv[arg_index++] = target_c;
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
    result = vitte_run_argv(argv);
  }
  if (stream != NULL) {
    fclose(stream);
  }
  remove(assembly_path);
  free(assembly_path);
  free(target_c);
  free(object_c);
  return result;
}

static uint16_t vitte_read_u16_le(const unsigned char *data) {
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t vitte_read_u32_le(const unsigned char *data) {
  return (uint32_t)data[0] |
         ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

static uint64_t vitte_read_u64_le(const unsigned char *data) {
  return (uint64_t)vitte_read_u32_le(data) |
         ((uint64_t)vitte_read_u32_le(data + 4) << 32);
}

static int vitte_binary_range_valid(size_t length, uint64_t offset, uint64_t size) {
  return offset <= (uint64_t)length && size <= (uint64_t)length - offset;
}

static const unsigned char *vitte_elf64_section(const unsigned char *data, size_t length, uint64_t section_offset, uint16_t section_size, uint16_t section_count, uint16_t index) {
  uint64_t offset;
  if (index >= section_count || section_size < 64) {
    return NULL;
  }
  offset = section_offset + (uint64_t)section_size * index;
  if (!vitte_binary_range_valid(length, offset, section_size)) {
    return NULL;
  }
  return data + offset;
}

static const char *vitte_elf_string(const unsigned char *table, uint64_t table_size, uint32_t offset) {
  const unsigned char *end;
  if ((uint64_t)offset >= table_size) {
    return NULL;
  }
  end = (const unsigned char *)memchr(table + offset, '\0', (size_t)(table_size - offset));
  if (end == NULL) {
    return NULL;
  }
  return (const char *)(table + offset);
}

int32_t vitte_host_verify_native_object(VitteString object_path, VitteString target_triple, VitteString expected_symbol, int32_t require_relocations, int32_t require_debug) {
  VitteString object_data = vitte_host_read_file(object_path);
  char *target_c = vitte_string_to_c(target_triple);
  char *expected_c = vitte_string_to_c(expected_symbol);
  const unsigned char *data = (const unsigned char *)object_data.data;
  size_t length = object_data.len;
  uint16_t expected_machine = 0;
  uint64_t section_offset;
  uint16_t section_size;
  uint16_t section_count;
  uint16_t string_section_index;
  const unsigned char *section_names_header;
  const unsigned char *section_names;
  uint64_t section_names_size;
  const unsigned char *symbol_header = NULL;
  const unsigned char *symbol_strings_header = NULL;
  int saw_text = 0;
  int saw_symbol_table = 0;
  int saw_string_table = 0;
  int saw_unwind = 0;
  int saw_debug_info = 0;
  int saw_debug_line = 0;
  int saw_expected_symbol = 0;
  uint64_t relocation_count = 0;
  int32_t result = 0;
  uint16_t i;

  if (data == NULL || target_c == NULL || expected_c == NULL || expected_c[0] == '\0') {
    result = 1;
    goto cleanup;
  }
  if (strstr(target_c, "x86_64") != NULL) {
    expected_machine = 62;
  } else if (strstr(target_c, "aarch64") != NULL || strstr(target_c, "arm64") != NULL) {
    expected_machine = 183;
  } else {
    result = 2;
    goto cleanup;
  }
  if (length < 64 || data[0] != 0x7f || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') {
    result = 3;
    goto cleanup;
  }
  if (data[4] != 2 || data[5] != 1) {
    result = 4;
    goto cleanup;
  }
  if (vitte_read_u16_le(data + 16) != 1) {
    result = 5;
    goto cleanup;
  }
  if (vitte_read_u16_le(data + 18) != expected_machine) {
    result = 6;
    goto cleanup;
  }
  section_offset = vitte_read_u64_le(data + 40);
  section_size = vitte_read_u16_le(data + 58);
  section_count = vitte_read_u16_le(data + 60);
  string_section_index = vitte_read_u16_le(data + 62);
  if (section_count == 0 || string_section_index >= section_count ||
      !vitte_binary_range_valid(length, section_offset, (uint64_t)section_size * section_count)) {
    result = 7;
    goto cleanup;
  }
  section_names_header = vitte_elf64_section(data, length, section_offset, section_size, section_count, string_section_index);
  if (section_names_header == NULL) {
    result = 7;
    goto cleanup;
  }
  section_names_size = vitte_read_u64_le(section_names_header + 32);
  if (!vitte_binary_range_valid(length, vitte_read_u64_le(section_names_header + 24), section_names_size)) {
    result = 7;
    goto cleanup;
  }
  section_names = data + vitte_read_u64_le(section_names_header + 24);

  for (i = 0; i < section_count; ++i) {
    const unsigned char *section = vitte_elf64_section(data, length, section_offset, section_size, section_count, i);
    uint32_t type;
    uint64_t size;
    uint64_t entry_size;
    const char *name;
    if (section == NULL) {
      result = 7;
      goto cleanup;
    }
    type = vitte_read_u32_le(section + 4);
    size = vitte_read_u64_le(section + 32);
    entry_size = vitte_read_u64_le(section + 56);
    name = vitte_elf_string(section_names, section_names_size, vitte_read_u32_le(section));
    if (name != NULL && strcmp(name, ".text") == 0 && size > 0) {
      saw_text = 1;
    }
    if (type == 2) {
      saw_symbol_table = 1;
      symbol_header = section;
    }
    if (name != NULL && strcmp(name, ".strtab") == 0 && type == 3) {
      saw_string_table = 1;
    }
    if (name != NULL && strcmp(name, ".eh_frame") == 0 && size > 0) {
      saw_unwind = 1;
    }
    if (name != NULL && strcmp(name, ".debug_info") == 0 && size > 0) {
      saw_debug_info = 1;
    }
    if (name != NULL && strcmp(name, ".debug_line") == 0 && size > 0) {
      saw_debug_line = 1;
    }
    if ((type == 4 || type == 9) && entry_size > 0 && name != NULL &&
        (strcmp(name, ".rela.text") == 0 || strcmp(name, ".rel.text") == 0)) {
      relocation_count += size / entry_size;
    }
  }
  if (!saw_text) {
    result = 8;
    goto cleanup;
  }
  if (!saw_symbol_table || symbol_header == NULL) {
    result = 9;
    goto cleanup;
  }
  if (!saw_string_table) {
    result = 10;
    goto cleanup;
  }
  if (!saw_unwind) {
    result = 13;
    goto cleanup;
  }
  if (require_debug && (!saw_debug_info || !saw_debug_line)) {
    result = 14;
    goto cleanup;
  }
  {
    uint32_t link = vitte_read_u32_le(symbol_header + 40);
    uint64_t symbol_offset = vitte_read_u64_le(symbol_header + 24);
    uint64_t symbol_size = vitte_read_u64_le(symbol_header + 32);
    uint64_t symbol_entry_size = vitte_read_u64_le(symbol_header + 56);
    uint64_t symbol_count;
    uint64_t symbol_index;
    const unsigned char *symbol_strings;
    uint64_t symbol_strings_size;
    if (link >= section_count || symbol_entry_size < 24 ||
        !vitte_binary_range_valid(length, symbol_offset, symbol_size)) {
      result = 9;
      goto cleanup;
    }
    symbol_strings_header = vitte_elf64_section(data, length, section_offset, section_size, section_count, (uint16_t)link);
    if (symbol_strings_header == NULL) {
      result = 10;
      goto cleanup;
    }
    symbol_strings_size = vitte_read_u64_le(symbol_strings_header + 32);
    if (!vitte_binary_range_valid(length, vitte_read_u64_le(symbol_strings_header + 24), symbol_strings_size)) {
      result = 10;
      goto cleanup;
    }
    symbol_strings = data + vitte_read_u64_le(symbol_strings_header + 24);
    symbol_count = symbol_size / symbol_entry_size;
    for (symbol_index = 0; symbol_index < symbol_count; ++symbol_index) {
      const unsigned char *symbol = data + symbol_offset + symbol_index * symbol_entry_size;
      const char *name = vitte_elf_string(symbol_strings, symbol_strings_size, vitte_read_u32_le(symbol));
      if (name != NULL && strcmp(name, expected_c) == 0 && vitte_read_u16_le(symbol + 6) != 0) {
        saw_expected_symbol = 1;
        break;
      }
    }
  }
  if (!saw_expected_symbol) {
    result = 11;
    goto cleanup;
  }
  if (require_relocations && relocation_count == 0) {
    result = 12;
    goto cleanup;
  }

cleanup:
  free((void *)object_data.data);
  free(target_c);
  free(expected_c);
  return result;
}

int32_t vitte_host_link_executable(VitteString object_path, VitteString executable_path) {
  char *object_c = vitte_string_to_c(object_path);
  char *executable_c = vitte_string_to_c(executable_path);
  char *argv[8];
  int32_t result = -1;
  if (object_c == NULL || executable_c == NULL) {
    free(object_c);
    free(executable_c);
    return -1;
  }
  argv[0] = "cc";
  argv[1] = object_c;
  argv[2] = "src/vitte/compiler/backends/runtime_c/vitte_runtime.c";
  argv[3] = "-I";
  argv[4] = "src/vitte/compiler/backends/runtime_c";
  argv[5] = "-o";
  argv[6] = executable_c;
  argv[7] = NULL;
  result = vitte_run_argv(argv);
  free(object_c);
  free(executable_c);
  return result;
}

int32_t vitte_host_run_executable(VitteString executable_path) {
  char *executable_c = vitte_string_to_c(executable_path);
  char *argv[2];
  int32_t result = -1;
  if (executable_c == NULL) {
    return -1;
  }
  argv[0] = executable_c;
  argv[1] = NULL;
  result = vitte_run_argv(argv);
  free(executable_c);
  return result;
}

void vitte_builtin_trap(VitteString msg) {
  vitte_note_panic(2);
  if (msg.data != NULL && msg.len > 0) {
    fwrite(msg.data, 1, msg.len, stderr);
    fputc('\n', stderr);
  } else {
    fputs("vitte trap\n", stderr);
  }
  if (g_boundary_depth <= 0) {
    abort();
  }
}

VitteSliceI32 vitte_empty_slice_i32(void) {
  VitteSliceI32 out = {0};
  return out;
}

VitteSliceString vitte_empty_slice_string(void) {
  VitteSliceString out = {0};
  return out;
}

VitteSliceI32 vitte_slice_push_i32(VitteSliceI32 base, int32_t value) {
  size_t next_len = base.len + 1;
  int32_t *next = (int32_t *)realloc(base.data, next_len * sizeof(int32_t));
  if (next == NULL) {
    vitte_note_panic(3);
    return base;
  }
  next[base.len] = value;
  VitteSliceI32 out = {next, next_len};
  return out;
}

VitteSliceString vitte_slice_push_string(VitteSliceString base, VitteString value) {
  size_t next_len = base.len + 1;
  VitteString *next = (VitteString *)realloc(base.data, next_len * sizeof(VitteString));
  if (next == NULL) {
    vitte_note_panic(3);
    return base;
  }
  next[base.len] = value;
  VitteSliceString out = {next, next_len};
  return out;
}

VitteString vitte_string_concat(VitteString a, VitteString b) {
  size_t total = a.len + b.len;
  char *buf = (char *)malloc(total + 1);
  if (buf == NULL) {
    vitte_note_panic(3);
    VitteString empty = {NULL, 0};
    return empty;
  }
  if (a.data != NULL && a.len > 0) {
    memcpy(buf, a.data, a.len);
  }
  if (b.data != NULL && b.len > 0) {
    memcpy(buf + a.len, b.data, b.len);
  }
  buf[total] = '\0';
  VitteString out = {buf, total};
  return out;
}

VitteString vitte_i32_to_string(int32_t v) {
  char tmp[32];
  int n = snprintf(tmp, sizeof(tmp), "%d", v);
  if (n < 0) {
    vitte_note_panic(3);
    VitteString empty = {NULL, 0};
    return empty;
  }
  size_t len = (size_t)n;
  char *buf = (char *)malloc(len + 1);
  if (buf == NULL) {
    vitte_note_panic(3);
    VitteString empty = {NULL, 0};
    return empty;
  }
  memcpy(buf, tmp, len + 1);
  VitteString out = {buf, len};
  return out;
}

void vitte_set_args(int argc, const char **argv) {
  g_argc = argc;
  g_argv = argv;
}

VitteSliceString cli_args(void) {
  if (g_argv == NULL || g_argc <= 0) {
    return vitte_empty_slice_string();
  }
  VitteString *arr = (VitteString *)calloc((size_t)g_argc, sizeof(VitteString));
  if (arr == NULL) {
    vitte_note_panic(3);
    return vitte_empty_slice_string();
  }
  for (int i = 0; i < g_argc; i += 1) {
    const char *s = g_argv[i];
    if (s == NULL) {
      arr[i].data = "";
      arr[i].len = 0;
    } else {
      arr[i].data = s;
      arr[i].len = strlen(s);
    }
  }
  VitteSliceString out = {arr, (size_t)g_argc};
  return out;
}
