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

impl Span {
    /// Construit un nouveau `Span` à partir d'une position (ligne, colonne, offset).
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
    /// Nom de la fonction.
    pub name: String,
    /// Paramètres positionnels de la fonction.
    pub params: Vec<Param>,
    /// Type de retour attendu (`None` → `void`).
    pub return_type: Option<Type>,
    /// Corps de la fonction.
    pub body: Block,
    /// Localisation de la déclaration.
    pub span: Option<Span>,
}

/// Paramètre de fonction
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Param {
    /// Nom du paramètre.
    pub name: String,
    /// Type attendu.
    pub ty: Type,
    /// Localisation du paramètre.
    pub span: Option<Span>,
}

/// Déclaration de constante
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct ConstDecl {
    /// Nom de la constante.
    pub name: String,
    /// Type explicite (si fourni).
    pub ty: Option<Type>,
    /// Valeur de la constante.
    pub value: Expr,
    /// Localisation de la déclaration.
    pub span: Option<Span>,
}

/// Déclaration de structure
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct StructDecl {
    /// Nom de la structure.
    pub name: String,
    /// Champs composant la structure.
    pub fields: Vec<Field>,
    /// Localisation de la déclaration.
    pub span: Option<Span>,
}

/// Déclaration d’énumération
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct EnumDecl {
    /// Nom de l'énumération.
    pub name: String,
    /// Variantes déclarées.
    pub variants: Vec<EnumVariant>,
    /// Localisation de la déclaration.
    pub span: Option<Span>,
}

/// Champ de structure
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Field {
    /// Nom du champ.
    pub name: String,
    /// Type du champ.
    pub ty: Type,
    /// Localisation du champ.
    pub span: Option<Span>,
}

/// Variante d’énumération
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct EnumVariant {
    /// Nom de la variante.
    pub name: String,
    /// Types des éléments associés à la variante.
    pub fields: Vec<Type>,
    /// Localisation de la variante.
    pub span: Option<Span>,
}

/// Un bloc d’instructions
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Block {
    /// Instructions contenues dans le bloc.
    pub stmts: Vec<Stmt>,
    /// Localisation du bloc.
    pub span: Option<Span>,
}

/// Une instruction
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Stmt {
    /// Déclaration de variable
    Let {
        /// Nom de la variable.
        name: String,
        /// Type annoté (si fourni).
        ty: Option<Type>,
        /// Valeur initiale.
        value: Option<Expr>,
        /// Localisation de la déclaration.
        span: Option<Span>,
    },
    /// Expression seule (souvent un appel)
    Expr(Expr),
    /// Retour d’une fonction
    Return(Option<Expr>, Option<Span>),
    /// Boucle `while`
    While {
        /// Condition évaluée à chaque itération.
        condition: Expr,
        /// Corps de la boucle.
        body: Block,
        /// Localisation de la boucle.
        span: Option<Span>,
    },
    /// Boucle `for`
    For {
        /// Nom de la variable itérée.
        var: String,
        /// Expression donnant l'itérable.
        iter: Expr,
        /// Corps de la boucle.
        body: Block,
        /// Localisation de la boucle.
        span: Option<Span>,
    },
    /// Conditionnelle `if ... else`
    If {
        /// Expression conditionnelle.
        condition: Expr,
        /// Bloc exécuté si la condition est vraie.
        then_block: Block,
        /// Bloc optionnel exécuté sinon.
        else_block: Option<Block>,
        /// Localisation de l'instruction.
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
        /// Expression représentant la fonction appelée.
        func: Box<Expr>,
        /// Arguments passés à l'appel.
        args: Vec<Expr>,
    },
    /// Opération binaire
    Binary {
        /// Opérande gauche.
        left: Box<Expr>,
        /// Opérateur appliqué.
        op: BinaryOp,
        /// Opérande droite.
        right: Box<Expr>,
    },
    /// Opération unaire
    Unary {
        /// Opérateur unaire appliqué.
        op: UnaryOp,
        /// Expression ciblée par l'opérateur.
        expr: Box<Expr>,
    },
    /// Accès champ structure
    Field {
        /// Expression support (struct ou tuple).
        expr: Box<Expr>,
        /// Nom du champ accédé.
        field: String,
    },
}

/// Valeurs littérales
#[derive(Debug, Clone, PartialEq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Literal {
    /// Entier signé 64 bits.
    Int(i64),
    /// Nombre flottant 64 bits.
    Float(f64),
    /// Booléen.
    Bool(bool),
    /// Chaîne UTF-8.
    Str(String),
    /// Valeur nulle (`null`).
    Null,
}

/// Opérateurs binaires
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum BinaryOp {
    /// Addition.
    Add,
    /// Soustraction.
    Sub,
    /// Multiplication.
    Mul,
    /// Division.
    Div,
    /// Modulo.
    Mod,
    /// Égalité.
    Eq,
    /// Différence.
    Ne,
    /// Inférieur strict.
    Lt,
    /// Inférieur ou égal.
    Le,
    /// Supérieur strict.
    Gt,
    /// Supérieur ou égal.
    Ge,
    /// Conjonction logique.
    And,
    /// Disjonction logique.
    Or,
}

/// Opérateurs unaires
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum UnaryOp {
    /// Négation arithmétique (`-x`).
    Neg,
    /// Négation logique (`!x`).
    Not,
}

/// Types du langage Vitte
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub enum Type {
    /// Type entier signé.
    Int,
    /// Type flottant.
    Float,
    /// Type booléen.
    Bool,
    /// Type chaîne.
    Str,
    /// Absence de valeur (`void`).
    Void,
    /// Type défini par l'utilisateur.
    Custom(String),
}
