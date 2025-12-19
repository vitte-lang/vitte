#include "vittec/muf.h"

#include <assert.h>
#include <string.h>

int main(void) {
  const char* src =
      "kind \"tool\"\n"
      "name \"vittec\"\n"
      "version \"0.1.0\"\n";

  char out[4096];
  size_t out_len = 0;

  const int rc = vittec_muf_normalize(src, strlen(src), out, sizeof(out), &out_len);
  assert(rc == 0);
  assert(out_len > 0);
  assert(out[0] != '\0');
  assert(strstr(out, "kind") != NULL);
  assert(strstr(out, "name") != NULL);

  return 0;
}

