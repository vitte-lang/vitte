#![deny(missing_docs)]
//! vitte-pgo — aides pour Profile-Guided Optimization (PGO)
//!
//! Fournit :
//! - Détection du mode PGO (instrument, use-profile, none).
//! - Construction des RUSTFLAGS pour compiler avec instrumentation ou avec profil.
//! - Helpers pour intégration CI/CD (affichage diagnostics, badges).
//! - Vérifications d'environnement (RUSTFLAGS, fichiers profil présents).
//! - Helpers pour exécuter llvm-profdata/llvm-cov si `llvm-tools` est activé.
//!
//! Exemple :
//! ```
//! use vitte_pgo as pgo;
//! println!("Instrument flags: {}", pgo::rustflags_instrument("./pgo-data"));
//! println!("Diagnostics: {}", pgo::diagnostics());
//! ```

use std::{env, path::Path};
use thiserror::Error;

/// Modes PGO possibles.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PgoMode {
    /// Compilation avec instrumentation (-Cprofile-generate).
    Instrument,
    /// Compilation avec profil (-Cprofile-use).
    Use,
    /// Aucun PGO.
    None,
}

impl PgoMode {
    /// Détecte le mode via features compilées.
    pub fn detect() -> Self {
        cfg_if::cfg_if! {
            if #[cfg(feature="instrument")] { PgoMode::Instrument }
            else if #[cfg(feature="use-profile")] { PgoMode::Use }
            else { PgoMode::None }
        }
    }
}

/// Erreurs possibles liées à PGO.
#[derive(Debug, Error)]
pub enum PgoError {
    /// Fichier de profil `.profraw`/`.profdata` introuvable ou chemin invalide.
    #[error("profil introuvable: {0}")]
    ProfileNotFound(String),
    /// Incohérence des variables d'environnement liées au PGO (ex: RUSTFLAGS).
    #[error("environnement invalide: {0}")]
    Env(String),
    /// Outils LLVM requis absents ou non exécutables (llvm-profdata/llvm-cov).
    #[error("outil llvm absent: {0}")]
    LlvmTool(String),
    /// Échec d'exécution d'une commande externe liée au PGO.
    #[error("commande échouée: {0}")]
    Command(String),
}

/// RUSTFLAGS pour instrumentation.
pub fn rustflags_instrument(dir: &str) -> String {
    format!("-Cprofile-generate={dir}")
}

/// RUSTFLAGS pour utilisation de profil.
pub fn rustflags_use(dir: &str) -> String {
    format!("-Cprofile-use={dir} -Cllvm-args=-pgo-warn-missing-function")
}

/// Vérifie qu'un fichier `.profraw` existe dans un dossier donné.
pub fn has_profraw(dir: &Path) -> bool {
    if let Ok(rd) = std::fs::read_dir(dir) {
        rd.flatten()
            .any(|e| e.path().extension().map(|x| x == "profraw").unwrap_or(false))
    } else {
        false
    }
}

/// Affiche diagnostics pour CI.
pub fn diagnostics() -> String {
    format!("PGO mode: {:?}\nRUSTFLAGS={:?}", PgoMode::detect(), env::var("RUSTFLAGS").ok(),)
}

/// Vérifie que l’environnement PGO est cohérent.
pub fn check_env() -> Result<(), PgoError> {
    if let Ok(val) = env::var("RUSTFLAGS") {
        if val.contains("profile-generate") && PgoMode::detect() != PgoMode::Instrument {
            return Err(PgoError::Env(
                "RUSTFLAGS demande profile-generate mais feature instrument non activée".into(),
            ));
        }
        if val.contains("profile-use") && PgoMode::detect() != PgoMode::Use {
            return Err(PgoError::Env(
                "RUSTFLAGS demande profile-use mais feature use-profile non activée".into(),
            ));
        }
    }
    Ok(())
}

/// Génère un badge Markdown pour CI.
pub fn badge() -> String {
    let mode = PgoMode::detect();
    let color = match mode {
        PgoMode::Instrument => "orange",
        PgoMode::Use => "green",
        PgoMode::None => "lightgrey",
    };
    format!("![PGO: {:?}](https://img.shields.io/badge/PGO-{:?}-{color})", mode, mode)
}

/// Si `llvm-tools` est activé, helpers pour lancer llvm-profdata/llvm-cov.
#[cfg(feature = "llvm-tools")]
pub mod llvm {
    use super::*;
    use std::process::Command;

    /// Fusionne plusieurs `.profraw` en un `.profdata`.
    pub fn merge_profraw(inputs: &[&str], output: &str) -> Result<(), PgoError> {
        let status = Command::new("llvm-profdata")
            .arg("merge")
            .arg("-o")
            .arg(output)
            .args(inputs)
            .status()
            .map_err(|e| PgoError::LlvmTool(e.to_string()))?;
        if !status.success() {
            return Err(PgoError::Command(format!("llvm-profdata rc={:?}", status.code())));
        }
        Ok(())
    }

    /// Lance llvm-cov show.
    pub fn show_cov(binary: &str, profdata: &str) -> Result<(), PgoError> {
        let status = Command::new("llvm-cov")
            .arg("show")
            .arg(binary)
            .arg(format!("-instr-profile={profdata}"))
            .status()
            .map_err(|e| PgoError::LlvmTool(e.to_string()))?;
        if !status.success() {
            return Err(PgoError::Command(format!("llvm-cov rc={:?}", status.code())));
        }
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::env;

    #[test]
    fn mode_detect_any() {
        let _m = PgoMode::detect();
    }

    #[test]
    fn rustflags_strings() {
        assert!(rustflags_instrument("d").contains("profile-generate"));
        assert!(rustflags_use("d").contains("profile-use"));
    }

    #[test]
    fn badge_format() {
        let b = badge();
        assert!(b.contains("PGO"));
    }

    #[test]
    fn check_env_ok() {
        env::remove_var("RUSTFLAGS");
        assert!(check_env().is_ok());
    }
}
