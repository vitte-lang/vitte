//! vitte-compress — unified compression/decompression for Vitte
//!
//! Features
//! - In-memory `compress` / `decompress` with pluggable backends.
//! - Optional `io` helpers (sync, read-all → write-all).
//! - Optional `async-io` helpers (tokio), same semantics.
//!
//! Backends (feature-gated)
//! - `deflate` → flate2 (deflate/zlib/gzip via options or zopfli for gzip if `zopfli` enabled)
//! - `zstd`    → zstd
//! - `lz4`     → lz4_flex (block format)
//! - `brotli`  → brotli (generic)
//! - `xz`      → xz2 (lzma/xz)
//! - `zopfli`  → zopfli (slow, high ratio gzip/deflate)
//!
//! Default: `std` only. Enable what you need.

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};
#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

/// Crate-wide result type.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// High-level error kinds.
#[derive(Debug)]
pub enum Error {
    /// Requested algorithm is not compiled in.
    Unsupported(&'static str),
    /// Input is invalid or corrupted.
    InvalidData(String),
    /// Generic I/O or backend error (stringified to avoid heavy trait bounds in no_std).
    Backend(String),
}

impl From<&str> for Error {
    #[inline]
    fn from(s: &str) -> Self {
        Error::Backend(s.into())
    }
}
impl From<String> for Error {
    #[inline]
    fn from(s: String) -> Self {
        Error::Backend(s)
    }
}

/// Compression algorithm.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Algo {
    /// DEFLATE family. See [`DeflateFlavor`].
    Deflate(DeflateFlavor),
    /// Zstandard.
    Zstd,
    /// LZ4 block format (not .lz4 frame).
    Lz4,
    /// Brotli.
    Brotli,
    /// XZ (LZMA2 in .xz container).
    Xz,
    /// Zopfli optimal DEFLATE (slow). Flavor applies.
    Zopfli(DeflateFlavor),
}

/// Deflate flavor.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DeflateFlavor {
    /// Raw DEFLATE stream (no zlib header).
    Raw,
    /// Zlib-wrapped.
    Zlib,
    /// Gzip-wrapped.
    Gzip,
}

/// Compression options. Fields ignored by algorithms that do not use them.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Options {
    /// Compression level. Semantics per backend.
    pub level: i32,
    /// Brotli quality (0..=11), Zstd level (negative..positive), LZ4 ignored.
    pub brotli_window_bits: i32,
}
impl Default for Options {
    fn default() -> Self {
        Self { level: 3, brotli_window_bits: 22 }
    }
}

/// Compress whole buffer with given algorithm and options.
pub fn compress(input: &[u8], algo: Algo, opt: Options) -> Result<Vec<u8>> {
    match algo {
        Algo::Deflate(flavor) => compress_deflate(input, flavor, opt),
        Algo::Zstd => compress_zstd(input, opt),
        Algo::Lz4 => compress_lz4(input),
        Algo::Brotli => compress_brotli(input, opt),
        Algo::Xz => compress_xz(input, opt),
        Algo::Zopfli(flavor) => compress_zopfli(input, flavor, opt),
    }
}

/// Decompress whole buffer with given algorithm.
/// For autodetection, see [`sniff_algo`].
pub fn decompress(input: &[u8], algo: Algo) -> Result<Vec<u8>> {
    match algo {
        Algo::Deflate(flavor) => decompress_deflate(input, flavor),
        Algo::Zstd => decompress_zstd(input),
        Algo::Lz4 => decompress_lz4(input),
        Algo::Brotli => decompress_brotli(input),
        Algo::Xz => decompress_xz(input),
        Algo::Zopfli(_f) => Err(Error::Unsupported("zopfli is encoder-only")),
    }
}

/// Try to guess algorithm from magic bytes. Returns None if unknown.
/// - gzip: 1F 8B
/// - zstd: 28 B5 2F FD
/// - xz:   FD 37 7A 58 5A 00
/// - brotli: no fixed magic for raw brotli stream (returns None)
/// - zlib: 78 ?? (heuristic; may collide)
pub fn sniff_algo(data: &[u8]) -> Option<Algo> {
    if data.len() >= 2 && data[0] == 0x1F && data[1] == 0x8B {
        return Some(Algo::Deflate(DeflateFlavor::Gzip));
    }
    if data.len() >= 4 && data[..4] == [0x28, 0xB5, 0x2F, 0xFD] {
        return Some(Algo::Zstd);
    }
    if data.len() >= 6 && data[..6] == [0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00] {
        return Some(Algo::Xz);
    }
    if data.len() >= 2 && data[0] == 0x78 {
        return Some(Algo::Deflate(DeflateFlavor::Zlib));
    }
    None
}

/* -------------------------- Backends: Deflate --------------------------- */

#[cfg(feature = "deflate")]
fn compress_deflate(input: &[u8], flavor: DeflateFlavor, opt: Options) -> Result<Vec<u8>> {
    use flate2::write::{DeflateEncoder, GzEncoder, ZlibEncoder};
    use flate2::Compression;
    let lvl = Compression::new(opt.level.max(0) as u32);
    match flavor {
        DeflateFlavor::Raw => {
            use flate2::write::DeflateEncoder as RawEnc;
            let mut w = RawEnc::new(Vec::new(), lvl);
            use core::fmt::Write as _;
            // no_std friendly write_all
            w.write_all(input).map_err(|e| Error::Backend(e.to_string()))?;
            w.finish().map_err(|e| Error::Backend(e.to_string()))
        }
        DeflateFlavor::Zlib => {
            let mut w = ZlibEncoder::new(Vec::new(), lvl);
            w.write_all(input).map_err(|e| Error::Backend(e.to_string()))?;
            w.finish().map_err(|e| Error::Backend(e.to_string()))
        }
        DeflateFlavor::Gzip => {
            let mut w = GzEncoder::new(Vec::new(), lvl);
            w.write_all(input).map_err(|e| Error::Backend(e.to_string()))?;
            w.finish().map_err(|e| Error::Backend(e.to_string()))
        }
    }
}

#[cfg(not(feature = "deflate"))]
fn compress_deflate(_input: &[u8], _flavor: DeflateFlavor, _opt: Options) -> Result<Vec<u8>> {
    Err(Error::Unsupported("deflate"))
}

#[cfg(feature = "deflate")]
fn decompress_deflate(input: &[u8], flavor: DeflateFlavor) -> Result<Vec<u8>> {
    use flate2::read::{DeflateDecoder, GzDecoder, ZlibDecoder};
    use std::io::Read;
    match flavor {
        DeflateFlavor::Raw => {
            let mut r = DeflateDecoder::new(input);
            let mut out = Vec::new();
            r.read_to_end(&mut out).map_err(|e| Error::InvalidData(e.to_string()))?;
            Ok(out)
        }
        DeflateFlavor::Zlib => {
            let mut r = ZlibDecoder::new(input);
            let mut out = Vec::new();
            r.read_to_end(&mut out).map_err(|e| Error::InvalidData(e.to_string()))?;
            Ok(out)
        }
        DeflateFlavor::Gzip => {
            let mut r = GzDecoder::new(input);
            let mut out = Vec::new();
            r.read_to_end(&mut out).map_err(|e| Error::InvalidData(e.to_string()))?;
            Ok(out)
        }
    }
}

#[cfg(not(feature = "deflate"))]
fn decompress_deflate(_input: &[u8], _flavor: DeflateFlavor) -> Result<Vec<u8>> {
    Err(Error::Unsupported("deflate"))
}

/* ---------------------------- Backends: Zstd ---------------------------- */

#[cfg(feature = "zstd")]
fn compress_zstd(input: &[u8], opt: Options) -> Result<Vec<u8>> {
    zstd::encode_all(input, opt.level).map_err(|e| Error::Backend(e.to_string()))
}
#[cfg(not(feature = "zstd"))]
fn compress_zstd(_input: &[u8], _opt: Options) -> Result<Vec<u8>> {
    Err(Error::Unsupported("zstd"))
}

#[cfg(feature = "zstd")]
fn decompress_zstd(input: &[u8]) -> Result<Vec<u8>> {
    zstd::decode_all(input).map_err(|e| Error::InvalidData(e.to_string()))
}
#[cfg(not(feature = "zstd"))]
fn decompress_zstd(_input: &[u8]) -> Result<Vec<u8>> {
    Err(Error::Unsupported("zstd"))
}

/* ----------------------------- Backends: LZ4 ---------------------------- */

#[cfg(feature = "lz4")]
fn compress_lz4(input: &[u8]) -> Result<Vec<u8>> {
    Ok(lz4_flex::block::compress_prepend_size(input))
}
#[cfg(not(feature = "lz4"))]
fn compress_lz4(_input: &[u8]) -> Result<Vec<u8>> {
    Err(Error::Unsupported("lz4"))
}

#[cfg(feature = "lz4")]
fn decompress_lz4(input: &[u8]) -> Result<Vec<u8>> {
    lz4_flex::block::decompress_size_prepended(input).map_err(|e| Error::InvalidData(e.to_string()))
}
#[cfg(not(feature = "lz4"))]
fn decompress_lz4(_input: &[u8]) -> Result<Vec<u8>> {
    Err(Error::Unsupported("lz4"))
}

/* ---------------------------- Backends: Brotli -------------------------- */

#[cfg(feature = "brotli")]
fn compress_brotli(input: &[u8], opt: Options) -> Result<Vec<u8>> {
    let mut out = Vec::new();
    let mut enc = brotli::CompressorWriter::new(
        &mut out,
        4096,
        opt.level.clamp(0, 11) as u32,
        opt.brotli_window_bits.clamp(10, 24) as u32,
    );
    use core::fmt::Write as _;
    enc.write_all(input).map_err(|e| Error::Backend(e.to_string()))?;
    enc.flush().map_err(|e| Error::Backend(e.to_string()))?;
    Ok(out)
}
#[cfg(not(feature = "brotli"))]
fn compress_brotli(_input: &[u8], _opt: Options) -> Result<Vec<u8>> {
    Err(Error::Unsupported("brotli"))
}

#[cfg(feature = "brotli")]
fn decompress_brotli(input: &[u8]) -> Result<Vec<u8>> {
    use std::io::Read;
    let mut r = brotli::Decompressor::new(input, 4096);
    let mut out = Vec::new();
    r.read_to_end(&mut out).map_err(|e| Error::InvalidData(e.to_string()))?;
    Ok(out)
}
#[cfg(not(feature = "brotli"))]
fn decompress_brotli(_input: &[u8]) -> Result<Vec<u8>> {
    Err(Error::Unsupported("brotli"))
}

/* ------------------------------ Backends: XZ ---------------------------- */

#[cfg(feature = "xz")]
fn compress_xz(input: &[u8], opt: Options) -> Result<Vec<u8>> {
    use std::io::Write;
    let mut out = Vec::new();
    {
        let mut w = xz2::write::XzEncoder::new(&mut out, opt.level.max(0) as u32);
        w.write_all(input).map_err(|e| Error::Backend(e.to_string()))?;
        w.finish().map_err(|e| Error::Backend(e.to_string()))?;
    }
    Ok(out)
}
#[cfg(not(feature = "xz"))]
fn compress_xz(_input: &[u8], _opt: Options) -> Result<Vec<u8>> {
    Err(Error::Unsupported("xz"))
}

#[cfg(feature = "xz")]
fn decompress_xz(input: &[u8]) -> Result<Vec<u8>> {
    use std::io::Read;
    let mut r = xz2::read::XzDecoder::new(input);
    let mut out = Vec::new();
    r.read_to_end(&mut out).map_err(|e| Error::InvalidData(e.to_string()))?;
    Ok(out)
}
#[cfg(not(feature = "xz"))]
fn decompress_xz(_input: &[u8]) -> Result<Vec<u8>> {
    Err(Error::Unsupported("xz"))
}

/* ---------------------------- Backends: Zopfli -------------------------- */

#[cfg(feature = "zopfli")]
fn compress_zopfli(input: &[u8], flavor: DeflateFlavor, _opt: Options) -> Result<Vec<u8>> {
    // Zopfli focuses on gzip/deflate. zlib not directly supported. For zlib, fall back to deflate if compiled.
    match flavor {
        DeflateFlavor::Raw => {
            let mut out = Vec::new();
            zopfli::compress(
                &zopfli::Options::default(),
                &zopfli::Format::Deflate,
                input,
                &mut out,
            )
            .map_err(|e| Error::Backend(e.to_string()))?;
            Ok(out)
        }
        DeflateFlavor::Gzip => {
            let mut out = Vec::new();
            zopfli::compress(&zopfli::Options::default(), &zopfli::Format::Gzip, input, &mut out)
                .map_err(|e| Error::Backend(e.to_string()))?;
            Ok(out)
        }
        DeflateFlavor::Zlib => {
            #[cfg(feature = "deflate")]
            {
                // Fallback to flate2 zlib when zopfli not available for zlib.
                return compress_deflate(input, DeflateFlavor::Zlib, Options::default());
            }
            #[cfg(not(feature = "deflate"))]
            {
                return Err(Error::Unsupported("zopfli zlib (fallback needs deflate)"));
            }
        }
    }
}
#[cfg(not(feature = "zopfli"))]
fn compress_zopfli(_input: &[u8], _flavor: DeflateFlavor, _opt: Options) -> Result<Vec<u8>> {
    Err(Error::Unsupported("zopfli"))
}

/* ------------------------------- I/O sync ------------------------------- */

#[cfg(all(feature = "std", feature = "io"))]
mod io_sync {
    use super::*;
    use std::io::{Read, Write};

    /// Read all from `r`, compress, write all to `w`.
    pub fn compress_to<R: Read, W: Write>(
        mut r: R,
        mut w: W,
        algo: Algo,
        opt: Options,
    ) -> Result<()> {
        let mut buf = Vec::new();
        r.read_to_end(&mut buf).map_err(|e| Error::Backend(e.to_string()))?;
        let out = super::compress(&buf, algo, opt)?;
        w.write_all(&out).map_err(|e| Error::Backend(e.to_string()))
    }

    /// Read all, decompress, write all.
    pub fn decompress_to<R: Read, W: Write>(mut r: R, mut w: W, algo: Algo) -> Result<()> {
        let mut buf = Vec::new();
        r.read_to_end(&mut buf).map_err(|e| Error::Backend(e.to_string()))?;
        let out = super::decompress(&buf, algo)?;
        w.write_all(&out).map_err(|e| Error::Backend(e.to_string()))
    }
}
#[cfg(all(feature = "std", feature = "io"))]
pub use io_sync::*;

/* ------------------------------- I/O async ------------------------------ */

#[cfg(all(feature = "async-io", feature = "std"))]
mod io_async {
    use super::*;
    use tokio::io::{AsyncRead, AsyncReadExt, AsyncWrite, AsyncWriteExt};

    /// Read all from `r`, compress, write all to `w`. Not streaming.
    pub async fn compress_to_async<R: AsyncRead + Unpin, W: AsyncWrite + Unpin>(
        mut r: R,
        mut w: W,
        algo: Algo,
        opt: Options,
    ) -> Result<()> {
        let mut buf = Vec::new();
        r.read_to_end(&mut buf).await.map_err(|e| Error::Backend(e.to_string()))?;
        let out = super::compress(&buf, algo, opt)?;
        w.write_all(&out).await.map_err(|e| Error::Backend(e.to_string()))
    }

    /// Read all, decompress, write all. Not streaming.
    pub async fn decompress_to_async<R: AsyncRead + Unpin, W: AsyncWrite + Unpin>(
        mut r: R,
        mut w: W,
        algo: Algo,
    ) -> Result<()> {
        let mut buf = Vec::new();
        r.read_to_end(&mut buf).await.map_err(|e| Error::Backend(e.to_string()))?;
        let out = super::decompress(&buf, algo)?;
        w.write_all(&out).await.map_err(|e| Error::Backend(e.to_string()))
    }
}
#[cfg(all(feature = "async-io", feature = "std"))]
pub use io_async::*;

/* --------------------------------- Tests -------------------------------- */

#[cfg(any(test, feature = "std"))]
mod tests {
    #[allow(unused_imports)]
    use super::*;

    #[test]
    #[cfg(feature = "zstd")]
    fn roundtrip_zstd() {
        let data = b"The quick brown fox jumps over the lazy dog";
        let c = compress(data, Algo::Zstd, Options { level: 5, ..Default::default() }).unwrap();
        let d = decompress(&c, Algo::Zstd).unwrap();
        assert_eq!(&d, data);
    }

    #[test]
    #[cfg(all(feature = "deflate"))]
    fn roundtrip_gzip() {
        let data = b"abcdefghabcdefghabcdefghabcdefghabcdefgh";
        let c = compress(
            data,
            Algo::Deflate(DeflateFlavor::Gzip),
            Options { level: 6, ..Default::default() },
        )
        .unwrap();
        assert!(matches!(sniff_algo(&c), Some(Algo::Deflate(DeflateFlavor::Gzip))));
        let d = decompress(&c, Algo::Deflate(DeflateFlavor::Gzip)).unwrap();
        assert_eq!(&d, data);
    }

    #[test]
    #[cfg(feature = "lz4")]
    fn roundtrip_lz4() {
        let data = vec![123u8; 4096];
        let c = compress(&data, Algo::Lz4, Options::default()).unwrap();
        let d = decompress(&c, Algo::Lz4).unwrap();
        assert_eq!(d, data);
    }

    #[test]
    #[cfg(feature = "brotli")]
    fn roundtrip_brotli() {
        let data = b"brotli works over repetitive data brotli works";
        let c = compress(data, Algo::Brotli, Options { level: 5, brotli_window_bits: 20 }).unwrap();
        let d = decompress(&c, Algo::Brotli).unwrap();
        assert_eq!(&d, data);
    }

    #[test]
    #[cfg(feature = "xz")]
    fn roundtrip_xz() {
        let data = b"xz sample";
        let c = compress(data, Algo::Xz, Options { level: 3, ..Default::default() }).unwrap();
        let d = decompress(&c, Algo::Xz).unwrap();
        assert_eq!(&d, data);
    }
}

/* ------------------------------ Mini write_all -------------------------- */

#[cfg(all(feature = "deflate", not(feature = "std")))]
mod write_all_no_std {
    use core::fmt;
    // flate2's writer path requires std::io::Write; this shim only compiles with std.
    // When no_std, deflate backend via writers is not compiled anyway.
    pub trait WriteAll {
        fn write_all(&mut self, _b: &[u8]) -> fmt::Result {
            Ok(())
        }
        fn flush(&mut self) -> fmt::Result {
            Ok(())
        }
    }
}
