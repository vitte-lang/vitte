#ifndef VITTEC_INCLUDE_VITTEC_MUF_H
#define VITTEC_INCLUDE_VITTEC_MUF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Normalize/pretty-print a Muffin manifest string.
 *
 * - Writes a NUL-terminated string into `out` (if out_cap > 0).
 * - On success: returns 0 and sets `*out_len` to the written length (excluding NUL).
 * - On error: returns a vitte_err_code_t-compatible code (from rust/include-gen/vitte_rust_api.h).
 */
int vittec_muf_normalize(const char* src,
                         size_t src_len,
                         char* out,
                         size_t out_cap,
                         size_t* out_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

