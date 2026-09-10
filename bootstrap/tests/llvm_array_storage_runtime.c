#include "vitte_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint64_t code;
  uint64_t weight;
  unsigned char fatal;
} Diagnostic;

int main(int argc, char** argv) {
  if (argc > 1) {
    if (strcmp(argv[1], "overflow-count") == 0) {
      vitte_llvm_array_alloc(UINT64_MAX, sizeof(Diagnostic));
    } else if (strcmp(argv[1], "overflow-header") == 0) {
      vitte_llvm_array_alloc(1, SIZE_MAX);
    } else if (strcmp(argv[1], "zero-size") == 0) {
      vitte_llvm_array_alloc(1, 0);
    } else if (strcmp(argv[1], "null-data") == 0) {
      vitte_llvm_array_at(NULL, 1, 0, sizeof(Diagnostic));
    } else if (strcmp(argv[1], "empty") == 0) {
      vitte_llvm_array_at(NULL, 0, 0, sizeof(Diagnostic));
    }
    return 1; /* Every case above must trap, not silently return. */
  }
  const uint64_t start = vitte_host_memory_checkpoint();
  Diagnostic* retained = vitte_llvm_array_alloc(2, sizeof(Diagnostic));
  retained[1].code = UINT64_C(4294967296);
  retained[1].fatal = 1;
  assert(vitte_llvm_array_alloc(0, sizeof(Diagnostic)) == NULL);
  assert(vitte_llvm_array_at(retained, 2, 1, sizeof(Diagnostic)) == &retained[1]);
  for (unsigned i = 0; i < 200; ++i) {
    const uint64_t checkpoint = vitte_host_memory_checkpoint();
    Diagnostic* scratch = vitte_llvm_array_alloc(8192, sizeof(Diagnostic));
    assert(scratch[8191].code == 0);
    scratch[8191].code = i;
    assert(vitte_host_memory_rewind(checkpoint) == 0);
    assert(retained[1].code == UINT64_C(4294967296));
    assert(retained[1].fatal == 1);
  }
  assert(vitte_host_memory_rewind(start) == 0);
  return 0;
}
