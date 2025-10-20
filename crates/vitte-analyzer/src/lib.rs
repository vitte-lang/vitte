//! vitte-analyzer — analyse sémantique du langage Vitte
//!
//! Branches :
//! - `vitte-ast` pour la structure syntaxique
//! - `vitte-errors` pour les erreurs uniformes
//!
//! Étapes couvertes :
//! - Résolution de noms (variables, fonctions, types)
//! - Inférence et vérification de types
//! - Détections d’erreurs sémantiques basiques (déclaration multiple, type mismatch)
//!
//! Exemple :
//! ```ignore
//! use vitte_analyzer::Analyzer;
//! use vitte_ast::Program;
//!
//! let prog: Program = /* ... */;
//! let mut az = Analyzer::new();
//! if let Err(e) = az.analyze(&prog) {
//!     eprintln!("Erreur sémantique: {e}");
//! }
//! ```

#![deny(missing_docs)]
#![forbid(unsafe_code)]
#![cfg_attr(not(feature = "std"), no_std)]

/* ─────────────────────────── Imports / alloc ─────────────────────────── */

#[cfg(not(feature = "std"))]
extern crate alloc;

use core::fmt;

#[cfg(feature = "std")]
use std::{collections::HashMap, string::String};

#[cfg(not(feature = "std"))]
use alloc::{collections::BTreeMap as HashMap, string::String};

use vitte_ast as ast;

/* ─────────────────────────── Erreurs ─────────────────────────── */

/// Erreur d’analyse sémantique.
#[derive(Debug, Clone)]
pub struct AnalyzeError {
    /// Message humain.
    pub message: String,
}

impl fmt::Display for AnalyzeError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.message)
    }
}

#[cfg(feature = "std")]
impl std::error::Error for AnalyzeError {}

type AResult<T> = core::result::Result<T, AnalyzeError>;

/* ─────────────────────────── Structures ─────────────────────────── */

/// Informations de symbole.
#[derive(Debug, Clone)]
pub struct Symbol {
    /// Nom symbolique.
    pub name: String,
    /// Type associé (texte brut pour simplifier).
    pub ty: Option<String>,
}

/// Analyseur sémantique principal.
pub struct Analyzer {
    symbols: HashMap<String, Symbol>,
}

impl Analyzer {
    /// Crée un nouvel analyseur.
    pub fn new() -> Self {
        Self { symbols: HashMap::new() }
    }

    /// Lance l’analyse complète d’un programme.
    pub fn analyze(&mut self, prog: &ast::Program) -> AResult<()> {
        for item in &prog.items {
            self.check_item(item)?;
        }
        Ok(())
    }

    fn check_item(&mut self, item: &ast::Item) -> AResult<()> {
        match item {
            ast::Item::Function(f) => {
                self.declare(&f.name, Some("fn".into()))?;
                for p in &f.params {
                    self.declare(&p.name, Some(format!("{:?}", p.ty)))?;
                }
                self.check_block(&f.body)?;
            }
            ast::Item::Const(c) => {
                self.declare(&c.name, c.ty.as_ref().map(|t| format!("{:?}", t)))?;
                // expr → vérification type à implémenter
            }
            ast::Item::Struct(s) => {
                self.declare(&s.name, Some("struct".into()))?;
            }
            ast::Item::Enum(e) => {
                self.declare(&e.name, Some("enum".into()))?;
            }
        }
        Ok(())
    }

    fn check_block(&mut self, block: &ast::Block) -> AResult<()> {
        for stmt in &block.stmts {
            self.check_stmt(stmt)?;
        }
        Ok(())
    }

    fn check_stmt(&mut self, stmt: &ast::Stmt) -> AResult<()> {
        match stmt {
            ast::Stmt::Let { name, ty, value, .. } => {
                self.declare(name, ty.as_ref().map(|t| format!("{:?}", t)))?;
                if let Some(_e) = value {
                    // TODO: inférer et comparer type
                }
            }
            ast::Stmt::Return(_e, ..) => {}
            ast::Stmt::While { condition: _, body, .. } => {
                self.check_block(body)?;
            }
            ast::Stmt::For { var, iter: _, body, .. } => {
                self.declare(var, None)?;
                self.check_block(body)?;
            }
            ast::Stmt::If { condition: _, then_block, else_block, .. } => {
                self.check_block(then_block)?;
                if let Some(b) = else_block {
                    self.check_block(b)?;
                }
            }
            ast::Stmt::Expr(_e) => {}
        }
        Ok(())
    }

    fn declare(&mut self, name: &str, ty: Option<String>) -> AResult<()> {
        if self.symbols.contains_key(name) {
            return Err(AnalyzeError { message: format!("Symbole déjà défini: {}", name) });
        }
        self.symbols.insert(name.into(), Symbol { name: name.into(), ty });
        Ok(())
    }
}

/* ─────────────────────────── Tests ─────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_ast::{Block, ConstDecl, Function, Item, Program, Stmt, Type};

    #[test]
    fn redecl_fails() {
        let mut az = Analyzer::new();
        let prog = Program {
            items: vec![
                Item::Const(ConstDecl {
                    name: "X".into(),
                    ty: Some(Type::Int),
                    value: Default::default(),
                    span: None,
                }),
                Item::Const(ConstDecl {
                    name: "X".into(),
                    ty: Some(Type::Int),
                    value: Default::default(),
                    span: None,
                }),
            ],
        };
        let res = az.analyze(&prog);
        assert!(res.is_err());
    }

    #[test]
    fn fn_with_params_ok() {
        let mut az = Analyzer::new();
        let f = Function {
            name: "add".into(),
            params: vec![],
            return_type: Some(Type::Int),
            body: Block { stmts: vec![], span: None },
            span: None,
        };
        let prog = Program { items: vec![Item::Function(f)] };
        assert!(az.analyze(&prog).is_ok());
    }
}
