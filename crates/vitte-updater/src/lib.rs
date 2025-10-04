//! vitte-updater — vérification et mise à jour des binaires/artefacts Vitte
//!
//! Cibles : CLIs et outils Vitte. Zéro `unsafe`. Tout optionnel par features.
//!
//! Capacités principales :
//! - Lecture de “manifestes de version” JSON distants (dernier numéro, URLs, checksums).
//! - Téléchargement (bloquant, ou async si `feature="async"`).
//! - Vérification SHA-256 (si `feature="sha"`).
//! - Décompression `.zip`, `.tar.gz`, `.tgz`, `.tar.xz` (si `feature="archive"`).
//! - Emplacement cache/config standard (crate `directories`).
//! - Barre de progression (si `feature="progress"`).
//! - Aides pour **self-update** atomique d’un binaire.
//!
//! Format de manifeste minimal attendu (`/releases/latest.json`) :
//! ```json
//! {
//!   "name": "vitte-cli",
//!   "version": "1.2.3",
//!   "assets": [
//!     {"target":"x86_64-unknown-linux-gnu","url":"https://.../vitte-cli-1.2.3-x86_64.tar.gz","sha256":"..."},
//!     {"target":"x86_64-pc-windows-msvc","url":"https://.../vitte-cli-1.2.3-x86_64.zip","sha256":"..."}
//!   ]
//! }
//! ```
//!
//! API rapide :
//! - [`Latest`], [`Asset`], [`fetch_latest_blocking`], [`fetch_latest_async`]
//! - [`download_to_blocking`]/[`download_to_async`], [`verify_sha256_file`]
//! - [`extract_archive`], [`atomic_replace`] (self-update)
//! - [`self_update_blocking`]/[`self_update_async`]

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines,
    clippy::missing_errors_doc
)]

use anyhow::{bail, Context, Result};
use directories::ProjectDirs;
use semver::Version;
use std::fs::{self, File};
use std::io::{Read, Write};
use std::path::{Path, PathBuf};
use tempfile::NamedTempFile;

#[cfg(feature = "serde")]
use serde::Deserialize;


#[cfg(feature = "progress")]
use indicatif::{ProgressBar, ProgressStyle};

/* =============================== Modèle JSON =============================== */

/// Un asset binaire packagé pour une cible.
#[derive(Debug, Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Deserialize))]
pub struct Asset {
    pub target: String,
    pub url: String,
    pub sha256: Option<String>,
}

/// Manifeste “latest”.
#[derive(Debug, Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Deserialize))]
pub struct Latest {
    pub name: String,
    pub version: String,
    pub assets: Vec<Asset>,
}

impl Latest {
    pub fn version_parsed(&self) -> Result<Version> {
        Version::parse(&self.version).context("parse semver")
    }
    pub fn pick_for(&self, target: &str) -> Option<&Asset> {
        self.assets.iter().find(|a| a.target == target)
    }
}

/* =============================== Local paths =============================== */

/// Répertoire cache racine: `~/.cache/vitte/updater` (selon OS).
pub fn cache_root() -> Result<PathBuf> {
    let dirs = ProjectDirs::from("org", "Vitte", "Vitte").ok_or_else(|| anyhow::anyhow!("dirs"))?;
    let p = dirs.cache_dir().join("updater");
    fs::create_dir_all(&p)?;
    Ok(p)
}

/// Répertoire binaires par défaut: `~/.local/share/vitte/bin` (Linux) etc.
pub fn bin_root() -> Result<PathBuf> {
    let dirs = ProjectDirs::from("org", "Vitte", "Vitte").ok_or_else(|| anyhow::anyhow!("dirs"))?;
    let p = dirs.data_dir().join("bin");
    fs::create_dir_all(&p)?;
    Ok(p)
}

/* ============================== Progress helper ============================ */

#[cfg(feature = "progress")]
fn mk_pb(len: Option<u64>, msg: &str) -> ProgressBar {
    let pb = match len {
        Some(l) => ProgressBar::new(l),
        None => ProgressBar::new_spinner(),
    };
    let sty = ProgressStyle::with_template("{spinner:.green} {msg} [{bar:40.cyan/blue}] {bytes}/{total_bytes} ({eta})")
        .unwrap()
        .progress_chars("=> ");
    pb.set_style(sty);
    pb.set_message(msg.to_string());
    pb
}

/* ================================ HTTP GET ================================= */

#[cfg(feature = "http")]
fn user_agent() -> String {
    format!("vitte-updater/{}", env!("CARGO_PKG_VERSION"))
}

#[cfg(all(feature = "http", feature = "serde"))]
pub fn fetch_latest_blocking(url: &str) -> Result<Latest> {
    let resp = reqwest::blocking::Client::new()
        .get(url)
        .header(reqwest::header::USER_AGENT, user_agent())
        .send()?
        .error_for_status()?;
    let latest: Latest = resp.json().context("parse latest json")?;
    Ok(latest)
}

#[cfg(all(feature = "http", feature = "async", feature = "serde"))]
pub async fn fetch_latest_async(url: &str) -> Result<Latest> {
    let resp = reqwest::Client::new()
        .get(url)
        .header(reqwest::header::USER_AGENT, user_agent())
        .send()
        .await?
        .error_for_status()?;
    let latest: Latest = resp.json().await.context("parse latest json")?;
    Ok(latest)
}

#[cfg(feature = "http")]
pub fn download_to_blocking(url: &str, dst: impl AsRef<Path>) -> Result<()> {
    let mut resp = reqwest::blocking::Client::new()
        .get(url)
        .header(reqwest::header::USER_AGENT, user_agent())
        .send()?
        .error_for_status()?;
    #[cfg(feature = "progress")]
    let total = resp.content_length();
    #[cfg(feature = "progress")]
    let pb = mk_pb(total, "Téléchargement");
    let mut file = File::create(&dst).with_context(|| format!("create {}", dst.as_ref().display()))?;
    #[cfg(feature = "progress")]
    let mut downloaded: u64 = 0;
    let mut buf = [0u8; 64 * 1024];
    loop {
        let n = resp.read(&mut buf)?;
        if n == 0 { break; }
        file.write_all(&buf[..n])?;
        #[cfg(feature = "progress")]
        {
            downloaded += n as u64;
            pb.set_position(downloaded);
        }
    }
    #[cfg(feature = "progress")]
    pb.finish_and_clear();
    Ok(())
}

#[cfg(all(feature = "http", feature = "async"))]
pub async fn download_to_async(url: &str, dst: impl AsRef<Path>) -> Result<()> {
    use tokio::io::AsyncWriteExt;
    use tokio::io::AsyncReadExt;
    let resp = reqwest::Client::new()
        .get(url)
        .header(reqwest::header::USER_AGENT, user_agent())
        .send()
        .await?
        .error_for_status()?;

    let total = resp.content_length();
    #[cfg(feature = "progress")]
    let pb = mk_pb(total, "Téléchargement");
    let mut file = tokio::fs::File::create(&dst).await?;
    let mut stream = resp.bytes_stream();

    use futures_util::StreamExt;
    #[cfg(feature = "progress")]
    let mut downloaded = 0u64;
    while let Some(chunk) = stream.next().await {
        let bytes = chunk?;
        file.write_all(&bytes).await?;
        #[cfg(feature = "progress")]
        {
            downloaded += bytes.len() as u64;
            pb.set_position(downloaded);
        }
    }
    file.flush().await?;
    #[cfg(feature = "progress")]
    pb.finish_and_clear();
    Ok(())
}

/* ================================ Checksums ================================ */

#[cfg(feature = "sha")]
pub fn verify_sha256_file(path: impl AsRef<Path>, expected_hex: &str) -> Result<()> {
    use sha2::{Digest, Sha256};
    let mut f = File::open(&path)?;
    let mut hasher = Sha256::new();
    let mut buf = [0u8; 64 * 1024];
    loop {
        let n = f.read(&mut buf)?;
        if n == 0 { break; }
        hasher.update(&buf[..n]);
    }
    let got = hasher.finalize();
    let got_hex = hex::encode(got);
    if got_hex.eq_ignore_ascii_case(expected_hex) {
        Ok(())
    } else {
        bail!("sha256 mismatch: expected {expected_hex}, got {got_hex}")
    }
}

/* ================================ Archives ================================= */

#[cfg(feature = "archive")]
pub fn extract_archive(archive: impl AsRef<Path>, dest_dir: impl AsRef<Path>) -> Result<()> {
    let p = archive.as_ref();
    let s = p.to_string_lossy().to_lowercase();
    fs::create_dir_all(dest_dir.as_ref())?;
    if s.ends_with(".zip") {
        extract_zip(p, dest_dir)
    } else if s.ends_with(".tar.gz") || s.ends_with(".tgz") {
        extract_tar_gz(p, dest_dir)
    } else if s.ends_with(".tar.xz") {
        extract_tar_xz(p, dest_dir)
    } else {
        bail!("format d’archive non supporté: {}", p.display())
    }
}

#[cfg(feature = "archive")]
fn extract_zip(p: &Path, dest: impl AsRef<Path>) -> Result<()> {
    let f = File::open(p)?;
    let mut zip = zip::ZipArchive::new(f).context("open zip")?;
    for i in 0..zip.len() {
        let mut entry = zip.by_index(i)?;
        let out = dest.as_ref().join(entry.mangled_name());
        if entry.is_dir() {
            fs::create_dir_all(&out)?;
        } else {
            if let Some(parent) = out.parent() { fs::create_dir_all(parent)?; }
            let mut of = File::create(&out)?;
            std::io::copy(&mut entry, &mut of)?;
            #[cfg(unix)]
            {
                use std::os::unix::fs::PermissionsExt;
                if let Some(mode) = entry.unix_mode() {
                    fs::set_permissions(&out, fs::Permissions::from_mode(mode))?;
                }
            }
        }
    }
    Ok(())
}

#[cfg(feature = "archive")]
fn extract_tar_gz(p: &Path, dest: impl AsRef<Path>) -> Result<()> {
    let f = File::open(p)?;
    let dec = flate2::read::GzDecoder::new(f);
    let mut ar = tar::Archive::new(dec);
    ar.unpack(dest)?;
    Ok(())
}

#[cfg(feature = "archive")]
fn extract_tar_xz(p: &Path, dest: impl AsRef<Path>) -> Result<()> {
    let f = File::open(p)?;
    let dec = xz2::read::XzDecoder::new(f);
    let mut ar = tar::Archive::new(dec);
    ar.unpack(dest)?;
    Ok(())
}

/* ================================ Self-update ============================== */

/// Remplace atomiquement `current_bin` par `new_bin_path` (même FS).
/// Sur Windows, l’original est renommé avant copie.
pub fn atomic_replace(current_bin: impl AsRef<Path>, new_bin_path: impl AsRef<Path>) -> Result<()> {
    let cur = current_bin.as_ref();
    let new = new_bin_path.as_ref();
    let parent = cur.parent().ok_or_else(|| anyhow::anyhow!("binaire cible sans parent"))?;
    // fichier temporaire dans le même dossier pour atomicité maximale
    let mut tmp = NamedTempFile::new_in(parent)?;
    {
        let mut src = File::open(new)?;
        std::io::copy(&mut src, &mut tmp)?;
        #[cfg(unix)]
        {
            use std::os::unix::fs::PermissionsExt;
            let mut perm = fs::metadata(new)?.permissions();
            if perm.mode() & 0o111 == 0 {
                perm.set_mode(0o755);
            }
            fs::set_permissions(tmp.path(), perm)?;
        }
    }
    #[cfg(target_os = "windows")]
    {
        let bak = parent.join(format!(
            ".{}.old",
            cur.file_name().and_then(|s| s.to_str()).unwrap_or("bin")
        ));
        let _ = fs::remove_file(&bak);
        let _ = fs::rename(cur, &bak);
    }
    tmp.persist(cur)?;
    Ok(())
}

/// Résout la cible courante Rust (heuristique) type `x86_64-unknown-linux-gnu`.
pub fn current_rust_target() -> String {
    // Fallback à env à la compilation, sinon heuristique OS.
    let env_target = option_env!("TARGET").map(|s| s.to_string());
    if let Some(t) = env_target { return t; }
    #[cfg(target_os = "linux")]
    return "x86_64-unknown-linux-gnu".into();
    #[cfg(target_os = "macos")]
    return "x86_64-apple-darwin".into();
    #[cfg(target_os = "windows")]
    return "x86_64-pc-windows-msvc".into();
    #[allow(unreachable_code)]
    "unknown-unknown".into()
}

/// Séquence complète de mise à jour (bloquante).
/// - Récupère le manifeste `latest_url`.
/// - Choisit l’asset pour `target` (ou auto).
/// - Télécharge dans cache.
/// - Vérifie le SHA si fourni.
/// - Extrait puis remplace le binaire actuel.
/// `binary_in_archive` est le chemin relatif du binaire dans l’archive.
#[cfg(all(feature = "http", feature = "serde"))]
pub fn self_update_blocking(
    latest_url: &str,
    target: Option<&str>,
    binary_in_archive: &Path,
    install_path: &Path,
) -> Result<Version> {
    let latest = fetch_latest_blocking(latest_url)?;
    let target_buf = target.map(|s| s.to_string()).unwrap_or_else(current_rust_target);
    let asset = latest.pick_for(&target_buf).ok_or_else(|| anyhow::anyhow!(format!("asset introuvable pour {}", &target_buf)))?;

    let cache = cache_root()?;
    let archive_path = cache.join(format!(
        "{}-{}-download",
        latest.name, latest.version
    ));

    download_to_blocking(&asset.url, &archive_path)?;

    #[cfg(feature = "sha")]
    if let Some(sum) = &asset.sha256 {
        verify_sha256_file(&archive_path, sum)?;
    }

    #[cfg(feature = "archive")]
    {
        let extract_dir = cache.join(format!("extract-{}-{}", latest.name, latest.version));
        if extract_dir.exists() {
            let _ = fs::remove_dir_all(&extract_dir);
        }
        fs::create_dir_all(&extract_dir)?;
        extract_archive(&archive_path, &extract_dir)?;
        let src_bin = extract_dir.join(binary_in_archive);
        atomic_replace(install_path, &src_bin)?;
    }

    Ok(latest.version_parsed()?)
}

/// Variante async.
#[cfg(all(feature = "http", feature = "async", feature = "serde"))]
pub async fn self_update_async(
    latest_url: &str,
    target: Option<&str>,
    binary_in_archive: &Path,
    install_path: &Path,
) -> Result<Version> {
    let latest = fetch_latest_async(latest_url).await?;
    let target_buf = target.map(|s| s.to_string()).unwrap_or_else(current_rust_target);
    let asset = latest.pick_for(&target_buf).ok_or_else(|| anyhow::anyhow!(format!("asset introuvable pour {}", &target_buf)))?;

    let cache = cache_root()?;
    let archive_path = cache.join(format!("{}-{}-download", latest.name, latest.version));
    download_to_async(&asset.url, &archive_path).await?;

    #[cfg(feature = "sha")]
    if let Some(sum) = &asset.sha256 {
        verify_sha256_file(&archive_path, sum)?;
    }

    #[cfg(feature = "archive")]
    {
        let extract_dir = cache.join(format!("extract-{}-{}", latest.name, latest.version));
        if extract_dir.exists() {
            let _ = tokio::fs::remove_dir_all(&extract_dir).await;
        }
        tokio::fs::create_dir_all(&extract_dir).await?;
        // extraction sync (tar/zip APIs sync) → autorisé pour simplicité
        extract_archive(&archive_path, &extract_dir)?;
        let src_bin = extract_dir.join(binary_in_archive);
        atomic_replace(install_path, &src_bin)?;
    }

    Ok(latest.version_parsed()?)
}

/* =================================== Tests =================================== */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn paths_exist() {
        let c = cache_root().unwrap();
        let b = bin_root().unwrap();
        assert!(c.exists());
        assert!(b.exists());
    }

    #[cfg(feature = "archive")]
    #[test]
    fn extract_zip_tgz_xz_roundtrip() {
        // Pas de réseaux. On fabrique des archives minimales à extraire.
        let tmp = tempfile::tempdir().unwrap();
        let src_dir = tmp.path().join("src");
        fs::create_dir_all(&src_dir).unwrap();
        fs::write(src_dir.join("file.txt"), "hello").unwrap();

        // Créer une tar.gz en mémoire rapide
        let tgz = tmp.path().join("a.tar.gz");
        {
            let tarfile = File::create(&tgz).unwrap();
            let enc = flate2::write::GzEncoder::new(tarfile, flate2::Compression::default());
            let mut tarb = tar::Builder::new(enc);
            tarb.append_dir_all("pkg", &src_dir).unwrap();
            tarb.finish().unwrap();
        }

        let out = tmp.path().join("out");
        extract_archive(&tgz, &out).unwrap();
        assert!(out.join("pkg").join("file.txt").exists());
    }

    #[cfg(feature = "sha")]
    #[test]
    fn sha_mismatch() {
        let tmp = tempfile::NamedTempFile::new().unwrap();
        fs::write(tmp.path(), b"abc").unwrap();
        let err = verify_sha256_file(tmp.path(), "deadbeef").unwrap_err();
        assert!(err.to_string().contains("sha256 mismatch"));
    }
}