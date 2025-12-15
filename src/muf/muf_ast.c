#include "steel/muf/muf_ast.h"
#include <stdlib.h>

void muf_program_init(muf_program* p) { p->blocks = NULL; p->len = 0; p->cap = 0; }

void muf_program_free(muf_program* p) {
  if (!p) return;
  for (size_t i = 0; i < p->len; i++) free(p->blocks[i].kvs);
  free(p->blocks);
  p->blocks = NULL; p->len = p->cap = 0;
}

int muf__push_block(muf_program* p, muf_block b) {
  if (p->len + 1 > p->cap) {
    size_t nc = p->cap ? p->cap * 2 : 8;
    muf_block* nb = (muf_block*)realloc(p->blocks, nc * sizeof(muf_block));
    if (!nb) return -1;
    p->blocks = nb; p->cap = nc;
  }
  p->blocks[p->len++] = b;
  return 0;
}

int muf__push_kv(muf_block* b, const char* k, const char* v) {
  if (b->kv_len + 1 > b->kv_cap) {
    size_t nc = b->kv_cap ? b->kv_cap * 2 : 8;
    muf_kv* nk = (muf_kv*)realloc(b->kvs, nc * sizeof(muf_kv));
    if (!nk) return -1;
    b->kvs = nk; b->kv_cap = nc;
  }
  b->kvs[b->kv_len].key = k;
  b->kvs[b->kv_len].val = v;
  b->kv_len++;
  return 0;
}
