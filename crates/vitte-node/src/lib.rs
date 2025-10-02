

#![deny(missing_docs)]
//! vitte-node — Interop Node.js pour Vitte (N-API)
//!
//! Fournit :
//! - Erreurs typées et `Result` commun.
//! - Exemples d’exports N-API (`greet`, `to_json`, `from_json`).
//! - Aides de conversion et vérifications minimales.
//!
//! # Features
//! - `napi`  : active N-API via `napi`.
//! - `serde` : conversions JSON optionnelles.

use thiserror::Error;

/// Erreurs Node interop.
#[derive(Debug, Error)]
pub enum NodeError {
    #[error("interop error: {0}")] Interop(String),
    #[error("conversion error: {0}")] Conversion(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, NodeError>;

/// API N-API (activée avec `napi`).
#[cfg(feature = "napi")]
pub mod napi_api {
    use super::*;
    use napi::bindgen_prelude::*;
    use napi_derive::napi;

    /// Renvoie un message de salutation.
    #[napi]
    pub fn greet(name: String) -> String {
        format!("Hello, {name} from Vitte/Node")
    }

    /// Sérialise une valeur JSON (string -> string) pour démonstration.
    #[napi]
    pub fn to_json(value: String) -> String {
        #[cfg(feature="serde")]
        { serde_json::to_string(&value).unwrap_or_default() }
        #[cfg(not(feature="serde"))]
        { value }
    }

    /// Désérialise une chaîne JSON vers chaîne Rust.
    #[napi]
    pub fn from_json(json: String) -> String {
        #[cfg(feature="serde")]
        { serde_json::from_str::<String>(&json).unwrap_or_default() }
        #[cfg(not(feature="serde"))]
        { json }
    }

    /// Addition simple pour tests.
    #[napi]
    pub fn add(a: i32, b: i32) -> i32 { a + b }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_fmt() {
        let e = NodeError::Conversion("bad".into());
        assert!(format!("{e}").contains("bad"));
    }
}