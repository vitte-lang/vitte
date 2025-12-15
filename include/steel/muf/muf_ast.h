#pragma once
#include <stddef.h>

typedef enum {
  MUF_BLOCK_WORKSPACE = 1,
  MUF_BLOCK_TOOLCHAIN = 2,
  MUF_BLOCK_PACKAGE = 3,
  MUF_BLOCK_PROFILE = 4,
} muf_block_kind;

typedef struct { const char* key; const char* val; } muf_kv;

typedef struct {
  muf_block_kind kind;
  const char* name;
  muf_kv* kvs;
  size_t kv_len;
  size_t kv_cap;
} muf_block;

typedef struct {
  muf_block* blocks;
  size_t len;
  size_t cap;
} muf_program;

void muf_program_init(muf_program* p);
void muf_program_free(muf_program* p);
