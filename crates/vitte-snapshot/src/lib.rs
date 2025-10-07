

#![deny(missing_docs)]
//! vitte-snapshot — gestion de snapshots déterministes pour Vitte
//!
//! Fournit :
//! - Capture d’état sérialisable via `serde` (optionnel).
//! - Sauvegarde/chargement JSON stable.
//! - Diff textuel optionnel (feature `text-diff`).
//! - Hash SHA256 + horodatage (optionnels).
//! - Compression Zstd optionnelle.
//!
//! Exemple :
//! ```
//! use vitte_snapshot::Snapshot;
//! use serde::{Serialize, Deserialize};
//!
//! #[derive(Serialize, Deserialize)]
//! struct Foo { a: i32 }
//!
//! let snap = Snapshot::capture(&Foo { a: 42 }).unwrap();
//! assert!(snap.to_json().unwrap().contains("42"));
//! ```

use thiserror::Error;

#[cfg(feature = "serde")]
use serde::Serialize;

#[cfg(feature = "time")]
use time::OffsetDateTime;

#[cfg(feature = "hash")]
use sha2::{Sha256, Digest};

#[cfg(feature = "hash")]
use base64::engine::general_purpose::STANDARD as B64;
#[cfg(feature = "hash")]
use base64::Engine;

#[cfg(feature = "text-diff")]
use similar::TextDiff;

/// Erreurs snapshot
#[derive(Debug, Error)]
pub enum SnapshotError {
    /// Erreur d'entrée/sortie sous-jacente (lecture/écriture de fichier).
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
    /// Erreur de sérialisation/désérialisation JSON via 
    /// 
    /// Présente uniquement si la feature `serde` est activée.
    #[error("serde: {0}")]
    #[cfg(feature = "serde")]
    Serde(String),
    /// Erreur liée à la compression/décompression Zstandard (zstd).
    /// 
    /// Présente uniquement si la feature `zstd` est activée.
    #[error("compress: {0}")]
    #[cfg(feature = "zstd")]
    Zstd(String),
    /// Catégorie fourre-tout pour les erreurs non classées.
    #[error("other: {0}")]
    Other(String),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, SnapshotError>;

/// Snapshot générique
#[cfg_attr(feature = "serde", derive(Serialize, serde::Deserialize))]
#[derive(Debug, Clone)]
pub struct Snapshot {
    /// Contenu JSON stable
    #[cfg(feature = "serde")]
    pub json: String,
    /// Horodatage
    #[cfg(feature = "time")]
    pub timestamp: String,
    /// Hash SHA256 du JSON
    #[cfg(feature = "hash")]
    pub hash: String,
}

impl Snapshot {
    /// Capture un snapshot sérialisé depuis une valeur
    #[cfg(feature = "serde")]
    pub fn capture<T: Serialize>(value: &T) -> Result<Self> {
        let json = serde_json::to_string_pretty(value)
            .map_err(|e| SnapshotError::Serde(e.to_string()))?;
        #[cfg(feature = "time")]
        let timestamp = OffsetDateTime::now_utc().format(&time::format_description::well_known::Rfc3339).unwrap();

        #[cfg(feature = "hash")]
        let hash = {
            let mut h = Sha256::new();
            h.update(&json);
            B64.encode(h.finalize())
        };

        Ok(Self {
            json,
            #[cfg(feature = "time")]
            timestamp,
            #[cfg(feature = "hash")]
            hash,
        })
    }

    /// Retourne JSON du snapshot
    #[cfg(feature = "serde")]
    pub fn to_json(&self) -> Result<&str> { Ok(&self.json) }

    /// Sauvegarde vers fichier (optionnellement compressé)
    pub fn save_to_file(&self, path: &std::path::Path) -> Result<()> {
        use std::fs::File;
        use std::io::Write;
        let mut f = File::create(path)?;
        #[cfg(feature = "zstd")]
        {
            let mut enc = zstd::Encoder::new(&mut f, 3).map_err(|e| SnapshotError::Zstd(e.to_string()))?;
            #[cfg(feature = "serde")]
            enc.write_all(self.json.as_bytes())?;
            enc.finish().map_err(|e| SnapshotError::Zstd(e.to_string()))?;
            return Ok(());
        }
        #[cfg(feature = "serde")]
        f.write_all(self.json.as_bytes())?;
        Ok(())
    }

    /// Charge depuis fichier
    #[cfg(feature = "serde")]
    pub fn load_from_file(path: &std::path::Path) -> Result<Self> {
        use std::fs;
        let data = fs::read(path)?;
        #[cfg(feature = "zstd")]
        let data = {
            zstd::decode_all(&data[..]).map_err(|e| SnapshotError::Zstd(e.to_string()))?
        };
        let json = String::from_utf8_lossy(&data).to_string();
        let value: serde_json::Value =
            serde_json::from_str(&json).map_err(|e| SnapshotError::Serde(e.to_string()))?;
        Self::capture(&value)
    }

    /// Calcule diff textuel entre deux snapshots
    #[cfg(all(feature = "serde", feature = "text-diff"))]
    pub fn diff(&self, other: &Self) -> String {
        let diff = TextDiff::from_lines(&self.json, &other.json);
        diff.unified_diff().to_string()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[cfg(feature = "serde")]
    #[test]
    fn json_roundtrip() {
        let v = vec![1,2,3];
        let s = Snapshot::capture(&v).unwrap();
        assert!(s.to_json().unwrap().contains("1"));
    }
}