//! vitte-encoding — text encoding helpers (stub implementation).
//!
//! The full implementation handles many encodings and automatic detection. This
//! stub keeps the public API so other crates can depend on it while only UTF-8
//! is actually processed. All other encodings return `Error::UnsupportedEncoding`.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::borrow::Cow;
use alloc::string::String;
use alloc::vec::Vec;

/// Result alias for the encoding helpers.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Errors that can occur when converting text.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Error {
    /// The requested encoding is not supported by the stub.
    UnsupportedEncoding,
    /// Input data was invalid for the requested encoding.
    InvalidData,
}

impl core::fmt::Display for Error {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            Error::UnsupportedEncoding => write!(f, "unsupported encoding"),
            Error::InvalidData => write!(f, "invalid data for encoding"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}

/// Enumerates encodings recognised by the API. In the stub, only `Utf8` is
/// implemented while the other variants exist for compatibility.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Encoding {
    Utf8,
    Utf16LE,
    Utf16BE,
    Utf32LE,
    Utf32BE,
    Latin1,
    Iso8859(u8),
    Windows125(u16),
    Koi8R,
    Koi8U,
    MacRoman,
    GBK,
    GB18030,
    Big5,
    ShiftJIS,
    EUCJP,
    EUCKR,
    #[cfg(feature = "encoding-rs")]
    Label(&'static str),
}

/// Result of an encoding detection attempt.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct DetectResult {
    /// Detected encoding.
    pub encoding: Encoding,
    /// Confidence level (0–100).
    pub confidence: u8,
    /// Whether a BOM was observed.
    pub had_bom: bool,
}

/// Attempts to decode bytes by first checking for a UTF-8 BOM, then falling
/// back to UTF-8 decoding.
pub fn decode_auto(bytes: &[u8]) -> Result<String> {
    // UTF-8 BOM check (0xEF,0xBB,0xBF)
    let bytes = if bytes.starts_with(&[0xEF, 0xBB, 0xBF]) {
        &bytes[3..]
    } else {
        bytes
    };
    decode(bytes, Encoding::Utf8)
}

/// Decodes bytes with the given encoding. Only UTF-8 succeeds in the stub.
pub fn decode(bytes: &[u8], enc: Encoding) -> Result<String> {
    match enc {
        Encoding::Utf8 => String::from_utf8(bytes.to_vec()).map_err(|_| Error::InvalidData),
        _ => Err(Error::UnsupportedEncoding),
    }
}

/// Decodes bytes with a lossy strategy (returns a `Cow`). Only UTF-8 is
/// supported; other encodings fall back to replacing invalid data.
pub fn decode_lossy<'a>(bytes: &'a [u8], enc: Encoding) -> Cow<'a, str> {
    match decode(bytes, enc) {
        Ok(s) => Cow::Owned(s),
        Err(_) => Cow::Owned(String::from_utf8_lossy(bytes).into_owned()),
    }
}

/// Encodes text into the target encoding. Only UTF-8 is implemented.
pub fn encode(text: &str, enc: Encoding) -> Result<Vec<u8>> {
    match enc {
        Encoding::Utf8 => Ok(text.as_bytes().to_vec()),
        _ => Err(Error::UnsupportedEncoding),
    }
}

/// Performs a simple decode→encode pipeline between encodings.
pub fn transcode(bytes: &[u8], from: Encoding, to: Encoding) -> Result<Vec<u8>> {
    let text = decode(bytes, from)?;
    encode(&text, to)
}

/// Heuristic detection placeholder (always returns UTF-8 with low confidence).
pub fn detect(bytes: &[u8]) -> Option<DetectResult> {
    Some(DetectResult { encoding: Encoding::Utf8, confidence: if bytes.is_empty() { 0 } else { 50 }, had_bom: bytes.starts_with(&[0xEF, 0xBB, 0xBF]) })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn decode_encode_utf8() {
        let text = "Hello";
        let bytes = encode(text, Encoding::Utf8).unwrap();
        assert_eq!(decode(&bytes, Encoding::Utf8).unwrap(), text);
    }
}
