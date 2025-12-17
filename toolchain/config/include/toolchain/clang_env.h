// toolchain/config/include/toolchain/clang_env.h
// C17 - Toolchain environment helpers for invoking Clang/LLVM components.

#ifndef VITTE_TOOLCHAIN_CLANG_ENV_H
#define VITTE_TOOLCHAIN_CLANG_ENV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#if defined(_WIN32) || defined(_WIN64)
  #define VITTE_TC_PLATFORM_WINDOWS 1
#else
  #define VITTE_TC_PLATFORM_WINDOWS 0
#endif

#if defined(__APPLE__)
  #define VITTE_TC_PLATFORM_APPLE 1
#else
  #define VITTE_TC_PLATFORM_APPLE 0
#endif

#if defined(__linux__)
  #define VITTE_TC_PLATFORM_LINUX 1
#else
  #define VITTE_TC_PLATFORM_LINUX 0
#endif

#if defined(__FreeBSD__)
  #define VITTE_TC_PLATFORM_FREEBSD 1
#else
  #define VITTE_TC_PLATFORM_FREEBSD 0
#endif

//------------------------------------------------------------------------------
// Result codes
//------------------------------------------------------------------------------

typedef enum vitte_tc_status_e {
    VITTE_TC_OK = 0,
    VITTE_TC_ERR_INVALID_ARG = 1,
    VITTE_TC_ERR_NOT_FOUND = 2,
    VITTE_TC_ERR_IO = 3,
    VITTE_TC_ERR_NOMEM = 4,
    VITTE_TC_ERR_OVERFLOW = 5,
    VITTE_TC_ERR_INTERNAL = 100
} vitte_tc_status;

//------------------------------------------------------------------------------
// String view / buffers
//------------------------------------------------------------------------------

typedef struct vitte_tc_sv_s {
    const char* ptr;
    size_t      len;
} vitte_tc_sv;

typedef struct vitte_tc_buf_s {
    char*  data;
    size_t len;
    size_t cap;
} vitte_tc_buf;

// Initializes a buffer with user-provided memory (stack or heap).
static inline void vitte_tc_buf_init(vitte_tc_buf* b, char* mem, size_t cap) {
    if (!b) return;
    b->data = mem;
    b->len = 0;
    b->cap = cap;
    if (b->data && b->cap) b->data[0] = '\0';
}

//------------------------------------------------------------------------------
// Tool identifiers
//------------------------------------------------------------------------------

typedef enum vitte_tc_tool_e {
    VITTE_TC_TOOL_CLANG = 0,
    VITTE_TC_TOOL_CLANGXX,
    VITTE_TC_TOOL_LLD,
    VITTE_TC_TOOL_AR,
    VITTE_TC_TOOL_RANLIB,
    VITTE_TC_TOOL_STRIP,
    VITTE_TC_TOOL_LLVM_NM,
    VITTE_TC_TOOL_LLVM_OBJDUMP,
    VITTE_TC_TOOL_LLVM_READOBJ,
    VITTE_TC_TOOL_LLVM_SIZE
} vitte_tc_tool;

//------------------------------------------------------------------------------
// Environment model
//------------------------------------------------------------------------------

typedef struct vitte_tc_env_s {
    // Optional explicit tool paths (absolute or relative). If empty => resolved via PATH / discovery.
    vitte_tc_sv clang;
    vitte_tc_sv clangxx;
    vitte_tc_sv lld;
    vitte_tc_sv ar;
    vitte_tc_sv ranlib;
    vitte_tc_sv strip;

    // Optional SDK/sysroot hints (platform dependent).
    vitte_tc_sv sysroot;
    vitte_tc_sv sdkroot;

    // Optional additional PATH search root(s) (semicolon-separated on Windows, colon-separated elsewhere).
    vitte_tc_sv extra_path;

    // Cached/normalized platform flags.
    bool is_windows;
    bool is_apple;
    bool is_linux;
    bool is_freebsd;
} vitte_tc_env;

// Zero-init helper.
static inline vitte_tc_env vitte_tc_env_zero(void) {
    vitte_tc_env e;
    e.clang = (vitte_tc_sv){0,0};
    e.clangxx = (vitte_tc_sv){0,0};
    e.lld = (vitte_tc_sv){0,0};
    e.ar = (vitte_tc_sv){0,0};
    e.ranlib = (vitte_tc_sv){0,0};
    e.strip = (vitte_tc_sv){0,0};
    e.sysroot = (vitte_tc_sv){0,0};
    e.sdkroot = (vitte_tc_sv){0,0};
    e.extra_path = (vitte_tc_sv){0,0};
    e.is_windows = (VITTE_TC_PLATFORM_WINDOWS != 0);
    e.is_apple = (VITTE_TC_PLATFORM_APPLE != 0);
    e.is_linux = (VITTE_TC_PLATFORM_LINUX != 0);
    e.is_freebsd = (VITTE_TC_PLATFORM_FREEBSD != 0);
    return e;
}

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

// Fill platform booleans from compile-time macros (and optionally runtime checks if needed).
void vitte_tc_env_init_platform(vitte_tc_env* env);

// Read common environment variables to populate tool paths/hints.
// Convention (suggested):
//   VITTE_CLANG, VITTE_CLANGXX, VITTE_LLD, VITTE_AR, VITTE_RANLIB, VITTE_STRIP
//   VITTE_SYSROOT, VITTE_SDKROOT, VITTE_EXTRA_PATH
void vitte_tc_env_load_from_process_env(vitte_tc_env* env);

// Resolve a tool path into out_buf (NUL-terminated). If env has explicit path, use it.
// Otherwise search PATH (and env->extra_path if provided) for the executable.
vitte_tc_status vitte_tc_env_resolve_tool(
    const vitte_tc_env* env,
    vitte_tc_tool tool,
    vitte_tc_buf* out_buf);

// Resolve sysroot / sdkroot (if any) into out_buf (NUL-terminated). Returns OK with empty string if unset.
vitte_tc_status vitte_tc_env_resolve_sysroot(const vitte_tc_env* env, vitte_tc_buf* out_buf);
vitte_tc_status vitte_tc_env_resolve_sdkroot(const vitte_tc_env* env, vitte_tc_buf* out_buf);

// Helpers: PATH-like join using platform separator.
vitte_tc_status vitte_tc_env_build_search_path(
    const vitte_tc_env* env,
    vitte_tc_buf* out_buf);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_TOOLCHAIN_CLANG_ENV_H
