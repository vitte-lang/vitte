//! vitte-core/loader.rs
//!
//! Chargeur/Enregistreur binaire de `RawProgram` (défini dans `asm.rs`).
//!
//! Format **VITBC** (Little Endian) — compact, stable, extensible, avec intégrité CRC32.
//!
//! FILE:
//!   HEADER  |  SECTIONS  |  TRAILER
//!
//! HEADER:
//!   magic[6]   = b"VITBC\0"
//!   version    = u32                  (FILE_VERSION)
//!   flags      = u32                  (bit0: compressed=zstd si présent)
//!   entry_pc   = i64                  (=-1 si None)
//!   counts: n_ints u32, n_floats u32, n_strings u32, n_data u32, n_code u32
//!
//! SECTIONS (dans cet ordre):
//!   INT    : n × (name_len u16, name utf8, value i64)
//!   FLOAT  : n × (name_len u16, name utf8, value f64(bits u64))
//!   STR    : n × (name_len u16, name utf8, blob_len u32, blob utf8)
//!   DATA   : n × (has_name u8, [name_len u16, name]?, has_addr u8, [addr u32]?, blob_len u32, blob bytes)
//!   CODE   : n × (opcode u16, argc u8, _pad u8, args[argc] × u64)
//!
//! TRAILER:
//!   crc32  : u32  (CRC32/IEEE de tout le contenu après MAGIC jusqu'avant TRAILER)
//!   vend   : [6] = b"VEND\0\0"

use std::convert::TryInto;
use std::fmt;
use std::fs::File;
use std::io::{self, Read, Write};
use std::io::Cursor;
use std::path::Path;

use crate::asm::{DataBlob, RawOp, RawProgram};

pub const MAGIC: &[u8; 6] = b"VITBC\0";
pub const TRAILER_MAGIC: &[u8; 6] = b"VEND\0\0";
pub const FILE_VERSION: u32 = 2;

// Flags
const FLAG_COMPRESSED_ZSTD: u32 = 0x0000_0001;

// Garde-fous (anti OOM / fichiers malicieux)
const MAX_NAME_LEN: usize = u16::MAX as usize;
const MAX_STR_LEN: usize = 64 * 1024 * 1024;   // 64 MiB / string
const MAX_BLOB_LEN: usize = 256 * 1024 * 1024; // 256 MiB / blob
const MAX_CONSTS: u32 = 1_000_000;
const MAX_DATA: u32 = 1_000_000;
const MAX_CODE: u32 = 10_000_000;

// ---- Compression zstd (optionnelle) ----
// Les wrappers renvoient des trait objects avec lifetime pour éviter 'static imposé.
#[cfg(feature = "zstd")]
mod zstd_io {
    use super::*;
    pub fn maybe_wrap_writer<'a, W>(w: W, compressed: bool) -> io::Result<Box<dyn Write + 'a>>
    where
        W: Write + 'a,
    {
        if compressed {
            let enc = zstd::stream::write::Encoder::new(w, 0)?; // niveau par défaut
            Ok(Box::new(enc.auto_finish()))
        } else {
            Ok(Box::new(w))
        }
    }
    pub fn maybe_wrap_reader<'a, R>(r: R, compressed: bool) -> io::Result<Box<dyn Read + 'a>>
    where
        R: Read + 'a,
    {
        if compressed {
            let dec = zstd::stream::read::Decoder::new(r)?;
            Ok(Box::new(dec))
        } else {
            Ok(Box::new(r))
        }
    }
}

#[cfg(not(feature = "zstd"))]
mod zstd_io {
    use super::*;
    pub fn maybe_wrap_writer<'a, W>(w: W, _compressed: bool) -> io::Result<Box<dyn Write + 'a>>
    where
        W: Write + 'a,
    {
        Ok(Box::new(w))
    }
    pub fn maybe_wrap_reader<'a, R>(r: R, _compressed: bool) -> io::Result<Box<dyn Read + 'a>>
    where
        R: Read + 'a,
    {
        Ok(Box::new(r))
    }
}

#[derive(Debug, Clone)]
pub struct FileImageHeader {
    pub version: u32,
    pub flags: u32,
    pub entry_pc: Option<usize>,
    pub n_ints: u32,
    pub n_floats: u32,
    pub n_strings: u32,
    pub n_data: u32,
    pub n_code: u32,
}

#[derive(Debug)]
pub enum LoaderError {
    Io(io::Error),
    BadMagic,
    BadTrailer,
    BadVersion { got: u32, expected: u32 },
    BadFormat(&'static str),
    TooBig(&'static str),
    ChecksumMismatch { expected: u32, got: u32 },
    CompressionUnsupported,
}

impl fmt::Display for LoaderError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            LoaderError::Io(e) => write!(f, "I/O: {e}"),
            LoaderError::BadMagic => write!(f, "Mauvaise empreinte (MAGIC) — pas un fichier VITBC"),
            LoaderError::BadTrailer => write!(f, "Trailer manquant/corrompu"),
            LoaderError::BadVersion { got, expected } =>
                write!(f, "Version non supportée (got {got}, expected {expected})"),
            LoaderError::BadFormat(msg) => write!(f, "Format invalide: {msg}"),
            LoaderError::TooBig(what) => write!(f, "Taille excessive: {what}"),
            LoaderError::ChecksumMismatch { expected, got } =>
                write!(f, "CRC32 invalide (expected=0x{expected:08X}, got=0x{got:08X})"),
            LoaderError::CompressionUnsupported =>
                write!(f, "Compression demandée mais feature `zstd` désactivée"),
        }
    }
}
impl std::error::Error for LoaderError {}
impl From<io::Error> for LoaderError {
    fn from(e: io::Error) -> Self { LoaderError::Io(e) }
}

// -----------------------------
// API conviviale (fichiers/flux)
// -----------------------------

/// Sauve vers un fichier (compression facultative).
pub fn save_raw_program_to_path<P: AsRef<Path>>(
    path: P,
    prog: &RawProgram,
    compress: bool,
) -> Result<(), LoaderError> {
    #[cfg(not(feature = "zstd"))]
    if compress { return Err(LoaderError::CompressionUnsupported); }

    let mut f = File::create(path)?;
    save_raw_program(&mut f, prog, compress)
}

/// Charge depuis un fichier (décompresse si nécessaire).
pub fn load_raw_program_from_path<P: AsRef<Path>>(path: P) -> Result<RawProgram, LoaderError> {
    let mut f = File::open(path)?;
    load_raw_program(&mut f)
}

/// Sauve sur un `Write`. Si `compress == true` et la feature zstd est active,
/// les sections sont écrites compressées (CRC sur les données **décompressées**).
pub fn save_raw_program<W: Write>(
    mut sink: W,
    prog: &RawProgram,
    compress: bool,
) -> Result<(), LoaderError> {
    #[cfg(not(feature = "zstd"))]
    if compress { return Err(LoaderError::CompressionUnsupported); }

    // MAGIC
    sink.write_all(MAGIC)?;

    // Corps (header+sections) en mémoire pour calculer le CRC
    let mut body = Vec::with_capacity(4096);

    // Header + compteurs
    write_u32(&mut body, FILE_VERSION)?;
    let mut flags: u32 = 0;
    if compress { flags |= FLAG_COMPRESSED_ZSTD; }
    write_u32(&mut body, flags)?;

    let entry = prog.entry_pc.map(|pc| pc as i64).unwrap_or(-1);
    write_i64(&mut body, entry)?;

    let n_ints    = prog.const_pool.ints.len()    as u32;
    let n_floats  = prog.const_pool.floats.len()  as u32;
    let n_strings = prog.const_pool.strings.len() as u32;
    let n_data    = prog.data_blobs.len()         as u32;
    let n_code    = prog.code.len()               as u32;

    write_u32(&mut body, n_ints)?;
    write_u32(&mut body, n_floats)?;
    write_u32(&mut body, n_strings)?;
    write_u32(&mut body, n_data)?;
    write_u32(&mut body, n_code)?;

    // Sections (optionnellement compressées)
    let mut sect_buf = Vec::with_capacity(8192);
    {
        let writer = zstd_io::maybe_wrap_writer(&mut sect_buf, compress)?;
        write_sections(writer, prog)?;
    }
    body.extend_from_slice(&sect_buf);

    // CRC + TRAILER
    let crc = crc32_ieee(&body);
    write_u32(&mut body, crc)?;
    body.extend_from_slice(TRAILER_MAGIC);

    // Flush final
    sink.write_all(&body)?;
    Ok(())
}

/// Charge depuis un `Read` (auto-détection compression).
pub fn load_raw_program<R: Read>(mut source: R) -> Result<RawProgram, LoaderError> {
    // MAGIC d’en-tête
    let mut magic = [0u8; 6];
    source.read_exact(&mut magic)?;
    if &magic != MAGIC { return Err(LoaderError::BadMagic); }

    // Lis tout le reste (pour CRC + trailer)
    let mut body = Vec::new();
    source.read_to_end(&mut body)?;

    // Trailer = ... [crc u32][VEND\0\0]
    if body.len() < 4 + TRAILER_MAGIC.len() {
        return Err(LoaderError::BadTrailer);
    }
    let split = body.len() - (4 + TRAILER_MAGIC.len());
    let (body_no_trailer, trailer) = body.split_at(split);

    let mut tm = [0u8; 6];
    tm.copy_from_slice(&trailer[4..10]);
    if &tm != TRAILER_MAGIC { return Err(LoaderError::BadTrailer); }

    let got_crc = u32::from_le_bytes(trailer[0..4].try_into().unwrap());
    let exp_crc = crc32_ieee(body_no_trailer);
    if got_crc != exp_crc {
        return Err(LoaderError::ChecksumMismatch { expected: exp_crc, got: got_crc });
    }

    // Parse `body_no_trailer`
    let mut cursor = CursorLE::new(body_no_trailer);

    // Header
    let version = cursor.read_u32()?;
    if version != FILE_VERSION {
        return Err(LoaderError::BadVersion { got: version, expected: FILE_VERSION });
    }
    let flags = cursor.read_u32()?;
    let compressed = (flags & FLAG_COMPRESSED_ZSTD) != 0;

    #[cfg(not(feature = "zstd"))]
    if compressed { return Err(LoaderError::CompressionUnsupported); }

    let entry_raw = cursor.read_i64()?;
    let n_ints    = cursor.read_u32()?;
    let n_floats  = cursor.read_u32()?;
    let n_strings = cursor.read_u32()?;
    let n_data    = cursor.read_u32()?;
    let n_code    = cursor.read_u32()?;

    // Garde-fous
    guard_counts(n_ints, MAX_CONSTS, "n_ints")?;
    guard_counts(n_floats, MAX_CONSTS, "n_floats")?;
    guard_counts(n_strings, MAX_CONSTS, "n_strings")?;
    guard_counts(n_data, MAX_DATA, "n_data")?;
    guard_counts(n_code, MAX_CODE, "n_code")?;

    // Récupère les bytes des sections (potentiellement compressées)
    let sections_bytes = cursor.remaining_bytes();

    // &[u8] n’implémente pas Read → Cursor<&[u8]> ; lifetime propagée par maybe_wrap_reader
    let cur = Cursor::new(sections_bytes);
    let mut sect_reader = zstd_io::maybe_wrap_reader(cur, compressed)?;

    // Reconstruire le RawProgram
    let mut prog = RawProgram::default();
    prog.entry_pc = if entry_raw >= 0 { Some(entry_raw as usize) } else { None };

    // INT
    for _ in 0..n_ints {
        let name = read_name_from(&mut sect_reader)?;
        let val  = read_i64_from(&mut sect_reader)?;
        prog.const_pool.ints.insert(name, val);
    }

    // FLOAT
    for _ in 0..n_floats {
        let name = read_name_from(&mut sect_reader)?;
        let bits = read_u64_from(&mut sect_reader)?;
        prog.const_pool.floats.insert(name, f64::from_bits(bits));
    }

    // STRING
    for _ in 0..n_strings {
        let name = read_name_from(&mut sect_reader)?;
        let len  = read_u32_from(&mut sect_reader)? as usize;
        if len > MAX_STR_LEN { return Err(LoaderError::TooBig("string")); }
        let mut buf = vec![0u8; len];
        sect_reader.read_exact(&mut buf)?;
        let s = String::from_utf8(buf).map_err(|_| LoaderError::BadFormat("UTF-8 string invalide"))?;
        prog.const_pool.strings.insert(name, s);
    }

    // DATA
    for _ in 0..n_data {
        let has_name = read_u8_from(&mut sect_reader)?;
        let name = if has_name != 0 { Some(read_name_from(&mut sect_reader)?) } else { None };
        let has_addr = read_u8_from(&mut sect_reader)?;
        let addr = if has_addr != 0 { Some(read_u32_from(&mut sect_reader)?) } else { None };
        let len  = read_u32_from(&mut sect_reader)? as usize;
        if len > MAX_BLOB_LEN { return Err(LoaderError::TooBig("data blob")); }
        let mut bytes = vec![0u8; len];
        sect_reader.read_exact(&mut bytes)?;
        prog.data_blobs.push(DataBlob { name, bytes, addr });
    }

    // CODE
    for _ in 0..n_code {
        let opcode = read_u16_from(&mut sect_reader)?;
        let argc   = read_u8_from(&mut sect_reader)?;
        let _pad   = read_u8_from(&mut sect_reader)?;
        if argc > 3 { return Err(LoaderError::BadFormat("argc > 3")); }
        let mut args = [0u64; 3];
        for i in 0..(argc as usize) {
            args[i] = read_u64_from(&mut sect_reader)?;
        }
        prog.code.push(RawOp { opcode, argc, args });
    }

    Ok(prog)
}

// -----------------------------
// Écriture des sections (non compressées ici)
// -----------------------------
fn write_sections<W: Write>(mut w: W, prog: &RawProgram) -> Result<(), LoaderError> {
    // INT
    for (name, val) in &prog.const_pool.ints {
        write_name_to(&mut w, name)?;
        write_i64_to(&mut w, *val)?;
    }
    // FLOAT
    for (name, val) in &prog.const_pool.floats {
        write_name_to(&mut w, name)?;
        write_u64_to(&mut w, val.to_bits())?;
    }
    // STRING
    for (name, s) in &prog.const_pool.strings {
        write_name_to(&mut w, name)?;
        let bytes = s.as_bytes();
        if bytes.len() > MAX_STR_LEN { return Err(LoaderError::TooBig("string")); }
        write_u32_to(&mut w, bytes.len() as u32)?;
        w.write_all(bytes)?;
    }
    // DATA
    for blob in &prog.data_blobs {
        match &blob.name {
            Some(n) => { write_u8_to(&mut w, 1)?; write_name_to(&mut w, n)?; }
            None    => { write_u8_to(&mut w, 0)?; }
        }
        match blob.addr {
            Some(a) => { write_u8_to(&mut w, 1)?; write_u32_to(&mut w, a)?; }
            None    => { write_u8_to(&mut w, 0)?; }
        }
        if blob.bytes.len() > MAX_BLOB_LEN { return Err(LoaderError::TooBig("data blob")); }
        write_u32_to(&mut w, blob.bytes.len() as u32)?;
        w.write_all(&blob.bytes)?;
    }
    // CODE
    for op in &prog.code {
        write_u16_to(&mut w, op.opcode)?;
        write_u8_to(&mut w, op.argc)?;
        write_u8_to(&mut w, 0)?; // pad
        for i in 0..(op.argc as usize) {
            write_u64_to(&mut w, op.args[i])?;
        }
    }
    Ok(())
}

// -----------------------------
// Helpers LE (version « direct to Write »)
// -----------------------------
#[inline] fn write_u8_to<W: Write>(w: &mut W, v: u8) -> io::Result<()> { w.write_all(&[v]) }
#[inline] fn write_u16_to<W: Write>(w: &mut W, v: u16) -> io::Result<()> { w.write_all(&v.to_le_bytes()) }
#[inline] fn write_u32_to<W: Write>(w: &mut W, v: u32) -> io::Result<()> { w.write_all(&v.to_le_bytes()) }
#[inline] fn write_u64_to<W: Write>(w: &mut W, v: u64) -> io::Result<()> { w.write_all(&v.to_le_bytes()) }
#[inline] fn write_i64_to<W: Write>(w: &mut W, v: i64) -> io::Result<()> { w.write_all(&v.to_le_bytes()) }

// Variante sur Vec<u8> (utilisée pour le `body` non compressé)
#[inline] fn write_u32(buf: &mut Vec<u8>, v: u32) -> io::Result<()> { buf.extend_from_slice(&v.to_le_bytes()); Ok(()) }
#[inline] fn write_i64(buf: &mut Vec<u8>, v: i64) -> io::Result<()> { buf.extend_from_slice(&v.to_le_bytes()); Ok(()) }

// -----------------------------
// Helpers LE (lecture depuis Read)
// -----------------------------
#[inline] fn read_u8_from<R: Read>(r: &mut R) -> io::Result<u8> { let mut b=[0u8;1]; r.read_exact(&mut b)?; Ok(b[0]) }
#[inline] fn read_u16_from<R: Read>(r: &mut R) -> io::Result<u16> { let mut b=[0u8;2]; r.read_exact(&mut b)?; Ok(u16::from_le_bytes(b)) }
#[inline] fn read_u32_from<R: Read>(r: &mut R) -> io::Result<u32> { let mut b=[0u8;4]; r.read_exact(&mut b)?; Ok(u32::from_le_bytes(b)) }
#[inline] fn read_u64_from<R: Read>(r: &mut R) -> io::Result<u64> { let mut b=[0u8;8]; r.read_exact(&mut b)?; Ok(u64::from_le_bytes(b)) }
#[inline] fn read_i64_from<R: Read>(r: &mut R) -> io::Result<i64> { let mut b=[0u8;8]; r.read_exact(&mut b)?; Ok(i64::from_le_bytes(b)) }

fn write_name_to<W: Write>(w: &mut W, s: &str) -> io::Result<()> {
    let bytes = s.as_bytes();
    if bytes.len() > MAX_NAME_LEN {
        return Err(io::Error::new(io::ErrorKind::InvalidInput, "nom trop long (> u16::MAX)"));
    }
    write_u16_to(w, bytes.len() as u16)?;
    w.write_all(bytes)
}

fn read_name_from<R: Read>(r: &mut R) -> Result<String, LoaderError> {
    let len = read_u16_from(r)? as usize;
    if len > MAX_NAME_LEN { return Err(LoaderError::TooBig("name")); }
    let mut buf = vec![0u8; len];
    r.read_exact(&mut buf)?;
    String::from_utf8(buf).map_err(|_| LoaderError::BadFormat("nom non UTF-8"))
}

// -----------------------------
// Petit reader LE (pour parser le body et garder le reste en « sections »)
// -----------------------------
struct CursorLE<'a> {
    buf: &'a [u8],
    off: usize,
}
impl<'a> CursorLE<'a> {
    fn new(buf: &'a [u8]) -> Self { Self { buf, off: 0 } }
    fn read_exact(&mut self, n: usize) -> Result<&'a [u8], LoaderError> {
        if self.off + n > self.buf.len() { return Err(LoaderError::BadFormat("EOF")); }
        let s = &self.buf[self.off..self.off+n];
        self.off += n;
        Ok(s)
    }
    fn read_u32(&mut self) -> Result<u32, LoaderError> {
        let b = self.read_exact(4)?; Ok(u32::from_le_bytes(b.try_into().unwrap()))
    }
    fn read_i64(&mut self) -> Result<i64, LoaderError> {
        let b = self.read_exact(8)?; Ok(i64::from_le_bytes(b.try_into().unwrap()))
    }
    fn remaining_bytes(&mut self) -> &'a [u8] {
        let s = &self.buf[self.off..];
        self.off = self.buf.len();
        s
    }
}

// -----------------------------
// CRC32 (IEEE, polynôme 0xEDB88320), version safe et const
// -----------------------------
fn crc32_ieee(data: &[u8]) -> u32 {
    const fn make_table() -> [u32; 256] {
        let mut tbl = [0u32; 256];
        let mut i = 0;
        while i < 256 {
            let mut c = i as u32;
            let mut j = 0;
            while j < 8 {
                c = if (c & 1) != 0 {
                    0xEDB88320u32 ^ (c >> 1)
                } else {
                    c >> 1
                };
                j += 1;
            }
            tbl[i] = c;
            i += 1;
        }
        tbl
    }

    const TABLE: [u32; 256] = make_table();

    let mut c: u32 = 0xFFFF_FFFF;
    for &b in data {
        c = TABLE[((c ^ (b as u32)) & 0xFF) as usize] ^ (c >> 8);
    }
    !c
}
// -----------------------------
// Garde-fous
// -----------------------------
fn guard_counts(val: u32, max: u32, label: &'static str) -> Result<(), LoaderError> {
    if val > max { Err(LoaderError::TooBig(label)) } else { Ok(()) }
}

// -----------------------------
// Tests basiques
// -----------------------------
#[cfg(test)]
mod tests {
    use super::*;

    fn sample_prog() -> RawProgram {
        let mut p = RawProgram::default();
        p.entry_pc = Some(2);
        p.const_pool.ints.insert("X".into(), 7);
        p.const_pool.floats.insert("PI".into(), 3.14);
        p.const_pool.strings.insert("msg".into(), "Hello".into());
        p.data_blobs.push(DataBlob {
            name: Some("blob".into()),
            bytes: vec![1,2,3,4,5,6,7,8],
            addr: Some(0x1000),
        });
        p.code.push(RawOp { opcode: 0x01, argc: 2, args: [1,2,0] });
        p.code.push(RawOp { opcode: 0x0C, argc: 0, args: [0,0,0] });
        p
    }

    #[test]
    fn roundtrip_uncompressed() {
        let prog = sample_prog();
        let mut buf = Vec::new();
        save_raw_program(&mut buf, &prog, false).unwrap();
        let back = load_raw_program(&buf[..]).unwrap();
        assert_eq!(back.entry_pc, Some(2));
        assert_eq!(back.const_pool.ints.get("X"), Some(&7));
        assert_eq!(back.const_pool.strings.get("msg").map(String::as_str), Some("Hello"));
        assert_eq!(back.data_blobs.len(), 1);
        assert_eq!(back.code.len(), 2);
        assert_eq!(back.code[0].opcode, 0x01);
    }

    #[cfg(feature = "zstd")]
    #[test]
    fn roundtrip_compressed() {
        let prog = sample_prog();
        let mut buf = Vec::new();
        save_raw_program(&mut buf, &prog, true).unwrap();
        let back = load_raw_program(&buf[..]).unwrap();
        assert_eq!(back.entry_pc, Some(2));
        assert_eq!(back.code.len(), 2);
    }

    #[test]
    fn checksum_detects_corruption() {
        let prog = sample_prog();
        let mut buf = Vec::new();
        save_raw_program(&mut buf, &prog, false).unwrap();
        if let Some(b) = buf.get_mut(MAGIC.len() + 12) { *b ^= 0xFF; }
        let err = load_raw_program(&buf[..]).unwrap_err();
        match err {
            LoaderError::ChecksumMismatch { .. } => {}
            e => panic!("attendu ChecksumMismatch, got {:?}", e),
        }
    }
}
