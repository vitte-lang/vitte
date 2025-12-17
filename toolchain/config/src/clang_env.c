/*
 * src/toolchain/clang_env.c
 *
 * Vitte toolchain – environment & probing for LLVM/Clang toolchain paths.
 *
 * Responsibilities (max):
 *  - Read toolchain overrides from environment variables
 *  - Resolve default tool names (clang/clang++/lld/llvm-ar/llvm-ranlib)
 *  - Probe common install layouts (Windows/MSYS2/LLVM, macOS Homebrew/Xcode CLT, Linux)
 *  - Normalize paths (slashes) and ensure executables exist (via toolchain_fs)
 *
 * This file depends on:
 *  - toolchain/toolchain_fs.h      : file existence, path ops (join, normalize)
 *  - toolchain/toolchain_process.h : optional "which" via spawn (not mandatory)
 *
 * Design:
 *  - No heap allocations; everything is written into tc_clang_paths.
 *  - Probing is best-effort; if not found, keep tool names (clang) and rely on PATH.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_env.h"
#include "toolchain/clang_paths.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_fs.h"
#include "toolchain/toolchain_process.h"

/* -----------------------------------------------------------------------------
 * Env keys
 * -------------------------------------------------------------------------- */

#define TC_ENV_LLVM_BIN_DIR   "VITTE_LLVM_BIN"
#define TC_ENV_TOOLCHAIN_ROOT "VITTE_TOOLCHAIN_ROOT"

#define TC_ENV_CLANG          "VITTE_CLANG"
#define TC_ENV_CLANGXX        "VITTE_CLANGXX"
#define TC_ENV_LLD            "VITTE_LLD"
#define TC_ENV_LLVM_AR        "VITTE_LLVM_AR"
#define TC_ENV_LLVM_RANLIB    "VITTE_LLVM_RANLIB"

/* Optional, for CI / explicit PATH injection */
#define TC_ENV_PATH           "PATH"

/* -----------------------------------------------------------------------------
 * Platform helpers
 * -------------------------------------------------------------------------- */

static bool tc_is_windows(void) {
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

static bool tc_is_macos(void) {
#if defined(__APPLE__) && defined(__MACH__)
    return true;
#else
    return false;
#endif
}

static bool tc_is_linux(void) {
#if defined(__linux__)
    return true;
#else
    return false;
#endif
}

static bool tc_is_freebsd(void) {
#if defined(__FreeBSD__)
    return true;
#else
    return false;
#endif
}

static void tc_path_set(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return;
    if (!src) src = "";
    size_t n = strnlen(src, cap);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static bool tc_path_nonempty(const char* p) {
    return p && p[0] != '\0';
}

/* join(dir, name) -> out (normalized if toolchain_fs provides normalize) */
static tc_toolchain_err tc_join_tool(char* out, size_t cap, const char* dir, const char* name) {
    if (!out || !cap || !dir || !name) return TC_TOOLCHAIN_EINVAL;
    out[0] = '\0';
    return tc_fs_path_join(out, cap, dir, name);
}

static bool tc_try_set_if_exists(char* dst, size_t cap, const char* candidate) {
    if (!candidate || !*candidate) return false;
    if (!tc_fs_is_file(candidate)) return false;
    tc_path_set(dst, cap, candidate);
    return true;
}

/* Try dir + toolname, with platform extension. */
static bool tc_try_in_dir(char* dst, size_t cap, const char* dir, const char* base_name) {
    if (!dst || !dir || !base_name) return false;

    char cand[TC_TOOL_PATH_MAX];
    cand[0] = '\0';

#if defined(_WIN32)
    /* prefer .exe on windows */
    char exe_name[128];
    exe_name[0] = '\0';
    (void)snprintf(exe_name, sizeof(exe_name), "%s.exe", base_name);

    if (tc_join_tool(cand, sizeof(cand), dir, exe_name) == TC_TOOLCHAIN_OK) {
        if (tc_try_set_if_exists(dst, cap, cand)) return true;
    }
#endif

    if (tc_join_tool(cand, sizeof(cand), dir, base_name) == TC_TOOLCHAIN_OK) {
        if (tc_try_set_if_exists(dst, cap, cand)) return true;
    }

    return false;
}

/* -----------------------------------------------------------------------------
 * Environment reading
 * -------------------------------------------------------------------------- */

static bool tc_env_get(char* out, size_t cap, const char* key) {
    if (!out || cap == 0 || !key) return false;
    out[0] = '\0';

    const char* v = tc_process_getenv(key); /* platform wrapper; must exist */
    if (!v || !*v) return false;

    size_t n = strnlen(v, cap);
    if (n >= cap) n = cap - 1;
    memcpy(out, v, n);
    out[n] = '\0';
    return true;
}

static void tc_apply_env_overrides(tc_clang_paths* p) {
    char buf[TC_TOOL_PATH_MAX];

    if (tc_env_get(buf, sizeof(buf), TC_ENV_LLVM_BIN_DIR)) {
        tc_path_set(p->llvm_bin_dir, sizeof(p->llvm_bin_dir), buf);
    }
    if (tc_env_get(buf, sizeof(buf), TC_ENV_TOOLCHAIN_ROOT)) {
        tc_path_set(p->toolchain_root, sizeof(p->toolchain_root), buf);
    }

    if (tc_env_get(buf, sizeof(buf), TC_ENV_CLANG)) {
        tc_path_set(p->clang, sizeof(p->clang), buf);
    }
    if (tc_env_get(buf, sizeof(buf), TC_ENV_CLANGXX)) {
        tc_path_set(p->clangxx, sizeof(p->clangxx), buf);
    }
    if (tc_env_get(buf, sizeof(buf), TC_ENV_LLD)) {
        tc_path_set(p->lld, sizeof(p->lld), buf);
    }
    if (tc_env_get(buf, sizeof(buf), TC_ENV_LLVM_AR)) {
        tc_path_set(p->llvm_ar, sizeof(p->llvm_ar), buf);
    }
    if (tc_env_get(buf, sizeof(buf), TC_ENV_LLVM_RANLIB)) {
        tc_path_set(p->llvm_ranlib, sizeof(p->llvm_ranlib), buf);
    }
}

/* -----------------------------------------------------------------------------
 * Default names (PATH-based)
 * -------------------------------------------------------------------------- */

static void tc_set_default_names(tc_clang_paths* p) {
    if (!tc_path_nonempty(p->clang)) {
#if defined(_WIN32)
        tc_path_set(p->clang, sizeof(p->clang), "clang");
#else
        tc_path_set(p->clang, sizeof(p->clang), "clang");
#endif
    }
    if (!tc_path_nonempty(p->clangxx)) {
        tc_path_set(p->clangxx, sizeof(p->clangxx), "clang++");
    }
    if (!tc_path_nonempty(p->lld)) {
#if defined(_WIN32)
        /* some setups prefer lld-link; keep lld as default */
        tc_path_set(p->lld, sizeof(p->lld), "lld");
#else
        tc_path_set(p->lld, sizeof(p->lld), "lld");
#endif
    }
    if (!tc_path_nonempty(p->llvm_ar)) {
        tc_path_set(p->llvm_ar, sizeof(p->llvm_ar), "llvm-ar");
    }
    if (!tc_path_nonempty(p->llvm_ranlib)) {
        tc_path_set(p->llvm_ranlib, sizeof(p->llvm_ranlib), "llvm-ranlib");
    }
}

/* -----------------------------------------------------------------------------
 * Probing strategies
 * -------------------------------------------------------------------------- */

static void tc_probe_from_llvm_bin_dir(tc_clang_paths* p) {
    if (!tc_path_nonempty(p->llvm_bin_dir)) return;

    /* If user gave LLVM bin dir, try to resolve exact tools there. */
    (void)tc_try_in_dir(p->clang, sizeof(p->clang), p->llvm_bin_dir, "clang");
    (void)tc_try_in_dir(p->clangxx, sizeof(p->clangxx), p->llvm_bin_dir, "clang++");
#if defined(_WIN32)
    /* windows: lld-link may be preferred for MSVC; keep lld too */
    if (!tc_try_in_dir(p->lld, sizeof(p->lld), p->llvm_bin_dir, "lld-link")) {
        (void)tc_try_in_dir(p->lld, sizeof(p->lld), p->llvm_bin_dir, "lld");
    }
#else
    (void)tc_try_in_dir(p->lld, sizeof(p->lld), p->llvm_bin_dir, "ld.lld");
    (void)tc_try_in_dir(p->lld, sizeof(p->lld), p->llvm_bin_dir, "lld");
#endif
    (void)tc_try_in_dir(p->llvm_ar, sizeof(p->llvm_ar), p->llvm_bin_dir, "llvm-ar");
    (void)tc_try_in_dir(p->llvm_ranlib, sizeof(p->llvm_ranlib), p->llvm_bin_dir, "llvm-ranlib");
}

static void tc_probe_common_install_locations(tc_clang_paths* p) {
    /* best-effort; only fill missing / still default names if files exist */

    if (tc_is_windows()) {
        /* Common LLVM installer path */
        const char* cands[] = {
            "C:\\Program Files\\LLVM\\bin",
            "C:\\LLVM\\bin",
            "C:\\Program Files (x86)\\LLVM\\bin",
            NULL
        };
        for (size_t i = 0; cands[i]; ++i) {
            if (!tc_path_nonempty(p->llvm_bin_dir)) {
                if (tc_fs_is_dir(cands[i])) {
                    tc_path_set(p->llvm_bin_dir, sizeof(p->llvm_bin_dir), cands[i]);
                }
            }
            if (tc_path_nonempty(p->llvm_bin_dir)) break;
        }
        if (tc_path_nonempty(p->llvm_bin_dir)) tc_probe_from_llvm_bin_dir(p);
        return;
    }

    if (tc_is_macos()) {
        /* Homebrew (Apple Silicon + Intel) */
        const char* brew[] = {
            "/opt/homebrew/opt/llvm/bin",
            "/usr/local/opt/llvm/bin",
            "/opt/homebrew/bin",
            "/usr/local/bin",
            NULL
        };
        for (size_t i = 0; brew[i]; ++i) {
            if (tc_fs_is_dir(brew[i])) {
                if (!tc_path_nonempty(p->llvm_bin_dir)) {
                    tc_path_set(p->llvm_bin_dir, sizeof(p->llvm_bin_dir), brew[i]);
                }
                tc_probe_from_llvm_bin_dir(p);
                /* even if partial, keep probing other dirs */
            }
        }
        return;
    }

    if (tc_is_linux() || tc_is_freebsd()) {
        /* Typical */
        const char* dirs[] = {
            "/usr/lib/llvm-18/bin",
            "/usr/lib/llvm-17/bin",
            "/usr/lib/llvm-16/bin",
            "/usr/lib/llvm-15/bin",
            "/usr/local/bin",
            "/usr/bin",
            NULL
        };
        for (size_t i = 0; dirs[i]; ++i) {
            if (tc_fs_is_dir(dirs[i])) {
                if (!tc_path_nonempty(p->llvm_bin_dir)) {
                    tc_path_set(p->llvm_bin_dir, sizeof(p->llvm_bin_dir), dirs[i]);
                }
                tc_probe_from_llvm_bin_dir(p);
            }
        }
        return;
    }
}

/* Try to resolve tool names through PATH (optional "which" implementation).
 * If your toolchain_process layer doesn't implement which, this is a no-op.
 */
static void tc_probe_via_which(tc_clang_paths* p) {
    if (!p) return;

    char resolved[TC_TOOL_PATH_MAX];

    if (tc_process_which(resolved, sizeof(resolved), p->clang)) {
        tc_path_set(p->clang, sizeof(p->clang), resolved);
    }
    if (tc_process_which(resolved, sizeof(resolved), p->clangxx)) {
        tc_path_set(p->clangxx, sizeof(p->clangxx), resolved);
    }
    if (tc_process_which(resolved, sizeof(resolved), p->lld)) {
        tc_path_set(p->lld, sizeof(p->lld), resolved);
    }
    if (tc_process_which(resolved, sizeof(resolved), p->llvm_ar)) {
        tc_path_set(p->llvm_ar, sizeof(p->llvm_ar), resolved);
    }
    if (tc_process_which(resolved, sizeof(resolved), p->llvm_ranlib)) {
        tc_path_set(p->llvm_ranlib, sizeof(p->llvm_ranlib), resolved);
    }
}

/* -----------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

tc_toolchain_err tc_clang_env_resolve(tc_clang_paths* out_paths,
                                      const tc_clang_env_opts* opts,
                                      tc_toolchain_process_result* out_diag) {
    if (!out_paths) return TC_TOOLCHAIN_EINVAL;

    tc_clang_paths_zero(out_paths);

    /* 1) apply env overrides */
    tc_apply_env_overrides(out_paths);

    /* 2) set defaults (PATH names) */
    tc_set_default_names(out_paths);

    /* 3) probe explicit llvm_bin_dir if provided */
    tc_probe_from_llvm_bin_dir(out_paths);

    /* 4) probe common install dirs if still not resolved */
    tc_probe_common_install_locations(out_paths);

    /* 5) optional: resolve via which */
    if (!opts || opts->probe_via_which) {
        tc_probe_via_which(out_paths);
    }

    /* 6) normalize slashes (optional) */
    tc_fs_path_normalize(out_paths->clang, sizeof(out_paths->clang));
    tc_fs_path_normalize(out_paths->clangxx, sizeof(out_paths->clangxx));
    tc_fs_path_normalize(out_paths->lld, sizeof(out_paths->lld));
    tc_fs_path_normalize(out_paths->llvm_ar, sizeof(out_paths->llvm_ar));
    tc_fs_path_normalize(out_paths->llvm_ranlib, sizeof(out_paths->llvm_ranlib));
    tc_fs_path_normalize(out_paths->llvm_bin_dir, sizeof(out_paths->llvm_bin_dir));
    tc_fs_path_normalize(out_paths->toolchain_root, sizeof(out_paths->toolchain_root));

    /* 7) basic validation info (optional diagnostics payload) */
    if (out_diag) {
        memset(out_diag, 0, sizeof(*out_diag));
        /* Implementation-specific: you may want to log resolved paths here. */
    }

    (void)opts;
    return TC_TOOLCHAIN_OK;
}
