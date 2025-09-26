// src/lib.rs
//! Vitte Compiler - front/middle/back + émission VITBC
//!
//! - Entrée : `vitte_ast::Program`
//! - Sortie : bytecode VITBC (en mémoire ou sur disque)
//! - Diagnostics : erreurs/warnings collectés avec `Span`
//! - Table des symboles + typage minimal
//! - Passes (trait `Pass`) + Backend (trait `Emitter`)
//!
//! Features :
//! - `std` (par défaut)
//! - `serde` (pour sérialiser certaines structures internes si besoin)
//! - `zstd` (compresser la section CODE avant écriture VITBC)
//!
//! API principale :
//! ```ignore
//! use vitte_compiler::{Compiler, CompilerOptions};
//! use vitte_ast::Program;
//!
//! let mut c = Compiler::new(CompilerOptions::default());
//! let prog: Program = /* ... venant du parser ... */;
//! let bc = c.compile(&prog)?;
//! #[cfg(feature="std")]
//! bc.write_to_file("out.vitbc")?;
//! ```

#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

// ───────────── alloc seulement en no_std ─────────────
#[cfg(not(feature = "std"))]
extern crate alloc;

// ───────────── Imports conditionnels std / no_std ─────────────
#[cfg(feature = "std")]
use std::{
    collections::BTreeMap,
    fs::File,
    io::Write,
    path::Path,
    string::String,
    vec::Vec,
};

#[cfg(not(feature = "std"))]
use alloc::{
    collections::BTreeMap,
    string::String,
    vec::Vec,
};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

use vitte_ast as ast;

// ─────────────────────────────────────────────────────────────────────────────
// Options
// ─────────────────────────────────────────────────────────────────────────────

/// Options du compilateur
#[derive(Debug, Clone)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct CompilerOptions {
    /// Activer des vérifications strictes (warnings→erreurs)
    pub deny_warnings: bool,
    /// Version de format VITBC à écrire
    pub vitbc_version: u16,
    /// Compresser la section CODE (nécessite feature `zstd`)
    pub compress_code: bool,
    /// Insérer des noms (debug) dans un mini-symtab embarqué
    pub embed_names: bool,
}

impl Default for CompilerOptions {
    fn default() -> Self {
        Self {
            deny_warnings: false,
            vitbc_version: 2,
            compress_code: false,
            embed_names: true,
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/* Diagnostics */
// ─────────────────────────────────────────────────────────────────────────────

/// Gravité d’un diagnostic
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Severity {
    /// Info
    Info,
    /// Alerte
    Warning,
    /// Erreur bloquante
    Error,
}

/// Un diagnostic (message, gravité, span optionnel)
#[derive(Debug, Clone)]
pub struct Diagnostic {
    /// Gravité
    pub severity: Severity,
    /// Message humain
    pub message: String,
    /// Localisation
    pub span: Option<ast::Span>,
}

impl Diagnostic {
    /// Construit une erreur
    pub fn error(msg: impl Into<String>, span: Option<ast::Span>) -> Self {
        Self { severity: Severity::Error, message: msg.into(), span }
    }
    /// Construit un warning
    pub fn warn(msg: impl Into<String>, span: Option<ast::Span>) -> Self {
        Self { severity: Severity::Warning, message: msg.into(), span }
    }
}

/// Erreur globale de compilation
#[derive(Debug)]
pub struct CompileError {
    /// Diagnostics accumulés
    pub diagnostics: Vec<Diagnostic>,
}

impl CompileError {
}

type CompileResult<T> = core::result::Result<T, CompileError>;

// ─────────────────────────────────────────────────────────────────────────────
/* Table des symboles + types */
// ─────────────────────────────────────────────────────────────────────────────

/// Identité d’un symbole
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct SymId(u32);

/// Informations sur un symbole
#[derive(Debug, Clone)]
pub struct Symbol {
    /// Nom
    pub name: String,
    /// Type du symbole
    pub ty: TypeId,
    /// Portée (niveau de scope)
    pub scope: u32,
}

/// Table des symboles avec pile de scopes
#[derive(Debug)]
pub struct SymTable {
    scopes: Vec<BTreeMap<String, SymId>>,
    data: Vec<Symbol>,
}

impl SymTable {
    /// Nouvelle table
    pub fn new() -> Self {
        Self { scopes: vec![BTreeMap::new()], data: Vec::new() }
    }
    /// Réinitialise la table (garde le scope racine vide).
    pub fn reset(&mut self) {
        self.scopes.clear();
        self.scopes.push(BTreeMap::new());
        self.data.clear();
    }
    /// Entre dans un nouveau scope
    pub fn push_scope(&mut self) {
        self.scopes.push(BTreeMap::new());
    }
    /// Quitte le scope courant
    pub fn pop_scope(&mut self) {
        if self.scopes.len() > 1 {
            self.scopes.pop();
        }
    }
    /// Déclare un symbole dans le scope courant
    pub fn declare(&mut self, name: String, ty: TypeId, scope: u32) -> SymId {
        let id = SymId(self.data.len() as u32);
        self.data.push(Symbol { name: name.clone(), ty, scope });
        self.scopes.last_mut().unwrap().insert(name, id);
        id
    }
    /// Recherche dans les scopes (de l’intérieur vers l’extérieur)
    pub fn resolve(&self, name: &str) -> Option<SymId> {
        for scope in self.scopes.iter().rev() {
            if let Some(id) = scope.get(name) { return Some(*id); }
        }
        None
    }
    /// Infos
    pub fn get(&self, id: SymId) -> &Symbol { &self.data[id.0 as usize] }
}

/// Identité d’un type interne
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum TypeId {
    /// i64
    Int,
    /// f64
    Float,
    /// bool
    Bool,
    /// string
    Str,
    /// void
    Void,
    /// type utilisateur
    Custom(u32),
}

impl TypeId {
    fn from_ast(t: &ast::Type) -> Self {
        use ast::Type::*;
        match t {
            Int => TypeId::Int,
            Float => TypeId::Float,
            Bool => TypeId::Bool,
            Str => TypeId::Str,
            Void => TypeId::Void,
            Custom(_) => TypeId::Custom(0),
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/* Contexte & Passes */
// ─────────────────────────────────────────────────────────────────────────────

/// Contexte mut du compilateur partagé entre passes
pub struct Ctx<'a> {
    /// Options
    pub opts: &'a CompilerOptions,
    /// Diagnostics accumulés
    pub diags: &'a mut Vec<Diagnostic>,
    /// Table des symboles
    pub symtab: &'a mut SymTable,
    /// Prochaine profondeur de scope (pour tagger Symbol.scope)
    pub scope_depth: u32,
}

impl<'a> Ctx<'a> {
    /// Nouveau scope
    pub fn enter_scope(&mut self) { self.scope_depth += 1; self.symtab.push_scope(); }
    /// Quitte scope
    pub fn leave_scope(&mut self) {
        self.symtab.pop_scope();
        self.scope_depth = self.scope_depth.saturating_sub(1);
    }
}

/// Trait générique d’une passe de compilation
pub trait Pass {
    /// Exécuter la passe sur le programme
    fn run(&mut self, ctx: &mut Ctx<'_>, program: &ast::Program) -> CompileResult<()>;
}

// ─────────────────────────────────────────────────────────────────────────────
/* Passe : Collecte des symboles top-level */
// ─────────────────────────────────────────────────────────────────────────────

struct CollectSymbols;

impl Pass for CollectSymbols {
    fn run(&mut self, ctx: &mut Ctx<'_>, program: &ast::Program) -> CompileResult<()> {
        use ast::Item::*;
        for item in &program.items {
            match item {
                Function(fun) => {
                    let name = fun.name.clone();
                    let ty = if let Some(rt) = &fun.return_type {
                        TypeId::from_ast(rt)
                    } else {
                        TypeId::Void
                    };
                    ctx.symtab.declare(name, ty, ctx.scope_depth);
                }
                Const(cst) => {
                    let name = cst.name.clone();
                    let ty = cst.ty.as_ref().map(TypeId::from_ast).unwrap_or(TypeId::Int);
                    ctx.symtab.declare(name, ty, ctx.scope_depth);
                }
                Struct(sd) => {
                    let name = sd.name.clone();
                    ctx.symtab.declare(name, TypeId::Custom(0), ctx.scope_depth);
                }
                Enum(ed) => {
                    let name = ed.name.clone();
                    ctx.symtab.declare(name, TypeId::Custom(0), ctx.scope_depth);
                }
            }
        }
        Ok(())
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/* Passe : Typage (très basique, à étendre) */
// ─────────────────────────────────────────────────────────────────────────────

struct TypeCheck;

impl TypeCheck {
    fn type_of_expr(&self, ctx: &mut Ctx<'_>, e: &ast::Expr) -> TypeId {
        use ast::Expr::*;
        match e {
            ast::Expr::Literal(l) => match l {
                ast::Literal::Int(_) => TypeId::Int,
                ast::Literal::Float(_) => TypeId::Float,
                ast::Literal::Bool(_) => TypeId::Bool,
                ast::Literal::Str(_) => TypeId::Str,
                ast::Literal::Null => TypeId::Void,
            },
            Ident(name) => ctx.symtab
                .resolve(name)
                .map(|id| ctx.symtab.get(id).ty)
                .unwrap_or_else(|| {
                    ctx.diags.push(Diagnostic::error(
                        format!("Identifiant inconnu: `{name}`"), None));
                    TypeId::Int
                }),
            Call { .. } => TypeId::Int, // simplification
            Binary { left, op, right } => {
                let lt = self.type_of_expr(ctx, left);
                let rt = self.type_of_expr(ctx, right);
                // très basique : exige même type pour arithmétique
                match op {
                    ast::BinaryOp::Add | ast::BinaryOp::Sub
                    | ast::BinaryOp::Mul | ast::BinaryOp::Div | ast::BinaryOp::Mod => {
                        if lt != rt {
                            ctx.diags.push(Diagnostic::error(
                                "Op binaire sur types incompatibles", None));
                        }
                        lt
                    }
                    ast::BinaryOp::Eq | ast::BinaryOp::Ne
                    | ast::BinaryOp::Lt | ast::BinaryOp::Le
                    | ast::BinaryOp::Gt | ast::BinaryOp::Ge
                    | ast::BinaryOp::And | ast::BinaryOp::Or => TypeId::Bool,
                }
            }
            Unary { op, expr } => {
                let t = self.type_of_expr(ctx, expr);
                match op {
                    ast::UnaryOp::Neg => t,
                    ast::UnaryOp::Not => TypeId::Bool,
                }
            }
            Field { expr, .. } => {
                let _t = self.type_of_expr(ctx, expr);
                TypeId::Int
            }
        }
    }

    fn check_stmt(&self, ctx: &mut Ctx<'_>, s: &ast::Stmt) {
        use ast::Stmt::*;
        match s {
            Let { name, ty, value, .. } => {
                let inferred = value.as_ref().map(|e| self.type_of_expr(ctx, e));
                let declared = ty.as_ref().map(TypeId::from_ast);
                let final_ty = declared.or(inferred).unwrap_or(TypeId::Int);
                ctx.symtab.declare(name.clone(), final_ty, ctx.scope_depth);
            }
            Expr(e) => { let _ = self.type_of_expr(ctx, e); }
            Return(e, _) => { let _ = e.as_ref().map(|x| self.type_of_expr(ctx, x)); }
            While { condition, body, .. } => {
                let _ = self.type_of_expr(ctx, condition);
                ctx.enter_scope();
                for st in &body.stmts { self.check_stmt(ctx, st); }
                ctx.leave_scope();
            }
            For { var, iter, body, .. } => {
                let _ = self.type_of_expr(ctx, iter);
                ctx.enter_scope();
                ctx.symtab.declare(var.clone(), TypeId::Int, ctx.scope_depth);
                for st in &body.stmts { self.check_stmt(ctx, st); }
                ctx.leave_scope();
            }
            If { condition, then_block, else_block, .. } => {
                let _ = self.type_of_expr(ctx, condition);
                ctx.enter_scope();
                for st in &then_block.stmts { self.check_stmt(ctx, st); }
                ctx.leave_scope();
                if let Some(eb) = else_block {
                    ctx.enter_scope();
                    for st in &eb.stmts { self.check_stmt(ctx, st); }
                    ctx.leave_scope();
                }
            }
        }
    }
}

impl Pass for TypeCheck {
    fn run(&mut self, ctx: &mut Ctx<'_>, program: &ast::Program) -> CompileResult<()> {
        for item in &program.items {
            if let ast::Item::Function(fun) = item {
                ctx.enter_scope();
                for p in &fun.params {
                    let ty = TypeId::from_ast(&p.ty);
                    ctx.symtab.declare(p.name.clone(), ty, ctx.scope_depth);
                }
                for st in &fun.body.stmts {
                    self.check_stmt(ctx, st);
                }
                ctx.leave_scope();
            }
        }
        Ok(())
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/* IR/Bytecode minimal & Émetteur VITBC */
// ─────────────────────────────────────────────────────────────────────────────

/// Quelques opcodes de base (démo)
#[derive(Debug, Clone, Copy)]
#[repr(u8)]
pub enum Op {
    /// NOP
    Nop = 0x00,
    /// Const i64 (suivi d’un i64 little-endian)
    ConstI64 = 0x01,
    /// Add i64
    AddI64 = 0x02,
    /// Return
    Ret = 0x10,
    /// Appel de fonction (suivi d’un u32: index)
    Call = 0x20,
}

/// Builder de code
#[derive(Default, Debug)]
pub struct CodeBuf {
    buf: Vec<u8>,
}

impl CodeBuf {
    /// Écrit un opcode
    pub fn op(&mut self, op: Op) { self.buf.push(op as u8); }
    /// Écrit un i64 LE
    pub fn i64(&mut self, x: i64) { self.buf.extend_from_slice(&x.to_le_bytes()); }
    /// Écrit un u32 LE
    pub fn u32(&mut self, x: u32) { self.buf.extend_from_slice(&x.to_le_bytes()); }
    /// Accès bytes
    pub fn as_slice(&self) -> &[u8] { &self.buf }
    /// Taille
    pub fn len(&self) -> usize { self.buf.len() }
}

/// Sections VITBC
#[derive(Default, Debug)]
pub struct Sections {
    ints: Vec<i64>,
    floats: Vec<f64>,
    strings: Vec<String>,
    data: Vec<u8>,
    code: CodeBuf,
    names: Vec<String>, // facultatif: noms des fonctions (debug)
}

/// Bytecode complet VITBC (en mémoire)
#[derive(Debug)]
pub struct Bytecode {
    /// Version de format
    pub version: u16,
    /// Sections
    pub sections: Sections,
    /// CRC32 du payload (calculé à la fin)
    pub crc32: u32,
}

impl Bytecode {
    /// Sérialise en VITBC (optionnellement compresse CODE)
    pub fn to_bytes(&self, compress_code: bool) -> Vec<u8> {
        // Header: "VITBC\0" + version (u16 LE)
        let mut out = Vec::new();
        out.extend_from_slice(b"VITBC\0");
        out.extend_from_slice(&self.version.to_le_bytes());

        // Encode chaque section: [tag:4][len:u32][payload]
        // Tags: "INTS","FLTS","STRS","DATA","CODE","NAME"
        fn emit_u32(out: &mut Vec<u8>, x: u32) { out.extend_from_slice(&x.to_le_bytes()); }
        fn emit_tag(out: &mut Vec<u8>, tag: &[u8; 4]) { out.extend_from_slice(tag); }

        // INTS
        {
            let mut buf = Vec::new();
            for v in &self.sections.ints { buf.extend_from_slice(&v.to_le_bytes()); }
            emit_tag(&mut out, b"INTS");
            emit_u32(&mut out, buf.len() as u32);
            out.extend_from_slice(&buf);
        }
        // FLTS
        {
            let mut buf = Vec::new();
            for v in &self.sections.floats { buf.extend_from_slice(&v.to_le_bytes()); }
            emit_tag(&mut out, b"FLTS");
            emit_u32(&mut out, buf.len() as u32);
            out.extend_from_slice(&buf);
        }
        // STRS
        {
            let mut buf = Vec::new();
            for s in &self.sections.strings {
                let bytes = s.as_bytes();
                let len = bytes.len() as u32;
                buf.extend_from_slice(&len.to_le_bytes());
                buf.extend_from_slice(bytes);
            }
            emit_tag(&mut out, b"STRS");
            emit_u32(&mut out, buf.len() as u32);
            out.extend_from_slice(&buf);
        }
        // DATA
        {
            emit_tag(&mut out, b"DATA");
            emit_u32(&mut out, self.sections.data.len() as u32);
            out.extend_from_slice(&self.sections.data);
        }
        // CODE (option: zstd)
        {
            let code_bytes = self.sections.code.as_slice();
            let payload: Vec<u8>;
            #[cfg(feature = "zstd")]
            {
                if compress_code {
                    payload = zstd::bulk::compress(code_bytes, 3).unwrap_or_else(|_| code_bytes.to_vec());
                } else {
                    payload = code_bytes.to_vec();
                }
            }
            #[cfg(not(feature = "zstd"))]
            {
                let _ = compress_code;
                payload = code_bytes.to_vec();
            }

            emit_tag(&mut out, b"CODE");
            emit_u32(&mut out, payload.len() as u32);
            out.extend_from_slice(&payload);
        }
        // NAME (facultatif)
        {
            let mut buf = Vec::new();
            for s in &self.sections.names {
                let b = s.as_bytes();
                let len = b.len() as u32;
                buf.extend_from_slice(&len.to_le_bytes());
                buf.extend_from_slice(b);
            }
            emit_tag(&mut out, b"NAME");
            emit_u32(&mut out, buf.len() as u32);
            out.extend_from_slice(&buf);
        }

        // CRC32 (sur tout après "VITBC\0")
        let crc = crc32_ieee(&out[6..]);
        // TAG "CRCC"
        out.extend_from_slice(b"CRCC");
        out.extend_from_slice(&crc.to_le_bytes());

        out
    }

    /// Écrit dans un fichier (nécessite `std`)
    #[cfg(feature = "std")]
    pub fn write_to_file<P: AsRef<Path>>(&self, path: P) -> core::result::Result<(), std::io::Error> {
        let bytes = self.to_bytes(false);
        let mut f = File::create(path)?;
        f.write_all(&bytes)?;
        Ok(())
    }
}

// CRC32 IEEE (sans table, 8 itérations/byte)
fn crc32_ieee(data: &[u8]) -> u32 {
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

// ─────────────────────────────────────────────────────────────────────────────
/* Backend : Emitter */
// ─────────────────────────────────────────────────────────────────────────────

/// Trait d’émission backend
pub trait Emitter {
    /// Émet le bytecode à partir d’un AST
    fn emit(&mut self, program: &ast::Program) -> Bytecode;
}

/// Implémentation par défaut simple (démo)
pub struct DefaultEmitter {
    opts: CompilerOptions,
}

impl DefaultEmitter {
    /// Nouveau
    pub fn new(opts: CompilerOptions) -> Self {
        Self { opts }
    }

    fn emit_expr(&self, sections: &mut Sections, e: &ast::Expr) {
        match e {
            ast::Expr::Literal(l) => match l {
                ast::Literal::Int(i) => {
                    let code = &mut sections.code;
                    code.op(Op::ConstI64);
                    code.i64(*i);
                }
                ast::Literal::Float(f) => {
                    sections.floats.push(*f);
                    sections.code.op(Op::Nop);
                }
                ast::Literal::Bool(b) => {
                    let code = &mut sections.code;
                    code.op(Op::ConstI64);
                    code.i64(if *b { 1 } else { 0 });
                }
                ast::Literal::Str(s) => {
                    sections.strings.push(s.clone());
                    sections.code.op(Op::Nop);
                }
                ast::Literal::Null => sections.code.op(Op::Nop),
            },
            ast::Expr::Ident(_name) => sections.code.op(Op::Nop),
            ast::Expr::Call { func: _, args } => {
                for a in args {
                    self.emit_expr(sections, a);
                }
                let code = &mut sections.code;
                code.op(Op::Call);
                code.u32(0);
            }
            ast::Expr::Binary { left, op, right } => {
                self.emit_expr(sections, left);
                self.emit_expr(sections, right);
                let code = &mut sections.code;
                match op {
                    ast::BinaryOp::Add => code.op(Op::AddI64),
                    _ => code.op(Op::Nop),
                }
            }
            ast::Expr::Unary { .. } => sections.code.op(Op::Nop),
            ast::Expr::Field { .. } => sections.code.op(Op::Nop),
        }
    }
}

impl Emitter for DefaultEmitter {
    fn emit(&mut self, program: &ast::Program) -> Bytecode {
        let mut sections = Sections::default();

        // enregistrer noms (debug) si demandé
        if self.opts.embed_names {
            for it in &program.items {
                if let ast::Item::Function(f) = it {
                    sections.names.push(f.name.clone());
                }
            }
        }

        // émet un petit code par fonction (démo)
        for it in &program.items {
            if let ast::Item::Function(f) = it {
                for s in &f.body.stmts {
                    if let ast::Stmt::Expr(e) = s {
                        self.emit_expr(&mut sections, e);
                    }
                }
                sections.code.op(Op::Ret);
            }
        }

        Bytecode { version: self.opts.vitbc_version, sections, crc32: 0 }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
/* Compiler façade */
// ─────────────────────────────────────────────────────────────────────────────

/// Le compilateur Vitte : orchestre passes + backend
pub struct Compiler {
    /// Options
    pub options: CompilerOptions,
    diags: Vec<Diagnostic>,
    symtab: SymTable,
}

impl Compiler {
    /// Crée un compilateur
    pub fn new(options: CompilerOptions) -> Self {
        Self { options, diags: Vec::new(), symtab: SymTable::new() }
    }

    /// Compile un programme AST → Bytecode VITBC
    pub fn compile(&mut self, program: &ast::Program) -> CompileResult<Bytecode> {
        self.diags.clear();
        self.symtab.reset();

        // 1) Collecte symboles
        {
            let mut pass = CollectSymbols;
            let mut ctx = Ctx {
                opts: &self.options,
                diags: &mut self.diags,
                symtab: &mut self.symtab,
                scope_depth: 0,
            };
            pass.run(&mut ctx, program)?;
        }

        // 2) Typage
        {
            let mut pass = TypeCheck;
            let mut ctx = Ctx {
                opts: &self.options,
                diags: &mut self.diags,
                symtab: &mut self.symtab,
                scope_depth: 0,
            };
            pass.run(&mut ctx, program)?;
        }

        // Vérif diagnostics
        let has_errors = self.diags.iter().any(|d| d.severity == Severity::Error);
        let has_warnings = self.diags.iter().any(|d| d.severity == Severity::Warning);

        if has_errors || (self.options.deny_warnings && has_warnings) {
            return Err(CompileError { diagnostics: core::mem::take(&mut self.diags) });
        }

        // 3) Backend
        let mut emitter = DefaultEmitter::new(self.options.clone());
        let bc = emitter.emit(program);

        // 4) Le CRC est ajouté par `to_bytes()`, donc RAS ici.
        Ok(bc)
    }

    /// Récupère et vide les diagnostics accumulés
    pub fn take_diagnostics(&mut self) -> Vec<Diagnostic> {
        core::mem::take(&mut self.diags)
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests basiques (compile-time)
// ─────────────────────────────────────────────────────────────────────────────
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn crc32_smoke() {
        let v = b"hello";
        let c = crc32_ieee(v);
        // Juste vérifier que ça renvoie un nombre stable
        assert_eq!(c, crc32_ieee(v));
    }
}
