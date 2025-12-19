#include "vittec/config.h"
#include "vittec/muf.h"

#include <stdint.h>

#if VITTEC_ENABLE_RUST_API
  #include "vitte_rust_api.h"
#endif

int vittec_muf_normalize(const char* src,
                         size_t src_len,
                         char* out,
                         size_t out_cap,
                         size_t* out_len) {
  if (out_len) *out_len = 0;

#if !VITTEC_ENABLE_RUST_API
  (void)src;
  (void)src_len;
  (void)out;
  (void)out_cap;
  return 251; /* VITTE_ERR_ABI_VERSION_UNSUPPORTED (best-effort, stable int) */
#else
  vitte_str_t in;
  in.ptr = (const uint8_t*)src;
  in.len = src_len;

  vitte_slice_mut_u8_t out_slice;
  out_slice.ptr = (uint8_t*)out;
  out_slice.len = out_cap ? (out_cap - 1) : 0; /* reserve for NUL */

  vitte_status_t st = vitte_muf_parse_and_normalize(in, out_slice);
  if (st.code != 0) {
    if (out && out_cap) out[0] = '\0';
    if (out_len) *out_len = st.written;
    return (int)st.code;
  }

  if (out && out_cap) out[st.written] = '\0';
  if (out_len) *out_len = st.written;
  return 0;
#endif
}

