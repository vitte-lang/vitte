#pragma once
/*
  Public Vitte SDK header — C ABI surface (FFI)
  TODO: stabilized API.
*/
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { const uint8_t* ptr; size_t len; } vitte_bytes;
typedef struct { uint8_t* ptr; size_t len; } vitte_mut_bytes;

typedef uint32_t vitte_handle;

#ifdef __cplusplus
}
#endif
