//! vitte-golden — Golden tests utilities for Vitte
//!
//! Features:
//! - Text and binary golden files
//! - Clean diffs using `similar`
//! - Auto-update with `VITTE_GOLDEN_UPDATE=1`
//! - Helpers to resolve paths near tests and write atomically
//! - Optional JSON pretty+stable compare (feature `serde_json`)
//!
//! Typical usage:
//! ```no_run
//! use vitte_golden::{compare_text, GoldenOptions};
//!
//! #[test]
//! fn cli_help() {
//!     let actual = "--help output...\n";
//!     compare_text("tests/golden/cli_help.txt", actual, &GoldenOptions::default()).unwrap();
//! }
//! ```
//!
//! Or with the assertion macro:
//! ```no_run
//! use vitte_golden::golden_assert_text_eq;
//! #[test] fn t() { golden_assert_text_eq!("tests/golden/x.txt", "value\n"); }
//! ```

#![forbid(unsafe_code)]
#![deny(missing_docs)]

use std::env;
use std::fs;
use std::io::{self, Write};
use std::path::{Path, PathBuf};

use similar::{ChangeTag, TextDiff};
use thiserror::Error;

/// Lib result type.
pub type Result<T> = std::result::Result<T, GoldenError>;

/// Error type.
#[derive(Error, Debug)]
pub enum GoldenError {
    /// I/O error.
    #[error("io: {0}")]
    Io(#[from] io::Error),
    /// UTF-8 decoding error.
    #[error("utf8: {0}")]
    Utf8(#[from] std::string::FromUtf8Error),
    /// Compare error with diff preview.
    #[error("mismatch:\n{0}")]
    Mismatch(String),
    /// Invalid argument.
    #[error("invalid: {0}")]
    Invalid(&'static str),
    /// JSON feature not enabled.
    #[error("json feature not enabled")]
    JsonFeatureDisabled,
    /// JSON error.
    #[cfg(feature = "serde_json")]
    #[error("json: {0}")]
    Json(#[from] serde_json::Error),
}

/// Options controlling comparison and normalization.
#[derive(Clone, Debug)]
pub struct GoldenOptions {
    /// Normalize line endings to `\n`.
    pub normalize_line_endings: bool,
    /// Trim trailing whitespace on each line.
    pub trim_trailing_ws: bool,
    /// Ensure text ends with a single trailing newline.
    pub ensure_trailing_newline: bool,
    /// Ignore differences in leading/trailing whitespace on whole text.
    pub trim_text: bool,
    /// If true, produce unified diff, else inline diff.
    pub unified_diff: bool,
}

impl Default for GoldenOptions {
    fn default() -> Self {
        Self {
            normalize_line_endings: true,
            trim_trailing_ws: false,
            ensure_trailing_newline: true,
            trim_text: false,
            unified_diff: true,
        }
    }
}

/// Outcome of a comparison.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Outcome {
    /// Files matched.
    Match,
    /// File created because it did not exist and update mode was enabled.
    Created,
    /// File updated to match actual output.
    Updated,
}

/// Assert-like convenience: compare text golden at `path` to `actual`.
/// Panics with readable diff when mismatch and not in update mode.
#[macro_export]
macro_rules! golden_assert_text_eq {
    ($path:expr, $actual:expr) => {{
        let opts = $crate::GoldenOptions::default();
        $crate::compare_text($path, $actual, &opts).unwrap();
    }};
    ($path:expr, $actual:expr, $opts:expr) => {{
        $crate::compare_text($path, $actual, $opts).unwrap();
    }};
}

/// Assert-like convenience: compare binary golden at `path` to `actual`.
#[macro_export]
macro_rules! golden_assert_bytes_eq {
    ($path:expr, $actual:expr) => {{
        $crate::compare_bytes($path, $actual).unwrap();
    }};
}

/// Returns true if update mode is active: VITTE_GOLDEN_UPDATE=1 (or "true") and not on CI.
/// CI is detected via common envs (CI=true, GITHUB_ACTIONS, GITLAB_CI, BUILD_NUMBER).
pub fn is_update_mode() -> bool {
    let v = env_var_truthy("VITTE_GOLDEN_UPDATE").or_else(|| env_var_truthy("UPDATE_GOLDEN"));
    let ci = env_var_truthy("CI")
        .or_else(|| env_var_truthy("GITHUB_ACTIONS"))
        .or_else(|| env_var_truthy("GITLAB_CI"))
        .or_else(|| env::var("BUILD_NUMBER").ok().map(|_| true));
    matches!(v, Some(true)) && !matches!(ci, Some(true))
}

/// Compare text with golden file at `golden_path`.
/// Will auto-create or update if `is_update_mode()` is true.
/// Returns `Outcome` when an update happens, or `Ok(())` on match.
/// On mismatch without update, returns `GoldenError::Mismatch` with diff.
pub fn compare_text<P: AsRef<Path>>(
    golden_path: P,
    actual_text: impl AsRef<str>,
    opts: &GoldenOptions,
) -> Result<()> {
    let golden_path = resolve_test_path(golden_path);
    let actual = normalize_text(actual_text.as_ref(), opts);

    if !golden_path.exists() {
        if is_update_mode() {
            write_text_atomic(&golden_path, &actual)?;
            eprintln!("[vitte-golden] created {}", golden_path.display());
            return Ok(());
        }
        return Err(GoldenError::Mismatch(missing_file_msg(&golden_path, &actual)));
    }

    let expected_raw = fs::read_to_string(&golden_path)?;
    let expected = normalize_text(&expected_raw, opts);

    if expected == actual {
        return Ok(());
    }

    if is_update_mode() {
        write_text_atomic(&golden_path, &actual)?;
        eprintln!("[vitte-golden] updated {}", golden_path.display());
        return Ok(());
    }

    let diff = render_diff(&expected, &actual, opts.unified_diff, &golden_path);
    Err(GoldenError::Mismatch(diff))
}

/// Compare raw bytes with golden file at `golden_path`.
pub fn compare_bytes<P: AsRef<Path>>(golden_path: P, actual: impl AsRef<[u8]>) -> Result<()> {
    let golden_path = resolve_test_path(golden_path);
    let actual = actual.as_ref().to_vec();

    if !golden_path.exists() {
        if is_update_mode() {
            write_bytes_atomic(&golden_path, &actual)?;
            eprintln!("[vitte-golden] created {}", golden_path.display());
            return Ok(());
        }
        return Err(GoldenError::Mismatch(format!(
            "golden missing: {}\n(len actual: {})",
            golden_path.display(),
            actual.len()
        )));
    }

    let expected = fs::read(&golden_path)?;
    if expected == actual {
        return Ok(());
    }

    if is_update_mode() {
        write_bytes_atomic(&golden_path, &actual)?;
        eprintln!("[vitte-golden] updated {}", golden_path.display());
        return Ok(());
    }

    // Produce a small preview diff in hex for first bytes
    let preview = hex_preview(&expected, &actual, 64);
    Err(GoldenError::Mismatch(format!("binary mismatch: {}\n{}", golden_path.display(), preview)))
}

/// Compare JSON values using stable pretty formatting (feature `serde_json`).
/// Normalizes both sides with sorted keys and pretty output to reduce noise.
#[cfg(feature = "serde_json")]
pub fn compare_json<P: AsRef<Path>, T: serde::Serialize>(golden_path: P, actual: &T) -> Result<()> {
    use serde_json::{to_value, Value};

    let golden_path = resolve_test_path(golden_path);
    let actual_val: Value = to_value(actual)?;
    let actual_norm = normalize_json(&actual_val);
    let actual_pretty = serde_json::to_string_pretty(&actual_norm)? + "\n";

    if !golden_path.exists() {
        if is_update_mode() {
            write_text_atomic(&golden_path, &actual_pretty)?;
            eprintln!("[vitte-golden] created {}", golden_path.display());
            return Ok(());
        }
        return Err(GoldenError::Mismatch(missing_file_msg(&golden_path, &actual_pretty)));
    }

    let expected_raw = fs::read_to_string(&golden_path)?;
    let expected_val: Value = serde_json::from_str(&expected_raw)?;
    let expected_norm = normalize_json(&expected_val);
    let expected_pretty = serde_json::to_string_pretty(&expected_norm)? + "\n";

    if expected_pretty == actual_pretty {
        return Ok(());
    }

    if is_update_mode() {
        write_text_atomic(&golden_path, &actual_pretty)?;
        eprintln!("[vitte-golden] updated {}", golden_path.display());
        return Ok(());
    }

    let diff = render_diff(&expected_pretty, &actual_pretty, true, &golden_path);
    Err(GoldenError::Mismatch(diff))
}

/// Compare JSON using `serde_json::Value` input (feature `serde_json`).
#[cfg(feature = "serde_json")]
pub fn compare_json_value<P: AsRef<Path>>(
    golden_path: P,
    actual: &serde_json::Value,
) -> Result<()> {
    compare_json(golden_path, actual)
}

/// Fallback when JSON feature is disabled.
#[cfg(not(feature = "serde_json"))]
pub fn compare_json<P: AsRef<Path>, T>(_golden_path: P, _actual: &T) -> Result<()> {
    Err(GoldenError::JsonFeatureDisabled)
}

// --------------------- Path and I/O helpers ---------------------

/// Resolve a test-relative path. If the input is relative, anchor it at
/// `CARGO_MANIFEST_DIR` if set, else current directory.
pub fn resolve_test_path<P: AsRef<Path>>(p: P) -> PathBuf {
    let p = p.as_ref();
    if p.is_absolute() {
        return p.to_path_buf();
    }
    if let Ok(root) = env::var("CARGO_MANIFEST_DIR") {
        return Path::new(&root).join(p);
    }
    env::current_dir().unwrap_or_else(|_| PathBuf::from(".")).join(p)
}

/// Create parent dirs then write atomically: write to temp sibling and rename.
pub fn write_text_atomic(path: &Path, s: &str) -> io::Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let tmp = tmp_sibling(path);
    {
        let mut f = fs::File::create(&tmp)?;
        f.write_all(s.as_bytes())?;
        f.sync_all()?;
    }
    fs::rename(tmp, path)?;
    Ok(())
}

/// Binary variant of `write_text_atomic`.
pub fn write_bytes_atomic(path: &Path, data: &[u8]) -> io::Result<()> {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    let tmp = tmp_sibling(path);
    {
        let mut f = fs::File::create(&tmp)?;
        f.write_all(data)?;
        f.sync_all()?;
    }
    fs::rename(tmp, path)?;
    Ok(())
}

fn tmp_sibling(path: &Path) -> PathBuf {
    use std::time::{SystemTime, UNIX_EPOCH};
    let ts = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_nanos();
    let stem = path.file_name().unwrap_or_default().to_string_lossy();
    let mut tmp =
        path.parent().unwrap_or_else(|| Path::new(".")).join(format!("{}.{}.tmp", stem, ts));
    // if collision, add random suffix
    if tmp.exists() {
        tmp = tmp.with_extension(format!("tmp.{}", ts ^ 0x9e3779b97f4a7c15));
    }
    tmp
}

// --------------------- Normalization and diff ---------------------

fn normalize_text(s: &str, opts: &GoldenOptions) -> String {
    let mut text = s.to_owned();
    if opts.normalize_line_endings {
        text = text.replace("\r\n", "\n").replace('\r', "\n");
    }
    if opts.trim_text {
        text = text.trim().to_owned();
    }
    if opts.trim_trailing_ws {
        let mut out = String::with_capacity(text.len());
        for line in text.split_inclusive('\n') {
            if let Some(stripped) = line.strip_suffix('\n') {
                out.push_str(stripped.trim_end());
                out.push('\n');
            } else {
                out.push_str(line.trim_end());
            }
        }
        text = out;
    }
    if opts.ensure_trailing_newline && !text.is_empty() && !text.ends_with('\n') {
        text.push('\n');
    }
    text
}

fn render_diff(expected: &str, actual: &str, unified: bool, golden_path: &Path) -> String {
    if unified {
        let diff = TextDiff::from_lines(expected, actual);
        let mut out = String::new();
        out.push_str(&format!("--- expected: {}\n+++ actual\n", golden_path.display()));
        for op in diff.ops() {
            for change in diff.iter_changes(op) {
                let sign = match change.tag() {
                    ChangeTag::Delete => "-",
                    ChangeTag::Insert => "+",
                    ChangeTag::Equal => " ",
                };
                for line in change.value().split_inclusive('\n') {
                    out.push_str(sign);
                    out.push_str(line);
                    if !line.ends_with('\n') {
                        out.push('\n');
                    }
                }
            }
        }
        out
    } else {
        let diff = TextDiff::from_lines(expected, actual);
        let mut out = String::new();
        out.push_str("diff (inline):\n");
        for (idx, group) in diff.grouped_ops(3).iter().enumerate() {
            if idx > 0 {
                out.push_str("...\n");
            }
            for op in group {
                for change in diff.iter_changes(op) {
                    let sign = match change.tag() {
                        ChangeTag::Delete => "-",
                        ChangeTag::Insert => "+",
                        ChangeTag::Equal => " ",
                    };
                    out.push_str(sign);
                    out.push_str(change.value());
                }
            }
        }
        out
    }
}

fn hex_preview(expected: &[u8], actual: &[u8], bytes: usize) -> String {
    fn hex(first: &[u8]) -> String {
        first.iter().map(|b| format!("{:02x}", b)).collect::<Vec<_>>().join(" ")
    }
    let e = &expected[..expected.len().min(bytes)];
    let a = &actual[..actual.len().min(bytes)];
    format!(
        "expected len={} head[{}]: {}\nactual   len={} head[{}]: {}",
        expected.len(),
        e.len(),
        hex(e),
        actual.len(),
        a.len(),
        hex(a)
    )
}

fn env_var_truthy(name: &str) -> Option<bool> {
    match env::var(name).ok()?.to_ascii_lowercase().as_str() {
        "1" | "true" | "yes" | "on" => Some(true),
        "0" | "false" | "no" | "off" => Some(false),
        _ => Some(true), // any non-empty value counts as true
    }
}

fn missing_file_msg(path: &Path, actual: &str) -> String {
    format!(
        "golden missing: {}\nset VITTE_GOLDEN_UPDATE=1 to create it.\n--- actual begin ---\n{}--- actual end ---",
        path.display(),
        actual
    )
}

// --------------------- JSON normalization (feature) ---------------------

#[cfg(feature = "serde_json")]
fn normalize_json(v: &serde_json::Value) -> serde_json::Value {
    use serde_json::{Map, Value};
    match v {
        Value::Object(m) => {
            // sort keys
            let mut keys: Vec<_> = m.keys().cloned().collect();
            keys.sort_unstable();
            let mut nm = Map::with_capacity(keys.len());
            for k in keys {
                nm.insert(k.clone(), normalize_json(&m[&k]));
            }
            Value::Object(nm)
        }
        Value::Array(arr) => Value::Array(arr.iter().map(normalize_json).collect()),
        _ => v.clone(),
    }
}

// --------------------- Tests ---------------------

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;

    fn tmpdir() -> PathBuf {
        let mut p = env::temp_dir();
        p.push(format!("vitte-golden-{}", std::process::id()));
        fs::create_dir_all(&p).unwrap();
        p
    }

    #[test]
    fn normalize_basic() {
        let opts = GoldenOptions::default();
        assert_eq!(normalize_text("a\r\nb\r\n", &opts), "a\nb\n");
        let mut opts2 = opts.clone();
        opts2.trim_trailing_ws = true;
        assert_eq!(normalize_text("x  \ny\t\n", &opts2), "x\ny\n");
    }

    #[test]
    fn text_compare_equal() {
        let dir = tmpdir();
        let g = dir.join("eq.txt");
        write_text_atomic(&g, "abc\n").unwrap();
        compare_text(&g, "abc\n", &GoldenOptions::default()).unwrap();
    }

    #[test]
    fn text_compare_mismatch_produces_diff() {
        let dir = tmpdir();
        let g = dir.join("diff.txt");
        write_text_atomic(&g, "hello\n").unwrap();
        let err = compare_text(&g, "world\n", &GoldenOptions::default()).unwrap_err();
        match err {
            GoldenError::Mismatch(s) => assert!(s.contains("hello")),
            _ => panic!("expected mismatch"),
        }
    }

    #[test]
    fn bytes_compare_equal() {
        let dir = tmpdir();
        let g = dir.join("bin.bin");
        write_bytes_atomic(&g, b"\x00\x01").unwrap();
        compare_bytes(&g, b"\x00\x01").unwrap();
    }

    #[cfg(feature = "serde_json")]
    #[test]
    fn json_compare_stable_keys() {
        let dir = tmpdir();
        let g = dir.join("j.json");
        write_text_atomic(&g, "{\n  \"b\": 2,\n  \"a\": 1\n}\n").unwrap();
        #[derive(serde::Serialize)]
        struct Obj {
            a: u8,
            b: u8,
        }
        compare_json(&g, &Obj { a: 1, b: 2 }).unwrap();
    }
}
