//! vitte-vitbc — Spécification & IO du format binaire VITBC
//!
//! Format :
//! ```text
//! Header: "VITBC\0" (6 bytes) + version u16 LE
//! [Section*]
//!   section = TAG[4] + len u32 LE + payload
//! Dernière section: "CRCC" + u32 (CRC32 sur tout après header)
//! ```
//!
//! Sections supportées :
//! - "INTS" : tableau d’i64
//! - "FLTS" : tableau de f64
//! - "STRS" : strings (len u32 + bytes)
//! - "DATA" : octets bruts
//! - "CODE" : bytecode (optionnellement compressé zstd)
//! - "NAME" : noms (fonctions, etc.)
//!
//! API :
//! - `Module::to_bytes()` / `from_bytes()`
//! - `write_file()`, `read_file()` (feature std)
//!
//! Ce crate est utilisé par `vitte-compiler` pour sérialiser le bytecode produit.

#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]
#![cfg_attr(not(feature = "std"), no_std)]

// ─── alloc uniquement en no_std ───
#[cfg(not(feature = "std"))]
extern crate alloc;

// Imports côté std
#[cfg(feature = "std")]
use std::{fs, io::Read, path::Path};

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

// Imports côté no_std (via alloc)
#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};

use vitte_core::{crc32_ieee, ByteReader, ByteWriter, CoreError, CoreResult, SectionTag};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "zstd")]
use zstd::bulk;

/// Représentation d’un module VITBC en mémoire.
#[derive(Debug, Clone, Default)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Module {
    /// Version du format
    pub version: u16,
    /// Entiers
    pub ints: Vec<i64>,
    /// Flottants
    pub floats: Vec<f64>,
    /// Chaînes
    pub strings: Vec<String>,
    /// Données brutes
    pub data: Vec<u8>,
    /// Code (bytecode VM)
    pub code: Vec<u8>,
    /// Noms (debug)
    pub names: Vec<String>,
    /// CRC32 attendu (calculé à l’écriture / vérifié à la lecture)
    pub crc32: u32,
}

impl Module {
    /// Sérialise en binaire VITBC (avec CRC32).
    pub fn to_bytes(&self, compress_code: bool) -> CoreResult<Vec<u8>> {
        let mut w = ByteWriter::new();

        // Magic + version
        w.write_bytes(b"VITBC\0");
        w.write_u16_le(self.version);

        // INTS
        if !self.ints.is_empty() {
            w.write_tag(SectionTag::INTS);
            let mut buf = ByteWriter::new();
            for v in &self.ints {
                buf.write_i64_le(*v);
            }
            w.write_u32_le(buf.as_slice().len() as u32);
            w.write_bytes(buf.as_slice());
        }

        // FLTS
        if !self.floats.is_empty() {
            w.write_tag(SectionTag::FLTS);
            let mut buf = ByteWriter::new();
            for v in &self.floats {
                buf.write_f64_le(*v);
            }
            w.write_u32_le(buf.as_slice().len() as u32);
            w.write_bytes(buf.as_slice());
        }

        // STRS
        if !self.strings.is_empty() {
            w.write_tag(SectionTag::STRS);
            let mut buf = ByteWriter::new();
            for s in &self.strings {
                buf.write_u32_le(s.len() as u32);
                buf.write_bytes(s.as_bytes());
            }
            w.write_u32_le(buf.as_slice().len() as u32);
            w.write_bytes(buf.as_slice());
        }

        // DATA
        if !self.data.is_empty() {
            w.write_tag(SectionTag::DATA);
            w.write_u32_le(self.data.len() as u32);
            w.write_bytes(&self.data);
        }

        // CODE
        if !self.code.is_empty() {
            w.write_tag(SectionTag::CODE);
            let payload: Vec<u8>;
            #[cfg(feature = "zstd")]
            {
                if compress_code {
                    payload = bulk::compress(&self.code, 3).unwrap_or(self.code.clone());
                } else {
                    payload = self.code.clone();
                }
            }
            #[cfg(not(feature = "zstd"))]
            {
                let _ = compress_code;
                payload = self.code.clone();
            }
            w.write_u32_le(payload.len() as u32);
            w.write_bytes(&payload);
        }

        // NAME
        if !self.names.is_empty() {
            w.write_tag(SectionTag::NAME);
            let mut buf = ByteWriter::new();
            for s in &self.names {
                buf.write_u32_le(s.len() as u32);
                buf.write_bytes(s.as_bytes());
            }
            w.write_u32_le(buf.as_slice().len() as u32);
            w.write_bytes(buf.as_slice());
        }

        // CRC32 sur tout sauf magic/version
        let bytes = w.into_vec();
        let crc = crc32_ieee(&bytes[6..]);
        let mut out = bytes;
        out.extend_from_slice(b"CRCC");
        out.extend_from_slice(&crc.to_le_bytes());

        Ok(out)
    }

    /// Reconstruit un module VITBC à partir d’octets.
    pub fn from_bytes(data: &[u8]) -> CoreResult<Self> {
        let mut r = ByteReader::new(data);
        let magic = r.read_bytes(6)?;
        if magic != b"VITBC\0" {
            return Err(CoreError::corrupted("invalid VITBC magic"));
        }
        let version = r.read_u16_le()?;

        let mut m = Module::default();
        m.version = version;

        // Sections
        while r.remaining() > 0 {
            let tag = r.read_tag()?;
            if tag == SectionTag::CRCC {
                let expected = r.read_u32_le()?;
                let crc = crc32_ieee(&data[6..data.len() - 8]); // 6=magic+ver, 8=CRCC+u32
                if expected != crc {
                    return Err(CoreError::corrupted("CRC32 mismatch"));
                }
                m.crc32 = expected;
                break;
            }
            let len = r.read_u32_le()? as usize;
            let payload = r.read_bytes(len)?;
            match tag {
                SectionTag::INTS => {
                    let mut rr = ByteReader::new(payload);
                    while rr.remaining() > 0 {
                        m.ints.push(rr.read_i64_le()?);
                    }
                }
                SectionTag::FLTS => {
                    let mut rr = ByteReader::new(payload);
                    while rr.remaining() > 0 {
                        m.floats.push(rr.read_f64_le()?);
                    }
                }
                SectionTag::STRS => {
                    let mut rr = ByteReader::new(payload);
                    while rr.remaining() > 0 {
                        let l = rr.read_u32_le()? as usize;
                        let s = rr.read_bytes(l)?;
                        m.strings.push(
                            core::str::from_utf8(s)
                                .map_err(|_| CoreError::corrupted("invalid UTF-8"))?
                                .to_string(),
                        );
                    }
                }
                SectionTag::DATA => m.data.extend_from_slice(payload),
                SectionTag::CODE => {
                    #[cfg(feature = "zstd")]
                    {
                        if let Ok(decomp) = bulk::decompress(payload, usize::MAX) {
                            m.code = decomp;
                        } else {
                            m.code = payload.to_vec();
                        }
                    }
                    #[cfg(not(feature = "zstd"))]
                    {
                        m.code = payload.to_vec();
                    }
                }
                SectionTag::NAME => {
                    let mut rr = ByteReader::new(payload);
                    while rr.remaining() > 0 {
                        let l = rr.read_u32_le()? as usize;
                        let s = rr.read_bytes(l)?;
                        m.names.push(
                            core::str::from_utf8(s)
                                .map_err(|_| CoreError::corrupted("invalid UTF-8"))?
                                .to_string(),
                        );
                    }
                }
                _ => {
                    // ignore inconnu
                }
            }
        }

        Ok(m)
    }

    /// Écrit dans un fichier (nécessite std).
    #[cfg(feature = "std")]
    pub fn write_file<P: AsRef<Path>>(&self, path: P) -> CoreResult<()> {
        let bytes = self.to_bytes(false)?;
        fs::write(path, bytes).map_err(|e| CoreError::corrupted(format!("io write error: {e}")))
    }

    /// Lit un module depuis un fichier (nécessite std).
    #[cfg(feature = "std")]
    pub fn read_file<P: AsRef<Path>>(path: P) -> CoreResult<Self> {
        let mut buf = Vec::new();
        let mut file = fs::File::open(path).map_err(|e| CoreError::corrupted(format!("io open error: {e}")))?;
        file.read_to_end(&mut buf).map_err(|e| CoreError::corrupted(format!("io read error: {e}")))?;
        Self::from_bytes(&buf)
}
}

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn roundtrip_basic() {
        let mut m = Module::default();
        m.version = 2;
        m.ints = vec![1, 2, 3];
        m.floats = vec![1.0, 2.0];
        m.strings = vec!["hi".into()];
        m.data = vec![42];
        m.code = vec![0xAA, 0xBB];
        m.names = vec!["foo".into()];

        let bytes = m.to_bytes(false).unwrap();
        let m2 = Module::from_bytes(&bytes).unwrap();
        assert_eq!(m.ints, m2.ints);
        assert_eq!(m.strings, m2.strings);
        assert_eq!(m.names, m2.names);
    }
}
