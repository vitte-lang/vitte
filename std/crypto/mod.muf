# /Users/vincent/Documents/Github/vitte/std/crypto/mod.muf
# -----------------------------------------------------------------------------
# Muffin manifest for std/crypto
# -----------------------------------------------------------------------------
# MAX goals:
# - Provide std.crypto as a standalone package (workspace-member compatible).
# - Offer core cryptographic primitives with clean APIs:
#     * hashes: sha2 (sha256/sha512), sha3 (optional), blake3 (optional)
#     * mac: hmac
#     * kdf: hkdf, pbkdf2 (optional)
#     * symmetric: aead (chacha20-poly1305 optional), stream (chacha20 optional)
#     * encoding helpers: constant-time compare, hex/base64 glue (via std-encoding)
# - Keep deps minimal: std-core, std-runtime, std-string.
# - Feature-gate heavy/optional algorithms and any OS entropy backend.
# - Include unit tests.
#
# Notes:
# - This package should avoid platform syscalls by default.
# - Randomness should come from std-runtime RNG facade, optionally std-sys.
# -----------------------------------------------------------------------------

muf 1

package
  name        = "std-crypto"
  version     = "0.1.0"
  license     = "MIT"
  description = "Vitte standard library: cryptography primitives (hash/MAC/KDF/AEAD)."
  homepage    = "https://github.com/roussov/vitte"
  keywords    = ["stdlib", "crypto", "hash", "hmac", "hkdf", "aead", "sha256"]
.end

workspace
  kind = "member"
.end

target
  name = "std-crypto"
  kind = "lib"
  lang = "vitte"

  src_dir  = "src"
  test_dir = "tests"

  # Public entry modules
  entry
    module = "std.crypto"
  .end
  entry
    module = "std.crypto.hash"
  .end
  entry
    module = "std.crypto.hmac"
  .end
  entry
    module = "std.crypto.kdf"
  .end
  entry
    module = "std.crypto.aead"
  .end
  entry
    module = "std.crypto.ct"
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
    name     = "std-encoding"
    path     = "../encoding"
    optional = true
  .end

  dep
    name     = "std-sys"
    path     = "../sys"
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
    name = "sha2"
    description = "Enable SHA-2 family (sha256/sha512)."
    default = true
  .end

  feature
    name = "sha3"
    description = "Enable SHA-3 family (keccak) (optional)."
    default = false
  .end

  feature
    name = "blake3"
    description = "Enable BLAKE3 hash (optional)."
    default = false
  .end

  feature
    name = "hmac"
    description = "Enable HMAC (requires sha2 or sha3)."
    default = true
  .end

  feature
    name = "hkdf"
    description = "Enable HKDF (requires hmac)."
    default = true
  .end

  feature
    name = "pbkdf2"
    description = "Enable PBKDF2 (optional; requires hmac)."
    default = false
  .end

  feature
    name = "chacha20"
    description = "Enable ChaCha20 stream cipher (optional)."
    default = false
  .end

  feature
    name = "chacha20_poly1305"
    description = "Enable ChaCha20-Poly1305 AEAD (optional; requires chacha20)."
    default = false
    requires = ["chacha20"]
  .end

  feature
    name = "ct"
    description = "Enable constant-time utilities (memcmp, eq)."
    default = true
  .end

  feature
    name = "encoding_glue"
    description = "Enable helpers that integrate with std-encoding (hex/base64)."
    default = true
    requires = ["std-encoding"]
  .end

  feature
    name = "sys_entropy"
    description = "Allow using OS entropy backend via std-sys if available."
    default = true
    requires = ["std-sys"]
  .end

  feature
    name = "deterministic_tests"
    description = "Deterministic test vectors and fixed RNG for CI."
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
    run  = "muffin test std-crypto"
  .end

  script
    name = "check"
    run  = "muffin check std-crypto"
  .end
.end
