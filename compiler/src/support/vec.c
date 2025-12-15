#include "vittec/support/vec.h"
#include "vittec/support/assert.h"
#include <stdlib.h>
#include <string.h>

void vittec_vec_u32_init(vittec_vec_u32_t* v) {
  v->data = NULL;
  v->len = 0;
  v->cap = 0;
}

void vittec_vec_u32_free(vittec_vec_u32_t* v) {
  if (v->data) free(v->data);
  v->data = NULL;
  v->len = 0;
  v->cap = 0;
}

void vittec_vec_u32_push(vittec_vec_u32_t* v, uint32_t x) {
  if (v->len == v->cap) {
    uint32_t new_cap = v->cap ? (v->cap * 2) : 16;
    uint32_t* p = (uint32_t*)realloc(v->data, (size_t)new_cap * sizeof(uint32_t));
    VITTEC_ASSERT(p != NULL);
    v->data = p;
    v->cap = new_cap;
  }
  v->data[v->len++] = x;
}
