//! vitte-cov — coverage tooling stubs for the Vitte workspace.
//!
//! The original design orchestrated `llvm-profdata`/`llvm-cov` invocations and
//! parsed their output. For now we provide a lightweight placeholder so that the
//! workspace builds on stable Rust without requiring external LLVM tooling.
//!
//! The public API intentionally keeps the high-level types (`Config`,
//! `Summary`, etc.) so other crates depending on `vitte-cov` continue to
//! compile. Each operation simply reports that coverage support is not yet
//! available.

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

extern crate alloc;

use alloc::{string::String, vec, vec::Vec};
use camino::{Utf8Path, Utf8PathBuf};

/// Crate-wide result type.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Errors produced by the stub implementation.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// The requested capability is not currently implemented.
    #[error("coverage support unavailable: {0}")]
    Unsupported(&'static str),
}

/// Discovered LLVM tooling (stub).
#[derive(Debug, Clone)]
pub struct LlvmTools {
    /// Path to `llvm-profdata`.
    pub profdata: Utf8PathBuf,
    /// Path to `llvm-cov`.
    pub cov: Utf8PathBuf,
}

/// Attempt to locate LLVM tooling. Always returns [`Error::Unsupported`].
pub fn find_llvm_tools() -> Result<LlvmTools> {
    Err(Error::Unsupported("llvm tooling discovery is disabled in the stub"))
}

/// Coverage workflow mode (tests vs. custom binary).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Mode {
    /// Run `cargo test` under coverage instrumentation.
    Tests,
    /// Run an arbitrary binary (not implemented in the stub).
    Binary,
}

/// Output format desired by the caller.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Export {
    /// Text summary (stdout or file).
    Text,
    /// LCOV report.
    Lcov,
    /// HTML report tree.
    Html,
}

/// Configuration describing how coverage should be produced.
#[derive(Debug, Clone)]
pub struct Config {
    /// Workspace or crate root (Cargo manifest directory).
    pub manifest_dir: Utf8PathBuf,
    /// Directory for temporary/output artefacts.
    pub out_dir: Utf8PathBuf,
    /// Explicit package filters (`cargo test -p`).
    pub packages: Vec<String>,
    /// Coverage execution mode.
    pub mode: Mode,
    /// Desired export format.
    pub export: Export,
    /// Destination for export artefacts (file or directory).
    pub export_path: Utf8PathBuf,
    /// Regex-style include filters (placeholder only).
    pub include: Vec<String>,
    /// Regex-style exclude filters (placeholder only).
    pub exclude: Vec<String>,
    /// Additional arguments forwarded to the underlying command.
    pub extra_args: Vec<String>,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            manifest_dir: Utf8PathBuf::from("."),
            out_dir: Utf8PathBuf::from("target/coverage"),
            packages: vec![],
            mode: Mode::Tests,
            export: Export::Text,
            export_path: Utf8PathBuf::from("coverage.lcov"),
            include: vec![],
            exclude: vec![],
            extra_args: vec![],
        }
    }
}

/// Coverage metrics per file (placeholder structure).
#[derive(Debug, Clone, Default)]
pub struct FileSummary {
    /// File path in UTF-8.
    pub file: String,
    /// Lines reported as covered.
    pub lines_covered: u64,
    /// Total lines measured.
    pub lines_total: u64,
}

/// Aggregated summary across all files.
#[derive(Debug, Clone, Default)]
pub struct Summary {
    /// Individual file summaries.
    pub files: Vec<FileSummary>,
    /// Total number of lines measured.
    pub total_lines: u64,
    /// Total number of lines covered.
    pub total_covered: u64,
    /// Percentage basis points (0‥=10000) for convenience.
    pub percent_bp: u32,
}

impl Summary {
    /// Construct a summary from raw pairs (helper for future implementations).
    pub fn from_pairs(pairs: &[(String, (u64, u64))]) -> Self {
        let mut summary = Summary::default();
        for (file, (covered, total)) in pairs {
            summary.files.push(FileSummary {
                file: file.clone(),
                lines_covered: *covered,
                lines_total: *total,
            });
            summary.total_covered += *covered;
            summary.total_lines += *total;
        }
        summary.percent_bp = if summary.total_lines == 0 {
            0
        } else {
            ((summary.total_covered as u128 * 10_000u128) / summary.total_lines as u128) as u32
        };
        summary
    }
}

/// Execute the coverage pipeline. Currently returns [`Error::Unsupported`].
pub fn run(_config: &Config) -> Result<Summary> {
    Err(Error::Unsupported("coverage orchestration is stubbed out"))
}

/// Render a simple HTML summary into `dir`. Currently returns [`Error::Unsupported`].
pub fn render_html_summary(_dir: &Utf8Path, _summary: &Summary) -> Result<()> {
    Err(Error::Unsupported("HTML reporting is stubbed out"))
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn summary_math() {
        let pairs = vec![("a".to_string(), (5, 10)), ("b".to_string(), (3, 5))];
        let summary = Summary::from_pairs(&pairs);
        assert_eq!(summary.files.len(), 2);
        assert_eq!(summary.total_lines, 15);
        assert_eq!(summary.total_covered, 8);
        assert!(summary.percent_bp > 0);
    }

    #[test]
    fn run_stub() {
        let cfg = Config::default();
        let err = run(&cfg).unwrap_err();
        matches!(err, Error::Unsupported(_));
    }
}
