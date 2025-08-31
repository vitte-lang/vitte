// src/lib.rs
//! Vitte AST (Abstract Syntax Tree)
//!
//! Ce crate définit les structures de données utilisées pour représenter
//! le code source du langage Vitte après parsing, mais avant la génération IR.
//!
//! - Compatible avec `vitte-core` et `vitte-compiler`
//! - Inclut `Span` pour localisation (fichier, ligne, colonne)
//! - No_std compatible (optionnel)
//!
//! # Features
//! - `std` (par défaut) : active affichage amélioré et I/O
//! - `serde` : permet la sérialisation/désérialisation de l’AST
//!
//! # Exemple
//! ```rust
//! use vitte_ast::{Expr, Literal};
//!
//! let lit = Expr::Literal(Literal::Int(42));
//! ```

#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

// ─── alloc uniquement en no_std ───
#[cfg(not(feature = "std"))]
extern crate alloc;

// ─── Imports conditionnels ───
#[cfg(feature = "std")]
use std::{boxed::Box, string::String, vec::Vec};

#[cfg(not(feature = "std"))]
use alloc::{boxed::Box, string::String, vec::Vec};

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Localisation dans le code source
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Span {
    /// Ligne de début
    pub line: u32,
    /// Colonne de début
    pub column: u32,
    /// Offset en bytes depuis le début du fichier
    pub offset: u32,
}

type NewFn = fn(line: u32, column: u32, offset: u32) -> Self;

impl Span {
    pub fn new(line: u32, column: u32, offset: u32) -> Self {
        Self { line, column, offset }
    }
}

/// Un programme Vitte complet
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Program {
    /// Liste de déclarations top-level
    pub items: Vec<Item>,
}

/// Un élément top-level (fonction, constante, struct…)
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Item {
    /// Déclaration de fonction
    Function(Function),
    /// Déclaration de constante globale
    Const(ConstDecl),
    /// Définition de structure
    Struct(StructDecl),
    /// Définition d’énumération
    Enum(EnumDecl),
}

/// Déclaration de fonction
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Function {
    pub name: String,
    pub params: Vec<Param>,
    pub return_type: Option<Type>,
    pub body: Block,
    pub span: Option<Span>,
}

/// Paramètre de fonction
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Param {
    pub name: String,
    pub ty: Type,
    pub span: Option<Span>,
}

/// Déclaration de constante
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct ConstDecl {
    pub name: String,
    pub ty: Option<Type>,
    pub value: Expr,
    pub span: Option<Span>,
}

/// Déclaration de structure
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct StructDecl {
    pub name: String,
    pub fields: Vec<Field>,
    pub span: Option<Span>,
}

/// Déclaration d’énumération
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct EnumDecl {
    pub name: String,
    pub variants: Vec<EnumVariant>,
    pub span: Option<Span>,
}

/// Champ de structure
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Field {
    pub name: String,
    pub ty: Type,
    pub span: Option<Span>,
}

/// Variante d’énumération
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct EnumVariant {
    pub name: String,
    pub fields: Vec<Type>,
    pub span: Option<Span>,
}

/// Un bloc d’instructions
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Block {
    pub stmts: Vec<Stmt>,
    pub span: Option<Span>,
}

/// Une instruction
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Stmt {
    /// Déclaration de variable
    Let {
        name: String,
        ty: Option<Type>,
        value: Option<Expr>,
        span: Option<Span>,
    },
    /// Expression seule (souvent un appel)
    Expr(Expr),
    /// Retour d’une fonction
    Return(Option<Expr>, Option<Span>),
    /// Boucle `while`
    While {
        condition: Expr,
        body: Block,
        span: Option<Span>,
    },
    /// Boucle `for`
    For {
        var: String,
        iter: Expr,
        body: Block,
        span: Option<Span>,
    },
    /// Conditionnelle `if ... else`
    If {
        condition: Expr,
        then_block: Block,
        else_block: Option<Block>,
        span: Option<Span>,
    },
}

/// Une expression
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Expr {
    /// Valeur littérale
    Literal(Literal),
    /// Référence à une variable
    Ident(String),
    /// Appel de fonction
    Call {
        func: Box<Expr>,
        args: Vec<Expr>,
    },
    /// Opération binaire
    Binary {
        left: Box<Expr>,
        op: BinaryOp,
        right: Box<Expr>,
    },
    /// Opération unaire
    Unary {
        op: UnaryOp,
        expr: Box<Expr>,
    },
    /// Accès champ structure
    Field {
        expr: Box<Expr>,
        field: String,
    },
}

/// Valeurs littérales
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Literal {
    Int(i64),
    Float(f64),
    Bool(bool),
    Str(String),
    Null,
}

/// Opérateurs binaires
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum BinaryOp {
    Add, Sub, Mul, Div, Mod,
    Eq, Ne, Lt, Le, Gt, Ge,
    And, Or,
}

/// Opérateurs unaires
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum UnaryOp {
    Neg,   // -x
    Not,   // !x
}

/// Types du langage Vitte
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Type {
    Int,
    Float,
    Bool,
    Str,
    Void,
    Custom(String),
}
