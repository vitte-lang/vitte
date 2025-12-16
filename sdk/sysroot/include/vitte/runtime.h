/**
 * Vitte Runtime ABI Header
 *
 * Defines runtime support functions and types needed by compiled Vitte code.
 */

#ifndef VITTE_RUNTIME_H
#define VITTE_RUNTIME_H

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Entry Point ========== */

/**
 * Main entry point for Vitte programs
 */
int32_t vitte_main(int32_t argc, const char** argv);

/* ========== String Support ========== */

/**
 * Create a string slice from C string
 */
vitte_str_t vitte_cstr_to_slice(const char* s);

/**
 * Create a string slice from data and length
 */
vitte_str_t vitte_slice_from_data(const char* data, size_t len);

/**
 * Create a C string from Vitte string (caller must free)
 */
char* vitte_str_to_cstr(vitte_str_t s);

/* ========== Debug Support ========== */

/**
 * Print debug message
 */
void vitte_debug_print(const char* format, ...);

/**
 * Print with color (for terminals)
 */
void vitte_debug_print_colored(const char* color, const char* format, ...);

/* ========== Backtrace Support ========== */

/**
 * Get current call stack
 */
typedef struct {
  const char** frames;
  size_t count;
} vitte_backtrace_t;

vitte_backtrace_t vitte_get_backtrace(void);

/**
 * Free backtrace resources
 */
void vitte_free_backtrace(vitte_backtrace_t bt);

/* ========== Benchmarking ========== */

typedef struct {
  uint64_t nanoseconds;
} vitte_duration_t;

/**
 * Measure time for benchmarking
 */
vitte_duration_t vitte_benchmark_start(void);
vitte_duration_t vitte_benchmark_end(vitte_duration_t start);

/* ========== Type Information ========== */

/**
 * Get size of a type (for debugging)
 */
size_t vitte_sizeof(const char* type_name);

/**
 * Get alignment of a type
 */
size_t vitte_alignof(const char* type_name);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_RUNTIME_H */
