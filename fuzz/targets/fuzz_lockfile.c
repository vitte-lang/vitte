// fuzz_lockfile.c
// Fuzz target: lockfile parser placeholder.

#include <stddef.h>
#include <stdint.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

static int lockfile_placeholder(const uint8_t* data, size_t size) {
  fuzz_reader r = fuzz_target_reader(data, size);
  (void)fuzz_target_mode(&r);

  uint64_t n = 0;
  if (!fuzz_reader_read_uvar(&r, &n, 4))
    return 0;
  if (n > 4096)
    n = 4096;

  for (uint64_t i = 0; i < n && fuzz_reader_remaining(&r) > 0; ++i) {
    fuzz_bytes_view k;
    fuzz_bytes_view v;
    if (!fuzz_reader_read_len_prefixed(&r, 64, &k))
      break;
    if (!fuzz_reader_read_len_prefixed(&r, 256, &v))
      break;
    FUZZ_TRACE_TAG_U32("lock.k", fuzz_fnv1a32(k.data, k.size));
    FUZZ_TRACE_TAG_U32("lock.v", fuzz_fnv1a32(v.data, v.size));
  }

  return 0;
}

FUZZ_TARGET(fuzz_lockfile_target) {
  return lockfile_placeholder(data, size);
}

#define FUZZ_DRIVER_TARGET fuzz_lockfile_target
#include "fuzz/fuzz_driver.h"
