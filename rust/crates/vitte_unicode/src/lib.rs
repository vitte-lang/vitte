// vitte/rust/crates/vitte_unicode/src/lib.rs
//
// vitte_unicode — UTF-8 / Unicode helpers for the Vitte ecosystem
//
// Goals:
//   - Provide stable, small Unicode utilities used by compiler/tools/runtime
//   - Avoid leaking dependency choices (unicode-normalization, unicode-segmentation, etc.)
//   - Offer:
//       * UTF-8 validation (bytes)
//       * ASCII fastpaths
//       * Normalization (NFC/NFD/NFKC/NFKD)
//       * Simple case-folding helpers
//       * Grapheme/word boundaries (optional)
//
// Design:
//   - Keep core utilities available without heavy deps.
//   - Gate optional functionality behind Cargo features.
//
// Features (suggested):
//   default = ["std", "normalization", "segmentation"]
//   std
//   normalization -> unicode-normalization
//   segmentation  -> unicode-segmentation
//
// Notes:
//   - This crate is std-only by default for convenience.
//   - UTF-8 validator here operates on bytes and returns a bool.
//   - This crate complements the ASM utf8 stub; the ASM side can call into Rust if needed.

#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "alloc")]
extern crate alloc;

use core::fmt;

use vitte_common::err::{ErrCode, VResult, VitteError};

/// Crate version.
pub const VITTE_UNICODE_VERSION: &str = env!("CARGO_PKG_VERSION");

// =============================================================================
// UTF-8 validation (bytes)
// =============================================================================

/// Validate UTF-8 (strict) for a byte slice.
#[inline]
pub fn utf8_is_valid(bytes: &[u8]) -> bool {
    core::str::from_utf8(bytes).is_ok()
}

/// Validate UTF-8 and return a `&str` on success.
#[inline]
pub fn utf8_as_str(bytes: &[u8]) -> VResult<&str> {
    core::str::from_utf8(bytes).map_err(|_| VitteError::new(ErrCode::Utf8Invalid))
}

// =============================================================================
// ASCII helpers
// =============================================================================

/// Returns true if the string is ASCII only.
#[inline]
pub fn is_ascii_str(s: &str) -> bool {
    s.is_ascii()
}

/// Returns true if all bytes are ASCII.
#[inline]
pub fn is_ascii_bytes(b: &[u8]) -> bool {
    b.iter().all(|&x| x < 0x80)
}

/// Lowercase ASCII in-place.
#[inline]
pub fn ascii_lowercase_in_place(buf: &mut [u8]) {
    for b in buf {
        if (b'A'..=b'Z').contains(b) {
            *b = *b + 32;
        }
    }
}

/// Uppercase ASCII in-place.
#[inline]
pub fn ascii_uppercase_in_place(buf: &mut [u8]) {
    for b in buf {
        if (b'a'..=b'z').contains(b) {
            *b = *b - 32;
        }
    }
}

/// ASCII case-insensitive equality (fast).
#[inline]
pub fn ascii_eq_ignore_case(a: &[u8], b: &[u8]) -> bool {
    if a.len() != b.len() {
        return false;
    }
    for i in 0..a.len() {
        let mut x = a[i];
        let mut y = b[i];
        if (b'A'..=b'Z').contains(&x) {
            x += 32;
        }
        if (b'A'..=b'Z').contains(&y) {
            y += 32;
        }
        if x != y {
            return false;
        }
    }
    true
}

// =============================================================================
// Normalization (optional)
// =============================================================================

#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum NormalForm {
    Nfc,
    Nfd,
    Nfkc,
    Nfkd,
}

impl fmt::Display for NormalForm {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(match self {
            NormalForm::Nfc => "NFC",
            NormalForm::Nfd => "NFD",
            NormalForm::Nfkc => "NFKC",
            NormalForm::Nfkd => "NFKD",
        })
    }
}

#[cfg(feature = "alloc")]
fn to_string<I: Iterator<Item = char>>(it: I) -> alloc::string::String {
    let mut s = alloc::string::String::new();
    for c in it {
        s.push(c);
    }
    s
}

/// Normalize a string to the requested form (alloc required).
#[cfg(all(feature = "alloc", feature = "normalization"))]
pub fn normalize(s: &str, form: NormalForm) -> alloc::string::String {
    use unicode_normalization::UnicodeNormalization;
    match form {
        NormalForm::Nfc => to_string(s.nfc()),
        NormalForm::Nfd => to_string(s.nfd()),
        NormalForm::Nfkc => to_string(s.nfkc()),
        NormalForm::Nfkd => to_string(s.nfkd()),
    }
}

/// Normalize and return `Unsupported` if feature is missing.
#[cfg(not(all(feature = "alloc", feature = "normalization")))]
pub fn normalize(_s: &str, _form: NormalForm) -> VResult<()> {
    Err(VitteError::new(ErrCode::Unsupported))
}

/// Returns true if `s` is already normalized in the requested form.
#[cfg(feature = "normalization")]
pub fn is_normalized(s: &str, form: NormalForm) -> bool {
    use unicode_normalization::UnicodeNormalization;
    match form {
        NormalForm::Nfc => s.is_nfc(),
        NormalForm::Nfd => s.is_nfd(),
        NormalForm::Nfkc => s.is_nfkc(),
        NormalForm::Nfkd => s.is_nfkd(),
    }
}

#[cfg(not(feature = "normalization"))]
pub fn is_normalized(_s: &str, _form: NormalForm) -> bool {
    false
}

// =============================================================================
// Segmentation (optional)
// =============================================================================

/// Grapheme cluster iterator (requires `unicode-segmentation`).
#[cfg(feature = "segmentation")]
pub fn graphemes<'a>(s: &'a str) -> impl Iterator<Item = &'a str> + 'a {
    use unicode_segmentation::UnicodeSegmentation;
    s.graphemes(true)
}

/// Word boundary iterator (requires `unicode-segmentation`).
#[cfg(feature = "segmentation")]
pub fn words<'a>(s: &'a str) -> impl Iterator<Item = &'a str> + 'a {
    use unicode_segmentation::UnicodeSegmentation;
    s.unicode_words()
}

#[cfg(not(feature = "segmentation"))]
pub fn graphemes<'a>(_s: &'a str) -> core::iter::Empty<&'a str> {
    core::iter::empty()
}

#[cfg(not(feature = "segmentation"))]
pub fn words<'a>(_s: &'a str) -> core::iter::Empty<&'a str> {
    core::iter::empty()
}

// =============================================================================
// Simple case folding helpers (minimal)
// =============================================================================

/// Simple Unicode case-fold for comparisons.
/// This is not full ICU case folding; it uses `to_lowercase()` which is good enough for many cases.
#[cfg(feature = "alloc")]
pub fn fold_case(s: &str) -> alloc::string::String {
    let mut out = alloc::string::String::new();
    for c in s.chars() {
        for lc in c.to_lowercase() {
            out.push(lc);
        }
    }
    out
}

#[cfg(not(feature = "alloc"))]
pub fn fold_case(_s: &str) -> VResult<()> {
    Err(VitteError::new(ErrCode::Unsupported))
}

// =============================================================================
// Tests
// =============================================================================

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ascii_helpers() {
        assert!(is_ascii_str("abc"));
        assert!(!is_ascii_str("é"));
        assert!(ascii_eq_ignore_case(b"AbC", b"aBc"));
    }

    #[test]
    fn utf8_valid() {
        assert!(utf8_is_valid("hello".as_bytes()));
        assert!(!utf8_is_valid(&[0xFF, 0xFF]));
    }

    #[cfg(all(feature = "alloc", feature = "normalization"))]
    #[test]
    fn normalization_nfc() {
        // "e" + combining acute
        let s = "e\u{0301}";
        let n = normalize(s, NormalForm::Nfc);
        assert_eq!(n, "é");
        assert!(is_normalized(&n, NormalForm::Nfc));
    }

    #[cfg(feature = "segmentation")]
    #[test]
    fn segmentation() {
        let s = "a🇫🇷b";
        let g: Vec<&str> = graphemes(s).collect();
        assert_eq!(g.len(), 3);
    }
}
