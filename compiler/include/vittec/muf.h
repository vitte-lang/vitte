#ifndef VITTEC_INCLUDE_VITTEC_MUF_H
#define VITTEC_INCLUDE_VITTEC_MUF_H

#include <stddef.h>

enum vitte_err_code_t;
typedef enum vitte_err_code_t vitte_err_code_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Normalize/pretty-print a Muffin manifest string.
 *
 * - Writes a NUL-terminated string into `out` (if out_cap > 0).
 * - On success: returns `VITTE_ERR_OK` and sets `*out_len` to the written length (excluding NUL).
 * - On error: returns a vitte_err_code_t (Rust ABI error code).
 */
vitte_err_code_t vittec_muf_normalize(const char* src,
                                      size_t src_len,
                                      char* out,
                                      size_t out_cap,
                                      size_t* out_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

