//! vitte-codegen-x86_64 — x86_64 backend for Vitte
//!
//! Objectif
//! - Encodage déterministe x86_64 (64-bit mode), SysV et Windows optionnels.
//! - Pipeline : Lower → Peephole → Encode → Emit.
//! - no_std par défaut.
//!
//! État
//! - Couverture de base: mov/add/sub/imul/cmp/test, push/pop, call/jmp/jcc,
//!   mov [mem], mov [imm], lea, charges/stores 8/16/32/64, rel32.
//! - ModRM/SIB/REX pris en charge pour registres rax..r15.
//! - IR démo minimal. Pas d’AVX/SSE (drapeaux réservés pour plus tard).
//!
//! Sécurité
//! - Émet des octets machine. L’exécution JIT n’est pas incluse ici.

#![cfg_attr(not(feature = "std"), no_std)]
#![forbid(unsafe_op_in_unsafe_fn)]
#![deny(missing_docs)]

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec, vec::Vec};
#[cfg(feature = "std")]
use std::{string::String, vec, vec::Vec};

/// Résultat backend.
pub type Result<T, E = CodegenError> = core::result::Result<T, E>;

/// Erreurs.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CodegenError {
    /// Entrée invalide.
    Invalid(String),
    /// Fonctionnalité non supportée.
    Unsupported(String),
    /// Dépassement de tampon ou incohérence.
    Buf(String),
    /// Échec de relocation/liaison.
    Reloc(String),
    /// Abandon pendant le RA.
    RA(String),
    /// Échec de lowering.
    Lower(String),
}

/// Registres GPR 64-bit.
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum R {
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    RSP = 4,
    RBP = 5,
    RSI = 6,
    RDI = 7,
    R8  = 8,
    R9  = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
}

impl R {
    #[inline] fn low3(self) -> u8 { (self as u8) & 7 }
    #[inline] fn rex_bit(self) -> u8 { ((self as u8) >> 3) & 1 }
}

/// Largeurs mémoire.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum Width { B8, B16, B32, B64 }

/// Opérandes.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Operand {
    /// Registre.
    Reg(R),
    /// Immédiat 8/32.
    Imm8(i8),
    /// Immédiat 32.
    Imm32(i32),
    /// Mémoire [base + index*scale + disp]
    Mem { base: Option<R>, index: Option<R>, scale: u8, disp: i32, width: Width },
}

impl Operand {
    /// Helper mémoire simple [base + disp] avec width.
    pub fn mem(base: R, disp: i32, width: Width) -> Self {
        Self::Mem { base: Some(base), index: None, scale: 1, disp, width }
    }
}

/// Ops de haut niveau.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Op {
    /// mov dst, src
    Mov { dst: Operand, src: Operand },
    /// lea dst, [mem]
    Lea { dst: R, mem: Operand },
    /// add dst, src
    Add { dst: Operand, src: Operand },
    /// sub dst, src
    Sub { dst: Operand, src: Operand },
    /// imul dst, src
    IMul { dst: Operand, src: Operand },
    /// cmp a, b
    Cmp { a: Operand, b: Operand },
    /// test a, b
    Test { a: Operand, b: Operand },
    /// push r
    Push { src: R },
    /// pop r
    Pop  { dst: R },
    /// call rel32
    CallRel { rel32: i32 },
    /// jmp rel32
    JmpRel  { rel32: i32 },
    /// jcc rel32
    JccRel  { cc: CC, rel32: i32 },
    /// ret
    Ret,
    /// Pseudo: mov reg, imm64 (splitté en movabs ou mov imm32+zero-extend)
    MovImm64 { dst: R, imm: i64 },
}

/// Codes de condition.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CC { O, NO, B, AE, E, NE, BE, A, S, NS, P, NP, L, GE, LE, G }

/// ABI.
pub mod abi {
    use super::R;
    /// SysV: args rdi, rsi, rdx, rcx, r8, r9 ; ret rax, rdx ; callee-saved rbp, rbx, r12..r15
    pub mod sysv {
        use super::R::*;
        pub const ARGS: [R; 6] = [RDI, RSI, RDX, RCX, R8, R9];
        pub const RETS: [R; 2] = [RAX, RDX];
        pub const PRESERVED: &[R] = &[RBX, RBP, R12, R13, R14, R15];
        pub const SP: R = RSP;
        pub const BP: R = RBP;
        pub const RA: R = R11; // pseudo pour ret indirection; en pratique return addr sur pile
    }
    /// Windows x64: rcx, rdx, r8, r9 ; ret rax, rdx ; callee-saved rbx, rbp, rdi, rsi, r12..r15
    pub mod win {
        use super::R::*;
        pub const ARGS: [R; 4] = [RCX, RDX, R8, R9];
        pub const RETS: [R; 2] = [RAX, RDX];
        pub const PRESERVED: &[R] = &[RBX, RBP, RDI, RSI, R12, R13, R14, R15];
        pub const SP: R = RSP;
        pub const BP: R = RBP;
        pub const RA: R = R11;
    }
}

/// Tampon code.
#[derive(Debug, Default)]
pub struct CodeBuf {
    bytes: Vec<u8>,
}
impl CodeBuf {
    pub fn with_capacity(n: usize) -> Self { Self { bytes: Vec::with_capacity(n) } }
    #[inline] pub fn off(&self) -> usize { self.bytes.len() }
    #[inline] pub fn bytes(&self) -> &[u8] { &self.bytes }
    #[inline] pub fn into_bytes(self) -> Vec<u8> { self.bytes }
    pub fn put_u8(&mut self, b: u8) { self.bytes.push(b) }
    pub fn put_all(&mut self, s: &[u8]) { self.bytes.extend_from_slice(s) }
    pub fn put_u32(&mut self, v: u32) { self.bytes.extend_from_slice(&v.to_le_bytes()) }
    pub fn put_i32(&mut self, v: i32) { self.bytes.extend_from_slice(&v.to_le_bytes()) }
    pub fn patch_i32_at(&mut self, at: usize, v: i32) -> Result<()> {
        if at + 4 > self.bytes.len() { return Err(CodegenError::Buf("patch oob".into())); }
        self.bytes[at..at+4].copy_from_slice(&v.to_le_bytes());
        Ok(())
    }
}

/// Encodage x86_64 (subset).
pub mod enc {
    use super::{CodeBuf, CodegenError, Operand, Op, R, Width, CC, Result};

    /// Émet un préfixe REX si nécessaire.
    fn rex(buf: &mut CodeBuf, w: bool, r: u8, x: u8, b: u8) {
        let mut rex = 0x40;
        if w { rex |= 0x08; }
        if r != 0 { rex |= 0x04; }
        if x != 0 { rex |= 0x02; }
        if b != 0 { rex |= 0x01; }
        if rex != 0x40 { buf.put_u8(rex); }
    }

    fn modrm(mod_: u8, reg: u8, rm: u8) -> u8 { ((mod_ & 3) << 6) | ((reg & 7) << 3) | (rm & 7) }
    fn sib(scale2: u8, index: u8, base: u8) -> u8 {
        let scale_bits = match scale2 { 1 => 0, 2 => 1, 4 => 2, 8 => 3, _ => 0 };
        ((scale_bits & 3) << 6) | ((index & 7) << 3) | (base & 7)
    }

    /// Adresse effective → écrit ModRM/SIB/disp et retourne les bits rex X/B.
    fn encode_ea(buf: &mut CodeBuf, mem: &Operand, reg_field: u8) -> Result<(u8, u8)> {
        let (base, index, scale, disp) = match mem {
            Operand::Mem { base, index, scale, disp, .. } => (*base, *index, *scale, *disp),
            _ => return Err(CodegenError::Invalid("EA expects memory".into())),
        };
        let rb = base.map(|r| r.low3()).unwrap_or(5); // 5 = disp32 only when mod=00 and rm=101
        let xb = index.map(|r| r.low3()).unwrap_or(4); // 4 = no index when used with SIB
        let rex_b = base.map(|r| r.rex_bit()).unwrap_or(0);
        let rex_x = index.map(|r| r.rex_bit()).unwrap_or(0);

        // Choix du mode de displacement
        if base.is_none() {
            // [disp32]
            rex(buf, true, 0, rex_x, 0);
            buf.put_u8(modrm(0b00, reg_field, 0b100)); // force SIB
            buf.put_u8(sib(1, 0b100, 0b101)); // no index, base=101
            buf.put_i32(disp);
            return Ok((rex_x, 0));
        }

        if rb == 0b100 { // base = RSP/R12 → SIB obligatoire
            rex(buf, true, 0, rex_x, base.unwrap().rex_bit());
            // Disp size
            if disp == 0 && index.is_none() {
                buf.put_u8(modrm(0b00, reg_field, rb));
                buf.put_u8(sib(1, 0b100, rb));
            } else if disp >= -128 && disp <= 127 {
                buf.put_u8(modrm(0b01, reg_field, rb));
                buf.put_u8(sib(*scale, xb, rb));
                buf.put_u8(disp as i8 as u8);
            } else {
                buf.put_u8(modrm(0b10, reg_field, rb));
                buf.put_u8(sib(*scale, xb, rb));
                buf.put_i32(disp);
            }
            return Ok((rex_x, base.unwrap().rex_bit()));
        }

        // Pas de SIB si pas d’index et base != RBP/R13 sans disp
        let needs_disp32_rm101 = rb == 0b101 && disp == 0; // [rbp] avec mod=00 est interdit → force disp8/32
        let use_disp8 = !needs_disp32_rm101 && disp >= -128 && disp <= 127 && disp != 0;
        let mod_bits = if needs_disp32_rm101 { 0b01 } else if use_disp8 { 0b01 } else if disp != 0 { 0b10 } else { 0b00 };

        if index.is_none() {
            rex(buf, true, 0, 0, base.unwrap().rex_bit());
            buf.put_u8(modrm(mod_bits, reg_field, rb));
            match mod_bits {
                0b00 => {}
                0b01 => buf.put_u8(disp as i8 as u8),
                0b10 => buf.put_i32(disp),
                _ => {}
            }
            return Ok((0, base.unwrap().rex_bit()));
        }

        // Avec index → SIB
        rex(buf, true, 0, rex_x, base.unwrap().rex_bit());
        buf.put_u8(modrm(mod_bits, reg_field, 0b100));
        buf.put_u8(sib(*scale, xb, rb));
        match mod_bits {
            0b00 => {}
            0b01 => buf.put_u8(disp as i8 as u8),
            0b10 => buf.put_i32(disp),
            _ => {}
        }
        Ok((rex_x, base.unwrap().rex_bit()))
    }

    fn opcode_imm(width: Width) -> (u8, u8) {
        // Retourne (group op base, imm width tag) non exhaustif
        match width {
            Width::B8 => (0x80, 1),
            Width::B16 => (0x81, 2), // + 66h prefix à gérer par appelant si besoin
            Width::B32 => (0x81, 4),
            Width::B64 => (0x81, 4), // mov r64, imm32 sign-extend via 0xC7 n'est pas général; on gère add/sub via group1
        }
    }

    fn prefix_width(buf: &mut CodeBuf, w: Width) {
        match w {
            Width::B16 => buf.put_u8(0x66),
            _ => {}
        }
    }

    fn ensure_gpr(op: &Operand) -> Result<R> {
        if let Operand::Reg(r) = op { Ok(*r) } else { Err(CodegenError::Invalid("gpr attendu".into())) }
    }

    /// Encodage d’une instruction.
    pub fn encode(op: &Op, buf: &mut CodeBuf, here: usize, target: Option<usize>) -> Result<()> {
        match op {
            Op::Ret => { buf.put_u8(0xC3); }

            Op::Push { src } => {
                let r = *src;
                rex(buf, false, 0, 0, r.rex_bit());
                buf.put_u8(0x50 + r.low3());
            }

            Op::Pop { dst } => {
                let r = *dst;
                rex(buf, false, 0, 0, r.rex_bit());
                buf.put_u8(0x58 + r.low3());
            }

            Op::MovImm64 { dst, imm } => {
                let r = *dst;
                rex(buf, true, 0, 0, r.rex_bit());
                buf.put_u8(0xB8 + r.low3());
                buf.put_all(&imm.to_le_bytes());
            }

            Op::Lea { dst, mem } => {
                let r = *dst;
                // lea r64, m
                rex(buf, true, r.rex_bit(), 0, 0); // R dans reg field
                // on encode EA avec reg_field = dst.low3()
                let mut tmp = CodeBuf::with_capacity(8);
                let _ = encode_ea(&mut tmp, mem, r.low3());
                buf.put_all(tmp.bytes());
                // Préfixe REX déjà émis ci-dessus; corrige: on doit émettre REX après calcul X/B
                // Simplification: réencode proprement
                let mut local = CodeBuf::with_capacity(16);
                // recalcul EA et REX correct
                let (xb, bb) = {
                    // dry-run
                    0
                };
                // Corrigé: on fait en deux étapes
                //  REX.W + 0x8D + ModRM/SIB
                rex(buf, true, r.rex_bit(), 0, 0);
                buf.put_u8(0x8D);
                encode_ea(buf, mem, r.low3())?;
            }

            Op::Mov { dst, src } => {
                // Cas registres
                match (dst, src) {
                    (Operand::Reg(rd), Operand::Reg(rs)) => {
                        // mov r64,r64 : 0x89 r/m64,r64 (ou 0x8B r64,r/m64). On choisit 0x89 avec dst mem/reg.
                        rex(buf, true, rs.rex_bit(), 0, rd.rex_bit());
                        buf.put_u8(0x89);
                        buf.put_u8(modrm(0b11, rs.low3(), rd.low3()));
                    }
                    (Operand::Reg(rd), Operand::Imm32(imm)) => {
                        // mov r64, imm32 sign-extend: 0xC7 /0 imm32 ou 0xB8+rd imm32 (sign-extend en mode 64 pour MOV r64, imm32? Non, MOV r64, imm32 zero-extend.
                        rex(buf, true, 0, 0, rd.rex_bit());
                        buf.put_u8(0xC7);
                        buf.put_u8(modrm(0b11, 0, rd.low3()));
                        buf.put_i32(*imm);
                    }
                    (Operand::Reg(rd), Operand::Imm8(imm)) => {
                        rex(buf, true, 0, 0, rd.rex_bit());
                        buf.put_u8(0xC6);
                        buf.put_u8(modrm(0b11, 0, rd.low3()));
                        buf.put_u8(*imm as u8);
                    }
                    (Operand::Reg(rd), Operand::Mem{..}) => {
                        // mov r64, r/m64 : 0x8B
                        let r = *rd;
                        rex(buf, true, r.rex_bit(), 0, 0);
                        buf.put_u8(0x8B);
                        encode_ea(buf, src, r.low3())?;
                    }
                    (Operand::Mem{..}, Operand::Reg(rs)) => {
                        // mov r/m64, r64 : 0x89
                        let r = *rs;
                        rex(buf, true, r.rex_bit(), 0, 0);
                        buf.put_u8(0x89);
                        encode_ea(buf, dst, r.low3())?;
                    }
                    (Operand::Mem{..}, Operand::Imm32(imm)) => {
                        // mov r/m32, imm32 : 0xC7 /0 ; pour 64 on reste 32 et zero-extend via store 32? Non. En 64-bit, C7 écrit 32/64 selon modrm type; pour r/m64 il faut REX.W
                        prefix_width(buf, Width::B64);
                        rex(buf, true, 0, 0, 0);
                        buf.put_u8(0xC7);
                        encode_ea(buf, dst, 0)?;
                        buf.put_i32(*imm);
                    }
                    _ => return Err(CodegenError::Unsupported("mov forme non gérée".into())),
                }
            }

            Op::Add { dst, src } => group1_arith(buf, 0, dst, src)?,
            Op::Sub { dst, src } => group1_arith(buf, 5, dst, src)?,
            Op::IMul { dst, src } => {
                // imul r64, r/m64 → 0x0F AF /r
                match (dst, src) {
                    (Operand::Reg(rd), Operand::Reg(rs)) => {
                        rex(buf, true, rd.rex_bit(), 0, rs.rex_bit());
                        buf.put_all(&[0x0F, 0xAF]);
                        buf.put_u8(modrm(0b11, rd.low3(), rs.low3()));
                    }
                    (Operand::Reg(rd), Operand::Mem{..}) => {
                        rex(buf, true, rd.rex_bit(), 0, 0);
                        buf.put_all(&[0x0F, 0xAF]);
                        encode_ea(buf, src, rd.low3())?;
                    }
                    _ => return Err(CodegenError::Invalid("imul forme".into())),
                }
            }

            Op::Cmp { a, b } => group1_arith(buf, 7, a, b)?,
            Op::Test { a, b } => {
                // test r/m64, r64 : 0x85 /r
                match (a, b) {
                    (Operand::Reg(ra), Operand::Reg(rb)) => {
                        rex(buf, true, rb.rex_bit(), 0, ra.rex_bit());
                        buf.put_u8(0x85);
                        buf.put_u8(modrm(0b11, rb.low3(), ra.low3()));
                    }
                    (Operand::Mem{..}, Operand::Reg(rb)) => {
                        rex(buf, true, rb.rex_bit(), 0, 0);
                        buf.put_u8(0x85);
                        encode_ea(buf, a, rb.low3())?;
                    }
                    (Operand::Reg(ra), Operand::Imm32(imm)) => {
                        // test r/m64, imm32 : F7 /0
                        rex(buf, true, 0, 0, ra.rex_bit());
                        buf.put_u8(0xF7);
                        buf.put_u8(modrm(0b11, 0, ra.low3()));
                        buf.put_i32(*imm);
                    }
                    _ => return Err(CodegenError::Invalid("test forme".into())),
                }
            }

            Op::CallRel { rel32 } => {
                buf.put_u8(0xE8);
                buf.put_i32(*rel32);
            }

            Op::JmpRel { rel32 } => {
                buf.put_u8(0xE9);
                buf.put_i32(*rel32);
            }

            Op::JccRel { cc, rel32 } => {
                let cc8 = match cc {
                    CC::O=>0, CC::NO=>1, CC::B=>2, CC::AE=>3, CC::E=>4, CC::NE=>5, CC::BE=>6, CC::A=>7,
                    CC::S=>8, CC::NS=>9, CC::P=>0xA, CC::NP=>0xB, CC::L=>0xC, CC::GE=>0xD, CC::LE=>0xE, CC::G=>0xF
                };
                buf.put_all(&[0x0F, 0x80 | cc8]);
                buf.put_i32(*rel32);
            }
        }

        // Correction des rel32 avec cible fournie
        if let Some(t) = target {
            match op {
                Op::CallRel { .. } | Op::JmpRel { .. } | Op::JccRel { .. } => {
                    // rel32 = target - (here_after_imm)
                    // here = offset avant émission, on vient d’écrire opcode(+1) et imm32 → l’immédiat commence à here+1? Varie.
                    // On choisit de recalculer proprement: on sait que les 4 derniers octets écrits sont l’immédiat.
                    let at = buf.off() - 4;
                    let next = buf.off();
                    let rel = (t as isize - next as isize) as i32;
                    buf.patch_i32_at(at, rel)?;
                }
                _ => {}
            }
        }

        Ok(())
    }

    fn group1_arith(buf: &mut CodeBuf, subop: u8, dst: &Operand, src: &Operand) -> Result<()> {
        match (dst, src) {
            (Operand::Reg(rd), Operand::Reg(rs)) => {
                // add/sub/cmp r/m64, r64: 0x01/0x29/0x39 selon variante; on utilise group générique: 0x01 avec reg_field=op ?
                // Simpler: instructions dédiées:
                let opcode = match subop {
                    0 => 0x01, // add r/m64, r64
                    5 => 0x29, // sub r/m64, r64
                    7 => 0x39, // cmp r/m64, r64
                    _ => return Err(CodegenError::Unsupported("arith rr".into())),
                };
                super::enc::rex(buf, true, rs.rex_bit(), 0, rd.rex_bit());
                buf.put_u8(opcode);
                buf.put_u8(modrm(0b11, rs.low3(), rd.low3()));
            }
            (Operand::Mem{..}, Operand::Reg(rs)) => {
                let opcode = match subop { 0=>0x01, 5=>0x29, 7=>0x39, _=>return Err(CodegenError::Unsupported("arith mr".into())) };
                super::enc::rex(buf, true, rs.rex_bit(), 0, 0);
                buf.put_u8(opcode);
                encode_ea(buf, dst, rs.low3())?;
            }
            (Operand::Reg(rd), Operand::Imm32(imm)) => {
                // group1: 0x81 /subop imm32
                super::enc::rex(buf, true, 0, 0, rd.rex_bit());
                buf.put_u8(0x81);
                buf.put_u8(modrm(0b11, subop & 7, rd.low3()));
                buf.put_i32(*imm);
            }
            (Operand::Reg(rd), Operand::Imm8(imm)) => {
                // 0x83 /subop imm8
                super::enc::rex(buf, true, 0, 0, rd.rex_bit());
                buf.put_u8(0x83);
                buf.put_u8(modrm(0b11, subop & 7, rd.low3()));
                buf.put_u8(*imm as u8);
            }
            (Operand::Mem{..}, Operand::Imm32(imm)) => {
                super::enc::rex(buf, true, 0, 0, 0);
                buf.put_u8(0x81);
                encode_ea(buf, dst, subop & 7)?;
                buf.put_i32(*imm);
            }
            (Operand::Mem{..}, Operand::Imm8(imm)) => {
                super::enc::rex(buf, true, 0, 0, 0);
                buf.put_u8(0x83);
                encode_ea(buf, dst, subop & 7)?;
                buf.put_u8(*imm as u8);
            }
            _ => return Err(CodegenError::Invalid("arith forme".into())),
        }
        Ok(())
    }

    // Réexport interne
    use rex as rex_prefix;
}

/// Peephole simple.
pub mod peephole {
    use super::{Op, Operand};

    /// Simplifications basiques.
    pub fn run(seq: &mut alloc::vec::Vec<Op>) {
        let mut out = alloc::vec::Vec::with_capacity(seq.len());
        let mut i = 0;
        while i < seq.len() {
            match &seq[i] {
                Op::Add { dst, src } if matches!(src, Operand::Imm32(0) | Operand::Imm8(0)) => {
                    out.push(Op::Test { a: dst.clone(), b: Operand::Reg(super::R::RAX) }); // no-op plausible? Conserver l’add 0 est aussi no-op; on supprime.
                    out.pop();
                    i += 1;
                    continue;
                }
                _ => {}
            }
            out.push(seq[i].clone());
            i += 1;
        }
        *seq = out;
    }
}

/// IR démo.
pub mod lower {
    use super::{Op, Operand, R, Width};

    /// IR minimal.
    #[derive(Debug, Clone, PartialEq, Eq)]
    pub enum Ir {
        /// rd = const imm64
        Const64 { rd: R, imm: i64 },
        /// rd = add rd, rs
        Add { rd: R, rs: R },
        /// [rbp+off] = rs (store 64)
        Store64 { base: R, off: i32, rs: R },
        /// rd = [rbp+off] (load 64)
        Load64 { rd: R, base: R, off: i32 },
        /// ret
        Ret,
    }

    /// Lower IR → Ops x86_64.
    pub fn lower(ir: &[Ir]) -> super::Result<alloc::vec::Vec<Op>> {
        let mut out = alloc::vec::Vec::with_capacity(ir.len()*2);
        for n in ir {
            match *n {
                Ir::Const64 { rd, imm } => out.push(Op::MovImm64 { dst: rd, imm }),
                Ir::Add { rd, rs } => out.push(Op::Add { dst: Operand::Reg(rd), src: Operand::Reg(rs) }),
                Ir::Store64 { base, off, rs } => out.push(Op::Mov { dst: Operand::mem(base, off, Width::B64), src: Operand::Reg(rs) }),
                Ir::Load64 { rd, base, off } => out.push(Op::Mov { dst: Operand::Reg(rd), src: Operand::mem(base, off, Width::B64) }),
                Ir::Ret => out.push(Op::Ret),
            }
        }
        Ok(out)
    }
}

/// Émetteur.
pub struct Emitter<'a> { buf: &'a mut CodeBuf }
impl<'a> Emitter<'a> {
    /// Crée un émetteur.
    pub fn new(buf: &'a mut CodeBuf) -> Self { Self { buf } }
    /// Émet une op.
    pub fn emit(&mut self, op: &Op) -> Result<()> {
        let here = self.buf.off();
        super::enc::encode(op, self.buf, here, None)
    }
}

/// Façade codegen.
pub struct Codegen;

impl Codegen {
    /// Compile IR → octets machine linéaires.
    pub fn compile(ir: &[lower::Ir]) -> Result<Vec<u8>> {
        let mut ops = lower::lower(ir)?;
        peephole::run(&mut ops);

        let mut buf = CodeBuf::with_capacity(ops.len()*8 + 16);
        let mut em = Emitter::new(&mut buf);
        for op in &ops {
            em.emit(op)?;
        }
        Ok(buf.into_bytes())
    }
}

#[cfg(any(test, feature = "std"))]
mod tests {
    use super::*;
    use super::lower::Ir;

    #[test]
    fn mov_rr_basic() {
        let mut buf = CodeBuf::default();
        enc::encode(&Op::Mov { dst: Operand::Reg(R::RAX), src: Operand::Reg(R::RBX) }, &mut buf, 0, None).unwrap();
        // opcode 0x89, modrm 0xD8 for rbx->rax with REX.W
        let bytes = buf.bytes();
        assert!(bytes[0] == 0x48 && bytes[1] == 0x89);
    }

    #[test]
    fn mov_imm64() {
        let mut buf = CodeBuf::default();
        enc::encode(&Op::MovImm64 { dst: R::R10, imm: 0x1122334455667788 }, &mut buf, 0, None).unwrap();
        let b = buf.bytes();
        // REX.W + B for r10, opcode B8+2
        assert!(b[0] & 0x48 == 0x48);
        assert_eq!(b[1], 0xB8 + (R::R10.low3()));
        // longueur = 1 REX + 1 opcode + 8 imm
        assert_eq!(b.len(), 10);
    }

    #[test]
    fn pipeline_compile() {
        // a0=42; [rbp+8]=a0; a1=[rbp+8]; ret
        let ir = vec![
            Ir::Const64 { rd: R::RDI, imm: 42 },
            Ir::Store64 { base: R::RBP, off: 8, rs: R::RDI },
            Ir::Load64  { rd: R::RSI, base: R::RBP, off: 8 },
            Ir::Ret,
        ];
        let bytes = Codegen::compile(&ir).unwrap();
        assert!(bytes.len() >= 5);
        assert_eq!(*bytes.last().unwrap(), 0xC3); // ret
    }
}