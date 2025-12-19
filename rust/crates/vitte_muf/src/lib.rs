// vitte/rust/crates/vitte_muf/src/lib.rs
//
// Minimal Muffin (.muf) manifest parser + pretty printer.
//
// The C project consumes this through `vitte_rust_api`. This crate focuses on:
//   - Deterministic output for normalization
//   - Strict validation to avoid "normalizing" invalid inputs silently
//
// Supported input syntaxes:
//   - Phase 0: line-oriented `key value` (legacy stubs in this repo)
//   - Phase 1: block syntax starting with `muffin <ver>` and terminated by `.end`

#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "alloc")]
extern crate alloc;

use vitte_common::err::{ErrCode, VResult, VitteError};

#[cfg(feature = "alloc")]
use alloc::string::String;

#[cfg(feature = "alloc")]
use alloc::vec::Vec;

#[cfg(feature = "alloc")]
#[derive(Clone, Debug)]
pub struct Manifest {
    lines: Vec<String>,
}

#[cfg(feature = "alloc")]
pub fn parse_manifest_str(input: &str) -> VResult<Manifest> {
    // Find first meaningful line to select parser mode.
    let first = input
        .lines()
        .map(|l| l.trim())
        .find(|l| !l.is_empty() && !l.starts_with('#') && !l.starts_with("//"));

    let Some(first) = first else {
        return Err(VitteError::new(ErrCode::ParseError).with_msg("empty manifest"));
    };

    if first.starts_with("muffin") {
        parse_phase1_block_manifest(input)
    } else {
        parse_phase0_kv_manifest(input)
    }
}

#[cfg(not(feature = "alloc"))]
pub fn parse_manifest_str(_: &str) -> VResult<()> {
    Err(VitteError::new(ErrCode::Unsupported))
}

// =============================================================================
// Phase 0: strict line-oriented `key value` (legacy)
// =============================================================================

#[cfg(feature = "alloc")]
fn parse_phase0_kv_manifest(input: &str) -> VResult<Manifest> {
    // This allow-list intentionally stays small: the goal is to catch typos and
    // avoid treating arbitrary text as a valid manifest.
    const ALLOWED_HEAD_KEYS: &[&str] = &[
        "kind",
        "name",
        "version",
        "src",
        "include",
        "tests",
        "deps",
        "dep",
        "module",
        "package",
        "workspace",
        "target",
        "profile",
        "toolchain",
        "scripts",
        "publish",
        "lock",
        "meta",
        "c",
        "cpp",
        "ld",
    ];

    fn is_ident(s: &str) -> bool {
        let mut chars = s.chars();
        let Some(c0) = chars.next() else { return false };
        if !(c0 == '_' || c0.is_ascii_alphabetic()) {
            return false;
        }
        chars.all(|c| c == '_' || c.is_ascii_alphanumeric())
    }

    fn is_key(s: &str) -> bool {
        let mut it = s.split('.');
        let Some(head) = it.next() else { return false };
        if !is_ident(head) {
            return false;
        }
        it.all(is_ident)
    }

    fn is_allowed(s: &str) -> bool {
        let head = s.split('.').next().unwrap_or("");
        ALLOWED_HEAD_KEYS.iter().any(|k| *k == head)
    }

    fn is_value(v: &str) -> bool {
        let v = v.trim();
        if v.is_empty() {
            return false;
        }
        // Accept either a quoted string (possibly containing spaces),
        // or a single bare token.
        if v.starts_with('"') {
            v.len() >= 2 && v.ends_with('"')
        } else {
            !v.chars().any(|c| c.is_whitespace())
        }
    }

    let mut out: Vec<String> = Vec::new();

    for (line_idx, raw) in input.lines().enumerate() {
        let line = raw.trim();
        if line.is_empty() || line.starts_with('#') || line.starts_with("//") {
            continue;
        }

        let mut parts = line.splitn(2, char::is_whitespace);
        let key = parts.next().unwrap_or("");
        let value = parts.next().unwrap_or("").trim();

        if !is_key(key) {
            return Err(VitteError::new(ErrCode::ParseError)
                .with_msg("invalid key syntax")
                .with_detail("line", (line_idx + 1).to_string())
                .with_detail("key", key.to_string()));
        }
        if !is_allowed(key) {
            return Err(VitteError::new(ErrCode::ParseError)
                .with_msg("unknown directive")
                .with_detail("line", (line_idx + 1).to_string())
                .with_detail("key", key.to_string()));
        }
        if !is_value(value) {
            return Err(VitteError::new(ErrCode::ParseError)
                .with_msg("invalid value syntax")
                .with_detail("line", (line_idx + 1).to_string())
                .with_detail("key", key.to_string()));
        }

        out.push(format!("{key} {value}"));
    }

    if out.is_empty() {
        return Err(VitteError::new(ErrCode::ParseError).with_msg("empty manifest"));
    }

    Ok(Manifest { lines: out })
}

// =============================================================================
// Phase 1: `muffin <ver>` header + `.end` blocks (early)
// =============================================================================

#[cfg(feature = "alloc")]
fn parse_phase1_block_manifest(input: &str) -> VResult<Manifest> {
    fn leading_spaces(s: &str) -> usize {
        s.as_bytes().iter().take_while(|b| **b == b' ').count()
    }

    fn strip_comment(s: &str) -> &str {
        // Conservative: only strips when comment marker appears after optional whitespace.
        let t = s.trim_start();
        if t.starts_with('#') || t.starts_with("//") {
            ""
        } else {
            s
        }
    }

    fn is_ident(s: &str) -> bool {
        let mut chars = s.chars();
        let Some(c0) = chars.next() else { return false };
        if !(c0 == '_' || c0.is_ascii_alphabetic()) {
            return false;
        }
        chars.all(|c| c == '_' || c.is_ascii_alphanumeric())
    }

    fn is_module_path(s: &str) -> bool {
        let mut it = s.split('/');
        let Some(head) = it.next() else { return false };
        if !is_ident(head) {
            return false;
        }
        it.all(is_ident)
    }

    fn is_header(line: &str) -> bool {
        let t = line.trim();
        if !t.starts_with("muffin") {
            return false;
        }
        let rest = t["muffin".len()..].trim_start();
        if rest.is_empty() {
            return false;
        }
        // Accept `muffin 1`, `muffin \"1\"`, or `muffin = \"1\"`.
        if rest.starts_with('=') {
            let rhs = rest[1..].trim();
            rhs.starts_with('"') && rhs.ends_with('"') && rhs.len() >= 2
        } else {
            (rest.chars().all(|c| c.is_ascii_digit() || c == '.'))
                || (rest.starts_with('"') && rest.ends_with('"') && rest.len() >= 2)
        }
    }

    fn is_kv_stmt(t: &str) -> bool {
        // key = value (we validate key-ish and non-empty rhs)
        let Some((lhs, rhs)) = t.split_once('=') else { return false };
        let key = lhs.trim();
        let rhs = rhs.trim();
        if key.is_empty() || rhs.is_empty() {
            return false;
        }
        let mut it = key.split('.');
        let Some(head) = it.next() else { return false };
        if !is_ident(head) || !it.all(is_ident) {
            return false;
        }
        true
    }

    fn is_block_start(t: &str) -> bool {
        // A minimal, strict allow-list for top-level and nested blocks.
        // (We don't attempt to fully enforce per-body grammar yet.)
        let mut it = t.split_whitespace();
        let Some(kw) = it.next() else { return false };
        matches!(
            kw,
            "workspace"
                | "package"
                | "module"
                | "target"
                | "profile"
                | "toolchain"
                | "deps"
                | "scripts"
                | "publish"
                | "lock"
                | "meta"
                | "overrides"
                | "features"
                | "members"
                | "paths"
                | "allow"
                | "deny"
        ) && match kw {
            // keyword must be followed by name/path for some blocks
            "workspace" | "module" => match it.next() {
                Some(v) => is_module_path(v),
                None => false,
            },
            "package" | "target" | "profile" | "toolchain" => match it.next() {
                Some(v) => is_ident(v),
                None => false,
            },
            _ => true,
        }
    }

    fn is_include(t: &str) -> bool {
        let t = t.trim();
        if !t.starts_with("include ") {
            return false;
        }
        let rest = t["include".len()..].trim();
        rest.starts_with('"') && rest.ends_with('"') && rest.len() >= 2
    }

    let mut out: Vec<String> = Vec::new();
    let mut stack: Vec<usize> = Vec::new(); // indentation of block starts
    let mut header_seen = false;

    for (line_idx, raw0) in input.lines().enumerate() {
        let raw0 = strip_comment(raw0);
        let trimmed = raw0.trim_end();
        let t = trimmed.trim();
        if t.is_empty() {
            continue;
        }

        let indent = leading_spaces(trimmed);
        if !header_seen {
            if indent != 0 {
                return Err(VitteError::new(ErrCode::ParseError)
                    .with_msg("header must start at column 1")
                    .with_detail("line", (line_idx + 1).to_string()));
            }
            if !is_header(t) {
                return Err(VitteError::new(ErrCode::ParseError)
                    .with_msg("expected header `muffin <ver>`")
                    .with_detail("line", (line_idx + 1).to_string()));
            }
            header_seen = true;
            out.push(t.to_string());
            continue;
        }

        // Indentation rules:
        // - `.end` must align with its block opener indentation.
        // - Any non-`.end` line inside a block must be more indented than its parent opener.
        if t == ".end" {
            let Some(block_indent) = stack.pop() else {
                return Err(VitteError::new(ErrCode::ParseError)
                    .with_msg("unexpected .end (no open block)")
                    .with_detail("line", (line_idx + 1).to_string()));
            };
            if indent != block_indent {
                return Err(VitteError::new(ErrCode::ParseError)
                    .with_msg(".end indentation mismatch")
                    .with_detail("line", (line_idx + 1).to_string()));
            }
            out.push(format!("{}{}", "  ".repeat(stack.len()), ".end"));
            continue;
        }

        if let Some(&parent_indent) = stack.last() {
            if indent <= parent_indent {
                return Err(VitteError::new(ErrCode::ParseError)
                    .with_msg("expected indented line inside block")
                    .with_detail("line", (line_idx + 1).to_string()));
            }
        } else if indent != 0 {
            return Err(VitteError::new(ErrCode::ParseError)
                .with_msg("unexpected indentation at top-level")
                .with_detail("line", (line_idx + 1).to_string()));
        }

        // Minimal syntax validation.
        if !(is_include(t) || is_kv_stmt(t) || is_block_start(t)) {
            return Err(VitteError::new(ErrCode::ParseError)
                .with_msg("unexpected directive")
                .with_detail("line", (line_idx + 1).to_string()));
        }

        // Normalize indentation to 2 spaces per nesting level.
        out.push(format!("{}{}", "  ".repeat(stack.len()), t));

        // Block openers push their indentation. We use the original indent as the
        // alignment requirement for `.end`, but output is normalized anyway.
        if is_block_start(t) {
            stack.push(indent);
        }
    }

    if !header_seen {
        return Err(VitteError::new(ErrCode::ParseError).with_msg("missing header"));
    }
    if !stack.is_empty() {
        return Err(VitteError::new(ErrCode::ParseError).with_msg("unterminated block(s)"));
    }

    Ok(Manifest { lines: out })
}

pub mod pretty {
    #[cfg(feature = "alloc")]
    use super::{Manifest, String};

    #[cfg(feature = "alloc")]
    pub fn manifest_to_string(m: &Manifest) -> String {
        let mut out = String::new();
        for (i, line) in m.lines.iter().enumerate() {
            if i != 0 {
                out.push('\n');
            }
            out.push_str(line);
        }
        out.push('\n');
        out
    }
}
