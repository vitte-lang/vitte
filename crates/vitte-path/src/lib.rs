//! vitte-path — utilitaires de chemins **ultra complets**
//!
//! Capacités principales (toutes optionnelles par features):
//! - UTF-8 friendly (`camino`): `Utf8Path`, `Utf8PathBuf` (fallback std::path)
//! - Normalisation: nettoyage `.`/`..`, slash, UNC→DOS (Windows), path clean
//! - Résolution: absolu, relatif, `~` (home), env vars (`$VAR`/`%VAR%`)
//! - Glob: correspondances rapides (`globset`) + respect `.gitignore` (`ignore`)
//! - Temp: fichiers et répertoires temporaires (`tempfile`)
//! - Home: répertoire de l’utilisateur (`home`)
//! - Symlinks: création/lecture (std), "même fichier?" (inode/mtime heuristique std-only)
//! - Watcher FS (`notify`): observe les changements sur un chemin
//! - Comparaisons insensibles à la casse (`unicase`)
//! - URL: `file://` ↔ path, percent-encoding stable (`percent-encoding`)
//!
//! Zéro `unsafe`.
//!
//! Exemple rapide:
//! ```no_run
//! use vitte_path as vp;
//! let p = vp::expand("~/.config/vitte").unwrap();
//! let norm = vp::normalize(&p);
//! let url = vp::to_file_url(&norm).unwrap();
//! println!("{} -> {}", norm.display(), url);
//! ```

#![forbid(unsafe_code)]

/* ============================== imports ============================== */

#[cfg(all(not(feature="std"), not(feature="alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.");

#[cfg(feature="alloc-only")]
extern crate alloc;

#[cfg(feature="alloc-only")]
use alloc::{vec::Vec, string::String, format};

#[cfg(feature="std")]
use std::{vec::Vec, string::String, env, fs, io, path::{Path, PathBuf}, time::SystemTime};

#[cfg(feature="serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature="errors")]
use thiserror::Error;

#[cfg(feature="utf8")]
pub use camino::{Utf8Path, Utf8PathBuf};

#[cfg(all(not(feature="utf8"), feature="std"))]
pub type Utf8Path = Path;
#[cfg(all(not(feature="utf8"), feature="std"))]
pub type Utf8PathBuf = PathBuf;

#[cfg(feature="normalize")]
use {path_clean::PathClean, dunce};

#[cfg(feature="glob")]
use globset::{Glob, GlobSet, GlobSetBuilder};

#[cfg(feature="glob")]
use ignore::{WalkBuilder, types::TypesBuilder};

#[cfg(feature="home")]
use home as home_crate;

#[cfg(feature="temp")]
use tempfile as tempfile_crate;

#[cfg(feature="casefold")]
use unicase::UniCase;

#[cfg(feature="watch")]
use notify::{RecommendedWatcher, Watcher, RecursiveMode, Event, Config as NotifyConfig};

#[cfg(feature="percent-enc")]
use percent_encoding::{utf8_percent_encode, percent_decode_str, AsciiSet, CONTROLS};

/* ============================== erreurs ============================== */

#[cfg(feature="errors")]
#[derive(Debug, Error, PartialEq, Eq)]
pub enum PathError {
    #[error("feature not enabled")]
    FeatureMissing,
    #[error("invalid utf8 path")]
    InvalidUtf8,
    #[error("io error: {0}")]
    Io(String),
    #[error("invalid file url")]
    InvalidFileUrl,
    #[error("invalid env var expansion")]
    InvalidEnv,
}
#[cfg(not(feature="errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum PathError { FeatureMissing, InvalidUtf8, Io(String), InvalidFileUrl, InvalidEnv }

pub type Result<T> = core::result::Result<T, PathError>;

#[inline] fn io_err<T>(e: impl core::fmt::Display) -> Result<T> { Err(PathError::Io(e.to_string())) }

/* ============================== encodage URL ============================== */

#[cfg(feature="percent-enc")]
const PATH_ENCODE_SET: &AsciiSet = &CONTROLS
    .add(b' ').add(b'"').add(b'#').add(b'%').add(b'<').add(b'>').add(b'?')
    .add(b'`').add(b'{').add(b'}')
    // garder / et : pour préserver structure file://host/path
;

/* ============================== API de base ============================== */

/// Retourne `true` si le système de fichiers courant est *probablement* case-insensitive.
/// Heuristique: Windows => true, Mac par défaut true, else false.
#[cfg(feature="std")]
pub fn is_probably_case_insensitive() -> bool {
    #[cfg(target_os="windows")] { return true; }
    #[cfg(target_os="macos")]   { return true; }
    #[cfg(not(any(target_os="windows", target_os="macos")))] { false }
}

/// Normalise un chemin en conservant le côté "humain":
/// - nettoie `.` et `..`
/// - convertit séparateurs en standard
/// - sous Windows, corrige les chemins UNC/doubles (via `dunce`)
#[cfg(feature="normalize")]
pub fn normalize<P: AsRef<Path>>(p: P) -> PathBuf {
    let mut out = p.as_ref().to_path_buf().clean();
    #[cfg(target_os="windows")] { out = dunce::simplified(&out).to_path_buf(); }
    out
}
#[cfg(not(feature="normalize"))]
pub fn normalize<P: AsRef<Path>>(p: P) -> PathBuf { p.as_ref().to_path_buf() }

/// Canonicalise de façon *tolérante* : essaie `fs::canonicalize`, sinon fallback `normalize`.
#[cfg(feature="std")]
pub fn canonicalize_soft<P: AsRef<Path>>(p: P) -> PathBuf {
    fs::canonicalize(&p).unwrap_or_else(|_| normalize(p))
}

/// Développe `~` (home) et variables d’environnement (`$VAR` / `%VAR%` Windows).
#[cfg(feature="std")]
pub fn expand(s: &str) -> Result<PathBuf> {
    let s = expand_home(s)?;
    let s = expand_env(&s)?;
    Ok(PathBuf::from(s))
}

#[cfg(feature="std")]
fn expand_home(input: &str) -> Result<String> {
    if !input.starts_with('~') { return Ok(input.to_string()); }
    #[cfg(feature="home")]
    let home = home_crate::home_dir();
    #[cfg(not(feature="home"))]
    let home: Option<PathBuf> = env::var_os("HOME").map(PathBuf::from)
        .or_else(|| env::var_os("USERPROFILE").map(PathBuf::from));
    let Some(h) = home else { return Err(PathError::InvalidEnv); };
    let mut out = h.to_string_lossy().to_string();
    if input.len()>1 {
        if !out.ends_with(std::path::MAIN_SEPARATOR) { out.push(std::path::MAIN_SEPARATOR); }
        out.push_str(&input[2.min(input.len())..]);
    }
    Ok(out)
}

#[cfg(feature="std")]
fn expand_env(input: &str) -> Result<String> {
    #[cfg(target_os="windows")]
    {
        // %VAR% style
        let mut out = String::with_capacity(input.len());
        let mut i = 0;
        let b = input.as_bytes();
        while i < b.len() {
            if b[i]==b'%' {
                if let Some(j)=input[i+1..].find('%') {
                    let name = &input[i+1..i+1+j];
                    let val = env::var(name).unwrap_or_default();
                    out.push_str(&val);
                    i += j+2; continue;
                }
            }
            out.push(b[i] as char); i+=1;
        }
        return Ok(out);
    }
    #[cfg(not(target_os="windows"))]
    {
        // $VAR or ${VAR}
        let mut out = String::with_capacity(input.len());
        let mut chars = input.chars().peekable();
        while let Some(c)=chars.next() {
            if c=='$' {
                if let Some('{') = chars.peek().copied() {
                    chars.next();
                    let mut name=String::new();
                    while let Some(ch)=chars.next() {
                        if ch=='}' { break; }
                        name.push(ch);
                    }
                    let val=env::var(&name).unwrap_or_default();
                    out.push_str(&val);
                } else {
                    let mut name=String::new();
                    while let Some(&ch)=chars.peek() {
                        if ch.is_ascii_alphanumeric() || ch=='_' { name.push(ch); chars.next(); } else { break; }
                    }
                    let val=env::var(&name).unwrap_or_default();
                    out.push_str(&val);
                }
            } else { out.push(c); }
        }
        Ok(out)
    }
}

/* ============================== Glob ============================== */

/// Construit un `GlobSet` compact avec des patterns style `.gitignore` (`**/*.rs`, etc.).
#[cfg(feature="glob")]
pub fn build_glob_set(patterns: &[&str]) -> Result<GlobSet> {
    let mut b = GlobSetBuilder::new();
    for p in patterns {
        let g = Glob::new(p).map_err(|e| PathError::Io(e.to_string()))?;
        b.add(g);
    }
    b.build().map_err(|e| PathError::Io(e.to_string()))
}

/// Itère sur un répertoire avec respect `.gitignore` et filtre par `GlobSet`.
/// `root` peut être fichier ou dossier. Renvoie chemins **absolus** si possible.
#[cfg(feature="glob")]
pub fn walk_with_glob<P: AsRef<Path>>(root: P, set: &GlobSet) -> Result<Vec<PathBuf>> {
    let mut out = Vec::new();
    let mut wb = WalkBuilder::new(root.as_ref());
    wb.hidden(false).ignore(true).git_global(true).git_exclude(true).git_ignore(true);
    let it = wb.build();
    for r in it {
        let ent = r.map_err(|e| PathError::Io(e.to_string()))?;
        let p = ent.path();
        if ent.file_type().map(|t| t.is_dir()).unwrap_or(false) { continue; }
        if set.is_match(p) {
            out.push(canonicalize_soft(p));
        }
    }
    Ok(out)
}

/* ============================== Temp ============================== */

/// Fichier temporaire créé et retourné avec son `PathBuf`.
#[cfg(all(feature="temp", feature="std"))]
pub fn temp_file() -> Result<(tempfile_crate::NamedTempFile, PathBuf)> {
    let f = tempfile_crate::NamedTempFile::new().map_err(|e| PathError::Io(e.to_string()))?;
    let p = f.path().to_path_buf();
    Ok((f, p))
}

/// Répertoire temporaire unique.
#[cfg(all(feature="temp", feature="std"))]
pub fn temp_dir() -> Result<tempfile_crate::TempDir> {
    tempfile_crate::tempdir().map_err(|e| PathError::Io(e.to_string()))
}

/* ============================== Home ============================== */

/// Répertoire HOME de l’utilisateur.
#[cfg(all(feature="home", feature="std"))]
pub fn home_dir() -> Option<PathBuf> { home_crate::home_dir() }

/* ============================== Symlinks ============================== */

/// Crée un lien symbolique `link -> target`.
#[cfg(all(feature="std", unix))]
pub fn symlink<P: AsRef<Path>, Q: AsRef<Path>>(target: P, link: Q) -> Result<()> {
    std::os::unix::fs::symlink(target, link).map_err(|e| PathError::Io(e.to_string()))
}
#[cfg(all(feature="std", windows))]
pub fn symlink<P: AsRef<Path>, Q: AsRef<Path>>(target: P, link: Q) -> Result<()> {
    use std::os::windows::fs as wfs;
    let md = fs::metadata(&target).ok();
    let res = match md.map(|m| m.is_dir()).unwrap_or(false) {
        true  => wfs::symlink_dir(target, link),
        false => wfs::symlink_file(target, link),
    };
    res.map_err(|e| PathError::Io(e.to_string()))
}

/// Lit la cible d’un symlink si présent.
#[cfg(feature="std")]
pub fn read_link<P: AsRef<Path>>(p: P) -> Result<PathBuf> {
    fs::read_link(p).map_err(|e| PathError::Io(e.to_string()))
}

/// Heuristique "même fichier?" via canonicalize + metadata.
/// Ne garantit pas 100% (réseaux, permissions).
#[cfg(feature="std")]
pub fn is_same_file<P: AsRef<Path>, Q: AsRef<Path>>(a: P, b: Q) -> bool {
    let (a_path, b_path) = (a.as_ref(), b.as_ref());
    let (ca, cb) = (fs::canonicalize(a_path), fs::canonicalize(b_path));
    if let (Ok(pa), Ok(pb)) = (ca, cb) { if pa == pb { return true; } }
    let (ma, mb) = (fs::metadata(a_path), fs::metadata(b_path));
    if let (Ok(ma), Ok(mb)) = (ma, mb) {
        #[cfg(unix)]
        { use std::os::unix::fs::MetadataExt; return ma.ino()==mb.ino() && ma.dev()==mb.dev() && ma.len()==mb.len(); }
        #[cfg(not(unix))]
        { return ma.len()==mb.len() && ma.modified().ok()==mb.modified().ok(); }
    }
    false
}

/* ============================== Watcher ============================== */

/// Observe les changements sur `path`. Appelle `on_event` pour chaque `notify::Event`.
/// Bloquant tant que `keep_running()` renvoie true.
/// Exemple:
/// ```no_run
/// # #[cfg(feature="watch")]
/// # {
/// use std::sync::{Arc, atomic::{AtomicBool, Ordering}};
/// let running = Arc::new(AtomicBool::new(true));
/// let flag = running.clone();
/// std::thread::spawn(move || { std::thread::sleep(std::time::Duration::from_secs(2)); flag.store(false, Ordering::SeqCst); });
/// vitte_path::watch_path(".", move |e| println!("{:?}", e), move || running.load(Ordering::SeqCst)).unwrap();
/// # }
/// ```
#[cfg(all(feature="watch", feature="std"))]
pub fn watch_path<P, F, G>(path: P, mut on_event: F, mut keep_running: G) -> Result<()>
where
    P: AsRef<Path>,
    F: FnMut(Event) + Send + 'static,
    G: FnMut() -> bool,
{
    use std::sync::mpsc::channel;
    let (tx, rx) = channel();
    let mut watcher = RecommendedWatcher::new(tx, NotifyConfig::default())
        .map_err(|e| PathError::Io(e.to_string()))?;
    watcher.watch(path.as_ref(), RecursiveMode::Recursive)
        .map_err(|e| PathError::Io(e.to_string()))?;
    while keep_running() {
        match rx.recv_timeout(std::time::Duration::from_millis(200)) {
            Ok(ev) => { if let Ok(e)=ev { on_event(e); } },
            Err(std::sync::mpsc::RecvTimeoutError::Timeout) => {},
            Err(e) => return io_err(e),
        }
    }
    Ok(())
}

/* ============================== Comparaisons ============================== */

/// Egalité de chemins insensible à la casse si feature `casefold`.
#[cfg(all(feature="casefold", feature="std"))]
pub fn eq_casefold<P: AsRef<Path>, Q: AsRef<Path>>(a: P, b: Q) -> bool {
    let sa = a.as_ref().to_string_lossy().into_owned();
    let sb = b.as_ref().to_string_lossy().into_owned();
    UniCase::new(sa) == UniCase::new(sb)
}
#[cfg(any(not(feature="casefold"), not(feature="std")))]
pub fn eq_casefold<P: AsRef<Path>, Q: AsRef<Path>>(_a: P, _b: Q) -> bool { false }

/* ============================== URL file:// ============================== */

/// Convertit un path absolu en URL `file://...`.
#[cfg(all(feature="percent-enc", feature="std"))]
pub fn to_file_url<P: AsRef<Path>>(p: P) -> Result<String> {
    let path = fs::canonicalize(p.as_ref()).unwrap_or_else(|_| p.as_ref().to_path_buf());
    if !path.is_absolute() { return Err(PathError::InvalidFileUrl); }
    let s = path.to_string_lossy();
    #[cfg(target_os="windows")]
    {
        // file:///<drive>:/path
        let s = s.replace('\\', "/");
        let enc = utf8_percent_encode(&s, PATH_ENCODE_SET).to_string();
        return Ok(format!("file:///{}", enc.trim_start_matches('/')));
    }
    #[cfg(not(target_os="windows"))]
    {
        let enc = utf8_percent_encode(&s, PATH_ENCODE_SET).to_string();
        Ok(format!("file://{}", if enc.starts_with('/') { enc } else { format!("/{}", enc) }))
    }
}

/// Convertit une URL `file://` vers PathBuf.
#[cfg(all(feature="percent-enc", feature="std"))]
pub fn from_file_url(url: &str) -> Result<PathBuf> {
    if !url.to_ascii_lowercase().starts_with("file://") { return Err(PathError::InvalidFileUrl); }
    let mut rest = &url[7..]; // après file://
    // Autoriser file:///path et file://HOST/path; on ignore l'host si localhost ou vide
    if rest.starts_with('/') {
        // file:///...
        while rest.starts_with('/') { rest = &rest[1..]; } // trim leading
        #[cfg(target_os="windows")]
        {
            // attendu: C:/...
            let decoded = percent_decode_str(rest).decode_utf8().map_err(|_| PathError::InvalidUtf8)?.to_string();
            let s = decoded.replace('/', "\\");
            return Ok(PathBuf::from(s));
        }
        #[cfg(not(target_os="windows"))]
        {
            let decoded = format!("/{}", percent_decode_str(rest).decode_utf8().map_err(|_| PathError::InvalidUtf8)?);
            return Ok(PathBuf::from(decoded));
        }
    } else {
        // host/path
        let parts: Vec<_> = rest.splitn(2, '/').collect();
        let _host = parts.get(0).copied().unwrap_or("");
        let rel = parts.get(1).copied().unwrap_or("");
        #[cfg(target_os="windows")]
        {
            let decoded = percent_decode_str(rel).decode_utf8().map_err(|_| PathError::InvalidUtf8)?.to_string();
            return Ok(PathBuf::from(decoded.replace('/', "\\")));
        }
        #[cfg(not(target_os="windows"))]
        {
            let decoded = percent_decode_str(rel).decode_utf8().map_err(|_| PathError::InvalidUtf8)?;
            return Ok(PathBuf::from(format!("/{}", decoded)));
        }
    }
}

/* ============================== Utils ============================== */

/// Joint proprement des segments de chemin.
#[cfg(feature="std")]
pub fn join<P: AsRef<Path>>(base: P, segments: &[&str]) -> PathBuf {
    let mut p = base.as_ref().to_path_buf();
    for s in segments { p.push(s); }
    normalize(p)
}

/// Assure que le dossier existe (mkdir -p).
#[cfg(feature="std")]
pub fn ensure_dir<P: AsRef<Path>>(p: P) -> Result<()> {
    fs::create_dir_all(&p).map_err(|e| PathError::Io(e.to_string()))
}

/// Lit fichier texte UTF-8 complet.
#[cfg(feature="std")]
pub fn read_to_string<P: AsRef<Path>>(p: P) -> Result<String> {
    fs::read_to_string(p).map_err(|e| PathError::Io(e.to_string()))
}

/// Ecrit fichier texte, crée dossiers si besoin.
#[cfg(feature="std")]
pub fn write_string<P: AsRef<Path>>(p: P, s: &str) -> Result<()> {
    if let Some(dir) = p.as_ref().parent() { fs::create_dir_all(dir).map_err(|e| PathError::Io(e.to_string()))?; }
    fs::write(p, s).map_err(|e| PathError::Io(e.to_string()))
}

/* ============================== Tests ============================== */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn normalize_basic() {
        let p = PathBuf::from("a/./b/../c");
        let n = normalize(&p);
        assert!(n.ends_with("a/c"));
    }

    #[cfg(feature="std")]
    #[test]
    fn join_and_ensure() {
        let base = env::temp_dir();
        let p = join(&base, &["vitte-path-test", "nested"]);
        ensure_dir(&p).unwrap();
        assert!(p.exists());
        // cleanup best-effort
        let _ = fs::remove_dir_all(base.join("vitte-path-test"));
    }

    #[cfg(all(feature="std", feature="percent-enc"))]
    #[test]
    fn file_url_roundtrip() {
        let base = env::temp_dir();
        let p = join(&base, &["vitte-path-url.txt"]);
        write_string(&p, "x").unwrap();
        let url = to_file_url(&p).unwrap();
        let back = from_file_url(&url).unwrap();
        assert!(is_same_file(&p, &back) || p == back);
        let _ = fs::remove_file(&p);
    }

    #[cfg(feature="glob")]
    #[test]
    fn build_globs() {
        let set = build_glob_set(&["**/*.rs", "!target/**"]).unwrap();
        assert!(set.len() >= 1);
    }

    #[cfg(feature="std")]
    #[test]
    fn expand_env_home() {
        #[cfg(feature="home")]
        let _ = home_dir(); // smoke
        let s = expand("$DOES_NOT_EXIST/xx").unwrap();
        assert!(s.ends_with("xx"));
    }

    #[cfg(all(feature="temp", feature="std"))]
    #[test]
    fn tempfile_dir() {
        let (f, p) = temp_file().unwrap();
        assert!(p.exists());
        drop(f);
        let _ = fs::remove_file(p);
        let td = temp_dir().unwrap();
        assert!(td.path().exists());
    }
}
