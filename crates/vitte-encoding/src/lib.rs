//! vitte-encoding — encodages texte génériques + transcodage
//!
//! API principale :
//! - `decode(bytes, enc)`  → `String`
//! - `encode(text, enc)`   → `Vec<u8>`
//! - `transcode(bytes, from, to)`
//! - `decode_auto(bytes)`  → BOM > heuristique (si `detect`)
//!
//! Implémentations rapides :
//! - UTF-8/16/32 natifs (BOM aware)
//! - Nombreux encodages via `encoding_rs` (si feature `encoding-rs`)
//! - Détection heuristique via `chardetng` (si feature `detect`)
//!
//! Sans `encoding-rs`, seuls UTF-8/16/32 et ISO-8859-1 (latin1) sont garantis.
//! Le reste retournera `Error::UnsupportedEncoding`.

#![forbid(unsafe_code)]

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.");

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{string::String, vec::Vec, format};

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

#[cfg(feature = "serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature = "errors")]
use thiserror::Error;

#[cfg(feature = "encoding-rs")]
use encoding_rs::{Encoding as RsEncoding, UTF_8 as RS_UTF8};

#[cfg(feature = "detect")]
use chardetng::EncodingDetector;

/* ================================ TYPES ================================ */

/// Famille d’encodage.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Encoding {
    // UTF
    Utf8,
    Utf16LE,
    Utf16BE,
    Utf32LE,
    Utf32BE,
    // Latin/ISO
    Latin1,                 // ISO-8859-1
    Iso8859(u8),            // 2..=16
    // Windows single-byte
    Windows125(u16),        // 1250..=1258
    // KOI8
    Koi8R,
    Koi8U,
    // Mac
    MacRoman,
    // East Asia
    GBK,
    GB18030,
    Big5,
    ShiftJIS,
    EUCJP,
    EUCKR,
    // Fallback WHATWG label (si encoding_rs)
    #[cfg(feature = "encoding-rs")]
    Label(&'static str),
}

#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct DetectResult {
    pub encoding: Encoding,
    pub confidence: u8, // 0..100
    pub had_bom: bool,
}

#[cfg(feature = "errors")]
#[derive(Debug, Error, PartialEq, Eq)]
pub enum Error {
    #[error("unsupported encoding")]
    UnsupportedEncoding,
    #[error("invalid data for encoding")]
    InvalidData,
    #[error("encoding conversion failed")]
    EncodeFailed,
}
#[cfg(not(feature = "errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum Error { UnsupportedEncoding, InvalidData, EncodeFailed }

#[cfg(feature = "errors")]
pub type Result<T> = core::result::Result<T, Error>;
#[cfg(not(feature = "errors"))]
pub type Result<T> = core::result::Result<T, &'static str>;

/* ================================ API ================================= */

/// Détecte d’abord BOM, sinon heuristique (si `detect`), sinon `Utf8` par défaut.
/// Ne modifie pas `bytes`. Retourne `String`.
pub fn decode_auto(bytes: &[u8]) -> Result<String> {
    if let Some((enc, _bom_len)) = sniff_bom(bytes) {
        return decode(bytes, enc);
    }
    #[cfg(feature = "detect")]
    {
        if let Some(det) = detect(bytes) {
            return decode(bytes, det.encoding);
        }
    }
    // Fallback strict: tenter UTF-8 lossless
    decode(bytes, Encoding::Utf8)
}

/// Décode `bytes` selon `enc` en `String`. BOM honoré pour les UTF.
pub fn decode(bytes: &[u8], enc: Encoding) -> Result<String> {
    match enc {
        Encoding::Utf8 => decode_utf8(bytes),
        Encoding::Utf16LE => decode_utf16_le(bytes),
        Encoding::Utf16BE => decode_utf16_be(bytes),
        Encoding::Utf32LE => decode_utf32_le(bytes),
        Encoding::Utf32BE => decode_utf32_be(bytes),
        Encoding::Latin1 => decode_latin1(bytes),
        Encoding::Iso8859(n) => {
            if n == 1 { return decode_latin1(bytes); }
            #[cfg(feature = "encoding-rs")]
            { decode_rs(bytes, iso_label(n)) }
            #[cfg(not(feature = "encoding-rs"))]
            { err_unsup() }
        }
        Encoding::Windows125(cp) => {
            if cp == 1252 { return decode_win1252(bytes); }
            #[cfg(feature = "encoding-rs")]
            { decode_rs(bytes, win_label(cp)) }
            #[cfg(not(feature = "encoding-rs"))]
            { err_unsup() }
        }
        Encoding::Koi8R => rs_decode_label(bytes, "koi8-r"),
        Encoding::Koi8U => rs_decode_label(bytes, "koi8-u"),
        Encoding::MacRoman => rs_decode_label(bytes, "macintosh"),
        Encoding::GBK => rs_decode_label(bytes, "gbk"),
        Encoding::GB18030 => rs_decode_label(bytes, "gb18030"),
        Encoding::Big5 => rs_decode_label(bytes, "big5"),
        Encoding::ShiftJIS => rs_decode_label(bytes, "shift_jis"),
        Encoding::EUCJP => rs_decode_label(bytes, "euc-jp"),
        Encoding::EUCKR => rs_decode_label(bytes, "euc-kr"),
        #[cfg(feature = "encoding-rs")]
        Encoding::Label(lbl) => decode_rs(bytes, lbl),
    }
}

/// Encode `text` vers `enc`. N’ajoute pas de BOM par défaut.
/// Utiliser `encode_with_bom` pour préfixer.
pub fn encode(text: &str, enc: Encoding) -> Result<Vec<u8>> {
    encode_with_bom(text, enc, false)
}

/// Encode `text` vers `enc`. Ajoute BOM si `with_bom = true` et encodage le supporte.
pub fn encode_with_bom(text: &str, enc: Encoding, with_bom: bool) -> Result<Vec<u8>> {
    let mut out = match enc {
        Encoding::Utf8 => Ok(text.as_bytes().to_vec()),
        Encoding::Utf16LE => encode_utf16_le(text),
        Encoding::Utf16BE => encode_utf16_be(text),
        Encoding::Utf32LE => encode_utf32_le(text),
        Encoding::Utf32BE => encode_utf32_be(text),
        Encoding::Latin1 => encode_latin1(text),
        Encoding::Iso8859(n) => {
            if n == 1 { encode_latin1(text) }
            else {
                #[cfg(feature = "encoding-rs")]
                { encode_rs(text, iso_label(n)) }
                #[cfg(not(feature = "encoding-rs"))]
                { err_unsup() }
            }
        }
        Encoding::Windows125(cp) => {
            if cp == 1252 { encode_win1252(text) }
            else {
                #[cfg(feature = "encoding-rs")]
                { encode_rs(text, win_label(cp)) }
                #[cfg(not(feature = "encoding-rs"))]
                { err_unsup() }
            }
        }
        Encoding::Koi8R => rs_encode_label(text, "koi8-r"),
        Encoding::Koi8U => rs_encode_label(text, "koi8-u"),
        Encoding::MacRoman => rs_encode_label(text, "macintosh"),
        Encoding::GBK => rs_encode_label(text, "gbk"),
        Encoding::GB18030 => rs_encode_label(text, "gb18030"),
        Encoding::Big5 => rs_encode_label(text, "big5"),
        Encoding::ShiftJIS => rs_encode_label(text, "shift_jis"),
        Encoding::EUCJP => rs_encode_label(text, "euc-jp"),
        Encoding::EUCKR => rs_encode_label(text, "euc-kr"),
        #[cfg(feature = "encoding-rs")]
        Encoding::Label(lbl) => encode_rs(text, lbl),
    }?;

    if with_bom {
        match enc {
            Encoding::Utf8     => { prefix(&mut out, &[0xEF, 0xBB, 0xBF]); }
            Encoding::Utf16LE  => { prefix(&mut out, &[0xFF, 0xFE]); }
            Encoding::Utf16BE  => { prefix(&mut out, &[0xFE, 0xFF]); }
            Encoding::Utf32LE  => { prefix(&mut out, &[0xFF, 0xFE, 0x00, 0x00]); }
            Encoding::Utf32BE  => { prefix(&mut out, &[0x00, 0x00, 0xFE, 0xFF]); }
            _ => {}
        }
    }
    Ok(out)
}

/// Transcode `bytes` de `from` vers `to`.
pub fn transcode(bytes: &[u8], from: Encoding, to: Encoding) -> Result<Vec<u8>> {
    let s = decode(bytes, from)?;
    encode(&s, to)
}

/// Détection heuristique (`detect`) + BOM.
#[cfg(feature = "detect")]
pub fn detect(bytes: &[u8]) -> Option<DetectResult> {
    if let Some((enc, _bom)) = sniff_bom(bytes) {
        return Some(DetectResult { encoding: enc, confidence: 100, had_bom: true });
    }
    let mut det = EncodingDetector::new();
    det.feed(bytes, true);
    let enc_rs = det.guess(None, true);
    // Map vers Encoding
    let label = enc_rs.name();
    let enc = map_label(label)?;
    Some(DetectResult { encoding: enc, confidence: 80, had_bom: false })
}

/* =============================== UTF =============================== */

fn sniff_bom(bytes: &[u8]) -> Option<(Encoding, usize)> {
    if bytes.len() >= 3 && bytes[0..3] == [0xEF, 0xBB, 0xBF] {
        return Some((Encoding::Utf8, 3));
    }
    if bytes.len() >= 2 && bytes[0..2] == [0xFF, 0xFE] {
        // Could be UTF-16LE or UTF-32LE BOM prefix; check length
        if bytes.len() >= 4 && bytes[2..4] == [0x00, 0x00] { return Some((Encoding::Utf32LE, 4)); }
        return Some((Encoding::Utf16LE, 2));
    }
    if bytes.len() >= 2 && bytes[0..2] == [0xFE, 0xFF] {
        return Some((Encoding::Utf16BE, 2));
    }
    if bytes.len() >= 4 && bytes[0..4] == [0x00, 0x00, 0xFE, 0xFF] {
        return Some((Encoding::Utf32BE, 4));
    }
    None
}

fn strip_bom<'a>(bytes: &'a [u8], enc: &Encoding) -> &'a [u8] {
    match enc {
        Encoding::Utf8 if bytes.starts_with(&[0xEF,0xBB,0xBF]) => &bytes[3..],
        Encoding::Utf16LE if bytes.starts_with(&[0xFF,0xFE]) => &bytes[2..],
        Encoding::Utf16BE if bytes.starts_with(&[0xFE,0xFF]) => &bytes[2..],
        Encoding::Utf32LE if bytes.starts_with(&[0xFF,0xFE,0x00,0x00]) => &bytes[4..],
        Encoding::Utf32BE if bytes.starts_with(&[0x00,0x00,0xFE,0xFF]) => &bytes[4..],
        _ => bytes,
    }
}

fn decode_utf8(bytes: &[u8]) -> Result<String> {
    let b = strip_bom(bytes, &Encoding::Utf8);
    match core::str::from_utf8(b) {
        Ok(s) => Ok(s.to_string()),
        Err(_) => Err(err_invalid()),
    }
}

fn decode_utf16_le(bytes: &[u8]) -> Result<String> {
    let b = strip_bom(bytes, &Encoding::Utf16LE);
    if b.len() % 2 != 0 { return Err(err_invalid()); }
    let mut u16s = Vec::with_capacity(b.len()/2);
    for chunk in b.chunks_exact(2) {
        u16s.push(u16::from_le_bytes([chunk[0], chunk[1]]));
    }
    String::from_utf16(&u16s).map_err(|_| err_invalid())
}

fn decode_utf16_be(bytes: &[u8]) -> Result<String> {
    let b = strip_bom(bytes, &Encoding::Utf16BE);
    if b.len() % 2 != 0 { return Err(err_invalid()); }
    let mut u16s = Vec::with_capacity(b.len()/2);
    for chunk in b.chunks_exact(2) {
        u16s.push(u16::from_be_bytes([chunk[0], chunk[1]]));
    }
    String::from_utf16(&u16s).map_err(|_| err_invalid())
}

fn decode_utf32_le(bytes: &[u8]) -> Result<String> {
    let b = strip_bom(bytes, &Encoding::Utf32LE);
    if b.len() % 4 != 0 { return Err(err_invalid()); }
    let iter = b.chunks_exact(4).map(|c| u32::from_le_bytes([c[0],c[1],c[2],c[3]]));
    decode_utf32_codepoints(iter)
}

fn decode_utf32_be(bytes: &[u8]) -> Result<String> {
    let b = strip_bom(bytes, &Encoding::Utf32BE);
    if b.len() % 4 != 0 { return Err(err_invalid()); }
    let iter = b.chunks_exact(4).map(|c| u32::from_be_bytes([c[0],c[1],c[2],c[3]]));
    decode_utf32_codepoints(iter)
}

fn decode_utf32_codepoints<I: Iterator<Item=u32>>(iter: I) -> Result<String> {
    let mut s = String::new();
    for cp in iter {
        match core::char::from_u32(cp) {
            Some(ch) => s.push(ch),
            None => return Err(err_invalid()),
        }
    }
    Ok(s)
}

fn encode_utf16_le(text: &str) -> Result<Vec<u8>> {
    let mut out = Vec::with_capacity(text.len()*2);
    for u in text.encode_utf16() {
        out.extend_from_slice(&u.to_le_bytes());
    }
    Ok(out)
}

fn encode_utf16_be(text: &str) -> Result<Vec<u8>> {
    let mut out = Vec::with_capacity(text.len()*2);
    for u in text.encode_utf16() {
        out.extend_from_slice(&u.to_be_bytes());
    }
    Ok(out)
}

fn encode_utf32_le(text: &str) -> Result<Vec<u8>> {
    let mut out = Vec::with_capacity(text.len()*4);
    for ch in text.chars() {
        out.extend_from_slice(&(ch as u32).to_le_bytes());
    }
    Ok(out)
}

fn encode_utf32_be(text: &str) -> Result<Vec<u8>> {
    let mut out = Vec::with_capacity(text.len()*4);
    for ch in text.chars() {
        out.extend_from_slice(&(ch as u32).to_be_bytes());
    }
    Ok(out)
}

/* ============================ LATIN1 / 1252 ============================ */

fn decode_latin1(bytes: &[u8]) -> Result<String> {
    let s: String = bytes.iter().map(|&b| b as char).collect();
    Ok(s)
}

fn encode_latin1(text: &str) -> Result<Vec<u8>> {
    let mut out = Vec::with_capacity(text.len());
    for ch in text.chars() {
        if (ch as u32) <= 0xFF {
            out.push(ch as u8);
        } else {
            return Err(err_encode());
        }
    }
    Ok(out)
}

fn decode_win1252(bytes: &[u8]) -> Result<String> {
    // Subset: map direct except 0x80..0x9F with Windows-1252 specials via encoding_rs if present
    #[cfg(feature = "encoding-rs")]
    { rs_decode_label(bytes, "windows-1252") }
    #[cfg(not(feature = "encoding-rs"))]
    {
        // crude fallback: treat as latin1
        decode_latin1(bytes)
    }
}

fn encode_win1252(text: &str) -> Result<Vec<u8>> {
    #[cfg(feature = "encoding-rs")]
    { rs_encode_label(text, "windows-1252") }
    #[cfg(not(feature = "encoding-rs"))]
    { encode_latin1(text) }
}

/* =========================== encoding_rs bridge =========================== */

#[cfg(feature = "encoding-rs")]
fn decode_rs(bytes: &[u8], label: &str) -> Result<String> {
    match RsEncoding::for_label(label.as_bytes()) {
        Some(enc) => {
            let (cow, _, had_errors) = enc.decode(bytes);
            if had_errors { return Err(err_invalid()); }
            Ok(cow.into_owned())
        }
        None => Err(err_unsup()),
    }
}

#[cfg(feature = "encoding-rs")]
fn encode_rs(text: &str, label: &str) -> Result<Vec<u8>> {
    match RsEncoding::for_label(label.as_bytes()) {
        Some(enc) => {
            let (cow, _, had_errors) = enc.encode(text);
            if had_errors { return Err(err_encode()); }
            Ok(cow.into_owned())
        }
        None => Err(err_unsup()),
    }
}

#[cfg(feature = "encoding-rs")]
fn rs_decode_label(bytes: &[u8], label: &str) -> Result<String> { decode_rs(bytes, label) }
#[cfg(not(feature = "encoding-rs"))]
fn rs_decode_label(_: &[u8], _: &str) -> Result<String> { err_unsup() }

#[cfg(feature = "encoding-rs")]
fn rs_encode_label(text: &str, label: &str) -> Result<Vec<u8>> { encode_rs(text, label) }
#[cfg(not(feature = "encoding-rs"))]
fn rs_encode_label(_: &str, _: &str) -> Result<Vec<u8>> { err_unsup() }

#[cfg(feature = "encoding-rs")]
fn map_label(label: &str) -> Option<Encoding> {
    let l = label.to_ascii_lowercase();
    Some(match l.as_str() {
        "utf-8" => Encoding::Utf8,
        "utf-16le" => Encoding::Utf16LE,
        "utf-16be" => Encoding::Utf16BE,
        "utf-32le" => Encoding::Utf32LE,
        "utf-32be" => Encoding::Utf32BE,
        "iso-8859-1" | "latin1" => Encoding::Latin1,
        "windows-1252" => Encoding::Windows125(1252),
        other => Encoding::Label(Box::leak(other.into_boxed_str())),
    })
}

/* =============================== HELPERS =============================== */

fn prefix(buf: &mut Vec<u8>, pfx: &[u8]) {
    let mut out = Vec::with_capacity(pfx.len() + buf.len());
    out.extend_from_slice(pfx);
    out.extend_from_slice(buf);
    *buf = out;
}

#[cfg(feature = "errors")]
#[inline] fn err_unsup<T>() -> Result<T> { Err(Error::UnsupportedEncoding) }
#[cfg(not(feature = "errors"))]
#[inline] fn err_unsup<T>() -> Result<T> { Err("unsupported encoding") }

#[cfg(feature = "errors")]
#[inline] fn err_invalid<T>() -> Result<T> { Err(Error::InvalidData) }
#[cfg(not(feature = "errors"))]
#[inline] fn err_invalid<T>() -> Result<T> { Err("invalid data") }

#[cfg(feature = "errors")]
#[inline] fn err_encode<T>() -> Result<T> { Err(Error::EncodeFailed) }
#[cfg(not(feature = "errors"))]
#[inline] fn err_encode<T>() -> Result<T> { Err("encode failed") }

#[cfg(feature = "encoding-rs")]
fn iso_label(n: u8) -> &'static str {
    match n {
        1 => "iso-8859-1",
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
        _ => "iso-8859-1",
    }
}

#[cfg(feature = "encoding-rs")]
fn win_label(cp: u16) -> &'static str {
    match cp {
        1250 => "windows-1250",
        1251 => "windows-1251",
        1252 => "windows-1252",
        1253 => "windows-1253",
        1254 => "windows-1254",
        1255 => "windows-1255",
        1256 => "windows-1256",
        1257 => "windows-1257",
        1258 => "windows-1258",
        _ => "windows-1252",
    }
}

/* ================================= TESTS ================================ */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn utf8_roundtrip() {
        let s = "héllo € – 日本";
        let enc = encode(s, Encoding::Utf8).unwrap();
        let out = decode(&enc, Encoding::Utf8).unwrap();
        assert_eq!(s, out);
    }

    #[test]
    fn utf16_bom() {
        let s = "abcé";
        let enc = encode_with_bom(s, Encoding::Utf16LE, true).unwrap();
        let dec = decode(&enc, Encoding::Utf16LE).unwrap();
        assert_eq!(s, dec);
    }

    #[test]
    fn latin1_basic() {
        let s = "Cafe\u{00E9}"; // Café
        let enc = encode(s, Encoding::Latin1).unwrap();
        let dec = decode(&enc, Encoding::Latin1).unwrap();
        assert_eq!(dec, "Café");
    }

    #[cfg(feature = "encoding-rs")]
    #[test]
    fn shift_jis_decode() {
        // "日本" in Shift_JIS -> 0x93 0xFA 0x96 0x7B
        let bytes = [0x93, 0xFA, 0x96, 0x7B];
        let s = decode(&bytes, Encoding::ShiftJIS).unwrap();
        assert!(s.contains('日') && s.contains('本'));
    }

    #[cfg(feature = "detect")]
    #[test]
    fn detect_utf8() {
        let s = "hello";
        let r = detect(s.as_bytes()).unwrap();
        assert!(matches!(r.encoding, Encoding::Utf8));
    }

    #[test]
    fn transcode_utf8_to_utf16() {
        let s = "Zélé";
        let out = transcode(s.as_bytes(), Encoding::Utf8, Encoding::Utf16BE).unwrap();
        // Starts with BOM? not requested. Ensure length > 0.
        assert!(!out.is_empty());
    }
}