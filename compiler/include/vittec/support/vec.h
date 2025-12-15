#ifndef VITTEC_INCLUDE_VITTEC_SUPPORT_VEC_H
    #define VITTEC_INCLUDE_VITTEC_SUPPORT_VEC_H

    #include <stdint.h>

    typedef struct vittec_vec_u32 {
  uint32_t* data;
  uint32_t len;
  uint32_t cap;
} vittec_vec_u32_t;

void vittec_vec_u32_init(vittec_vec_u32_t* v);
void vittec_vec_u32_free(vittec_vec_u32_t* v);
void vittec_vec_u32_push(vittec_vec_u32_t* v, uint32_t x);

    #endif /* VITTEC_INCLUDE_VITTEC_SUPPORT_VEC_H */
