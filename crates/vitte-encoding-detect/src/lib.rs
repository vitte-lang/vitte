//! vitte-encoding-detect — détection d’encodage (BOM + heuristique)
//!
//! Objectif: choisir un encodage probable pour un flux d’octets.
//!
//! Fourni:
//! - `sniff_bom(bytes)` → détecte UTF-8/16/32 via BOM
//! - `detect(bytes)` → BOM d’abord, sinon heuristique (feature `chardetng`), sinon `Utf8` faible
//! - `detect_with_fallback(bytes, fallback)` → idem avec repli contrôlé
//! - `best_label(bytes)` → label WHATWG (si `vitte-encoding` + `encoding-rs` activés côté dépendant)
//!
//! Renvoie `DetectResult { encoding, confidence, had_bom }`.
//!
//! Notes:
//! - Sans feature `chardetng`, la détection hors BOM est limitée: on renvoie Utf8 avec faible confiance.
//! - L’heuristique `chardetng` suit WHATWG et retourne un label convertible par `vitte-encoding`.

#![forbid(unsafe_code)]

#[cfg(all(not(feature = "std"), not(feature = "alloc-only")))]
compile_error!("Enable feature `std` (default) or `alloc-only`.") ;

#[cfg(feature = "alloc-only")]
extern crate alloc;

#[cfg(feature = "alloc-only")]
use alloc::{string::String, vec::Vec};

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

#[cfg(feature = "serde")]
use serde::{Serialize, Deserialize};

#[cfg(feature = "errors")]
use thiserror::Error;

use vitte_encoding::Encoding;

/* =============================== TYPES =============================== */

/// Résultat de détection.
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct DetectResult {
    pub encoding: Encoding,
    pub confidence: u8, // 0..100
    pub had_bom: bool,
}

#[cfg(feature = "errors")]
#[derive(Debug, Error, PartialEq, Eq)]
pub enum DetectError {
    #[error("empty input")]
    EmptyInput,
}
#[cfg(not(feature = "errors"))]
#[derive(Debug, PartialEq, Eq)]
pub enum DetectError { EmptyInput }

#[cfg(feature = "errors")]
pub type Result<T> = core::result::Result<T, DetectError>;
#[cfg(not(feature = "errors"))]
pub type Result<T> = core::result::Result<T, &'static str>;

/* ============================== PUBLIC API ============================== */

/// Détection avec priorité au BOM, puis heuristique (`chardetng` si activée),
/// sinon repli Utf8 avec faible confiance.
/// Ne modifie pas `bytes`.
pub fn detect(bytes: &[u8]) -> Option<DetectResult> {
    if bytes.is_empty() {
        // Détecter "quelque chose" est arbitraire. On retourne None.
        return None;
    }
    if let Some((enc, _bom_len)) = sniff_bom(bytes) {
        return Some(DetectResult { encoding: enc, confidence: 100, had_bom: true });
    }
    #[cfg(feature = "chardetng")]
    {
        if let Some(d) = detect_chardetng(bytes) {
            return Some(d);
        }
    }
    // Repli pauvre: supposer UTF-8 (fréquent). Confiance faible.
    Some(DetectResult { encoding: Encoding::Utf8, confidence: 10, had_bom: false })
}

/// Comme `detect`, mais si None est renvoyé (flux vide), retourne `fallback`.
pub fn detect_with_fallback(bytes: &[u8], fallback: Encoding) -> DetectResult {
    detect(bytes).unwrap_or(DetectResult { encoding: fallback, confidence: 0, had_bom: false })
}

/// Détection du BOM seul (UTF-8/16/32). Retourne l’encodage et la taille du BOM.
pub fn sniff_bom(bytes: &[u8]) -> Option<(Encoding, usize)> {
    if bytes.len() >= 3 && bytes[0..3] == [0xEF, 0xBB, 0xBF] {
        return Some((Encoding::Utf8, 3));
    }
    if bytes.len() >= 4 && bytes[0..4] == [0xFF, 0xFE, 0x00, 0x00] {
        return Some((Encoding::Utf32LE, 4));
    }
    if bytes.len() >= 4 && bytes[0..4] == [0x00, 0x00, 0xFE, 0xFF] {
        return Some((Encoding::Utf32BE, 4));
    }
    if bytes.len() >= 2 && bytes[0..2] == [0xFF, 0xFE] {
        return Some((Encoding::Utf16LE, 2));
    }
    if bytes.len() >= 2 && bytes[0..2] == [0xFE, 0xFF] {
        return Some((Encoding::Utf16BE, 2));
    }
    None
}

/// Label WHATWG approximatif pour l’encodage détecté.
/// Ici on se limite à quelques cas communs et noms stables.
/// Utile quand un shell ou un fichier sidecar veut un label texte.
pub fn best_label(dr: &DetectResult) -> &'static str {
    match dr.encoding {
        Encoding::Utf8 => "utf-8",
        Encoding::Utf16LE => "utf-16le",
        Encoding::Utf16BE => "utf-16be",
        Encoding::Utf32LE => "utf-32le",
        Encoding::Utf32BE => "utf-32be",
        Encoding::Latin1 => "iso-8859-1",
        Encoding::Iso8859(n) => iso_label(n),
        Encoding::Windows125(cp) => win_label(cp),
        Encoding::Koi8R => "koi8-r",
        Encoding::Koi8U => "koi8-u",
        Encoding::MacRoman => "macintosh",
        Encoding::GBK => "gbk",
        Encoding::GB18030 => "gb18030",
        Encoding::Big5 => "big5",
        Encoding::ShiftJIS => "shift_jis",
        Encoding::EUCJP => "euc-jp",
        Encoding::EUCKR => "euc-kr",
        #[cfg(feature = "encoding-rs")]
        Encoding::Label(l) => l,
    }
}

/* ============================ IMPL HEURISTIQUE ============================ */

#[cfg(feature = "chardetng")]
fn detect_chardetng(bytes: &[u8]) -> Option<DetectResult> {
    use chardetng::EncodingDetector;
    let mut det = EncodingDetector::new();
    det.feed(bytes, true);
    let rs = det.guess(None, true);
    let label = rs.name().to_ascii_lowercase();

    // Mapping WHATWG -> Encoding (sous-ensemble courant de vitte-encoding)
    let enc = match label.as_str() {
        "utf-8"      => Encoding::Utf8,
        "utf-16le"   => Encoding::Utf16LE,
        "utf-16be"   => Encoding::Utf16BE,
        "utf-32le"   => Encoding::Utf32LE,
        "utf-32be"   => Encoding::Utf32BE,
        "iso-8859-1" | "latin1" => Encoding::Latin1,
        "iso-8859-2" => Encoding::Iso8859(2),
        "iso-8859-3" => Encoding::Iso8859(3),
        "iso-8859-4" => Encoding::Iso8859(4),
        "iso-8859-5" => Encoding::Iso8859(5),
        "iso-8859-6" => Encoding::Iso8859(6),
        "iso-8859-7" => Encoding::Iso8859(7),
        "iso-8859-8" => Encoding::Iso8859(8),
        "iso-8859-9" => Encoding::Iso8859(9),
        "iso-8859-10"=> Encoding::Iso8859(10),
        "iso-8859-11"=> Encoding::Iso8859(11),
        "iso-8859-13"=> Encoding::Iso8859(13),
        "iso-8859-14"=> Encoding::Iso8859(14),
        "iso-8859-15"=> Encoding::Iso8859(15),
        "iso-8859-16"=> Encoding::Iso8859(16),
        "windows-1250" => Encoding::Windows125(1250),
        "windows-1251" => Encoding::Windows125(1251),
        "windows-1252" => Encoding::Windows125(1252),
        "windows-1253" => Encoding::Windows125(1253),
        "windows-1254" => Encoding::Windows125(1254),
        "windows-1255" => Encoding::Windows125(1255),
        "windows-1256" => Encoding::Windows125(1256),
        "windows-1257" => Encoding::Windows125(1257),
        "windows-1258" => Encoding::Windows125(1258),
        "koi8-r" => Encoding::Koi8R,
        "koi8-u" => Encoding::Koi8U,
        "macintosh" => Encoding::MacRoman,
        "gbk" => Encoding::GBK,
        "gb18030" => Encoding::GB18030,
        "big5" => Encoding::Big5,
        "shift_jis" => Encoding::ShiftJIS,
        "euc-jp" => Encoding::EUCJP,
        "euc-kr" => Encoding::EUCKR,
        other => {
            // Dernier recours: exposer le label brut si la dépendance `encoding-rs`
            // est active dans vitte-encoding (Encoding::Label), sinon Utf8 faible.
            #[cfg(feature = "encoding-rs")]
            { Encoding::Label(Box::leak(other.to_string().into_boxed_str())) }
            #[cfg(not(feature = "encoding-rs"))]
            { Encoding::Utf8 }
        }
    };

    // chardetng n’expose pas un pourcentage standardisé; on donne 80 pour heuristique.
    Some(DetectResult { encoding: enc, confidence: 80, had_bom: false })
}

/* ================================ HELPERS ================================ */

fn iso_label(n: u8) -> &'static str {
    match n {
        1  => "iso-8859-1",
        2  => "iso-8859-2",
        3  => "iso-8859-3",
        4  => "iso-8859-4",
        5  => "iso-8859-5",
        6  => "iso-8859-6",
        7  => "iso-8859-7",
        8  => "iso-8859-8",
        9  => "iso-8859-9",
        10 => "iso-8859-10",
        11 => "iso-8859-11",
        13 => "iso-8859-13",
        14 => "iso-8859-14",
        15 => "iso-8859-15",
        16 => "iso-8859-16",
        _  => "iso-8859-1",
    }
}

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

/* ================================= TESTS ================================= */

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_encoding::Encoding;

    #[test]
    fn bom_utf8() {
        let data = [0xEF,0xBB,0xBF, b'a', b'b'];
        let d = detect(&data).unwrap();
        assert_eq!(d.encoding, Encoding::Utf8);
        assert!(d.had_bom);
        assert_eq!(d.confidence, 100);
    }

    #[test]
    fn no_bom_defaults_utf8() {
        let data = b"hello";
        let d = detect(data).unwrap();
        assert_eq!(d.encoding, Encoding::Utf8);
        assert!(!d.had_bom);
        assert!(d.confidence >= 10);
    }

    #[test]
    fn empty_returns_none() {
        assert!(detect(&[]).is_none());
    }

    #[cfg(feature = "chardetng")]
    #[test]
    fn heuristics_runs() {
        // bytes Shift_JIS for "日本" = 0x93 0xFA 0x96 0x7B
        let data = [0x93, 0xFA, 0x96, 0x7B];
        let d = detect(&data).unwrap();
        // Selon heuristique, Shift_JIS ou proche.
        // On vérifie juste que ce n’est pas Utf8 forcé.
        assert!(d.encoding != Encoding::Utf8 || d.had_bom); // si Utf8 alors BOM
    }
}