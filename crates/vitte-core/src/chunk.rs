//! Bytecode Chunk (constantes + instructions) avec roundtrip binaire et CRC32.
//!
//! Format binaire (LE) :
//! ```text
//! "CHNK"(4) | VER(1)=1 | FLAGS(1)
//! | N_CONST(u32) | CONSTS...
//! | N_OPS(u32)   | OPS...
//! | CRC32(u32)   // CRC du payload sans le CRC lui-même
//! ```
//! CONSTS:
//!   0x01 | LEN(u32) | BYTES  // String UTF-8
//! OPS:
//!   0x10 | INDEX(u32)        // LoadConst
//!   0x11                     // Print
//!   0x12                     // Return

#![cfg_attr(not(feature = "std"), no_std)]

use core::fmt;

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};
#[cfg(feature = "std")]
use std::{string::String, vec::Vec};
#[cfg(not(feature = "std"))]
extern crate alloc;

/// CRC32 (IEEE 802.3) compact (pas de table).
#[inline]
pub fn crc32_ieee(data: &[u8]) -> u32 {
    let mut crc: u32 = 0xFFFF_FFFF;
    for &b in data {
        let mut x = (crc ^ (b as u32)) & 0xFF;
        for _ in 0..8 {
            let mask = (x & 1).wrapping_neg() & 0xEDB8_8320;
            x = (x >> 1) ^ mask;
        }
        crc = (crc >> 8) ^ x;
    }
    !crc
}

/// OpCodes minimalistes pour la démo/tests.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Op {
    /// Charge la constante d’index `u32`.
    LoadConst(u32),
    /// Affiche (debug/demo).
    Print,
    /// Retourne de la fonction.
    Return,
}

/// Constantes du chunk.
#[derive(Debug, Clone, PartialEq)]
pub enum K {
    /// Chaîne UTF-8.
    Str(String),
}

/// Erreurs de parsing/validation d’un chunk.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ChunkError {
    /// Format invalide (détail textuel).
    Format(&'static str),
    /// Incohérence d’intégrité (CRC32).
    HashMismatch { expected: u32, found: u32 },
}

impl fmt::Display for ChunkError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ChunkError::Format(m) => write!(f, "format error: {m}"),
            ChunkError::HashMismatch { expected, found } => {
                write!(
                    f,
                    "hash mismatch (crc32): expected=0x{expected:08X}, found=0x{found:08X}"
                )
            }
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ChunkError {}

/// Conteneur de bytecode (constantes + instructions).
#[derive(Debug, Clone, PartialEq)]
pub struct Chunk {
    /// Table des constantes.
    pub consts: Vec<K>,
    /// Suite d’instructions.
    pub ops: Vec<Op>,
    /// Flag interne (peut servir à la compression, etc.).
    pub compact: bool,
}

impl Chunk {
    /// Construit un chunk vide.
    pub fn new(compact: bool) -> Self {
        Self {
            consts: Vec::new(),
            ops: Vec::new(),
            compact,
        }
    }

    /// Sérialise en bytes avec CRC32 final.
    ///
    /// Voir l’en-tête du fichier pour le format précis.
    pub fn to_bytes(&self) -> Vec<u8> {
        let mut out = Vec::new();

        // Header
        out.extend_from_slice(b"CHNK");
        out.push(1); // version
        out.push(if self.compact { 1 } else { 0 });

        // Constantes
        write_u32(&mut out, self.consts.len() as u32);
        for k in &self.consts {
            match k {
                K::Str(s) => {
                    out.push(0x01);
                    let b = s.as_bytes();
                    write_u32(&mut out, b.len() as u32);
                    out.extend_from_slice(b);
                }
            }
        }

        // Ops
        write_u32(&mut out, self.ops.len() as u32);
        for op in &self.ops {
            match *op {
                Op::LoadConst(ix) => {
                    out.push(0x10);
                    write_u32(&mut out, ix);
                }
                Op::Print => out.push(0x11),
                Op::Return => out.push(0x12),
            }
        }

        // CRC (sur tout le payload précédent)
        let crc = crc32_ieee(&out);
        write_u32(&mut out, crc);

        out
    }

    /// Désérialise depuis des bytes et valide le CRC32.
    pub fn from_bytes(data: &[u8]) -> Result<Self, ChunkError> {
        // Taille minimale: magic(4)+ver(1)+flags(1)+n_consts(4)+n_ops(4)+crc(4)
        if data.len() < 18 {
            return Err(ChunkError::Format("too short"));
        }

        // Magic + version
        if &data[0..4] != b"CHNK" {
            return Err(ChunkError::Format("bad magic"));
        }
        let ver = data[4];
        if ver != 1 {
            return Err(ChunkError::Format("unsupported version"));
        }
        let flags = data[5];

        // Séparer payload / CRC
        let payload = &data[..data.len().saturating_sub(4)];
        let crc_given = read_u32(&data[data.len() - 4..])?;
        let crc_calc = crc32_ieee(payload);
        if crc_given != crc_calc {
            return Err(ChunkError::HashMismatch {
                expected: crc_calc,
                found: crc_given,
            });
        }

        // Lecture séquentielle
        let mut off = 6;

        // Constantes
        let n_consts = read_u32(slice(data, off, 4)?)?;
        off += 4;
        let mut consts = Vec::with_capacity(n_consts as usize);
        for _ in 0..n_consts {
            let tag = *data.get(off).ok_or(ChunkError::Format("const tag eof"))?;
            off += 1;
            match tag {
                0x01 => {
                    let len = read_u32(slice(data, off, 4)?)? as usize;
                    off += 4;
                    let sbytes = slice(data, off, len)?;
                    off += len;
                    let s = core::str::from_utf8(sbytes).map_err(|_| ChunkError::Format("utf8"))?;
                    consts.push(K::Str(s.to_string()));
                }
                _ => return Err(ChunkError::Format("unknown const tag")),
            }
        }

        // Ops
        let n_ops = read_u32(slice(data, off, 4)?)?;
        off += 4;
        let mut ops = Vec::with_capacity(n_ops as usize);
        for _ in 0..n_ops {
            let tag = *data.get(off).ok_or(ChunkError::Format("op tag eof"))?;
            off += 1;
            match tag {
                0x10 => {
                    let ix = read_u32(slice(data, off, 4)?)?;
                    off += 4;
                    ops.push(Op::LoadConst(ix));
                }
                0x11 => ops.push(Op::Print),
                0x12 => ops.push(Op::Return),
                _ => return Err(ChunkError::Format("unknown op tag")),
            }
        }

        Ok(Chunk {
            consts,
            ops,
            compact: flags & 1 == 1,
        })
    }
}

/* ─────────────────────────── Helpers internes ─────────────────────────── */

#[inline]
fn write_u32(buf: &mut Vec<u8>, v: u32) {
    buf.extend_from_slice(&v.to_le_bytes());
}

#[inline]
fn read_u32(b: &[u8]) -> Result<u32, ChunkError> {
    if b.len() < 4 {
        return Err(ChunkError::Format("u32 eof"));
    }
    Ok(u32::from_le_bytes([b[0], b[1], b[2], b[3]]))
}

#[inline]
fn slice<'a>(b: &'a [u8], off: usize, len: usize) -> Result<&'a [u8], ChunkError> {
    if off.checked_add(len).map(|e| e <= b.len()).unwrap_or(false) {
        Ok(&b[off..off + len])
    } else {
        Err(ChunkError::Format("slice oob"))
    }
}

/* ─────────────────────────── Helpers pour tests / ergonomie ─────────────────────────── */

/// Helpers simples utilisés par les tests.
pub mod helpers {
    use super::{Chunk, K};

    /// Crée un chunk vide.
    pub fn new_chunk(compact: bool) -> Chunk {
        Chunk::new(compact)
    }

    /// Ajoute une constante string et renvoie son index.
    pub fn k_str(c: &mut Chunk, s: &str) -> u32 {
        c.consts.push(K::Str(s.to_string()));
        (c.consts.len() - 1) as u32
    }
}

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn roundtrip_ok_and_crc_catches_corruption() {
        let mut c = helpers::new_chunk(false);
        let k = helpers::k_str(&mut c, "yo");
        c.ops.push(Op::LoadConst(k));
        c.ops.push(Op::Print);
        c.ops.push(Op::Return);

        let mut bytes = c.to_bytes();
        // lecture OK
        let c2 = Chunk::from_bytes(&bytes).expect("roundtrip ok");
        assert_eq!(c2.ops.len(), 3);

        // corruption volontaire
        let last = bytes.len().saturating_sub(1);
        bytes[last] ^= 0xFF;
        let err = Chunk::from_bytes(&bytes).unwrap_err();
        let s = err.to_string().to_lowercase();
        assert!(s.contains("hash"));
    }
}
