#ifndef VITTE_SDK_ABI_ABI_RESULT_H
#define VITTE_SDK_ABI_ABI_RESULT_H




#include "vitte/abi/abi_types.h"

/* A minimal Result layout: tag + payload union (ABI-stable and C-friendly). */
typedef enum vitte_result_tag {
  VITTE_RESULT_OK  = 0,
  VITTE_RESULT_ERR = 1
} vitte_result_tag_t;

typedef struct vitte_result_u32 {
  vitte_result_tag_t tag;
  union {
    vitte_u32 ok;
    vitte_u32 err; /* error codes */
  } as;
} vitte_result_u32_t;


#endif /* VITTE_SDK_ABI_ABI_RESULT_H */
