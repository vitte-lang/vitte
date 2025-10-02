//! vitte-frontend — pipeline frontend du compilateur Vitte
//!
//! Rôle :
//! - Enchaîner lexer → parser → AST → analyse sémantique.
//! - Fournir une API haut-niveau pour compiler du code source en AST validé.
//! - Gérer erreurs lexicales/syntaxiques/sémantiques.
//!
//! Exemple d’utilisation :
//! ```ignore
//! use vitte_frontend::compile_to_ast;
//! let src = "fn main() { 1+2 }";
//! let ast = compile_to_ast(src).unwrap();
//! println!("{ast:#?}");
//! ```

#![deny(missing_docs)]

use vitte_core::Span;
use vitte_lexer::{Lexer, Token};
use vitte_parser::{Parser, ParseError};
use vitte_ast::Ast;

/// Résultat standard pour le frontend.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Erreurs possibles dans le frontend.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Erreur lexicale.
    #[error("lexical error at {0:?}: {1}")]
    Lex(Span, String),
    /// Erreur syntaxique.
    #[error("parse error: {0}")]
    Parse(#[from] ParseError),
    /// Erreur sémantique.
    #[error("semantic error: {0}")]
    Semantic(String),
}

/// Compile une source en AST validé.
pub fn compile_to_ast(src: &str) -> Result<Ast> {
    // Étape 1: lexing
    let lexer = Lexer::new(src);
    let tokens: Vec<Token> = lexer.collect::<core::result::Result<_, _>>()
        .map_err(|(sp, msg)| Error::Lex(sp, msg))?;

    // Étape 2: parsing
    let mut parser = Parser::new(&tokens);
    let ast = parser.parse().map_err(Error::Parse)?;

    // Étape 3: analyse sémantique (simplifiée)
    semantic_check(&ast)?;

    Ok(ast)
}

/// Vérifie quelques règles sémantiques basiques.
/// Ici, stub simplifié.
fn semantic_check(ast: &Ast) -> Result<()> {
    if ast.items.is_empty() {
        return Err(Error::Semantic("program is empty".into()));
    }
    Ok(())
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn simple_fn() {
        let src = "fn main() { 42 }";
        let ast = compile_to_ast(src);
        assert!(ast.is_ok());
    }

    #[test]
    fn empty_program() {
        let src = "";
        let ast = compile_to_ast(src);
        assert!(matches!(ast, Err(Error::Semantic(_))));
    }
}