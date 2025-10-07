//! vitte-packager — packaging et distribution des toolchains Vitte
//!
//! Fournit :
//! - Construction d’archives `.tar.gz` et `.zip`
//! - Lecture/écriture de manifeste JSON (nom, version, plateforme, url)
//! - Vérification basique d’intégrité
//!
//! API :
//! - [`Manifest`] : métadonnées d’un paquet
//! - [`create_tarball`], [`create_zip`] : générer archives
//! - [`extract_archive`] : extraire une archive
//! - [`verify`] : contrôler le contenu
//!
//! Exemples :
//! ```ignore
//! use vitte_packager::{Manifest, create_tarball};
//! let m = Manifest::new("vitte", "0.1.0", "x86_64-unknown-linux-gnu");
//! m.to_file("manifest.json").unwrap();
//! create_tarball("pkg", "pkg.tar.gz").unwrap();
//! ```

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions, clippy::doc_markdown, clippy::too_many_lines)]

use anyhow::{bail, Result};
use std::path::Path;

#[cfg(feature = "json")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "json")]
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Manifest {
    pub name: String,
    pub version: String,
    pub triple: String,
    pub url: Option<String>,
}

#[cfg(feature = "json")]
impl Manifest {
    pub fn new(name: impl Into<String>, version: impl Into<String>, triple: impl Into<String>) -> Self {
        Self { name: name.into(), version: version.into(), triple: triple.into(), url: None }
    }

    pub fn to_string(&self) -> Result<String> {
        Ok(serde_json::to_string_pretty(self)?)
    }

    pub fn to_file(&self, path: impl AsRef<Path>) -> Result<()> {
        let s = self.to_string()?;
        std::fs::write(path, s)?;
        Ok(())
    }

    pub fn from_file(path: impl AsRef<Path>) -> Result<Self> {
        let data = std::fs::read_to_string(path)?;
        Ok(serde_json::from_str(&data)?)
    }
}

#[cfg(feature = "archive")]
pub fn create_tarball(src_dir: impl AsRef<Path>, dst: impl AsRef<Path>) -> Result<()> {
    use flate2::write::GzEncoder;
    use flate2::Compression;
    use tar::Builder;

    let tar_gz = std::fs::File::create(&dst)?;
    let enc = GzEncoder::new(tar_gz, Compression::default());
    let mut tar = Builder::new(enc);
    tar.append_dir_all(".", &src_dir)?;
    Ok(())
}

#[cfg(feature = "archive")]
pub fn create_zip(src_dir: impl AsRef<Path>, dst: impl AsRef<Path>) -> Result<()> {
    use zip::write::FileOptions;
    use std::io::Write;

    let file = std::fs::File::create(&dst)?;
    let mut zip = zip::ZipWriter::new(file);

    let opts = FileOptions::default();
    for entry in walkdir::WalkDir::new(src_dir.as_ref()) {
        let entry = entry?;
        let path = entry.path();
        let rel = path.strip_prefix(src_dir.as_ref()).unwrap();
        if path.is_file() {
            zip.start_file(rel.to_string_lossy(), opts)?;
            let mut f = std::fs::File::open(path)?;
            io::copy(&mut f, &mut zip)?;
        } else if !rel.as_os_str().is_empty() {
            zip.add_directory(rel.to_string_lossy(), opts)?;
        }
    }
    zip.finish()?;
    Ok(())
}

#[cfg(feature = "archive")]
pub fn extract_archive(archive: impl AsRef<Path>, dst: impl AsRef<Path>) -> Result<()> {
    let path = archive.as_ref();
    if path.extension().and_then(|s| s.to_str()) == Some("gz") {
        let f = std::fs::File::open(path)?;
        let dec = flate2::read::GzDecoder::new(f);
        let mut ar = tar::Archive::new(dec);
        ar.unpack(dst)?;
        return Ok(());
    }
    if path.extension().and_then(|s| s.to_str()) == Some("zip") {
        let f = std::fs::File::open(path)?;
        let mut zip = zip::ZipArchive::new(f)?;
        zip.extract(dst)?;
        return Ok(());
    }
    bail!("extension d’archive non supportée: {}", path.display());
}

/// Vérifie que le répertoire contient au moins un binaire vitte.
pub fn verify(dir: impl AsRef<Path>) -> Result<()> {
    let p = dir.as_ref().join("bin").join("vitte");
    if !p.exists() {
        bail!("binaire vitte manquant dans {}", p.display());
    }
    Ok(())
}

// =========================== Tests ========================================

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::tempdir;

    #[cfg(feature = "json")]
    #[test]
    fn manifest_roundtrip() {
        let tmp = tempdir().unwrap();
        let path = tmp.path().join("m.json");
        let m = Manifest::new("vitte", "0.1.0", "x86_64-unknown-linux-gnu");
        m.to_file(&path).unwrap();
        let m2 = Manifest::from_file(&path).unwrap();
        assert_eq!(m, m2);
    }

    #[test]
    fn verify_fails_without_bin() {
        let tmp = tempdir().unwrap();
        let err = verify(tmp.path());
        assert!(err.is_err());
    }
}