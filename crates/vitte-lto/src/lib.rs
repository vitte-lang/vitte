#![deny(missing_docs)]
//! vitte-lto — helpers et conventions pour LTO/ThinLTO dans Vitte
//!
//! Ce crate ne configure pas directement LTO (ça se fait dans `[profile.*]` du Cargo.toml racine).
//! Il fournit :
//! - Constantes pour détecter les features activées (`thin-lto`, `fat-lto`, etc.).
//! - Fonctions de diagnostic (afficher mode LTO attendu, vérifier panic=abort, codegen-units).
//! - Helpers d'intégration CI/CD : impression formatée, badge.
//! - Fonctions pour vérifier les variables d'environnement Cargo liées au build.
//!
//! Exemple :
//! ```
//! use vitte_lto as lto;
//! println!("mode LTO: {:?}", lto::current_mode());
//! println!("{}", lto::diagnostics());
//! ```

use std::env;
use std::fmt;
use thiserror::Error;

/// Modes de LTO supportés.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LtoMode {
    /// ThinLTO (rapide, scalable).
    Thin,
    /// Fat/full LTO (optimisation maximale).
    Fat,
    /// Désactivé.
    None,
}

impl fmt::Display for LtoMode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            LtoMode::Thin => write!(f, "thin"),
            LtoMode::Fat  => write!(f, "fat"),
            LtoMode::None => write!(f, "none"),
        }
    }
}

/// Erreurs possibles de vérification.
#[derive(Debug, Error)]
pub enum LtoError {
    #[error("profil inattendu: {0}")]
    UnexpectedProfile(String),
    #[error("configuration incohérente: {0}")]
    Incoherent(String),
    #[error("variable d'environnement invalide: {0}")]
    BadEnv(String),
}

/// Détecte le mode LTO compilé via features.
pub fn current_mode() -> LtoMode {
    cfg_if::cfg_if! {
        if #[cfg(feature="thin-lto")] { LtoMode::Thin }
        else if #[cfg(feature="fat-lto")] { LtoMode::Fat }
        else { LtoMode::None }
    }
}

/// Indique si panic=abort est attendu.
pub fn expect_panic_abort() -> bool {
    cfg!(feature="panic-abort")
}

/// Indique si codegen-units=1 est attendu.
pub fn expect_codegen_units_1() -> bool {
    cfg!(feature="codegen-units-1")
}

/// Retourne un diagnostic global sous forme de chaîne multi‑ligne.
pub fn diagnostics() -> String {
    format!(
        "LTO mode: {}\npanic=abort: {}\ncodegen-units=1: {}\nCARGO_PROFILE_RELEASE_LTO={:?}",
        current_mode(),
        expect_panic_abort(),
        expect_codegen_units_1(),
        env::var("CARGO_PROFILE_RELEASE_LTO").ok(),
    )
}

/// Imprime un diagnostic formaté pour CI.
pub fn print_ci() {
    println!("::group::Vitte LTO Diagnostics");
    println!("{}", diagnostics());
    println!("::endgroup::");
}

/// Génère un badge textuel utilisable dans des rapports Markdown/CI.
pub fn badge() -> String {
    let color = match current_mode() {
        LtoMode::Thin => "blue",
        LtoMode::Fat => "green",
        LtoMode::None => "lightgrey",
    };
    format!("![LTO: {}](https://img.shields.io/badge/LTO-{}-{})", current_mode(), current_mode(), color)
}

/// Vérifie la cohérence de l'environnement de build Cargo.
pub fn check_env() -> Result<(), LtoError> {
    if let Ok(val) = env::var("CARGO_PROFILE_RELEASE_LTO") {
        let mode = current_mode();
        if val == "thin" && mode != LtoMode::Thin {
            return Err(LtoError::Incoherent("Cargo profile has thin LTO but feature not set".into()));
        }
        if val == "true" && mode != LtoMode::Fat {
            return Err(LtoError::Incoherent("Cargo profile has fat LTO but feature not set".into()));
        }
        if val != "thin" && val != "true" && val != "false" {
            return Err(LtoError::BadEnv(val));
        }
    }
    Ok(())
}

/// Vérifie plusieurs paramètres attendus (panic, codegen-units).
pub fn check_consistency() -> Result<(), LtoError> {
    if expect_panic_abort() {
        if let Ok(val) = env::var("CARGO_PROFILE_RELEASE_PANIC") {
            if val != "abort" {
                return Err(LtoError::Incoherent("panic-abort feature set but profile not abort".into()));
            }
        }
    }
    if expect_codegen_units_1() {
        if let Ok(val) = env::var("CARGO_PROFILE_RELEASE_CODEGEN_UNITS") {
            if val != "1" {
                return Err(LtoError::Incoherent("codegen-units-1 feature set but profile not 1".into()));
            }
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn mode_display() {
        let m = current_mode();
        assert!(m==LtoMode::Thin || m==LtoMode::Fat || m==LtoMode::None);
        assert!(!m.to_string().is_empty());
    }
    #[test]
    fn diag_contains() {
        let s = diagnostics();
        assert!(s.contains("LTO mode"));
    }
    #[test]
    fn badge_format() {
        let b = badge();
        assert!(b.contains("![LTO:"));
    }
}