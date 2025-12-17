// fuzz_io.h
// Small I/O helpers for fuzzing (bounded reads, mem readers) — C17.
//
// Goals:
//  - deterministic, bounded parsing utilities (no UB on short buffers)
//  - helpers for reading files into memory for standalone harnesses
//  - mem-reader for structured parsing (u8/u16/u32/u64, varints, strings)
//
// This header is portable; no platform syscalls required.
//
// Depends on: fuzz_assert.h

#ifndef VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_IO_H
#define VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_IO_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz_assert.h"

//------------------------------------------------------------------------------
// Mem reader
//------------------------------------------------------------------------------

typedef struct fuzz_reader {
  const uint8_t* p;
  const uint8_t* end;
} fuzz_reader;

FUZZ_INLINE static fuzz_reader
fuzz_reader_from(const void* data, size_t size) {
  FUZZ_ASSERT(data || size == 0);
  fuzz_reader r;
  r.p = (const uint8_t*)data;
  r.end = (const uint8_t*)data + size;
  return r;
}

FUZZ_INLINE static size_t
fuzz_reader_remaining(const fuzz_reader* r) {
  FUZZ_ASSERT(r);
  return (size_t)(r->end - r->p);
}

FUZZ_INLINE static int
fuzz_reader_eof(const fuzz_reader* r) {
  FUZZ_ASSERT(r);
  return r->p >= r->end;
}

FUZZ_INLINE static int
fuzz_reader_skip(fuzz_reader* r, size_t n) {
  FUZZ_ASSERT(r);
  if ((size_t)(r->end - r->p) < n)
    return 0;
  r->p += n;
  return 1;
}

FUZZ_INLINE static int
fuzz_reader_peek_u8(const fuzz_reader* r, uint8_t* out) {
  FUZZ_ASSERT(r);
  FUZZ_ASSERT(out);
  if (r->p >= r->end)
    return 0;
  *out = *r->p;
  return 1;
}

FUZZ_INLINE static int
fuzz_reader_read_u8(fuzz_reader* r, uint8_t* out) {
  FUZZ_ASSERT(r);
  FUZZ_ASSERT(out);
  if (r->p >= r->end)
    return 0;
  *out = *r->p++;
  return 1;
}

FUZZ_INLINE static int
fuzz_reader_read_bytes(fuzz_reader* r, void* dst, size_t n) {
  FUZZ_ASSERT(r);
  FUZZ_ASSERT(dst || n == 0);
  if ((size_t)(r->end - r->p) < n)
    return 0;
  if (n)
    memcpy(dst, r->p, n);
  r->p += n;
  return 1;
}

// Little-endian fixed-width reads (common for binary IR/bytecode fuzzing)
FUZZ_INLINE static int
fuzz_reader_read_u16_le(fuzz_reader* r, uint16_t* out) {
  FUZZ_ASSERT(out);
  uint8_t b0, b1;
  if (!fuzz_reader_read_u8(r, &b0))
    return 0;
  if (!fuzz_reader_read_u8(r, &b1))
    return 0;
  *out = (uint16_t)((uint16_t)b0 | ((uint16_t)b1 << 8));
  return 1;
}

FUZZ_INLINE static int
fuzz_reader_read_u32_le(fuzz_reader* r, uint32_t* out) {
  FUZZ_ASSERT(out);
  uint8_t b[4];
  if (!fuzz_reader_read_bytes(r, b, 4))
    return 0;
  *out = (uint32_t)(((uint32_t)b[0]) | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) |
                    ((uint32_t)b[3] << 24));
  return 1;
}

FUZZ_INLINE static int
fuzz_reader_read_u64_le(fuzz_reader* r, uint64_t* out) {
  FUZZ_ASSERT(out);
  uint8_t b[8];
  if (!fuzz_reader_read_bytes(r, b, 8))
    return 0;
  *out = ((uint64_t)b[0]) | ((uint64_t)b[1] << 8) | ((uint64_t)b[2] << 16) |
         ((uint64_t)b[3] << 24) | ((uint64_t)b[4] << 32) | ((uint64_t)b[5] << 40) |
         ((uint64_t)b[6] << 48) | ((uint64_t)b[7] << 56);
  return 1;
}

// Big-endian reads (sometimes useful for “wrong endianness” coverage)
FUZZ_INLINE static int
fuzz_reader_read_u16_be(fuzz_reader* r, uint16_t* out) {
  FUZZ_ASSERT(out);
  uint8_t b0, b1;
  if (!fuzz_reader_read_u8(r, &b0))
    return 0;
  if (!fuzz_reader_read_u8(r, &b1))
    return 0;
  *out = (uint16_t)(((uint16_t)b0 << 8) | (uint16_t)b1);
  return 1;
}

FUZZ_INLINE static int
fuzz_reader_read_u32_be(fuzz_reader* r, uint32_t* out) {
  FUZZ_ASSERT(out);
  uint8_t b[4];
  if (!fuzz_reader_read_bytes(r, b, 4))
    return 0;
  *out = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | ((uint32_t)b[3]);
  return 1;
}

FUZZ_INLINE static int
fuzz_reader_read_u64_be(fuzz_reader* r, uint64_t* out) {
  FUZZ_ASSERT(out);
  uint8_t b[8];
  if (!fuzz_reader_read_bytes(r, b, 8))
    return 0;
  *out = ((uint64_t)b[0] << 56) | ((uint64_t)b[1] << 48) | ((uint64_t)b[2] << 40) |
         ((uint64_t)b[3] << 32) | ((uint64_t)b[4] << 24) | ((uint64_t)b[5] << 16) |
         ((uint64_t)b[6] << 8) | ((uint64_t)b[7]);
  return 1;
}

//------------------------------------------------------------------------------
// Varints (LEB128-like) – bounded
//------------------------------------------------------------------------------

FUZZ_INLINE static int
fuzz_reader_read_uvar(fuzz_reader* r, uint64_t* out, unsigned max_bytes) {
  FUZZ_ASSERT(r);
  FUZZ_ASSERT(out);
  if (max_bytes == 0)
    max_bytes = 10;

  uint64_t v = 0;
  unsigned shift = 0;

  for (unsigned i = 0; i < max_bytes; ++i) {
    uint8_t b = 0;
    if (!fuzz_reader_read_u8(r, &b))
      return 0;

    v |= (uint64_t)(b & 0x7Fu) << shift;
    if ((b & 0x80u) == 0) {
      *out = v;
      return 1;
    }

    shift += 7;
    if (shift >= 64)
      return 0;
  }

  return 0;
}

FUZZ_INLINE static int
fuzz_reader_read_svar(fuzz_reader* r, int64_t* out, unsigned max_bytes) {
  FUZZ_ASSERT(r);
  FUZZ_ASSERT(out);
  if (max_bytes == 0)
    max_bytes = 10;

  uint64_t u = 0;
  if (!fuzz_reader_read_uvar(r, &u, max_bytes))
    return 0;

  // ZigZag decode
  int64_t v = (int64_t)((u >> 1) ^ (uint64_t)-(int64_t)(u & 1u));
  *out = v;
  return 1;
}

//------------------------------------------------------------------------------
// Bounded “string” reads from a fuzz buffer
//  - length-prefixed (uvar) string slices (no NUL requirement)
//------------------------------------------------------------------------------

typedef struct fuzz_bytes_view {
  const uint8_t* data;
  size_t size;
} fuzz_bytes_view;

FUZZ_INLINE static int
fuzz_reader_read_bytes_view(fuzz_reader* r, size_t n, fuzz_bytes_view* out) {
  FUZZ_ASSERT(r);
  FUZZ_ASSERT(out);
  if (fuzz_reader_remaining(r) < n)
    return 0;
  out->data = r->p;
  out->size = n;
  r->p += n;
  return 1;
}

FUZZ_INLINE static int
fuzz_reader_read_len_prefixed(fuzz_reader* r, size_t max_len, fuzz_bytes_view* out) {
  FUZZ_ASSERT(r);
  FUZZ_ASSERT(out);

  uint64_t n64 = 0;
  if (!fuzz_reader_read_uvar(r, &n64, 10))
    return 0;

  size_t n = (size_t)n64;
  if ((uint64_t)n != n64)
    return 0; // overflow
  if (n > max_len)
    return 0;

  return fuzz_reader_read_bytes_view(r, n, out);
}

//------------------------------------------------------------------------------
// File read helper (standalone tools)
//------------------------------------------------------------------------------

#ifndef FUZZ_IO_MAX_FILE
#define FUZZ_IO_MAX_FILE (64u * 1024u * 1024u) // 64 MiB
#endif

FUZZ_INLINE static uint8_t*
fuzz_io_read_file(const char* path, size_t* out_size) {
  FUZZ_ASSERT(path);
  FUZZ_ASSERT(out_size);

  *out_size = 0;

  FILE* f = fopen(path, "rb");
  if (!f)
    return NULL;

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  long sz = ftell(f);
  if (sz < 0) {
    fclose(f);
    return NULL;
  }
  if ((uint64_t)sz > (uint64_t)FUZZ_IO_MAX_FILE) {
    fclose(f);
    return NULL;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return NULL;
  }

  size_t n = (size_t)sz;
  uint8_t* buf = (uint8_t*)malloc(n ? n : 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  size_t got = 0;
  if (n)
    got = fread(buf, 1, n, f);
  fclose(f);

  if (got != n) {
    free(buf);
    return NULL;
  }

  *out_size = n;
  return buf;
}

//------------------------------------------------------------------------------
// Small utility: hex dump (for triage / debug)
//------------------------------------------------------------------------------

FUZZ_INLINE static void
fuzz_io_hexdump(FILE* out, const void* data, size_t n, size_t max_bytes) {
  FUZZ_ASSERT(out);
  FUZZ_ASSERT(data || n == 0);

  if (max_bytes && n > max_bytes)
    n = max_bytes;

  const uint8_t* p = (const uint8_t*)data;
  for (size_t i = 0; i < n; i += 16) {
    fprintf(out, "%08zx  ", i);
    for (size_t j = 0; j < 16; ++j) {
      if (i + j < n)
        fprintf(out, "%02x ", (unsigned)p[i + j]);
      else
        fputs("   ", out);
    }
    fputs(" |", out);
    for (size_t j = 0; j < 16 && i + j < n; ++j) {
      unsigned char c = (unsigned char)p[i + j];
      fputc((c >= 0x20u && c <= 0x7Eu) ? (int)c : '.', out);
    }
    fputs("|\n", out);
  }
}

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_IO_H
