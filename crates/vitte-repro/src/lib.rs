#![deny(missing_docs)]
//! vitte-repro — reproductibilité des builds pour Vitte
//!
//! Fournit :
//! - Empreintes déterministes des artefacts (SHA256, XXH3).
//! - Capture d'environnement (vars, OS, arch, version).
//! - Métadonnées de build (horodatage, git si dispo).
//! - Export JSON des informations.
//!
//! Exemple :
//! ```
//! use vitte_repro as repro;
//! let meta = repro::BuildMeta::capture();
//! println!("Build fingerprint: {}", meta.fingerprint());
//! ```

use thiserror::Error;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "time")]
use time::OffsetDateTime;

#[cfg(feature = "hash")]
use sha2::{Digest, Sha256};

#[cfg(feature = "hash")]
use xxhash_rust::xxh3::xxh3_128;

/// Erreurs liées à la reproductibilité.
#[derive(Debug, Error)]
pub enum ReproError {
    /// Erreur d'entrée/sortie lors de la lecture ou de l'écriture de fichiers/flux.
    #[error("io: {0}")]
    Io(#[from] std::io::Error),

    /// Erreur de (dé)sérialisation JSON (disponible avec la feature `serde`).
    #[cfg(feature = "serde")]
    #[error("json: {0}")]
    Json(#[from] serde_json::Error),

    /// Erreur générique pour les cas non couverts.
    #[error("autre: {0}")]
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, ReproError>;

/// Métadonnées de build.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
pub struct BuildMeta {
    /// Nom d’hôte ou machine.
    pub host: String,
    /// Système d’exploitation.
    pub os: String,
    /// Architecture.
    pub arch: String,
    /// Variables d’environnement capturées.
    pub env: Vec<(String, String)>,
    /// Horodatage ISO8601.
    #[cfg(feature = "time")]
    pub timestamp: String,
    /// Commit git si dispo.
    pub git_commit: Option<String>,
}

impl BuildMeta {
    /// Capture l’état actuel de l’environnement.
    pub fn capture() -> Self {
        let host = std::env::var("HOSTNAME")
            .or_else(|_| std::env::var("COMPUTERNAME"))
            .unwrap_or_else(|_| "unknown".to_string());
        let os = std::env::consts::OS.to_string();
        let arch = std::env::consts::ARCH.to_string();
        let env: Vec<(String, String)> = std::env::vars().collect();
        let git_commit = std::env::var("GIT_COMMIT").ok();
        #[cfg(feature = "time")]
        let timestamp = OffsetDateTime::now_utc()
            .format(&time::format_description::well_known::Rfc3339)
            .unwrap();
        Self {
            host,
            os,
            arch,
            env,
            #[cfg(feature = "time")]
            timestamp,
            git_commit,
        }
    }

    /// Empreinte (SHA256 si hash activé).
    #[cfg(feature = "hash")]
    pub fn fingerprint(&self) -> String {
        let json = serde_json::to_vec(self).unwrap_or_default();
        let mut h = Sha256::new();
        h.update(&json);
        format!("{:x}", h.finalize())
    }

    /// Empreinte XXH3-128 si hash activé.
    #[cfg(feature = "hash")]
    pub fn fingerprint_xxh3(&self) -> String {
        let json = serde_json::to_vec(self).unwrap_or_default();
        format!("{:x}", xxh3_128(&json))
    }

    /// Export JSON pretty.
    #[cfg(feature = "serde")]
    pub fn to_json(&self) -> Result<String> {
        Ok(serde_json::to_string_pretty(self)?)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn capture_meta() {
        let m = BuildMeta::capture();
        assert!(!m.os.is_empty());
    }

    #[cfg(all(feature = "serde", feature = "hash"))]
    #[test]
    fn fingerprint_and_json() {
        let m = BuildMeta::capture();
        let f = m.fingerprint();
        assert!(!f.is_empty());
        let j = m.to_json().unwrap();
        assert!(j.contains("os"));
    }
}
