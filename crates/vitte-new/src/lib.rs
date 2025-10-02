

#![deny(missing_docs)]
//! vitte-new — bibliothèque interne pour l’outil de scaffolding Vitte
//!
//! Fournit :
//! - Fonctions pour générer un nouveau crate/module.
//! - Gestion des templates intégrés.
//! - Manipulation de Cargo.toml via `toml_edit`.
//! - Helpers de fichiers et dossiers.
//!
//! Utilisé par le binaire `vitte-new`.

use std::path::{Path, PathBuf};
use anyhow::{Result, Context};
use fs_err as fs;
use toml_edit::{DocumentMut, value};

/// Crée un nouveau crate dans le dossier `dir` avec nom `name`.
pub fn create_crate(dir: &Path, name: &str) -> Result<PathBuf> {
    let crate_dir = dir.join(name);
    if crate_dir.exists() {
        anyhow::bail!("destination already exists: {}", crate_dir.display());
    }
    fs::create_dir_all(crate_dir.join("src"))?;
    // Cargo.toml de base
    let mut doc = DocumentMut::new();
    doc["package"]["name"] = value(name);
    doc["package"]["version"] = value("0.1.0");
    doc["package"]["edition"] = value("2021");
    let cargo_path = crate_dir.join("Cargo.toml");
    fs::write(&cargo_path, doc.to_string())?;
    // lib.rs
    fs::write(crate_dir.join("src/lib.rs"), "//! auto-generated lib\n\n")?;
    Ok(crate_dir)
}

/// Ajoute une dépendance dans Cargo.toml d’un crate existant.
pub fn add_dependency(cargo_toml: &Path, dep: &str, version: &str) -> Result<()> {
    let s = fs::read_to_string(cargo_toml).with_context(|| format!("read {:?}", cargo_toml))?;
    let mut doc = s.parse::<DocumentMut>()?;
    doc["dependencies"][dep] = value(version);
    fs::write(cargo_toml, doc.to_string())?;
    Ok(())
}

/// Crée un module source vide.
pub fn create_module(crate_dir: &Path, module: &str) -> Result<PathBuf> {
    let src = crate_dir.join("src");
    if !src.exists() { anyhow::bail!("src/ not found"); }
    let path = src.join(format!("{module}.rs"));
    if path.exists() { anyhow::bail!("module exists: {module}"); }
    fs::write(&path, format!("//! module {module}\n\n"))?;
    Ok(path)
}

/// Supprime un crate (best-effort).
pub fn remove_crate(dir: &Path) -> Result<()> {
    if dir.exists() {
        fs::remove_dir_all(dir)?;
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[test]
    fn crate_lifecycle() {
        let tmp = tempdir().unwrap();
        let root = tmp.path();
        let cdir = create_crate(root, "demo").unwrap();
        assert!(cdir.join("Cargo.toml").exists());
        add_dependency(&cdir.join("Cargo.toml"), "serde", "1.0").unwrap();
        let m = create_module(&cdir, "foo").unwrap();
        assert!(m.exists());
        remove_crate(&cdir).unwrap();
        assert!(!cdir.exists());
    }
}
