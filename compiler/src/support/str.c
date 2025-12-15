#include "vittec/support/str.h"

int vittec_sv_eq(vittec_sv_t a, vittec_sv_t b) {
  if (a.len != b.len) return 0;
  for (uint64_t i = 0; i < a.len; i++) {
    if (a.data[i] != b.data[i]) return 0;
  }
  return 1;
}

/* FNV-1a 64-bit */
uint64_t vittec_hash_sv(vittec_sv_t s) {
  uint64_t h = 1469598103934665603ull;
  for (uint64_t i = 0; i < s.len; i++) {
    h ^= (unsigned char)s.data[i];
    h *= 1099511628211ull;
  }
  return h;
}
