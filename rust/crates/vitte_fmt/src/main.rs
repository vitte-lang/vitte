// vitte/rust/crates/vitte_fmt/src/main.rs
//
// vitte-fmt — formatter tool (CLI)
//
// Goals:
//   - Read Vitte source files, format them deterministically, write back or stdout
//   - Support check mode (CI), diff mode (optional), stdin/stdout
//   - Respect Vitte "core + phrase" style (indentation + `.end` blocks)
//
// Architecture (pragmatic):
//   - main.rs: CLI parsing, IO, file walking
//   - formatter module: tokenization + lightweight formatting rules
//
// Notes:
//   - This is a tool crate (std required).
//   - Formatting is intentionally conservative (no semantic rewriting).
//
// Suggested future:
//   - Integrate parser AST when available
//   - Range formatting via LSP
//   - Config file (vittefmt.toml) + .editorconfig bridge
//
// Build: cargo run -p vitte_fmt -- <args>

use std::ffi::OsStr;
use std::fs;
use std::io::{self, Read, Write};
use std::path::{Path, PathBuf};
use std::time::Instant;

use vitte_common::err::{ErrCode, VResult, VitteError};
use vitte_common::{vitte_bail, vitte_err};

fn main() {
    let code = match run() {
        Ok(_) => 0,
        Err(e) => {
            eprintln!("{}", e);
            2
        }
    };
    std::process::exit(code);
}

// =============================================================================
// CLI
// =============================================================================

#[derive(Debug, Clone)]
struct Cli {
    inputs: Vec<PathBuf>,
    write: bool,     // --write (in-place)
    check: bool,     // --check (exit non-zero if changes)
    quiet: bool,     // --quiet
    stdin: bool,     // --stdin
    stdout: bool,    // --stdout
    recursive: bool, // --recursive
    ext: Vec<String>, // extensions to include
    line_ending: LineEnding,
    indent: IndentStyle,
    max_width: usize,
}

#[derive(Debug, Copy, Clone)]
enum LineEnding {
    Auto,
    Lf,
    CrLf,
}

#[derive(Debug, Copy, Clone)]
enum IndentStyle {
    Spaces(u8),
    Tabs(u8), // display width
}

impl Default for Cli {
    fn default() -> Self {
        Self {
            inputs: Vec::new(),
            write: false,
            check: false,
            quiet: false,
            stdin: false,
            stdout: false,
            recursive: true,
            ext: vec!["vitte".into(), "vit".into(), "vitl".into()],
            line_ending: LineEnding::Auto,
            indent: IndentStyle::Spaces(2),
            max_width: 100,
        }
    }
}

fn parse_args() -> VResult<Cli> {
    let mut cli = Cli::default();
    let mut it = std::env::args().skip(1).peekable();

    while let Some(arg) = it.next() {
        match arg.as_str() {
            "-h" | "--help" => {
                print_help();
                std::process::exit(0);
            }
            "-V" | "--version" => {
                println!("vitte-fmt {}", vitte_common::VITTE_COMMON_VERSION);
                std::process::exit(0);
            }
            "-w" | "--write" => cli.write = true,
            "--check" => cli.check = true,
            "-q" | "--quiet" => cli.quiet = true,
            "--stdin" => cli.stdin = true,
            "--stdout" => cli.stdout = true,
            "-r" | "--recursive" => cli.recursive = true,
            "--no-recursive" => cli.recursive = false,
            "--lf" => cli.line_ending = LineEnding::Lf,
            "--crlf" => cli.line_ending = LineEnding::CrLf,
            "--tabs" => {
                let n = it
                    .next()
                    .ok_or_else(|| vitte_err!(ErrCode::InvalidArgument, "--tabs requires a number"))?;
                let w: u8 = n.parse().map_err(|_| vitte_err!(ErrCode::InvalidArgument, "bad --tabs"))?;
                cli.indent = IndentStyle::Tabs(w.max(1));
            }
            "--spaces" => {
                let n = it
                    .next()
                    .ok_or_else(|| vitte_err!(ErrCode::InvalidArgument, "--spaces requires a number"))?;
                let w: u8 = n
                    .parse()
                    .map_err(|_| vitte_err!(ErrCode::InvalidArgument, "bad --spaces"))?;
                cli.indent = IndentStyle::Spaces(w.max(1));
            }
            "--max-width" => {
                let n = it
                    .next()
                    .ok_or_else(|| vitte_err!(ErrCode::InvalidArgument, "--max-width requires a number"))?;
                cli.max_width = n
                    .parse()
                    .map_err(|_| vitte_err!(ErrCode::InvalidArgument, "bad --max-width"))?;
                if cli.max_width < 40 {
                    vitte_bail!(ErrCode::OutOfRange, "--max-width too small");
                }
            }
            "--ext" => {
                let e = it
                    .next()
                    .ok_or_else(|| vitte_err!(ErrCode::InvalidArgument, "--ext requires a value"))?;
                cli.ext.push(e.trim_start_matches('.').to_string());
            }
            s if s.starts_with('-') => {
                return Err(vitte_err!(ErrCode::InvalidArgument, format!("unknown flag: {s}")));
            }
            path => cli.inputs.push(PathBuf::from(path)),
        }
    }

    if cli.stdin && !cli.inputs.is_empty() {
        return Err(vitte_err!(
            ErrCode::InvalidArgument,
            "--stdin cannot be combined with file paths"
        ));
    }

    if cli.stdout && cli.write {
        return Err(vitte_err!(
            ErrCode::InvalidArgument,
            "--stdout cannot be combined with --write"
        ));
    }

    if !cli.stdin && cli.inputs.is_empty() {
        // default to current directory
        cli.inputs.push(PathBuf::from("."));
    }

    Ok(cli)
}

fn print_help() {
    println!(
        r#"vitte-fmt — Vitte formatter

USAGE:
  vitte-fmt [FLAGS] [PATHS...]

FLAGS:
  -w, --write           Write formatted files in place
      --check           Check mode (non-zero if changes would be made)
      --stdin           Read from stdin (single document)
      --stdout          Write formatted output to stdout (single doc or per-file if one path)
  -q, --quiet           Reduce output
  -r, --recursive       Recurse into directories (default)
      --no-recursive    Do not recurse
      --lf              Force LF line endings
      --crlf            Force CRLF line endings
      --tabs N          Indent with tabs (display width N)
      --spaces N        Indent with spaces (default: 2)
      --max-width N     Preferred max line width (default: 100)
      --ext EXT         Add file extension to include (default: vitte, vit, vitl)
  -h, --help            Show help
  -V, --version         Show version

EXAMPLES:
  vitte-fmt --write src/
  vitte-fmt --check .
  cat file.vitte | vitte-fmt --stdin --stdout
"#
    );
}

// =============================================================================
// Runner
// =============================================================================

fn run() -> VResult<()> {
    let cli = parse_args()?;
    let t0 = Instant::now();

    if cli.stdin {
        let mut input = String::new();
        io::stdin().read_to_string(&mut input).map_err(VitteError::from)?;
        let formatted = format_vitte(&input, &cli);
        if cli.check && formatted != input {
            vitte_bail!(ErrCode::BadFormat, "formatting changes required");
        }
        if cli.stdout || !cli.write {
            io::stdout().write_all(formatted.as_bytes()).map_err(VitteError::from)?;
        }
        return Ok(());
    }

    let mut files = Vec::new();
    for p in &cli.inputs {
        collect_inputs(p, cli.recursive, &cli.ext, &mut files)?;
    }

    if files.is_empty() {
        if !cli.quiet {
            eprintln!("vitte-fmt: no input files");
        }
        return Ok(());
    }

    let mut changed = 0usize;
    let mut checked = 0usize;

    for path in files {
        let original = fs::read_to_string(&path).map_err(|e| {
            vitte_err!(ErrCode::Io, format!("read failed: {}: {}", path.display(), e))
        })?;

        let formatted = format_vitte(&original, &cli);
        checked += 1;

        if formatted != original {
            changed += 1;

            if cli.check {
                // keep going, but exit non-zero later
                if !cli.quiet {
                    eprintln!("needs format: {}", path.display());
                }
                continue;
            }

            if cli.write {
                fs::write(&path, formatted.as_bytes()).map_err(|e| {
                    vitte_err!(ErrCode::Io, format!("write failed: {}: {}", path.display(), e))
                })?;
                if !cli.quiet {
                    eprintln!("formatted: {}", path.display());
                }
            } else if cli.stdout && cli.inputs.len() == 1 && checked == 1 {
                io::stdout().write_all(formatted.as_bytes()).map_err(VitteError::from)?;
            }
        } else if !cli.quiet {
            // optional: silence to reduce noise
        }
    }

    if !cli.quiet {
        eprintln!(
            "checked {} file(s), changed {} file(s) in {:?}",
            checked,
            changed,
            t0.elapsed()
        );
    }

    if cli.check && changed > 0 {
        return Err(vitte_err!(ErrCode::BadFormat, "formatting changes required"));
    }

    Ok(())
}

// =============================================================================
// File collection
// =============================================================================

fn collect_inputs(path: &Path, recursive: bool, exts: &[String], out: &mut Vec<PathBuf>) -> VResult<()> {
    let md = fs::metadata(path).map_err(|e| vitte_err!(ErrCode::Io, format!("stat: {}: {}", path.display(), e)))?;
    if md.is_file() {
        if is_target_file(path, exts) {
            out.push(path.to_path_buf());
        }
        return Ok(());
    }

    if !md.is_dir() {
        return Ok(());
    }

    let mut stack = vec![path.to_path_buf()];
    while let Some(dir) = stack.pop() {
        for ent in fs::read_dir(&dir).map_err(|e| vitte_err!(ErrCode::Io, format!("readdir: {}: {}", dir.display(), e)))? {
            let ent = ent.map_err(|e| vitte_err!(ErrCode::Io, format!("readdir entry: {}", e)))?;
            let p = ent.path();
            let md = ent.metadata().map_err(|e| vitte_err!(ErrCode::Io, format!("stat: {}: {}", p.display(), e)))?;
            if md.is_dir() {
                if recursive {
                    stack.push(p);
                }
            } else if md.is_file() {
                if is_target_file(&p, exts) {
                    out.push(p);
                }
            }
        }
    }

    Ok(())
}

fn is_target_file(path: &Path, exts: &[String]) -> bool {
    let ext = path.extension().and_then(OsStr::to_str).unwrap_or("");
    exts.iter().any(|e| e.eq_ignore_ascii_case(ext))
}

// =============================================================================
// Formatter core (conservative, token-lite)
// =============================================================================

fn format_vitte(input: &str, cli: &Cli) -> String {
    let mut s = normalize_newlines(input);

    // Strip trailing whitespace
    s = s
        .lines()
        .map(|l| l.trim_end_matches(|c: char| c == ' ' || c == '\t'))
        .collect::<Vec<_>>()
        .join("\n");

    // Ensure final newline
    if !s.ends_with('\n') {
        s.push('\n');
    }

    // Indentation & `.end` blocks: very conservative heuristics
    s = reindent_blocks(&s, cli);

    // Apply preferred line endings
    match cli.line_ending {
        LineEnding::Auto => input_line_ending(input).apply(&s),
        LineEnding::Lf => s,
        LineEnding::CrLf => s.replace('\n', "\r\n"),
    }
}

fn normalize_newlines(input: &str) -> String {
    // Convert CRLF/CR -> LF
    let mut out = String::with_capacity(input.len());
    let mut it = input.chars().peekable();
    while let Some(c) = it.next() {
        if c == '\r' {
            if it.peek() == Some(&'\n') {
                it.next();
            }
            out.push('\n');
        } else {
            out.push(c);
        }
    }
    out
}

#[derive(Debug, Copy, Clone)]
enum DetectedEol {
    Lf,
    CrLf,
}

impl DetectedEol {
    fn apply(self, s: &str) -> String {
        match self {
            DetectedEol::Lf => s.to_string(),
            DetectedEol::CrLf => s.replace('\n', "\r\n"),
        }
    }
}

fn input_line_ending(input: &str) -> DetectedEol {
    // If we see a CRLF anywhere, preserve CRLF; else LF
    if input.as_bytes().windows(2).any(|w| w == b"\r\n") {
        DetectedEol::CrLf
    } else {
        DetectedEol::Lf
    }
}

fn indent_str(cli: &Cli, level: usize) -> String {
    match cli.indent {
        IndentStyle::Spaces(n) => " ".repeat((n as usize) * level),
        IndentStyle::Tabs(_) => "\t".repeat(level),
    }
}

fn is_comment_line(line: &str) -> bool {
    let t = line.trim_start();
    t.starts_with('#') || t.starts_with("//")
}

fn reindent_blocks(input: &str, cli: &Cli) -> String {
    // Heuristic:
    //   - Increase indent after lines that begin a block:
    //       mod/use/type/struct/union/enum/fn/scn/prog/if/elif/else/while/for/match/when/loop
    //     when they do NOT end with ".end"
    //   - Decrease indent when the trimmed line is exactly ".end"
    //
    // This does not parse expressions; it enforces a consistent block shape.
    let mut out = String::with_capacity(input.len());
    let mut level = 0usize;

    for raw in input.lines() {
        let trimmed = raw.trim();

        if trimmed.is_empty() {
            out.push('\n');
            continue;
        }

        // De-indent on .end
        if trimmed == ".end" {
            if level > 0 {
                level -= 1;
            }
            out.push_str(&indent_str(cli, level));
            out.push_str(".end\n");
            continue;
        }

        // Keep comment-only lines at current indent (but normalized)
        if is_comment_line(raw) {
            out.push_str(&indent_str(cli, level));
            out.push_str(trimmed);
            out.push('\n');
            continue;
        }

        // Normal line
        out.push_str(&indent_str(cli, level));
        out.push_str(trimmed);
        out.push('\n');

        // Decide if this line opens a block
        if opens_block(trimmed) {
            level += 1;
        }
    }

    out
}

fn opens_block(line: &str) -> bool {
    // Very conservative openers (line starts with keyword and is not a single-line block)
    // Also skip if the line already contains ".end" (one-liner style).
    if line.contains(".end") {
        return false;
    }

    let l = line.trim_start();

    // Keywords
    const KW: &[&str] = &[
        "mod ", "use ", "export ", "type ", "struct ", "union ", "enum ",
        "fn ", "scn ", "prog ",
        "if ", "elif ", "else",
        "while ", "for ", "match ",
        "when ", "loop ",
    ];

    // Special: "else" often appears alone
    if l == "else" {
        return true;
    }

    KW.iter().any(|k| l.starts_with(k))
}
