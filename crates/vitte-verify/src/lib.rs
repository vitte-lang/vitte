

#![deny(missing_docs)]
//! vitte-verify — vérification d’intégrité et signatures pour Vitte
//!
//! Fournit :
//! - Hachages SHA256 et BLAKE3 (hex, base16).
//! - Vérification de signatures Ed25519 (clé brute 32o, PKCS#8 SPKI, X.509 cert).
//! - Petit format d’attestation JSON (optionnel `serde`).
//!
//! Exemple rapide :
//! ```
//! use vitte_verify::{sha256_hex, blake3_hex};
//! assert_eq!(sha256_hex(b"abc").len(), 64);
//! assert_eq!(blake3_hex(b"abc").len(), 64);
//! ```

use thiserror::Error;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "hash")]
use sha2::{Digest as ShaDigest, Sha256};

#[cfg(feature = "hash")]
use blake3::Hasher as Blake3Hasher;

#[cfg(feature = "signature")]
use ed25519_dalek::{Signature, VerifyingKey};

#[cfg(feature = "signature")]
use pkcs8::{spki::SubjectPublicKeyInfoRef, DecodePublicKey};

#[cfg(feature = "signature")]
use x509_cert::Certificate;

/// Erreurs de vérification
#[derive(Debug, Error)]
pub enum VerifyError {
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
    #[cfg(feature = "hash")]
    #[error("hex: {0}")]
    Hex(#[from] hex::FromHexError),
    #[cfg(feature = "signature")]
    #[error("ed25519: {0}")]
    Ed25519(#[from] ed25519_dalek::SignatureError),
    #[cfg(feature = "signature")]
    #[error("pkcs8: {0}")]
    Pkcs8(#[from] pkcs8::Error),
    #[cfg(feature = "signature")]
    #[error("x509: {0}")]
    X509(#[from] x509_cert::der::Error),
    #[error("invalid key length: expected {expected}, got {actual}")]
    InvalidKeyLen { expected: usize, actual: usize },
    #[error("signature mismatch")]
    BadSignature,
    #[error("unsupported key algorithm")]
    UnsupportedKeyAlgorithm,
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, VerifyError>;

// =====================================================================
// Hash
// =====================================================================

#[cfg(feature = "hash")]
/// SHA256 en hex minuscule
pub fn sha256_hex(data: &[u8]) -> String {
    let mut h = Sha256::new();
    h.update(data);
    let out = h.finalize();
    hex::encode(out)
}

#[cfg(feature = "hash")]
/// BLAKE3 en hex minuscule
pub fn blake3_hex(data: &[u8]) -> String {
    let mut h = Blake3Hasher::new();
    h.update(data);
    let out = h.finalize();
    out.to_hex().to_string()
}

#[cfg(feature = "hash")]
/// Compare une empreinte hex attendue (constant-time sur longueur égale)
pub fn equals_hex<const N: usize>(data: &[u8], expected_hex: &str) -> bool {
    let got = sha256_hex(data);
    // fallback simple: compare timing-insensitive en évitant early-return
    if got.len() != expected_hex.len() { return false; }
    let mut diff = 0u8;
    for (a, b) in got.as_bytes().iter().zip(expected_hex.as_bytes()) {
        diff |= a ^ b;
    }
    diff == 0
}

// =====================================================================
// Signatures Ed25519
// =====================================================================

#[cfg(feature = "signature")]
/// Vérifie une signature Ed25519 détachée avec clé publique brute 32 octets.
pub fn verify_ed25519_raw(pubkey: &[u8], msg: &[u8], sig: &[u8]) -> Result<()> {
    if pubkey.len() != 32 { return Err(VerifyError::InvalidKeyLen { expected: 32, actual: pubkey.len() }); }
    if sig.len() != 64 { return Err(VerifyError::InvalidKeyLen { expected: 64, actual: sig.len() }); }
    let key = VerifyingKey::from_bytes(pubkey.try_into().unwrap())?;
    let sig = Signature::from_bytes(sig.try_into().unwrap());
    key.verify_strict(msg, &sig).map_err(|_| VerifyError::BadSignature)
}

#[cfg(feature = "signature")]
/// Vérifie avec clé publique en PKCS#8 SPKI (DER) Ed25519.
pub fn verify_ed25519_spki(spki_der: &[u8], msg: &[u8], sig: &[u8]) -> Result<()> {
    let spki = SubjectPublicKeyInfoRef::try_from(spki_der)?;
    // OID Ed25519 = 1.3.101.112
    let oid_ed25519 = pkcs8::ObjectIdentifier::new_unwrap("1.3.101.112");
    if spki.algorithm.oid != oid_ed25519 { return Err(VerifyError::UnsupportedKeyAlgorithm); }
    let pk_bytes = spki.subject_public_key.raw_bytes();
    verify_ed25519_raw(pk_bytes, msg, sig)
}

#[cfg(feature = "signature")]
/// Vérifie avec un certificat X.509 contenant une clé Ed25519.
pub fn verify_ed25519_x509(cert_der: &[u8], msg: &[u8], sig: &[u8]) -> Result<()> {
    let cert = Certificate::from_der(cert_der)?;
    let spki = cert.tbs_certificate.subject_public_key_info.to_der()?
        .into_vec();
    verify_ed25519_spki(&spki, msg, sig)
}

// =====================================================================
// Attestation JSON (optionnel)
// =====================================================================

/// Attestation minimale d’un blob signé
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Attestation {
    /// Algorithme de hash ("sha256" ou "blake3")
    pub hash_alg: String,
    /// Empreinte hex du contenu
    pub digest_hex: String,
    /// Signature hex (Ed25519)
    pub sig_hex: Option<String>,
}

#[cfg(feature = "serde")]
impl Attestation {
    /// Construit une attestation à partir de données et d’un algo
    pub fn from_bytes(data: &[u8], alg: &str) -> Self {
        let digest_hex = match alg {
            "sha256" => sha256_hex(data),
            "blake3" => blake3_hex(data),
            _ => sha256_hex(data),
        };
        Self { hash_alg: alg.to_string(), digest_hex, sig_hex: None }
    }
}

// =====================================================================
// Utilitaires d’E/S
// =====================================================================

/// Lit un fichier en mémoire
pub fn read_file(path: &std::path::Path) -> Result<Vec<u8>> {
    Ok(std::fs::read(path)?)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[cfg(feature = "hash")]
    #[test]
    fn hash_ok() {
        assert_eq!(sha256_hex(b""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        assert_eq!(blake3_hex(b"abc").len(), 64);
    }

    #[cfg(feature = "signature")]
    #[test]
    fn ed25519_lengths() {
        let r = verify_ed25519_raw(&[0u8;31], b"m", &[0u8;64]);
        assert!(matches!(r, Err(VerifyError::InvalidKeyLen{..})));
    }
}