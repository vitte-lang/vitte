#include "steel/common/vec.h"
#include <stdlib.h>
#include <string.h>

static int grow(void** p, size_t* cap, size_t elem_size, size_t need) {
  if (*cap >= need) return 0;
  size_t ncap = (*cap == 0) ? 16 : (*cap * 2);
  while (ncap < need) ncap *= 2;
  void* np = realloc(*p, ncap * elem_size);
  if (!np) return -1;
  *p = np;
  *cap = ncap;
  return 0;
}

void steel_vec_u8_init(steel_vec_u8* v) { v->data = NULL; v->len = 0; v->cap = 0; }
void steel_vec_u8_free(steel_vec_u8* v) { free(v->data); v->data = NULL; v->len = v->cap = 0; }
int steel_vec_u8_reserve(steel_vec_u8* v, size_t cap) { return grow((void**)&v->data, &v->cap, 1, cap); }
int steel_vec_u8_push(steel_vec_u8* v, uint8_t x) {
  if (grow((void**)&v->data, &v->cap, 1, v->len + 1) != 0) return -1;
  v->data[v->len++] = x;
  return 0;
}

void steel_vec_char_init(steel_vec_char* v) { v->data = NULL; v->len = 0; v->cap = 0; }
void steel_vec_char_free(steel_vec_char* v) { free(v->data); v->data = NULL; v->len = v->cap = 0; }
int steel_vec_char_reserve(steel_vec_char* v, size_t cap) { return grow((void**)&v->data, &v->cap, 1, cap); }
int steel_vec_char_push(steel_vec_char* v, char c) {
  if (grow((void**)&v->data, &v->cap, 1, v->len + 1) != 0) return -1;
  v->data[v->len++] = c;
  return 0;
}
int steel_vec_char_append(steel_vec_char* v, const char* s) {
  if (!s) return 0;
  size_t n = 0; while (s[n]) n++;
  if (grow((void**)&v->data, &v->cap, 1, v->len + n) != 0) return -1;
  memcpy(v->data + v->len, s, n);
  v->len += n;
  return 0;
}
