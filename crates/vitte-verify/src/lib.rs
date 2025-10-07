//! vitte-verify — hachages et signatures Ed25519

#[cfg(feature = "hash")]
use blake3;
#[cfg(feature = "hash")]
use sha2::{Digest, Sha256};

#[cfg(feature = "signature")]
use ed25519_dalek::{Signature, VerifyingKey};
#[cfg(feature = "signature")]
use pkcs8::spki::SubjectPublicKeyInfoRef;
#[cfg(feature = "signature")]
use x509_cert::{
    Certificate,
    der::{Decode, Encode},
};

#[derive(Debug)]
pub enum VerifyError {
    Io(std::io::Error),
    #[cfg(feature = "hash")]
    Hex(hex::FromHexError),
    #[cfg(feature = "signature")]
    Ed25519(ed25519_dalek::SignatureError),
    #[cfg(feature = "signature")]
    Pkcs8(pkcs8::Error),
    #[cfg(feature = "signature")]
    X509(x509_cert::der::Error),
    InvalidKeyLen {
        expected: usize,
        actual: usize,
    },
    BadSignature,
    UnsupportedKeyAlgorithm,
    Other(String),
}

impl core::fmt::Display for VerifyError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "{:?}", self)
    }
}

#[cfg(feature = "std")]
impl std::error::Error for VerifyError {}

impl From<std::io::Error> for VerifyError {
    fn from(e: std::io::Error) -> Self {
        Self::Io(e)
    }
}
#[cfg(feature = "hash")]
impl From<hex::FromHexError> for VerifyError {
    fn from(e: hex::FromHexError) -> Self {
        Self::Hex(e)
    }
}
#[cfg(feature = "signature")]
impl From<ed25519_dalek::SignatureError> for VerifyError {
    fn from(e: ed25519_dalek::SignatureError) -> Self {
        Self::Ed25519(e)
    }
}
#[cfg(feature = "signature")]
impl From<pkcs8::Error> for VerifyError {
    fn from(e: pkcs8::Error) -> Self {
        Self::Pkcs8(e)
    }
}
#[cfg(feature = "signature")]
impl From<x509_cert::der::Error> for VerifyError {
    fn from(e: x509_cert::der::Error) -> Self {
        Self::X509(e)
    }
}

pub type Result<T> = std::result::Result<T, VerifyError>;

/* ------------------------------ Hash utils ------------------------------ */

#[cfg(feature = "hash")]
pub fn sha256_hex(d: &[u8]) -> String {
    hex::encode(Sha256::digest(d))
}

#[cfg(feature = "hash")]
pub fn blake3_hex(d: &[u8]) -> String {
    blake3::hash(d).to_hex().to_string()
}

#[cfg(feature = "hash")]
pub fn equals_hex(d: &[u8], exp: &str) -> bool {
    let g = sha256_hex(d);
    g.len() == exp.len() && g.bytes().zip(exp.bytes()).fold(0u8, |acc, (a, b)| acc | (a ^ b)) == 0
}

/* ------------------------------ Signatures ------------------------------ */

#[cfg(feature = "signature")]
pub fn verify_ed25519_raw(pk: &[u8], msg: &[u8], sg: &[u8]) -> Result<()> {
    if pk.len() != 32 {
        return Err(VerifyError::InvalidKeyLen { expected: 32, actual: pk.len() });
    }
    if sg.len() != 64 {
        return Err(VerifyError::InvalidKeyLen { expected: 64, actual: sg.len() });
    }
    let pk_arr: [u8; 32] = pk
        .try_into()
        .map_err(|_| VerifyError::InvalidKeyLen { expected: 32, actual: pk.len() })?;
    let sg_arr: [u8; 64] = sg
        .try_into()
        .map_err(|_| VerifyError::InvalidKeyLen { expected: 64, actual: sg.len() })?;

    let key = VerifyingKey::from_bytes(&pk_arr)?;
    let sig = Signature::from_bytes(&sg_arr);
    key.verify_strict(msg, &sig).map_err(|_| VerifyError::BadSignature)
}

#[cfg(feature = "signature")]
pub fn verify_ed25519_spki(spki_der: &[u8], msg: &[u8], sig: &[u8]) -> Result<()> {
    let spki = SubjectPublicKeyInfoRef::try_from(spki_der)
        .map_err(|e| VerifyError::Other(e.to_string()))?;
    let oid = pkcs8::ObjectIdentifier::new("1.3.101.112")
        .map_err(|e| VerifyError::Other(e.to_string()))?;
    if spki.algorithm.oid != oid {
        return Err(VerifyError::UnsupportedKeyAlgorithm);
    }
    verify_ed25519_raw(spki.subject_public_key.raw_bytes(), msg, sig)
}

#[cfg(feature = "signature")]
pub fn verify_ed25519_x509(cert_der: &[u8], msg: &[u8], sig: &[u8]) -> Result<()> {
    let cert = Certificate::from_der(cert_der)?;
    let spki = cert.tbs_certificate.subject_public_key_info;
    let spki_bytes = spki.to_der().map_err(|e| VerifyError::Other(e.to_string()))?;
    verify_ed25519_spki(&spki_bytes, msg, sig)
}

/* --------------------------------- Misc -------------------------------- */

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Attestation {
    pub hash_alg: String,
    pub digest_hex: String,
    pub sig_hex: Option<String>,
}

#[cfg(feature = "serde")]
impl Attestation {
    pub fn from_bytes(d: &[u8], alg: &str) -> Self {
        let h = match alg {
            "sha256" => sha256_hex(d),
            "blake3" => blake3_hex(d),
            _ => sha256_hex(d),
        };
        Self { hash_alg: alg.into(), digest_hex: h, sig_hex: None }
    }
}

pub fn read_file(p: &std::path::Path) -> Result<Vec<u8>> {
    Ok(std::fs::read(p)?)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[cfg(feature = "hash")]
    #[test]
    fn h() {
        assert_eq!(
            sha256_hex(b""),
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
        );
    }
}
