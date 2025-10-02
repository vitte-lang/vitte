//! vitte-codegen-aarch64 — backend minimal AArch64 (ultra complet)
//!
//! Couvre un sous-ensemble sûr et déterministe, sans dépendances JIT:
//! - Génère du code machine AArch64 pour une fonction feuille qui retourne un
//!   entier 64-bit immédiat dans `x0`, terminée par `ret`.
//! - Encodage direct des instructions: `movz`/`movk` (large immediate) + `ret`.
//! - API stable: [`Codegen`], [`FuncSpec`], [`CodeBlob`], [`CodegenOptions`].
//! - Optionnel: rendu “asm” symbolique lisible (`emit_asm_string`).
//!
//! Notes:
//! - Sortie = *blob* de `.text` brut (pas d’ELF/Mach-O). Lien/embedding à la
//!   charge de l’appelant (ex: écrire dans une page RX et `fn()`).
//! - Alignement 4 octets, endianness little-endian (AArch64 LE standard).
//!
//! Tests: vérifient l’encodage MOVZ/MOVK/RET et la construction de séquences.

#![forbid(unsafe_code)]

use core::fmt;
use std::borrow::Cow;

/// Cible. Ici figée à AArch64 little-endian.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Target {
    AArch64Le,
}

impl Target {
    pub fn pointer_bits(self) -> u32 { 64 }
    pub fn is_little_endian(self) -> bool { true }
}

/// Options de génération.
#[derive(Debug, Clone, Copy)]
pub struct CodegenOptions {
    pub target: Target,
    /// Si vrai: ajoute un `hint #0` (NOP) au début pour facilité d’ID.
    pub prologue_nop: bool,
    /// Si vrai: chaîne asm symbolique jointe.
    pub emit_asm_string: bool,
}
impl Default for CodegenOptions {
    fn default() -> Self {
        Self { target: Target::AArch64Le, prologue_nop: false, emit_asm_string: false }
    }
}

/// Spécification minimale d’une fonction.
#[derive(Debug, Clone)]
pub struct FuncSpec<'a> {
    /// Nom symbolique (diagnostics/asm). Non émis en binaire.
    pub name: Cow<'a, str>,
    /// Retourne une constante 64-bit dans x0.
    pub returns_const_u64: u64,
}
impl<'a> FuncSpec<'a> {
    pub fn new(name: impl Into<Cow<'a, str>>, value: u64) -> Self {
        Self { name: name.into(), returns_const_u64: value }
    }
}

/// Blob de code généré.
#[derive(Clone)]
pub struct CodeBlob {
    /// Octets `.text` (instructions 32-bit alignées).
    pub text: Vec<u8>,
    /// Longueur en instructions (32-bit words).
    pub instr_len: usize,
    /// Optionnel: pseudo-asm pour debug.
    pub asm: Option<String>,
}
impl fmt::Debug for CodeBlob {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("CodeBlob")
            .field("text_len", &self.text.len())
            .field("instr_len", &self.instr_len)
            .field("asm_present", &self.asm.is_some())
            .finish()
    }
}

/// Erreurs de génération.
#[derive(Debug, thiserror::Error)]
pub enum CodegenError {
    #[error("target non supportée")]
    UnsupportedTarget,
    #[error("invariant brisé: {0}")]
    Invariant(&'static str),
}

/// Backend façade.
pub struct Codegen {
    opts: CodegenOptions,
}
impl Codegen {
    pub fn new(opts: CodegenOptions) -> Self { Self { opts } }

    /// Compile une fonction feuille “retimm64”.
    pub fn compile_ret_const(&self, spec: &FuncSpec<'_>) -> Result<CodeBlob, CodegenError> {
        if self.opts.target != Target::AArch64Le {
            return Err(CodegenError::UnsupportedTarget);
        }
        let mut enc = Encoder::new();

        if self.opts.prologue_nop {
            enc.nop();
        }

        enc.mov_abs_x0(spec.returns_const_u64);
        enc.ret();

        let asm = self.opts.emit_asm_string.then(|| {
            let mut s = String::new();
            if self.opts.prologue_nop {
                s.push_str(&format!("{}:\n    hint    #0        ; nop\n", spec.name));
            } else {
                s.push_str(&format!("{}:\n", spec.name));
            }
            for (i, ins) in enc.asm_lines.iter().enumerate() {
                if i == 0 && !self.opts.prologue_nop {
                    s.push_str(&format!("    {ins}\n"));
                } else {
                    s.push_str(&format!("    {ins}\n"));
                }
            }
            s
        });

        Ok(CodeBlob { text: enc.bytes, instr_len: enc.words, asm })
    }
}

// ──────────────────────────────────────────────────────────────────────────
// AArch64 encodage minimal (MOVZ/MOVK/RET/NOP)
// Références: ARM® Architecture Reference Manual for A-profile (A64) — MOVZ/MOVK Wide immediates.
// Champs clés:
//  - sf=1 (64-bit), opc pour MOVZ/MOVK, hw=imm shift/16 (bits 21–22), imm16 bits 5–20, Rd bits 0–4.
//  - MOVZ 64: base 0xD280_0000, MOVK 64: base 0xF280_0000
//  - RET: 0xD65F_03C0
// NOP (hint #0): 0xD503_201F
// ──────────────────────────────────────────────────────────────────────────

struct Encoder {
    bytes: Vec<u8>,
    words: usize,
    asm_lines: Vec<String>,
}
impl Encoder {
    fn new() -> Self { Self { bytes: Vec::with_capacity(6 * 4), words: 0, asm_lines: vec![] } }

    #[inline]
    fn push_u32(&mut self, w: u32, asm: &str) {
        // LE: 32-bit little endian
        self.bytes.extend_from_slice(&w.to_le_bytes());
        self.words += 1;
        self.asm_lines.push(asm.to_string());
    }

    fn nop(&mut self) {
        self.push_u32(0xD503_201F, "hint    #0");
    }

    fn ret(&mut self) {
        self.push_u32(0xD65F_03C0, "ret");
    }

    /// MOVZ X0 + MOVK* pour bâtir une constante 64-bit.
    fn mov_abs_x0(&mut self, val: u64) {
        // Décompose en 4 moitiés 16-bit: [lo..hi], 0,16,32,48
        let parts = [
            (val & 0xFFFF) as u16,
            ((val >> 16) & 0xFFFF) as u16,
            ((val >> 32) & 0xFFFF) as u16,
            ((val >> 48) & 0xFFFF) as u16,
        ];

        // Première: MOVZ X0, imm16, LSL #shift
        let mut first_done = false;
        for (i, imm16) in parts.iter().enumerate() {
            let shift = (i as u32) * 16;
            if !first_done {
                // Toujours émettre un MOVZ initial (zéroifie les autres moitiés)
                let ins = encode_movz_xd_imm16_hw(0, *imm16, shift);
                self.push_u32(ins, &format!("movz    x0, #{}, lsl #{}", *imm16 as u32, shift));
                first_done = true;
            } else if *imm16 != 0 {
                // “Patch” les moitiés non nulles avec MOVK
                let ins = encode_movk_xd_imm16_hw(0, *imm16, shift);
                self.push_u32(ins, &format!("movk    x0, #{}, lsl #{}", *imm16 as u32, shift));
            }
        }
        // Cas dégénéré: si val==0, boucle a émis un MOVZ x0,#0,lsl#0 → OK.
    }
}

#[inline]
fn encode_movz_xd_imm16_hw(rd: u8, imm16: u16, shift: u32) -> u32 {
    // Base MOVZ 64: 1101 0010 1000 0000 0000 0000 0000 0000 = 0xD280_0000
    // hw = shift/16 dans bits 21–22. imm16 dans 5–20. Rd dans 0–4.
    let hw = (shift / 16) & 0b11;
    0xD280_0000 | ((imm16 as u32) << 5) | (hw << 21) | (rd as u32)
}

#[inline]
fn encode_movk_xd_imm16_hw(rd: u8, imm16: u16, shift: u32) -> u32 {
    // Base MOVK 64: 1111 0010 1000 0000 0000 0000 0000 0000 = 0xF280_0000
    let hw = (shift / 16) & 0b11;
    0xF280_0000 | ((imm16 as u32) << 5) | (hw << 21) | (rd as u32)
}

// ──────────────────────────────────────────────────────────────────────────
// Helpers publics
// ──────────────────────────────────────────────────────────────────────────

/// Génère une fonction feuille qui renvoie `value` en `x0` puis `ret`.
pub fn compile_return_u64(name: &str, value: u64, opts: CodegenOptions) -> Result<CodeBlob, CodegenError> {
    Codegen::new(opts).compile_ret_const(&FuncSpec::new(name, value))
}

/// Rend un dump hex compact du blob.
pub fn hexdump32_le(code: &[u8]) -> String {
    let mut out = String::new();
    for (i, chunk) in code.chunks(4).enumerate() {
        if i > 0 { out.push(' '); }
        let mut w = [0u8; 4];
        w[..chunk.len()].copy_from_slice(chunk);
        out.push_str(&format!("{:08x}", u32::from_le_bytes(w)));
    }
    out
}

// ──────────────────────────────────────────────────────────────────────────
// Tests
// ──────────────────────────────────────────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn enc_zero() {
        let blob = compile_return_u64("ret_zero", 0, CodegenOptions { emit_asm_string: true, ..Default::default() }).unwrap();
        // Doit contenir: movz x0,#0 ; ret
        let hex = hexdump32_le(&blob.text);
        // MOVZ x0,#0,lsl#0 = 0xD2800000 ; RET = 0xD65F03C0
        assert!(hex.contains("d2800000"));
        assert!(hex.ends_with("d65f03c0"));
        assert!(blob.instr_len >= 2);
        assert!(blob.asm.as_ref().unwrap().contains("movz"));
        assert!(blob.asm.as_ref().unwrap().contains("ret"));
    }

    #[test]
    fn enc_imm64() {
        let val = 0x1122_3344_5566_7788u64;
        let blob = compile_return_u64("ret_imm", val, CodegenOptions::default()).unwrap();
        // Devrait contenir MOVZ + trois MOVK (quatre quads car toutes moitiés != 0)
        // Séquence attendue: MOVZ(0x7788, lsl48) ? Non. Notre ordre est de bas en haut:
        // movz lsl#0, movk lsl#16, movk lsl#32, movk lsl#48
        // Vérifions juste la présence de MOVK base 0xF2800000 et RET final.
        let hex = hexdump32_le(&blob.text);
        assert!(hex.contains("f280")); // au moins un movk
        assert!(hex.ends_with("d65f03c0"));
    }

    #[test]
    fn prologue_nop_and_asm() {
        let blob = compile_return_u64(
            "with_nop",
            42,
            CodegenOptions { prologue_nop: true, emit_asm_string: true, ..Default::default() },
        )
        .unwrap();
        let hex = hexdump32_le(&blob.text);
        assert!(hex.starts_with("d503201f")); // hint #0
        assert!(blob.asm.as_ref().unwrap().contains("hint    #0"));
        assert!(blob.asm.as_ref().unwrap().contains("mov"));
    }
}