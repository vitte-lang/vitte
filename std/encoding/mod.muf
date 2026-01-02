

# /Users/vincent/Documents/Github/vitte/std/encoding/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/encoding
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.encoding as a standalone package (workspace-member compatible).
# - Offer text/binary encodings and codecs:
#     * base16/hex
#     * base32
#     * base64
#     * percent-encoding (URL)
#     * UTF-8 validation/normalization helpers (optional)
# - Keep deps minimal: std-core, std-runtime, std-string.
# - Include unit tests.
# - Feature-gate heavier tables/algos.
#
# Notes:
# - Align keys with your canonical Muffin EBNF if names differ.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-encoding"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: encoding/decoding utilities (base16/base32/base64/url/utf8)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "encoding", "codec", "base64", "utf8", "url"]
.end

workspace
  kind = "member"
.end

target
  name = "std-encoding"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.encoding"
  .end
  entry
    module = "std.encoding.hex"
  .end
  entry
    module = "std.encoding.base32"
  .end
  entry
    module = "std.encoding.base64"
  .end
  entry
    module = "std.encoding.url"
  .end
  entry
    module = "std.encoding.utf8"
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

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "base16"
    description = "Enable base16/hex codec."
    default = true
  .end

  feature
    name = "base32"
    description = "Enable base32 codec."
    default = true
  .end

  feature
    name = "base64"
    description = "Enable base64 codec."
    default = true
  .end

  feature
    name = "url"
    description = "Enable percent-encoding codec."
    default = true
  .end

  feature
    name = "utf8"
    description = "Enable UTF-8 validation utilities."
    default = true
  .end

  feature
    name = "tables"
    description = "Use precomputed tables for faster codecs (more binary size)."
    default = true
  .end

  feature
    name = "no_alloc"
    description = "Prefer caller-provided buffers; avoid allocations when possible."
    default = false
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
    run  = "muffin test std-encoding"
  .end

  script
    name = "check"
    run  = "muffin check std-encoding"
  .end
.end