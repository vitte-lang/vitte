// fuzir_verify.c
// Fuzz target: IR verifier placeholder.

#include <stddef.h>
#include <stdint.h>

#include "fuzz/fuzz.h"

FUZZ_TARGET(fuzir_verify_target) {
  fuzz_reader r = fuzz_reader_from(data, size);
  uint8_t mode = 0;
  (void)fuzz_reader_read_u8(&r, &mode);

  uint64_t n = 0;
  if (!fuzz_reader_read_uvar(&r, &n, 4))
    return 0;
  if (n > 8192)
    n = 8192;

  for (uint64_t i = 0; i < n && fuzz_reader_remaining(&r) > 0; ++i) {
    uint8_t tag = 0;
    uint64_t len = 0;
    if (!fuzz_reader_read_u8(&r, &tag))
      break;
    if (!fuzz_reader_read_uvar(&r, &len, 4))
      break;
    size_t take = (size_t)len;
    if ((uint64_t)take != len)
      break;
    if (take > fuzz_reader_remaining(&r))
      take = fuzz_reader_remaining(&r);
    fuzz_bytes_view payload;
    if (!fuzz_reader_read_bytes_view(&r, take, &payload))
      break;

    FUZZ_TRACE_TAG_U32("ir.tag", (uint32_t)tag);
    FUZZ_TRACE_TAG_U32("ir.len", (uint32_t)payload.size);

    if ((mode & 1u) && payload.size >= 8) {
      fuzz_reader rr = fuzz_reader_from(payload.data, payload.size);
      uint64_t x = 0;
      if (fuzz_reader_read_u64_le(&rr, &x))
        FUZZ_TRACE_TAG_U32("ir.u64lo", (uint32_t)x);
    }
  }
  return 0;
}

#define FUZZ_DRIVER_TARGET fuzir_verify_target
#include "fuzz/fuzz_driver.h"
