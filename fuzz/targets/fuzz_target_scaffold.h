// fuzz_target_scaffold.h
// Tiny helpers shared by fuzz harness stubs to reduce boilerplate.

#ifndef VITTE_FUZZ_TARGETS_FUZZ_TARGET_SCAFFOLD_H
#define VITTE_FUZZ_TARGETS_FUZZ_TARGET_SCAFFOLD_H

#include <stddef.h>
#include <stdint.h>

#include "fuzz/fuzz.h"

FUZZ_INLINE static uint32_t
fuzz_scaffold_seed32(const uint8_t* data, size_t size) {
  if (!data || size == 0)
    return 0;
  size_t take = size < 16 ? size : 16;
  return fuzz_fnv1a32(data, take);
}

FUZZ_INLINE static size_t
fuzz_scaffold_bounded(fuzz_reader* r, size_t max_cap) {
  uint64_t raw = 0;
  if (!fuzz_reader_read_uvar(r, &raw, 4)) {
    return 0;
  }
  size_t cap = (size_t)(raw % (max_cap + 1));
  return cap;
}

FUZZ_INLINE static void
fuzz_scaffold_consume_chunks(fuzz_reader* r, size_t max_chunks) {
  for (size_t i = 0; i < max_chunks && fuzz_reader_remaining(r) > 0; ++i) {
    size_t take = fuzz_scaffold_bounded(r, fuzz_reader_remaining(r));
    fuzz_bytes_view view;
    if (take == 0 || !fuzz_reader_read_bytes_view(r, take, &view))
      break;
    FUZZ_TRACE_TAG_U32("chunk.len", (uint32_t)view.size);
  }
}

#endif // VITTE_FUZZ_TARGETS_FUZZ_TARGET_SCAFFOLD_H
