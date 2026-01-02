

# /Users/vincent/Documents/Github/vitte/std/compression/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/compression
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.compression as a standalone package (workspace-member compatible).
# - Offer compression/decompression codecs with clean streaming APIs:
#     * deflate/zlib (optional)
#     * gzip (optional)
#     * zstd (optional)
#     * lz4 (optional)
#     * run-length encoding (rle) (small, always-on)
# - Keep deps minimal: std-core, std-runtime, std-io, std-string.
# - Feature-gate heavy codecs and any native bindings.
# - Include unit tests and known test vectors.
#
# Notes:
# - Pure-Vitte implementations may exist for small codecs; heavy ones may bind
#   to host libs when feature-enabled.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-compression"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: compression codecs (rle/deflate/gzip/zstd/lz4)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "compression", "codec", "gzip", "zstd", "lz4", "deflate"]
.end

workspace
  kind = "member"
.end

target
  name = "std-compression"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.compression"
  .end
  entry
    module = "std.compression.rle"
  .end
  entry
    module = "std.compression.deflate"
  .end
  entry
    module = "std.compression.gzip"
  .end
  entry
    module = "std.compression.zstd"
  .end
  entry
    module = "std.compression.lz4"
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
    name = "std-io"
    path = "../io"
  .end

  dep
    name = "std-string"
    path = "../string"
  .end

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "rle"
    description = "Enable RLE codec (always available)."
    default = true
  .end

  feature
    name = "deflate"
    description = "Enable DEFLATE codec (zlib compatible)."
    default = false
  .end

  feature
    name = "gzip"
    description = "Enable GZIP container helpers (requires deflate)."
    default = false
    requires = ["deflate"]
  .end

  feature
    name = "zstd"
    description = "Enable Zstandard codec (may bind to host libzstd)."
    default = false
  .end

  feature
    name = "lz4"
    description = "Enable LZ4 codec (may bind to host liblz4)."
    default = false
  .end

  feature
    name = "native"
    description = "Allow binding to native host libraries for performance."
    default = true
  .end

  feature
    name = "pure"
    description = "Force pure-Vitte implementations only (no native bindings)."
    default = false
    conflicts = ["native"]
  .end

  feature
    name = "streaming"
    description = "Enable streaming encoder/decoder APIs."
    default = true
  .end

  feature
    name = "test_vectors"
    description = "Include extended test vectors (larger test time)."
    default = true
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
    run  = "muffin test std-compression"
  .end

  script
    name = "check"
    run  = "muffin check std-compression"
  .end
.end