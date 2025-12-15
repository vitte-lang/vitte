#include "steel/muf/muf_validate.h"
#include <string.h>

static const char* get_kv(const muf_block* b, const char* k) {
  for (size_t i = 0; i < b->kv_len; i++) {
    if (strcmp(b->kvs[i].key, k) == 0) return b->kvs[i].val;
  }
  return NULL;
}

muf_validate_result muf_validate(const muf_program* p) {
  int has_ws = 0, has_tc = 0;
  int has_steel = 0, has_steelc = 0;

  for (size_t i = 0; i < p->len; i++) {
    const muf_block* b = &p->blocks[i];
    if (b->kind == MUF_BLOCK_WORKSPACE) has_ws = 1;
    if (b->kind == MUF_BLOCK_TOOLCHAIN) has_tc = 1;
    if (b->kind == MUF_BLOCK_PACKAGE) {
      const char* name = get_kv(b, "name");
      const char* kind = get_kv(b, "kind");
      const char* src  = get_kv(b, "src");
      if (!name || !kind || !src) return (muf_validate_result){0, "package requires: name kind src"};
      if (strcmp(name, "steel") == 0) has_steel = 1;
      if (strcmp(name, "steelc") == 0) has_steelc = 1;
    }
  }

  if (!has_ws) return (muf_validate_result){0, "missing workspace block"};
  if (!has_tc) return (muf_validate_result){0, "missing toolchain block"};
  if (!has_steel || !has_steelc) return (muf_validate_result){0, "expected packages: steel and steelc"};
  return (muf_validate_result){1, NULL};
}
