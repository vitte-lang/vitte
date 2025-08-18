//! chunk.rs — Représentation binaire d’un “chunk” de bytecode pour Vitte.
//! Dépend de serde + bincode.

use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fmt::{self, Write as _};
use std::ops::Range;
use std::time::{SystemTime, UNIX_EPOCH};

use crate::bytecode::Op;

/// Numéro de version de format de chunk.
pub const CHUNK_VERSION: u16 = 1;

/// Magic file header: b"VITC"
pub const CHUNK_MAGIC: [u8; 4] = *b"VITC";

/// Flags de chunk (réservé pour options futures).
#[derive(Debug, Clone, Copy, Default, Serialize, Deserialize, PartialEq, Eq)]
pub struct ChunkFlags {
    /// Si vrai, le chunk est “stripped” (pas d’infos debug).
    pub stripped: bool,
}

/// Valeurs constantes supportées par la VM.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum ConstValue {
    Null,
    Bool(bool),
    I64(i64),
    F64(f64),
    /// Chaînes UTF-8 (internées côté pool).
    Str(String),
    /// Données brutes (par ex. blobs, literals compilés).
    Bytes(Vec<u8>),
}

impl fmt::Display for ConstValue {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ConstValue::Null => f.write_str("null"),
            ConstValue::Bool(b) => write!(f, "{b}"),
            ConstValue::I64(i) => write!(f, "{i}"),
            ConstValue::F64(x) => {
                if x.is_nan() {
                    f.write_str("NaN")
                } else if x.is_infinite() {
                    if x.is_sign_positive() {
                        f.write_str("+Inf")
                    } else {
                        f.write_str("-Inf")
                    }
                } else {
                    write!(f, "{x}")
                }
            }
            ConstValue::Str(s) => {
                f.write_char('"')?;
                for ch in s.chars() {
                    match ch {
                        '\\' => f.write_str("\\\\")?,
                        '"' => f.write_str("\\\"")?,
                        '\n' => f.write_str("\\n")?,
                        '\r' => f.write_str("\\r")?,
                        '\t' => f.write_str("\\t")?,
                        c if c.is_control() => write!(f, "\\u{{{:x}}}", c as u32)?,
                        c => f.write_char(c)?,
                    }
                }
                f.write_char('"')
            }
            ConstValue::Bytes(b) => write!(f, "bytes[{}]", b.len()),
        }
    }
}

/// Pool de constantes avec dé-dupe basique des chaînes.
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct ConstPool {
    pub(crate) values: Vec<ConstValue>,
    #[serde(skip)]
    str_index: HashMap<String, u32>,
}

impl ConstPool {
    pub fn new() -> Self {
        Self { values: Vec::new(), str_index: HashMap::new() }
    }

    pub fn add(&mut self, v: ConstValue) -> u32 {
        match &v {
            ConstValue::Str(s) => {
                if let Some(&idx) = self.str_index.get(s) {
                    return idx;
                }
                let idx = self.push_raw(v);
                self.str_index.insert(s.clone(), idx);
                idx
            }
            _ => self.push_raw(v),
        }
    }

    fn push_raw(&mut self, v: ConstValue) -> u32 {
        let idx = self.values.len() as u32;
        self.values.push(v);
        idx
    }

    pub fn get(&self, idx: u32) -> Option<&ConstValue> {
        self.values.get(idx as usize)
    }

    pub fn len(&self) -> usize {
        self.values.len()
    }

    pub fn is_empty(&self) -> bool {
        self.values.is_empty()
    }

    pub fn iter(&self) -> impl Iterator<Item = (u32, &ConstValue)> {
        self.values.iter().enumerate().map(|(i, v)| (i as u32, v))
    }

    fn values_as_view(&self) -> ConstPoolView<'_> {
        ConstPoolView { values: &self.values }
    }
}

/// Entrée compressée de la table de lignes (RLE).
#[derive(Debug, Clone, Serialize, Deserialize)]
struct LineRun {
    start_pc: u32,
    line: u32,
    len: u32,
}

/// Table des lignes : map PC -> line via segments RLE.
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct LineTable {
    runs: Vec<LineRun>,
}

impl LineTable {
    pub fn new() -> Self {
        Self { runs: Vec::new() }
    }

    pub fn push_line(&mut self, pc: u32, line: u32) {
        match self.runs.last_mut() {
            Some(last) if last.line == line && last.start_pc + last.len == pc => last.len += 1,
            _ => self.runs.push(LineRun { start_pc: pc, line, len: 1 }),
        }
    }

    pub fn line_for_pc(&self, pc: u32) -> Option<u32> {
        for run in &self.runs {
            if pc >= run.start_pc && pc < run.start_pc + run.len {
                return Some(run.line);
            }
        }
        None
    }

    pub fn runs(&self) -> &[LineRun] { &self.runs }

    pub fn iter_ranges(&self) -> impl Iterator<Item = (Range<u32>, u32)> + '_ {
        self.runs.iter().map(|r| (r.start_pc..(r.start_pc + r.len), r.line))
    }

    pub fn is_empty(&self) -> bool { self.runs.is_empty() }
}

/// Informations de debug optionnelles.
#[derive(Debug, Clone, Default, Serialize, Deserialize)]
pub struct DebugInfo {
    pub main_file: Option<String>,
    pub files: Vec<String>,
    pub symbols: Vec<(String, u32)>,
}

/// En-tête de chunk, séparé pour contrôle d’intégrité.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct ChunkHeader {
    magic: [u8; 4],
    version: u16,
    flags: ChunkFlags,
    created_unix_secs: u64,
    hash_fnv1a_64: u64,
}

/// Le chunk complet : ops + constantes + métadonnées.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Chunk {
    header: ChunkHeader,
    pub ops: Vec<Op>,
    pub consts: ConstPool,
    pub lines: LineTable,
    pub debug: DebugInfo,
}

impl Chunk {
    pub fn new(flags: ChunkFlags) -> Self {
        Self {
            header: ChunkHeader {
                magic: CHUNK_MAGIC,
                version: CHUNK_VERSION,
                flags,
                created_unix_secs: now_unix(),
                hash_fnv1a_64: 0,
            },
            ops: Vec::new(),
            consts: ConstPool::new(),
            lines: LineTable::new(),
            debug: DebugInfo::default(),
        }
    }

    pub fn version(&self) -> u16 { self.header.version }
    pub fn flags(&self) -> ChunkFlags { self.header.flags }

    pub fn push_op(&mut self, op: Op, line: Option<u32>) -> u32 {
        let pc = self.ops.len() as u32;
        self.ops.push(op);
        if let Some(l) = line { self.lines.push_line(pc, l); }
        pc
    }

    pub fn add_const(&mut self, v: ConstValue) -> u32 { self.consts.add(v) }
    pub fn const_at(&self, idx: u32) -> Option<&ConstValue> { self.consts.get(idx) }
    pub fn len(&self) -> usize { self.ops.len() }
    pub fn is_empty(&self) -> bool { self.ops.is_empty() }

    pub fn compute_hash(&self) -> u64 {
        let mut hasher = Fnv1a64::new();
        fn feed_ser<T: serde::Serialize>(h: &mut Fnv1a64, v: &T) {
            let bytes = bincode::DefaultOptions::new()
                .with_fixint_encoding()
                .with_little_endian()
                .serialize(v)
                .expect("serialize ok");
            h.write(&bytes);
        }
        feed_ser(&mut hasher, &self.ops);
        feed_ser(&mut hasher, &self.consts.values_as_view());
        feed_ser(&mut hasher, &self.lines);
        let debug_wo_files_times = (&self.debug.files, &self.debug.symbols, &self.debug.main_file);
        feed_ser(&mut hasher, &debug_wo_files_times);
        hasher.finish()
    }

    fn finalize_header(&mut self) {
        self.header.created_unix_secs = now_unix();
        self.header.hash_fnv1a_64 = self.compute_hash();
    }

    pub fn to_bytes(&mut self) -> Vec<u8> {
        self.finalize_header();
        bincode::DefaultOptions::new()
            .with_fixint_encoding()
            .with_little_endian()
            .serialize(self)
            .expect("serialize chunk")
    }

    pub fn from_bytes(bytes: &[u8]) -> Result<Self, ChunkLoadError> {
        let mut chunk: Self = bincode::DefaultOptions::new()
            .with_fixint_encoding()
            .with_little_endian()
            .deserialize(bytes)
            .map_err(ChunkLoadError::Bincode)?;

        if chunk.header.magic != CHUNK_MAGIC {
            return Err(ChunkLoadError::BadMagic(chunk.header.magic));
        }
        if chunk.header.version != CHUNK_VERSION {
            return Err(ChunkLoadError::BadVersion { expected: CHUNK_VERSION, found: chunk.header.version });
        }

        chunk.rebuild_string_index();

        let expect_hash = chunk.header.hash_fnv1a_64;
        let got_hash = chunk.compute_hash();
        if expect_hash != got_hash {
            return Err(ChunkLoadError::BadHash { expected: expect_hash, found: got_hash });
        }

        Ok(chunk)
    }

    fn rebuild_string_index(&mut self) {
        self.consts.str_index.clear();
        for (i, v) in self.consts.values.iter().enumerate() {
            if let ConstValue::Str(s) = v {
                self.consts.str_index.insert(s.clone(), i as u32);
            }
        }
    }

    pub fn disassemble(&self, title: &str) -> String {
        let mut out = String::new();
        let _ = writeln!(&mut out, "== Disassemble: {title} ==");
        let _ = writeln!(
            &mut out,
            "magic={:?} version={} flags={:?} consts={} ops={}",
            self.header.magic, self.header.version, self.header.flags, self.consts.len(), self.ops.len()
        );
        let _ = writeln!(
            &mut out,
            "created_unix_secs={} hash=0x{:016x}",
            self.header.created_unix_secs, self.header.hash_fnv1a_64
        );

        if !self.consts.is_empty() {
            let _ = writeln!(&mut out, "\n# ConstPool");
            for (i, v) in self.consts.iter() {
                let _ = writeln!(&mut out, "  [{}] = {}", i, v);
            }
        }

        let _ = writeln!(&mut out, "\n# Code");
        for (pc, op) in self.ops.iter().enumerate() {
            let pc = pc as u32;
            let line = self.lines.line_for_pc(pc).unwrap_or(u32::MAX);
            let _ = writeln!(&mut out, "{pc:05}  (line {line:>4})  {}", fmt_op(op, &self.consts));
        }

        if !self.lines.is_empty() {
            let _ = writeln!(&mut out, "\n# LineTable (RLE)");
            for (range, line) in self.lines.iter_ranges() {
                let _ = writeln!(&mut out, "  pc {}..{}  -> line {}", range.start, range.end, line);
            }
        }

        if !self.debug.files.is_empty() || !self.debug.symbols.is_empty() || self.debug.main_file.is_some() {
            let _ = writeln!(&mut out, "\n# DebugInfo");
            if let Some(main) = &self.debug.main_file {
                let _ = writeln!(&mut out, "  main_file: {main}");
            }
            if !self.debug.files.is_empty() {
                let _ = writeln!(&mut out, "  files:");
                for f in &self.debug.files {
                    let _ = writeln!(&mut out, "    - {f}");
                }
            }
            if !self.debug.symbols.is_empty() {
                let _ = writeln!(&mut out, "  symbols:");
                for (name, pc) in &self.debug.symbols {
                    let _ = writeln!(&mut out, "    {name} @ pc {pc}");
                }
            }
        }

        out
    }
}

#[derive(Debug)]
pub enum ChunkLoadError {
    Bincode(bincode::Error),
    BadMagic([u8; 4]),
    BadVersion { expected: u16, found: u16 },
    BadHash { expected: u64, found: u64 },
}

impl fmt::Display for ChunkLoadError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ChunkLoadError::Bincode(e) => write!(f, "bincode: {e}"),
            ChunkLoadError::BadMagic(m) => write!(f, "mauvais magic: {:?}", m),
            ChunkLoadError::BadVersion { expected, found } => {
                write!(f, "mauvaise version: attendu {expected}, trouvé {found}")
            }
            ChunkLoadError::BadHash { expected, found } => {
                write!(f, "hash invalide: attendu 0x{expected:016x}, trouvé 0x{found:016x}")
            }
        }
    }
}
impl std::error::Error for ChunkLoadError {}

fn fmt_op(op: &Op, pool: &ConstPool) -> String {
    let s = format!("{op:?}");
    if let Some((prefix, idx_str, suffix)) = extract_const_index(&s) {
        if let Ok(idx) = idx_str.parse::<u32>() {
            if let Some(val) = pool.get(idx) {
                return format!("{prefix}{idx} /* {val} */{suffix}");
            }
        }
    }
    s
}

/// Extrait un index de constante d’une représentation Debug d’op (heuristique).
fn extract_const_index(s: &str) -> Option<(&str, &str, &str)> {
    // Cas: "OpName(123)"
    if let Some(open) = s.find('(') {
        if s.ends_with(')') {
            let idx_str = &s[open + 1..s.len() - 1];
            if idx_str.chars().all(|c| c.is_ascii_digit()) {
                return Some((&s[..open + 1], idx_str, ")"));
            }
        }
    }
    // Cas: "... idx: 123 ..."
    if let Some(pos) = s.find("idx: ") {
        let after = &s[pos + 5..];
        let end = after.chars().position(|c| !c.is_ascii_digit()).unwrap_or(after.len());
        let idx_str = &after[..end];
        let prefix = &s[..pos + 5];
        let suffix = &after[end..];
        if !idx_str.is_empty() {
            return Some((prefix, idx_str, suffix));
        }
    }
    None
}

#[derive(Serialize)]
struct ConstPoolView<'a> {
    values: &'a [ConstValue],
}

fn now_unix() -> u64 {
    SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default().as_secs()
}

#[derive(Default)]
struct Fnv1a64(u64);
impl Fnv1a64 {
    const OFFSET_BASIS: u64 = 0xcbf29ce484222325;
    const PRIME: u64 = 0x100000001b3;
    fn new() -> Self { Self(Self::OFFSET_BASIS) }
    fn write(&mut self, bytes: &[u8]) {
        let mut h = self.0;
        for &b in bytes {
            h ^= b as u64;
            h = h.wrapping_mul(Self::PRIME);
        }
        self.0 = h;
    }
    fn finish(&self) -> u64 { self.0 }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn roundtrip() {
        let mut c = Chunk::new(ChunkFlags { stripped: false });
        let k_hello = c.add_const(ConstValue::Str("hello".into()));
        let k_num = c.add_const(ConstValue::I64(42));

        // ⚠️ Ajuste ces opcodes selon TON enum Op.
        // Exemples génériques (à adapter si Op::LoadConst prend des champs nommés):
        c.push_op(Op::Nop, Some(1));
        // Si ton Op est LoadConst(idx): 
        // c.push_op(Op::LoadConst(k_hello), Some(2));
        // Si ton Op est LoadConst { dst, k }:
        // c.push_op(Op::LoadConst { dst: 0, k: k_hello }, Some(2));
        let _ = k_num; // évite warning si pas utilisé

        // Sérialisation / intégrité
        let mut bytes = c.to_bytes();
        let loaded = Chunk::from_bytes(&bytes).expect("load ok");
        assert_eq!(loaded.consts.len(), 2);

        // Corruption → hash mismatch
        bytes[bytes.len() - 1] ^= 0xFF;
        let err = Chunk::from_bytes(&bytes).unwrap_err();
        assert!(matches!(err, ChunkLoadError::BadHash { .. }));
    }
}
