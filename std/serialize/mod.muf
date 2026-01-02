

# /Users/vincent/Documents/Github/vitte/std/serialize/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/serialize
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.serialize as a standalone package (workspace-member compatible).
# - Expose core serialization primitives:
#     * Serde-style traits/interfaces (Serialize/Deserialize) (if your language supports)
#     * Binary codec helpers (varint, endian helpers)
#     * Text codec helpers (json-like AST kept OUT unless feature enabled)
#     * Stream-based encode/decode that works with std.io Reader/Writer (optional)
# - Keep deps minimal: std-core, std-runtime, std-mem, std-string; std-io optional.
# - Include unit tests and golden vectors.
# - Feature-gate formats (json, toml, msgpack) into separate packages if needed.
#
# Notes:
# - This is the foundational layer; heavy parsers belong in std/json, std/toml.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-serialize"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: core serialization primitives (binary codecs, serde-style interfaces)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "serialize", "deserialize", "codec", "binary", "varint", "json"]
.end

workspace
  kind = "member"
.end

target
  name = "std-serialize"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.serialize"
  .end
  entry
    module = "std.serialize.codec"
  .end
  entry
    module = "std.serialize.varint"
  .end
  entry
    module = "std.serialize.endian"
  .end
  entry
    module = "std.serialize.hex"
  .end
  entry
    module = "std.serialize.base64"
  .end
  entry
    module = "std.serialize.json"
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
    name = "std-mem"
    path = "../mem"
  .end

  dep
    name = "std-string"
    path = "../string"
  .end

  dep
    name     = "std-io"
    path     = "../io"
    optional = true
  .end

  dep
    name     = "std-collections"
    path     = "../collections"
    optional = true
  .end
.end

features
  feature
    name = "traits"
    description = "Enable Serialize/Deserialize interfaces (serde-style)."
    default = true
  .end

  feature
    name = "binary"
    description = "Enable binary codecs (varint/endian helpers)."
    default = true
  .end

  feature
    name = "hex"
    description = "Enable hex encoding/decoding."
    default = true
  .end

  feature
    name = "base64"
    description = "Enable base64 encoding/decoding."
    default = true
  .end

  feature
    name = "stream"
    description = "Enable stream-based encode/decode (requires std-io)."
    default = false
    requires = ["std-io"]
  .end

  feature
    name = "json_core"
    description = "Enable minimal JSON token/AST helpers (no full parser)."
    default = false
  .end

  feature
    name = "collections"
    description = "Enable helpers for collections types (requires std-collections)."
    default = false
    requires = ["std-collections"]
  .end

  feature
    name = "no_alloc"
    description = "Prefer caller-provided buffers; reduce allocations."
    default = false
  .end

  feature
    name = "test_vectors"
    description = "Enable extended golden vectors for codecs."
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
    run  = "muffin test std-serialize"
  .end

  script
    name = "check"
    run  = "muffin check std-serialize"
  .end
.end