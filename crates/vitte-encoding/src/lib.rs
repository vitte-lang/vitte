//! vitte-encoding — Universal text encoding/decoding abstraction
//!
//! Provides a unified API for handling various encodings (UTF-8/16/Latin-1/etc.)
//! used by the Vitte language toolchain, including detection helpers for
//! vitte-encoding-detect.
//!
//! Platform: cross-platform (no_std optional)

#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(any(feature = "std", feature = "alloc-only"))]
extern crate alloc;

/// Supported encodings recognized by the Vitte runtime.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Encoding {
    Utf8,
    Utf16LE,
    Utf16BE,
    Latin1,
    Ascii,
    #[cfg(feature = "iso_8859")]
    Iso8859(u8),
    #[cfg(feature = "win125x")]
    Windows125(u16),
    #[cfg(feature = "koi8")]
    Koi8R,
    #[cfg(feature = "koi8")]
    Koi8U,
    #[cfg(feature = "mac")]
    MacRoman,
    #[cfg(feature = "eastasia")]
    GBK,
    #[cfg(feature = "eastasia")]
    GB18030,
    #[cfg(feature = "eastasia")]
    Big5,
    #[cfg(feature = "eastasia")]
    ShiftJIS,
    #[cfg(feature = "eastasia")]
    EUCJP,
    #[cfg(feature = "eastasia")]
    EUCKR,
}

impl Encoding {
    /// Returns the canonical label for this encoding.
    pub fn label(&self) -> &'static str {
        match self {
            Encoding::Utf8 => "utf-8",
            Encoding::Utf16LE => "utf-16le",
            Encoding::Utf16BE => "utf-16be",
            Encoding::Latin1 => "iso-8859-1",
            Encoding::Ascii => "us-ascii",
            #[cfg(feature = "iso_8859")]
            Encoding::Iso8859(n) => match n {
                2 => "iso-8859-2",
                3 => "iso-8859-3",
                4 => "iso-8859-4",
                5 => "iso-8859-5",
                6 => "iso-8859-6",
                7 => "iso-8859-7",
                8 => "iso-8859-8",
                9 => "iso-8859-9",
                10 => "iso-8859-10",
                11 => "iso-8859-11",
                13 => "iso-8859-13",
                14 => "iso-8859-14",
                15 => "iso-8859-15",
                16 => "iso-8859-16",
                _ => "iso-8859",
            },
            #[cfg(feature = "win125x")]
            Encoding::Windows125(cp) => match cp {
                1250 => "windows-1250",
                1251 => "windows-1251",
                1252 => "windows-1252",
                1253 => "windows-1253",
                1254 => "windows-1254",
                1255 => "windows-1255",
                1256 => "windows-1256",
                1257 => "windows-1257",
                1258 => "windows-1258",
                _ => "windows-125x",
            },
            #[cfg(feature = "koi8")]
            Encoding::Koi8R => "koi8-r",
            #[cfg(feature = "koi8")]
            Encoding::Koi8U => "koi8-u",
            #[cfg(feature = "mac")]
            Encoding::MacRoman => "macintosh",
            #[cfg(feature = "eastasia")]
            Encoding::GBK => "gbk",
            #[cfg(feature = "eastasia")]
            Encoding::GB18030 => "gb18030",
            #[cfg(feature = "eastasia")]
            Encoding::Big5 => "big5",
            #[cfg(feature = "eastasia")]
            Encoding::ShiftJIS => "shift_jis",
            #[cfg(feature = "eastasia")]
            Encoding::EUCJP => "euc-jp",
            #[cfg(feature = "eastasia")]
            Encoding::EUCKR => "euc-kr",
        }
    }

    /// Guess encoding from byte-order mark (BOM).
    pub fn from_bom(bom: &[u8]) -> Option<(Self, usize)> {
        if bom.starts_with(&[0xEF, 0xBB, 0xBF]) {
            Some((Encoding::Utf8, 3))
        } else if bom.starts_with(&[0xFF, 0xFE]) {
            Some((Encoding::Utf16LE, 2))
        } else if bom.starts_with(&[0xFE, 0xFF]) {
            Some((Encoding::Utf16BE, 2))
        } else {
            None
        }
    }

    /// Guess encoding by label (case-insensitive).
    pub fn from_label(label: &str) -> Option<Self> {
        match label.to_ascii_lowercase().as_str() {
            "utf-8" => Some(Encoding::Utf8),
            "utf-16le" => Some(Encoding::Utf16LE),
            "utf-16be" => Some(Encoding::Utf16BE),
            "iso-8859-1" => Some(Encoding::Latin1),
            "us-ascii" => Some(Encoding::Ascii),
            _ => None,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_labels() {
        assert_eq!(Encoding::Utf8.label(), "utf-8");
        assert_eq!(Encoding::Utf16BE.label(), "utf-16be");
    }

    #[test]
    fn test_from_bom() {
        assert_eq!(Encoding::from_bom(&[0xEF, 0xBB, 0xBF]), Some((Encoding::Utf8, 3)));
    }

    #[test]
    fn test_from_label() {
        assert_eq!(Encoding::from_label("utf-8"), Some(Encoding::Utf8));
        assert_eq!(Encoding::from_label("UTF-16LE"), Some(Encoding::Utf16LE));
        assert_eq!(Encoding::from_label("unknown"), None);
    }
}
