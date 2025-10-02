//! vitte-frontend — pipeline frontend du compilateur Vitte (stub).
//!
//! Cette implémentation temporaire ne réalise pas encore le lexing/parsing
//! complet. Elle fournit toutefois une surface API stable (`compile_to_ast`)
//! utilisée par les autres crates du workspace. Les évolutions futures
//! remplaceront ce squelette par un vrai pipeline.

#![deny(missing_docs)]
#![forbid(unsafe_code)]

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

/// Résultat standard pour le frontend.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Représentation d’AST minimaliste retournée par le stub.
#[derive(Debug, Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(Serialize, Deserialize))]
pub struct Ast {
    /// Source brute fournie par l’utilisateur.
    pub source: String,
}

impl Ast {
    /// Construit un AST rudimentaire à partir d’une source.
    pub fn new(source: impl Into<String>) -> Self {
        Self { source: source.into() }
    }
}

/// Erreurs possibles dans le frontend stub.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Source vide rejetée pour éviter les états incohérents.
    #[error("source is empty")]
    EmptySource,
    /// Fonctionnalité encore absente.
    #[error("frontend functionality unavailable: {0}")]
    Unsupported(&'static str),
}

/// Compile une source en AST (stub).
///
/// Retourne une structure contenant simplement la source pour l’instant, ou
/// une erreur si la chaîne est vide.
pub fn compile_to_ast(src: &str) -> Result<Ast> {
    if src.trim().is_empty() {
        return Err(Error::EmptySource);
    }
    Ok(Ast::new(src))
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn non_empty_source_ok() {
        let src = "fn main() { 42 }";
        let ast = compile_to_ast(src).unwrap();
        assert_eq!(ast.source, src);
    }

    #[test]
    fn empty_source_err() {
        let ast = compile_to_ast("   \n\t");
        assert!(matches!(ast, Err(Error::EmptySource)));
    }
}
