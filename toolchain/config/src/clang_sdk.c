/*
 * toolchain/config/src/clang_sdk.c
 *
 * Vitte toolchain – SDK discovery & sysroot modeling for clang.
 *
 * Responsibilities (max):
 *  - Discover sysroot / SDK paths for:
 *      * Windows (MSVC/Windows SDK – best-effort)
 *      * macOS (xcrun --show-sdk-path)
 *      * iOS (xcrun --sdk iphoneos/iphonesimulator)
 *      * Linux (sysroot optional; typically none)
 *      * Android NDK (via env variables)
 *  - Expose a normalized tc_clang_sdk describing:
 *      * sysroot path
 *      * include roots (optional)
 *      * library roots (optional)
 *      * target triple hint
 *
 * Notes:
 *  - No heap allocations; everything is fixed-size buffers.
 *  - This module is best-effort; if not found, sysroot may be empty.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_sdk.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_process.h"
#include "toolchain/toolchain_fs.h"

/* -----------------------------------------------------------------------------
 * Utilities
 * -------------------------------------------------------------------------- */

static void tc_set(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return;
    if (!src) src = "";
    size_t n = strnlen(src, cap);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static bool tc_has(const char* s) { return s && s[0] != '\0'; }

static bool tc_is_macos_host(void) {
#if defined(__APPLE__) && defined(__MACH__)
    return true;
#else
    return false;
#endif
}

static bool tc_is_windows_host(void) {
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

/* Trim trailing whitespace */
static void tc_rstrip(char* s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n) {
        char c = s[n - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') n--;
        else break;
    }
    s[n] = '\0';
}

/* Run a command and capture stdout (toolchain_process layer must support capture). */
static bool tc_run_capture_stdout(const char* const* argv, size_t argc,
                                 char* out, size_t cap) {
    if (!argv || !argc || !out || cap == 0) return false;
    out[0] = '\0';

    tc_toolchain_process_result r = {0};
    if (tc_toolchain_process_spawn(argv, argc, &r) != TC_TOOLCHAIN_OK) return false;
    if (r.exit_code != 0) return false;
    if (!r.stdout_buf || r.stdout_len == 0) return false;

    size_t n = r.stdout_len;
    if (n >= cap) n = cap - 1;
    memcpy(out, r.stdout_buf, n);
    out[n] = '\0';
    tc_rstrip(out);
    return out[0] != '\0';
}

/* -----------------------------------------------------------------------------
 * OS-specific discovery
 * -------------------------------------------------------------------------- */

static void tc_sdk_detect_macos(tc_clang_sdk* s, tc_sdk_kind kind) {
    if (!tc_is_macos_host()) return;

    /* Prefer xcrun --show-sdk-path (macOS); for iOS use --sdk */
    char path[TC_SDK_PATH_MAX] = {0};

    if (kind == TC_SDK_MACOS) {
        const char* argv[] = { "xcrun", "--show-sdk-path", NULL };
        if (tc_run_capture_stdout(argv, 2, path, sizeof(path))) {
            tc_set(s->sysroot, sizeof(s->sysroot), path);
            s->found = true;
        }
        return;
    }

    if (kind == TC_SDK_IOS) {
        const char* argv[] = { "xcrun", "--sdk", "iphoneos", "--show-sdk-path", NULL };
        if (tc_run_capture_stdout(argv, 4, path, sizeof(path))) {
            tc_set(s->sysroot, sizeof(s->sysroot), path);
            s->found = true;
        }
        return;
    }

    if (kind == TC_SDK_IOS_SIM) {
        const char* argv[] = { "xcrun", "--sdk", "iphonesimulator", "--show-sdk-path", NULL };
        if (tc_run_capture_stdout(argv, 4, path, sizeof(path))) {
            tc_set(s->sysroot, sizeof(s->sysroot), path);
            s->found = true;
        }
        return;
    }
}

static void tc_sdk_detect_android(tc_clang_sdk* s) {
    /* Android NDK: accept env var ANDROID_NDK_ROOT or NDK_HOME and derive sysroot. */
    const char* ndk = tc_process_getenv("ANDROID_NDK_ROOT");
    if (!ndk || !*ndk) ndk = tc_process_getenv("NDK_HOME");
    if (!ndk || !*ndk) return;

    /* sysroot = <ndk>/toolchains/llvm/prebuilt/<host>/sysroot
     * host differs; we probe a few typical.
     */
    const char* prebuilt_cands[] = {
#if defined(_WIN32)
        "windows-x86_64",
#elif defined(__APPLE__) && defined(__MACH__)
        "darwin-x86_64",
        "darwin-arm64",
#else
        "linux-x86_64",
        "linux-aarch64",
#endif
        NULL
    };

    char base[TC_SDK_PATH_MAX] = {0};
    char cand[TC_SDK_PATH_MAX] = {0};

    (void)snprintf(base, sizeof(base), "%s/toolchains/llvm/prebuilt", ndk);
    for (size_t i = 0; prebuilt_cands[i]; ++i) {
        (void)snprintf(cand, sizeof(cand), "%s/%s/sysroot", base, prebuilt_cands[i]);
        tc_fs_path_normalize(cand, sizeof(cand));
        if (tc_fs_is_dir(cand)) {
            tc_set(s->sysroot, sizeof(s->sysroot), cand);
            s->found = true;
            break;
        }
    }
}

static void tc_sdk_detect_windows(tc_clang_sdk* s) {
    if (!tc_is_windows_host()) return;

    /* Best-effort:
     * - Use env vars from Visual Studio dev prompt if present:
     *     WindowsSdkDir, VCToolsInstallDir
     * - If WindowsSdkDir is present, treat it as root (not exact sysroot).
     *
     * Clang on Windows usually does NOT use --sysroot for MSVC; instead it uses:
     *   -fms-compatibility + -fms-runtime-lib + /imsvc etc.
     * This module only reports “sdk roots”; policy is handled elsewhere.
     */
    const char* win_sdk = tc_process_getenv("WindowsSdkDir");
    if (win_sdk && *win_sdk) {
        tc_set(s->sysroot, sizeof(s->sysroot), win_sdk);
        s->found = true;
        return;
    }

    /* fallback: empty (caller can still compile if clang is configured) */
}

/* -----------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

void tc_clang_sdk_zero(tc_clang_sdk* s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->kind = TC_SDK_NONE;
}

tc_toolchain_err tc_clang_sdk_detect(const tc_target* target,
                                     const tc_clang_sdk_opts* opts,
                                     tc_clang_sdk* out_sdk) {
    if (!out_sdk) return TC_TOOLCHAIN_EINVAL;
    tc_clang_sdk_zero(out_sdk);

    /* Decide sdk kind */
    tc_sdk_kind kind = TC_SDK_NONE;

    if (opts && opts->force_kind != TC_SDK_NONE) {
        kind = opts->force_kind;
    } else if (target) {
        if (target->os == TC_OS_DARWIN) kind = TC_SDK_MACOS;
        else if (target->os == TC_OS_IOS) kind = (opts && opts->ios_simulator) ? TC_SDK_IOS_SIM : TC_SDK_IOS;
        else if (target->os == TC_OS_ANDROID) kind = TC_SDK_ANDROID;
        else if (target->os == TC_OS_WINDOWS) kind = TC_SDK_WINDOWS;
        else kind = TC_SDK_NONE;
    }

    out_sdk->kind = kind;

    switch (kind) {
        case TC_SDK_MACOS:    tc_sdk_detect_macos(out_sdk, TC_SDK_MACOS); break;
        case TC_SDK_IOS:      tc_sdk_detect_macos(out_sdk, TC_SDK_IOS); break;
        case TC_SDK_IOS_SIM:  tc_sdk_detect_macos(out_sdk, TC_SDK_IOS_SIM); break;
        case TC_SDK_ANDROID:  tc_sdk_detect_android(out_sdk); break;
        case TC_SDK_WINDOWS:  tc_sdk_detect_windows(out_sdk); break;
        case TC_SDK_NONE:
        default:              break;
    }

    /* Optional include/lib roots (best-effort, user can inject later) */
    if (out_sdk->found && out_sdk->sysroot[0]) {
        /* Common sysroot layout (Apple/Android): <sysroot>/usr/include, <sysroot>/usr/lib */
        char inc[TC_SDK_PATH_MAX] = {0};
        char lib[TC_SDK_PATH_MAX] = {0};

        (void)snprintf(inc, sizeof(inc), "%s/usr/include", out_sdk->sysroot);
        (void)snprintf(lib, sizeof(lib), "%s/usr/lib", out_sdk->sysroot);
        tc_fs_path_normalize(inc, sizeof(inc));
        tc_fs_path_normalize(lib, sizeof(lib));

        if (tc_fs_is_dir(inc)) tc_set(out_sdk->include_root, sizeof(out_sdk->include_root), inc);
        if (tc_fs_is_dir(lib)) tc_set(out_sdk->lib_root, sizeof(out_sdk->lib_root), lib);
    }

    return TC_TOOLCHAIN_OK;
}
