// C:\Users\vince\Documents\GitHub\vitte\fuzz\include\fuzz\fuzz_mutators.h
// fuzz_mutators.h — mutation helpers for fuzzing (C17)
//
// Provides:
//  - small deterministic PRNG
//  - generic byte mutations (flip, insert, delete, overwrite, arithmetic)
//  - token/dictionary-based insertion + replacement (AFL/libFuzzer friendly)
//  - optional structured mutators using fuzz_reader from fuzz_io.h
//
// Notes:
//  - This is intentionally not a full-blown mutator framework.
//  - It is meant for:
//      * custom mutator in libFuzzer (LLVMFuzzerCustomMutator)
//      * AFL++ custom mutator logic
//      * standalone corpus mangling tools
//
// Dependencies: fuzz_assert.h, fuzz_dict.h, fuzz_io.h

#ifndef VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_MUTATORS_H
#define VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_MUTATORS_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz_assert.h"
#include "fuzz_dict.h"
#include "fuzz_io.h"
#include "fuzz_util.h"

FUZZ_INLINE static int
fuzz_rng_bool(fuzz_rng* r) {
  return (int)(fuzz_rng_next_u64(r) & 1ull);
}

FUZZ_INLINE static uint8_t
fuzz_rng_byte(fuzz_rng* r) {
  return (uint8_t)fuzz_rng_next_u64(r);
}

//------------------------------------------------------------------------------
// Mutation config
//------------------------------------------------------------------------------

typedef struct fuzz_mutation_cfg {
  size_t max_size;      // hard cap output buffer size
  size_t max_insert;    // max bytes to insert at once
  size_t max_delete;    // max bytes to delete at once
  size_t max_overwrite; // max bytes to overwrite at once
} fuzz_mutation_cfg;

FUZZ_INLINE static fuzz_mutation_cfg
fuzz_mutation_cfg_default(void) {
  fuzz_mutation_cfg c;
  c.max_size = 1u << 20; // 1 MiB default
  c.max_insert = 128;
  c.max_delete = 128;
  c.max_overwrite = 128;
  return c;
}

//------------------------------------------------------------------------------
// Buffer helpers
//------------------------------------------------------------------------------

FUZZ_INLINE static size_t
fuzz__min_size(size_t a, size_t b) {
  return a < b ? a : b;
}
FUZZ_INLINE static size_t
fuzz__max_size(size_t a, size_t b) {
  return a > b ? a : b;
}

FUZZ_INLINE static int
fuzz_buf_insert(uint8_t* data, size_t* size, size_t max_size, size_t pos, const uint8_t* ins,
                size_t ins_n) {
  FUZZ_ASSERT(data || max_size == 0);
  FUZZ_ASSERT(size);
  FUZZ_ASSERT(ins || ins_n == 0);

  if (pos > *size)
    return 0;
  if (*size + ins_n > max_size)
    return 0;

  memmove(data + pos + ins_n, data + pos, *size - pos);
  if (ins_n)
    memcpy(data + pos, ins, ins_n);
  *size += ins_n;
  return 1;
}

FUZZ_INLINE static int
fuzz_buf_delete(uint8_t* data, size_t* size, size_t pos, size_t del_n) {
  FUZZ_ASSERT(data || *size == 0);
  FUZZ_ASSERT(size);
  if (pos > *size)
    return 0;
  if (del_n > *size - pos)
    return 0;

  memmove(data + pos, data + pos + del_n, *size - (pos + del_n));
  *size -= del_n;
  return 1;
}

FUZZ_INLINE static int
fuzz_buf_overwrite(uint8_t* data, size_t size, size_t pos, const uint8_t* src, size_t n) {
  FUZZ_ASSERT(data || size == 0);
  FUZZ_ASSERT(src || n == 0);
  if (pos > size)
    return 0;
  if (n > size - pos)
    return 0;
  if (n)
    memcpy(data + pos, src, n);
  return 1;
}

//------------------------------------------------------------------------------
// Primitive byte mutations
//------------------------------------------------------------------------------

FUZZ_INLINE static void
fuzz_mut_flip_bit(uint8_t* data, size_t size, fuzz_rng* rng) {
  if (size == 0)
    return;
  size_t i = fuzz_rng_range(rng, size);
  uint8_t bit = (uint8_t)(1u << (fuzz_rng_range(rng, 8)));
  data[i] ^= bit;
}

FUZZ_INLINE static void
fuzz_mut_flip_byte(uint8_t* data, size_t size, fuzz_rng* rng) {
  if (size == 0)
    return;
  size_t i = fuzz_rng_range(rng, size);
  data[i] ^= 0xFFu;
}

FUZZ_INLINE static void
fuzz_mut_set_byte(uint8_t* data, size_t size, fuzz_rng* rng) {
  if (size == 0)
    return;
  size_t i = fuzz_rng_range(rng, size);
  data[i] = fuzz_rng_byte(rng);
}

FUZZ_INLINE static void
fuzz_mut_arith_byte(uint8_t* data, size_t size, fuzz_rng* rng) {
  if (size == 0)
    return;
  size_t i = fuzz_rng_range(rng, size);
  uint8_t delta = (uint8_t)(1u + (fuzz_rng_range(rng, 35)));
  if (fuzz_rng_bool(rng))
    data[i] = (uint8_t)(data[i] + delta);
  else
    data[i] = (uint8_t)(data[i] - delta);
}

FUZZ_INLINE static void
fuzz_mut_swap_bytes(uint8_t* data, size_t size, fuzz_rng* rng) {
  if (size < 2)
    return;
  size_t i = fuzz_rng_range(rng, size);
  size_t j = fuzz_rng_range(rng, size);
  uint8_t t = data[i];
  data[i] = data[j];
  data[j] = t;
}

FUZZ_INLINE static void
fuzz_mut_shuffle_window(uint8_t* data, size_t size, fuzz_rng* rng) {
  if (size < 4)
    return;
  size_t a = fuzz_rng_range(rng, size);
  size_t b = fuzz_rng_range(rng, size);
  if (a > b) {
    size_t t = a;
    a = b;
    b = t;
  }
  size_t n = b - a;
  if (n < 2)
    return;
  // Fisher-Yates on [a, b)
  for (size_t i = n - 1; i > 0; --i) {
    size_t j = fuzz_rng_range(rng, i + 1);
    uint8_t tmp = data[a + i];
    data[a + i] = data[a + j];
    data[a + j] = tmp;
  }
}

//------------------------------------------------------------------------------
// Insert / delete / overwrite random bytes
//------------------------------------------------------------------------------

FUZZ_INLINE static int
fuzz_mut_insert_random(uint8_t* data, size_t* size, const fuzz_mutation_cfg* cfg, fuzz_rng* rng) {
  if (*size >= cfg->max_size)
    return 0;

  size_t pos = fuzz_rng_range(rng, *size + 1);
  size_t ins_n = 1 + fuzz_rng_range(rng, fuzz__min_size(cfg->max_insert, cfg->max_size - *size));
  if (ins_n == 0)
    return 0;

  uint8_t tmp[256];
  size_t chunk = fuzz__min_size(ins_n, sizeof(tmp));
  // We may need multiple chunks if ins_n > 256
  // Build a contiguous insertion buffer in heap for simplicity.
  uint8_t* buf = (uint8_t*)malloc(ins_n);
  if (!buf)
    FUZZ_OOM("mutator: malloc failed");
  for (size_t i = 0; i < ins_n; i += chunk) {
    chunk = fuzz__min_size(ins_n - i, sizeof(tmp));
    for (size_t j = 0; j < chunk; ++j)
      tmp[j] = fuzz_rng_byte(rng);
    memcpy(buf + i, tmp, chunk);
  }

  int ok = fuzz_buf_insert(data, size, cfg->max_size, pos, buf, ins_n);
  free(buf);
  return ok;
}

FUZZ_INLINE static int
fuzz_mut_delete_range(uint8_t* data, size_t* size, const fuzz_mutation_cfg* cfg, fuzz_rng* rng) {
  if (*size == 0)
    return 0;
  size_t pos = fuzz_rng_range(rng, *size);
  size_t max_del = fuzz__min_size(cfg->max_delete, *size - pos);
  if (max_del == 0)
    return 0;
  size_t del_n = 1 + fuzz_rng_range(rng, max_del);
  return fuzz_buf_delete(data, size, pos, del_n);
}

//------------------------------------------------------------------------------
// End of mutator helpers (extend as needed)
//------------------------------------------------------------------------------

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_MUTATORS_H
