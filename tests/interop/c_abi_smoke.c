#include "vitte/vitte_abi.h"

int main(void) {
  const char* version = vitte_c_abi_version();
  (void)version;

  VitteSliceI32 xs = vitte_empty_slice_i32();
  xs = vitte_slice_push_i32(xs, 42);

  VitteString hi = { "hi", 2 };
  VitteString out = vitte_string_concat(hi, hi);
  (void)out;

  VitteSliceString args = cli_args();
  (void)args;
  return 0;
}
