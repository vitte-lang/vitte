#![forbid(unsafe_code)]

//! Lightweight crypto helpers used across the workspace.

pub use self::errors::Error;

mod errors {
    use thiserror::Error;

    /// Error type for the `vitte-crypto` crate.
    #[derive(Debug, Error)]
    pub enum Error {
        /// The input was invalid for the requested operation.
        #[error("Invalid input: {0}")]
        Invalid(&'static str),
        /// A verification step failed (e.g., MAC mismatch).
        #[error("Verification failed")]
        Verify,
        /// Randomness could not be obtained from the OS.
        #[error("Randomness unavailable")]
        Random,
        /// A requested feature is not supported in this build.
        #[error("Unsupported feature: {0}")]
        Unsupported(&'static str),
    }
}

use sha2::{Digest, Sha256, Sha512};

/// Compute the SHA‑256 digest of `data`.
pub fn sha256(data: &[u8]) -> [u8; 32] {
    let mut hasher = Sha256::new();
    hasher.update(data);
    let out = hasher.finalize();
    let mut arr = [0u8; 32];
    arr.copy_from_slice(&out);
    arr
}

/// Compute the SHA‑512 digest of `data`.
pub fn sha512(data: &[u8]) -> [u8; 64] {
    let mut hasher = Sha512::new();
    hasher.update(data);
    let out = hasher.finalize();
    let mut arr = [0u8; 64];
    arr.copy_from_slice(&out);
    arr
}

use hmac::{Hmac, Mac};

/// Compute HMAC‑SHA256 over `data` with the given `key`.
pub fn hmac_sha256(key: &[u8], data: &[u8]) -> [u8; 32] {
    let mut mac = Hmac::<Sha256>::new_from_slice(key).expect("HMAC key");
    mac.update(data);
    let tag = mac.finalize().into_bytes();
    let mut out = [0u8; 32];
    out.copy_from_slice(&tag);
    out
}

/// Fill `buf` with cryptographically secure random bytes from the OS.
pub fn fill_random(buf: &mut [u8]) -> Result<(), Error> {
    getrandom::getrandom(buf).map_err(|_| Error::Random)
}
