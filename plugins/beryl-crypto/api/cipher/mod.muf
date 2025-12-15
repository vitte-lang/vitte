# plugins/crypto/api/cipher/mod.vitte
mod plugins.crypto.api.cipher

pub mod stream
pub mod block
pub mod aes
pub mod chacha20
pub mod ctr
pub mod cbc_stub

.end

# plugins/beryl/api/cipher/mod.muf
# Cipher API umbrella module — MAX++ (2025)
#
# Conventions:
#   - This file is a module index (no logic).
#   - Uses `.end` block terminator.
#   - Re-exports submodules for a stable import surface.

mod plugins.crypto.api.cipher

# ---------------------------------------------------------------------------
# Generic contracts
# ---------------------------------------------------------------------------

pub mod block
pub mod stream

# ---------------------------------------------------------------------------
# Concrete ciphers
# ---------------------------------------------------------------------------

pub mod aes
pub mod chacha20

# ---------------------------------------------------------------------------
# Modes / wrappers over `cipher.block`
# ---------------------------------------------------------------------------

pub mod ctr
pub mod cbc_stub

# ---------------------------------------------------------------------------
# Future:
#   - gcm, xts, cfb, ofb wrappers
#   - camellia, sm4, aria, twofish, serpent, blowfish providers
# ---------------------------------------------------------------------------

.end