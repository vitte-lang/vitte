#![deny(missing_docs)]
//! vitte-serde — utilitaires de (dé)sérialisation pour Vitte
//!
//! Fournit :
//! - Helpers JSON/TOML/YAML/CBOR/Bincode/MessagePack.
//! - Adapters pour `chrono`, `uuid`, `bytes`, `base64`.
//! - Traits simples `ToJson` / `FromJson`.
//!
//! Exemple :
//! ```
//! use vitte_serde::ToJson;
//! use serde::Serialize;
//!
//! #[derive(Serialize)]
//! struct Foo { a: i32 }
//! let s = Foo { a: 42 }.to_json_pretty().unwrap();
//! ```

use serde::{de::DeserializeOwned, Serialize};
use thiserror::Error;

#[cfg(feature = "chrono")]
pub use chrono;

#[cfg(feature = "uuid")]
pub use uuid;

#[cfg(feature = "bytes")]
pub use bytes;

#[derive(Debug, Error)]
/// Erreurs de sérialisation
pub enum SerdeError {
    #[error("JSON: {0}")]
    /// Erreur lors de la sérialisation ou désérialisation JSON.
    Json(String),
    #[error("YAML: {0}")]
    /// Erreur lors de la sérialisation ou désérialisation YAML.
    Yaml(String),
    #[error("TOML: {0}")]
    /// Erreur lors de la sérialisation ou désérialisation TOML.
    Toml(String),
    #[error("CBOR: {0}")]
    /// Erreur lors de la sérialisation ou désérialisation CBOR.
    Cbor(String),
    #[error("Bincode: {0}")]
    /// Erreur lors de la sérialisation ou désérialisation Bincode.
    Bincode(String),
    #[error("RMP: {0}")]
    /// Erreur lors de la sérialisation ou désérialisation MessagePack (rmp-serde).
    Rmp(String),
    #[error("Other: {0}")]
    /// Erreur de sérialisation/désérialisation d'un autre format.
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, SerdeError>;

/// Extension pour sérialisation JSON
pub trait ToJson {
    /// Sérialisation compacte
    fn to_json(&self) -> Result<String>;
    /// Sérialisation pretty
    fn to_json_pretty(&self) -> Result<String>;
}
impl<T: Serialize> ToJson for T {
    fn to_json(&self) -> Result<String> {
        serde_json::to_string(self).map_err(|e| SerdeError::Json(e.to_string()))
    }
    fn to_json_pretty(&self) -> Result<String> {
        serde_json::to_string_pretty(self).map_err(|e| SerdeError::Json(e.to_string()))
    }
}

/// Extension pour désérialisation JSON
pub trait FromJson: Sized {
    /// Parse depuis JSON
    fn from_json(s: &str) -> Result<Self>;
}
impl<T: DeserializeOwned> FromJson for T {
    fn from_json(s: &str) -> Result<Self> {
        serde_json::from_str(s).map_err(|e| SerdeError::Json(e.to_string()))
    }
}

/// Helpers TOML
#[cfg(feature = "toml")]
pub mod toml_format {
    use super::*;
    pub fn to_toml<T: Serialize>(v: &T) -> Result<String> {
        toml::to_string(v).map_err(|e| SerdeError::Toml(e.to_string()))
    }
    pub fn from_toml<T: DeserializeOwned>(s: &str) -> Result<T> {
        toml::from_str(s).map_err(|e| SerdeError::Toml(e.to_string()))
    }
}

/// Helpers YAML
#[cfg(feature = "yaml")]
pub mod yaml_format {
    use super::*;
    pub fn to_yaml<T: Serialize>(v: &T) -> Result<String> {
        serde_yaml::to_string(v).map_err(|e| SerdeError::Yaml(e.to_string()))
    }
    pub fn from_yaml<T: DeserializeOwned>(s: &str) -> Result<T> {
        serde_yaml::from_str(s).map_err(|e| SerdeError::Yaml(e.to_string()))
    }
}

/// Helpers CBOR
#[cfg(feature = "cbor")]
pub mod cbor_format {
    use super::*;
    pub fn to_cbor<T: Serialize>(v: &T) -> Result<Vec<u8>> {
        serde_cbor::to_vec(v).map_err(|e| SerdeError::Cbor(e.to_string()))
    }
    pub fn from_cbor<T: DeserializeOwned>(b: &[u8]) -> Result<T> {
        serde_cbor::from_slice(b).map_err(|e| SerdeError::Cbor(e.to_string()))
    }
}

/// Helpers Bincode
#[cfg(feature = "bincode")]
pub mod bincode_format {
    use super::*;
    pub fn to_bin<T: Serialize>(v: &T) -> Result<Vec<u8>> {
        bincode::serialize(v).map_err(|e| SerdeError::Bincode(e.to_string()))
    }
    pub fn from_bin<T: DeserializeOwned>(b: &[u8]) -> Result<T> {
        bincode::deserialize(b).map_err(|e| SerdeError::Bincode(e.to_string()))
    }
}

/// Helpers MessagePack (rmp-serde)
#[cfg(feature = "rmp")]
pub mod rmp_format {
    use super::*;
    pub fn to_rmp<T: Serialize>(v: &T) -> Result<Vec<u8>> {
        rmp_serde::to_vec(v).map_err(|e| SerdeError::Rmp(e.to_string()))
    }
    pub fn from_rmp<T: DeserializeOwned>(b: &[u8]) -> Result<T> {
        rmp_serde::from_slice(b).map_err(|e| SerdeError::Rmp(e.to_string()))
    }
}

/// Adapter Base64
#[cfg(feature = "base64")]
pub mod base64_adapter {
    use super::*;
    pub fn encode(data: &[u8]) -> String {
        base64::engine::general_purpose::STANDARD.encode(data)
    }
    pub fn decode(s: &str) -> Result<Vec<u8>> {
        base64::engine::general_purpose::STANDARD
            .decode(s)
            .map_err(|e| SerdeError::Other(e.to_string()))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde::{Deserialize, Serialize};

    #[derive(Serialize, Deserialize, Debug, PartialEq)]
    struct Foo {
        a: i32,
    }

    #[test]
    fn json_roundtrip() {
        let f = Foo { a: 5 };
        let j = f.to_json().unwrap();
        let g: Foo = Foo::from_json(&j).unwrap();
        assert_eq!(f, g);
    }

    #[cfg(feature = "toml")]
    #[test]
    fn toml_roundtrip() {
        let f = Foo { a: 7 };
        let t = toml_format::to_toml(&f).unwrap();
        let g: Foo = toml_format::from_toml(&t).unwrap();
        assert_eq!(f, g);
    }
}
