#ifndef VITTE_SDK_ABI_ABI_TYPES_H
#define VITTE_SDK_ABI_ABI_TYPES_H




#include <stdint.h>
#include <stddef.h>

typedef uint8_t  vitte_u8;
typedef uint16_t vitte_u16;
typedef uint32_t vitte_u32;
typedef uint64_t vitte_u64;
typedef int8_t   vitte_i8;
typedef int16_t  vitte_i16;
typedef int32_t  vitte_i32;
typedef int64_t  vitte_i64;

typedef float    vitte_f32;
typedef double   vitte_f64;

typedef uint8_t  vitte_bool; /* 0/1 */

typedef struct vitte_slice_u8 {
  const vitte_u8* ptr;
  vitte_u64 len;
} vitte_slice_u8_t;

typedef struct vitte_mut_slice_u8 {
  vitte_u8* ptr;
  vitte_u64 len;
} vitte_mut_slice_u8_t;

/* UTF-8 bytes (not NUL-terminated) */
typedef struct vitte_string {
  const vitte_u8* ptr;
  vitte_u64 len;
} vitte_string_t;


#endif /* VITTE_SDK_ABI_ABI_TYPES_H */
