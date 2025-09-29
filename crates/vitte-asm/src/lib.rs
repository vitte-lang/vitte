//! vitte-asm — émission d’assembleur x86_64 / aarch64 pour Vitte.
//!
//! Objectif minimal:
//! - API stable pour émettre du texte assembleur à partir d’un IR simple.
//! - Cibles: `x86_64` et `aarch64`.
//! - Sortie: `String` ou `Vec<u8>` selon besoin appelant.
//!
//! Exemple:
//! ```rust
//! use vitte_asm::{Arch, Config, Module, Function, Inst, Reg, assemble_to_string};
//!
//! let mut f = Function::new("add42");
//! f.push(Inst::Prologue);
//! f.push(Inst::MovImm { dst: Reg::R0, imm: 42 });
//! f.push(Inst::Epilogue);
//!
//! let m = Module::new("demo").with_fn(f);
//! let s = assemble_to_string(Config::new(Arch::X86_64), &m).unwrap();
//! assert!(s.contains("global add42"));
//! ```

#![deny(missing_docs)]
#![forbid(unsafe_code)]
#![cfg_attr(not(feature = "std"), no_std)]

/* ─────────────────────────── alloc / imports ─────────────────────────── */

#[cfg(not(feature = "std"))]
extern crate alloc;

use core::fmt::{self, Write};

#[cfg(feature = "std")]
use std::{string::String, vec::Vec};

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec::Vec};

use vitte_errors::VitteError;

/* ─────────────────────────── API publique ─────────────────────────── */

/// Architecture cible.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Arch {
    /// x86_64 System V.
    X86_64,
    /// AArch64 (ARM64) SysV-like.
    Aarch64,
}

/// Configuration de l’émetteur.
#[derive(Debug, Clone)]
pub struct Config {
    arch: Arch,
}

impl Config {
    /// Nouvelle config.
    pub const fn new(arch: Arch) -> Self {
        Self { arch }
    }
    /// Architecture courante.
    pub const fn arch(&self) -> Arch {
        self.arch
    }
}

/// Registres génériques. Mappés selon l’arch cible.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Reg {
    /// R0 : registre de retour / premier argument.
    R0,
    /// R1..R7 : registres de travail.
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
}

/// Instruction assembleur abstraite minimale.
#[allow(missing_docs)]
#[derive(Debug, Clone, PartialEq)]
pub enum Inst {
    Prologue,
    Epilogue,
    /// dst = src
    Mov { dst: Reg, src: Reg },
    /// dst = imm
    MovImm { dst: Reg, imm: i64 },
    /// dst = a + b
    Add { dst: Reg, a: Reg, b: Reg },
    /// return reg
    Ret { reg: Reg },
}

/// Fonction (liste d’instructions).
#[derive(Debug, Clone, PartialEq)]
pub struct Function {
    /// Nom symbole.
    pub name: String,
    /// Corps.
    pub body: Vec<Inst>,
}

impl Function {
    /// Crée une fonction vide.
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            body: Vec::new(),
        }
    }
    /// Ajoute une instruction.
    pub fn push(&mut self, inst: Inst) -> &mut Self {
        self.body.push(inst);
        self
    }
}

/// Module (fichier de sortie).
#[derive(Debug, Clone, PartialEq)]
pub struct Module {
    /// Nom logique du module.
    pub name: String,
    /// Fonctions.
    pub funs: Vec<Function>,
}

impl Module {
    /// Crée un module.
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            funs: Vec::new(),
        }
    }
    /// Ajoute une fonction et renvoie `self`.
    pub fn with_fn(mut self, f: Function) -> Self {
        self.funs.push(f);
        self
    }
}

/* ─────────────────────────── Émission ─────────────────────────── */

/// Assemble vers une `String` UTF-8.
pub fn assemble_to_string(cfg: Config, m: &Module) -> Result<String, VitteError> {
    let mut out = String::new();
    Emitter::new(cfg, &mut out).emit_module(m)?;
    Ok(out)
}

/// Assemble vers des octets (UTF-8).
pub fn assemble_to_bytes(cfg: Config, m: &Module) -> Result<Vec<u8>, VitteError> {
    Ok(assemble_to_string(cfg, m)?.into_bytes())
}

/* ─────────────────────────── Impl interne ─────────────────────────── */

struct Emitter<'a> {
    cfg: Config,
    out: &'a mut dyn Write,
}

impl<'a> Emitter<'a> {
    fn new(cfg: Config, out: &'a mut dyn Write) -> Self {
        Self { cfg, out }
    }

    fn emit_module(&mut self, m: &Module) -> Result<(), VitteError> {
        match self.cfg.arch {
            Arch::X86_64 => writeln!(self.out, "default rel")?,
            Arch::Aarch64 => (), // rien
        }
        for f in &m.funs {
            self.emit_function(f)?;
        }
        Ok(())
    }

    fn emit_function(&mut self, f: &Function) -> Result<(), VitteError> {
        match self.cfg.arch {
            Arch::X86_64 => {
                writeln!(self.out, "global {}", f.name)?;
                writeln!(self.out, "section .text")?;
                writeln!(self.out, "{}:", f.name)?;
                for ins in &f.body {
                    self.emit_inst_x64(ins)?;
                }
                writeln!(self.out)?;
            }
            Arch::Aarch64 => {
                writeln!(self.out, ".globl {}", f.name)?;
                writeln!(self.out, ".text")?;
                writeln!(self.out, "{}:", f.name)?;
                for ins in &f.body {
                    self.emit_inst_a64(ins)?;
                }
                writeln!(self.out)?;
            }
        }
        Ok(())
    }

    /* ───── x86_64 SysV ───── */

    fn emit_inst_x64(&mut self, i: &Inst) -> Result<(), VitteError> {
        match *i {
            Inst::Prologue => {
                self.ln("push rbp")?;
                self.ln("mov rbp, rsp")?;
            }
            Inst::Epilogue => {
                self.ln("mov rsp, rbp")?;
                self.ln("pop rbp")?;
                self.ln("ret")?;
            }
            Inst::Mov { dst, src } => {
                self.lni("mov {}, {}", reg_x64(dst), reg_x64(src))?;
            }
            Inst::MovImm { dst, imm } => {
                self.lni("mov {}, {}", reg_x64(dst), imm)?;
            }
            Inst::Add { dst, a, b } => {
                if dst != a {
                    self.lni("mov {}, {}", reg_x64(dst), reg_x64(a))?;
                }
                self.lni("add {}, {}", reg_x64(dst), reg_x64(b))?;
            }
            Inst::Ret { reg } => {
                // convention: valeur de retour dans R0 → rax
                if reg != Reg::R0 {
                    self.lni("mov rax, {}", reg_x64(reg))?;
                } else {
                    // déjà dans rax selon nos conventions de génération
                    self.ln("; return in rax")?;
                }
                self.ln("ret")?;
            }
        }
        Ok(())
    }

    /* ───── AArch64 SysV-like ───── */

    fn emit_inst_a64(&mut self, i: &Inst) -> Result<(), VitteError> {
        match *i {
            Inst::Prologue => {
                self.ln("stp x29, x30, [sp, #-16]!")?;
                self.ln("mov x29, sp")?;
            }
            Inst::Epilogue => {
                self.ln("mov sp, x29")?;
                self.ln("ldp x29, x30, [sp], #16")?;
                self.ln("ret")?;
            }
            Inst::Mov { dst, src } => {
                self.lni("mov {}, {}", reg_a64(dst), reg_a64(src))?;
            }
            Inst::MovImm { dst, imm } => {
                // mov immédiat 64 bits: movz/movk simplifié pour petit imm
                if imm >= 0 && imm <= i16::MAX as i64 {
                    self.lni("mov {}, #{}", reg_a64(dst), imm)?;
                } else {
                    // fallback minimal: charge 0 puis ajoute
                    self.lni("mov {}, #0", reg_a64(dst))?;
                    self.lni("movk {}, #{}", reg_a64(dst), (imm & 0xFFFF) as i64)?;
                }
            }
            Inst::Add { dst, a, b } => {
                if dst == a {
                    self.lni("add {}, {}, {}", reg_a64(dst), reg_a64(a), reg_a64(b))?;
                } else {
                    self.lni("mov {}, {}", reg_a64(dst), reg_a64(a))?;
                    self.lni("add {}, {}, {}", reg_a64(dst), reg_a64(dst), reg_a64(b))?;
                }
            }
            Inst::Ret { reg } => {
                // valeur de retour → x0
                if reg != Reg::R0 {
                    self.lni("mov x0, {}", reg_a64(reg))?;
                }
                self.ln("ret")?;
            }
        }
        Ok(())
    }

    /* ───── utils émission ───── */

    #[inline]
    fn ln(&mut self, s: &str) -> Result<(), VitteError> {
        writeln!(self.out, "    {s}").map_err(io_err)
    }
    #[inline]
    fn lni<T: fmt::Display>(&mut self, fmtstr: &str, a: T) -> Result<(), VitteError> {
        writeln!(self.out, "    {}", OneArg(fmtstr, a)).map_err(io_err)
    }
    #[allow(clippy::too_many_arguments)]
    fn lni2<A: fmt::Display, B: fmt::Display>(
        &mut self,
        fmtstr: &str,
        a: A,
        b: B,
    ) -> Result<(), VitteError> {
        writeln!(self.out, "    {}", TwoArgs(fmtstr, a, b)).map_err(io_err)
    }
}

/* ─────────────────────────── Format helpers ─────────────────────────── */

struct OneArg<'a, T>(&'a str, T);
impl<'a, T: fmt::Display> fmt::Display for OneArg<'a, T> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // remplace le premier "{}"
        let mut parts = self.0.splitn(2, "{}");
        f.write_str(parts.next().unwrap_or_default())?;
        write!(f, "{}", self.1)?;
        f.write_str(parts.next().unwrap_or_default())
    }
}
struct TwoArgs<'a, A, B>(&'a str, A, B);
impl<'a, A: fmt::Display, B: fmt::Display> fmt::Display for TwoArgs<'a, A, B> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // remplace les deux "{}"
        let mut parts = self.0.splitn(3, "{}");
        f.write_str(parts.next().unwrap_or_default())?;
        write!(f, "{}", self.1)?;
        f.write_str(parts.next().unwrap_or_default())?;
        write!(f, "{}", self.2)?;
        f.write_str(parts.next().unwrap_or_default())
    }
}

/* ─────────────────────────── Mapping registres ───────────────────────── */

#[inline]
fn reg_x64(r: Reg) -> &'static str {
    match r {
        Reg::R0 => "rax",
        Reg::R1 => "rbx",
        Reg::R2 => "rcx",
        Reg::R3 => "rdx",
        Reg::R4 => "r8",
        Reg::R5 => "r9",
        Reg::R6 => "r10",
        Reg::R7 => "r11",
    }
}

#[inline]
fn reg_a64(r: Reg) -> &'static str {
    match r {
        Reg::R0 => "x0",
        Reg::R1 => "x1",
        Reg::R2 => "x2",
        Reg::R3 => "x3",
        Reg::R4 => "x4",
        Reg::R5 => "x5",
        Reg::R6 => "x6",
        Reg::R7 => "x7",
    }
}

/* ─────────────────────────── Erreurs I/O → VitteError ───────────────── */

#[inline]
fn io_err(e: core::fmt::Error) -> VitteError {
    VitteError::Io(std_io_err(e))
}

#[cfg(feature = "std")]
fn std_io_err(_: core::fmt::Error) -> std::io::Error {
    // Pas d’info fine depuis fmt::Error.
    std::io::Error::new(std::io::ErrorKind::Other, "format error")
}

#[cfg(not(feature = "std"))]
fn std_io_err(_: core::fmt::Error) -> core::convert::Infallible {
    // no_std: placeholder, ne sera pas converti car non appelé avec no_std + subscriber.
    unreachable!()
}

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn x64_minimal_ret_42() {
        let mut f = Function::new("ret42");
        f.push(Inst::Prologue);
        f.push(Inst::MovImm { dst: Reg::R0, imm: 42 });
        // convention: valeur de retour en R0 → rax
        f.push(Inst::Ret { reg: Reg::R0 });

        let m = Module::new("demo").with_fn(f);
        let s = assemble_to_string(Config::new(Arch::X86_64), &m).unwrap();
        assert!(s.contains("global ret42"));
        assert!(s.contains("mov rax, 42") || s.contains("mov rax, rax"));
        assert!(s.contains("ret"));
    }

    #[test]
    fn a64_minimal_ret_7() {
        let mut f = Function::new("ret7");
        f.push(Inst::Prologue);
        f.push(Inst::MovImm { dst: Reg::R0, imm: 7 });
        f.push(Inst::Ret { reg: Reg::R0 });

        let m = Module::new("demo").with_fn(f);
        let s = assemble_to_string(Config::new(Arch::Aarch64), &m).unwrap();
        assert!(s.contains(".globl ret7"));
        assert!(s.contains("mov x0, #7"));
        assert!(s.contains("ret"));
    }
}
