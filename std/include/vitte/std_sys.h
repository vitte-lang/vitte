/* /Users/vincent/Documents/Github/vitte/std/include/vitte/std_sys.h
 * -----------------------------------------------------------------------------
 * vitte stdlib - system/OS facade C ABI
 * -----------------------------------------------------------------------------
 * This header defines the OS/syscall facade used by std.fs, std.net, std.process,
 * and other std modules that require system access.
 *
 * Design goals:
 * - Keep syscalls behind a stable, testable C ABI.
 * - Support multiple backends (posix/windows/custom sandbox).
 * - Avoid leaking platform-specific structs in public ABI.
 *
 * Notes:
 * - Most functions return vitte_sys_status (0 = OK, negative = error).
 * - For output strings/buffers, the sys layer allocates and the caller frees
 *   using vitte_sys_free_buf().
 * -----------------------------------------------------------------------------
 */

#ifndef VITTE_STD_SYS_H
#define VITTE_STD_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#ifndef VITTE_STD_SYS_VERSION
#define VITTE_STD_SYS_VERSION 1
#endif

/* -----------------------------------------------------------------------------
 * Platform export / calling convention
 * -----------------------------------------------------------------------------
 */
#if defined(_WIN32) || defined(__CYGWIN__)
  #define VITTE_STDCALL __cdecl
  #if defined(VITTE_STD_SYS_BUILD)
    #define VITTE_SYS_API __declspec(dllexport)
  #else
    #define VITTE_SYS_API __declspec(dllimport)
  #endif
#else
  #define VITTE_STDCALL
  #if defined(__GNUC__) || defined(__clang__)
    #define VITTE_SYS_API __attribute__((visibility("default")))
  #else
    #define VITTE_SYS_API
  #endif
#endif


#if !defined(VITTE_SYS_WARN_UNUSED)
  #if defined(__GNUC__) || defined(__clang__)
    #define VITTE_SYS_WARN_UNUSED __attribute__((warn_unused_result))
  #else
    #define VITTE_SYS_WARN_UNUSED
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

typedef uint8_t   vitte_bool;

typedef vitte_i32 vitte_sys_status;

/* -----------------------------------------------------------------------------
 * Error model
 * -----------------------------------------------------------------------------
 */
enum vitte_sys_status_codes {
  VITTE_SYS_OK            = 0,

  VITTE_SYS_E_INVALID     = -1,
  VITTE_SYS_E_OOB         = -2,
  VITTE_SYS_E_NOMEM       = -3,
  VITTE_SYS_E_IO          = -4,
  VITTE_SYS_E_NOT_FOUND   = -5,
  VITTE_SYS_E_PERM        = -6,
  VITTE_SYS_E_EXISTS      = -7,
  VITTE_SYS_E_NOT_EMPTY   = -8,
  VITTE_SYS_E_BUSY        = -9,
  VITTE_SYS_E_TIMEDOUT    = -10,
  VITTE_SYS_E_UNSUPPORTED = -11,
  VITTE_SYS_E_INTERNAL    = -12
};

typedef struct vitte_sys_err {
  vitte_sys_status code;
  const char*      msg; /* optional null-terminated */
} vitte_sys_err;

/* -----------------------------------------------------------------------------
 * Slices / buffers
 * -----------------------------------------------------------------------------
 */
typedef struct vitte_sys_str {
  const char* ptr; /* UTF-8 */
  size_t      len;
} vitte_sys_str;

typedef struct vitte_sys_bytes {
  const vitte_u8* ptr;
  size_t          len;
} vitte_sys_bytes;

typedef struct vitte_sys_mut_bytes {
  vitte_u8* ptr;
  size_t    len;
} vitte_sys_mut_bytes;

typedef struct vitte_sys_buf {
  void*  ptr;
  size_t len;
  size_t cap;
} vitte_sys_buf;

/* Free buffer allocated by sys layer. Safe on NULL. */
VITTE_SYS_API void VITTE_STDCALL vitte_sys_free_buf(vitte_sys_buf* b);

/* -----------------------------------------------------------------------------
 * File types / stat
 * -----------------------------------------------------------------------------
 */
typedef enum vitte_sys_file_type {
  VITTE_SYS_FT_UNKNOWN = 0,
  VITTE_SYS_FT_FILE    = 1,
  VITTE_SYS_FT_DIR     = 2,
  VITTE_SYS_FT_SYMLINK = 3
} vitte_sys_file_type;

typedef struct vitte_sys_file_info {
  vitte_sys_file_type type;
  vitte_u64           size;
  vitte_u64           mtime_ns;
  vitte_u64           atime_ns;
  vitte_u64           ctime_ns;
} vitte_sys_file_info;

VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_stat(vitte_sys_str path, vitte_bool follow_symlinks,
                                                            vitte_sys_file_info* out_info, vitte_sys_err* out_err);

/* -----------------------------------------------------------------------------
 * Directory iteration
 * -----------------------------------------------------------------------------
 * Opaque handle:
 * - Do not assume pointer size => we use u64 token.
 */
typedef struct vitte_sys_dir {
  vitte_u64 token;
} vitte_sys_dir;

typedef struct vitte_sys_dirent {
  vitte_sys_str      name;  /* entry name (not full path) */
  vitte_sys_file_type type; /* best-effort without extra syscalls */
} vitte_sys_dirent;

VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_dir_open(vitte_sys_str path, vitte_sys_dir* out_dir, vitte_sys_err* out_err);

VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_dir_read(vitte_sys_dir* dir, vitte_sys_dirent* out_ent,
                                                                vitte_bool* out_has, vitte_sys_err* out_err);

VITTE_SYS_API void VITTE_STDCALL vitte_sys_dir_close(vitte_sys_dir* dir);

/* -----------------------------------------------------------------------------
 * File operations (minimal set for std.fs)
 * -----------------------------------------------------------------------------
 */
VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_read_file(vitte_sys_str path, vitte_sys_buf* out_buf, vitte_sys_err* out_err);

VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_write_file(vitte_sys_str path, vitte_sys_bytes data, vitte_sys_err* out_err);

VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_remove_file(vitte_sys_str path, vitte_sys_err* out_err);
VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_mkdir(vitte_sys_str path, vitte_bool recursive, vitte_sys_err* out_err);
VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_rmdir(vitte_sys_str path, vitte_bool recursive, vitte_sys_err* out_err);

VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_rename(vitte_sys_str from, vitte_sys_str to, vitte_sys_err* out_err);
VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_copy_file(vitte_sys_str from, vitte_sys_str to, vitte_sys_err* out_err);

/* -----------------------------------------------------------------------------
 * Current working directory / absolute path
 * -----------------------------------------------------------------------------
 */
VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_get_cwd(vitte_sys_buf* out_buf, vitte_sys_err* out_err);
VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_set_cwd(vitte_sys_str path, vitte_sys_err* out_err);

VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_realpath(vitte_sys_str path, vitte_sys_buf* out_buf, vitte_sys_err* out_err);

/* -----------------------------------------------------------------------------
 * Process / env (optional, minimal)
 * -----------------------------------------------------------------------------
 */
VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_get_env(vitte_sys_str key, vitte_sys_buf* out_buf, vitte_sys_err* out_err);

VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_set_env(vitte_sys_str key, vitte_sys_str value, vitte_sys_err* out_err);
VITTE_SYS_API vitte_sys_status VITTE_STDCALL vitte_sys_unset_env(vitte_sys_str key, vitte_sys_err* out_err);

/* -----------------------------------------------------------------------------
 * Dynamic loading: sys API table (optional)
 * -----------------------------------------------------------------------------
 */
typedef struct vitte_sys_api {
  vitte_u32 abi_version;

  void (*free_buf)(vitte_sys_buf* b);

  vitte_sys_status (VITTE_STDCALL *stat)(vitte_sys_str path, vitte_bool follow, vitte_sys_file_info* out, vitte_sys_err* err);

  vitte_sys_status (VITTE_STDCALL *dir_open)(vitte_sys_str path, vitte_sys_dir* out_dir, vitte_sys_err* err);
  vitte_sys_status (VITTE_STDCALL *dir_read)(vitte_sys_dir* dir, vitte_sys_dirent* out_ent, vitte_bool* out_has, vitte_sys_err* err);
  void             (VITTE_STDCALL *dir_close)(vitte_sys_dir* dir);

  vitte_sys_status (VITTE_STDCALL *read_file)(vitte_sys_str path, vitte_sys_buf* out, vitte_sys_err* err);
  vitte_sys_status (VITTE_STDCALL *write_file)(vitte_sys_str path, vitte_sys_bytes data, vitte_sys_err* err);

  vitte_sys_status (VITTE_STDCALL *remove_file)(vitte_sys_str path, vitte_sys_err* err);
  vitte_sys_status (VITTE_STDCALL *mkdir)(vitte_sys_str path, vitte_bool recursive, vitte_sys_err* err);
  vitte_sys_status (VITTE_STDCALL *rmdir)(vitte_sys_str path, vitte_bool recursive, vitte_sys_err* err);

  vitte_sys_status (VITTE_STDCALL *rename_)(vitte_sys_str from, vitte_sys_str to, vitte_sys_err* err);
  vitte_sys_status (VITTE_STDCALL *copy_file)(vitte_sys_str from, vitte_sys_str to, vitte_sys_err* err);

  vitte_sys_status (VITTE_STDCALL *get_cwd)(vitte_sys_buf* out, vitte_sys_err* err);
  vitte_sys_status (VITTE_STDCALL *set_cwd)(vitte_sys_str path, vitte_sys_err* err);

  vitte_sys_status (VITTE_STDCALL *realpath)(vitte_sys_str path, vitte_sys_buf* out, vitte_sys_err* err);

  vitte_sys_status (VITTE_STDCALL *get_env)(vitte_sys_str key, vitte_sys_buf* out, vitte_sys_err* err);
  vitte_sys_status (VITTE_STDCALL *set_env)(vitte_sys_str key, vitte_sys_str value, vitte_sys_err* err);
  vitte_sys_status (VITTE_STDCALL *unset_env)(vitte_sys_str key, vitte_sys_err* err);
} vitte_sys_api;

/* Exported by sys shared lib when dynamic API is enabled. */
VITTE_SYS_API const vitte_sys_api* VITTE_STDCALL vitte_sys_get_api(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_STD_SYS_H */