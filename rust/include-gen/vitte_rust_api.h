// vitte/rust/include-gen/vitte_rust_api.h
//
// Auto-generated or manually maintained C header for vitte_rust_api.
// This header defines the public C ABI for Rust components.
//
// Notes:
//  - This must match the Rust extern "C" symbols in:
//      rust/crates/vitte_rust_api/src/lib.rs
//  - Keep types POD and ABI-stable.
//  - Strings are passed as (ptr,len) and are NOT NUL-terminated.
//  - Unless specified, caller owns buffers and provides output storage.
//
// If you generate this with cbindgen, align naming and packed rules
// and keep this file as the canonical interface for C consumers.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// ABI version
// =============================================================================

#define VITTE_RUST_API_ABI_VERSION 2u

uint32_t vitte_rust_api_abi_version(void);

// =============================================================================
// Error codes (mirror of vitte_common::ErrCode)
// =============================================================================
//
// Keep in sync with rust/crates/vitte_common/src/err.rs (ErrCode)
// and any C-side rust_abi.h if you have one.
//
typedef enum vitte_err_code_t {
  VITTE_ERR_OK = 0,

  VITTE_ERR_UNKNOWN = 1,
  VITTE_ERR_INVALID_ARGUMENT = 2,
  VITTE_ERR_OUT_OF_RANGE = 3,
  VITTE_ERR_OVERFLOW = 4,
  VITTE_ERR_UNDERFLOW = 5,
  VITTE_ERR_NOT_FOUND = 6,
  VITTE_ERR_ALREADY_EXISTS = 7,
  VITTE_ERR_UNSUPPORTED = 8,
  VITTE_ERR_NOT_IMPLEMENTED = 9,
  VITTE_ERR_PERMISSION_DENIED = 10,
  VITTE_ERR_WOULD_BLOCK = 11,
  VITTE_ERR_TIMED_OUT = 12,
  VITTE_ERR_INTERRUPTED = 13,
  VITTE_ERR_CANCELLED = 14,
  VITTE_ERR_CORRUPT_DATA = 15,
  VITTE_ERR_BAD_FORMAT = 16,
  VITTE_ERR_BAD_STATE = 17,

  VITTE_ERR_IO = 50,
  VITTE_ERR_EOF = 51,
  VITTE_ERR_PATH_INVALID = 52,
  VITTE_ERR_PATH_TOO_LONG = 53,
  VITTE_ERR_NO_SPACE = 54,

  VITTE_ERR_UTF8_INVALID = 80,
  VITTE_ERR_UTF16_INVALID = 81,
  VITTE_ERR_UNICODE_INVALID = 82,

  VITTE_ERR_PARSE_ERROR = 100,
  VITTE_ERR_LEX_ERROR = 101,
  VITTE_ERR_SYNTAX_ERROR = 102,

  VITTE_ERR_TOOL_NOT_FOUND = 150,
  VITTE_ERR_TOOL_FAILED = 151,
  VITTE_ERR_COMPILE_ERROR = 152,
  VITTE_ERR_LINK_ERROR = 153,

  VITTE_ERR_VM_TRAP = 200,
  VITTE_ERR_VM_PANIC = 201,
  VITTE_ERR_VM_OOM = 202,

  VITTE_ERR_ABI_MISMATCH = 250,
  VITTE_ERR_ABI_VERSION_UNSUPPORTED = 251,
  VITTE_ERR_NULL_POINTER = 252,
  VITTE_ERR_BUFFER_TOO_SMALL = 253,
  VITTE_ERR_BAD_HANDLE = 254
} vitte_err_code_t;

// =============================================================================
// Core ABI types
// =============================================================================

typedef struct vitte_slice_u8_t {
  const uint8_t* ptr;
  size_t len;
} vitte_slice_u8_t;

typedef struct vitte_slice_mut_u8_t {
  uint8_t* ptr;
  size_t len;
} vitte_slice_mut_u8_t;

typedef struct vitte_str_t {
  const uint8_t* ptr;
  size_t len;
} vitte_str_t;

typedef struct vitte_status_t {
  int32_t code;      // vitte_err_code_t
  size_t written;    // bytes written OR required if code=BUFFER_TOO_SMALL
} vitte_status_t;

typedef struct vitte_handle_t {
  size_t raw; // 0 = null; opaque pointer value from Rust
} vitte_handle_t;

// =============================================================================
// Bench output (C -> Rust helper, structless ABI)
// =============================================================================
//
// Goal: avoid copying C benchmark structs into this header and avoid the
// "size query + malloc + second pass" pattern by streaming via callback.
//
// The C side provides a `vitte_bench_report_view_t` whose vtable exposes getters
// for report metadata and per-result fields. Rust emits stable JSON/CSV.

typedef enum vitte_bench_status_t {
  VITTE_BENCH_STATUS_OK = 0,
  VITTE_BENCH_STATUS_FAILED = 1,
  VITTE_BENCH_STATUS_SKIPPED = 2
} vitte_bench_status_t;

typedef int32_t (*vitte_write_fn)(void* ctx, const uint8_t* bytes, size_t len);

typedef struct vitte_writer_t {
  void* ctx;
  vitte_write_fn write;
  size_t max_bytes; // 0 => library default cap
} vitte_writer_t;

typedef struct vitte_bench_report_vtable_t {
  // Report metadata
  int32_t (*get_schema)(void* ctx, vitte_str_t* out);       // default handled by Rust if empty
  int32_t (*get_suite)(void* ctx, vitte_str_t* out);        // default handled by Rust if empty
  int32_t (*get_timestamp_ms)(void* ctx, int64_t* out);
  int32_t (*get_seed)(void* ctx, uint64_t* out);
  int32_t (*get_threads)(void* ctx, int32_t* out);
  int32_t (*get_repeat)(void* ctx, int32_t* out);
  int32_t (*get_warmup)(void* ctx, int32_t* out);
  int32_t (*get_iters)(void* ctx, int64_t* out);
  int32_t (*get_calibrate_ms)(void* ctx, int64_t* out);
  int32_t (*get_cpu_index)(void* ctx, int32_t* out);
  int32_t (*get_cpu_pinned)(void* ctx, int32_t* out);
  int32_t (*get_include_samples)(void* ctx, int32_t* out); // 0/1

  // Results
  int32_t (*get_results_count)(void* ctx, int32_t* out);
  int32_t (*get_result_name)(void* ctx, int32_t idx, vitte_str_t* out);
  int32_t (*get_result_status)(void* ctx, int32_t idx, int32_t* out); // vitte_bench_status_t
  int32_t (*get_result_error)(void* ctx, int32_t idx, vitte_str_t* out); // empty => ""

  // Metrics (double => JSON number; NaN/Inf normalized by Rust)
  int32_t (*get_metric_ns_per_op)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_ns_per_op_median)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_ns_per_op_p95)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_ns_per_op_mad)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_ns_per_op_iqr)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_ns_per_op_ci95_low)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_ns_per_op_ci95_high)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_bytes_per_sec)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_items_per_sec)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_iterations)(void* ctx, int32_t idx, int64_t* out);
  int32_t (*get_metric_elapsed_ms)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_iters_per_call)(void* ctx, int32_t idx, int64_t* out);
  int32_t (*get_metric_calls_per_sample)(void* ctx, int32_t idx, int64_t* out);
  int32_t (*get_metric_target_time_ms)(void* ctx, int32_t idx, int64_t* out);
  int32_t (*get_metric_cycles_per_sec_min)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_cycles_per_sec_max)(void* ctx, int32_t idx, double* out);
  int32_t (*get_metric_throttling_suspected)(void* ctx, int32_t idx, int32_t* out); // 0/1

  // Samples (optional)
  int32_t (*get_samples)(void* ctx, int32_t idx, const double** out_ptr, int32_t* out_count);
} vitte_bench_report_vtable_t;

typedef struct vitte_bench_report_view_t {
  void* ctx;
  const vitte_bench_report_vtable_t* vt;
} vitte_bench_report_view_t;

// Stream JSON/CSV by calling `writer.write(writer.ctx, bytes, len)`.
// - code=0 => ok, `written` is bytes written
// - other code => failure (writer error code may be forwarded)
vitte_status_t vitte_bench_report_write_json(vitte_bench_report_view_t view, vitte_writer_t writer);
vitte_status_t vitte_bench_report_write_csv(vitte_bench_report_view_t view, vitte_writer_t writer);

static inline vitte_str_t vitte_str_from_cstr(const char* s) {
  // Convenience only; assumes NUL-terminated input and uses strlen.
  // Avoid in low-level code if you want no libc.
  vitte_str_t out;
  out.ptr = (const uint8_t*)s;
  out.len = 0;
  if (!s) return out;
  while (s[out.len] != 0) out.len++;
  return out;
}

// =============================================================================
// Last error channel (thread-local)
// =============================================================================

void vitte_rust_api_clear_last_error(void);

// Writes last error into `out` (not NUL-terminated).
// If `out.len` is too small, returns code=BUFFER_TOO_SMALL and `written`=required size.
vitte_status_t vitte_rust_api_last_error(vitte_slice_mut_u8_t out);

// =============================================================================
// Muffin (.muf)
// =============================================================================

// Parse a muffin manifest and write a normalized/pretty representation to `out`.
// If `out` too small, returns BUFFER_TOO_SMALL with required `written`.
vitte_status_t vitte_muf_parse_and_normalize(vitte_str_t src, vitte_slice_mut_u8_t out);

// =============================================================================
// Regex (optional feature in Rust; may return NOT_IMPLEMENTED if disabled)
// =============================================================================

// Compile regex. Returns 0 on success, else vitte_err_code_t.
// On success, writes handle to *out_handle. Must be freed by vitte_regex_free.
int32_t vitte_regex_compile(vitte_str_t pattern, uint32_t flags, vitte_handle_t* out_handle);

void vitte_regex_free(vitte_handle_t handle);

// Find first match.
// Returns 0 on success, NOT_FOUND if no match, or other vitte_err_code_t on error.
// On success, writes byte offsets to out_start/out_end.
int32_t vitte_regex_find_first(vitte_handle_t handle, vitte_str_t haystack, size_t* out_start, size_t* out_end);

// RegexFlags bitset (must mirror vitte_regex::RegexFlags)
enum {
  VITTE_REGEX_I = 1u << 0, // case-insensitive
  VITTE_REGEX_M = 1u << 1, // multi-line
  VITTE_REGEX_S = 1u << 2, // dot matches newline
  VITTE_REGEX_U = 1u << 3  // swap greediness
};

// =============================================================================
// UTF-8 validate
// =============================================================================

// Returns 1 if valid UTF-8, else 0.
int32_t vitte_utf8_validate(vitte_slice_u8_t src);

#ifdef __cplusplus
} // extern "C"
#endif
