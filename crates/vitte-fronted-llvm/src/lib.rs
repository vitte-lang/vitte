//! vitte-fronted-llvm — stub minimal sans dépendances externes.
//!
//! Ce crate compile sans LLVM ni `thiserror`. Ajoutez des features plus tard.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic)]
#![allow(clippy::module_name_repetitions)]

use core::fmt;
use std::vec::Vec;

/// Erreurs spécifiques au backend LLVM (version sans dépendances).
#[derive(Debug)]
pub enum LlvmError {
    /// Le backend LLVM n'est pas disponible.
    Unavailable,
    /// Erreur lors de la manipulation d'un IR LLVM (message brut).
    Ir(String),
    /// Erreur I/O lors de l'émission de fichiers (message brut).
    Io(String),
}

impl fmt::Display for LlvmError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            LlvmError::Unavailable => write!(f, "LLVM backend unavailable"),
            LlvmError::Ir(s) => write!(f, "LLVM IR error: {s}"),
            LlvmError::Io(s) => write!(f, "IO error: {s}"),
        }
    }
}


/// Résultat spécialisé du backend.
pub type Result<T, E = LlvmError> = core::result::Result<T, E>;

/// Backend LLVM minimal de test.
pub struct LlvmBackend;

impl LlvmBackend {
    /// Crée un backend.
    pub fn new(_name: &str) -> Self { LlvmBackend }
    /// Abaissement d'AST — no-op ou erreur si attendu.
    pub fn lower_ast(&mut self, _ast: &Ast) -> Result<()> {
        Err(LlvmError::Unavailable)
    }
    /// Optimisation — no-op.
    pub fn optimize_default(&self) {}
    /// Émet un objet — no-op ou erreur.
    pub fn emit_object(&self, _path: &str) -> Result<()> {
        Err(LlvmError::Unavailable)
    }
    /// Représentation textuelle — vide.
    pub fn to_string(&self) -> String { String::new() }
}

/* --------------------------------------------------------------------- */
/* Types minimaux pour compiler sans dépendre d’autres crates            */
/* --------------------------------------------------------------------- */

/// AST minimal local, pour éviter une dépendance forte à `vitte-ast`.
#[derive(Debug, Default, Clone)]
pub struct Ast {
    /// Noeuds fictifs pour le stub.
    pub items: Vec<()>,
}

/* --------------------------------------------------------------------- */
/* Tests                                                                 */
/* --------------------------------------------------------------------- */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stub_compiles_without_llvm() {
        let mut be = LlvmBackend::new("unit");
        let ast = Ast { items: vec![] };
        let _ = be.lower_ast(&ast);
        let _ = be.emit_object("out.o");
        let _ = be.to_string();
        be.optimize_default();
    }
}