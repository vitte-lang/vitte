#include "vitte_rust_api.h"

#include <assert.h>

int main(void) {
  assert(vitte_rust_api_abi_version() == VITTE_RUST_API_ABI_VERSION);
  return 0;
}
