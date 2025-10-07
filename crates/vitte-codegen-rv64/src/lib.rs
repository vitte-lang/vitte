//! vitte-codegen-rv64 — RV64 backend for Vitte
//!
//! Goals
//! - Deterministic encodings for RV64I/M/A/F/D (+V optional via `simd`).
//! - Clean separation: Lowering → RA → Encode → Emit → Link.
//! - No-std by default. `std` feature unlocks tests and conveniences.
//!
//! Status
//! - Implements core RV64I (+M) encodings, basic ABI (SysV), minimal RA,
//!   peephole combine, relocations, and an incremental code buffer.
//! - Floating point encodings and atomics are scaffolded.
//! - Vector (`simd`) provides type/enum stubs and a few example encodings.
//!
//! Safety
//! - Emits raw machine code. Executing JIT memory is out of scope here.
//!
//! Copyright
//! - MIT licensed (workspace).

#![cfg_attr(not(feature = "std"), no_std)]
#![forbid(unsafe_op_in_unsafe_fn)]
#![warn(missing_docs)]
#![allow(clippy::upper_case_acronyms)]

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(feature = "std")]
extern crate std as alloc_std;

#[cfg(not(feature = "std"))]
use alloc::string::String;
#[cfg(feature = "std")]
use alloc_std::string::String;

/// Result alias.
pub type Result<T, E = CodegenError> = core::result::Result<T, E>;

/// Backend error kinds.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CodegenError {
    /// Generic invalid input.
    Invalid(String),
    /// Unsupported instruction or feature.
    Unsupported(String),
    /// Buffer capacity or bounds error.
    Buf(String),
    /// Relocation failure.
    Reloc(String),
    /// Register allocation failure.
    RA(String),
    /// Lowering failure.
    Lower(String),
}

/// RV64 architectural constants.
pub mod arch {
    /// RV64 base XLEN.
    pub const XLEN: u32 = 64;
    /// Instruction width in bytes.
    pub const INSN_BYTES: usize = 4;

    /// Condition codes for branches.
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum Cc {
        /// Equal
        Eq,
        /// Not equal
        Ne,
        /// Signed less than
        Lt,
        /// Signed greater or equal
        Ge,
        /// Unsigned less than
        Ltu,
        /// Unsigned greater or equal
        Geu,
    }

    /// SystemV (lp64/lp64d) call-preserved registers on RV64.
    pub mod abi_preserved {
        use super::super::reg::X;
        /// Call-preserved integer registers as defined by the SysV ABI on RV64.
        pub const PRESERVED: &[X] = &[
            X::S0,
            X::S1,
            X::S2,
            X::S3,
            X::S4,
            X::S5,
            X::S6,
            X::S7,
            X::S8,
            X::S9,
            X::S10,
            X::S11,
        ];
    }
}

/// Integer registers.
#[allow(missing_docs)]
pub mod reg {
    /// Integer register (x0..x31).
    #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
    #[repr(u8)]
    pub enum X {
        Zero = 0,
        Ra = 1,
        Sp = 2,
        Gp = 3,
        Tp = 4,
        T0 = 5,
        T1 = 6,
        T2 = 7,
        S0 = 8,
        S1 = 9,
        A0 = 10,
        A1 = 11,
        A2 = 12,
        A3 = 13,
        A4 = 14,
        A5 = 15,
        A6 = 16,
        A7 = 17,
        S2 = 18,
        S3 = 19,
        S4 = 20,
        S5 = 21,
        S6 = 22,
        S7 = 23,
        S8 = 24,
        S9 = 25,
        S10 = 26,
        S11 = 27,
        T3 = 28,
        T4 = 29,
        T5 = 30,
        T6 = 31,
    }

    impl X {
        #[inline]
        pub fn idx(self) -> u8 {
            self as u8
        }
    }

    /// Floating point register (f0..f31).
    #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
    #[repr(u8)]
    pub enum F {
        F0 = 0,
        F1 = 1,
        F2 = 2,
        F3 = 3,
        F4 = 4,
        F5 = 5,
        F6 = 6,
        F7 = 7,
        F8 = 8,
        F9 = 9,
        F10 = 10,
        F11 = 11,
        F12 = 12,
        F13 = 13,
        F14 = 14,
        F15 = 15,
        F16 = 16,
        F17 = 17,
        F18 = 18,
        F19 = 19,
        F20 = 20,
        F21 = 21,
        F22 = 22,
        F23 = 23,
        F24 = 24,
        F25 = 25,
        F26 = 26,
        F27 = 27,
        F28 = 28,
        F29 = 29,
        F30 = 30,
        F31 = 31,
    }

    impl F {
        #[inline]
        pub fn idx(self) -> u8 {
            self as u8
        }
    }
}

/// RV64 instruction forms and opcodes.
#[allow(missing_docs)]
pub mod inst {
    use super::arch::Cc;
    use super::reg::{F, X};

    /// High-level op enumeration for the encoder.
    #[derive(Debug, Clone, PartialEq)]
    pub enum Op {
        // RV64I
        Lui { rd: X, imm20: i32 },
        Auipc { rd: X, imm20: i32 },
        Jal { rd: X, rel: i32 },
        Jalr { rd: X, rs1: X, imm12: i16 },
        Br { cc: Cc, rs1: X, rs2: X, rel: i32 },
        Load { rd: X, rs1: X, imm12: i16, w: Width },
        Store { rs2: X, rs1: X, imm12: i16, w: Width },
        OpImm { rd: X, rs1: X, kind: IKind, imm12: i16, shamt6: Option<u8> },
        Op { rd: X, rs1: X, rs2: X, kind: RKind },

        // RV64M
        Mul { rd: X, rs1: X, rs2: X, kind: MulKind },

        // System
        Ecall,
        Ebreak,

        // FP (scaffold)
        FLoad { rd: F, rs1: X, imm12: i16, w: FWidth },
        FStore { rs2: F, rs1: X, imm12: i16, w: FWidth },

        // Pseudo
        Li { rd: X, imm: i64 },
        Ret,
    }

    /// Integer width.
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum Width {
        B,
        H,
        W,
        D,
    }

    /// FP width.
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum FWidth {
        S,
        D,
    }

    /// I-type arithmetic.
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum IKind {
        Addi,
        Slti,
        Sltiu,
        Xori,
        Ori,
        Andi,
        Slli, // uses shamt
        Srli, // uses shamt
        Srai, // uses shamt
    }

    /// R-type arithmetic.
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum RKind {
        Add,
        Sub,
        Sll,
        Slt,
        Sltu,
        Xor,
        Srl,
        Sra,
        Or,
        And,
    }

    /// Multiply/divide kinds (M extension subset).
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum MulKind {
        Mul,
        Mulh,
        Mulhsu,
        Mulhu,
        Div,
        Divu,
        Rem,
        Remu,
    }
}

/// Relocation kinds for RV64.
#[allow(missing_docs)]
pub mod relocs {
    /// Relocation kinds used by the backend.
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub enum Reloc {
        /// 20-bit upper immediate (LUI/AUIPC).
        Hi20,
        /// 12-bit low immediate (ADDI/JALR/LOAD/STORE).
        Lo12I,
        /// Branch 12-bit signed PC-rel (B-type).
        Br12,
        /// JAL 20-bit signed PC-rel.
        J20,
        /// Absolute address split into HI20+LO12 via GOT/abs (linker use).
        Abs64,
    }

    /// A pending relocation reference.
    #[derive(Debug, Clone)]
    pub struct PendingReloc {
        pub kind: Reloc,
        pub at_off: usize,
        pub target_label: u32,
    }
}

/// Minimal code buffer with backpatch support.
pub mod buf {
    use super::relocs::PendingReloc;
    #[cfg(not(feature = "std"))]
    use alloc::vec::Vec;
    #[cfg(feature = "std")]
    use alloc_std::vec::Vec;

    /// Growable code buffer.
    #[derive(Debug, Default)]
    pub struct CodeBuf {
        bytes: Vec<u8>,
        relocs: Vec<PendingReloc>,
        labels: Vec<Option<usize>>,
    }

    impl CodeBuf {
        /// Create with capacity.
        pub fn with_capacity(n: usize) -> Self {
            Self { bytes: Vec::with_capacity(n), relocs: Vec::new(), labels: Vec::new() }
        }

        /// Current offset.
        #[inline]
        pub fn off(&self) -> usize {
            self.bytes.len()
        }

        /// Reserve bytes.
        pub fn reserve(&mut self, n: usize) {
            self.bytes.reserve(n);
        }

        /// Write a little-endian u32.
        pub fn put_u32(&mut self, v: u32) {
            self.bytes.extend_from_slice(&v.to_le_bytes());
        }

        /// Define a label id at current offset. Returns the label id.
        pub fn define_label(&mut self, id: u32) {
            let idx = id as usize;
            if idx >= self.labels.len() {
                self.labels.resize(idx + 1, None);
            }
            self.labels[idx] = Some(self.off());
        }

        /// Record pending relocation.
        pub fn add_reloc(&mut self, r: PendingReloc) {
            self.relocs.push(r);
        }

        /// Get raw buffer.
        pub fn bytes(&self) -> &[u8] {
            &self.bytes
        }

        /// Mut raw buffer.
        pub fn bytes_mut(&mut self) -> &mut [u8] {
            &mut self.bytes
        }

        /// Labels table.
        pub fn labels(&self) -> &[Option<usize>] {
            &self.labels
        }

        /// Relocations slice.
        pub fn relocs(&self) -> &[PendingReloc] {
            &self.relocs
        }
    }
}

#[allow(dead_code)]
mod util {
    #[inline]
    pub fn sign_extend(value: i64, bits: u8) -> i64 {
        let shift = 64 - bits as i64;
        (value << shift) >> shift
    }

    #[inline]
    pub fn u32_as_bytes_le(v: u32) -> [u8; 4] {
        v.to_le_bytes()
    }
}

/// Encoder for RV64 instruction forms.
pub mod enc {
    use super::arch::INSN_BYTES;
    use super::inst::{IKind, MulKind, Op, RKind, Width};
    use super::reg::X;

    /// Encodes a single instruction into 4 bytes (LE).
    pub fn encode(
        op: &Op,
        here: usize,
        target_off: Option<usize>,
    ) -> super::Result<[u8; INSN_BYTES]> {
        let word = match *op {
            // U-type
            Op::Lui { rd, imm20 } => u_type(0b0110111, rd, imm20)?,
            Op::Auipc { rd, imm20 } => u_type(0b0010111, rd, imm20)?,

            // J-type
            Op::Jal { rd, rel } => j_type(0b1101111, rd, rel, here, target_off)?,

            // I-type
            Op::Jalr { rd, rs1, imm12 } => i_type(0b1100111, rd, rs1, imm12, 0)?,
            Op::OpImm { rd, rs1, kind, imm12, shamt6 } => {
                encode_op_imm(rd, rs1, kind, imm12, shamt6)?
            },

            // B-type
            Op::Br { cc, rs1, rs2, rel } => b_type(cc, rs1, rs2, rel, here, target_off)?,

            // Load/Store
            Op::Load { rd, rs1, imm12, w } => load(rd, rs1, imm12, w)?,
            Op::Store { rs2, rs1, imm12, w } => store(rs2, rs1, imm12, w)?,

            // R-type
            Op::Op { rd, rs1, rs2, kind } => encode_op(rd, rs1, rs2, kind)?,

            // M
            Op::Mul { rd, rs1, rs2, kind } => encode_mul(rd, rs1, rs2, kind)?,

            // System
            Op::Ecall => 0b1110011,
            Op::Ebreak => (1 << 20) | 0b1110011,

            // FP, pseudo handled in lowering; keep placeholders
            Op::FLoad { .. } | Op::FStore { .. } | Op::Li { .. } | Op::Ret => {
                return Err(super::CodegenError::Unsupported(
                    "pseudo/FP must be lowered before encode".into(),
                ));
            },
        };
        Ok(word.to_le_bytes())
    }

    fn reg_bits(x: X) -> u32 {
        x as u32
    }

    fn i_type(op: u32, rd: X, rs1: X, imm12: i16, funct3: u32) -> super::Result<u32> {
        let imm = imm12 as i32;
        if imm < -2048 || imm > 2047 {
            return Err(super::CodegenError::Invalid("imm12 out of range".into()));
        }
        Ok(((imm as u32 & 0xFFF) << 20)
            | (reg_bits(rs1) << 15)
            | (funct3 << 12)
            | (reg_bits(rd) << 7)
            | op)
    }

    fn u_type(op: u32, rd: X, imm20: i32) -> super::Result<u32> {
        if imm20 < -0x80000 || imm20 > 0x7FFFF {
            return Err(super::CodegenError::Invalid("imm20 out of range".into()));
        }
        Ok(((imm20 as u32 & 0xFFFFF) << 12) | (reg_bits(rd) << 7) | op)
    }

    fn s_type(op: u32, rs2: X, rs1: X, imm12: i16, funct3: u32) -> super::Result<u32> {
        let imm = imm12 as i32;
        if !(-2048..=2047).contains(&imm) {
            return Err(super::CodegenError::Invalid("imm12 out of range".into()));
        }
        let imm_u = imm as u32;
        let imm_11_5 = (imm_u >> 5) & 0x7F;
        let imm_4_0 = imm_u & 0x1F;
        Ok((imm_11_5 << 25)
            | (reg_bits(rs2) << 20)
            | (reg_bits(rs1) << 15)
            | (funct3 << 12)
            | (imm_4_0 << 7)
            | op)
    }

    fn b_type(
        cc: super::arch::Cc,
        rs1: X,
        rs2: X,
        rel: i32,
        here: usize,
        target: Option<usize>,
    ) -> super::Result<u32> {
        let op = 0b1100011;
        let funct3 = match cc {
            super::arch::Cc::Eq => 0b000,
            super::arch::Cc::Ne => 0b001,
            super::arch::Cc::Lt => 0b100,
            super::arch::Cc::Ge => 0b101,
            super::arch::Cc::Ltu => 0b110,
            super::arch::Cc::Geu => 0b111,
        };
        let pc_off = if let Some(t) = target { (t as isize - here as isize) as i32 } else { rel };
        // B-type offset is multiples of 2, 13-bit signed.
        if pc_off % 2 != 0 {
            return Err(super::CodegenError::Invalid("branch offset not aligned".into()));
        }
        let imm = pc_off;
        if imm < -4096 || imm > 4094 {
            return Err(super::CodegenError::Invalid("branch rel out of range".into()));
        }
        let imm_u = (imm as u32) >> 1;
        let imm_12 = (imm_u >> 11) & 0x1;
        let imm_10_5 = (imm_u >> 5) & 0x3F;
        let imm_4_1 = (imm_u >> 1) & 0xF;
        let imm_11 = imm_u & 0x1;

        Ok((imm_12 << 31)
            | (imm_10_5 << 25)
            | ((rs2 as u32) << 20)
            | ((rs1 as u32) << 15)
            | (funct3 << 12)
            | (imm_4_1 << 8)
            | (imm_11 << 7)
            | op)
    }

    fn j_type(op: u32, rd: X, rel: i32, here: usize, target: Option<usize>) -> super::Result<u32> {
        let pc_off = if let Some(t) = target { (t as isize - here as isize) as i32 } else { rel };
        if pc_off % 2 != 0 {
            return Err(super::CodegenError::Invalid("jal offset not aligned".into()));
        }
        // 21-bit signed, multiples of 2.
        if pc_off < -1_048_576 || pc_off > 1_048_574 {
            return Err(super::CodegenError::Invalid("jal rel out of range".into()));
        }
        let imm = (pc_off as u32) >> 1;
        let imm_20 = (imm >> 19) & 0x1;
        let imm_10_1 = (imm >> 9) & 0x3FF;
        let imm_11 = (imm >> 8) & 0x1;
        let imm_19_12 = imm & 0xFF;

        Ok((imm_20 << 31)
            | (imm_19_12 << 12)
            | (imm_11 << 20)
            | (imm_10_1 << 21)
            | ((rd as u32) << 7)
            | op)
    }

    fn load(rd: X, rs1: X, imm12: i16, w: Width) -> super::Result<u32> {
        let funct3 = match w {
            Width::B => 0b000,
            Width::H => 0b001,
            Width::W => 0b010,
            Width::D => 0b011,
        };
        i_type(0b0000011, rd, rs1, imm12, funct3)
    }

    fn store(rs2: X, rs1: X, imm12: i16, w: Width) -> super::Result<u32> {
        let funct3 = match w {
            Width::B => 0b000,
            Width::H => 0b001,
            Width::W => 0b010,
            Width::D => 0b011,
        };
        s_type(0b0100011, rs2, rs1, imm12, funct3)
    }

    fn encode_op_imm(
        rd: X,
        rs1: X,
        kind: IKind,
        imm12: i16,
        shamt6: Option<u8>,
    ) -> super::Result<u32> {
        let (funct3, sh_op, funct6) = match kind {
            IKind::Addi => (0b000, false, 0),
            IKind::Slti => (0b010, false, 0),
            IKind::Sltiu => (0b011, false, 0),
            IKind::Xori => (0b100, false, 0),
            IKind::Ori => (0b110, false, 0),
            IKind::Andi => (0b111, false, 0),
            IKind::Slli => (0b001, true, 0b000000),
            IKind::Srli => (0b101, true, 0b000000),
            IKind::Srai => (0b101, true, 0b010000),
        };
        if sh_op {
            let sh = shamt6.ok_or_else(|| super::CodegenError::Invalid("missing shamt".into()))?;
            if sh > 63 {
                return Err(super::CodegenError::Invalid("shamt out of range".into()));
            }
            let op = 0b0010011;
            Ok(((funct6 as u32) << 26)
                | ((sh as u32 & 0x3F) << 20)
                | ((rs1 as u32) << 15)
                | ((funct3 as u32) << 12)
                | ((rd as u32) << 7)
                | op)
        } else {
            i_type(0b0010011, rd, rs1, imm12, funct3)
        }
    }

    fn encode_op(rd: X, rs1: X, rs2: X, kind: RKind) -> super::Result<u32> {
        let (funct7, funct3) = match kind {
            RKind::Add => (0b0000000, 0b000),
            RKind::Sub => (0b0100000, 0b000),
            RKind::Sll => (0b0000000, 0b001),
            RKind::Slt => (0b0000000, 0b010),
            RKind::Sltu => (0b0000000, 0b011),
            RKind::Xor => (0b0000000, 0b100),
            RKind::Srl => (0b0000000, 0b101),
            RKind::Sra => (0b0100000, 0b101),
            RKind::Or => (0b0000000, 0b110),
            RKind::And => (0b0000000, 0b111),
        };
        Ok((funct7 << 25)
            | ((rs2 as u32) << 20)
            | ((rs1 as u32) << 15)
            | (funct3 << 12)
            | ((rd as u32) << 7)
            | 0b0110011)
    }

    fn encode_mul(rd: X, rs1: X, rs2: X, kind: MulKind) -> super::Result<u32> {
        let funct3 = match kind {
            MulKind::Mul => 0b000,
            MulKind::Mulh => 0b001,
            MulKind::Mulhsu => 0b010,
            MulKind::Mulhu => 0b011,
            MulKind::Div => 0b100,
            MulKind::Divu => 0b101,
            MulKind::Rem => 0b110,
            MulKind::Remu => 0b111,
        };
        Ok((0b0000001 << 25)
            | ((rs2 as u32) << 20)
            | ((rs1 as u32) << 15)
            | (funct3 << 12)
            | ((rd as u32) << 7)
            | 0b0110011)
    }
}

/// SysV ABI helpers (RV64, lp64/lp64d).
pub mod abi {
    use super::reg::X;

    /// Argument registers a0..a7.
    pub const ARGS: [X; 8] = [X::A0, X::A1, X::A2, X::A3, X::A4, X::A5, X::A6, X::A7];

    /// Return registers a0..a1.
    pub const RETS: [X; 2] = [X::A0, X::A1];

    /// Stack pointer register.
    pub const SP: X = X::Sp;

    /// Return address register.
    pub const RA: X = X::Ra;

    /// Frame pointer (s0/fp).
    pub const FP: X = X::S0;

    /// Stack alignment in bytes.
    pub const STACK_ALIGN: usize = 16;
}

/// Simple peephole optimizer.
pub mod peephole {
    use super::inst::{IKind, Op, RKind};
    use super::reg::X;
    #[cfg(not(feature = "std"))]
    use alloc::vec::Vec;
    #[cfg(feature = "std")]
    use alloc_std::vec::Vec;

    /// Run simple canonicalization and elimination.
    pub fn run(seq: &mut Vec<Op>) {
        let mut out = Vec::with_capacity(seq.len());
        let mut i = 0;
        while i < seq.len() {
            let cur = &seq[i];
            if let Op::OpImm { rs1, kind: IKind::Addi, imm12, .. } = *cur {
                if rs1 == X::Zero && imm12 == 0 {
                    out.push((*cur).clone());
                    i += 1;
                    continue;
                }
            }
            if let Op::Op { rd, rs1, rs2, kind: RKind::Sub } = *cur {
                if rs2 == X::Zero {
                    out.push(Op::Op { rd, rs1, rs2: X::Zero, kind: RKind::Add });
                    i += 1;
                    continue;
                }
            }
            out.push((*cur).clone());
            i += 1;
        }
        *seq = out;
    }
}

/// Trivial linear-scan RA placeholder.
pub mod ra {
    use super::reg::X;
    #[cfg(not(feature = "std"))]
    use alloc::{collections::BTreeMap, vec, vec::Vec};
    #[cfg(feature = "std")]
    use alloc_std::{collections::BTreeMap, vec, vec::Vec};

    /// Virtual register id.
    #[derive(Debug, Copy, Clone, PartialEq, Eq, Hash, PartialOrd, Ord)]
    pub struct VReg(pub u32);

    /// Mapping vregs to physical X registers.
    #[derive(Debug, Default)]
    pub struct Alloc {
        map: BTreeMap<VReg, X>,
        pool: Vec<X>,
    }

    impl Alloc {
        /// Create with a caller-saved pool by default.
        pub fn new() -> Self {
            use X::*;
            let pool = vec![T0, T1, T2, T3, T4, T5, T6, A0, A1, A2, A3, A4, A5, A6, A7];
            Self { map: Default::default(), pool }
        }

        /// Assign or reuse a mapping.
        pub fn get(&mut self, v: VReg) -> super::Result<X> {
            if let Some(x) = self.map.get(&v).copied() {
                return Ok(x);
            }
            let x = self
                .pool
                .pop()
                .ok_or_else(|| super::CodegenError::RA("no registers available".into()))?;
            self.map.insert(v, x);
            Ok(x)
        }
    }
}

/// Lowering from a hypothetical Vitte IR. The real project should plug actual IR nodes.
#[allow(missing_docs)]
pub mod lower {
    use super::inst::{IKind, Op, RKind, Width};
    use super::reg::X;
    #[cfg(not(feature = "std"))]
    use alloc::vec::Vec;
    #[cfg(feature = "std")]
    use alloc_std::vec::Vec;

    /// Minimal IR opcodes for demonstration.
    #[derive(Debug, Clone, PartialEq)]
    pub enum Ir {
        /// rd = const imm64
        Const { rd: X, imm: i64 },
        /// rd = add rs1, rs2
        Add { rd: X, rs1: X, rs2: X },
        /// rd = addi rs1, imm12
        Addi { rd: X, rs1: X, imm12: i16 },
        /// store rs2 -> [rs1 + imm12]
        Store { rs2: X, rs1: X, imm12: i16, w: Width },
        /// rd = load [rs1 + imm12]
        Load { rd: X, rs1: X, imm12: i16, w: Width },
        /// unconditional branch rel
        J { rel: i32 },
        /// return
        Ret,
    }

    /// Lower a linear IR slice to RV64 ops.
    pub fn lower(ir: &[Ir]) -> super::Result<Vec<Op>> {
        let mut out = Vec::with_capacity(ir.len() * 2);
        for n in ir {
            match *n {
                Ir::Const { rd, imm } => {
                    // LI pseudo: split to LUI+ADDI using 32-bit path; for full 64-bit, add more steps.
                    let hi = ((imm + (1 << 11)) >> 12) as i32; // round toward plus for sign-correct low
                    let lo = (imm & 0xfff) as i16;
                    out.push(Op::Lui { rd, imm20: hi });
                    out.push(Op::OpImm { rd, rs1: rd, kind: IKind::Addi, imm12: lo, shamt6: None });
                },
                Ir::Add { rd, rs1, rs2 } => {
                    out.push(Op::Op { rd, rs1, rs2, kind: RKind::Add });
                },
                Ir::Addi { rd, rs1, imm12 } => {
                    out.push(Op::OpImm { rd, rs1, kind: IKind::Addi, imm12, shamt6: None });
                },
                Ir::Store { rs2, rs1, imm12, w } => {
                    out.push(Op::Store { rs2, rs1, imm12, w });
                },
                Ir::Load { rd, rs1, imm12, w } => {
                    out.push(Op::Load { rd, rs1, imm12, w });
                },
                Ir::J { rel } => {
                    out.push(Op::Jal { rd: X::Zero, rel });
                },
                Ir::Ret => out.push(Op::Ret),
            }
        }
        Ok(out)
    }
}

/// Emitter producing bytes and recording relocations and labels.
pub mod emit {
    use super::arch::INSN_BYTES;
    use super::buf::CodeBuf;
    use super::enc;
    use super::inst::Op;

    /// Label id allocator.
    #[derive(Debug, Default)]
    pub struct LabelGen {
        next: u32,
    }

    impl LabelGen {
        /// Create.
        pub fn new() -> Self {
            Self { next: 0 }
        }
        /// Allocate a fresh label id.
        pub fn fresh(&mut self) -> u32 {
            let id = self.next;
            self.next += 1;
            id
        }
    }

    /// Emitter writes encoded instructions into a buffer.
    pub struct Emitter<'a> {
        /// Destination buffer receiving encoded bytes.
        pub buf: &'a mut CodeBuf,
    }

    impl<'a> Emitter<'a> {
        /// Create.
        pub fn new(buf: &'a mut CodeBuf) -> Self {
            Self { buf }
        }

        /// Emit a single instruction. `target_off` is used for resolved labels.
        pub fn emit(&mut self, op: &Op, target_off: Option<usize>) -> super::Result<()> {
            let here = self.buf.off();
            let enc = enc::encode(op, here, target_off)?;
            self.buf.reserve(INSN_BYTES);
            self.buf.put_u32(u32::from_le_bytes(enc));
            Ok(())
        }
    }
}

/// Simple assembler with labels and symbol table for short functions.
pub mod asm {
    use super::arch::INSN_BYTES;
    use super::buf::CodeBuf;
    use super::emit::Emitter;
    use super::inst::Op;
    #[cfg(not(feature = "std"))]
    use alloc::{collections::BTreeMap, vec::Vec};
    #[cfg(feature = "std")]
    use alloc_std::{collections::BTreeMap, vec::Vec};

    /// An instruction with optional target label reference for branches/jumps.
    #[derive(Debug, Clone)]
    pub struct LOp {
        /// The instruction to assemble.
        pub op: Op,
        /// Optional symbol label this instruction targets (for branches/jumps).
        pub target_label: Option<u32>,
    }

    /// Assembly program builder.
    #[derive(Default)]
    pub struct Asm {
        seq: Vec<LOp>,
        label_offsets: BTreeMap<u32, usize>,
        here_label: Option<u32>,
    }

    impl Asm {
        /// New.
        pub fn new() -> Self {
            Self::default()
        }

        /// Bind a label at the next instruction boundary.
        pub fn label(&mut self, id: u32) {
            self.here_label = Some(id);
        }

        /// Push an op with an optional target label.
        pub fn push(&mut self, op: Op, target: Option<u32>) {
            if let Some(l) = self.here_label.take() {
                // Will compute real offset during assemble; store symbolic for now
                self.label_offsets.insert(l, self.seq.len() * INSN_BYTES);
            }
            self.seq.push(LOp { op, target_label: target });
        }

        /// Assemble into bytes. Two-pass layout for label resolution in-sequence.
        pub fn assemble(self) -> super::Result<CodeBuf> {
            // First pass computed provisional offsets above.
            let mut buf = CodeBuf::with_capacity(self.seq.len() * INSN_BYTES + 16);
            // Materialize known labels from rough pass into the buffer using final offsets.
            // We rebuild label offsets with correct byte offsets as we emit.
            let mut final_label_offsets = BTreeMap::new();

            // Pre-scan to record labels that were bound before any pushes.
            for (lbl, off) in self.label_offsets.iter() {
                final_label_offsets.insert(*lbl, *off);
            }

            let mut em = Emitter::new(&mut buf);
            for (i, lop) in self.seq.iter().enumerate() {
                // If any label is bound exactly here by earlier `label()`, re-apply with final byte off.
                for (lbl, off) in self.label_offsets.iter() {
                    if *off == i * INSN_BYTES {
                        final_label_offsets.insert(*lbl, em.buf.off());
                        em.buf.define_label(*lbl);
                    }
                }
                let target_off =
                    lop.target_label.and_then(|l| final_label_offsets.get(&l).copied());
                em.emit(&lop.op, target_off)?;
            }
            Ok(buf)
        }
    }
}

/// Vector extension scaffolding guarded by `simd`.
#[cfg(feature = "simd")]
pub mod rvv {
    /// Vector register identifier.
    #[derive(Debug, Copy, Clone, PartialEq, Eq)]
    pub struct VReg(pub u8);

    /// Minimal vector op enum (placeholder).
    #[derive(Debug, Clone, PartialEq, Eq)]
    pub enum VOp {
        /// vadd.vv vrd, vrs1, vrs2
        VAddVV { rd: VReg, rs1: VReg, rs2: VReg },
    }

    impl VReg {
        #[inline]
        pub fn idx(self) -> u8 {
            self.0
        }
    }
}

/// Top-level Codegen pipeline: Lower → Peephole → Encode+Emit.
pub struct Codegen;

impl Codegen {
    /// Compile a small IR slice to bytes.
    pub fn compile(ir: &[lower::Ir]) -> Result<Vec<u8>> {
        let mut ops = lower::lower(ir)?;
        peephole::run(&mut ops);

        let mut buf = buf::CodeBuf::with_capacity(ops.len() * arch::INSN_BYTES);
        let mut em = emit::Emitter::new(&mut buf);

        for op in ops.iter() {
            match op {
                inst::Op::Ret => {
                    em.emit(&inst::Op::Jalr { rd: reg::X::Zero, rs1: abi::RA, imm12: 0 }, None)?;
                },
                inst::Op::Li { .. } => {
                    return Err(CodegenError::Unsupported("Li must not reach encode".into()));
                },
                _ => em.emit(op, None)?,
            }
        }
        Ok(buf.bytes().to_vec())
    }
}

#[cfg(test)]
mod tests {
    use super::inst::{IKind, Op, RKind, Width};
    use super::lower::{Ir, lower};
    use super::reg::X;
    use super::{Codegen, enc};

    #[test]
    fn encode_addi() {
        let op = Op::OpImm { rd: X::A0, rs1: X::A0, kind: IKind::Addi, imm12: 10, shamt6: None };
        let w = enc::encode(&op, 0, None).unwrap();
        assert_eq!(u32::from_le_bytes(w) & 0x7f, 0b0010011);
    }

    #[test]
    fn encode_add() {
        let op = Op::Op { rd: X::A0, rs1: X::A0, rs2: X::A1, kind: RKind::Add };
        let w = enc::encode(&op, 0, None).unwrap();
        assert_eq!(u32::from_le_bytes(w) & 0x7f, 0b0110011);
    }

    #[test]
    fn lower_and_codegen() {
        // a0 = 42; a0 = a0 + a1; store a0 -> [sp+8]; ret
        let ir = vec![
            Ir::Const { rd: X::A0, imm: 42 },
            Ir::Add { rd: X::A0, rs1: X::A0, rs2: X::A1 },
            Ir::Store { rs2: X::A0, rs1: X::Sp, imm12: 8, w: Width::D },
            Ir::Ret,
        ];
        let bytes = Codegen::compile(&ir).unwrap();
        assert!(bytes.len() >= 4);
        // Last is JALR x0, ra, 0 → opcode 0b1100111
        let last = u32::from_le_bytes(bytes[bytes.len() - 4..].try_into().unwrap());
        assert_eq!(last & 0x7f, 0b1100111);
    }
}
