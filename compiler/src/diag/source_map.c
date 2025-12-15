#include "vittec/diag/source_map.h"
#include "vittec/support/assert.h"
#include <stdlib.h>
#include <string.h>

void vittec_sourcemap_init(vittec_source_map_t* sm) {
  sm->files = NULL;
  sm->len = 0;
  sm->cap = 0;
}

void vittec_sourcemap_free(vittec_source_map_t* sm) {
  if (sm->files) free(sm->files);
  sm->files = NULL;
  sm->len = 0;
  sm->cap = 0;
}

uint32_t vittec_sourcemap_add(vittec_source_map_t* sm, const char* path, const char* src, uint32_t len) {
  if (sm->len == sm->cap) {
    uint32_t new_cap = sm->cap ? sm->cap * 2 : 8;
    vittec_source_file_t* p = (vittec_source_file_t*)realloc(sm->files, (size_t)new_cap * sizeof(vittec_source_file_t));
    VITTEC_ASSERT(p != NULL);
    sm->files = p;
    sm->cap = new_cap;
  }
  uint32_t id = sm->len++;
  sm->files[id].path = path;
  sm->files[id].src = src;
  sm->files[id].len = len;
  return id;
}

const vittec_source_file_t* vittec_sourcemap_get(const vittec_source_map_t* sm, uint32_t file_id) {
  if (file_id >= sm->len) return NULL;
  return &sm->files[file_id];
}
