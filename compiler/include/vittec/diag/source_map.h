#ifndef VITTEC_INCLUDE_VITTEC_DIAG_SOURCE_MAP_H
    #define VITTEC_INCLUDE_VITTEC_DIAG_SOURCE_MAP_H

    #include <stdint.h>

    #include "vittec/support/str.h"
#include "vittec/diag/span.h"

typedef struct vittec_source_file {
  const char* path;
  const char* src;
  uint32_t len;
} vittec_source_file_t;

typedef struct vittec_source_map {
  vittec_source_file_t* files;
  uint32_t len;
  uint32_t cap;
} vittec_source_map_t;

void vittec_sourcemap_init(vittec_source_map_t* sm);
void vittec_sourcemap_free(vittec_source_map_t* sm);

uint32_t vittec_sourcemap_add(vittec_source_map_t* sm, const char* path, const char* src, uint32_t len);
const vittec_source_file_t* vittec_sourcemap_get(const vittec_source_map_t* sm, uint32_t file_id);

    #endif /* VITTEC_INCLUDE_VITTEC_DIAG_SOURCE_MAP_H */
