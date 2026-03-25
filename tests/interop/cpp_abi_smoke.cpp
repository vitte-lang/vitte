#include "vitte/vitte_abi.hpp"

int main() {
  auto v = vitte::interop::vitte_c_abi_version();
  (void)v;

  VitteSliceI32 xs = vitte::interop::vitte_empty_slice_i32();
  xs = vitte::interop::vitte_slice_push_i32(xs, 7);

  VitteString s{ "ok", 2 };
  auto joined = vitte::interop::vitte_string_concat(s, s);
  (void)joined;
  return 0;
}
