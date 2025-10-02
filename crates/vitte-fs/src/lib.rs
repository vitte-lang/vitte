//! vitte-fs — Abstractions FS haut-niveau pour Vitte
//!
//! Couvre I/O synchrones et optionnellement asynchrones, chemins, sécurité d’écriture,
//! gestion répertoires, métadonnées, liens symboliques, JSON (optionnel), et utilitaires.
//!
//! Features :
//! - `std` (par défaut) : API synchrone basée sur std::fs
//! - `async`            : compléments asynchrones basés sur tokio::fs
//! - `serde`/`serde_json` (optionnelles) : lecture/écriture JSON
//!
//! Design :
//! - Erreurs unifiées via `FsError`.
//! - Fonctions "safe by default" : écritures atomiques, création de dossiers récursive.
//! - Helpers de chemin : normalisation, canonicalisation, relativisation, extensions, etc.

#![deny(missing_docs)]
#![forbid(unsafe_code)]

use std::borrow::Cow;
use std::ffi::OsStr;
use std::fmt;
use std::io::{Read, Write};
use std::path::{Component, Path, PathBuf};
use std::{fs, io};

use path_absolutize::Absolutize;
use thiserror::Error;

/// Résultat standard du crate.
pub type FsResult<T> = Result<T, FsError>;

/// Erreurs unifiées.
#[derive(Error, Debug)]
pub enum FsError {
    /// Erreur I/O brute.
    #[error("io error: {0}")]
    Io(#[from] io::Error),

    /// Chemin invalide pour l’opération demandée.
    #[error("invalid path: {0}")]
    InvalidPath(String),

    /// Opération non supportée sur cette plateforme / build.
    #[error("unsupported operation: {0}")]
    Unsupported(&'static str),

    /// Erreur de sérialisation JSON (activée par feature).
    #[cfg(feature = "serde_json")]
    #[error("json error: {0}")]
    Json(#[from] serde_json::Error),
}

/// Métadonnées simplifiées.
#[derive(Clone, Debug)]
pub struct FileInfo {
    /// Vrai si fichier ordinaire.
    pub is_file: bool,
    /// Vrai si répertoire.
    pub is_dir: bool,
    /// Vrai si lien symbolique.
    pub is_symlink: bool,
    /// Taille, si disponible.
    pub len: Option<u64>,
    /// Permissions en lecture seule.
    pub readonly: bool,
    /// Chemin absolu si obtenu via helper.
    pub abs_path: Option<PathBuf>,
}

impl From<fs::Metadata> for FileInfo {
    fn from(m: fs::Metadata) -> Self {
        FileInfo {
            is_file: m.is_file(),
            is_dir: m.is_dir(),
            is_symlink: false, // rempli par appelant si nécessaire
            len: Some(m.len()),
            readonly: m.permissions().readonly(),
            abs_path: None,
        }
    }
}

/// Options pour listage de répertoire.
#[derive(Clone, Debug)]
pub struct ListOptions {
    /// Inclure les fichiers.
    pub files: bool,
    /// Inclure les dossiers.
    pub dirs: bool,
    /// Suivre les liens symboliques.
    pub follow_symlinks: bool,
    /// Filtre d’extension (sans le point). Vide = tout.
    pub ext_filter: Vec<String>,
    /// Profondeur maximale (0 = seulement le répertoire, 1 = + enfants, etc.). None = illimité.
    pub max_depth: Option<usize>,
}

impl Default for ListOptions {
    fn default() -> Self {
        Self {
            files: true,
            dirs: true,
            follow_symlinks: false,
            ext_filter: Vec::new(),
            max_depth: Some(1),
        }
    }
}

/// Lecture de fichier entière (binaire).
pub fn read_file<P: AsRef<Path>>(path: P) -> FsResult<Vec<u8>> {
    Ok(fs::read(path)?)
}

/// Lecture de fichier entière (texte, UTF-8).
pub fn read_text<P: AsRef<Path>>(path: P) -> FsResult<String> {
    Ok(fs::read_to_string(path)?)
}

/// Écriture atomique binaire. Crée le dossier parent si nécessaire.
/// Stratégie : écrire vers `<file>.tmp.<pid>.<rand>` puis `rename`.
pub fn write_file_atomic<P: AsRef<Path>>(path: P, data: &[u8]) -> FsResult<()> {
    let path = path.as_ref();
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let tmp = tmp_sibling(path, "tmp")?;
    {
        let mut f = fs::File::create(&tmp)?;
        f.write_all(data)?;
        f.sync_all()?;
    }
    // rename est atomique sur la plupart des FS
    fs::rename(&tmp, path)?;
    Ok(())
}

/// Écriture atomique texte UTF-8.
pub fn write_text_atomic<P: AsRef<Path>>(path: P, text: &str) -> FsResult<()> {
    write_file_atomic(path, text.as_bytes())
}

/// Ajout (append) binaire. Crée le dossier parent.
pub fn append_file<P: AsRef<Path>>(path: P, data: &[u8]) -> FsResult<()> {
    let path = path.as_ref();
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let mut f = fs::OpenOptions::new().create(true).append(true).open(path)?;
    f.write_all(data)?;
    Ok(())
}

/// Ajout (append) texte.
pub fn append_text<P: AsRef<Path>>(path: P, text: &str) -> FsResult<()> {
    append_file(path, text.as_bytes())
}

/// Copie un fichier (écrase si `overwrite`).
pub fn copy_file<P: AsRef<Path>>(from: P, to: P, overwrite: bool) -> FsResult<u64> {
    let from = from.as_ref();
    let to = to.as_ref();
    if !overwrite && to.exists() {
        return Err(FsError::Io(io::Error::new(
            io::ErrorKind::AlreadyExists,
            "destination exists",
        )));
    }
    if let Some(parent) = to.parent() {
        fs::create_dir_all(parent)?;
    }
    if to.exists() {
        fs::remove_file(to)?;
    }
    Ok(fs::copy(from, to)?)
}

/// Déplacement/rename. Crée le dossier parent destination si nécessaire.
pub fn move_path<P: AsRef<Path>>(from: P, to: P) -> FsResult<()> {
    let from = from.as_ref();
    let to = to.as_ref();
    if let Some(parent) = to.parent() {
        fs::create_dir_all(parent)?;
    }
    fs::rename(from, to)?;
    Ok(())
}

/// Supprime fichier ou dossier (récursif).
pub fn remove_any<P: AsRef<Path>>(path: P) -> FsResult<()> {
    let path = path.as_ref();
    match fs::symlink_metadata(path) {
        Ok(md) if md.is_dir() => {
            fs::remove_dir_all(path)?;
        }
        Ok(_) => {
            fs::remove_file(path)?;
        }
        Err(e) if e.kind() == io::ErrorKind::NotFound => {}
        Err(e) => return Err(e.into()),
    }
    Ok(())
}

/// S’assure que le répertoire existe.
pub fn ensure_dir<P: AsRef<Path>>(dir: P) -> FsResult<()> {
    fs::create_dir_all(dir)?;
    Ok(())
}

/// Nettoie un répertoire (supprime récursivement le contenu mais pas le dossier lui-même).
pub fn clean_dir<P: AsRef<Path>>(dir: P) -> FsResult<()> {
    let dir = dir.as_ref();
    if !dir.is_dir() {
        return Err(FsError::InvalidPath(format!(
            "not a directory: {}",
            dir.display()
        )));
    }
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let p = entry.path();
        remove_any(p)?;
    }
    Ok(())
}

/// Retourne `FileInfo` d’un chemin.
pub fn stat<P: AsRef<Path>>(path: P) -> FsResult<FileInfo> {
    let path = path.as_ref();
    let md = fs::metadata(path).or_else(|_| fs::symlink_metadata(path))?;
    let mut info = FileInfo::from(md);
    info.is_symlink = fs::symlink_metadata(path)
        .map(|m| m.file_type().is_symlink())
        .unwrap_or(false);
    info.abs_path = canonicalize(path).ok();
    Ok(info)
}

/// Liste un répertoire avec options. Profondeur >= 1 => descend récursivement.
pub fn list_dir<P: AsRef<Path>>(root: P, opts: &ListOptions) -> FsResult<Vec<PathBuf>> {
    let mut out = Vec::new();
    let root = root.as_ref();
    if !root.is_dir() {
        return Err(FsError::InvalidPath(format!(
            "not a directory: {}",
            root.display()
        )));
    }
    walk_internal(root, opts, 0, &mut out)?;
    Ok(out)
}

fn walk_internal(dir: &Path, opts: &ListOptions, depth: usize, out: &mut Vec<PathBuf>) -> FsResult<()> {
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let p = entry.path();

        let md_link = fs::symlink_metadata(&p)?;
        let is_symlink = md_link.file_type().is_symlink();
        let md = if is_symlink && opts.follow_symlinks {
            match fs::metadata(&p) {
                Ok(m) => m,
                Err(_) => md_link, // cassé
            }
        } else {
            md_link
        };

        let is_file = md.is_file();
        let is_dir = md.is_dir();

        if is_file && !opts.files {
            // skip
        } else if is_dir && !opts.dirs {
            // skip
        } else if !opts.ext_filter.is_empty() && is_file {
            let pass = p
                .extension()
                .and_then(OsStr::to_str)
                .map(|e| opts.ext_filter.iter().any(|x| e.eq_ignore_ascii_case(x)))
                .unwrap_or(false);
            if pass {
                out.push(p.clone());
            }
        } else {
            out.push(p.clone());
        }

        if is_dir {
            let next = depth + 1;
            if opts.max_depth.map_or(true, |m| next <= m) {
                walk_internal(&p, opts, next, out)?;
            }
        }
    }
    Ok(())
}

/// Glob très simple par suffixe d’extension (ex: ["rs","toml"]) sur un dossier, non récursif par défaut.
pub fn glob_ext<P: AsRef<Path>>(dir: P, exts: &[&str]) -> FsResult<Vec<PathBuf>> {
    let opts = ListOptions {
        files: true,
        dirs: false,
        follow_symlinks: false,
        ext_filter: exts.iter().map(|s| s.to_string()).collect(),
        max_depth: Some(1),
    };
    list_dir(dir, &opts)
}

/// Normalise un chemin (résout `.` et `..` sans accès disque).
pub fn normalize<P: AsRef<Path>>(path: P) -> PathBuf {
    let mut stack: Vec<Component> = Vec::new();
    for c in path.as_ref().components() {
        match c {
            Component::CurDir => {}
            Component::ParentDir => {
                if let Some(last) = stack.last() {
                    match last {
                        Component::RootDir | Component::Prefix(_) => {}
                        _ => {
                            stack.pop();
                        }
                    }
                }
            }
            _ => stack.push(c),
        }
    }
    let mut out = PathBuf::new();
    for c in stack {
        out.push(c.as_os_str());
    }
    out
}

/// Canonicalise en ignorant erreurs symboliques quand possible.
pub fn canonicalize<P: AsRef<Path>>(path: P) -> FsResult<PathBuf> {
    // std::fs::canonicalize échoue si symlink cassé. On tente d’abord absolutize.
    let abs = absolutize(path)?;
    match fs::canonicalize(&abs) {
        Ok(c) => Ok(c),
        Err(_) => Ok(abs),
    }
}

/// Transforme en chemin absolu sans toucher au FS (autant que possible).
pub fn absolutize<P: AsRef<Path>>(path: P) -> FsResult<PathBuf> {
    let p = path.as_ref();
    let ab = p.absolutize().map_err(|e| io::Error::new(io::ErrorKind::Other, e.to_string()))?;
    Ok(ab.into_owned())
}

/// Donne un nom "sûr" de fichier (retire caractères problématiques).
pub fn safe_file_name<S: AsRef<str>>(name: S) -> String {
    let s = name.as_ref();
    let mut out = String::with_capacity(s.len());
    for ch in s.chars() {
        let ok = ch.is_alphanumeric() || "-_ .,@+=()[]{}".contains(ch);
        out.push(if ok { ch } else { '_' });
    }
    // trim espaces multiples
    out.split_whitespace().collect::<Vec<_>>().join(" ")
}

/// Joint segments de chemin en filtrant segments vides.
pub fn join<S: AsRef<OsStr>>(base: &Path, segments: &[S]) -> PathBuf {
    let mut p = PathBuf::from(base);
    for s in segments {
        if !s.as_ref().is_empty() {
            p.push(s);
        }
    }
    p
}

/// Relativise `path` par rapport à `base` si possible.
pub fn relativize<P: AsRef<Path>, B: AsRef<Path>>(path: P, base: B) -> FsResult<PathBuf> {
    let p = canonicalize(path)?;
    let b = canonicalize(base)?;
    pathdiff::diff_paths(p, b).ok_or_else(|| FsError::InvalidPath("cannot relativize".into()))
}

/// Lecture JSON en type T (feature `serde_json`).
#[cfg(feature = "serde_json")]
pub fn read_json<P: AsRef<Path>, T: serde::de::DeserializeOwned>(path: P) -> FsResult<T> {
    let s = read_text(path)?;
    Ok(serde_json::from_str(&s)?)
}

/// Écriture JSON pretty (feature `serde_json`).
#[cfg(feature = "serde_json")]
pub fn write_json_pretty<P: AsRef<Path>, T: serde::Serialize>(path: P, value: &T) -> FsResult<()> {
    let s = serde_json::to_string_pretty(value)?;
    write_text_atomic(path, &s)
}

/// Écriture JSON compact (feature `serde_json`).
#[cfg(feature = "serde_json")]
pub fn write_json<P: AsRef<Path>, T: serde::Serialize>(path: P, value: &T) -> FsResult<()> {
    let s = serde_json::to_string(value)?;
    write_text_atomic(path, &s)
}

/// Crée un lien symbolique. Sur Windows choisit file/dir selon source si existante.
pub fn symlink<P: AsRef<Path>, Q: AsRef<Path>>(src: P, dst: Q) -> FsResult<()> {
    let src = src.as_ref();
    let dst = dst.as_ref();
    #[cfg(unix)]
    {
        std::os::unix::fs::symlink(src, dst)?;
        Ok(())
    }
    #[cfg(windows)]
    {
        let meta = fs::metadata(src);
        if meta.as_ref().map(|m| m.is_dir()).unwrap_or(false) {
            std::os::windows::fs::symlink_dir(src, dst)?;
        } else {
            std::os::windows::fs::symlink_file(src, dst)?;
        }
        Ok(())
    }
}

/// Lit le chemin cible d’un lien symbolique.
pub fn read_link<P: AsRef<Path>>(path: P) -> FsResult<PathBuf> {
    Ok(fs::read_link(path)?)
}

/// Détecte si un chemin semble "caché" (dotfile sur Unix, attribut "hidden" ignoré).
pub fn is_hidden<P: AsRef<Path>>(path: P) -> bool {
    path.as_ref()
        .file_name()
        .and_then(OsStr::to_str)
        .map(|s| s.starts_with('.'))
        .unwrap_or(false)
}

/// Retourne le dossier home de l’utilisateur.
pub fn home_dir() -> Option<PathBuf> {
    dirs_next::home_dir()
}

/// Retourne le dossier temp.
pub fn temp_dir() -> PathBuf {
    std::env::temp_dir()
}

/// Construit un chemin frère temporaire unique de `path`.
fn tmp_sibling(path: &Path, tag: &str) -> FsResult<PathBuf> {
    use rand::{distributions::Alphanumeric, thread_rng, Rng};
    let parent = path.parent().ok_or_else(|| FsError::InvalidPath("no parent".into()))?;
    let stem = path.file_name().unwrap_or_else(|| OsStr::new("file"));
    let rand: String = thread_rng().sample_iter(&Alphanumeric).take(8).map(char::from).collect();
    let tmp = parent.join(format!(
        "{}.{}.{}.{}",
        stem.to_string_lossy(),
        tag,
        std::process::id(),
        rand
    ));
    Ok(tmp)
}

/// Lit un fichier ligne par ligne en itérateur de String.
/// Attention : charge en mémoire chunk par chunk. Usage simple.
pub fn read_lines<P: AsRef<Path>>(path: P) -> FsResult<impl Iterator<Item = FsResult<String>>> {
    let f = fs::File::open(path)?;
    let reader = io::BufReader::new(f);
    Ok(reader
        .lines()
        .map(|r| r.map_err(FsError::from)))
}

/// Détecte si un path a l’une des extensions fournies.
pub fn has_any_extension<P: AsRef<Path>>(path: P, exts: &[&str]) -> bool {
    path.as_ref()
        .extension()
        .and_then(OsStr::to_str)
        .map(|e| exts.iter().any(|x| e.eq_ignore_ascii_case(x)))
        .unwrap_or(false)
}

/// Change ou ajoute l’extension d’un chemin.
pub fn with_extension<P: AsRef<Path>>(path: P, ext: &str) -> PathBuf {
    let mut p = PathBuf::from(path.as_ref());
    p.set_extension(ext);
    p
}

/// Retourne nom de fichier sans extension.
pub fn file_stem<P: AsRef<Path>>(path: P) -> Option<String> {
    path.as_ref()
        .file_stem()
        .and_then(OsStr::to_str)
        .map(|s| s.to_string())
}

/// Crée un fichier vide s’il n’existe pas, sinon touche la mtime.
pub fn touch<P: AsRef<Path>>(path: P) -> FsResult<()> {
    use std::time::{SystemTime, UNIX_EPOCH};
    if path.as_ref().exists() {
        let now = filetime::FileTime::from_system_time(SystemTime::now());
        filetime::set_file_times(path.as_ref(), now, now)?;
        Ok(())
    } else {
        write_file_atomic(path, &[])
    }
}

/// Définit le bit lecture seule.
pub fn set_readonly<P: AsRef<Path>>(path: P, readonly: bool) -> FsResult<()> {
    let mut perms = fs::metadata(&path)?.permissions();
    perms.set_readonly(readonly);
    fs::set_permissions(path, perms)?;
    Ok(())
}

/// Récupère la taille d’un fichier si existant.
pub fn file_size<P: AsRef<Path>>(path: P) -> FsResult<u64> {
    Ok(fs::metadata(path)?.len())
}

/// ====== Section async (feature "async") ===================================
#[cfg(feature = "async")]
pub mod aio {
    //! API asynchrone basée sur tokio::fs
    use super::*;
    use tokio::io::{AsyncReadExt, AsyncWriteExt};

    /// Lecture fichier binaire.
    pub async fn read_file<P: AsRef<Path>>(path: P) -> FsResult<Vec<u8>> {
        let mut f = tokio::fs::File::open(path).await?;
        let mut v = Vec::new();
        f.read_to_end(&mut v).await?;
        Ok(v)
    }

    /// Lecture fichier texte UTF-8.
    pub async fn read_text<P: AsRef<Path>>(path: P) -> FsResult<String> {
        Ok(tokio::fs::read_to_string(path).await?)
    }

    /// Écriture atomique binaire.
    pub async fn write_file_atomic<P: AsRef<Path>>(path: P, data: &[u8]) -> FsResult<()> {
        let path = path.as_ref();
        if let Some(parent) = path.parent() {
            tokio::fs::create_dir_all(parent).await?;
        }
        let tmp = super::tmp_sibling(path, "tmp")?;
        {
            let mut f = tokio::fs::File::create(&tmp).await?;
            f.write_all(data).await?;
            f.flush().await?;
        }
        tokio::fs::rename(&tmp, path).await?;
        Ok(())
    }

    /// Écriture atomique texte.
    pub async fn write_text_atomic<P: AsRef<Path>>(path: P, text: &str) -> FsResult<()> {
        write_file_atomic(path, text.as_bytes()).await
    }

    /// S’assure que le répertoire existe.
    pub async fn ensure_dir<P: AsRef<Path>>(dir: P) -> FsResult<()> {
        tokio::fs::create_dir_all(dir).await?;
        Ok(())
    }

    /// Suppression récursive.
    pub async fn remove_any<P: AsRef<Path>>(path: P) -> FsResult<()> {
        let path = path.as_ref();
        match tokio::fs::symlink_metadata(path).await {
            Ok(md) if md.is_dir() => tokio::fs::remove_dir_all(path).await?,
            Ok(_) => tokio::fs::remove_file(path).await?,
            Err(e) if e.kind() == io::ErrorKind::NotFound => {}
            Err(e) => return Err(e.into()),
        }
        Ok(())
    }

    /// Copie fichier (overwrite contrôlé).
    pub async fn copy_file<P: AsRef<Path>>(from: P, to: P, overwrite: bool) -> FsResult<u64> {
        let to = to.as_ref();
        if !overwrite && to.exists() {
            return Err(FsError::Io(io::Error::new(
                io::ErrorKind::AlreadyExists,
                "destination exists",
            )));
        }
        if let Some(parent) = to.parent() {
            tokio::fs::create_dir_all(parent).await?;
        }
        if to.exists() {
            tokio::fs::remove_file(to).await?;
        }
        // tokio n’a pas de copy renvoyant u64; fallback sync (petit coût).
        super::copy_file(from, to, true)
    }
}

/// ====== Tests de fumée =====================================================
#[cfg(test)]
mod tests {
    use super::*;
    use std::time::SystemTime;

    #[test]
    fn safe_name_basic() {
        assert_eq!(safe_file_name("a/b:c*?.txt"), "a_b_c___.txt");
        assert_eq!(safe_file_name("   a   b   "), "a b");
    }

    #[test]
    fn io_cycle() {
        let dir = temp_dir().join(format!("vitte-fs-test-{}", SystemTime::now().duration_since(std::time::UNIX_EPOCH).unwrap().as_millis()));
        ensure_dir(&dir).unwrap();

        let f = dir.join("x.txt");
        write_text_atomic(&f, "hello").unwrap();
        assert_eq!(read_text(&f).unwrap(), "hello");

        append_text(&f, "\nworld").unwrap();
        let s = read_text(&f).unwrap();
        assert!(s.contains("world"));

        let g = dir.join("y.txt");
        copy_file(&f, &g, true).unwrap();
        assert!(g.exists());

        move_path(&g, dir.join("z.txt")).unwrap();
        assert!(dir.join("z.txt").exists());

        set_readonly(&f, true).unwrap();
        let info = stat(&f).unwrap();
        assert!(info.readonly);

        remove_any(&dir).unwrap();
        assert!(!dir.exists());
    }

    #[test]
    fn list_and_glob() {
        let base = temp_dir().join("vitte-fs-list");
        ensure_dir(&base).unwrap();
        write_text_atomic(base.join("a.rs"), "x").unwrap();
        write_text_atomic(base.join("b.toml"), "x").unwrap();
        ensure_dir(base.join("sub")).unwrap();

        let v = glob_ext(&base, &["rs"]).unwrap();
        assert_eq!(v.len(), 1);

        let all = list_dir(&base, &ListOptions::default()).unwrap();
        assert!(all.len() >= 2);

        remove_any(&base).unwrap();
    }
}

// ------------------------- Dépendances "runtime" non-exportées -------------------------
// pathdiff: calcul de chemins relatifs robustes
// filetime: set_file_times
// dirs-next: home_dir
// rand: tmp_sibling
// Ces crates sont référencées via Cargo.toml du crate parent.