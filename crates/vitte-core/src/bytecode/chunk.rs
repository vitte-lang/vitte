#![cfg_attr(not(feature = "std"), no_std)]

//! Core bytecode structures plus a compact binary roundtrip implementation.

use crate::crc32_ieee;

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};
#[cfg(feature = "std")]
use std::{string::String, vec::Vec};
#[cfg(not(feature = "std"))]
extern crate alloc;

use core::{fmt, hash::{Hash, Hasher}, mem, ops::Range, slice};

const MAGIC: &[u8; 4] = b"CHNK";
const CHUNK_VERSION: u16 = 1;

const CONST_TAG_NULL: u8 = 0x00;
const CONST_TAG_STR: u8 = 0x01;
const CONST_TAG_I64: u8 = 0x02;
const CONST_TAG_F64: u8 = 0x03;
const CONST_TAG_BOOL: u8 = 0x04;
const CONST_TAG_BYTES: u8 = 0x05;

const OP_LOAD_CONST: u8 = 0x10;
const OP_PRINT: u8 = 0x11;
const OP_RETURN: u8 = 0x12;

/// Flags stored alongside a chunk (mirrors the legacy API).
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct ChunkFlags {
    /// True if the chunk has been stripped of debug payloads.
    pub stripped: bool,
}

/// Lightweight debug payload kept for tooling convenience.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct DebugInfo {
    /// Logical main file for diagnostics.
    pub main_file: Option<String>,
    /// Additional files referenced by the chunk.
    pub files: Vec<String>,
    /// Symbol table `(name, pc)`.
    pub symbols: Vec<(String, u32)>,
}

/// Values that can live in the constant pool.
#[derive(Debug, Clone)]
pub enum ConstValue {
    /// Null literal.
    Null,
    /// Boolean literal.
    Bool(bool),
    /// 64-bit signed integer.
    I64(i64),
    /// 64-bit floating point number.
    F64(f64),
    /// UTF-8 string constant.
    Str(String),
    /// Raw byte blob.
    Bytes(Vec<u8>),
}

impl PartialEq for ConstValue {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (ConstValue::Null, ConstValue::Null) => true,
            (ConstValue::Bool(a), ConstValue::Bool(b)) => a == b,
            (ConstValue::I64(a), ConstValue::I64(b)) => a == b,
            (ConstValue::F64(a), ConstValue::F64(b)) => a.to_bits() == b.to_bits(),
            (ConstValue::Str(a), ConstValue::Str(b)) => a == b,
            (ConstValue::Bytes(a), ConstValue::Bytes(b)) => a == b,
            _ => false,
        }
    }
}

impl Eq for ConstValue {}

impl Hash for ConstValue {
    fn hash<H: Hasher>(&self, state: &mut H) {
        mem::discriminant(self).hash(state);
        match self {
            ConstValue::Null => {}
            ConstValue::Bool(b) => b.hash(state),
            ConstValue::I64(v) => v.hash(state),
            ConstValue::F64(v) => v.to_bits().hash(state),
            ConstValue::Str(s) => s.hash(state),
            ConstValue::Bytes(bytes) => bytes.hash(state),
        }
    }
}

/// Constant pool with stable indices (0-based).
#[derive(Debug, Clone, Default, PartialEq)]
pub struct ConstPool {
    values: Vec<ConstValue>,
}

impl ConstPool {
    /// Create an empty pool.
    pub fn new() -> Self { Self { values: Vec::new() } }

    /// Number of stored constants.
    pub fn len(&self) -> usize { self.values.len() }

    /// Whether the pool is empty.
    pub fn is_empty(&self) -> bool { self.values.is_empty() }

    /// Iterate as `(index, &ConstValue)`.
    pub fn iter(&self) -> ConstIter<'_> { ConstIter { inner: self.values.iter().enumerate() } }

    /// Pushes a value and returns its index.
    pub fn add(&mut self, value: ConstValue) -> u32 {
        let idx = self.values.len() as u32;
        self.values.push(value);
        idx
    }

    /// Lookup a constant by index.
    pub fn get(&self, idx: u32) -> Option<&ConstValue> { self.values.get(idx as usize) }

    /// Remove all constants.
    pub fn clear(&mut self) { self.values.clear(); }
}

/// Iterator returned by [`ConstPool::iter`].
pub struct ConstIter<'a> {
    inner: core::iter::Enumerate<slice::Iter<'a, ConstValue>>,
}

impl<'a> Iterator for ConstIter<'a> {
    type Item = (u32, &'a ConstValue);

    fn next(&mut self) -> Option<Self::Item> {
        self.inner.next().map(|(idx, value)| (idx as u32, value))
    }
}

impl<'a> IntoIterator for &'a ConstPool {
    type Item = (u32, &'a ConstValue);
    type IntoIter = ConstIter<'a>;

    fn into_iter(self) -> Self::IntoIter { self.iter() }
}

/// Line table (pc → source line) with contiguous range iteration helpers.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct LineTable {
    lines: Vec<u32>,
}

impl LineTable {
    /// Create an empty line table.
    pub fn new() -> Self { Self { lines: Vec::new() } }
    /// Append a mapping for the given program counter.
    pub fn push(&mut self, line: u32) { self.lines.push(line); }
    /// Number of stored line entries.
    pub fn len(&self) -> usize { self.lines.len() }
    /// Resolve the source line associated with a program counter.
    pub fn line_for_pc(&self, pc: u32) -> u32 { self.lines.get(pc as usize).copied().unwrap_or_default() }
    /// Iterate over contiguous ranges of the same line number.
    pub fn iter_ranges(&self) -> LineRangeIter<'_> { LineRangeIter { lines: &self.lines, index: 0 } }
    /// Remove all recorded mappings.
    pub fn clear(&mut self) { self.lines.clear(); }
    fn as_slice(&self) -> &[u32] { &self.lines }
}

/// Iterator yielding contiguous line ranges `(start..end, line)`.
pub struct LineRangeIter<'a> {
    lines: &'a [u32],
    index: usize,
}

impl<'a> Iterator for LineRangeIter<'a> {
    type Item = (Range<u32>, u32);

    fn next(&mut self) -> Option<Self::Item> {
        if self.index >= self.lines.len() {
            return None;
        }
        let start = self.index;
        let line = self.lines[start];
        self.index += 1;
        while self.index < self.lines.len() && self.lines[self.index] == line {
            self.index += 1;
        }
        Some((start as u32..self.index as u32, line))
    }
}

/// Minimal set of operations recognised by the chunk helpers.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Op {
    /// Push constant `u32` onto the VM stack.
    LoadConst(u32),
    /// Emit the top-of-stack value to stdout.
    Print,
    /// Exit the current chunk execution.
    Return,
}

/// Errors returned when decoding/validating a chunk.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ChunkError {
    /// Binary payload does not match the expected chunk layout.
    Format(&'static str),
    /// CRC32 mismatch detected while decoding a payload.
    HashMismatch {
        /// CRC32 recomputed from the payload.
        expected: u32,
        /// CRC32 value read from the payload.
        found: u32,
    },
}

impl fmt::Display for ChunkError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ChunkError::Format(msg) => write!(f, "format error: {msg}"),
            ChunkError::HashMismatch { expected, found } => write!(
                f,
                "hash mismatch (crc32): expected=0x{expected:08X}, found=0x{found:08X}"
            ),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ChunkError {}

/// Bytecode chunk with constants, ops, debug data and a line table.
#[derive(Debug, Clone, PartialEq)]
pub struct Chunk {
    version: u16,
    flags: ChunkFlags,
    /// Constant pool associated with the chunk.
    pub consts: ConstPool,
    /// Linear list of bytecode operations.
    pub ops: Vec<Op>,
    /// Mapping between program counters and source lines.
    pub lines: LineTable,
    /// Optional debug payload (files, symbols, etc.).
    pub debug: DebugInfo,
}

impl Default for Chunk {
    fn default() -> Self { Self::new(ChunkFlags::default()) }
}

impl Chunk {
    /// Create an empty chunk with the provided flags.
    pub fn new(flags: ChunkFlags) -> Self {
        Self {
            version: CHUNK_VERSION,
            flags,
            consts: ConstPool::new(),
            ops: Vec::new(),
            lines: LineTable::new(),
            debug: DebugInfo::default(),
        }
    }

    /// Chunk format version (independent from VITBC).
    pub fn version(&self) -> u16 { self.version }

    /// Override the stored version.
    pub fn set_version(&mut self, version: u16) { self.version = version; }

    /// Access to flags (immutable).
    pub fn flags(&self) -> &ChunkFlags { &self.flags }

    /// Mutable access to flags.
    pub fn flags_mut(&mut self) -> &mut ChunkFlags { &mut self.flags }

    /// Append a constant and return its index.
    pub fn add_const(&mut self, value: ConstValue) -> u32 { self.consts.add(value) }

    /// Push an opcode while recording its source line.
    pub fn push_op(&mut self, op: Op, line: u32) {
        self.ops.push(op);
        self.lines.push(line);
    }

    /// Simple hash (crc32 + length) used by tooling for cache invalidation.
    pub fn compute_hash(&self) -> u64 {
        let bytes = self.to_bytes();
        let crc = crc32_ieee(&bytes);
        ((crc as u64) << 32) | (bytes.len() as u64)
    }

    /// Encode the chunk to a compact binary payload.
    pub fn to_bytes(&self) -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(MAGIC);
        out.extend_from_slice(&self.version.to_le_bytes());
        out.push(if self.flags.stripped { 1 } else { 0 });
        out.push(0); // reserved/padding

        write_u32(&mut out, self.consts.len() as u32);
        for value in &self.consts.values {
            match value {
                ConstValue::Null => out.push(CONST_TAG_NULL),
                ConstValue::Str(s) => {
                    out.push(CONST_TAG_STR);
                    write_str(&mut out, s);
                }
                ConstValue::I64(v) => {
                    out.push(CONST_TAG_I64);
                    write_i64(&mut out, *v);
                }
                ConstValue::F64(v) => {
                    out.push(CONST_TAG_F64);
                    write_f64(&mut out, *v);
                }
                ConstValue::Bool(b) => {
                    out.push(CONST_TAG_BOOL);
                    out.push(if *b { 1 } else { 0 });
                }
                ConstValue::Bytes(bytes) => {
                    out.push(CONST_TAG_BYTES);
                    write_bytes(&mut out, bytes);
                }
            }
        }

        write_u32(&mut out, self.ops.len() as u32);
        for op in &self.ops {
            match *op {
                Op::LoadConst(ix) => {
                    out.push(OP_LOAD_CONST);
                    write_u32(&mut out, ix);
                }
                Op::Print => out.push(OP_PRINT),
                Op::Return => out.push(OP_RETURN),
            }
        }

        write_u32(&mut out, self.lines.len() as u32);
        for line in self.lines.as_slice() {
            write_u32(&mut out, *line);
        }

        match &self.debug.main_file {
            Some(main) => {
                out.push(1);
                write_str(&mut out, main);
            }
            None => out.push(0),
        }

        write_u32(&mut out, self.debug.files.len() as u32);
        for file in &self.debug.files {
            write_str(&mut out, file);
        }

        write_u32(&mut out, self.debug.symbols.len() as u32);
        for (name, pc) in &self.debug.symbols {
            write_str(&mut out, name);
            write_u32(&mut out, *pc);
        }

        let crc = crc32_ieee(&out);
        write_u32(&mut out, crc);
        out
    }

    /// Decode a chunk from [`Chunk::to_bytes`] output.
    pub fn from_bytes(data: &[u8]) -> Result<Self, ChunkError> {
        if data.len() < MAGIC.len() + 2 + 2 + 4 {
            return Err(ChunkError::Format("too short"));
        }

        let payload_len = data.len().checked_sub(4).ok_or(ChunkError::Format("too short"))?;
        let (payload, crc_bytes) = data.split_at(payload_len);
        let expected_crc = read_u32_from_slice(crc_bytes)?;
        let actual_crc = crc32_ieee(payload);
        if expected_crc != actual_crc {
            return Err(ChunkError::HashMismatch { expected: actual_crc, found: expected_crc });
        }

        if &payload[..MAGIC.len()] != MAGIC {
            return Err(ChunkError::Format("bad magic"));
        }
        let mut off = MAGIC.len();

        let version = read_u16(payload, &mut off)?;
        let flags = read_u8(payload, &mut off)?;
        let _reserved = read_u8(payload, &mut off)?;

        let mut chunk = Chunk::new(ChunkFlags { stripped: flags & 1 == 1 });
        chunk.version = version;

        let const_count = read_u32(payload, &mut off)? as usize;
        for _ in 0..const_count {
            let tag = read_u8(payload, &mut off)?;
            let value = match tag {
                CONST_TAG_NULL => ConstValue::Null,
                CONST_TAG_STR => ConstValue::Str(read_string(payload, &mut off)?),
                CONST_TAG_I64 => ConstValue::I64(read_i64(payload, &mut off)?),
                CONST_TAG_F64 => ConstValue::F64(read_f64(payload, &mut off)?),
                CONST_TAG_BOOL => {
                    let b = read_u8(payload, &mut off)?;
                    ConstValue::Bool(match b {
                        0 => false,
                        1 => true,
                        _ => return Err(ChunkError::Format("invalid bool")),
                    })
                }
                CONST_TAG_BYTES => ConstValue::Bytes(read_vec(payload, &mut off)?),
                _ => return Err(ChunkError::Format("unknown const tag")),
            };
            chunk.consts.add(value);
        }

        let op_count = read_u32(payload, &mut off)? as usize;
        for _ in 0..op_count {
            let tag = read_u8(payload, &mut off)?;
            let op = match tag {
                OP_LOAD_CONST => Op::LoadConst(read_u32(payload, &mut off)?),
                OP_PRINT => Op::Print,
                OP_RETURN => Op::Return,
                _ => return Err(ChunkError::Format("unknown op tag")),
            };
            chunk.ops.push(op);
        }

        let line_count = read_u32(payload, &mut off)? as usize;
        if line_count != chunk.ops.len() {
            return Err(ChunkError::Format("line/op length mismatch"));
        }
        for _ in 0..line_count {
            chunk.lines.push(read_u32(payload, &mut off)?);
        }

        let has_main = read_u8(payload, &mut off)?;
        if has_main > 1 {
            return Err(ChunkError::Format("invalid main_file flag"));
        }
        if has_main == 1 {
            chunk.debug.main_file = Some(read_string(payload, &mut off)?);
        }

        let file_count = read_u32(payload, &mut off)? as usize;
        for _ in 0..file_count {
            chunk.debug.files.push(read_string(payload, &mut off)?);
        }

        let symbol_count = read_u32(payload, &mut off)? as usize;
        for _ in 0..symbol_count {
            let name = read_string(payload, &mut off)?;
            let pc = read_u32(payload, &mut off)?;
            chunk.debug.symbols.push((name, pc));
        }

        if off != payload.len() {
            return Err(ChunkError::Format("trailing bytes"));
        }

        Ok(chunk)
    }
}

fn write_u32(buf: &mut Vec<u8>, v: u32) {
    buf.extend_from_slice(&v.to_le_bytes());
}

fn write_i64(buf: &mut Vec<u8>, v: i64) {
    buf.extend_from_slice(&v.to_le_bytes());
}

fn write_f64(buf: &mut Vec<u8>, v: f64) {
    buf.extend_from_slice(&v.to_le_bytes());
}

fn write_str(buf: &mut Vec<u8>, s: &str) {
    let bytes = s.as_bytes();
    write_u32(buf, bytes.len() as u32);
    buf.extend_from_slice(bytes);
}

fn write_bytes(buf: &mut Vec<u8>, bytes: &[u8]) {
    write_u32(buf, bytes.len() as u32);
    buf.extend_from_slice(bytes);
}

fn read_u8(data: &[u8], off: &mut usize) -> Result<u8, ChunkError> {
    if *off >= data.len() {
        return Err(ChunkError::Format("u8 eof"));
    }
    let v = data[*off];
    *off += 1;
    Ok(v)
}

fn read_u16(data: &[u8], off: &mut usize) -> Result<u16, ChunkError> {
    let bytes = read_exact(data, off, 2)?;
    Ok(u16::from_le_bytes([bytes[0], bytes[1]]))
}

fn read_u32(data: &[u8], off: &mut usize) -> Result<u32, ChunkError> {
    let bytes = read_exact(data, off, 4)?;
    Ok(u32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]))
}

fn read_i64(data: &[u8], off: &mut usize) -> Result<i64, ChunkError> {
    let bytes = read_exact(data, off, 8)?;
    Ok(i64::from_le_bytes([
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5], bytes[6], bytes[7],
    ]))
}

fn read_f64(data: &[u8], off: &mut usize) -> Result<f64, ChunkError> {
    let bytes = read_exact(data, off, 8)?;
    Ok(f64::from_bits(u64::from_le_bytes([
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5], bytes[6], bytes[7],
    ])))
}

fn read_string(data: &[u8], off: &mut usize) -> Result<String, ChunkError> {
    let len = read_u32(data, off)? as usize;
    let bytes = read_exact(data, off, len)?;
    let s = core::str::from_utf8(bytes).map_err(|_| ChunkError::Format("utf8"))?;
    Ok(String::from(s))
}

fn read_vec(data: &[u8], off: &mut usize) -> Result<Vec<u8>, ChunkError> {
    let len = read_u32(data, off)? as usize;
    let bytes = read_exact(data, off, len)?;
    Ok(bytes.to_vec())
}

fn read_exact<'a>(data: &'a [u8], off: &mut usize, len: usize) -> Result<&'a [u8], ChunkError> {
    let end = off.checked_add(len).ok_or(ChunkError::Format("overflow"))?;
    if end > data.len() {
        return Err(ChunkError::Format("slice eof"));
    }
    let slice = &data[*off..end];
    *off = end;
    Ok(slice)
}

fn read_u32_from_slice(bytes: &[u8]) -> Result<u32, ChunkError> {
    if bytes.len() != 4 {
        return Err(ChunkError::Format("crc eof"));
    }
    Ok(u32::from_le_bytes([bytes[0], bytes[1], bytes[2], bytes[3]]))
}
