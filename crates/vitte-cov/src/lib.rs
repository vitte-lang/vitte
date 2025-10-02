//! vitte-cov — library API
//!
//! Goals
//! - One API to run coverage, merge profiles, and export reports.
//! - Works with `llvm-cov`/`llvm-profdata` when `feature="llvm"`.
//! - Optional LCOV export and bare HTML via `minijinja` when `feature="html"`.
//!
//! Notes
//! - This crate only orchestrates. External tools must be available on PATH.
//! - File paths are UTF-8 (camino). No global mutable state.

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

extern crate alloc;

use alloc::{string::String, vec, vec::Vec};

use camino::{Utf8Path, Utf8PathBuf};
use regex::Regex;

#[cfg(feature = "std")]
use std::time::Duration;

#[cfg(feature = "std")]
use std::process::Stdio;

#[cfg(feature = "std")]
use duct::cmd;

/// Crate result.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Error kinds.
#[derive(thiserror::Error, Debug)]
pub enum Error {
    /// Tools not available or feature disabled.
    #[error("unsupported: {0}")]
    Unsupported(&'static str),
    /// Tool not found or not executable.
    #[error("tool not found: {0}")]
    ToolNotFound(&'static str),
    /// External command failed.
    #[error("process failed: {tool} (code={code:?})\n{stderr}")]
    Process {
        /// Tool name.
        tool: &'static str,
        /// Exit code.
        code: Option<i32>,
        /// Stderr content (truncated).
        stderr: String,
    },
    /// I/O or env error.
    #[error("io: {0}")]
    Io(String),
    /// Parse error.
    #[error("parse: {0}")]
    Parse(String),
}

/// Coverage toolchain discovered on PATH.
#[derive(Debug, Clone)]
pub struct LlvmTools {
    /// Path to `llvm-profdata`.
    pub profdata: Utf8PathBuf,
    /// Path to `llvm-cov`.
    pub cov: Utf8PathBuf,
}

/// High-level mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Mode {
    /// Run unit tests with coverage instrumentation, then report.
    Tests,
    /// Run a binary target under coverage (command provided).
    Binary,
}

/// What to export.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Export {
    /// Human text summary to stdout.
    Text,
    /// LCOV file.
    Lcov,
    /// HTML directory.
    Html,
}

/// Minimal configuration.
#[derive(Debug, Clone)]
pub struct Config {
    /// Workspace or crate root.
    pub manifest_dir: Utf8PathBuf,
    /// Where to write temporary and output artifacts.
    pub out_dir: Utf8PathBuf,
    /// Which packages (Cargo) to include. Empty → current/auto.
    pub packages: Vec<String>,
    /// Mode.
    pub mode: Mode,
    /// Export kind.
    pub export: Export,
    /// Optional path to LCOV file or HTML dir depending on export.
    pub export_path: Utf8PathBuf,
    /// Additional include filters (regex applied to file paths).
    pub include: Vec<String>,
    /// Additional exclude filters.
    pub exclude: Vec<String>,
    /// Additional args for `cargo test` or the binary command.
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

/// Summary line for a file.
#[derive(Debug, Clone)]
pub struct FileSummary {
    /// File path.
    pub file: String,
    /// Lines covered.
    pub lines_covered: u64,
    /// Lines total.
    pub lines_total: u64,
}

/// Global summary.
#[derive(Debug, Clone)]
pub struct Summary {
    /// Per-file.
    pub files: Vec<FileSummary>,
    /// Totals.
    pub total_lines: u64,
    /// Covered total.
    pub total_covered: u64,
    /// Percent [0..=10000] basis points (e.g., 8123 = 81.23%).
    pub percent_bp: u32,
}

impl Summary {
    fn from_pairs(pairs: &[(String, (u64, u64))]) -> Self {
        let mut files = Vec::with_capacity(pairs.len());
        let mut tot = 0u64;
        let mut cov = 0u64;
        for (f, (c, t)) in pairs {
            files.push(FileSummary { file: f.clone(), lines_covered: *c, lines_total: *t });
            cov += *c;
            tot += *t;
        }
        let percent_bp = if tot == 0 { 0 } else { ((cov as u128) * 10_000u128 / (tot as u128)) as u32 };
        Self { files, total_lines: tot, total_covered: cov, percent_bp }
    }
}

/* ------------------------------- Discovery ------------------------------ */

/// Find `llvm-profdata` and `llvm-cov` on PATH.
#[cfg(feature = "llvm")]
pub fn find_llvm_tools() -> Result<LlvmTools> {
    let prof = which::which("llvm-profdata").map_err(|_| Error::ToolNotFound("llvm-profdata"))?;
    let cov = which::which("llvm-cov").map_err(|_| Error::ToolNotFound("llvm-cov"))?;
    Ok(LlvmTools { profdata: pathbuf(prof), cov: pathbuf(cov) })
}

#[cfg(not(feature = "llvm"))]
pub fn find_llvm_tools() -> Result<LlvmTools> { Err(Error::Unsupported("llvm feature disabled")) }

#[cfg(feature = "llvm")]
fn pathbuf(p: std::path::PathBuf) -> Utf8PathBuf {
    Utf8PathBuf::from_path_buf(p).expect("utf8 path")
}

/* --------------------------------- Flow -------------------------------- */

/// End-to-end: run coverage and export report. Returns computed summary.
pub fn run(config: &Config) -> Result<Summary> {
    #[cfg(not(all(feature = "std", feature = "llvm")))]
    { return Err(Error::Unsupported("requires features: std + llvm")); }

    #[cfg(all(feature = "std", feature = "llvm"))]
    {
        let tools = find_llvm_tools()?;
        ensure_dirs(&config.out_dir)?;
        let profraw_dir = config.out_dir.join("profraw");
        ensure_dirs(&profraw_dir)?;

        // 1) Run instrumented workload to produce *.profraw
        match config.mode {
            Mode::Tests => run_cargo_tests_with_coverage(&config.manifest_dir, &profraw_dir, &config.packages, &config.extra_args)?,
            Mode::Binary => return Err(Error::Unsupported("binary mode orchestration not wired; run your cmd with LLVM_PROFILE_FILE then call merge+report")),
        }

        // 2) Merge profiles into .profdata
        let profdata_path = config.out_dir.join("coverage.profdata");
        merge_profraw(&tools, &profraw_dir, &profdata_path)?;

        // 3) Report with llvm-cov show -format=lcov | text | html
        let (pairs, lcov_text_opt, html_dir_opt) = match config.export {
            Export::Text => {
                let text = cov_text(&tools, &config.manifest_dir, &profdata_path, &config.include, &config.exclude)?;
                (parse_text_summary(&text)?, None, None)
            }
            Export::Lcov => {
                let lcov = cov_lcov(&tools, &config.manifest_dir, &profdata_path, &config.include, &config.exclude)?;
                write_all(&config.export_path, lcov.as_bytes())?;
                (parse_lcov_summary(&lcov)?, Some(lcov), None)
            }
            Export::Html => {
                let dir = if config.export_path.as_str().is_empty() {
                    config.out_dir.join("html")
                } else { config.export_path.clone() };
                cov_html(&tools, &config.manifest_dir, &profdata_path, &dir, &config.include, &config.exclude)?;
                // For HTML we still extract a text summary for the return value.
                let text = cov_text(&tools, &config.manifest_dir, &profdata_path, &config.include, &config.exclude)?;
                (parse_text_summary(&text)?, None, Some(dir))
            }
        };

        let sum = Summary::from_pairs(&pairs);
        // Best-effort write a small summary.txt
        let _ = write_summary(&config.out_dir.join("summary.txt"), &sum);

        // If LCOV was requested and path wasn't a file, it's handled above.
        let _ = (lcov_text_opt, html_dir_opt); // silence

        Ok(sum)
    }
}

/* ------------------------------- Execution ------------------------------ */

#[cfg(all(feature = "std", feature = "llvm"))]
fn run_cargo_tests_with_coverage(manifest_dir: &Utf8Path, profraw_dir: &Utf8Path, pkgs: &[String], extra: &[String]) -> Result<()> {
    let profile_pat = profraw_dir.join("%p-%m.profraw");
    // RUSTFLAGS: enable coverage instrumentation
    let mut envs = vec![
        ("LLVM_PROFILE_FILE", profile_pat.as_str()),
        ("RUSTFLAGS", "-Zinstrument-coverage"),
        ("RUSTC_BOOTSTRAP", "1"),
        ("CARGO_INCREMENTAL", "0"),
    ];

    // Cargo args
    let mut args = vec!["test", "--tests", "--all-features", "--no-fail-fast", "--manifest-path", &manifest_dir.join("Cargo.toml").to_string()];
    if !pkgs.is_empty() {
        for p in pkgs {
            args.push("-p");
            args.push(p.as_str());
        }
    }
    for a in extra { args.push(a); }

    let out = cmd("cargo", args)
        .envs(envs.into_iter())
        .stdout(Stdio::inherit())
        .stderr_capture()
        .unchecked() // we want to read code
        .run()
        .map_err(|e| Error::Io(e.to_string()))?;

    if !out.status.success() {
        let code = out.status.code();
        let stderr = String::from_utf8_lossy(&out.stderr).into_owned();
        return Err(Error::Process { tool: "cargo test", code, stderr: truncate(stderr) });
    }
    Ok(())
}

#[cfg(all(feature = "std", feature = "llvm"))]
fn merge_profraw(tools: &LlvmTools, profraw_dir: &Utf8Path, out_profdata: &Utf8Path) -> Result<()> {
    // Gather all *.profraw
    let mut list: Vec<String> = vec![];
    for entry in std::fs::read_dir(profraw_dir.as_std_path()).map_err(|e| Error::Io(e.to_string()))? {
        let entry = entry.map_err(|e| Error::Io(e.to_string()))?;
        if let Some(ext) = entry.path().extension() {
            if ext == "profraw" {
                list.push(entry.path().to_string_lossy().into_owned());
            }
        }
    }
    if list.is_empty() {
        return Err(Error::Io("no .profraw files found; did tests run under coverage?".into()));
    }

    let args = {
        let mut a = vec!["merge", "-sparse"];
        for p in &list { a.push(p.as_str()); }
        a.push("-o");
        a.push(out_profdata.as_str());
        a
    };

    let out = cmd(tools.profdata.as_str(), args)
        .stderr_capture()
        .unchecked()
        .run()
        .map_err(|e| Error::Io(e.to_string()))?;

    if !out.status.success() {
        return Err(Error::Process {
            tool: "llvm-profdata merge",
            code: out.status.code(),
            stderr: truncate(String::from_utf8_lossy(&out.stderr).into_owned()),
        });
    }
    Ok(())
}

#[cfg(all(feature = "std", feature = "llvm"))]
fn cov_text(tools: &LlvmTools, manifest_dir: &Utf8Path, profdata: &Utf8Path, include: &[String], exclude: &[String]) -> Result<String> {
    let mut args = base_cov_args(manifest_dir, profdata)?;
    args.extend_from_slice(&["--summary-only", "--ignore-filename-regex", ignore_regex(include, exclude).as_str()]);
    let out = cmd(tools.cov.as_str(), args).stderr_capture().stdout_capture().unchecked().run()
        .map_err(|e| Error::Io(e.to_string()))?;
    if !out.status.success() {
        return Err(Error::Process { tool: "llvm-cov show", code: out.status.code(), stderr: truncate(String::from_utf8_lossy(&out.stderr).into_owned()) });
    }
    Ok(String::from_utf8_lossy(&out.stdout).into_owned())
}

#[cfg(all(feature = "std", feature = "llvm"))]
fn cov_lcov(tools: &LlvmTools, manifest_dir: &Utf8Path, profdata: &Utf8Path, include: &[String], exclude: &[String]) -> Result<String> {
    let mut args = base_cov_args(manifest_dir, profdata)?;
    args.extend_from_slice(&["--format=lcov", "--ignore-filename-regex", ignore_regex(include, exclude).as_str()]);
    let out = cmd(tools.cov.as_str(), args).stderr_capture().stdout_capture().unchecked().run()
        .map_err(|e| Error::Io(e.to_string()))?;
    if !out.status.success() {
        return Err(Error::Process { tool: "llvm-cov show", code: out.status.code(), stderr: truncate(String::from_utf8_lossy(&out.stderr).into_owned()) });
    }
    Ok(String::from_utf8_lossy(&out.stdout).into_owned())
}

#[cfg(all(feature = "std", feature = "llvm"))]
fn cov_html(tools: &LlvmTools, manifest_dir: &Utf8Path, profdata: &Utf8Path, out_dir: &Utf8Path, include: &[String], exclude: &[String]) -> Result<()> {
    ensure_dirs(out_dir)?;
    let mut args = base_cov_args(manifest_dir, profdata)?;
    args.extend_from_slice(&["--format=html", "--output-dir", out_dir.as_str(), "--ignore-filename-regex", ignore_regex(include, exclude).as_str()]);
    let out = cmd(tools.cov.as_str(), args).stderr_capture().stdout_capture().unchecked().run()
        .map_err(|e| Error::Io(e.to_string()))?;
    if !out.status.success() {
        return Err(Error::Process { tool: "llvm-cov show", code: out.status.code(), stderr: truncate(String::from_utf8_lossy(&out.stderr).into_owned()) });
    }
    Ok(())
}

#[cfg(all(feature = "std", feature = "llvm"))]
fn base_cov_args(manifest_dir: &Utf8Path, profdata: &Utf8Path) -> Result<Vec<&str>> {
    // `cargo test` places binaries in target/{debug,release}/deps. We rely on `cargo metadata` to locate the target dir,
    // but to keep deps minimal here we assume default target/debug. Users can pass `--target-dir` using Cargo config too.
    let target_dir = manifest_dir.join("target").join("debug");
    // Find binaries with coverage mapping. Heuristic: any executable in deps/.
    let mut objects: Vec<Utf8PathBuf> = Vec::new();
    let deps = target_dir.join("deps");
    if let Ok(rd) = std::fs::read_dir(deps.as_std_path()) {
        for e in rd {
            let e = e.map_err(|e| Error::Io(e.to_string()))?;
            let p = Utf8PathBuf::from_path_buf(e.path()).ok();
            if let Some(p) = p {
                if is_executable(&p) {
                    objects.push(p);
                }
            }
        }
    }
    if objects.is_empty() {
        return Err(Error::Io("no instrumented binaries found in target/debug/deps; ensure tests were built with -Zinstrument-coverage".into()));
    }
    let mut args: Vec<&str> = vec!["show", "--instr-profile", profdata.as_str()];
    for o in &objects { args.push("--object"); args.push(o.as_str()); }
    Ok(args)
}

#[cfg(all(feature = "std", feature = "llvm"))]
fn is_executable(p: &Utf8Path) -> bool {
    if p.as_str().ends_with(".dSYM") || p.as_str().ends_with(".rlib") || p.as_str().ends_with(".rmeta") { return false; }
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        if let Ok(md) = std::fs::metadata(p) {
            return md.is_file() && (md.permissions().mode() & 0o111) != 0;
        }
    }
    #[cfg(windows)]
    {
        return p.as_str().ends_with(".exe");
    }
    false
}

/* -------------------------------- Parsing ------------------------------- */

/// Parse `llvm-cov show --summary-only` text to per-file coverage.
fn parse_text_summary(s: &str) -> Result<Vec<(String, (u64, u64))>> {
    // Accept lines like:
    //  filename.rs:  lines: 123/150 (82.00%)
    // Be tolerant to spaces.
    let re = Regex::new(r#"^\s*(?P<f>.+?):\s+lines:\s+(?P<c>\d+)\s*/\s*(?P<t>\d+)\s*\("#)
        .map_err(|e| Error::Parse(e.to_string()))?;
    let mut out = Vec::new();
    for line in s.lines() {
        if let Some(cap) = re.captures(line) {
            let f = cap["f"].to_string();
            let c: u64 = cap["c"].parse().map_err(|e| Error::Parse(e.to_string()))?;
            let t: u64 = cap["t"].parse().map_err(|e| Error::Parse(e.to_string()))?;
            out.push((f, (c, t)));
        }
    }
    if out.is_empty() && !s.is_empty() {
        // Try total line fallback: "TOTAL:  123/200"
        let re2 = Regex::new(r#"(?i)total:\s*(?P<c>\d+)\s*/\s*(?P<t>\d+)"#).map_err(|e| Error::Parse(e.to_string()))?;
        if let Some(c2) = re2.captures(s) {
            let c: u64 = c2["c"].parse().map_err(|e| Error::Parse(e.to_string()))?;
            let t: u64 = c2["t"].parse().map_err(|e| Error::Parse(e.to_string()))?;
            out.push(("TOTAL".into(), (c, t)));
        }
    }
    Ok(out)
}

/// Parse LCOV to per-file counts.
fn parse_lcov_summary(s: &str) -> Result<Vec<(String, (u64, u64))>> {
    let mut out = Vec::new();
    let mut file = String::new();
    let mut hit = 0u64;
    let mut total = 0u64;
    for line in s.lines() {
        if let Some(rest) = line.strip_prefix("SF:") {
            if !file.is_empty() {
                out.push((core::mem::take(&mut file), (hit, total)));
                hit = 0; total = 0;
            }
            file = rest.trim().to_string();
        } else if let Some(rest) = line.strip_prefix("DA:") {
            // DA:<line>,<count>
            if let Some((_ln, cnt)) = rest.split_once(',') {
                let c: u64 = cnt.trim().parse().unwrap_or(0);
                total += 1;
                if c > 0 { hit += 1; }
            }
        } else if line == "end_of_record" && !file.is_empty() {
            out.push((core::mem::take(&mut file), (hit, total)));
            hit = 0; total = 0;
        }
    }
    Ok(out)
}

/* --------------------------------- HTML -------------------------------- */

/// Render a tiny HTML summary if `feature="html"` and no external HTML used.
#[cfg(all(feature = "html", feature = "std"))]
pub fn render_html_summary(dir: &Utf8Path, sum: &Summary) -> Result<()> {
    ensure_dirs(dir)?;
    let tpl = r#"<!doctype html><meta charset="utf-8"><title>Coverage</title>
<style>body{font-family:system-ui,Segoe UI,Roboto,Helvetica,Arial;font-size:14px;padding:24px}
h1{font-size:20px} table{border-collapse:collapse} td,th{border:1px solid #ddd;padding:6px 10px}
tr:nth-child(even){background:#fafafa}.pct{font-weight:600}</style>
<h1>Coverage summary</h1>
<p class="pct">{{ (sum.percent_bp/100) }}.%{{ (sum.percent_bp%100)|format("%02d") }}</p>
<table><thead><tr><th>File</th><th>Covered</th><th>Total</th><th>%</th></tr></thead><tbody>
{% for f in sum.files -%}
<tr><td>{{ f.file }}</td><td>{{ f.lines_covered }}</td><td>{{ f.lines_total }}</td>
<td>{{ (10000 * f.lines_covered // (f.lines_total if f.lines_total>0 else 1)) / 100 }}.%{{ (10000 * f.lines_covered // (f.lines_total if f.lines_total>0 else 1)) % 100 | format("%02d") }}</td></tr>
{% endfor -%}
</tbody></table>"#;

    let mut env = minijinja::Environment::new();
    env.add_template("summary.html", tpl).map_err(|e| Error::Io(e.to_string()))?;
    let tmpl = env.get_template("summary.html").map_err(|e| Error::Io(e.to_string()))?;
    use minijinja::value::Value as Jv;
    let ctx = jinja_summary(sum);
    let html = tmpl.render(ctx).map_err(|e| Error::Io(e.to_string()))?;
    write_all(&dir.join("index.html"), html.as_bytes())
}

#[cfg(all(feature = "html", feature = "std"))]
fn jinja_summary(sum: &Summary) -> minijinja::value::Value {
    use minijinja::value::{Value as Jv, ValueKind, Object};
    use alloc::collections::BTreeMap;
    let mut files: Vec<BTreeMap<&str, Jv>> = Vec::new();
    for f in &sum.files {
        let mut m = BTreeMap::new();
        m.insert("file", Jv::from(f.file.clone()));
        m.insert("lines_covered", Jv::from(f.lines_covered as i64));
        m.insert("lines_total", Jv::from(f.lines_total as i64));
        files.push(m);
    }
    let mut root = BTreeMap::new();
    root.insert("files", Jv::from(files));
    root.insert("percent_bp", Jv::from(sum.percent_bp as i64));
    Jv::from(root)
}

/* --------------------------------- Utils -------------------------------- */

#[cfg(feature = "std")]
fn ensure_dirs(p: &Utf8Path) -> Result<()> {
    std::fs::create_dir_all(p.as_std_path()).map_err(|e| Error::Io(e.to_string()))
}

#[cfg(feature = "std")]
fn write_all(p: &Utf8Path, bytes: &[u8]) -> Result<()> {
    if let Some(parent) = p.parent() { ensure_dirs(parent)?; }
    std::fs::write(p.as_std_path(), bytes).map_err(|e| Error::Io(e.to_string()))
}

fn truncate(mut s: String) -> String {
    const MAX: usize = 24_000;
    if s.len() > MAX { s.truncate(MAX); }
    s
}

/* --------------------------------- Tests -------------------------------- */

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;

    #[test]
    fn parse_text() {
        let txt = "src/lib.rs:  lines: 12/20 (60.00%)\nTOTAL:  12/20 (60%)\n";
        let v = parse_text_summary(txt).unwrap();
        assert!(v.iter().any(|(f, _)| f.contains("src/lib.rs")));
    }

    #[test]
    fn parse_lcov() {
        let lcov = "TN:\nSF:/tmp/a.rs\nDA:1,1\nDA:2,0\nend_of_record\n";
        let v = parse_lcov_summary(lcov).unwrap();
        assert_eq!(v[0].1, (1, 2));
    }

    #[test]
    fn bp_math() {
        let s = Summary::from_pairs(&[("a".into(), (50, 100))]);
        assert_eq!(s.percent_bp, 5000);
    }
}