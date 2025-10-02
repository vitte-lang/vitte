//! vitte-crypto — cryptographic utilities for Vitte
//!
//! Scope
//! - Hashing (blake3/sha2/sha3).
//! - AEAD (aes-gcm, chacha20poly1305).
//! - KDFs (hkdf, pbkdf2, argon2).
//! - Signatures (ed25519).
//! - Key exchange (x25519).
//!
//! Optional features gate algorithms. Always constant-time when available.
//!
//! Example:
//! ```
//! use vitte_crypto::hash::blake3_hash;
//! let d = blake3_hash(b"abc");
//! assert_eq!(d.len(), 32);
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::vec::Vec;

/// Error type.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Invalid input length or params.
    #[error("invalid: {0}")]
    Invalid(&'static str),
    /// Verification failed.
    #[error("verification failed")]
    Verify,
    /// RNG missing.
    #[error("randomness unavailable")]
    Rng,
    /// Not compiled with required feature.
    #[error("unsupported feature: {0}")]
    Unsupported(&'static str),
}

/// Result alias.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/* ------------------------------- Hashes ------------------------------- */

/// Hash functions.
pub mod hash {
    use super::*;
    #[cfg(feature = "blake3")]
    pub fn blake3_hash(input: &[u8]) -> Vec<u8> {
        blake3::hash(input).as_bytes().to_vec()
    }
    #[cfg(feature = "sha2")]
    pub fn sha2_256(input: &[u8]) -> Vec<u8> {
        use sha2::{Digest, Sha256};
        Sha256::digest(input).to_vec()
    }
    #[cfg(feature = "sha3")]
    pub fn sha3_256(input: &[u8]) -> Vec<u8> {
        use sha3::{Digest, Sha3_256};
        Sha3_256::digest(input).to_vec()
    }
}

/* ------------------------------- AEADs ------------------------------- */

/// AEAD encryption/decryption.
pub mod aead {
    use super::*;
    #[cfg(feature = "aes-gcm")]
    pub fn aes_gcm_encrypt(key: &[u8], nonce: &[u8], plaintext: &[u8], aad: &[u8]) -> Result<Vec<u8>> {
        use aes_gcm::{aead::{Aead, KeyInit}, Aes256Gcm, Nonce};
        let cipher = Aes256Gcm::new_from_slice(key).map_err(|_| Error::Invalid("bad key"))?;
        cipher.encrypt(Nonce::from_slice(nonce), aes_gcm::aead::Payload { msg: plaintext, aad })
              .map_err(|_| Error::Invalid("encryption failed"))
    }
    #[cfg(feature = "aes-gcm")]
    pub fn aes_gcm_decrypt(key: &[u8], nonce: &[u8], ciphertext: &[u8], aad: &[u8]) -> Result<Vec<u8>> {
        use aes_gcm::{aead::{Aead, KeyInit}, Aes256Gcm, Nonce};
        let cipher = Aes256Gcm::new_from_slice(key).map_err(|_| Error::Invalid("bad key"))?;
        cipher.decrypt(Nonce::from_slice(nonce), aes_gcm::aead::Payload { msg: ciphertext, aad })
              .map_err(|_| Error::Verify)
    }

    #[cfg(feature = "chacha20poly1305")]
    pub fn chacha20_encrypt(key: &[u8], nonce: &[u8], plaintext: &[u8], aad: &[u8]) -> Result<Vec<u8>> {
        use chacha20poly1305::{aead::{Aead, KeyInit}, ChaCha20Poly1305, Nonce};
        let cipher = ChaCha20Poly1305::new_from_slice(key).map_err(|_| Error::Invalid("bad key"))?;
        cipher.encrypt(Nonce::from_slice(nonce), chacha20poly1305::aead::Payload { msg: plaintext, aad })
              .map_err(|_| Error::Invalid("encryption failed"))
    }
    #[cfg(feature = "chacha20poly1305")]
    pub fn chacha20_decrypt(key: &[u8], nonce: &[u8], ciphertext: &[u8], aad: &[u8]) -> Result<Vec<u8>> {
        use chacha20poly1305::{aead::{Aead, KeyInit}, ChaCha20Poly1305, Nonce};
        let cipher = ChaCha20Poly1305::new_from_slice(key).map_err(|_| Error::Invalid("bad key"))?;
        cipher.decrypt(Nonce::from_slice(nonce), chacha20poly1305::aead::Payload { msg: ciphertext, aad })
              .map_err(|_| Error::Verify)
    }
}

/* ------------------------------- KDFs ------------------------------- */

/// Key derivation functions.
pub mod kdf {
    use super::*;
    #[cfg(feature = "hkdf")]
    pub fn hkdf_sha256(salt: &[u8], ikm: &[u8], info: &[u8], len: usize) -> Result<Vec<u8>> {
        use hkdf::Hkdf;
        use sha2::Sha256;
        let hk = Hkdf::<Sha256>::new(Some(salt), ikm);
        let mut okm = vec![0u8; len];
        hk.expand(info, &mut okm).map_err(|_| Error::Invalid("hkdf expand failed"))?;
        Ok(okm)
    }

    #[cfg(feature = "pbkdf2")]
    pub fn pbkdf2_sha256(password: &[u8], salt: &[u8], rounds: u32, len: usize) -> Vec<u8> {
        use pbkdf2::pbkdf2_hmac;
        use sha2::Sha256;
        let mut dk = vec![0u8; len];
        pbkdf2_hmac::<Sha256>(password, salt, rounds, &mut dk);
        dk
    }

    #[cfg(feature = "argon2")]
    pub fn argon2id(password: &[u8], salt: &[u8], len: usize) -> Vec<u8> {
        use argon2::{Argon2, password_hash::{PasswordHasher, SaltString}};
        let saltstr = SaltString::b64_encode(salt).unwrap();
        let a2 = Argon2::default();
        let ph = a2.hash_password(password, &saltstr).unwrap();
        ph.hash.unwrap().as_bytes()[..len.min(ph.hash.unwrap().as_bytes().len())].to_vec()
    }
}

/* ----------------------------- Signatures ----------------------------- */

/// Signature algorithms.
pub mod sign {
    use super::*;
    #[cfg(feature = "ed25519")]
    pub fn ed25519_keypair(seed: &[u8]) -> Result<(Vec<u8>, Vec<u8>)> {
        use ed25519_dalek::{SigningKey, VerifyingKey, Signer};
        use rand_core::OsRng;
        let sk = if seed.len() == 32 {
            SigningKey::from_bytes(seed.try_into().unwrap())
        } else {
            SigningKey::generate(&mut OsRng)
        };
        let vk: VerifyingKey = (&sk).into();
        Ok((sk.to_bytes().to_vec(), vk.to_bytes().to_vec()))
    }
    #[cfg(feature = "ed25519")]
    pub fn ed25519_sign(sk: &[u8], msg: &[u8]) -> Result<Vec<u8>> {
        use ed25519_dalek::{SigningKey, Signer};
        let sk = SigningKey::from_bytes(sk.try_into().map_err(|_| Error::Invalid("bad sk"))?);
        Ok(sk.sign(msg).to_bytes().to_vec())
    }
    #[cfg(feature = "ed25519")]
    pub fn ed25519_verify(vk: &[u8], msg: &[u8], sig: &[u8]) -> Result<()> {
        use ed25519_dalek::{VerifyingKey, Signature, Verifier};
        let vk = VerifyingKey::from_bytes(vk.try_into().map_err(|_| Error::Invalid("bad vk"))?).map_err(|_| Error::Invalid("vk err"))?;
        let sig = Signature::from_bytes(sig.try_into().map_err(|_| Error::Invalid("bad sig"))?);
        vk.verify(msg, &sig).map_err(|_| Error::Verify)
    }
}

/* ---------------------------- Key Exchange ---------------------------- */

/// Key exchange functions.
pub mod kx {
    use super::*;
    #[cfg(feature = "x25519")]
    pub fn x25519_shared(sk: &[u8; 32], pk: &[u8; 32]) -> [u8; 32] {
        use x25519_dalek::{PublicKey, StaticSecret};
        let secret = StaticSecret::from(*sk);
        let public = PublicKey::from(*pk);
        let shared = secret.diffie_hellman(&public);
        shared.to_bytes()
    }
}

/* ------------------------------- Tests ------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;

    #[cfg(feature = "blake3")]
    #[test]
    fn hash_blake3_len() {
        let d = crate::hash::blake3_hash(b"hi");
        assert_eq!(d.len(), 32);
    }

    #[cfg(feature = "aes-gcm")]
    #[test]
    fn roundtrip_aes() {
        let key = [0u8; 32];
        let nonce = [0u8; 12];
        let msg = b"secret";
        let c = crate::aead::aes_gcm_encrypt(&key, &nonce, msg, b"").unwrap();
        let p = crate::aead::aes_gcm_decrypt(&key, &nonce, &c, b"").unwrap();
        assert_eq!(p, msg);
    }
}