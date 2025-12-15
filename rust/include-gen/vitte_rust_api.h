#pragma once
#ifndef VITTE_RUST_API_H
#define VITTE_RUST_API_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VR_ABI_VERSION 1u

typedef struct vr_buf {
  uint8_t* ptr;
  size_t len;
} vr_buf;

typedef enum vr_err {
  VR_OK = 0,
  VR_EINVAL = 1,
  VR_EPARSE = 2,
  VR_EOOM = 3,
  VR_EINTERNAL = 100,
  VR_EABI_MISMATCH = 101,
} vr_err;

uint32_t vr_abi_version(void);
void vr_buf_free(vr_buf b);

vr_err vr_muf_to_json(const uint8_t* data, size_t len, vr_buf* out_json);
int vr_utf8_validate(const uint8_t* data, size_t len);
int vr_regex_is_match(const uint8_t* hay, size_t hay_len,
                      const uint8_t* pat, size_t pat_len);

#ifdef __cplusplus
}
#endif

#endif
