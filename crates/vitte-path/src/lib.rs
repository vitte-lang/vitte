#![deny(warnings)]
#![warn(missing_docs)]
#![doc = r#"
`vitte-path` crate provides utilities for file path manipulation, normalization,
search, resolution, and basic file/directory operations.
"#]

use std::fs;
use std::io;
use std::path::{Path, PathBuf};

use walkdir::WalkDir;

/// Custom error type for path operations.
#[derive(Debug, thiserror::Error)]
pub enum PathError {
    /// IO error wrapper.
    #[error("IO error: {0}")]
    Io(#[from] io::Error),

    /// WalkDir error wrapper.
    #[error("WalkDir error: {0}")]
    Walkdir(#[from] walkdir::Error),

    /// Invalid path error.
    #[error("Invalid path: {0}")]
    InvalidPath(String),
}

/// Result type alias for path operations.
pub type Result<T> = std::result::Result<T, PathError>;

/// Normalize a path by removing redundant components like `.` and `..`.
///
/// Returns a normalized `PathBuf`.
///
/// # Examples
///
/// ```
/// use vitte_path::normalize_path;
/// let path = normalize_path("foo/./bar/../baz");
/// assert_eq!(path, std::path::PathBuf::from("foo/baz"));
/// ```
pub fn normalize_path(path: &str) -> PathBuf {
    let p = Path::new(path);
    let mut components = vec![];

    for comp in p.components() {
        match comp {
            std::path::Component::ParentDir => {
                if components.last().map_or(false, |c| *c != std::path::Component::RootDir) {
                    components.pop();
                } else {
                    components.push(comp);
                }
            }
            std::path::Component::CurDir => {
                // skip
            }
            _ => components.push(comp),
        }
    }

    let mut normalized = PathBuf::new();
    for comp in components {
        normalized.push(comp.as_os_str());
    }
    normalized
}

/// Check if a path exists.
///
/// # Examples
///
/// ```
/// use vitte_path::exists;
/// assert!(exists(std::path::Path::new(".")));
/// ```
pub fn exists(path: &Path) -> bool {
    path.exists()
}

/// Check if a path is a directory.
///
/// # Examples
///
/// ```
/// use vitte_path::is_dir;
/// assert!(is_dir(std::path::Path::new(".")));
/// ```
pub fn is_dir(path: &Path) -> bool {
    path.is_dir()
}

/// Create a directory and all its parent components if they are missing.
///
/// # Errors
///
/// Returns an error if creation fails.
///
/// # Examples
///
/// ```
/// use vitte_path::{create_dir_all, exists, is_dir};
/// use std::path::Path;
///
/// let tmp_dir = std::env::temp_dir().join("vitte_path_test_dir");
/// let _ = std::fs::remove_dir_all(&tmp_dir);
/// create_dir_all(&tmp_dir).unwrap();
/// assert!(exists(&tmp_dir));
/// assert!(is_dir(&tmp_dir));
/// std::fs::remove_dir_all(&tmp_dir).unwrap();
/// ```
pub fn create_dir_all(path: &Path) -> Result<()> {
    fs::create_dir_all(path)?;
    Ok(())
}

/// Copy a file from `src` to `dst`.
///
/// # Errors
///
/// Returns an error if the copy fails.
///
/// # Examples
///
/// ```
/// use vitte_path::{copy_file, exists};
/// use std::path::PathBuf;
/// use std::fs;
///
/// let tmp_dir = std::env::temp_dir();
/// let src = tmp_dir.join("vitte_path_src.txt");
/// let dst = tmp_dir.join("vitte_path_dst.txt");
///
/// fs::write(&src, b"hello").unwrap();
/// let _ = fs::remove_file(&dst);
///
/// copy_file(&src, &dst).unwrap();
/// assert!(exists(&dst));
///
/// fs::remove_file(&src).unwrap();
/// fs::remove_file(&dst).unwrap();
/// ```
pub fn copy_file(src: &Path, dst: &Path) -> Result<()> {
    fs::copy(src, dst)?;
    Ok(())
}

/// Remove a file or directory at the given path.
///
/// If the path is a directory, it will be removed recursively.
///
/// # Errors
///
/// Returns an error if removal fails.
///
/// # Examples
///
/// ```
/// use vitte_path::{create_dir_all, remove, exists};
/// use std::path::PathBuf;
///
/// let tmp_dir = std::env::temp_dir().join("vitte_path_remove_test");
/// let _ = std::fs::remove_dir_all(&tmp_dir);
/// create_dir_all(&tmp_dir).unwrap();
/// assert!(exists(&tmp_dir));
/// remove(&tmp_dir).unwrap();
/// assert!(!exists(&tmp_dir));
/// ```
pub fn remove(path: &Path) -> Result<()> {
    if path.is_dir() {
        fs::remove_dir_all(path)?;
    } else if path.exists() {
        fs::remove_file(path)?;
    }
    Ok(())
}

/// Recursively walk a directory and collect all paths.
///
/// Returns a vector of all file and directory paths under the given path.
///
/// # Errors
///
/// Returns an error if the directory cannot be read.
///
/// # Examples
///
/// ```
/// use vitte_path::{walk_dir, create_dir_all};
/// use std::path::PathBuf;
///
/// let tmp_dir = std::env::temp_dir().join("vitte_path_walk_test");
/// let _ = std::fs::remove_dir_all(&tmp_dir);
/// create_dir_all(&tmp_dir).unwrap();
///
/// let paths = walk_dir(&tmp_dir).unwrap();
/// assert!(paths.is_empty());
///
/// std::fs::remove_dir_all(&tmp_dir).unwrap();
/// ```
pub fn walk_dir(path: &Path) -> Result<Vec<PathBuf>> {
    if !path.exists() {
        return Err(PathError::InvalidPath(format!(
            "Path does not exist: {}",
            path.display()
        )));
    }
    let mut paths = Vec::new();
    for entry in WalkDir::new(path) {
        let entry = entry?;
        paths.push(entry.path().to_path_buf());
    }
    Ok(paths)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_normalize_path() {
        let input = "foo/./bar/../baz";
        let expected = PathBuf::from("foo/baz");
        assert_eq!(normalize_path(input), expected);

        let input2 = "/foo//bar/.././baz/";
        let expected2 = PathBuf::from("/foo/baz");
        assert_eq!(normalize_path(input2), expected2);

        let input3 = "./././";
        let expected3 = PathBuf::new();
        assert_eq!(normalize_path(input3), expected3);
    }

    #[test]
    fn test_exists_and_is_dir() {
        let current_dir = std::env::current_dir().unwrap();
        assert!(exists(&current_dir));
        assert!(is_dir(&current_dir));

        let file_path = std::env::temp_dir().join("vitte_path_test_file");
        std::fs::write(&file_path, b"test").unwrap();
        assert!(exists(&file_path));
        assert!(!is_dir(&file_path));
        std::fs::remove_file(&file_path).unwrap();
    }

    #[test]
    fn test_create_dir_all_and_remove() {
        let tmp_dir = std::env::temp_dir().join("vitte_path_test_dir");
        let _ = std::fs::remove_dir_all(&tmp_dir);
        create_dir_all(&tmp_dir).unwrap();
        assert!(exists(&tmp_dir));
        assert!(is_dir(&tmp_dir));

        remove(&tmp_dir).unwrap();
        assert!(!exists(&tmp_dir));
    }

    #[test]
    fn test_copy_file() {
        let tmp_dir = std::env::temp_dir();
        let src = tmp_dir.join("vitte_path_src.txt");
        let dst = tmp_dir.join("vitte_path_dst.txt");

        std::fs::write(&src, b"hello").unwrap();
        let _ = std::fs::remove_file(&dst);

        copy_file(&src, &dst).unwrap();
        assert!(exists(&dst));

        std::fs::remove_file(&src).unwrap();
        std::fs::remove_file(&dst).unwrap();
    }

    #[test]
    fn test_walk_dir() {
        let tmp_dir = std::env::temp_dir().join("vitte_path_walk_test");
        let _ = std::fs::remove_dir_all(&tmp_dir);
        create_dir_all(&tmp_dir).unwrap();

        let paths = walk_dir(&tmp_dir).unwrap();
        // Should at least contain the root directory itself
        assert!(paths.contains(&tmp_dir));

        std::fs::remove_dir_all(&tmp_dir).unwrap();
    }

    #[test]
    fn test_remove_file() {
        let tmp_file = std::env::temp_dir().join("vitte_path_remove_file_test.txt");
        std::fs::write(&tmp_file, b"test").unwrap();
        assert!(exists(&tmp_file));
        remove(&tmp_file).unwrap();
        assert!(!exists(&tmp_file));
    }
}
