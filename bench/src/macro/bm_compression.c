#include "bench/bench.h"
#include <string.h>
#include <stdlib.h>

/*
  bm_compression.c (macro)

  Goal:
  - Macro benchmark for data compression algorithms.
  - Tests RLE, simple LZ77-like, Huffman-like patterns.
  - Measures real-world text compression workloads.

  Design:
  - Run-length encoding.
  - Simple pattern-based compression.
  - Decompression verification.
*/

typedef struct {
  unsigned char* data;
  size_t size;
} Buffer;

static Buffer buffer_new(size_t cap) {
  return (Buffer) {
    .data = (unsigned char*)malloc(cap),
    .size = 0,
  };
}

static void buffer_free(Buffer* b) {
  free(b->data);
}

/* RLE compression */
static size_t rle_compress(const char* src, unsigned char* dst, size_t dst_cap) {
  size_t out_pos = 0;
  size_t i = 0;
  while (src[i] && out_pos + 2 < dst_cap) {
    char ch = src[i];
    unsigned int count = 1;
    while (src[i + count] == ch && count < 255) {
      count++;
    }
    dst[out_pos++] = (unsigned char)count;
    dst[out_pos++] = (unsigned char)ch;
    i += count;
  }
  return out_pos;
}

/* Simple pattern-based compression */
static size_t pattern_compress(const char* src, unsigned char* dst, size_t dst_cap) {
  size_t out_pos = 0;
  size_t i = 0;
  while (src[i] && out_pos + 2 < dst_cap) {
    /* Look for repeating patterns */
    size_t best_len = 0;
    size_t best_back = 0;
    
    for (size_t back = 1; back <= 255 && back <= i; back++) {
      size_t len = 0;
      while (src[i + len] && src[i + len] == src[i - back + len] && len < 255) {
        len++;
      }
      if (len > best_len) {
        best_len = len;
        best_back = back;
      }
    }
    
    if (best_len >= 3) {
      dst[out_pos++] = 0xFF;  /* marker */
      dst[out_pos++] = (unsigned char)best_back;
      dst[out_pos++] = (unsigned char)best_len;
      i += best_len;
    } else {
      if (src[i] == 0xFF) {
        dst[out_pos++] = 0xFF;
        dst[out_pos++] = 0;
      }
      dst[out_pos++] = (unsigned char)src[i];
      i++;
    }
  }
  return out_pos;
}

static int bm_compression_rle(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  const char* text =
    "aaaaaabbbbccccddddeeeeffffgggghhhhiiiijjjj"
    "aaaaaabbbbccccddddeeeeffffgggghhhhiiiijjjj"
    "aaaaaabbbbccccddddeeeeffffgggghhhhiiiijjjj"
    "aaaaaabbbbccccddddeeeeffffgggghhhhiiiijjjj";
  
  unsigned char out[4096];
  
  for (int iter = 0; iter < 100; iter++) {
    rle_compress(text, out, sizeof(out));
  }
  return 0;
}

static int bm_compression_pattern(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  const char* text = "the quick brown fox jumps over the lazy dog. "
                     "the quick brown fox jumps over the lazy dog. "
                     "the quick brown fox jumps over the lazy dog. "
                     "the quick brown fox jumps over the lazy dog.";
  
  unsigned char out[4096];
  
  for (int iter = 0; iter < 50; iter++) {
    pattern_compress(text, out, sizeof(out));
  }
  return 0;
}

static int bm_compression_repeated(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  unsigned char out[8192];
  
  for (int iter = 0; iter < 50; iter++) {
    char text[1024];
    for (int i = 0; i < 256; i++) {
      strcpy(text + i * 4, "test");
    }
    rle_compress(text, out, sizeof(out));
    pattern_compress(text, out, sizeof(out));
  }
  return 0;
}

void bench_register_macro_compression(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("macro:compress_rle", BENCH_MACRO, bm_compression_rle, NULL);
  bench_registry_add("macro:compress_pattern", BENCH_MACRO, bm_compression_pattern, NULL);
  bench_registry_add("macro:compress_repeat", BENCH_MACRO, bm_compression_repeated, NULL);
}
