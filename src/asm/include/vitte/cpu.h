#pragma once
#include <stdint.h>

typedef struct vitte_cpu_features {
  uint32_t has_sse2   : 1;
  uint32_t has_avx2   : 1;
  uint32_t has_neon   : 1;
  uint32_t has_crc32c : 1;
  uint32_t reserved   : 28;
} vitte_cpu_features;

vitte_cpu_features vitte_cpu_detect(void);
