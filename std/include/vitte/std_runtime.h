/* /Users/vincent/Documents/Github/vitte/std/include/vitte/std_runtime.h
 * -----------------------------------------------------------------------------
 * vitte stdlib - runtime C API (host/runtime integration)
 * -----------------------------------------------------------------------------
 * Stable C ABI for the Vitte runtime layer.
 *
 * Scope:
 * - allocator hooks + allocation APIs
 * - panic/assert/log hooks
 * - time facade
 * - RNG facade
 * - args/env facade
 * - optional dynamic API table
 *
 * C17-clean:
 * - No compiler attributes that trigger C/C++(1619) on struct-by-value params.
 * -----------------------------------------------------------------------------
 */

#ifndef VITTE_STD_RUNTIME_H
#define VITTE_STD_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#ifndef VITTE_STD_RUNTIME_VERSION
#define VITTE_STD_RUNTIME_VERSION 1
#endif

/* -----------------------------------------------------------------------------
 * Platform export / calling convention
 * -----------------------------------------------------------------------------
 */
#if defined(_WIN32) || defined(__CYGWIN__)
  #define VITTE_STDCALL __cdecl
  #if defined(VITTE_STD_RUNTIME_BUILD)
    #define VITTE_RT_API __declspec(dllexport)
  #else
    #define VITTE_RT_API __declspec(dllimport)
  #endif
#else
  #define VITTE_STDCALL
  #if defined(__GNUC__) || defined(__clang__)
    #define VITTE_RT_API __attribute__((visibility("default")))
  #else
    #define VITTE_RT_API
  #endif
#endif

#if !defined(VITTE_RT_WARN_UNUSED)
  #if defined(__GNUC__) || defined(__clang__)
    #define VITTE_RT_WARN_UNUSED __attribute__((warn_unused_result))
  #else
    #define VITTE_RT_WARN_UNUSED
  #endif
#endif

/* -----------------------------------------------------------------------------
 * Scalar types
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

typedef vitte_i32 vitte_status;

enum vitte_rt_status_codes {
  VITTE_RT_OK             = 0,
  VITTE_RT_E_INVALID      = -1,
  VITTE_RT_E_OOB          = -2,
  VITTE_RT_E_NOMEM        = -3,
  VITTE_RT_E_IO           = -4,
  VITTE_RT_E_NOT_FOUND    = -5,
  VITTE_RT_E_UNSUPPORTED  = -6,
  VITTE_RT_E_INTERNAL     = -7
};

typedef struct vitte_rt_err {
  vitte_status code;
  const char*  msg; /* optional null-terminated UTF-8 (may be NULL) */
} vitte_rt_err;

/* -----------------------------------------------------------------------------
 * Slices / buffers
 * -----------------------------------------------------------------------------
 */
typedef struct vitte_rt_str {
  const char* ptr; /* UTF-8 */
  size_t      len;
} vitte_rt_str;

typedef struct vitte_rt_bytes {
  const vitte_u8* ptr;
  size_t          len;
} vitte_rt_bytes;

typedef struct vitte_rt_mut_bytes {
  vitte_u8* ptr;
  size_t    len;
} vitte_rt_mut_bytes;

/* Owned buffer allocated by runtime; must be freed with vitte_rt_free_buf(). */
typedef struct vitte_rt_buf {
  void*  ptr;
  size_t len;
  size_t cap;
} vitte_rt_buf;

/* Free an owned runtime buffer (safe on NULL pointer; ignores empty buffers). */
VITTE_RT_API void VITTE_STDCALL vitte_rt_free_buf(vitte_rt_buf* b);

/* -----------------------------------------------------------------------------
 * Allocator hooks
 * -----------------------------------------------------------------------------
 */
typedef void* (VITTE_STDCALL *vitte_rt_alloc_fn)(void* ctx, size_t size, size_t align);
typedef void  (VITTE_STDCALL *vitte_rt_free_fn)(void* ctx, void* ptr, size_t size, size_t align);
typedef void* (VITTE_STDCALL *vitte_rt_realloc_fn)(void* ctx, void* ptr, size_t old_size, size_t new_size, size_t align);

typedef struct vitte_rt_alloc {
  void*               ctx;
  vitte_rt_alloc_fn   alloc;
  vitte_rt_free_fn    free;
  vitte_rt_realloc_fn realloc;
} vitte_rt_alloc;

/* Set/override runtime allocator. Must be configured early (before std init). */
VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_set_alloc(const vitte_rt_alloc* a, vitte_rt_err* out_err);

/* Allocate/free via runtime (used by stdlib and generated code). */
VITTE_RT_API void* VITTE_STDCALL vitte_rt_malloc(size_t size, size_t align, vitte_rt_err* out_err) VITTE_RT_WARN_UNUSED;
VITTE_RT_API void  VITTE_STDCALL vitte_rt_free(void* ptr, size_t size, size_t align);
VITTE_RT_API void* VITTE_STDCALL vitte_rt_realloc(void* ptr, size_t old_size, size_t new_size, size_t align, vitte_rt_err* out_err) VITTE_RT_WARN_UNUSED;

/* -----------------------------------------------------------------------------
 * Panic / assert / logging
 * -----------------------------------------------------------------------------
 */
typedef void (VITTE_STDCALL *vitte_rt_panic_fn)(void* ctx, vitte_rt_str msg);
typedef void (VITTE_STDCALL *vitte_rt_log_fn)(void* ctx, vitte_rt_str msg);

typedef struct vitte_rt_hooks {
  void*             ctx;
  vitte_rt_panic_fn panic;
  vitte_rt_log_fn   log;
} vitte_rt_hooks;

/* Install hooks for panic/log. If not set, runtime uses default behavior. */
VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_set_hooks(const vitte_rt_hooks* h, vitte_rt_err* out_err);

/* Runtime-provided panic/assert (used by Vitte std.runtime::assert). */
VITTE_RT_API void VITTE_STDCALL vitte_rt_panic(vitte_rt_str msg);
VITTE_RT_API void VITTE_STDCALL vitte_rt_assert(vitte_bool cond, vitte_rt_str msg);

/* Logging helper */
VITTE_RT_API void VITTE_STDCALL vitte_rt_log(vitte_rt_str msg);

/* -----------------------------------------------------------------------------
 * Time facade
 * -----------------------------------------------------------------------------
 */
typedef struct vitte_rt_time {
  vitte_i64 unix_ns;
} vitte_rt_time;

VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_time_monotonic(vitte_rt_time* out, vitte_rt_err* out_err);
VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_time_realtime(vitte_rt_time* out, vitte_rt_err* out_err);

/* -----------------------------------------------------------------------------
 * RNG facade
 * -----------------------------------------------------------------------------
 */
VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_random_bytes(vitte_rt_mut_bytes out, vitte_rt_err* out_err);

/* -----------------------------------------------------------------------------
 * Args / env facade
 * -----------------------------------------------------------------------------
 * Ownership:
 * - vitte_rt_get_argv/get_env return owned vitte_rt_buf; free with vitte_rt_free_buf.
 */
VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_get_argc(vitte_u32* out_argc, vitte_rt_err* out_err);
VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_get_argv(vitte_u32 index, vitte_rt_buf* out, vitte_rt_err* out_err);

VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_get_env(vitte_rt_str key, vitte_rt_buf* out, vitte_rt_err* out_err);
VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_set_env(vitte_rt_str key, vitte_rt_str value, vitte_rt_err* out_err);
VITTE_RT_API vitte_status VITTE_STDCALL vitte_rt_unset_env(vitte_rt_str key, vitte_rt_err* out_err);

/* -----------------------------------------------------------------------------
 * Dynamic loading: runtime API table (optional)
 * -----------------------------------------------------------------------------
 */
typedef struct vitte_rt_api {
  vitte_u32 abi_version;

  vitte_status (VITTE_STDCALL *set_alloc)(const vitte_rt_alloc* a, vitte_rt_err* out_err);
  void*        (VITTE_STDCALL *malloc)(size_t size, size_t align, vitte_rt_err* out_err);
  void         (VITTE_STDCALL *free)(void* ptr, size_t size, size_t align);
  void*        (VITTE_STDCALL *realloc)(void* ptr, size_t old_size, size_t new_size, size_t align, vitte_rt_err* out_err);
  void         (VITTE_STDCALL *free_buf)(vitte_rt_buf* b);

  vitte_status (VITTE_STDCALL *set_hooks)(const vitte_rt_hooks* h, vitte_rt_err* out_err);
  void         (VITTE_STDCALL *panic)(vitte_rt_str msg);
  void         (VITTE_STDCALL *assert_)(vitte_bool cond, vitte_rt_str msg);
  void         (VITTE_STDCALL *log)(vitte_rt_str msg);

  vitte_status (VITTE_STDCALL *time_monotonic)(vitte_rt_time* out, vitte_rt_err* err);
  vitte_status (VITTE_STDCALL *time_realtime)(vitte_rt_time* out, vitte_rt_err* err);

  vitte_status (VITTE_STDCALL *random_bytes)(vitte_rt_mut_bytes out, vitte_rt_err* err);

  vitte_status (VITTE_STDCALL *get_argc)(vitte_u32* out_argc, vitte_rt_err* err);
  vitte_status (VITTE_STDCALL *get_argv)(vitte_u32 index, vitte_rt_buf* out, vitte_rt_err* err);

  vitte_status (VITTE_STDCALL *get_env)(vitte_rt_str key, vitte_rt_buf* out, vitte_rt_err* err);
  vitte_status (VITTE_STDCALL *set_env)(vitte_rt_str key, vitte_rt_str value, vitte_rt_err* err);
  vitte_status (VITTE_STDCALL *unset_env)(vitte_rt_str key, vitte_rt_err* err);
} vitte_rt_api;

/* Exported by runtime shared lib when dynamic API is enabled. */
VITTE_RT_API const vitte_rt_api* VITTE_STDCALL vitte_rt_get_api(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_STD_RUNTIME_H */