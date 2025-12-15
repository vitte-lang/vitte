# plugins/crypto/api/kdf/mod.vitte
# plugins/crypto/api/kdf/mod.vitte
# KDF API umbrella module — MAX++ (2025)
#
# Conventions:
#   - Module index only (no logic).
#   - Uses `.end` block terminator.
#   - Re-exports submodules for a stable import surface.

mod plugins.crypto.api.kdf

# ---------------------------------------------------------------------------
# Modern KDFs
# ---------------------------------------------------------------------------

pub mod hkdf

# ---------------------------------------------------------------------------
# Password hashing KDFs (PHF)
# ---------------------------------------------------------------------------

pub mod pbkdf2
pub mod scrypt
pub mod argon2

# ---------------------------------------------------------------------------
# Future:
#   - bcrypt
#   - yescrypt
#   - balloon
#   - kdf_concat (NIST SP 800-56A)
#   - kdf_hmac (NIST SP 800-108)
#   - tls_prf (legacy)
# ---------------------------------------------------------------------------

.end
