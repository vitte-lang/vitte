/* /Users/vincent/Documents/Github/vitte/std/include/vitte/std_ffi.h
 * -----------------------------------------------------------------------------
 * vitte stdlib - C FFI surface
 * -----------------------------------------------------------------------------
 * Header-only ABI contract for consuming stdlib services from C/C++.
 *
 * Design goals:
 * - Stable C ABI (no C++ name mangling)
 * - Minimal, bootstrap-friendly types (slices, status codes)
 * - Optional dynamic loading (function table) OR direct linking
 *
 * NOTE:
 * - This file defines ABI types and function signatures only.
 * - Implementations live in the std runtime / host library.
 * -----------------------------------------------------------------------------
 */

#ifndef VITTE_STD_FFI_H
#define VITTE_STD_FFI_H

/* C/C++ compatibility */
#ifdef __cplusplus
extern "C" {
#endif

/* Standard includes */
#include <stddef.h>  /* size_t */
#include <stdint.h>  /* uint*_t, int*_t */

#ifndef VITTE_STD_FFI_VERSION
#define VITTE_STD_FFI_VERSION 1
#endif

/* -----------------------------------------------------------------------------
 * Platform export / calling convention
 * -----------------------------------------------------------------------------
 */
#if defined(_WIN32) || defined(__CYGWIN__)
  #define VITTE_STDCALL __cdecl
  #if defined(VITTE_STD_FFI_BUILD)
    #define VITTE_STD_API __declspec(dllexport)
  #else
    #define VITTE_STD_API __declspec(dllimport)
  #endif
#else
  #define VITTE_STDCALL
  #if defined(__GNUC__) || defined(__clang__)
    #define VITTE_STD_API __attribute__((visibility("default")))
  #else
    #define VITTE_STD_API
  #endif
#endif

#if !defined(VITTE_STD_WARN_UNUSED)
  #if defined(__GNUC__) || defined(__clang__)
    #define VITTE_STD_WARN_UNUSED __attribute__((warn_unused_result))
  #else
    #define VITTE_STD_WARN_UNUSED
  #endif
#endif

/* -----------------------------------------------------------------------------
 * ABI scalar types
 * -----------------------------------------------------------------------------
 */
typedef uint8_t   vitte_u8;
typedef uint16_t  vitte_u16;
typedef uint32_t  vitte_u32;
typedef uint64_t  vitte_u64;

typedef int8_t    vitte_i8;
typedef int16_t   vitte_i16;
typedef int32_t   vitte_i32;
typedef int64_t   vitte_i64;

typedef uint8_t   vitte_bool; /* 0/1 */

/* -----------------------------------------------------------------------------
 * Status / error model
 * -----------------------------------------------------------------------------
 * Conventions:
 * - vitte_status == 0 => OK
 * - non-zero => error (negative preferred, but not required)
 * - vitte_err contains a code + optional static message pointer
 */
typedef vitte_i32 vitte_status;

enum vitte_status_codes {
  VITTE_OK                 = 0,

  /* generic */
  VITTE_E_INVALID          = -1,
  VITTE_E_OOB              = -2,
  VITTE_E_NOMEM            = -3,
  VITTE_E_IO               = -4,
  VITTE_E_NOT_FOUND        = -5,
  VITTE_E_UNSUPPORTED      = -6,
  VITTE_E_INTERNAL         = -7,

  /* utf8 / strings */
  VITTE_E_UTF8             = -20,

  /* filesystem */
  VITTE_E_FS               = -30,

  /* crypto / compression */
  VITTE_E_CRYPTO           = -40,
  VITTE_E_COMPRESS         = -50
};

typedef struct vitte_err {
  vitte_status code;  /* VITTE_OK or negative error */
  const char*  msg;   /* optional, UTF-8, null-terminated (may be NULL) */
} vitte_err;

/* -----------------------------------------------------------------------------
 * Slice types (borrowed views)
 * -----------------------------------------------------------------------------
 */
typedef struct vitte_bytes {
  const vitte_u8* ptr;
  size_t          len;
} vitte_bytes;

typedef struct vitte_mut_bytes {
  vitte_u8* ptr;
  size_t    len;
} vitte_mut_bytes;

/* UTF-8 string slice (borrowed) */
typedef struct vitte_str {
  const char* ptr;
  size_t      len;
} vitte_str;

/* -----------------------------------------------------------------------------
 * Owned buffer (host allocated)
 * -----------------------------------------------------------------------------
 * Used for returning newly allocated data across the ABI.
 * Deallocate with vitte_std_free().
 */
typedef struct vitte_buf {
  void*  ptr;
  size_t len;
  size_t cap;
} vitte_buf;

/* -----------------------------------------------------------------------------
 * Allocator hooks (optional)
 * -----------------------------------------------------------------------------
 * If set via vitte_std_set_alloc(), stdlib will use these hooks for allocations.
 */
typedef void* (VITTE_STDCALL *vitte_alloc_fn)(void* ctx, size_t size, size_t align);
typedef void  (VITTE_STDCALL *vitte_free_fn)(void* ctx, void* ptr, size_t size, size_t align);
typedef void* (VITTE_STDCALL *vitte_realloc_fn)(void* ctx, void* ptr, size_t old_size, size_t new_size, size_t align);

typedef struct vitte_alloc {
  void*            ctx;
  vitte_alloc_fn   alloc;
  vitte_free_fn    free;
  vitte_realloc_fn realloc;
} vitte_alloc;

/* -----------------------------------------------------------------------------
 * Initialization / teardown
 * -----------------------------------------------------------------------------
 * - Call vitte_std_init() once per process before using other APIs (safe to call
 *   multiple times: refcounted).
 * - Call vitte_std_shutdown() to release global resources.
 */
VITTE_STD_API vitte_status VITTE_STDCALL vitte_std_init(vitte_err* out_err);
VITTE_STD_API void         VITTE_STDCALL vitte_std_shutdown(void);

/* Set allocator hooks (optional). Must be called before allocations occur. */
VITTE_STD_API vitte_status VITTE_STDCALL vitte_std_set_alloc(const vitte_alloc* a, vitte_err* out_err);

/* Free a buffer returned by stdlib. (safe on NULL) */
VITTE_STD_API void         VITTE_STDCALL vitte_std_free(vitte_buf* b);

/* -----------------------------------------------------------------------------
 * Runtime / diagnostics
 * -----------------------------------------------------------------------------
 */
VITTE_STD_API void VITTE_STDCALL vitte_std_panic(vitte_str msg);

/* -----------------------------------------------------------------------------
 * Core: cmp / hash (pure)
 * -----------------------------------------------------------------------------
 */
VITTE_STD_API vitte_i32 VITTE_STDCALL vitte_core_cmp_u32(vitte_u32 a, vitte_u32 b);
VITTE_STD_API vitte_i32 VITTE_STDCALL vitte_core_cmp_i32(vitte_i32 a, vitte_i32 b);
VITTE_STD_API vitte_i32 VITTE_STDCALL vitte_core_cmp_u64(vitte_u64 a, vitte_u64 b);
VITTE_STD_API vitte_i32 VITTE_STDCALL vitte_core_cmp_i64(vitte_i64 a, vitte_i64 b);
VITTE_STD_API vitte_i32 VITTE_STDCALL vitte_core_cmp_bool(vitte_bool a, vitte_bool b);
VITTE_STD_API vitte_i32 VITTE_STDCALL vitte_core_cmp_str(vitte_str a, vitte_str b);

VITTE_STD_API vitte_u64 VITTE_STDCALL vitte_core_hash_u32(vitte_u32 x);
VITTE_STD_API vitte_u64 VITTE_STDCALL vitte_core_hash_u64(vitte_u64 x);
VITTE_STD_API vitte_u64 VITTE_STDCALL vitte_core_hash_i32(vitte_i32 x);
VITTE_STD_API vitte_u64 VITTE_STDCALL vitte_core_hash_bool(vitte_bool x);
VITTE_STD_API vitte_u64 VITTE_STDCALL vitte_core_hash_str(vitte_str s);
VITTE_STD_API vitte_u64 VITTE_STDCALL vitte_core_hash64_combine(vitte_u64 a, vitte_u64 b);

/* -----------------------------------------------------------------------------
 * String utilities (facade)
 * -----------------------------------------------------------------------------
 * Implementations are expected to match std.string semantics in Vitte.
 */
VITTE_STD_API vitte_status VITTE_STDCALL vitte_string_cmp(vitte_str a, vitte_str b, vitte_i32* out_cmp, vitte_err* out_err);
VITTE_STD_API vitte_status VITTE_STDCALL vitte_string_find(vitte_str haystack, vitte_str needle, vitte_i32* out_index, vitte_err* out_err);
VITTE_STD_API vitte_status VITTE_STDCALL vitte_string_rfind(vitte_str haystack, vitte_str needle, vitte_i32* out_index, vitte_err* out_err);

/* Creates a new owned buffer: concat(a,b) */
VITTE_STD_API vitte_status VITTE_STDCALL vitte_string_concat(vitte_str a, vitte_str b, vitte_buf* out_buf, vitte_err* out_err);

/* Returns a slice copy in an owned buffer: slice(s,start,len) */
VITTE_STD_API vitte_status VITTE_STDCALL vitte_string_slice_copy(vitte_str s, size_t start, size_t len, vitte_buf* out_buf, vitte_err* out_err);

/* -----------------------------------------------------------------------------
 * FS: path (pure)
 * -----------------------------------------------------------------------------
 * Returns newly allocated normalized path (UTF-8) in out_buf.
 */
VITTE_STD_API vitte_status VITTE_STDCALL vitte_fs_path_normalize(vitte_str p, vitte_buf* out_buf, vitte_err* out_err);
VITTE_STD_API vitte_status VITTE_STDCALL vitte_fs_path_join(vitte_str a, vitte_str b, vitte_buf* out_buf, vitte_err* out_err);
VITTE_STD_API vitte_status VITTE_STDCALL vitte_fs_path_dirname(vitte_str p, vitte_buf* out_buf, vitte_err* out_err);
VITTE_STD_API vitte_status VITTE_STDCALL vitte_fs_path_basename(vitte_str p, vitte_buf* out_buf, vitte_err* out_err);
VITTE_STD_API vitte_status VITTE_STDCALL vitte_fs_path_extname(vitte_str p, vitte_buf* out_buf, vitte_err* out_err);
VITTE_STD_API vitte_status VITTE_STDCALL vitte_fs_path_stem(vitte_str p, vitte_buf* out_buf, vitte_err* out_err);
VITTE_STD_API vitte_status VITTE_STDCALL vitte_fs_path_is_absolute(vitte_str p, vitte_bool* out_abs, vitte_err* out_err);
VITTE_STD_API vitte_status VITTE_STDCALL vitte_fs_path_relpath(vitte_str from, vitte_str to, vitte_buf* out_buf, vitte_err* out_err);

/* -----------------------------------------------------------------------------
 * Dynamic loading: function table (optional)
 * -----------------------------------------------------------------------------
 * Use case:
 * - Load a single symbol `vitte_std_get_api` from a shared library and obtain
 *   the table of function pointers.
 */
typedef struct vitte_std_api {
  vitte_u32 abi_version;

  /* init / alloc */
  vitte_status (VITTE_STDCALL *init)(vitte_err* out_err);
  void         (VITTE_STDCALL *shutdown)(void);
  vitte_status (VITTE_STDCALL *set_alloc)(const vitte_alloc* a, vitte_err* out_err);
  void         (VITTE_STDCALL *free_buf)(vitte_buf* b);

  /* core */
  vitte_i32 (VITTE_STDCALL *cmp_u32)(vitte_u32 a, vitte_u32 b);
  vitte_u64 (VITTE_STDCALL *hash_str)(vitte_str s);

  /* fs path */
  vitte_status (VITTE_STDCALL *path_normalize)(vitte_str p, vitte_buf* out, vitte_err* err);

  /* extend as needed */
} vitte_std_api;

/* Exported by the stdlib shared library when built with dynamic API enabled. */
VITTE_STD_API const vitte_std_api* VITTE_STDCALL vitte_std_get_api(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_STD_FFI_H */