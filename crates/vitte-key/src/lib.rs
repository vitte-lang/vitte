#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]
//! vitte-key — utilitaires génériques de clés et empreintes
//!
//! Cible: `no_std` possible (désactiver `std`, `base64`, `base58`, `pem`, `der`).
//!
//! Fonctions principales:
//! - Génération aléatoire (feature `rand`)
//! - Encodage / décodage (Hex toujours, Base64 et Base58 optionnels)
//! - PEM / DER optionnels
//! - Empreintes SHA‑2 / SHA‑3 (feature `hash`)
//! - API unifiée `Key` + `KeyAlg` + `Fingerprint`
//!
//! Ce crate n’implémente **aucun chiffrement ni KDF**. Usage: formatage, transport, fingerprint.

extern crate alloc;

use alloc::string::{String, ToString};
use alloc::vec::Vec;
use core::fmt;

#[cfg(feature = "errors")]
use thiserror::Error;

/// Erreurs.
#[cfg_attr(feature = "errors", derive(Error))]
#[derive(Debug)]
pub enum KeyError {
    /// Encodage invalide.
    #[cfg_attr(feature = "errors", error("invalid encoding: {0}"))]
    InvalidEncoding(String),
    /// Format non pris en charge.
    #[cfg_attr(feature = "errors", error("unsupported format: {0}"))]
    UnsupportedFormat(String),
    /// Erreur d'aléa.
    #[cfg_attr(feature = "errors", error("random generation error"))]
    Random,
    /// Erreur de hachage.
    #[cfg_attr(feature = "errors", error("hashing error"))]
    Hash,
    /// Taille incorrecte.
    #[cfg_attr(feature = "errors", error("invalid length: expected {expected}, got {got}"))]
    InvalidLength {
        /// Taille attendue en octets.
        expected: usize,
        /// Taille observée en octets.
        got: usize,
    },
}

/// Résultat spécialisé.
pub type Result<T> = core::result::Result<T, KeyError>;

/// Algorithme ou nature d'une clé.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum KeyAlg {
    /// Matériau opaque, octets bruts.
    Raw,
    /// Identifiant générique (ex: UUID, hash…).
    Id,
    /// Réservé pour futures paires (Ed25519, etc.). Non utilisé sans dépendances crypto.
    Reserved(u16),
}

/// Encodages pris en charge.
#[derive(Clone, Copy, Debug)]
pub enum Encoding {
    /// Hexadécimal minuscule.
    Hex,
    /// Base64 (RFC4648, feature `base64`).
    Base64,
    /// Base58 (Bitcoin, feature `base58`).
    Base58,
}

impl fmt::Display for Encoding {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Encoding::Hex => write!(f, "hex"),
            Encoding::Base64 => write!(f, "base64"),
            Encoding::Base58 => write!(f, "base58"),
        }
    }
}

/// Conteneur simple de clé.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Key {
    /// Algorithme ou nature.
    pub alg: KeyAlg,
    /// Octets.
    pub bytes: Vec<u8>,
}

impl Key {
    /// Crée un `Key` à partir d'octets.
    pub fn new(alg: KeyAlg, bytes: Vec<u8>) -> Self {
        Self { alg, bytes }
    }

    /// Longueur en octets.
    pub fn len(&self) -> usize {
        self.bytes.len()
    }

    /// Vrai si vide.
    pub fn is_empty(&self) -> bool {
        self.bytes.is_empty()
    }

    /// Encode la clé.
    pub fn encode(&self, enc: Encoding) -> String {
        encode(&self.bytes, enc)
    }

    /// Empreinte en hex selon l'algo choisi.
    #[cfg(feature = "hash")]
    pub fn fingerprint_hex(&self, algo: Fingerprint) -> String {
        fingerprint_hex(&self.bytes, algo)
    }
}

/// Empreinte supportée.
#[cfg(feature = "hash")]
#[derive(Clone, Copy, Debug)]
pub enum Fingerprint {
    /// SHA‑256.
    Sha256,
    /// SHA3‑256.
    Sha3_256,
}

#[cfg(feature = "hash")]
/// Empreinte binaire.
pub fn fingerprint(data: &[u8], algo: Fingerprint) -> Vec<u8> {
    match algo {
        Fingerprint::Sha256 => {
            use sha2::{Digest, Sha256};
            let mut h = Sha256::new();
            h.update(data);
            h.finalize().to_vec()
        }
        Fingerprint::Sha3_256 => {
            use sha3::{Digest, Sha3_256};
            let mut h = Sha3_256::new();
            h.update(data);
            h.finalize().to_vec()
        }
    }
}

#[cfg(feature = "hash")]
/// Empreinte hexadécimale.
pub fn fingerprint_hex(data: &[u8], algo: Fingerprint) -> String {
    let bin = fingerprint(data, algo);
    encode(&bin, Encoding::Hex)
}

/// Génération de bytes aléatoires.
#[cfg(feature = "rand")]
pub fn generate(len: usize) -> Result<Vec<u8>> {
    use rand::RngCore;
    let mut buf = vec![0u8; len];
    rand::thread_rng().try_fill_bytes(&mut buf).map_err(|_| KeyError::Random)?;
    Ok(buf)
}

/// Encode des octets selon un encodage.
pub fn encode(data: &[u8], enc: Encoding) -> String {
    match enc {
        Encoding::Hex => hex_encode(data),
        #[cfg(feature = "base64")]
        Encoding::Base64 => base64::encode(data),
        #[cfg(feature = "base58")]
        Encoding::Base58 => bs58::encode(data).into_string(),
        #[allow(unreachable_patterns)]
        _ => hex_encode(data),
    }
}

/// Décode une chaîne selon un encodage.
pub fn decode(s: &str, enc: Encoding) -> Result<Vec<u8>> {
    match enc {
        Encoding::Hex => {
            #[cfg(feature = "std")]
            {
                hex::decode(s).map_err(|e| KeyError::InvalidEncoding(e.to_string()))
            }
            #[cfg(not(feature = "std"))]
            {
                hex_decode_nostd(s)
            }
        }
        #[cfg(feature = "base64")]
        Encoding::Base64 => base64::decode(s).map_err(|e| KeyError::InvalidEncoding(e.to_string())),
        #[cfg(feature = "base58")]
        Encoding::Base58 => {
            bs58::decode(s).into_vec().map_err(|e| KeyError::InvalidEncoding(e.to_string()))
        }
        #[allow(unreachable_patterns)]
        _ => Err(KeyError::UnsupportedFormat(enc.to_string())),
    }
}

/// HEX encode sans std, sans allocations inutiles côté formatage.
fn hex_encode(data: &[u8]) -> String {
    const HEX: &[u8; 16] = b"0123456789abcdef";
    let mut s = String::with_capacity(data.len() * 2);
    for &b in data {
        s.push(HEX[(b >> 4) as usize] as char);
        s.push(HEX[(b & 0x0f) as usize] as char);
    }
    s
}

/// HEX decode sans dépendre de `hex` si `std` est off.
#[cfg(not(feature = "std"))]
fn hex_decode_nostd(s: &str) -> Result<Vec<u8>> {
    fn val(c: u8) -> Result<u8> {
        match c {
            b'0'..=b'9' => Ok(c - b'0'),
            b'a'..=b'f' => Ok(10 + (c - b'a')),
            b'A'..=b'F' => Ok(10 + (c - b'A')),
            _ => Err(KeyError::InvalidEncoding("non-hex char".into())),
        }
    }
    let b = s.as_bytes();
    if b.len() % 2 != 0 {
        return Err(KeyError::InvalidEncoding("odd length".into()));
    }
    let mut out = Vec::with_capacity(b.len() / 2);
    let mut i = 0;
    while i < b.len() {
        let hi = val(b[i])?;
        let lo = val(b[i + 1])?;
        out.push((hi << 4) | lo);
        i += 2;
    }
    Ok(out)
}

/// Encapsule en PEM.
#[cfg(feature = "pem")]
pub fn to_pem(label: &str, data: &[u8]) -> Result<String> {
    let block = pem::Pem { tag: label.to_string(), contents: data.to_vec() };
    Ok(pem::encode(&block))
}

/// Extrait d'un PEM.
#[cfg(feature = "pem")]
pub fn from_pem(s: &str) -> Result<(String, Vec<u8>)> {
    let block = pem::parse(s).map_err(|e| KeyError::InvalidEncoding(e.to_string()))?;
    Ok((block.tag, block.contents))
}

/// Encapsulation DER brute (pass‑through).
#[cfg(feature = "der")]
pub fn to_der(data: &[u8]) -> Vec<u8> {
    data.to_vec()
}

/// Lecture DER brute (pass‑through).
#[cfg(feature = "der")]
pub fn from_der(bytes: &[u8]) -> Vec<u8> {
    bytes.to_vec()
}

/// Aide: crée une `Key` brute à partir d’une chaîne encodée.
pub fn key_from_str(alg: KeyAlg, s: &str, enc: Encoding) -> Result<Key> {
    Ok(Key::new(alg, decode(s, enc)?))
}

/// Aide: crée une chaîne encodée depuis une `Key`.
pub fn key_to_str(key: &Key, enc: Encoding) -> String {
    key.encode(enc)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hex_roundtrip() {
        let data = [0x00, 0x01, 0x10, 0xfe, 0xff];
        let s = encode(&data, Encoding::Hex);
        let back = decode(&s, Encoding::Hex).unwrap();
        assert_eq!(data.to_vec(), back);
    }

    #[cfg(feature = "base64")]
    #[test]
    fn b64_roundtrip() {
        let data = b"hello world";
        let s = encode(data, Encoding::Base64);
        let back = decode(&s, Encoding::Base64).unwrap();
        assert_eq!(data.to_vec(), back);
    }

    #[cfg(feature = "base58")]
    #[test]
    fn b58_roundtrip() {
        let data = b"\x01\x02\x03\x04\xff";
        let s = encode(data, Encoding::Base58);
        let back = decode(&s, Encoding::Base58).unwrap();
        assert_eq!(data.to_vec(), back);
    }

    #[cfg(feature = "pem")]
    #[test]
    fn pem_cycle() {
        let data = b"\x01\x02\x03";
        let p = to_pem("TEST", data).unwrap();
        let (tag, bytes) = from_pem(&p).unwrap();
        assert_eq!(tag, "TEST");
        assert_eq!(bytes, data);
    }

    #[cfg(feature = "hash")]
    #[test]
    fn fp_sizes() {
        let d = b"abc";
        let a = fingerprint(d, Fingerprint::Sha256);
        let b = fingerprint(d, Fingerprint::Sha3_256);
        assert_eq!(a.len(), 32);
        assert_eq!(b.len(), 32);
        let hx = fingerprint_hex(d, Fingerprint::Sha256);
        assert_eq!(hx.len(), 64);
    }

    #[cfg(feature = "rand")]
    #[test]
    fn random_len() {
        let k = generate(32).unwrap();
        assert_eq!(k.len(), 32);
    }

    #[test]
    fn key_helpers() {
        let k = Key::new(KeyAlg::Raw, vec![1, 2, 3]);
        let s = key_to_str(&k, Encoding::Hex);
        let k2 = key_from_str(KeyAlg::Raw, &s, Encoding::Hex).unwrap();
        assert_eq!(k.bytes, k2.bytes);
    }
}
