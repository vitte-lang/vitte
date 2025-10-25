//! Gestion du backend d'historique persistant pour le REPL.

use std::fs;
use std::io;
use std::path::{Path, PathBuf};
use thiserror::Error;

/// Configuration de l'historique.
#[derive(Debug, Clone, Default)]
pub struct HistoryConfig {
    /// Active ou désactive la persistance.
    pub enabled: bool,
    /// Emplacement explicite de l'historique.
    pub path: Option<PathBuf>,
    /// Clé de chiffrement optionnelle (XOR répétitif).
    pub encryption_key: Option<Vec<u8>>,
}

/// Erreurs liées à l'historique.
#[derive(Debug, Error)]
pub enum HistoryError {
    /// Erreur d'E/S.
    #[error("io error: {0}")]
    Io(#[from] io::Error),
    /// Sérialisation/ désérialisation JSON invalide.
    #[error("serialization error: {0}")]
    Serde(#[from] serde_json::Error),
}

/// Gestionnaire d'historique.
#[derive(Debug)]
pub struct HistoryManager {
    path: PathBuf,
    key: Option<Vec<u8>>,
    entries: Vec<String>,
}

impl HistoryManager {
    /// Construit un gestionnaire à partir d'une configuration.
    pub fn from_config(cfg: &HistoryConfig) -> Result<Option<Self>, HistoryError> {
        if !cfg.enabled {
            return Ok(None);
        }

        let path = cfg.path.clone().unwrap_or_else(default_history_path);
        let key = cfg.encryption_key.clone();

        let entries = if path.exists() {
            let mut data = fs::read(&path)?;
            if let Some(ref key) = key {
                xor_in_place(&mut data, key);
            }
            serde_json::from_slice::<Vec<String>>(&data)?
        } else {
            Vec::new()
        };

        Ok(Some(Self { path, key, entries }))
    }

    /// Enregistre une entrée et persiste immédiatement.
    pub fn record(&mut self, entry: &str) -> Result<(), HistoryError> {
        self.entries.push(entry.to_string());
        self.persist()
    }

    /// Retourne les entrées.
    pub fn entries(&self) -> &[String] {
        &self.entries
    }

    fn persist(&self) -> Result<(), HistoryError> {
        if let Some(parent) = self.path.parent() {
            fs::create_dir_all(parent)?;
        }
        let data = serde_json::to_vec(&self.entries)?;
        let mut payload = data;
        if let Some(ref key) = self.key {
            xor_in_place(&mut payload, key);
        }
        fs::write(&self.path, payload)?;
        Ok(())
    }
}

fn default_history_path() -> PathBuf {
    if let Some(home) = std::env::var_os("VITTE_HOME") {
        Path::new(&home).join("repl_history.json")
    } else if let Some(home) = std::env::var_os("HOME") {
        Path::new(&home).join(".vitte").join("repl_history.json")
    } else {
        std::env::current_dir().unwrap_or_else(|_| PathBuf::from(".")).join("repl_history.json")
    }
}

fn xor_in_place(data: &mut [u8], key: &[u8]) {
    if key.is_empty() {
        return;
    }
    for (idx, byte) in data.iter_mut().enumerate() {
        *byte ^= key[idx % key.len()];
    }
}
