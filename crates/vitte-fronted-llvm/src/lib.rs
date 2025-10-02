//! vitte-fronted-llvm — génération LLVM IR pour Vitte
//!
//! Objectif :
//! - Abaisser l’AST/HIR du frontend vers LLVM IR via inkwell.
//! - Offrir une API simple pour générer un module LLVM, appliquer des passes
//!   d’optimisation et émettre du code machine (objet, asm, binaire).
//!
//! Features :
//! - `inkwell` : utilise les bindings safe LLVM.
//! - `serde`   : sérialise les modules LLVM (métadonnées uniquement).
//!
//! Exemple d’utilisation :
//! ```ignore
//! use vitte_fronted_llvm::LlvmBackend;
//! use vitte_ast::Ast;
//! let ast = parse_some_ast();
//! let mut backend = LlvmBackend::new("demo");
//! backend.lower_ast(&ast).unwrap();
//! backend.optimize_default();
//! backend.emit_object("out.o").unwrap();
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;

use vitte_ast::Ast;
use vitte_frontend::Error;

/// Résultat typé.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Erreurs spécifiques au backend LLVM.
#[derive(Debug, thiserror::Error)]
pub enum LlvmError {
    #[error("LLVM backend unavailable (feature inkwell désactivée)")]
    Unavailable,
    #[error("LLVM IR error: {0}")]
    Ir(String),
    #[error("IO error: {0}")]
    Io(String),
}

#[cfg(feature = "inkwell")]
use inkwell::{
    context::Context,
    module::Module,
    passes::PassManager,
    targets::{InitializationConfig, Target, TargetMachine, FileType},
    OptimizationLevel,
};

/// Backend LLVM pour Vitte.
pub struct LlvmBackend<'ctx> {
    #[cfg(feature = "inkwell")]
    context: &'ctx Context,
    #[cfg(feature = "inkwell")]
    module: Module<'ctx>,
}

impl<'ctx> LlvmBackend<'ctx> {
    /// Crée un nouveau backend.
    #[cfg(feature = "inkwell")]
    pub fn new(name: &str) -> Self {
        let context = Box::leak(Box::new(Context::create()));
        let module = context.create_module(name);
        Self { context, module }
    }

    /// Abaisse un AST en LLVM IR (stub simplifié).
    #[cfg(feature = "inkwell")]
    pub fn lower_ast(&mut self, _ast: &Ast) -> core::result::Result<(), LlvmError> {
        // TODO: parcourir l’AST et générer instructions LLVM
        Ok(())
    }

    /// Applique des passes d’optimisation par défaut.
    #[cfg(feature = "inkwell")]
    pub fn optimize_default(&self) {
        let fpm = PassManager::create(());
        fpm.add_instruction_combining_pass();
        fpm.add_reassociate_pass();
        fpm.add_gvn_pass();
        fpm.add_cfg_simplification_pass();
        fpm.initialize();
        // TODO: appliquer passes sur fonctions
    }

    /// Écrit un objet compilé (fichier .o).
    #[cfg(feature = "inkwell")]
    pub fn emit_object(&self, path: &str) -> core::result::Result<(), LlvmError> {
        Target::initialize_all(&InitializationConfig::default());
        let triple = TargetMachine::get_default_triple();
        let target = Target::from_triple(&triple).map_err(|e| LlvmError::Ir(format!("{e:?}")))?;
        let tm = target
            .create_target_machine(
                &triple,
                "generic",
                "",
                OptimizationLevel::Default,
                inkwell::targets::RelocMode::Default,
                inkwell::targets::CodeModel::Default,
            )
            .ok_or_else(|| LlvmError::Ir("cannot create target machine".into()))?;
        tm.write_to_file(&self.module, FileType::Object, std::path::Path::new(path))
            .map_err(|e| LlvmError::Io(format!("{e:?}")))
    }

    /// Retourne le code LLVM IR sous forme de String.
    #[cfg(feature = "inkwell")]
    pub fn to_string(&self) -> String {
        self.module.print_to_string().to_string()
    }
}

/// Stub si inkwell désactivé.
#[cfg(not(feature = "inkwell"))]
impl<'ctx> LlvmBackend<'ctx> {
    /// Création non supportée.
    pub fn new(_name: &str) -> Self {
        panic!("inkwell feature désactivée");
    }
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */

#[cfg(all(test, feature = "inkwell", feature = "std"))]
mod tests {
    use super::*;
    #[test]
    fn create_backend() {
        let mut be = LlvmBackend::new("test");
        let ast = Ast { items: Vec::new() };
        assert!(be.lower_ast(&ast).is_ok());
        be.optimize_default();
        let ir = be.to_string();
        assert!(ir.contains("module"));
    }
}