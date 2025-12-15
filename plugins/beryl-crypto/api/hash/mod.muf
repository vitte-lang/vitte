# plugins/crypto/api/hash/mod.vitte
mod plugins.crypto.api.hash

pub mod digest
pub mod blake3
pub mod sha1_stub
pub mod sha2
pub mod sha3
pub mod ripemd_stub
pub mod xxhash_stub

# plugins/beryl/api/hash/mod.vitte
# Hash API umbrella module — MAX++ (2025)
#
# Conventions:
#   - Module index only (no logic).
#   - Uses `.end` block terminator.
#   - Re-exports submodules for a stable import surface.

mod plugins.crypto.api.hash

# ---------------------------------------------------------------------------
# Common/shared types
# ---------------------------------------------------------------------------

pub mod digest

# ---------------------------------------------------------------------------
# Legacy / compatibility
# ---------------------------------------------------------------------------

pub mod sha1

# ---------------------------------------------------------------------------
# SHA families
# ---------------------------------------------------------------------------

pub mod sha2
pub mod sha3

# ---------------------------------------------------------------------------
# Modern fast hashes / XOF
# ---------------------------------------------------------------------------

pub mod blake3

# ---------------------------------------------------------------------------
# Additional hashes
# ---------------------------------------------------------------------------

pub mod ripemd

# ---------------------------------------------------------------------------
# Non-cryptographic hashes (when needed)
# ---------------------------------------------------------------------------

pub mod xxhash_stub

# ---------------------------------------------------------------------------
# Future:
#   - md5 (legacy)
#   - sha512_t (variants)
#   - sm3, gost
#   - kangaroo_twelve, tuplehash
#   - siphash, highwayhash
#   - parallel tree hashes
# ---------------------------------------------------------------------------

.end
