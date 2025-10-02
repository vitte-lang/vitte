#![deny(unsafe_code)]
#![forbid(clippy::unwrap_used)]
//! vitte-backend-llvm — Backend LLVM pour Vitte.
//!
//! - API publique stable: [`Backend`], [`Config`], [`ArtifactKind`]
//! - Implémentation `LlvmBackend` activée par la feature `inkwell`
//! - Emission: objet, bitcode LLVM, IR LLVM, JIT (exécution symbolique simple)
//! - Pipelines d’optimisation: O0..O3 (LLVM function/module pass managers)
//!
//! ## Exemple rapide (avec `--features inkwell`)
//! ```no_run
//! use vitte_backend_llvm::{Backend, Config, ArtifactKind};
//! let cfg = Config::default().opt_level(OptimizationLevel::O2);
//! let mut be = vitte_backend_llvm::LlvmBackend::new(cfg)?;
//! // Ici on aurait un module IR de Vitte. On se contente d’un stub interne:
//! let mod_id = be.create_demo_add_module("demo");
//! let obj = be.emit(ArtifactKind::Object)?;
//! std::fs::write("demo.o", obj)?;
//! # Ok::<(), Box<dyn std::error::Error>>(())
//! ```

use std::path::Path;
use std::sync::Arc;

use thiserror::Error;

/// Niveau d’optimisation.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum OptimizationLevel {
    O0,
    O1,
    O2,
    O3,
}

/// Type d’artefact à produire.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ArtifactKind {
    /// Fichier objet natif (ex: `.o`, `.obj`)
    Object,
    /// Bitcode LLVM binaire (`.bc`)
    Bitcode,
    /// IR LLVM textuel (`.ll`)
    LlvmIr,
}

/// Configuration du backend.
#[derive(Clone, Debug)]
pub struct Config {
    pub opt_level: OptimizationLevel,
    pub target_triple: Option<String>,
    pub data_layout: Option<String>,
    pub reloc_pic: bool,
    pub verify: bool,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            opt_level: OptimizationLevel::O1,
            target_triple: None,
            data_layout: None,
            reloc_pic: true,
            verify: true,
        }
    }
}

impl Config {
    pub fn opt_level(mut self, lvl: OptimizationLevel) -> Self {
        self.opt_level = lvl; self
    }
    pub fn target_triple<S: Into<String>>(mut self, triple: S) -> Self {
        self.target_triple = Some(triple.into()); self
    }
    pub fn data_layout<S: Into<String>>(mut self, dl: S) -> Self {
        self.data_layout = Some(dl.into()); self
    }
    pub fn reloc_pic(mut self, pic: bool) -> Self {
        self.reloc_pic = pic; self
    }
    pub fn verify(mut self, v: bool) -> Self {
        self.verify = v; self
    }
}

/// Erreurs du backend.
#[derive(Error, Debug)]
pub enum BackendError {
    #[error("fonctionnalité LLVM désactivée: recompilez avec `--features inkwell`")]
    FeatureDisabled,
    #[error("initialisation LLVM: {0}")]
    Init(String),
    #[error("construction module: {0}")]
    Build(String),
    #[error("vérification module: {0}")]
    Verify(String),
    #[error("émission artefact: {0}")]
    Emit(String),
    #[error("JIT: {0}")]
    Jit(String),
    #[error(transparent)]
    Io(#[from] std::io::Error),
}

pub type Result<T> = std::result::Result<T, BackendError>;

/// Trait backend minimal pour Vitte.
pub trait Backend {
    /// Construit depuis une [`Config`].
    fn new(cfg: Config) -> Result<Self> where Self: Sized;

    /// Ajoute/abaisse le module IR courant.  
    /// Pour démonstration, on expose un stub `create_demo_add_module`.
    fn create_demo_add_module(&mut self, name: &str) -> Result<()>;

    /// Emet un artefact selon [`ArtifactKind`]. Retourne les bytes.
    fn emit(&mut self, kind: ArtifactKind) -> Result<Vec<u8>>;

    /// Ecrit l’artefact dans le fichier donné.
    fn emit_to_file<P: AsRef<Path>>(&mut self, kind: ArtifactKind, path: P) -> Result<()> {
        let bytes = self.emit(kind)?;
        std::fs::write(path, bytes)?;
        Ok(())
    }

    /// Optionnel: exécution JIT d’un symbole `fn(i32,i32)->i32`.
    fn jit_run_i32_binop(&mut self, symbol: &str, a: i32, b: i32) -> Result<i32>;
}

#[cfg(feature = "inkwell")]
mod llvm_impl {
    use super::*;
    use inkwell::context::Context;
    use inkwell::module::Module;
    use inkwell::passes::{PassManager, PassManagerBuilder};
    use inkwell::targets::{CodeModel, FileType, InitializationConfig, RelocMode, Target, TargetMachine, TargetTriple};
    use inkwell::OptimizationLevel as LlvmOpt;
    use inkwell::execution_engine::ExecutionEngine;
    use inkwell::values::FunctionValue;

    /// Implémentation LLVM du backend.
    pub struct LlvmBackend {
        cfg: Config,
        ctx: Arc<Context>,
        module: Module,
        fpm: PassManager<FunctionValue>,
        mpm: PassManager<Module>,
        tm: TargetMachine,
        ee: Option<ExecutionEngine>,
    }

    impl LlvmBackend {
        fn map_opt(o: OptimizationLevel) -> LlvmOpt {
            match o {
                OptimizationLevel::O0 => LlvmOpt::None,
                OptimizationLevel::O1 => LlvmOpt::Less,
                OptimizationLevel::O2 => LlvmOpt::Default,
                OptimizationLevel::O3 => LlvmOpt::Aggressive,
            }
        }

        fn map_reloc(pic: bool) -> RelocMode {
            if pic { RelocMode::PIC } else { RelocMode::Static }
        }

        fn init_llvm() -> Result<()> {
            Target::initialize_all(&InitializationConfig::default());
            Ok(())
        }

        fn build_pm(ctx: &Context, module: &Module, lvl: OptimizationLevel) -> (PassManager<FunctionValue>, PassManager<Module>) {
            let fpm = PassManager::create(module);
            let mpm = PassManager::create(());
            let mut pmb = PassManagerBuilder::create();
            pmb.set_optimization_level(Self::map_opt(lvl));
            pmb.populate_module_pass_manager(&mpm);
            pmb.populate_function_pass_manager(&fpm);
            fpm.initialize();
            (fpm, mpm)
        }

        fn target_machine(cfg: &Config) -> Result<TargetMachine> {
            let triple = cfg.target_triple
                .as_deref()
                .map(TargetTriple::create)
                .unwrap_or_else(|| TargetMachine::get_default_triple());

            let target = Target::from_triple(&triple)
                .map_err(|e| BackendError::Init(format!("Target::from_triple: {e}")))?;
            let cpu = "generic";
            let features = "";

            let tm = target
                .create_target_machine(
                    &triple,
                    cpu,
                    features,
                    Self::map_opt(cfg.opt_level),
                    Self::map_reloc(cfg.reloc_pic),
                    CodeModel::Default,
                )
                .ok_or_else(|| BackendError::Init("create_target_machine".into()))?;

            Ok(tm)
        }

        fn verify_module(module: &Module) -> Result<()> {
            if let Err(e) = module.verify() {
                return Err(BackendError::Verify(e.to_string()));
            }
            Ok(())
        }

        fn ensure_engine(&mut self) -> Result<()> {
            if self.ee.is_some() { return Ok(()); }
            let ee = self.module
                .create_jit_execution_engine(Self::map_opt(self.cfg.opt_level))
                .map_err(|e| BackendError::Jit(e.to_string()))?;
            self.ee = Some(ee);
            Ok(())
        }

        /// Démo: crée une fonction `add_i32(a:i32,b:i32)->i32`.
        fn build_demo_add(&self, name: &str) -> Result<FunctionValue> {
            let i32t = self.ctx.i32_type();
            let fn_ty = i32t.fn_type(&[i32t.into(), i32t.into()], false);
            let f = self.module.add_function(name, fn_ty, None);
            let entry = self.ctx.append_basic_block(f, "entry");
            let builder = self.ctx.create_builder();
            builder.position_at_end(entry);
            let a = f.get_nth_param(0).unwrap().into_int_value();
            let b = f.get_nth_param(1).unwrap().into_int_value();
            let sum = builder.build_int_add(a, b, "sum").map_err(|e| BackendError::Build(e.to_string()))?;
            builder.build_return(Some(&sum)).map_err(|e| BackendError::Build(e.to_string()))?;
            Ok(f)
        }
    }

    impl Backend for LlvmBackend {
        fn new(cfg: Config) -> Result<Self> {
            Self::init_llvm()?;
            let ctx = Arc::new(Context::create());
            let module = ctx.create_module("vitte");
            if let Some(dl) = &cfg.data_layout {
                module.set_data_layout(dl);
            }
            if let Some(tr) = &cfg.target_triple {
                module.set_triple(&TargetTriple::create(tr));
            } else {
                module.set_triple(&TargetMachine::get_default_triple());
            }

            let (fpm, mpm) = Self::build_pm(&ctx, &module, cfg.opt_level);
            let tm = Self::target_machine(&cfg)?;
            Ok(Self { cfg, ctx, module, fpm, mpm, tm, ee: None })
        }

        fn create_demo_add_module(&mut self, name: &str) -> Result<()> {
            let _ = self.build_demo_add(name)?;
            if self.cfg.verify {
                Self::verify_module(&self.module)?;
            }
            // Optimisations
            for f in self.module.get_functions() {
                self.fpm.run_on(&f);
            }
            self.mpm.run_on(&self.module);
            Ok(())
        }

        fn emit(&mut self, kind: ArtifactKind) -> Result<Vec<u8>> {
            match kind {
                ArtifactKind::Object => {
                    let buf = self.tm
                        .write_to_memory_buffer(&self.module, FileType::Object)
                        .map_err(|e| BackendError::Emit(e.to_string()))?;
                    Ok(buf.as_slice().to_vec())
                }
                ArtifactKind::Bitcode => {
                    let mb = self.module.write_bitcode_to_memory();
                    Ok(mb.as_slice().to_vec())
                }
                ArtifactKind::LlvmIr => {
                    let s = self.module.print_to_string();
                    Ok(s.to_bytes().to_vec())
                }
            }
        }

        fn jit_run_i32_binop(&mut self, symbol: &str, a: i32, b: i32) -> Result<i32> {
            self.ensure_engine()?;
            let ee = self.ee.as_ref().unwrap();
            unsafe {
                let addr = ee.get_function_address(symbol)
                    .map_err(|e| BackendError::Jit(e.to_string()))?;
                if addr == 0 {
                    return Err(BackendError::Jit(format!("symbole introuvable: {symbol}")));
                }
                // Signature: extern "C" fn(i32,i32)->i32
                let f: extern "C" fn(i32, i32) -> i32 = std::mem::transmute(addr);
                Ok(f(a, b))
            }
        }
    }

    // Réexport public quand la feature est active.
    pub use LlvmBackend;
}

#[cfg(not(feature = "inkwell"))]
mod no_llvm_impl {
    use super::*;

    /// Stub désactivé sans la feature `inkwell`.
    pub struct LlvmBackend {
        _cfg: Config,
    }

    impl Backend for LlvmBackend {
        fn new(cfg: Config) -> Result<Self> {
            let _ = cfg;
            Err(BackendError::FeatureDisabled)
        }
        fn create_demo_add_module(&mut self, _name: &str) -> Result<()> {
            Err(BackendError::FeatureDisabled)
        }
        fn emit(&mut self, _kind: ArtifactKind) -> Result<Vec<u8>> {
            Err(BackendError::FeatureDisabled)
        }
        fn jit_run_i32_binop(&mut self, _symbol: &str, _a: i32, _b: i32) -> Result<i32> {
            Err(BackendError::FeatureDisabled)
        }
    }

    pub use LlvmBackend;
}

// Surface publique: type concret exporté selon feature.
pub use cfg_if::cfg_if;
cfg_if! {
    if #[cfg(feature = "inkwell")] {
        pub use llvm_impl::LlvmBackend;
    } else {
        pub use no_llvm_impl::LlvmBackend;
    }
}

/// Helpers utilitaires communs.
pub mod util {
    /// Définit un triple par défaut raisonnable si la cible n’est pas spécifiée.
    pub fn default_triple() -> String {
        inkwell::targets::TargetMachine::get_default_triple().as_str().to_string()
    }

    /// Retourne vrai si la feature `inkwell` est active.
    pub fn has_llvm() -> bool {
        cfg!(feature = "inkwell")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn config_mutators() {
        let c = Config::default()
            .opt_level(OptimizationLevel::O3)
            .target_triple("x86_64-apple-darwin")
            .data_layout("e-m:o-i64:64-f80:128-n8:16:32:64-S128")
            .reloc_pic(false)
            .verify(false);
        assert!(matches!(c.opt_level, OptimizationLevel::O3));
        assert_eq!(c.target_triple.as_deref(), Some("x86_64-apple-darwin"));
        assert_eq!(c.reloc_pic, false);
        assert_eq!(c.verify, false);
        assert!(c.data_layout.is_some());
    }

    #[cfg(feature = "inkwell")]
    #[test]
    fn build_and_emit_ir() {
        let mut be = crate::LlvmBackend::new(Config::default()).expect("llvm init");
        be.create_demo_add_module("add_i32").expect("build");
        let ir = be.emit(ArtifactKind::LlvmIr).expect("emit ll");
        let s = String::from_utf8(ir).unwrap();
        assert!(s.contains("define i32 @add_i32(i32 %0, i32 %1)"));
    }

    #[cfg(feature = "inkwell")]
    #[test]
    fn jit_add() {
        let mut be = crate::LlvmBackend::new(Config::default()).expect("llvm init");
        be.create_demo_add_module("add_i32").expect("build");
        let r = be.jit_run_i32_binop("add_i32", 7, 35).expect("jit");
        assert_eq!(r, 42);
    }

    #[cfg(not(feature = "inkwell"))]
    #[test]
    fn feature_guard() {
        let e = crate::LlvmBackend::new(Config::default()).unwrap_err();
        matches!(e, BackendError::FeatureDisabled);
    }
}