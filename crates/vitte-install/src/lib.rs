//! vitte-install — gestion des toolchains Vitte (style rustup)
//!
//! Capacités:
//! - Résolution du répertoire home (~/.vitte/toolchains, ~/.vitte/bin)
//! - Lecture d’un manifeste JSON (canaux → URLs par OS/arch)
//! - Téléchargement optionnel (`feature = "net"`)
//! - Extraction .tar.gz / .zip optionnelle (`feature = "archive"`)
//! - Installation atomique dans `toolchains/<name>` + liens dans `bin/`
//! - Commutateur `default` via lien symbolique
//!
//! API:
//! - [`Manifest`], [`Channel`], [`Asset`]
//! - [`detect_triple`], [`install`], [`set_default`], [`list_toolchains`], [`remove_toolchain`]
//! - [`ensure_shims`] pour déposer des shims `vitte`, `vittec`, etc.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use anyhow::{bail, Context, Result};
use std::{
    env,
    fs,
    io,
    path::{Path, PathBuf},
    time::Duration,
};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

// ============================ Modèle de manifeste ==========================

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone, Default)]
pub struct Manifest {
    pub channels: Vec<Channel>,
}

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
pub struct Channel {
    pub name: String,              // ex: "stable", "beta", "nightly-2025-09-01"
    pub targets: Vec<Asset>,       // un asset par triple
    pub components: Vec<Component> // facultatif
}

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
pub struct Component {
    pub id: String, // ex: "vitte-src", "vitte-std"
    pub url: String,
    pub kind: String, // "tar.gz" ou "zip"
}

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Debug, Clone)]
pub struct Asset {
    pub triple: String, // ex: "x86_64-unknown-linux-gnu"
    pub url: String,
    pub kind: String,   // "tar.gz" ou "zip"
}

// ============================== Emplacements ===============================

pub fn vitte_home() -> Result<PathBuf> {
    let base = if let Ok(p) = env::var("VITTE_HOME") {
        PathBuf::from(p)
    } else if let Some(home) = dirs::home_dir() {
        home.join(".vitte")
    } else {
        bail!("impossible de résoudre le répertoire HOME");
    };
    fs::create_dir_all(&base)?;
    Ok(base)
}

pub fn toolchains_dir() -> Result<PathBuf> {
    let p = vitte_home()?.join("toolchains");
    fs::create_dir_all(&p)?;
    Ok(p)
}

pub fn bin_dir() -> Result<PathBuf> {
    let p = vitte_home()?.join("bin");
    fs::create_dir_all(&p)?;
    Ok(p)
}

pub fn default_link() -> Result<PathBuf> {
    Ok(vitte_home()?.join("default"))
}

// ============================== Plateforme =================================

pub fn detect_triple() -> String {
    let arch = std::env::consts::ARCH;
    let os = std::env::consts::OS;
    match (arch, os) {
        ("x86_64", "linux") => "x86_64-unknown-linux-gnu",
        ("aarch64", "linux") => "aarch64-unknown-linux-gnu",
        ("x86_64", "macos") => "x86_64-apple-darwin",
        ("aarch64", "macos") => "aarch64-apple-darwin",
        ("x86_64", "windows") => "x86_64-pc-windows-msvc",
        ("aarch64", "windows") => "aarch64-pc-windows-msvc",
        _ => "unknown-unknown-unknown",
    }
    .to_string()
}

// ============================== Entrées/Sorties ============================

#[cfg(feature = "serde")]
pub fn load_manifest_from_str(s: &str) -> Result<Manifest> {
    Ok(serde_json::from_str(s).context("parse manifeste JSON")?)
}

#[cfg(feature = "serde")]
pub fn load_manifest_from_file(p: &Path) -> Result<Manifest> {
    let data = fs::read_to_string(p).with_context(|| format!("read {}", p.display()))?;
    load_manifest_from_str(&data)
}

pub fn find_asset<'a>(m: &'a Manifest, channel: &str, triple: &str) -> Option<&'a Asset> {
    m.channels
        .iter()
        .find(|c| c.name == channel)
        .and_then(|c| c.targets.iter().find(|a| a.triple == triple))
}

// ============================== Téléchargement =============================

#[cfg(feature = "net")]
pub async fn download_to_file(url: &str, dst: &Path) -> Result<()> {
    let client = reqwest::Client::builder()
        .timeout(Duration::from_secs(300))
        .build()?;
    let resp = client.get(url).send().await?.error_for_status()?;
    let bytes = resp.bytes().await?;
    if let Some(parent) = dst.parent() { fs::create_dir_all(parent)?; }
    tokio::fs::write(dst, &bytes).await?;
    Ok(())
}

#[cfg(not(feature = "net"))]
pub fn download_to_file(_url: &str, _dst: &Path) -> Result<()> {
    bail!("feature `net` désactivée")
}

// ============================== Extraction =================================

#[cfg(all(feature = "archive"))]
pub fn extract_archive(archive_path: &Path, kind: &str, dest: &Path) -> Result<()> {
    fs::create_dir_all(dest)?;
    match kind {
        "tar.gz" => {
            let f = fs::File::open(archive_path)?;
            let dec = flate2::read::GzDecoder::new(f);
            let mut ar = tar::Archive::new(dec);
            ar.unpack(dest)?;
        }
        "zip" => {
            let f = fs::File::open(archive_path)?;
            let mut zip = zip::ZipArchive::new(f)?;
            for i in 0..zip.len() {
                let mut file = zip.by_index(i)?;
                let out = dest.join(file.mangled_name());
                if file.name().ends_with('/') {
                    fs::create_dir_all(&out)?;
                } else {
                    if let Some(p) = out.parent() { fs::create_dir_all(p)?; }
                    let mut out_f = fs::File::create(&out)?;
                    io::copy(&mut file, &mut out_f)?;
                }
            }
        }
        other => bail!("archive inconnue: {other}"),
    }
    Ok(())
}

#[cfg(not(feature = "archive"))]
pub fn extract_archive(_archive_path: &Path, _kind: &str, _dest: &Path) -> Result<()> {
    bail!("feature `archive` désactivée")
}

// ============================== Installation ===============================

/// Installe un toolchain à partir d’un asset.
/// `name` = ex: "stable-aarch64-apple-darwin".
#[cfg(feature = "net")]
pub async fn install(name: &str, asset: &Asset) -> Result<PathBuf> {
    let tdir = toolchains_dir()?;
    let final_dir = tdir.join(name);
    if final_dir.exists() {
        return Ok(final_dir);
    }

    let tmp_dir = tdir.join(format!(".{}.partial", name));
    if tmp_dir.exists() { fs::remove_dir_all(&tmp_dir)?; }
    fs::create_dir_all(&tmp_dir)?;

    // 1) Télécharger
    let archive_path = tmp_dir.join(match asset.kind.as_str() {
        "tar.gz" => "pkg.tar.gz",
        "zip" => "pkg.zip",
        k => bail!("kind inconnu: {k}"),
    });
    download_to_file(&asset.url, &archive_path).await?;

    // 2) Extraire
    let payload_dir = tmp_dir.join("payload");
    extract_archive(&archive_path, &asset.kind, &payload_dir)?;

    // 3) Déplacer atomiquement
    fs::rename(&payload_dir, &final_dir)?;

    // 4) Nettoyer
    let _ = fs::remove_dir_all(&tmp_dir);

    Ok(final_dir)
}

/// Crée les shims dans ~/.vitte/bin pointant vers le toolchain par défaut.
/// Les exécutables cherchés: ["vitte", "vittec", "vitte-lsp"].
pub fn ensure_shims() -> Result<()> {
    let bin = bin_dir()?;
    let def = default_link()?;
    let exes = ["vitte", "vittec", "vitte-lsp"];

    for exe in exes {
        let shim = bin.join(exe);
        if shim.exists() {
            #[cfg(unix)]
            { let _ = fs::remove_file(&shim); }

            #[cfg(windows)]
            { let _ = fs::remove_file(&shim); }
        }
        // Le shim est un lien -> default/<exe> (/bin sur Windows -> .exe si nécessaire).
        let target = def.join("bin").join(exe_with_ext(exe));
        symlink_file(&target, &shim)
            .with_context(|| format!("create shim {}", shim.display()))?;
    }
    Ok(())
}

fn exe_with_ext(name: &str) -> String {
    if cfg!(windows) { format!("{name}.exe") } else { name.to_string() }
}

fn symlink_file(target: &Path, link: &Path) -> Result<()> {
    #[cfg(unix)]
    {
        use std::os::unix::fs as unixfs;
        if link.exists() { fs::remove_file(link)?; }
        unixfs::symlink(target, link)?;
        Ok(())
    }
    #[cfg(windows)]
    {
        use std::os::windows::fs as winfs;
        if link.exists() { fs::remove_file(link)?; }
        // Sur Windows, créer un .cmd de fallback si symlink échoue.
        match winfs::symlink_file(target, link) {
            Ok(_) => Ok(()),
            Err(_) => {
                let mut s = String::new();
                s.push_str("@echo off\r\n");
                s.push_str(&format!("\"{}\" %*\r\n", target.display()));
                fs::write(link.with_extension("cmd"), s)?;
                Ok(())
            }
        }
    }
}

pub fn set_default(toolchain_dir: &Path) -> Result<()> {
    let def = default_link()?;
    if def.exists() {
        #[cfg(unix)]
        { let _ = fs::remove_file(&def); }
        #[cfg(windows)]
        { let _ = fs::remove_file(&def); }
    }
    #[cfg(unix)]
    {
        use std::os::unix::fs as unixfs;
        unixfs::symlink(toolchain_dir, &def)?;
    }
    #[cfg(windows)]
    {
        use std::os::windows::fs as winfs;
        winfs::symlink_dir(toolchain_dir, &def).or_else(|_| {
            // Fallback: copier un marqueur
            fs::create_dir_all(&def)?;
            Ok(())
        })?;
    }
    Ok(())
}

pub fn list_toolchains() -> Result<Vec<String>> {
    let tdir = toolchains_dir()?;
    let mut out = Vec::new();
    if tdir.exists() {
        for e in fs::read_dir(tdir)? {
            let e = e?;
            if e.file_type()?.is_dir() {
                out.push(e.file_name().to_string_lossy().to_string());
            }
        }
    }
    out.sort();
    Ok(out)
}

pub fn remove_toolchain(name: &str) -> Result<()> {
    let dir = toolchains_dir()?.join(name);
    if dir.exists() {
        fs::remove_dir_all(&dir)?;
    }
    Ok(())
}

// ================================ Tests ====================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn triples_known() {
        let t = detect_triple();
        assert!(!t.is_empty());
        assert!(t.contains('-') || t == "unknown-unknown-unknown");
    }

    #[test]
    fn fs_layout() {
        let home = vitte_home().unwrap();
        assert!(home.ends_with(".vitte"));
        let tc = toolchains_dir().unwrap();
        let bin = bin_dir().unwrap();
        assert!(tc.exists() && bin.exists());
    }

    #[cfg(feature = "serde")]
    #[test]
    fn manifest_parse_and_find() {
        let json = r#"
        {"channels":[
            {"name":"stable",
             "targets":[
                {"triple":"x86_64-apple-darwin","url":"https://example/sta-mac.tgz","kind":"tar.gz"},
                {"triple":"x86_64-unknown-linux-gnu","url":"https://example/sta-lnx.tgz","kind":"tar.gz"}
             ],
             "components":[]
            }
        ]}"#;
        let m = load_manifest_from_str(json).unwrap();
        let a = find_asset(&m, "stable", "x86_64-apple-darwin").unwrap();
        assert_eq!(a.kind, "tar.gz");
    }
}