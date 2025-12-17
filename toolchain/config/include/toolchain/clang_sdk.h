// toolchain/config/include/toolchain/clang_sdk.h
// C17 - SDK/sysroot discovery + target triples helpers for clang/lld driver.

#ifndef VITTE_TOOLCHAIN_CLANG_SDK_H
#define VITTE_TOOLCHAIN_CLANG_SDK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "toolchain/clang_errors.h"

//------------------------------------------------------------------------------
// Target triple model
//------------------------------------------------------------------------------

typedef struct vitte_tc_triple_s {
    // Example: "x86_64-apple-darwin", "aarch64-unknown-linux-gnu",
    //          "x86_64-pc-windows-msvc", "aarch64-pc-windows-msvc"
    char arch[32];
    char vendor[32];
    char sys[32];
    char abi[32]; // optional, may be empty
} vitte_tc_triple;

//------------------------------------------------------------------------------
// SDK/sysroot model
//------------------------------------------------------------------------------

typedef enum vitte_tc_sdk_kind_e {
    VITTE_TC_SDK_NONE = 0,
    VITTE_TC_SDK_SYSROOT,  // a sysroot directory explicitly provided
    VITTE_TC_SDK_APPLE,    // Apple SDK path
    VITTE_TC_SDK_WINDOWS,  // Windows SDK (optional)
    VITTE_TC_SDK_LINUX,    // distro sysroot (optional)
    VITTE_TC_SDK_BSD       // BSD sysroot (optional)
} vitte_tc_sdk_kind;

typedef struct vitte_tc_sdk_info_s {
    vitte_tc_sdk_kind kind;

    // Resolved paths (NUL-terminated). Empty means "unset".
    char sysroot[1024];
    char sdkroot[1024];

    // Apple-specific (if kind==APPLE)
    char apple_sdk_name[64];   // "macosx", "iphoneos", ...
    char apple_sdk_version[64]; // optional, best-effort
} vitte_tc_sdk_info;

//------------------------------------------------------------------------------
// Triple helpers
//------------------------------------------------------------------------------

vitte_tc_error vitte_tc_triple_parse(const char* triple_str, vitte_tc_triple* out);
vitte_tc_error vitte_tc_triple_format(const vitte_tc_triple* t, char* out, size_t out_cap);

bool vitte_tc_triple_is_msvc(const vitte_tc_triple* t);
bool vitte_tc_triple_is_mingw(const vitte_tc_triple* t);
bool vitte_tc_triple_is_darwin(const vitte_tc_triple* t);
bool vitte_tc_triple_is_linux(const vitte_tc_triple* t);
bool vitte_tc_triple_is_bsd(const vitte_tc_triple* t);

// Convenience: normalize common arch aliases (e.g. amd64->x86_64, arm64->aarch64).
void vitte_tc_triple_normalize_arch(vitte_tc_triple* t);

//------------------------------------------------------------------------------
// SDK discovery
//------------------------------------------------------------------------------
//
// Discovery rules (implementation expectations):
// - If user provides sysroot/sdkroot explicitly => use them.
// - Apple: prefer xcrun if available; else use SDKROOT; else fallback heuristics.
// - Windows: optional probing of Windows Kits + MSVC via vswhere / env vars (if you support it).
// - Linux/BSD: typically sysroot is optional; can remain empty.
//
// This header provides an interface, not platform-specific command execution.
//
typedef struct vitte_tc_sdk_req_s {
    // Optional explicit inputs (may be NULL/empty).
    const char* sysroot;
    const char* sdkroot;

    // Optional SDK name request for Apple (NULL => macosx default).
    const char* apple_sdk_name;

    // Optional target triple (influences defaults).
    const char* target_triple;
} vitte_tc_sdk_req;

vitte_tc_error vitte_tc_sdk_resolve(const vitte_tc_sdk_req* req, vitte_tc_sdk_info* out);

//------------------------------------------------------------------------------
// Argument materialization
//------------------------------------------------------------------------------
//
// These helpers produce clang-style argv fragments.
//
// If sysroot is set:
//   --sysroot=<path>
//
// If apple sdkroot is set:
//   -isysroot <path>
//
// Caller provides out buffer as a flat string (space-separated) OR uses a token builder.
// Here we expose a flat-string variant for rsp-file injection.
//
vitte_tc_error vitte_tc_sdk_build_clang_args(const vitte_tc_sdk_info* sdk, char* out, size_t out_cap);
vitte_tc_error vitte_tc_sdk_build_lld_args(const vitte_tc_sdk_info* sdk, char* out, size_t out_cap);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_TOOLCHAIN_CLANG_SDK_H
