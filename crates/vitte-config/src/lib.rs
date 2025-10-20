//! vitte-config — gestion de la configuration Vitte
//!
//! Objectifs:
//! - Fichier projet `vitte.{toml,json}`
//! - Fichier utilisateur `~/.config/vitte/config.{toml,json}`
//! - Merge: defaults < user < projet
//! - Profils `debug` / `release`
//! - Sauvegarde/chargement TOML/JSON selon features
//!
//! API principale:
//! - [`Config::discover`]  -> cherche, charge et fusionne
//! - [`Config::load_path`] -> charge un fichier
//! - [`Config::save_path`] -> écrit un fichier
//! - [`Config::resolve_profile`] -> renvoie un profil effectif

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use anyhow::{bail, Context, Result};
use dirs::config_dir;
use std::{
    collections::HashMap,
    fs,
    path::{Path, PathBuf},
};

#[cfg(feature = "json")]
use serde_json as _serde_json;

#[cfg(feature = "toml")]
use toml as _toml;

#[cfg(any(feature = "json", feature = "toml"))]
use serde::{Deserialize, Serialize};

/// Niveau d’optimisation.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
pub enum OptLevel {
    O0,
    O1,
    O2,
    O3,
    Os,
    Oz,
}

impl Default for OptLevel {
    fn default() -> Self {
        Self::O0
    }
}

/// Architecture cible.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
pub enum Arch {
    X86_64,
    Aarch64,
    Riscv64,
}

impl Default for Arch {
    fn default() -> Self {
        Self::X86_64
    }
}

/// Backend codegen.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
pub enum Backend {
    Asm,
    Cranelift,
    Llvm,
}

impl Default for Backend {
    fn default() -> Self {
        Self::Cranelift
    }
}

/// Options de build.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
pub struct BuildProfile {
    pub opt: OptLevel,
    pub debug_info: bool,
    pub lto: bool,
    pub incremental: bool,
}

impl Default for BuildProfile {
    fn default() -> Self {
        Self { opt: OptLevel::O0, debug_info: true, lto: false, incremental: true }
    }
}

impl BuildProfile {
    pub fn release() -> Self {
        Self { opt: OptLevel::O3, debug_info: false, lto: true, incremental: false }
    }
}

/// Toolchain et cible.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
pub struct Toolchain {
    pub backend: Backend,
    pub arch: Arch,
    pub triple: Option<String>,
}

impl Default for Toolchain {
    fn default() -> Self {
        Self { backend: Backend::Cranelift, arch: Arch::X86_64, triple: None }
    }
}

/// Configuration racine.
#[derive(Clone, Debug, PartialEq)]
#[cfg_attr(any(feature = "json", feature = "toml"), derive(Serialize, Deserialize))]
pub struct Config {
    /// Nom du profil par défaut.
    pub default_profile: String,
    /// Profils disponibles.
    pub profiles: HashMap<String, BuildProfile>,
    /// Toolchain globale.
    pub toolchain: Toolchain,
}

impl Default for Config {
    fn default() -> Self {
        let mut profiles = HashMap::new();
        profiles.insert("debug".into(), BuildProfile::default());
        profiles.insert("release".into(), BuildProfile::release());
        Self { default_profile: "debug".into(), profiles, toolchain: Toolchain::default() }
    }
}

impl Config {
    /// Chemins candidats dans un projet.
    pub fn project_candidates<P: AsRef<Path>>(root: P) -> Vec<PathBuf> {
        let r = root.as_ref();
        vec![
            r.join("vitte.toml"),
            r.join("vitte.json"),
            r.join(".vitte.toml"),
            r.join(".vitte.json"),
        ]
    }

    /// Chemins candidats utilisateur: ~/.config/vitte/config.{toml,json}
    pub fn user_candidates() -> Vec<PathBuf> {
        let mut v = Vec::new();
        if let Some(cfg) = config_dir() {
            let base = cfg.join("vitte");
            v.push(base.join("config.toml"));
            v.push(base.join("config.json"));
        }
        v
    }

    /// Détecte et charge un fichier si présent.
    pub fn load_first_existing(paths: &[PathBuf]) -> Result<Option<Self>> {
        for p in paths {
            if p.exists() {
                let cfg = Self::load_path(p)?;
                return Ok(Some(cfg));
            }
        }
        Ok(None)
    }

    /// Découverte + merge: defaults < user < projet.
    pub fn discover<P: AsRef<Path>>(project_root: P) -> Result<Self> {
        let mut cfg = Self::default();

        if let Some(user_cfg) = Self::load_first_existing(&Self::user_candidates())? {
            cfg = cfg.merge(user_cfg);
        }
        if let Some(proj_cfg) = Self::load_first_existing(&Self::project_candidates(project_root))?
        {
            cfg = cfg.merge(proj_cfg);
        }
        cfg.validate()?;
        Ok(cfg)
    }

    /// Merge champ à champ.
    pub fn merge(self, other: Self) -> Self {
        let mut out = self;
        out.default_profile = other.default_profile;
        out.toolchain = other.toolchain;

        for (k, v) in other.profiles {
            out.profiles.insert(k, v);
        }
        out
    }

    /// Valide la configuration.
    pub fn validate(&self) -> Result<()> {
        if !self.profiles.contains_key(&self.default_profile) {
            bail!("profil par défaut introuvable: {}", self.default_profile);
        }
        Ok(())
    }

    /// Résout un profil par nom, sinon retourne `default_profile`.
    pub fn resolve_profile(&self, name: Option<&str>) -> &BuildProfile {
        let key = name.unwrap_or(&self.default_profile);
        self.profiles.get(key).unwrap_or_else(|| {
            // garanti par validate() dans les flux normaux
            &self.profiles[&self.default_profile]
        })
    }

    /// Charge depuis un chemin TOML/JSON.
    pub fn load_path<P: AsRef<Path>>(path: P) -> Result<Self> {
        let path = path.as_ref();
        let data = fs::read(path).with_context(|| format!("read {}", path.display()))?;
        match path.extension().and_then(|s| s.to_str()).unwrap_or_default() {
            "toml" => Self::from_toml(&data),
            "json" => Self::from_json(&data),
            _ => {
                // fallback: tente TOML puis JSON
                Self::from_toml(&data).or_else(|_| Self::from_json(&data))
            }
        }
        .with_context(|| format!("parse {}", path.display()))
    }

    /// Écrit au chemin. Format déduit de l’extension.
    pub fn save_path<P: AsRef<Path>>(&self, path: P) -> Result<()> {
        let path = path.as_ref();
        let bytes = match path.extension().and_then(|s| s.to_str()).unwrap_or_default() {
            "toml" => self.to_toml()?,
            "json" => self.to_json_pretty()?,
            other => bail!("extension non supportée: {}", other),
        };
        if let Some(parent) = path.parent() {
            if !parent.exists() {
                fs::create_dir_all(parent)
                    .with_context(|| format!("mkdir {}", parent.display()))?;
            }
        }
        fs::write(path, bytes).with_context(|| format!("write {}", path.display()))
    }

    // --- Sérialisation ----------------------------------------------------

    #[cfg(feature = "toml")]
    fn from_toml(bytes: &[u8]) -> Result<Self> {
        let s = std::str::from_utf8(bytes).context("utf8")?;
        let cfg: Self = _toml::from_str(s).context("toml")?;
        Ok(cfg)
    }

    #[cfg(not(feature = "toml"))]
    fn from_toml(_: &[u8]) -> Result<Self> {
        bail!("feature `toml` désactivée")
    }

    #[cfg(feature = "json")]
    fn from_json(bytes: &[u8]) -> Result<Self> {
        let cfg: Self = _serde_json::from_slice(bytes).context("json")?;
        Ok(cfg)
    }

    #[cfg(not(feature = "json"))]
    fn from_json(_: &[u8]) -> Result<Self> {
        bail!("feature `json` désactivée")
    }

    #[cfg(feature = "toml")]
    fn to_toml(&self) -> Result<Vec<u8>> {
        let s = _toml::to_string_pretty(self).context("toml ser")?;
        Ok(s.into_bytes())
    }

    #[cfg(not(feature = "toml"))]
    fn to_toml(&self) -> Result<Vec<u8>> {
        bail!("feature `toml` désactivée")
    }

    #[cfg(feature = "json")]
    fn to_json_pretty(&self) -> Result<Vec<u8>> {
        let s = _serde_json::to_vec_pretty(self).context("json ser")?;
        Ok(s)
    }

    #[cfg(not(feature = "json"))]
    fn to_json_pretty(&self) -> Result<Vec<u8>> {
        bail!("feature `json` désactivée")
    }
}

// --- Tests ---------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[test]
    fn defaults_ok() {
        let c = Config::default();
        assert!(c.profiles.contains_key("debug"));
        assert!(c.profiles.contains_key("release"));
        assert_eq!(c.resolve_profile(None).opt, OptLevel::O0);
    }

    #[cfg(feature = "toml")]
    #[test]
    fn roundtrip_toml() {
        let dir = tempdir().unwrap();
        let p = dir.path().join("vitte.toml");

        let mut cfg = Config::default();
        cfg.default_profile = "release".into();
        cfg.save_path(&p).unwrap();

        let read = Config::load_path(&p).unwrap();
        assert_eq!(read.default_profile, "release");
    }

    #[cfg(feature = "json")]
    #[test]
    fn roundtrip_json() {
        let dir = tempdir().unwrap();
        let p = dir.path().join("vitte.json");

        let mut cfg = Config::default();
        cfg.toolchain.backend = Backend::Asm;
        cfg.save_path(&p).unwrap();

        let read = Config::load_path(&p).unwrap();
        assert_eq!(read.toolchain.backend, Backend::Asm);
    }

    #[test]
    fn merge_order() {
        let mut base = Config::default();
        base.default_profile = "debug".into();

        let mut user = Config::default();
        user.default_profile = "release".into();

        let merged = base.merge(user);
        assert_eq!(merged.default_profile, "release");
    }
}
