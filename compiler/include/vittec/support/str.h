#ifndef VITTEC_INCLUDE_VITTEC_SUPPORT_STR_H
    #define VITTEC_INCLUDE_VITTEC_SUPPORT_STR_H

    #include <stdint.h>

    typedef struct vittec_sv {
  const char* data;
  uint64_t len;
} vittec_sv_t;

static inline vittec_sv_t vittec_sv(const char* data, uint64_t len) {
  vittec_sv_t s; s.data = data; s.len = len; return s;
}

int vittec_sv_eq(vittec_sv_t a, vittec_sv_t b);
uint64_t vittec_hash_sv(vittec_sv_t s);

    #endif /* VITTEC_INCLUDE_VITTEC_SUPPORT_STR_H */
