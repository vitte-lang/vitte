// vitte/rust/crates/vitte_regex/src/lib.rs
//
// vitte_regex — regex wrapper crate for the Vitte ecosystem
//
// Goals:
//   - Provide a stable regex API for tools/runtime without leaking the backend crate
//   - Allow swapping backend (regex crate, fancy-regex, oniguruma, hyperscan, etc.)
//   - Offer common helpers (find, find_all, replace, split) + flags
//   - Provide predictable error codes (FFI-friendly)
//
// Backend (phase 1):
//   - Use Rust `regex` crate (fast, DFA/NFA hybrid, no backtracking features)
//
// Features (suggested):
//   - default = ["std"]
//   - std
//   - unicode (enabled by default via regex crate)
//
// Notes:
//   - This module is std-only for now (regex crate requires std).
//   - Keep surface minimal and future-proof.
//

use std::fmt;

use regex::{Regex, RegexBuilder};

use vitte_common::err::{ErrCode, VResult, VitteError};

/// Library version.
pub const VITTE_REGEX_VERSION: &str = env!("CARGO_PKG_VERSION");

// =============================================================================
// Flags
// =============================================================================

bitflags::bitflags! {
    /// Regex compilation flags.
    ///
    /// Mapping:
    ///   - I: case-insensitive
    ///   - M: multi-line
    ///   - S: dot matches newline
    ///   - U: swap greediness (ungreedy)
    ///
    /// Note: Rust regex doesn't support all PCRE semantics; these map to what exists.
    #[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
    pub struct RegexFlags: u32 {
        const I = 1 << 0;
        const M = 1 << 1;
        const S = 1 << 2;
        const U = 1 << 3;
    }
}

// =============================================================================
// Types
// =============================================================================

/// A compiled regular expression.
///
/// This is a thin wrapper around the backend `regex::Regex`.
#[derive(Clone)]
pub struct VRegex {
    re: Regex,
    pattern: String,
    flags: RegexFlags,
}

impl fmt::Debug for VRegex {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("VRegex")
            .field("pattern", &self.pattern)
            .field("flags", &self.flags)
            .finish()
    }
}

/// A match result (byte offsets).
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub struct VMatch {
    pub start: usize,
    pub end: usize,
}

impl VMatch {
    #[inline]
    pub fn len(self) -> usize {
        self.end.saturating_sub(self.start)
    }

    #[inline]
    pub fn is_empty(self) -> bool {
        self.start >= self.end
    }
}

// =============================================================================
// Construction
// =============================================================================

impl VRegex {
    /// Compile a regex with default flags.
    pub fn new(pattern: &str) -> VResult<Self> {
        Self::with_flags(pattern, RegexFlags::empty())
    }

    /// Compile a regex with flags.
    pub fn with_flags(pattern: &str, flags: RegexFlags) -> VResult<Self> {
        let mut b = RegexBuilder::new(pattern);

        b.case_insensitive(flags.contains(RegexFlags::I));
        b.multi_line(flags.contains(RegexFlags::M));
        b.dot_matches_new_line(flags.contains(RegexFlags::S));
        b.swap_greed(flags.contains(RegexFlags::U));

        // Keep this deterministic: no size limits unless you expose them.
        // b.size_limit(...);

        let re = b.build().map_err(|e| {
            // Map backend error to stable code
            let mut ve = VitteError::new(ErrCode::BadFormat);
            #[cfg(feature = "alloc")]
            {
                ve = ve.with_msg(format!("regex compile error: {e}"));
            }
            ve
        })?;

        Ok(Self {
            re,
            pattern: pattern.to_string(),
            flags,
        })
    }

    #[inline]
    pub fn pattern(&self) -> &str {
        &self.pattern
    }

    #[inline]
    pub fn flags(&self) -> RegexFlags {
        self.flags
    }
}

// =============================================================================
// Queries
// =============================================================================

impl VRegex {
    /// Returns true if there is a match anywhere in `haystack`.
    #[inline]
    pub fn is_match(&self, haystack: &str) -> bool {
        self.re.is_match(haystack)
    }

    /// Find first match.
    pub fn find(&self, haystack: &str) -> Option<VMatch> {
        self.re.find(haystack).map(|m| VMatch {
            start: m.start(),
            end: m.end(),
        })
    }

    /// Find all matches (non-overlapping).
    pub fn find_all(&self, haystack: &str) -> Vec<VMatch> {
        self.re
            .find_iter(haystack)
            .map(|m| VMatch {
                start: m.start(),
                end: m.end(),
            })
            .collect()
    }

    /// Count matches (non-overlapping).
    pub fn count(&self, haystack: &str) -> usize {
        self.re.find_iter(haystack).count()
    }

    /// Returns capture groups for first match, as byte ranges.
    ///
    /// The 0th entry is the whole match.
    pub fn captures(&self, haystack: &str) -> Option<Vec<Option<VMatch>>> {
        let caps = self.re.captures(haystack)?;
        let mut out = Vec::with_capacity(caps.len());
        for i in 0..caps.len() {
            if let Some(m) = caps.get(i) {
                out.push(Some(VMatch {
                    start: m.start(),
                    end: m.end(),
                }));
            } else {
                out.push(None);
            }
        }
        Some(out)
    }

    /// Returns named captures for first match.
    pub fn captures_named(&self, haystack: &str) -> Option<Vec<(String, VMatch)>> {
        let caps = self.re.captures(haystack)?;
        let mut out = Vec::new();
        for name in self.re.capture_names().flatten() {
            if let Some(m) = caps.name(name) {
                out.push((
                    name.to_string(),
                    VMatch {
                        start: m.start(),
                        end: m.end(),
                    },
                ));
            }
        }
        Some(out)
    }
}

// =============================================================================
// Transformations
// =============================================================================

impl VRegex {
    /// Replace first match with `replacement`.
    pub fn replace(&self, haystack: &str, replacement: &str) -> String {
        self.re.replace(haystack, replacement).into_owned()
    }

    /// Replace all matches with `replacement`.
    pub fn replace_all(&self, haystack: &str, replacement: &str) -> String {
        self.re.replace_all(haystack, replacement).into_owned()
    }

    /// Split string by matches.
    pub fn split<'a>(&'a self, haystack: &'a str) -> impl Iterator<Item = &'a str> + 'a {
        self.re.split(haystack)
    }

    /// Split, returning at most `limit` pieces (like many stdlibs).
    pub fn splitn<'a>(&'a self, haystack: &'a str, limit: usize) -> Vec<&'a str> {
        self.re.splitn(haystack, limit).collect()
    }
}

// =============================================================================
// Utilities
// =============================================================================

/// Escape a literal string so it can be inserted into a regex safely.
#[inline]
pub fn escape_literal(s: &str) -> String {
    regex::escape(s)
}

/// Validate a regex pattern without keeping the compiled regex (cheap API).
pub fn validate(pattern: &str, flags: RegexFlags) -> VResult<()> {
    let _ = VRegex::with_flags(pattern, flags)?;
    Ok(())
}

// =============================================================================
// Error mapping helpers (optional)
// =============================================================================

/// Map a regex backend error into a stable Vitte error.
///
/// Kept for future multi-backend support.
pub fn map_backend_error(msg: &str) -> VitteError {
    #[cfg(feature = "alloc")]
    {
        VitteError::new(ErrCode::BadFormat).with_msg(msg)
    }
    #[cfg(not(feature = "alloc"))]
    {
        let _ = msg;
        VitteError::new(ErrCode::BadFormat)
    }
}

// =============================================================================
// Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn compile_and_find() {
        let r = VRegex::new(r"ab+c").unwrap();
        assert!(r.is_match("xxabbbcxx"));
        let m = r.find("xxabbbcxx").unwrap();
        assert_eq!(&"xxabbbcxx"[m.start..m.end], "abbbc");
    }

    #[test]
    fn captures() {
        let r = VRegex::new(r"(\d+)-(\w+)").unwrap();
        let caps = r.captures("12-abc").unwrap();
        assert_eq!(caps.len(), 3);
    }

    #[test]
    fn replace_all() {
        let r = VRegex::new(r"\s+").unwrap();
        let s = r.replace_all("a   b\tc", " ");
        assert_eq!(s, "a b c");
    }

    #[test]
    fn escape() {
        let lit = escape_literal("a+b*c?");
        let r = VRegex::new(&lit).unwrap();
        assert!(r.is_match("xxa+b*c?yy"));
    }
}
