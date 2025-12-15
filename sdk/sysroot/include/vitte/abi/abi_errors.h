#ifndef VITTE_SDK_ABI_ABI_ERRORS_H
#define VITTE_SDK_ABI_ABI_ERRORS_H




#include "vitte/abi/abi_types.h"

/* Canonical error codes (stable). Extend with ranges per subsystem. */
typedef enum vitte_errc {
  VITTE_EOK = 0,

  /* generic */
  VITTE_EUNKNOWN = 1,
  VITTE_EINVAL   = 2,
  VITTE_ENOMEM   = 3,
  VITTE_EIO      = 4,
  VITTE_EUNSUP   = 5,

  /* runtime */
  VITTE_ERT_INIT_FAILED = 100,
  VITTE_ERT_SHUTDOWN_FAILED = 101
} vitte_errc_t;


#endif /* VITTE_SDK_ABI_ABI_ERRORS_H */
