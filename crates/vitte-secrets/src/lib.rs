#![deny(missing_docs)]
//! vitte-secrets — gestion sécurisée de secrets pour Vitte
//!
//! Fournit :
//! - Type `Secret` avec effacement mémoire optionnel (`zeroize`).
//! - Coffre en mémoire `MemoryVault` (clé -> secret).
//! - Chiffrement de fichiers via AES-256-GCM (nonce stocké en préfixe).
//! - Sérialisation optionnelle en Base64 (feature `serde`).
//!
//! Exemple rapide :
//! ```
//! use vitte_secrets::{Secret, MemoryVault};
//! let mut vault = MemoryVault::default();
//! vault.set("db_password", Secret::from_utf8("s3cr3t"));
//! assert!(vault.get("db_password").is_some());
//! ```

use std::collections::HashMap;
use std::fmt;

#[cfg(feature = "aesgcm")]
use std::{fs, path::Path};

use thiserror::Error;

#[cfg(feature = "zeroize")]
use zeroize::Zeroize;

#[cfg(feature = "serde")]
use base64::{Engine as _, engine::general_purpose::STANDARD};
#[cfg(feature = "serde")]
use serde::{Deserialize, Deserializer, Serialize, Serializer};

#[cfg(feature = "aesgcm")]
use aes_gcm::{
    Aes256Gcm, Key, Nonce,
    aead::{Aead, KeyInit},
};

#[cfg(feature = "aesgcm")]
use rand_core::{OsRng, RngCore};

/// Taille attendue pour la clé AES-256 (octets).
#[cfg(feature = "aesgcm")]
pub const AES256_KEY_LEN: usize = 32;
/// Taille de nonce GCM (octets).
#[cfg(feature = "aesgcm")]
pub const GCM_NONCE_LEN: usize = 12;

/// Erreurs liées aux secrets.
#[derive(Debug, Error)]
pub enum SecretError {
    /// Erreur d’entrée/sortie sous-jacente.
    #[error("I/O: {0}")]
    Io(#[from] std::io::Error),

    /// Longueur de clé AES‑256 invalide (en octets).
    #[error("crypto: clé invalide (attendu {0} octets)")]
    BadKeyLen(usize),

    /// Échec du déchiffrement AES‑GCM.
    #[error("crypto: déchiffrement échoué")]
    Decrypt,

    #[cfg(feature = "serde")]
    #[error("serde: {0}")]
    /// Erreur de sérialisation/désérialisation (Base64/JSON).
    Serde(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, SecretError>;

/// Secret binaire protégé.
#[cfg_attr(feature = "zeroize", derive(Zeroize))]
#[derive(Clone, Eq, PartialEq)]
pub struct Secret(Vec<u8>);

impl Secret {
    /// Crée un secret depuis des octets.
    pub fn new(bytes: Vec<u8>) -> Self {
        Self(bytes)
    }
    /// Crée un secret UTF‑8.
    pub fn from_utf8(s: &str) -> Self {
        Self(s.as_bytes().to_vec())
    }
    /// Accès en lecture au contenu.
    pub fn expose(&self) -> &[u8] {
        &self.0
    }
    /// Consomme et renvoie les octets (effacés si `zeroize`).
    pub fn into_bytes(mut self) -> Vec<u8> {
        #[cfg(feature = "zeroize")]
        self.0.zeroize();
        std::mem::take(&mut self.0)
    }
}

impl fmt::Debug for Secret {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Secret([redacted]; {} bytes)", self.0.len())
    }
}

#[cfg(feature = "serde")]
impl Serialize for Secret {
    fn serialize<S: Serializer>(&self, s: S) -> std::result::Result<S::Ok, S::Error> {
        let b64 = STANDARD.encode(&self.0);
        s.serialize_str(&b64)
    }
}

#[cfg(feature = "serde")]
impl<'de> Deserialize<'de> for Secret {
    fn deserialize<D: Deserializer<'de>>(d: D) -> std::result::Result<Self, D::Error> {
        let s = String::deserialize(d)?;
        let bytes = STANDARD.decode(s).map_err(serde::de::Error::custom)?;
        Ok(Secret(bytes))
    }
}

/// Coffre en mémoire simple.
#[derive(Default)]
pub struct MemoryVault {
    map: HashMap<String, Secret>,
}

impl MemoryVault {
    /// Ajoute ou remplace un secret.
    pub fn set(&mut self, key: &str, value: Secret) {
        self.map.insert(key.to_string(), value);
    }
    /// Récupère un secret (référence).
    pub fn get(&self, key: &str) -> Option<&Secret> {
        self.map.get(key)
    }
    /// Retire et retourne un secret.
    pub fn take(&mut self, key: &str) -> Option<Secret> {
        self.map.remove(key)
    }
    /// Vide le coffre.
    pub fn clear(&mut self) {
        self.map.clear();
    }
    /// Nombre d’entrées.
    pub fn len(&self) -> usize {
        self.map.len()
    }
    /// Indique si vide.
    pub fn is_empty(&self) -> bool {
        self.map.is_empty()
    }
}

// ===================================== AES-256-GCM fichiers =====================================

/// Chiffre `plaintext` et écrit dans `path`. Le fichier contient `nonce || ciphertext`.
#[cfg(feature = "aesgcm")]
pub fn encrypt_file_aes256gcm<P: AsRef<Path>>(path: P, key: &[u8], plaintext: &[u8]) -> Result<()> {
    if key.len() != AES256_KEY_LEN {
        return Err(SecretError::BadKeyLen(AES256_KEY_LEN));
    }
    let key = Key::<Aes256Gcm>::from_slice(key);
    let mut nonce_bytes = [0u8; GCM_NONCE_LEN];
    OsRng.fill_bytes(&mut nonce_bytes);
    let nonce = Nonce::from_slice(&nonce_bytes);
    let cipher = Aes256Gcm::new(key);
    let mut ct = cipher.encrypt(nonce, plaintext).map_err(|_| SecretError::Decrypt)?;
    // Préfixe nonce
    let mut out = nonce_bytes.to_vec();
    out.append(&mut ct);
    fs::write(path, out)?;
    Ok(())
}

/// Lit `path` et déchiffre avec AES-256-GCM. Attend `nonce || ciphertext`.
#[cfg(feature = "aesgcm")]
pub fn decrypt_file_aes256gcm<P: AsRef<Path>>(path: P, key: &[u8]) -> Result<Vec<u8>> {
    if key.len() != AES256_KEY_LEN {
        return Err(SecretError::BadKeyLen(AES256_KEY_LEN));
    }
    let data = fs::read(path)?;
    if data.len() < GCM_NONCE_LEN {
        return Err(SecretError::Decrypt);
    }
    let (nonce_bytes, ct) = data.split_at(GCM_NONCE_LEN);
    let key = Key::<Aes256Gcm>::from_slice(key);
    let nonce = Nonce::from_slice(nonce_bytes);
    let cipher = Aes256Gcm::new(key);
    cipher.decrypt(nonce, ct).map_err(|_| SecretError::Decrypt)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn secret_debug_redacted() {
        let s = Secret::from_utf8("abc");
        let d = format!("{:?}", s);
        assert!(d.contains("redacted"));
    }

    #[cfg(feature = "aesgcm")]
    #[test]
    fn aes256_file_roundtrip() {
        let dir = tempfile::tempdir().unwrap();
        let path = dir.path().join("s.bin");
        let key = [7u8; AES256_KEY_LEN];
        encrypt_file_aes256gcm(&path, &key, b"hello").unwrap();
        let p = decrypt_file_aes256gcm(&path, &key).unwrap();
        assert_eq!(&p, b"hello");
    }

    #[cfg(all(feature = "serde", feature = "zeroize"))]
    #[test]
    fn serde_roundtrip() {
        let s = Secret::from_utf8("tok");
        let j = serde_json::to_string(&s).unwrap();
        let d: Secret = serde_json::from_str(&j).unwrap();
        assert_eq!(d.expose(), b"tok");
    }
}
