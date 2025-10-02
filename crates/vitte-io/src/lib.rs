//! vitte-io — I/O basique et pratique pour Vitte
//!
//! - `stdin`/`stdout`/`stderr` helpers
//! - Fichiers: lire/écrire/ajouter/copier/lister
//! - Utilitaires de chemins et répertoires
//! - API textuelle et binaire
//! - Feature `errors` pour un `Error` ergonomique (thiserror)
//! - Feature `serde` si tu veux sérialiser des petites structures (ex: listings)
//!
//! Par défaut: `std` activé. Pour embarqué: `--no-default-features --features alloc-only`.

#![forbid(unsafe_code)]

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.");

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{string::String, vec::Vec};

#[cfg(feature = "std")]
use std::{
    fs, io,
    io::{Read, Write},
    path::{Path, PathBuf},
};

#[cfg(feature = "alloc-only")]
mod nostd_shims {
    // Shims minimales pour alloc-only (à étendre selon besoins réels)
    use alloc::{string::String, vec::Vec};

    #[derive(Debug)]
    pub struct IoError;

    pub type IoResult<T> = core::result::Result<T, IoError>;

    pub fn not_supported<T>() -> IoResult<T> {
        Err(IoError)
    }

    pub fn read_stdin_to_string(_: &mut String) -> IoResult<()> {
        not_supported()
    }

    pub fn write_stdout(_: &[u8]) -> IoResult<()> {
        not_supported()
    }

    pub fn write_stderr(_: &[u8]) -> IoResult<()> {
        not_supported()
    }

    pub fn read_file_to_bytes(_path: &str) -> IoResult<Vec<u8>> {
        not_supported()
    }

    pub fn write_file_bytes(_path: &str, _bytes: &[u8]) -> IoResult<()> {
        not_supported()
    }
}

#[cfg(feature = "errors")]
pub use error::Error;

#[cfg(feature = "errors")]
mod error {
    use core::fmt;

    #[cfg(feature = "std")]
    use thiserror::Error as ThisError;

    #[cfg_attr(feature = "std", derive(ThisError))]
    #[derive(Debug)]
    pub enum Error {
        #[cfg(feature = "std")]
        #[error("I/O error: {0}")]
        Io(#[from] std::io::Error),

        #[cfg(not(feature = "std"))]
        IoNoStd,

        #[error("Encoding error: {0}")]
        Encoding(&'static str),

        #[error("Invalid argument: {0}")]
        InvalidArg(&'static str),

        #[error("Other: {0}")]
        Other(&'static str),
    }

    impl From<&'static str> for Error {
        fn from(s: &'static str) -> Self {
            Error::Other(s)
        }
    }

    impl fmt::Display for Error {
        fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
            #[cfg(not(feature = "std"))]
            {
                use core::fmt::Debug;
                write!(f, "{:?}", self)
            }
            #[cfg(feature = "std")]
            {
                fmt::Debug::fmt(self, f)
            }
        }
    }
}

/// Alias résultat conditionné par la feature `errors`.
#[cfg(feature = "errors")]
pub type Result<T> = core::result::Result<T, Error>;

#[cfg(not(feature = "errors"))]
pub type Result<T> = core::result::Result<T, &'static str>;

/* =======================================================================
   SECTION: STDIN/STDOUT/STDERR
   ======================================================================= */

/// Lit tout `stdin` en `String`.
#[cfg(feature = "std")]
pub fn read_stdin_to_string() -> Result<String> {
    let mut s = String::new();
    io::stdin().read_to_string(&mut s).map_err(map_io())?;
    Ok(s)
}

/// Lit une ligne depuis `stdin` (sans `\n` final).
#[cfg(feature = "std")]
pub fn read_line() -> Result<String> {
    let mut s = String::new();
    io::stdin().read_line(&mut s).map_err(map_io())?;
    trim_newline(&mut s);
    Ok(s)
}

/// Écrit des octets sur `stdout` et flush.
#[cfg(feature = "std")]
pub fn write_stdout(bytes: &[u8]) -> Result<()> {
    let mut out = io::stdout().lock();
    out.write_all(bytes).map_err(map_io())?;
    out.flush().map_err(map_io())
}

/// Écrit des octets sur `stderr` et flush.
#[cfg(feature = "std")]
pub fn write_stderr(bytes: &[u8]) -> Result<()> {
    let mut err = io::stderr().lock();
    err.write_all(bytes).map_err(map_io())?;
    err.flush().map_err(map_io())
}

/// `print` sans retour ligne.
#[cfg(feature = "std")]
pub fn print(s: &str) -> Result<()> {
    write_stdout(s.as_bytes())
}

/// `println` avec retour ligne.
#[cfg(feature = "std")]
pub fn println(s: &str) -> Result<()> {
    let mut buf = Vec::with_capacity(s.len() + 1);
    buf.extend_from_slice(s.as_bytes());
    buf.push(b'\n');
    write_stdout(&buf)
}

/// `eprint` sans retour ligne.
#[cfg(feature = "std")]
pub fn eprint(s: &str) -> Result<()> {
    write_stderr(s.as_bytes())
}

/// `eprintln` avec retour ligne.
#[cfg(feature = "std")]
pub fn eprintln(s: &str) -> Result<()> {
    let mut buf = Vec::with_capacity(s.len() + 1);
    buf.extend_from_slice(s.as_bytes());
    buf.push(b'\n');
    write_stderr(&buf)
}

/* =======================================================================
   SECTION: FICHIERS ET FLUX
   ======================================================================= */

/// Lit un fichier texte entier en `String` (UTF-8 attendu).
#[cfg(feature = "std")]
pub fn read_file_to_string<P: AsRef<Path>>(path: P) -> Result<String> {
    fs::read_to_string(path).map_err(map_io())
}

/// Lit tout le fichier en octets.
#[cfg(feature = "std")]
pub fn read_file<P: AsRef<Path>>(path: P) -> Result<Vec<u8>> {
    fs::read(path).map_err(map_io())
}

/// Écrit des octets (crée/remplace).
#[cfg(feature = "std")]
pub fn write_file<P: AsRef<Path>>(path: P, bytes: &[u8]) -> Result<()> {
    fs::write(path, bytes).map_err(map_io())
}

/// Écrit une chaîne UTF-8 (crée/remplace).
#[cfg(feature = "std")]
pub fn write_file_string<P: AsRef<Path>>(path: P, text: &str) -> Result<()> {
    write_file(path, text.as_bytes())
}

/// Ajoute des octets à la fin du fichier (crée si absent).
#[cfg(feature = "std")]
pub fn append_file<P: AsRef<Path>>(path: P, bytes: &[u8]) -> Result<()> {
    use std::fs::OpenOptions;
    let mut f = OpenOptions::new()
        .create(true)
        .append(true)
        .open(path)
        .map_err(map_io())?;
    f.write_all(bytes).map_err(map_io())
}

/// Copie de fichier. Retourne le nombre d’octets copiés.
#[cfg(feature = "std")]
pub fn copy_file<P: AsRef<Path>, Q: AsRef<Path>>(from: P, to: Q) -> Result<u64> {
    fs::copy(from, to).map_err(map_io())
}

/// Supprime un fichier s’il existe.
#[cfg(feature = "std")]
pub fn remove_file<P: AsRef<Path>>(path: P) -> Result<()> {
    match fs::remove_file(path) {
        Ok(_) => Ok(()),
        Err(e) if e.kind() == io::ErrorKind::NotFound => Ok(()),
        Err(e) => Err(map_io()(e)),
    }
}

/* =======================================================================
   SECTION: RÉPERTOIRES ET CHEMINS
   ======================================================================= */

/// Crée récursivement les dossiers.
#[cfg(feature = "std")]
pub fn create_dir_all<P: AsRef<Path>>(path: P) -> Result<()> {
    fs::create_dir_all(path).map_err(map_io())
}

/// Supprime récursivement un dossier.
#[cfg(feature = "std")]
pub fn remove_dir_all<P: AsRef<Path>>(path: P) -> Result<()> {
    fs::remove_dir_all(path).map_err(map_io())
}

/// Teste l’existence d’un chemin.
#[cfg(feature = "std")]
pub fn exists<P: AsRef<Path>>(path: P) -> bool {
    path.as_ref().exists()
}

/// Teste si c’est un fichier.
#[cfg(feature = "std")]
pub fn is_file<P: AsRef<Path>>(path: P) -> bool {
    path.as_ref().is_file()
}

/// Teste si c’est un dossier.
#[cfg(feature = "std")]
pub fn is_dir<P: AsRef<Path>>(path: P) -> bool {
    path.as_ref().is_dir()
}

/// Liste le contenu d’un dossier (non récursif).
#[cfg(feature = "std")]
pub fn list_dir<P: AsRef<Path>>(path: P) -> Result<Vec<PathBuf>> {
    let mut out = Vec::new();
    for entry in fs::read_dir(path).map_err(map_io())? {
        let entry = entry.map_err(map_io())?;
        out.push(entry.path());
    }
    Ok(out)
}

/// Normalise un chemin simple: enlève `.` et résout `..` localement (sans I/O).
#[cfg(feature = "std")]
pub fn normalize_path<P: AsRef<Path>>(path: P) -> PathBuf {
    use std::path::Component;
    let mut stack: Vec<Component> = Vec::new();
    for comp in path.as_ref().components() {
        match comp {
            Component::CurDir => {}
            Component::ParentDir => {
                if let Some(last) = stack.last() {
                    if matches!(last, Component::Normal(_)) {
                        stack.pop();
                        continue;
                    }
                }
                stack.push(comp);
            }
            other => stack.push(other),
        }
    }
    let mut out = PathBuf::new();
    for c in stack {
        out.push(c.as_os_str());
    }
    out
}

/* =======================================================================
   SECTION: UTILITAIRES TEXTE/BINAIRE
   ======================================================================= */

/// Lit un fichier ligne par ligne en itérateur (bufferisé).
#[cfg(feature = "std")]
pub fn read_lines<P: AsRef<Path>>(path: P) -> Result<impl Iterator<Item = io::Result<String>>> {
    use std::{fs::File, io::BufRead};
    let file = File::open(path).map_err(map_io())?;
    let reader = io::BufReader::new(file);
    Ok(reader.lines())
}

/// Lit tout un `Read` dans un `Vec<u8>`.
#[cfg(feature = "std")]
pub fn read_to_end<R: Read>(mut r: R) -> Result<Vec<u8>> {
    let mut buf = Vec::new();
    r.read_to_end(&mut buf).map_err(map_io())?;
    Ok(buf)
}

/// Copie de `Read` vers `Write` avec buffer interne. Retourne octets copiés.
#[cfg(feature = "std")]
pub fn copy_stream<R: Read, W: Write>(mut r: R, mut w: W) -> Result<u64> {
    io::copy(&mut r, &mut w).map_err(map_io())
}

/* =======================================================================
   SECTION: MAPPINGS D’ERREURS
   ======================================================================= */

#[cfg(feature = "std")]
#[inline]
fn map_io() -> impl FnOnce(io::Error) -> crate::Result::Err {
    #[cfg(feature = "errors")]
    {
        |e| Error::from(e)
    }
    #[cfg(not(feature = "errors"))]
    {
        |_e| "io error"
    }
}

/// Supprime le `\n` final d’une ligne lue.
#[cfg(feature = "std")]
#[inline]
fn trim_newline(s: &mut String) {
    if s.ends_with('\n') {
        s.pop();
        if s.ends_with('\r') {
            s.pop();
        }
    }
}

/* =======================================================================
   SECTION: TESTS
   ======================================================================= */

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;
    use std::io::Cursor;

    #[test]
    fn normalize_rel() {
        let p = normalize_path("./a/./b/../c");
        assert!(p.ends_with("a/c"));
    }

    #[test]
    fn read_write_roundtrip() {
        let dir = tempfile::tempdir().unwrap();
        let file = dir.path().join("x.txt");
        write_file_string(&file, "hello\nworld").unwrap();
        assert!(exists(&file) && is_file(&file));
        let s = read_file_to_string(&file).unwrap();
        assert_eq!(s, "hello\nworld");
        append_file(&file, b"!").unwrap();
        let s2 = read_file_to_string(&file).unwrap();
        assert_eq!(s2, "hello\nworld!");
    }

    #[test]
    fn copy_stream_ok() {
        let src = Cursor::new(b"abcdef".to_vec());
        let mut dst = Vec::new();
        let n = copy_stream(src, &mut dst).unwrap();
        assert_eq!(n, 6);
        assert_eq!(dst, b"abcdef");
    }

    #[test]
    fn read_lines_ok() {
        let dir = tempfile::tempdir().unwrap();
        let file = dir.path().join("l.txt");
        write_file_string(&file, "a\nb\nc").unwrap();
        let lines = read_lines(&file).unwrap().collect::<Result<Vec<_>, _>>().unwrap();
        assert_eq!(lines, vec!["a", "b", "c"]);
    }
}