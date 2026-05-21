#include "vitte_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
