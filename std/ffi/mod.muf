# /Users/vincent/Documents/Github/vitte/std/ffi/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/ffi
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.ffi as a standalone package (workspace-member compatible).
# - Expose C ABI bridge types + helpers (handles, buffers, strings).
# - Own the public headers under std/include/vitte/*.h (std_ffi.h, std_runtime.h,
#   std_sys.h) and ensure they build cleanly as C17.
# - Keep deps minimal: std-core, std-runtime.
# - Include unit tests and header compile tests.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# - Avoid non-portable compiler attributes in headers by default.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-ffi"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: FFI bridge (C ABI headers, handle/buffer helpers)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "ffi", "c-abi", "headers", "interop"]
.end

workspace
  kind = "member"
.end

target
  name = "std-ffi"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.ffi"
  .end
  entry
    module = "std.ffi.handle"
  .end
  entry
    module = "std.ffi.buf"
  .end
  entry
    module = "std.ffi.str"
  .end

  # Headers shipped by this package
  include_dir = "../include"
.end

deps
  dep
    name = "std-core"
    path = "../core"
  .end

  dep
    name = "std-runtime"
    path = "../runtime"
  .end

  dep
    name     = "std-sys"
    path     = "../sys"
    optional = true
  .end
.end

features
  feature
    name = "headers"
    description = "Install/export C headers from std/include/vitte."
    default = true
  .end

  feature
    name = "header_compile_tests"
    description = "Compile-test headers as C17/C++ for CI."
    default = true
  .end

  feature
    name = "handles"
    description = "Enable runtime handle helpers for embedding."
    default = true
  .end

  feature
    name = "sys_bridge"
    description = "Expose std.sys ABI surface through FFI when available."
    default = true
    requires = ["std-sys"]
  .end
.end

profile
  opt_level = 2
  debug = true
  warnings_as_errors = false
.end

scripts
  script
    name = "test"
    run  = "muffin test std-ffi"
  .end

  script
    name = "check"
    run  = "muffin check std-ffi"
  .end

  script
    name = "headers"
    run  = "muffin run std-ffi:headers"
  .end
.end
