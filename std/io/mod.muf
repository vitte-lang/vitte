# /Users/vincent/Documents/Github/vitte/std/io/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/io
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide a clean, standalone std.io package that can be pulled as a dep.
# - Keep deps minimal (core/runtime/string/collections as needed).
# - Expose modules: reader, writer, buffered.
# - Include unit tests.
#
# NOTE:
# - This is a manifest-only description. The exact Muffin schema may differ in
#   your repo; align keys with your canonical Muffin EBNF.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-io"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: I/O interfaces and buffered adapters."
  homepage    = "https://github.com/roussov/vitte"
.end

workspace
  # Allow building inside the std workspace or standalone
  kind = "member"
.end

target
  name = "std-io"
  kind = "lib"
  lang = "vitte"

  src_dir = "src"
  test_dir = "tests"

  # Public entry modules (re-exported)
  entry
    module = "std.io.reader"
  .end
  entry
    module = "std.io.writer"
  .end
  entry
    module = "std.io.buffered"
  .end
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
    name = "std-string"
    path = "../string"
  .end

  # Optional if you replace local ByteBuf with std.collections Vec<u8>
  dep
    name = "std-collections"
    path = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "io_test_backend_real"
    description = "Enable behavioral IO tests once runtime handle backend exists."
    default = false
  .end
.end

profile
  # conservative defaults; override at workspace root if needed
  opt_level = 2
  debug = true
  warnings_as_errors = false
.end

scripts
  script
    name = "test"
    run  = "muffin test std-io"
  .end
.end