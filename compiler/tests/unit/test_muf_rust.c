#include "vittec/muf.h"
#include "vitte_rust_api.h"

#include <assert.h>
#include <string.h>

int main(void) {
  assert(vitte_rust_api_abi_version() == VITTE_RUST_API_ABI_VERSION);

  const char* src =
      "kind \"tool\"\n"
      "name \"vittec\"\n"
      "version \"0.1.0\"\n";

  char out[4096];
  size_t out_len = 0;

  const vitte_err_code_t rc = vittec_muf_normalize(src, strlen(src), out, sizeof(out), &out_len);
  assert(rc == VITTE_ERR_OK);
  assert(out_len > 0);
  assert(out[0] != '\0');
  assert(strstr(out, "kind") != NULL);
  assert(strstr(out, "name") != NULL);

  return 0;
}

