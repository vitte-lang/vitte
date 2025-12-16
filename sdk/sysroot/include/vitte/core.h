/**
 * Vitte Core ABI Header
 *
 * This header defines the core types and interfaces for the Vitte runtime.
 * All types here are ABI-stable and should not change between minor versions.
 */

#ifndef VITTE_CORE_H
#define VITTE_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Version Information ========== */

#define VITTE_MAJOR 0
#define VITTE_MINOR 2
#define VITTE_PATCH 0

typedef struct {
  uint32_t major;
  uint32_t minor;
  uint32_t patch;
} vitte_version_t;

vitte_version_t vitte_get_version(void);

/* ========== Basic Types ========== */

/* Unit type (equivalent to void) */
typedef void vitte_unit_t;

/* Option type */
typedef struct {
  bool is_some;
  void* value;
} vitte_option_t;

/* Result type */
typedef struct {
  bool is_ok;
  void* value;
  void* error;
} vitte_result_t;

/* String slice */
typedef struct {
  const char* data;
  size_t len;
} vitte_str_t;

/* Slice (generic) */
typedef struct {
  void* data;
  size_t len;
  size_t item_size;
} vitte_slice_t;

/* ========== Error Handling ========== */

typedef enum {
  VITTE_ERROR_NONE = 0,
  VITTE_ERROR_PANIC = 1,
  VITTE_ERROR_ALLOCATION = 2,
  VITTE_ERROR_BOUNDS = 3,
  VITTE_ERROR_INVALID_ARG = 4,
} vitte_error_code_t;

typedef struct {
  vitte_error_code_t code;
  const char* message;
  const char* location; /* File:line */
} vitte_error_t;

vitte_error_t vitte_get_error(void);
void vitte_clear_error(void);

/* ========== Memory Management ========== */

void* vitte_alloc(size_t size);
void* vitte_realloc(void* ptr, size_t new_size);
void vitte_free(void* ptr);

/* ========== Panics ========== */

void vitte_panic(const char* message, const char* location);
void vitte_assert(bool condition, const char* message, const char* location);

/* ========== Initialization ========== */

/**
 * Initialize Vitte runtime.
 * Must be called once before any other Vitte functions.
 */
void vitte_init(void);

/**
 * Cleanup Vitte runtime.
 * Call once when done using Vitte.
 */
void vitte_cleanup(void);

/* ========== Thread Local Storage ========== */

/**
 * Get thread-local error state
 */
vitte_error_t* vitte_thread_local_error(void);

/* ========== Callback Support ========== */

/**
 * Callback for panic handling
 */
typedef void (*vitte_panic_callback_t)(const char* message, void* userdata);

void vitte_set_panic_callback(vitte_panic_callback_t cb, void* userdata);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_CORE_H */
