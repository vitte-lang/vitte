// vitte-inc/src/util.rs
//! Utilitaires ultra complets pour l’incrémentalité.
//!
//! Points clés :
//! - Empreintes de fichiers stables (FNV-1a 64 bits) : contenu complet, rapide, ou métadonnées
//! - Empreinte de dossier récursive avec filtres et options
//! - Normalisation de chemins et helpers d’E/S atomiques
//! - Construction d’un `HashMap<String, Fingerprint>` prêt pour `compare_fingerprints`
//!
//! Aucune dépendance externe.

use crate::snapshot::Fingerprint;
use std::collections::HashMap;
use std::fs::{self, File, OpenOptions, ReadDir};
use std::io::{self, Read, Write, BufReader};
use std::path::{Component, Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

/// --------
/// Hash FNV-1a 64 bits (stable entre processus et plateformes)
/// --------

const FNV64_OFFSET: u64 = 0xcbf29ce484222325;
const FNV64_PRIME:  u64 = 0x00000100000001B3;

#[inline]
pub fn fnv1a64(bytes: &[u8]) -> u64 {
    let mut h = FNV64_OFFSET;
    for &b in bytes {
        h ^= b as u64;
        h = h.wrapping_mul(FNV64_PRIME);
    }
    h
}

#[inline]
fn fnv1a64_u64(x: u64) -> u64 {
    let mut buf = [0u8; 8];
    buf.copy_from_slice(&x.to_le_bytes());
    fnv1a64(&buf)
}

/// --------
/// Fingerprints de fichiers
/// --------

/// Empreinte basée uniquement sur les métadonnées (rapide, approximative).
/// Combine taille, mtime et ctime/ino si disponibles.
pub fn fingerprint_file_meta(path: &Path) -> io::Result<Fingerprint> {
    let meta = fs::metadata(path)?;
    let mut h = FNV64_OFFSET;

    // taille
    h ^= fnv1a64_u64(meta.len());
    h = h.wrapping_mul(FNV64_PRIME);

    // mtime
    if let Ok(m) = meta.modified() {
        h ^= fnv1a64_u64(system_time_to_secs(m));
        h = h.wrapping_mul(FNV64_PRIME);
    }

    // ctime si possible (non portable, fallback à created)
    if let Ok(c) = meta.created() {
        h ^= fnv1a64_u64(system_time_to_secs(c));
        h = h.wrapping_mul(FNV64_PRIME);
    }

    // identifiants de fichier si disponibles (best-effort)
    #[cfg(unix)]
    {
        use std::os::unix::fs::MetadataExt;
        h ^= fnv1a64_u64(meta.ino());
        h = h.wrapping_mul(FNV64_PRIME);
        h ^= fnv1a64_u64(meta.dev());
        h = h.wrapping_mul(FNV64_PRIME);
        h ^= fnv1a64_u64(meta.nlink());
        h = h.wrapping_mul(FNV64_PRIME);
    }

    Ok(h)
}

/// Empreinte « rapide » : métadonnées + premiers et derniers 64 KiB.
/// Idéal pour gros fichiers. Détecte la plupart des changements à coût réduit.
pub fn fingerprint_file_fast(path: &Path) -> io::Result<Fingerprint> {
    const CHUNK: u64 = 64 * 1024;
    let meta = fs::metadata(path)?;
    let size = meta.len();

    let mut h = fingerprint_file_meta(path)?;

    if size == 0 {
        return Ok(h);
    }

    let mut f = File::open(path)?;
    let mut buf = vec![0u8; CHUNK as usize];

    // début
    let read1 = f.read(&mut buf)?;
    h ^= fnv1a64(&buf[..read1]);
    h = h.wrapping_mul(FNV64_PRIME);

    // fin (si > CHUNK)
    if size > CHUNK {
        let seek_from = size.saturating_sub(CHUNK);
        // évite std::io::SeekFrom import explicite
        use std::io::Seek;
        f.seek(std::io::SeekFrom::Start(seek_from))?;
        let read2 = f.read(&mut buf)?;
        h ^= fnv1a64(&buf[..read2]);
        h = h.wrapping_mul(FNV64_PRIME);
    }

    Ok(h)
}

/// Empreinte « sûre » : contenu complet FNV-1a 64. Plus coûteux.
pub fn fingerprint_file_full(path: &Path) -> io::Result<Fingerprint> {
    let f = File::open(path)?;
    let mut r = BufReader::new(f);
    let mut h = FNV64_OFFSET;
    let mut buf = [0u8; 64 * 1024];

    loop {
        let n = r.read(&mut buf)?;
        if n == 0 { break; }
        h ^= fnv1a64(&buf[..n]);
        h = h.wrapping_mul(FNV64_PRIME);
    }

    // Mixe aussi la taille pour limiter collisions pathologiques
    if let Ok(meta) = r.get_ref().metadata() {
        h ^= fnv1a64_u64(meta.len());
        h = h.wrapping_mul(FNV64_PRIME);
    }

    Ok(h)
}

/// --------
/// Fingerprint de répertoire
/// --------

/// Options de parcours pour `fingerprint_dir_*`.
#[derive(Clone)]
pub struct DirFpOptions<'a> {
    /// Inclure les fichiers cachés.
    pub include_hidden: bool,
    /// Suivre les liens symboliques.
    pub follow_symlinks: bool,
    /// Fonction de filtrage. Retourne true pour inclure. Reçoit chemin relatif.
    pub filter_rel: Option<&'a dyn Fn(&str) -> bool>,
}

impl<'a> Default for DirFpOptions<'a> {
    fn default() -> Self {
        Self {
            include_hidden: false,
            follow_symlinks: false,
            filter_rel: None,
        }
    }
}

/// Parcours récursif d’un dossier en ordre déterministe.
/// Retourne la liste triée des fichiers réguliers relatifs à `root`.
pub fn list_files_recursive(root: &Path, opts: &DirFpOptions) -> io::Result<Vec<String>> {
    let mut out = Vec::new();
    visit_dir(root, root, opts, &mut out)?;
    out.sort(); // déterminisme fort
    Ok(out)
}

fn visit_dir(root: &Path, dir: &Path, opts: &DirFpOptions, out: &mut Vec<String>) -> io::Result<()> {
    let rd: ReadDir = fs::read_dir(dir)?;
    for ent in rd {
        let ent = ent?;
        let path = ent.path();
        let file_type = ent.file_type()?;

        let rel = normalize_rel_path(root, &path)?;
        let is_hidden = rel.split('/').any(|seg| seg.starts_with('.'));

        if !opts.include_hidden && is_hidden {
            continue;
        }
        if let Some(flt) = opts.filter_rel {
            if !flt(&rel) { continue; }
        }

        if file_type.is_dir() {
            visit_dir(root, &path, opts, out)?;
        } else if file_type.is_file() {
            out.push(rel);
        } else if file_type.is_symlink() && opts.follow_symlinks {
            let target = fs::read_link(&path)?;
            let resolved = if target.is_absolute() { target } else { path.parent().unwrap_or(dir).join(target) };
            let meta = fs::metadata(&resolved).ok();
            if let Some(m) = meta {
                if m.is_dir() {
                    visit_dir(root, &resolved, opts, out)?;
                } else if m.is_file() {
                    let rel_resolved = normalize_rel_path(root, &resolved)?;
                    out.push(rel_resolved);
                }
            }
        }
    }
    Ok(())
}

/// Construit un `HashMap<rel, fp>` pour un dossier à l’aide d’une stratégie de fingerprint.
pub enum FileFpStrategy {
    Meta,  // `fingerprint_file_meta`
    Fast,  // `fingerprint_file_fast`
    Full,  // `fingerprint_file_full`
}

pub fn build_fingerprint_map(
    root: &Path,
    opts: &DirFpOptions,
    strat: FileFpStrategy,
) -> io::Result<HashMap<String, Fingerprint>> {
    let files = list_files_recursive(root, opts)?;
    let mut map = HashMap::with_capacity(files.len());
    for rel in files {
        let abs = root.join(&rel);
        let fp = match strat {
            FileFpStrategy::Meta => fingerprint_file_meta(&abs)?,
            FileFpStrategy::Fast => fingerprint_file_fast(&abs)?,
            FileFpStrategy::Full => fingerprint_file_full(&abs)?,
        };
        map.insert(rel, fp);
    }
    Ok(map)
}

/// Empreinte globale d’un dossier : mix ordonné (stable) des empreintes de fichiers et de leurs chemins.
pub fn fingerprint_dir(
    root: &Path,
    opts: &DirFpOptions,
    strat: FileFpStrategy,
) -> io::Result<Fingerprint> {
    let files = list_files_recursive(root, opts)?;
    let mut h = FNV64_OFFSET;
    for rel in files {
        let abs = root.join(&rel);
        let fp = match strat {
            FileFpStrategy::Meta => fingerprint_file_meta(&abs)?,
            FileFpStrategy::Fast => fingerprint_file_fast(&abs)?,
            FileFpStrategy::Full => fingerprint_file_full(&abs)?,
        };
        h ^= fnv1a64(rel.as_bytes());
        h = h.wrapping_mul(FNV64_PRIME);
        h ^= fnv1a64_u64(fp);
        h = h.wrapping_mul(FNV64_PRIME);
    }
    Ok(h)
}

/// --------
/// Helpers chemins
/// --------

/// Normalise `path` en chemin relatif propre à partir de `base`, séparateur `/`.
/// - Supprime les `.` et traite les `..` avec prudence.
/// - Évite les préfixes de lecteur sous Windows dans la sortie.
/// - Garantit une sortie déterministe.
pub fn normalize_rel_path(base: &Path, path: &Path) -> io::Result<String> {
    let abs = if path.is_absolute() { path.to_path_buf() } else { base.join(path) };
    let rel = pathdiff::diff_paths(&abs, base).unwrap_or_else(|| PathBuf::from(""));

    let mut parts = Vec::new();
    for c in rel.components() {
        match c {
            Component::RootDir | Component::Prefix(_) => { /* ignorer */ }
            Component::CurDir => { /* ignorer */ }
            Component::ParentDir => { parts.pop(); }
            Component::Normal(s) => parts.push(s.to_string_lossy().into_owned()),
        }
    }
    Ok(parts.join("/"))
}

/// Diff de maps d’empreintes. Retourne (ajoutés, modifiés, supprimés).
pub fn diff_fingerprints(
    prev: &HashMap<String, Fingerprint>,
    now: &HashMap<String, Fingerprint>,
) -> (Vec<String>, Vec<String>, Vec<String>) {
    let mut added = Vec::new();
    let mut changed = Vec::new();
    let mut removed = Vec::new();

    for (k, &v) in now {
        match prev.get(k) {
            None => added.push(k.clone()),
            Some(&old) if old != v => changed.push(k.clone()),
            _ => {}
        }
    }
    for k in prev.keys() {
        if !now.contains_key(k) {
            removed.push(k.clone());
        }
    }
    added.sort();
    changed.sort();
    removed.sort();
    (added, changed, removed)
}

/// --------
/// E/S utilitaires
/// --------

/// Écrit atomiquement `bytes` vers `path` en utilisant un fichier temporaire et `rename`.
pub fn atomic_write(path: &Path, bytes: &[u8]) -> io::Result<()> {
    let parent = path.parent().ok_or_else(|| io::Error::new(io::ErrorKind::Other, "no parent dir"))?;
    fs::create_dir_all(parent).ok(); // best effort
    let mut tmp = parent.to_path_buf();
    tmp.push(format!(
        ".{}.tmp-{}",
        path.file_name().and_then(|s| s.to_str()).unwrap_or("vitte-inc"),
        now_nanos()
    ));

    {
        let mut f = OpenOptions::new().write(true).create(true).truncate(true).open(&tmp)?;
        f.write_all(bytes)?;
        f.sync_all()?;
    }

    // Sur Windows, replace si existe.
    #[cfg(windows)]
    {
        if path.exists() {
            fs::remove_file(path).ok();
        }
    }

    fs::rename(&tmp, path)?;
    Ok(())
}

/// Lit intégralement un fichier en mémoire.
pub fn read_all(path: &Path) -> io::Result<Vec<u8>> {
    fs::read(path)
}

/// Timestamp en secondes depuis epoch.
#[inline]
pub fn now_secs() -> u64 {
    system_time_to_secs(SystemTime::now())
}

/// Timestamp en nanosecondes (best-effort).
#[inline]
pub fn now_nanos() -> u128 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|d| d.as_nanos())
        .unwrap_or(0)
}

#[inline]
fn system_time_to_secs(t: SystemTime) -> u64 {
    t.duration_since(UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0)
}

/// --------
/// Dépendance locale minimale pour diff de chemins
/// --------
/// Implémentation simplifiée de pathdiff::diff_paths pour éviter une dépendance externe
/// quand non disponible. Utilise std::fs::canonicalize best-effort, sinon fallback.
/// Cette fonction n’est utilisée qu’en interne via `normalize_rel_path`.
mod pathdiff {
    use std::path::{Path, PathBuf};

    pub fn diff_paths(path: &Path, base: &Path) -> Option<PathBuf> {
        // Best-effort: si canonicalize échoue, on travaille sur les paths tels quels.
        let p = std::fs::canonicalize(path).unwrap_or_else(|_| path.to_path_buf());
        let b = std::fs::canonicalize(base).unwrap_or_else(|_| base.to_path_buf());

        let p_iter = p.components().collect::<Vec<_>>();
        let b_iter = b.components().collect::<Vec<_>>();

        let mut i = 0usize;
        while i < p_iter.len() && i < b_iter.len() && p_iter[i] == b_iter[i] {
            i += 1;
        }

        let mut result = PathBuf::new();
        for _ in i..b_iter.len() {
            result.push("..");
        }
        for comp in &p_iter[i..] {
            result.push(comp.as_os_str());
        }
        Some(result)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;
    use tempfile::tempdir;

    #[test]
    fn fnv_basic() {
        assert_ne!(fnv1a64(b"a"), fnv1a64(b"b"));
        assert_eq!(fnv1a64(b""), FNV64_OFFSET); // convention
    }

    #[test]
    fn file_fingerprints_variants() {
        let td = tempdir().unwrap();
        let p = td.path().join("x.txt");
        fs::write(&p, b"hello world").unwrap();

        let m = fingerprint_file_meta(&p).unwrap();
        let f = fingerprint_file_fast(&p).unwrap();
        let full = fingerprint_file_full(&p).unwrap();

        // full doit changer si contenu change
        fs::write(&p, b"hello world!").unwrap();
        let full2 = fingerprint_file_full(&p).unwrap();
        assert_ne!(full, full2);

        // fast change dans la plupart des cas
        let f2 = fingerprint_file_fast(&p).unwrap();
        assert_ne!(f, f2);

        // meta peut ne pas changer si mtime identique, on ne teste pas la stabilité ici.
        let _ = m;
    }

    #[test]
    fn dir_listing_and_map() {
        let td = tempdir().unwrap();
        let root = td.path();
        fs::create_dir_all(root.join("a/b")).unwrap();
        fs::write(root.join("a/one.txt"), b"1").unwrap();
        fs::write(root.join("a/b/two.txt"), b"2").unwrap();

        let opts = DirFpOptions::default();
        let files = list_files_recursive(root, &opts).unwrap();
        assert_eq!(files, vec!["a/b/two.txt".to_string(), "a/one.txt".to_string()].into_iter().sorted());

        let map = build_fingerprint_map(root, &opts, FileFpStrategy::Full).unwrap();
        assert_eq!(map.len(), 2);
        assert!(map.contains_key("a/one.txt"));
        assert!(map.contains_key("a/b/two.txt"));
    }

    #[test]
    fn diff_fp_works() {
        let mut a = HashMap::new();
        a.insert("x".into(), 1);
        a.insert("y".into(), 2);

        let mut b = HashMap::new();
        b.insert("y".into(), 3);
        b.insert("z".into(), 4);

        let (added, changed, removed) = diff_fingerprints(&a, &b);
        assert_eq!(added, vec!["z"]);
        assert_eq!(changed, vec!["y"]);
        assert_eq!(removed, vec!["x"]);
    }

    #[test]
    fn atomic_write_ok() {
        let td = tempdir().unwrap();
        let p = td.path().join("snap.bin");
        atomic_write(&p, b"abc").unwrap();
        assert_eq!(fs::read(&p).unwrap(), b"abc");
        atomic_write(&p, b"def").unwrap();
        assert_eq!(fs::read(&p).unwrap(), b"def");
    }

    // Petit helper pour comparer vecteurs triés dans test ci-dessus
    trait SortedExt<T> {
        fn sorted(self) -> Vec<T>;
    }
    impl<T: Ord> SortedExt<T> for std::vec::IntoIter<T> {
        fn sorted(mut self) -> Vec<T> {
            let mut v: Vec<T> = self.collect();
            v.sort();
            v
        }
    }
}
