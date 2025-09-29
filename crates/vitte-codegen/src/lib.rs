//! vitte-codegen — Génération de code pour Vitte
//!
//! Cible : transformer l’IR en code exécutable ou représentations intermédiaires.
//!
//! Backends supportés (features):
//! - `asm`        : émission assembleur brut (x86_64, aarch64)
//! - `cranelift`  : JIT et code natif via Cranelift
//! - `llvm`       : génération LLVM IR et code machine
//!
//! Options :
//! - `debug-dumps` pour tracer IR et dumps backend
//! - `serde` pour sérialiser les artefacts
//!
//! Organisation :
//! - `traits.rs`   : abstraction Backend
//! - `asm.rs`      : backend assembleur
//! - `cranelift.rs`: backend Cranelift
//! - `llvm.rs`     : backend LLVM
//! - `dispatcher.rs`: sélection dynamique selon config cible
//!
//! API publique :
//! - [`CodegenConfig`], [`CodegenTarget`], [`CodegenBackend`]
//! - [`compile_module`], [`emit_object`], [`emit_asm`]

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

pub mod traits;
pub mod dispatcher;

#[cfg(feature = "asm")]
pub mod asm;

#[cfg(feature = "cranelift")]
pub mod cranelift;

#[cfg(feature = "llvm")]
pub mod llvm;

use vitte_ir::Module;
use vitte_errors::Result;

pub use traits::{CodegenBackend, CodegenTarget, CodegenConfig};

/// Compile un module IR en objet binaire.
pub fn compile_module(module: &Module, cfg: &CodegenConfig) -> Result<Vec<u8>> {
    dispatcher::compile(module, cfg)
}

/// Émet de l’assembleur texte.
pub fn emit_asm(module: &Module, cfg: &CodegenConfig) -> Result<String> {
    dispatcher::emit_asm(module, cfg)
}

/// Émet un fichier objet prêt à l’édition de liens.
pub fn emit_object(module: &Module, cfg: &CodegenConfig) -> Result<Vec<u8>> {
    dispatcher::emit_object(module, cfg)
}

#[cfg(test)]
mod tests {
    use super::*;
    use vitte_ir::builder::ModuleBuilder;

    #[test]
    fn smoke_codegen_empty() {
        let mut builder = ModuleBuilder::new("test");
        let module = builder.finish();
        let cfg = CodegenConfig::default_target(CodegenTarget::X86_64);
        let obj = compile_module(&module, &cfg).expect("compile ok");
        assert!(!obj.is_empty());
    }
}