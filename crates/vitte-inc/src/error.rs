// vitte-inc/src/error.rs
//! Erreurs unifiées pour le moteur d’incrémentalité.

use thiserror::Error;

/// Ensemble d’erreurs possibles du crate `vitte-inc`.
#[derive(Debug, Error)]
pub enum IncError {
    /// Erreur d’entrée/sortie.
    #[error("I/O error: {0}")]
    Io(#[from] std::io::Error),

    /// Erreur de sérialisation / désérialisation (bincode).
    #[error("serialization error: {0}")]
    Serde(#[from] bincode::Error),

    /// Élément introuvable ou cycle détecté.
    #[error("entity not found or invalid graph state: {0}")]
    NotFound(String),

    /// Format de snapshot incompatible ou corrompu.
    #[error("invalid snapshot format: {0}")]
    InvalidFormat(String),

    /// Erreur générique d’usage interne.
    #[error("internal error: {0}")]
    Internal(String),
}

impl IncError {
    /// Permet de créer un `IncError::Internal` rapidement.
    pub fn internal(msg: impl Into<String>) -> Self {
        IncError::Internal(msg.into())
    }

    /// Crée un `IncError::InvalidFormat`.
    pub fn invalid(msg: impl Into<String>) -> Self {
        IncError::InvalidFormat(msg.into())
    }
}
