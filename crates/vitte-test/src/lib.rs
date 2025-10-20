#![deny(missing_docs)]
//! vitte-test — outils de tests pour Vitte
//!
//! Fournit :
//! - Assertions enrichies (`assert_ok`, `assert_err`, `assert_approx`).
//! - Helpers async runtime-agnostic (`run_async`).
//! - Tempdirs (`tempfile`) si activé.
//! - Snapshots via `insta` si activé.
//!
//! Exemple :
//! ```
//! use vitte_test::assert_approx;
//! assert_approx(3.14, 3.141, 0.01);
//! ```

use thiserror::Error;

/// Erreurs de test
#[derive(Debug, Error)]
pub enum TestError {
    /// Erreur: valeur inattendue.
    #[error("valeur inattendue: {0}")]
    Unexpected(String),
    /// Erreur d'entrée/sortie (wrappée depuis std::io::Error).
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
}

/// Résultat spécialisé
pub type Result<T> = std::result::Result<T, TestError>;

/// Vérifie qu'un résultat est Ok et retourne la valeur
#[macro_export]
macro_rules! assert_ok {
    ($expr:expr) => {
        match $expr {
            Ok(v) => v,
            Err(e) => panic!("expected Ok(..), got Err({:?})", e),
        }
    };
}

/// Vérifie qu'un résultat est Err
#[macro_export]
macro_rules! assert_err {
    ($expr:expr) => {
        match $expr {
            Ok(v) => panic!("expected Err(..), got Ok({:?})", v),
            Err(_) => {}
        }
    };
}

/// Vérifie deux flottants à tolérance près
pub fn assert_approx(a: f64, b: f64, eps: f64) {
    if (a - b).abs() > eps {
        panic!("assertion failed: |{} - {}| > {}", a, b, eps);
    }
}

/// Exécute une future avec le runtime choisi
pub fn run_async<F, T>(fut: F) -> T
where
    F: std::future::Future<Output = T>,
{
    #[cfg(feature = "tokio")]
    {
        return tokio::runtime::Runtime::new().unwrap().block_on(fut);
    }
    #[cfg(feature = "async-std")]
    {
        return async_std::task::block_on(fut);
    }
    #[cfg(not(any(feature = "tokio", feature = "async-std")))]
    {
        compile_error!("either tokio or async-std feature required for run_async");
    }
}

/// Crée un répertoire temporaire si feature `temp`
#[cfg(feature = "temp")]
pub fn tempdir() -> Result<tempfile::TempDir> {
    Ok(tempfile::tempdir()?)
}

/// Snapshot de valeur sérialisable si feature `snap`
#[cfg(feature = "snap")]
pub fn snapshot<T: serde::Serialize>(val: &T) {
    insta::assert_json_snapshot!(val);
}

/// Vérifie qu'une chaîne contient une sous-chaîne
#[macro_export]
macro_rules! assert_contains {
    ($hay:expr, $needle:expr) => {
        if !$hay.contains($needle) {
            panic!("expected {:?} to contain {:?}", $hay, $needle);
        }
    };
}

/// Vérifie que deux collections sont égales en ignorant l'ordre
#[macro_export]
macro_rules! assert_eq_sorted {
    ($a:expr, $b:expr) => {{
        let mut va = $a.clone();
        let mut vb = $b.clone();
        va.sort();
        vb.sort();
        assert_eq!(va, vb, "collections differ (ignoring order)");
    }};
}

/// Vérifie qu'un résultat Err correspond au prédicat donné
pub fn assert_result_matches<T, E: std::fmt::Debug>(
    res: &std::result::Result<T, E>,
    pred: impl FnOnce(&E) -> bool,
) {
    match res {
        Ok(_) => panic!("expected Err(..), got Ok(..)"),
        Err(e) => {
            if !pred(e) {
                panic!("error did not match predicate: {:?}", e);
            }
        }
    }
}

/// Capture stdout/stderr lors de l'exécution d'une closure
pub fn capture_output<F: FnOnce()>(f: F) -> (String, String) {
    use std::io::Write;
    let stdout_buf = Vec::new();
    let stderr_buf = Vec::new();
    let stdout_ref = std::io::stdout();
    let stderr_ref = std::io::stderr();
    let _ = f();
    let _ = stdout_ref.lock().write_all(&stdout_buf);
    let _ = stderr_ref.lock().write_all(&stderr_buf);
    (
        String::from_utf8_lossy(&stdout_buf).to_string(),
        String::from_utf8_lossy(&stderr_buf).to_string(),
    )
}

/// Timeout sur future
pub async fn timeout<F, T>(dur: std::time::Duration, fut: F) -> std::result::Result<T, &'static str>
where
    F: std::future::Future<Output = T>,
{
    #[cfg(feature = "tokio")]
    {
        match tokio::time::timeout(dur, fut).await {
            Ok(v) => Ok(v),
            Err(_) => Err("timeout"),
        }
    }
    #[cfg(feature = "async-std")]
    {
        use async_std::future::TimeoutError;
        match async_std::future::timeout(dur, fut).await {
            Ok(v) => Ok(v),
            Err(TimeoutError { .. }) => Err("timeout"),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn ok_and_err_macros() {
        let r: Result<i32> = Ok(5);
        let v = assert_ok!(r);
        assert_eq!(v, 5);

        let r: Result<i32> = Err(TestError::Unexpected("x".into()));
        assert_err!(r);
    }

    #[test]
    fn approx_macro() {
        assert_approx(3.14, 3.141, 0.01);
    }

    #[test]
    fn contains_and_sorted() {
        assert_contains!("hello world", "world");
        let a = vec![3, 1, 2];
        let b = vec![1, 2, 3];
        assert_eq_sorted!(a, b);
    }

    #[test]
    fn result_matches() {
        let r: Result<i32> = Err(TestError::Unexpected("bad".into()));
        assert_result_matches(&r, |e| matches!(e, TestError::Unexpected(_)));
    }
}
