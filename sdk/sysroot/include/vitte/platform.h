/**
 * Vitte Platform Abstraction Layer (PAL)
 *
 * Provides platform-independent access to OS functionality.
 */

#ifndef VITTE_PLATFORM_H
#define VITTE_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Platform Detection ========== */

#if defined(_WIN32) || defined(_WIN64)
#define VITTE_OS_WINDOWS 1
#elif defined(__linux__)
#define VITTE_OS_LINUX 1
#elif defined(__APPLE__)
#define VITTE_OS_MACOS 1
#elif defined(__unix__)
#define VITTE_OS_UNIX 1
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define VITTE_ARCH_X86_64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define VITTE_ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
#define VITTE_ARCH_ARM32 1
#endif

/* ========== File I/O ========== */

typedef int32_t vitte_fd_t;

#define VITTE_STDIN 0
#define VITTE_STDOUT 1
#define VITTE_STDERR 2

vitte_fd_t vitte_open(const char* path, int32_t flags);
int32_t vitte_close(vitte_fd_t fd);
int32_t vitte_read(vitte_fd_t fd, void* buf, size_t count);
int32_t vitte_write(vitte_fd_t fd, const void* buf, size_t count);

/* ========== Directory Operations ========== */

int32_t vitte_mkdir(const char* path, int32_t mode);
int32_t vitte_rmdir(const char* path);
int32_t vitte_remove(const char* path);

/* ========== Process ========== */

/**
 * Exit with status code
 */
void vitte_exit(int32_t code);

/**
 * Get environment variable
 */
const char* vitte_getenv(const char* name);

/**
 * Set environment variable
 */
int32_t vitte_setenv(const char* name, const char* value);

/* ========== Time ========== */

/**
 * Get current time in nanoseconds (monotonic)
 */
uint64_t vitte_time_ns(void);

/**
 * Get current time in seconds since epoch
 */
uint64_t vitte_time_sec(void);

/**
 * Sleep for nanoseconds
 */
void vitte_sleep_ns(uint64_t ns);

/* ========== Threading ========== */

typedef void* vitte_thread_t;
typedef void (*vitte_thread_fn_t)(void* arg);

/**
 * Create a new thread
 */
vitte_thread_t vitte_thread_create(vitte_thread_fn_t fn, void* arg);

/**
 * Join thread (wait for completion)
 */
int32_t vitte_thread_join(vitte_thread_t thread);

/**
 * Get current thread ID
 */
uint64_t vitte_thread_id(void);

/* ========== Synchronization ========== */

typedef void* vitte_mutex_t;
typedef void* vitte_cond_var_t;

vitte_mutex_t vitte_mutex_create(void);
void vitte_mutex_destroy(vitte_mutex_t mutex);
int32_t vitte_mutex_lock(vitte_mutex_t mutex);
int32_t vitte_mutex_unlock(vitte_mutex_t mutex);

/* ========== Dynamic Loading ========== */

typedef void* vitte_dynlib_t;

vitte_dynlib_t vitte_dynlib_open(const char* path);
void vitte_dynlib_close(vitte_dynlib_t lib);
void* vitte_dynlib_symbol(vitte_dynlib_t lib, const char* symbol);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_PLATFORM_H */
