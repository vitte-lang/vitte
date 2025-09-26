//! vitte-core — primitives partagées (no_std-ready)
//!
//! Fournit :
//! - `SourceId`, `Pos`, `Span`, `Spanned<T>`
//! - `Ident` + helper `ident()`
//! - Constances VITBC (`MAGIC_VITBC`, `VITBC_VERSION`) + `SectionTag` (fourcc)
//! - IO mémoire (little-endian) : `ByteWriter`, `ByteReader`
//! - `crc32_ieee` (compact, sans table)
//! - Erreurs `CoreError` + alias `CoreResult<T>`
//!
//! Features :
//! - `std` (par défaut) : impl `std::error::Error` & tests
//! - `serde` : derive (dé)sérialisation sur les structures utiles

#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

/* ─────────────────────────── Imports ─────────────────────────── */

use core::fmt;

#[cfg(feature = "std")]
use std::{borrow::Cow, string::String, vec::Vec};

#[cfg(not(feature = "std"))]
use alloc::{borrow::Cow, string::String, vec::Vec};

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/* ─────────────────────────── Modules publics ─────────────────────────── */

/// Primitives de bytecode (chunk, assembleur, désassembleur, helpers).
pub mod bytecode;

/// Compatibilité : ré-exporte les helpers de validation historiques.
pub use bytecode::helpers as helpers;
/// Compatibilité : ré-exporte le désassembleur textuel.
pub use bytecode::disasm as disasm;
/// Compatibilité : ré-exporte l'assembleur minimal.
pub use bytecode::asm as asm;
/// Compatibilité : ré-exporte le runtime d'évaluation simplifié.
pub use bytecode::runtime as runtime;

/* ─────────────────────────── Résultat commun ─────────────────────────── */

/// Alias résultat commun au core.
pub type CoreResult<T> = core::result::Result<T, CoreError>;

/* ─────────────────────────── Spans / Positions ─────────────────────────── */

/// Identifiant de source (fichier, buffer, etc.).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct SourceId(pub u32);

/// Position (offset byte) depuis le début de la source.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Pos(pub u32);

impl Pos {
    /// Position nulle.
    pub const ZERO: Self = Pos(0);
    /// Addition saturée.
    pub fn saturating_add(self, v: u32) -> Self { Pos(self.0.saturating_add(v)) }
}

/// Plage (demi-ouverte) `[start, end)` dans une source.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Span {
    /// Source d’où provient l’item.
    pub source: SourceId,
    /// Début inclus.
    pub start: Pos,
    /// Fin exclue.
    pub end: Pos,
}

impl Span {
    /// Crée un span.
    pub const fn new(source: SourceId, start: Pos, end: Pos) -> Self { Self { source, start, end } }
    /// Longueur en bytes.
    pub fn len(&self) -> u32 { self.end.0.saturating_sub(self.start.0) }
    /// Vrai si le span est vide.
    pub fn is_empty(&self) -> bool { self.start.0 >= self.end.0 }
}

/// Wrapper utilitaire « valeur + span ».
#[derive(Debug, Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Spanned<T> {
    /// La valeur.
    pub value: T,
    /// La localisation.
    pub span: Span,
}

impl<T> Spanned<T> {
    /// Construit un `Spanned<T>`.
    pub fn new(value: T, span: Span) -> Self { Self { value, span } }
    /// Applique une fonction à la valeur et conserve le span.
    pub fn map<U>(self, f: impl FnOnce(T) -> U) -> Spanned<U> { Spanned { value: f(self.value), span: self.span } }
}

/* ─────────────────────────── Ident / Symbolique légère ─────────────────────────── */

/// Identifiant simple (peut être interné plus tard).
pub type Ident = String;

/// Construit un identifiant.
pub fn ident<S: Into<String>>(s: S) -> Ident { s.into() }

/* ─────────────────────────── VITBC — Constances & Tags ─────────────────────────── */

/// Magic d’un fichier VITBC : `b"VITBC\0"`.
pub const MAGIC_VITBC: &[u8; 6] = b"VITBC\0";

/// Version actuelle de VITBC pour Vitte.
pub const VITBC_VERSION: u16 = 2;

/// Tags de section (fourcc) — exactement 4 octets.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u32)]
pub enum SectionTag {
    /// INTS : i64 array (LE)
    INTS = u32::from_be_bytes(*b"INTS"),
    /// FLTS : f64 array (LE)
    FLTS = u32::from_be_bytes(*b"FLTS"),
    /// STRS : [len:u32][bytes]*
    STRS = u32::from_be_bytes(*b"STRS"),
    /// DATA : blob libre
    DATA = u32::from_be_bytes(*b"DATA"),
    /// CODE : bytecode (optionnellement compressé)
    CODE = u32::from_be_bytes(*b"CODE"),
    /// NAME : symbol names (debug)
    NAME = u32::from_be_bytes(*b"NAME"),
    /// CRCC : CRC32 trailer (u32 LE)
    CRCC = u32::from_be_bytes(*b"CRCC"),
}

impl SectionTag {
    /// Renvoie le fourcc sous forme de 4 octets big-endian.
    pub const fn to_be_bytes(self) -> [u8; 4] { (self as u32).to_be_bytes() }
    /// Lit un tag depuis 4 octets big-endian.
    pub const fn from_be_bytes(b: [u8; 4]) -> Option<Self> {
        match u32::from_be_bytes(b) {
            x if x == SectionTag::INTS as u32 => Some(SectionTag::INTS),
            x if x == SectionTag::FLTS as u32 => Some(SectionTag::FLTS),
            x if x == SectionTag::STRS as u32 => Some(SectionTag::STRS),
            x if x == SectionTag::DATA as u32 => Some(SectionTag::DATA),
            x if x == SectionTag::CODE as u32 => Some(SectionTag::CODE),
            x if x == SectionTag::NAME as u32 => Some(SectionTag::NAME),
            x if x == SectionTag::CRCC as u32 => Some(SectionTag::CRCC),
            _ => None,
        }
    }
}

/* ─────────────────────────── CRC32 IEEE ─────────────────────────── */

/// CRC32 (IEEE 802.3) — implémentation compacte sans table.
pub fn crc32_ieee(data: &[u8]) -> u32 {
    let mut crc: u32 = 0xFFFF_FFFF;
    for &b in data {
        let mut x = (crc ^ (b as u32)) & 0xFF;
        // 8 itérations (bitwise) — polynôme 0xEDB88320
        for _ in 0..8 {
            let mask = (x & 1).wrapping_neg() & 0xEDB8_8320;
            x = (x >> 1) ^ mask;
        }
        crc = (crc >> 8) ^ x;
    }
    !crc
}

/* ─────────────────────────── Byte Writer (LE) ─────────────────────────── */

/// Buffer d’écriture (croît automatiquement).
#[derive(Debug, Default, Clone)]
pub struct ByteWriter {
    buf: Vec<u8>,
}

impl ByteWriter {
    /// Crée un writer vide.
    pub fn new() -> Self { Self { buf: Vec::new() } }
    /// Accès en lecture au contenu.
    pub fn as_slice(&self) -> &[u8] { &self.buf }
    /// Récupère le buffer (consomme).
    pub fn into_vec(self) -> Vec<u8> { self.buf }
    /// Ajoute des octets bruts.
    pub fn write_bytes(&mut self, bytes: &[u8]) { self.buf.extend_from_slice(bytes); }
    /// Écrit un tag (fourcc big-endian).
    pub fn write_tag(&mut self, tag: SectionTag) { self.write_bytes(&tag.to_be_bytes()); }
    /// Écrit un u16 little-endian.
    pub fn write_u16_le(&mut self, v: u16) { self.buf.extend_from_slice(&v.to_le_bytes()); }
    /// Écrit un u32 little-endian.
    pub fn write_u32_le(&mut self, v: u32) { self.buf.extend_from_slice(&v.to_le_bytes()); }
    /// Écrit un u64 little-endian.
    pub fn write_u64_le(&mut self, v: u64) { self.buf.extend_from_slice(&v.to_le_bytes()); }
    /// Écrit un i64 little-endian.
    pub fn write_i64_le(&mut self, v: i64) { self.buf.extend_from_slice(&v.to_le_bytes()); }
    /// Écrit un f64 little-endian.
    pub fn write_f64_le(&mut self, v: f64) { self.buf.extend_from_slice(&v.to_le_bytes()); }
}

/* ─────────────────────────── Byte Reader (LE) ─────────────────────────── */

/// Lecteur séquentiel sur un slice d’octets (helpers LE).
#[derive(Debug, Clone)]
pub struct ByteReader<'a> {
    data: &'a [u8],
    off: usize,
}

impl<'a> ByteReader<'a> {
    /// Construit un lecteur.
    pub fn new(data: &'a [u8]) -> Self { Self { data, off: 0 } }
    /// Offset courant.
    pub fn offset(&self) -> usize { self.off }
    /// Taille restante.
    pub fn remaining(&self) -> usize { self.data.len().saturating_sub(self.off) }

    /// Lit `n` octets (ou erreur si EOF).
    pub fn read_bytes(&mut self, n: usize) -> CoreResult<&'a [u8]> {
        if self.remaining() < n {
            return Err(CoreError::UnexpectedEof { needed: n as u64, at: self.off as u64 });
        }
        let start = self.off;
        self.off += n;
        Ok(&self.data[start..self.off])
    }

    /// Lit un tag (fourcc big-endian).
    pub fn read_tag(&mut self) -> CoreResult<SectionTag> {
        let b = self.read_bytes(4)?;
        let arr = [b[0], b[1], b[2], b[3]];
        SectionTag::from_be_bytes(arr).ok_or(CoreError::InvalidSectionTag { raw: u32::from_be_bytes(arr) })
    }

    /// Lit un u16 LE.
    pub fn read_u16_le(&mut self) -> CoreResult<u16> {
        let b = self.read_bytes(2)?;
        Ok(u16::from_le_bytes([b[0], b[1]]))
    }

    /// Lit un u32 LE.
    pub fn read_u32_le(&mut self) -> CoreResult<u32> {
        let b = self.read_bytes(4)?;
        Ok(u32::from_le_bytes([b[0], b[1], b[2], b[3]]))
    }

    /// Lit un u64 LE.
    pub fn read_u64_le(&mut self) -> CoreResult<u64> {
        let b = self.read_bytes(8)?;
        Ok(u64::from_le_bytes([b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]]))
    }

    /// Lit un i64 LE.
    pub fn read_i64_le(&mut self) -> CoreResult<i64> { Ok(self.read_u64_le()? as i64) }

    /// Lit un f64 LE.
    pub fn read_f64_le(&mut self) -> CoreResult<f64> {
        let bits = self.read_u64_le()?;
        Ok(f64::from_bits(bits))
    }
}

/* ─────────────────────────── Erreurs ─────────────────────────── */

/// Erreurs de bas niveau communes.
#[derive(Debug, Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum CoreError {
    /// Magic VITBC invalide (attendu `b"VITBC\0"`).
    InvalidMagic,
    /// Tag de section inconnu.
    InvalidSectionTag { /// Valeur brute du tag.
        raw: u32
    },
    /// Fin de buffer inattendue.
    UnexpectedEof { /// Nombre d’octets manquants.
        needed: u64, /// Offset où l’erreur s’est produite.
        at: u64
    },
    /// Longueur de section invalide (ex: dépasse le buffer).
    InvalidLength { /// Nom de section (si connu).
        section: Option<Cow<'static, str>>, /// Longueur fautive.
        len: u64
    },
    /// UTF-8 invalide.
    InvalidUtf8,
    /// Données corrompues (CRC / format).
    Corrupted(Cow<'static, str>),
}

impl CoreError {
    /// Construit une erreur « corrompu ».
    pub fn corrupted(msg: impl Into<Cow<'static, str>>) -> Self { CoreError::Corrupted(msg.into()) }
}

impl fmt::Display for CoreError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            CoreError::InvalidMagic => write!(f, "invalid VITBC magic"),
            CoreError::InvalidSectionTag { raw } => write!(f, "invalid section tag: 0x{raw:08X}"),
            CoreError::UnexpectedEof { needed, at } => write!(f, "unexpected EOF: need {needed} bytes at {at}"),
            CoreError::InvalidLength { section, len } => {
                if let Some(s) = section { write!(f, "invalid length for {s}: {len}") }
                else { write!(f, "invalid length: {len}") }
            }
            CoreError::InvalidUtf8 => write!(f, "invalid utf-8"),
            CoreError::Corrupted(msg) => write!(f, "corrupted: {msg}"),
        }
    }
}

/// Implémente `std::error::Error` uniquement avec la feature `std`.
#[cfg(feature = "std")]
impl std::error::Error for CoreError {}

/* ─────────────────────────── Prélude (reexports utiles) ─────────────────────────── */

/// Prélude pratique pour importer les types/funcs clés du crate.
pub mod prelude {
    /// Réexports utiles pour une importation rapide.
    pub use super::{
        crc32_ieee, ByteReader, ByteWriter, CoreError, CoreResult, Ident, Pos, SectionTag, Span,
        Spanned, SourceId, MAGIC_VITBC, VITBC_VERSION,
    };
}



/* ─────────────────────────── Tests ─────────────────────────── */
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn crc32_stable() {
        assert_eq!(crc32_ieee(b"hello"), crc32_ieee(b"hello"));
    }

    #[test]
    fn tags_roundtrip() {
        let t = SectionTag::CODE;
        assert_eq!(SectionTag::from_be_bytes(t.to_be_bytes()), Some(t));
    }

    #[test]
    fn writer_reader_le() -> CoreResult<()> {
        let mut w = ByteWriter::new();
        w.write_u16_le(0xBEEF);
        w.write_u32_le(0xDEAD_BEEF);
        w.write_i64_le(-42);
        w.write_f64_le(3.5);
        w.write_tag(SectionTag::DATA);

        let mut r = ByteReader::new(w.as_slice());
        assert_eq!(r.read_u16_le()?, 0xBEEF);
        assert_eq!(r.read_u32_le()?, 0xDEAD_BEEF);
        assert_eq!(r.read_i64_le()?, -42);
        assert_eq!(r.read_f64_le()?, 3.5);
        assert_eq!(r.read_tag()?, SectionTag::DATA);
        Ok(())
    }
}
