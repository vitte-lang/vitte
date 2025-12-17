// toolchain/config/include/toolchain/clang_paths.h
// C17 - Path helpers for locating Clang/LLVM tools and SDK/sysroot resources.

#ifndef VITTE_TOOLCHAIN_CLANG_PATHS_H
#define VITTE_TOOLCHAIN_CLANG_PATHS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "toolchain/c_lang_paths.h"

//------------------------------------------------------------------------------
// Common executable names (platform-aware)
//------------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
  #define VITTE_TC_EXE_SUFFIX ".exe"
#else
  #define VITTE_TC_EXE_SUFFIX ""
#endif

#define VITTE_TC_EXE_CLANG        "clang"   VITTE_TC_EXE_SUFFIX
#define VITTE_TC_EXE_CLANGXX      "clang++" VITTE_TC_EXE_SUFFIX
#define VITTE_TC_EXE_LLD_LD       "ld.lld"  VITTE_TC_EXE_SUFFIX
#define VITTE_TC_EXE_LLD_LINK     "lld-link" VITTE_TC_EXE_SUFFIX
#define VITTE_TC_EXE_LLVM_AR      "llvm-ar" VITTE_TC_EXE_SUFFIX
#define VITTE_TC_EXE_LLVM_RANLIB  "llvm-ranlib" VITTE_TC_EXE_SUFFIX
#define VITTE_TC_EXE_LLVM_STRIP   "llvm-strip" VITTE_TC_EXE_SUFFIX

//------------------------------------------------------------------------------
// Tool roots / discovery inputs
//------------------------------------------------------------------------------
//
// Typical discovery order (implementation-side):
//  1) Explicit config (env / CLI / config file)
//  2) Extra search roots (e.g., toolchain bundle dir)
//  3) PATH
//
// This header only defines conventions and helpers.
//
#define VITTE_TC_ENV_CLANG        "VITTE_CLANG"
#define VITTE_TC_ENV_CLANGXX      "VITTE_CLANGXX"
#define VITTE_TC_ENV_LLD          "VITTE_LLD"
#define VITTE_TC_ENV_AR           "VITTE_AR"
#define VITTE_TC_ENV_RANLIB       "VITTE_RANLIB"
#define VITTE_TC_ENV_STRIP        "VITTE_STRIP"

#define VITTE_TC_ENV_SYSROOT      "VITTE_SYSROOT"
#define VITTE_TC_ENV_SDKROOT      "VITTE_SDKROOT"
#define VITTE_TC_ENV_EXTRA_PATH   "VITTE_EXTRA_PATH"

//------------------------------------------------------------------------------
// Platform-specific hints
//------------------------------------------------------------------------------

// Apple SDK default probing (implementation-side):
// - Prefer xcrun --show-sdk-path if available.
// - Fallback to SDKROOT env var.
// - Fallback to /Applications/Xcode.app/... (if present) or CLT SDKs.
//
// We only expose conventional strings here.
#define VITTE_TC_APPLE_SDK_MACOSX "macosx"
#define VITTE_TC_APPLE_SDK_IPHONEOS "iphoneos"
#define VITTE_TC_APPLE_SDK_IPHONESIMULATOR "iphonesimulator"

// Windows: typical LLVM install roots (implementation-side hints)
#define VITTE_TC_WIN_HINT_LLVM_DIR_ENV "LLVM_INSTALL_DIR"
#define VITTE_TC_WIN_HINT_PROGRAMFILES "ProgramFiles"
#define VITTE_TC_WIN_HINT_PROGRAMFILES_X86 "ProgramFiles(x86)"

//------------------------------------------------------------------------------
// Small helpers (string ops are implementation-side; keep header minimal)
//------------------------------------------------------------------------------

typedef struct vitte_tc_path_pair_s {
    const char* dir;   // directory path (no trailing sep required)
    const char* file;  // filename
} vitte_tc_path_pair;

// True if 'path' appears absolute for the current platform.
bool vitte_tc_path_is_absolute(const char* path);

// Join dir + sep + file into out (NUL-terminated). Returns false if truncated/invalid.
// out_cap includes terminating NUL.
bool vitte_tc_path_join2(const char* dir, const char* file, char* out, size_t out_cap);

// Normalize separators in-place (best-effort). On Windows converts '/' -> '\'.
void vitte_tc_path_normalize_seps(char* path);

// If 'path' is quoted ("...") or ('...'), strip a single layer of matching quotes in-place.
void vitte_tc_path_strip_quotes(char* path);

// Return the preferred LLD frontend name for platform:
// - Windows => lld-link
// - Else    => ld.lld
static inline const char* vitte_tc_lld_frontend_exe(void) {
#if defined(_WIN32) || defined(_WIN64)
    return VITTE_TC_EXE_LLD_LINK;
#else
    return VITTE_TC_EXE_LLD_LD;
#endif
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_TOOLCHAIN_CLANG_PATHS_H
