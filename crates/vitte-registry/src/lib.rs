//! vitte-registry — index local de paquets Vitte + résolution simple
//!
//! Capacités principales :
//! - Index local au format JSON (option `json`) stocké sur disque
//! - Publication d’un paquet (enregistre métadonnées + place l’archive)
//! - Résolution des dépendances d’un projet (nom → artefact local ou URL)
//! - Téléchargement optionnel d’un paquet via `reqwest` (feature `reqwest`)
//!
//! Modèle simple, sans sémantique SemVer complète : sélection du *plus grand*
//! `version` par ordre lexicographique. Suffisant pour un POC.
//!
//! API rapide :
//! - [`Registry::open`], [`Registry::save`]
//! - [`Registry::publish_local`], [`Registry::resolve_one`], [`Registry::resolve_all`]
//! - [`download_to`] (activée si `reqwest`)
//!
//! Disposition sur disque :
//! ```text
//! <root>/index.json    # l’index (si feature json)
//! <root>/pkgs/<name>/<version>/<fichier-archive>
//! ```

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use anyhow::{bail, Context, Result};
use std::collections::HashMap;
use std::fs;
use std::path::{Path, PathBuf};

#[cfg(feature = "json")]
use serde::{Deserialize, Serialize};

/// Identité d’un paquet.
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct PackageId {
    pub name: String,
    pub version: String,
}

impl PackageId {
    pub fn new(name: impl Into<String>, version: impl Into<String>) -> Self {
        Self { name: name.into(), version: version.into() }
    }
}

/// Entrée d’index pour un artefact.
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PackageRecord {
    pub id: PackageId,
    /// URL distante éventuelle (HTTP/S3/etc.). `None` si artefact local uniquement.
    pub url: Option<String>,
    /// Chemin local de l’archive si présente dans le registre.
    pub local_path: Option<PathBuf>,
    /// Hash (non vérifié ici).
    pub checksum: Option<String>,
}

impl PackageRecord {
    pub fn with_local(id: PackageId, path: PathBuf) -> Self {
        Self { id, url: None, local_path: Some(path), checksum: None }
    }
}

/// Index complet : nom → liste de versions.
#[cfg_attr(feature = "json", derive(Serialize, Deserialize))]
#[derive(Debug, Default, Clone, PartialEq, Eq)]
pub struct Index {
    pub packages: HashMap<String, Vec<PackageRecord>>,
}

impl Index {
    pub fn add(&mut self, rec: PackageRecord) {
        self.packages.entry(rec.id.name.clone()).or_default().push(rec);
    }

    /// Sélectionne la "meilleure" version par ordre lexicographique.
    pub fn best(&self, name: &str, req: Option<&str>) -> Option<&PackageRecord> {
        let list = self.packages.get(name)?;
        // filtrage basique : si `req` est Some, on cherche version exactement égale,
        // sinon on prend la plus grande.
        if let Some(eq) = req {
            return list.iter().find(|r| r.id.version == eq);
        }
        list.iter().max_by(|a, b| a.id.version.cmp(&b.id.version))
    }
}

/// Registre encapsulant l’index + racine.
#[derive(Debug, Clone)]
pub struct Registry {
    root: PathBuf,
    index: Index,
}

impl Registry {
    /// Ouvre ou initialise un registre sous `root`.
    pub fn open(root: impl AsRef<Path>) -> Result<Self> {
        let root = root.as_ref().to_path_buf();
        fs::create_dir_all(root.join("pkgs"))?;
        let index = load_index(root.join("index.json"))?;
        Ok(Self { root, index })
    }

    /// Sauvegarde l’index.
    pub fn save(&self) -> Result<()> {
        save_index(self.root.join("index.json"), &self.index)
    }

    /// Retourne un snapshot de l’index.
    pub fn index(&self) -> &Index {
        &self.index
    }

    /// Publie un artefact local dans le registre.
    ///
    /// Copie `archive_path` vers `<root>/pkgs/<name>/<version>/<nom-fichier>`
    /// et ajoute l’entrée dans l’index.
    pub fn publish_local(
        &mut self,
        name: &str,
        version: &str,
        archive_path: impl AsRef<Path>,
        checksum: Option<String>,
    ) -> Result<PathBuf> {
        let dst_dir = self.root.join("pkgs").join(name).join(version);
        fs::create_dir_all(&dst_dir)?;
        let file_name = archive_path
            .as_ref()
            .file_name()
            .ok_or_else(|| anyhow::anyhow!("archive sans nom de fichier"))?;
        let dst = dst_dir.join(file_name);
        fs::copy(archive_path.as_ref(), &dst)
            .with_context(|| format!("copie vers {}", dst.display()))?;

        let mut rec = PackageRecord::with_local(PackageId::new(name, version), dst.clone());
        rec.checksum = checksum;
        self.index.add(rec);
        Ok(dst)
    }

    /// Enregistre une référence distante sans copier d’artefact local.
    pub fn publish_remote(
        &mut self,
        name: &str,
        version: &str,
        url: &str,
        checksum: Option<String>,
    ) {
        let rec = PackageRecord {
            id: PackageId::new(name, version),
            url: Some(url.to_string()),
            local_path: None,
            checksum,
        };
        self.index.add(rec);
    }

    /// Résout un paquet (nom, contrainte optionnelle) vers un artefact.
    pub fn resolve_one(&self, name: &str, version_req: Option<&str>) -> Result<Resolved> {
        let rec = self
            .index
            .best(name, version_req)
            .ok_or_else(|| anyhow::anyhow!("paquet introuvable: {name}"))?;
        Ok(Resolved {
            id: rec.id.clone(),
            source: if let Some(p) = &rec.local_path {
                Source::Local(p.clone())
            } else if let Some(u) = &rec.url {
                Source::Remote(u.clone())
            } else {
                bail!("enregistrement invalide pour {name}: pas de source")
            },
            checksum: rec.checksum.clone(),
        })
    }

    /// Résout un ensemble de dépendances `name -> version_req`.
    pub fn resolve_all(&self, deps: &HashMap<String, String>) -> Result<Vec<Resolved>> {
        let mut out = Vec::new();
        for (name, req) in deps {
            out.push(self.resolve_one(name, Some(req.as_str()))?);
        }
        Ok(out)
    }

    /// Chemin racine du registre.
    pub fn root(&self) -> &Path {
        &self.root
    }
}

/// Résolution d’un artefact.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Resolved {
    pub id: PackageId,
    pub source: Source,
    pub checksum: Option<String>,
}

/// Source d’un artefact.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Source {
    Local(PathBuf),
    Remote(String),
}

/* ============================ Entrées/Sorties ============================ */

#[cfg(feature = "json")]
fn load_index(path: impl AsRef<Path>) -> Result<Index> {
    let p = path.as_ref();
    if !p.exists() {
        return Ok(Index::default());
    }
    let data = fs::read_to_string(p).with_context(|| format!("read {}", p.display()))?;
    Ok(serde_json::from_str(&data).context("parse index.json")?)
}
#[cfg(not(feature = "json"))]
fn load_index(_path: impl AsRef<Path>) -> Result<Index> {
    Ok(Index::default())
}

#[cfg(feature = "json")]
fn save_index(path: impl AsRef<Path>, index: &Index) -> Result<()> {
    let s = serde_json::to_string_pretty(index)?;
    if let Some(parent) = path.as_ref().parent() {
        fs::create_dir_all(parent)?;
    }
    fs::write(path, s)?;
    Ok(())
}
#[cfg(not(feature = "json"))]
fn save_index(_path: impl AsRef<Path>, _index: &Index) -> Result<()> {
    Ok(())
}

/* =============================== Réseau ================================== */

/// Télécharge une URL vers `dst`. Activé si la feature `reqwest` est présente.
#[cfg(feature = "reqwest")]
pub fn download_to(url: &str, dst: impl AsRef<Path>) -> Result<()> {
    let resp = reqwest::blocking::get(url)?.error_for_status()?;
    let bytes = resp.bytes()?;
    if let Some(p) = dst.as_ref().parent() {
        fs::create_dir_all(p)?;
    }
    fs::write(dst, &bytes)?;
    Ok(())
}

#[cfg(not(feature = "reqwest"))]
pub fn download_to(_url: &str, _dst: impl AsRef<Path>) -> Result<()> {
    bail!("feature `reqwest` désactivée")
}

/* ================================ Tests ================================= */

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[test]
    fn publish_and_resolve_local() {
        let tmp = tempdir().unwrap();
        let root = tmp.path();

        // archive factice
        let pkg_dir = root.join("src");
        fs::create_dir_all(&pkg_dir).unwrap();
        let archive = pkg_dir.join("vitte-foo-0.1.0.tar.gz");
        fs::write(&archive, b"dummy").unwrap();

        let mut reg = Registry::open(root).unwrap();
        let dst = reg.publish_local("foo", "0.1.0", &archive, None).unwrap();
        assert!(dst.exists());
        reg.save().unwrap();

        // résolution
        let r = reg.resolve_one("foo", None).unwrap();
        match r.source {
            Source::Local(p) => assert!(p.ends_with("vitte-foo-0.1.0.tar.gz")),
            _ => panic!("attendu local"),
        }
    }

    #[test]
    fn remote_entry_resolves() {
        let tmp = tempdir().unwrap();
        let mut reg = Registry::open(tmp.path()).unwrap();
        reg.publish_remote("bar", "2.0.0", "https://example.invalid/bar.tgz", None);
        let r = reg.resolve_one("bar", Some("2.0.0")).unwrap();
        match r.source {
            Source::Remote(u) => assert!(u.contains("example")),
            _ => panic!("attendu remote"),
        }
    }

    #[test]
    fn resolve_all_uses_exact_match() {
        let tmp = tempdir().unwrap();
        let mut reg = Registry::open(tmp.path()).unwrap();
        reg.publish_remote("x", "1.0.0", "https://e/x-1.0.0.tgz", None);
        reg.publish_remote("x", "2.0.0", "https://e/x-2.0.0.tgz", None);

        let mut deps = HashMap::new();
        deps.insert("x".to_string(), "1.0.0".to_string());
        let all = reg.resolve_all(&deps).unwrap();
        assert_eq!(all.len(), 1);
        assert_eq!(all[0].id.version, "1.0.0");
    }
}
