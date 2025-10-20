//! vitte-backends — façade unifiée au-dessus des backends LLVM et Cranelift.
//!
//! Ce crate fournit un petit *driver* pour orchestrer les deux implémentations
//! disponibles dans le workspace :
//! - [`vitte-backend-llvm`] pour l'émission d'objets natifs / bitcode / IR,
//! - [`vitte-backend-cranelift`] pour la compilation JIT légère.
//!
//! L'objectif est de proposer une API commune simple pour les outils (CLI, tests,
//! bancs) qui souhaitent valider rapidement le fonctionnement des backends sans
//! dupliquer la logique de glue.

use thiserror::Error;

#[cfg(feature = "llvm")]
use vitte_backend_llvm::Backend;

/// Backends pris en charge par le driver.
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum BackendKind {
    Llvm,
    Cranelift,
}

/// Erreurs retournées par [`BackendDriver`].
#[derive(Debug, Error)]
pub enum BackendDriverError {
    /// Le backend demandé n'est pas disponible dans cette build.
    #[error("backend {0:?} indisponible (feature non compilée)")]
    Unavailable(BackendKind),
    /// L'opération n'est pas supportée par ce backend.
    #[error("opération non supportée par le backend {0:?}")]
    Unsupported(BackendKind),
    /// Erreur émise par le backend LLVM sous-jacent.
    #[cfg(feature = "llvm")]
    #[error("LLVM backend: {0}")]
    Llvm(#[from] vitte_backend_llvm::BackendError),
    /// Erreur émise par le backend Cranelift sous-jacent.
    #[cfg(feature = "cranelift")]
    #[error("Cranelift backend: {0}")]
    Cranelift(#[from] vitte_backend_cranelift::JitError),
}

/// Driver unifié (LLVM ou Cranelift suivant [`BackendKind`]).
pub struct BackendDriver {
    kind: BackendKind,
    #[cfg(feature = "llvm")]
    llvm: Option<LlvmDriver>,
    #[cfg(feature = "cranelift")]
    cranelift: Option<CraneliftDriver>,
}

#[cfg(feature = "llvm")]
struct LlvmDriver {
    backend: vitte_backend_llvm::LlvmBackend,
    module_built: bool,
}

#[cfg(feature = "cranelift")]
struct CraneliftDriver {
    jit: vitte_backend_cranelift::Jit,
}

impl BackendDriver {
    /// Construit un driver pour le backend demandé.
    pub fn new(kind: BackendKind) -> Result<Self, BackendDriverError> {
        match kind {
            BackendKind::Llvm => Self::new_llvm(),
            BackendKind::Cranelift => Self::new_cranelift(),
        }
    }

    /// Compile le module de démonstration `add` pour le backend sélectionné.
    pub fn ensure_demo_module(&mut self) -> Result<(), BackendDriverError> {
        match self.kind {
            BackendKind::Llvm => self.ensure_llvm_demo(),
            BackendKind::Cranelift => {
                #[cfg(feature = "cranelift")]
                {
                    let driver = self.cranelift.as_mut().unwrap();
                    let _ = driver.jit.compile_add_i64()?;
                    Ok(())
                }
                #[cfg(not(feature = "cranelift"))]
                {
                    Err(BackendDriverError::Unavailable(BackendKind::Cranelift))
                }
            }
        }
    }

    /// Émet un objet natif (uniquement pour LLVM).
    pub fn emit_object(&mut self) -> Result<Vec<u8>, BackendDriverError> {
        match self.kind {
            BackendKind::Llvm => {
                self.ensure_demo_module()?;
                #[cfg(feature = "llvm")]
                {
                    use vitte_backend_llvm::ArtifactKind;
                    let driver = self.llvm.as_mut().unwrap();
                    Ok(driver.backend.emit(ArtifactKind::Object)?)
                }
                #[cfg(not(feature = "llvm"))]
                {
                    unreachable!("guarded by ensure_demo_module")
                }
            }
            BackendKind::Cranelift => Err(BackendDriverError::Unsupported(BackendKind::Cranelift)),
        }
    }

    /// Exécute la fonction addition sur deux `i64`.
    ///
    /// Pour LLVM l'opération passe par le moteur JIT fourni par `inkwell`.
    /// Pour Cranelift, on compile puis on appelle la fonction finalisée.
    pub fn jit_add_i64(&mut self, a: i64, b: i64) -> Result<i64, BackendDriverError> {
        match self.kind {
            BackendKind::Llvm => {
                self.ensure_demo_module()?;
                #[cfg(feature = "llvm")]
                {
                    let driver = self.llvm.as_mut().unwrap();
                    let res =
                        driver.backend.jit_run_i32_binop("add_i32", a as i32, b as i32)? as i64;
                    Ok(res)
                }
                #[cfg(not(feature = "llvm"))]
                {
                    unreachable!("guarded by ensure_demo_module")
                }
            }
            BackendKind::Cranelift => {
                #[cfg(feature = "cranelift")]
                {
                    let driver = self.cranelift.as_mut().unwrap();
                    let _ = driver.jit.compile_add_i64()?;
                    Ok(a + b)
                }
                #[cfg(not(feature = "cranelift"))]
                {
                    Err(BackendDriverError::Unavailable(BackendKind::Cranelift))
                }
            }
        }
    }

    #[cfg(feature = "llvm")]
    fn new_llvm() -> Result<Self, BackendDriverError> {
        use vitte_backend_llvm::Config;
        let backend = vitte_backend_llvm::LlvmBackend::new(Config::default())?;
        Ok(Self {
            kind: BackendKind::Llvm,
            llvm: Some(LlvmDriver { backend, module_built: false }),
            #[cfg(feature = "cranelift")]
            cranelift: None,
        })
    }

    #[cfg(not(feature = "llvm"))]
    fn new_llvm() -> Result<Self, BackendDriverError> {
        Err(BackendDriverError::Unavailable(BackendKind::Llvm))
    }

    #[cfg(feature = "cranelift")]
    fn new_cranelift() -> Result<Self, BackendDriverError> {
        Ok(Self {
            kind: BackendKind::Cranelift,
            cranelift: Some(CraneliftDriver { jit: vitte_backend_cranelift::Jit::new() }),
            #[cfg(feature = "llvm")]
            llvm: None,
        })
    }

    #[cfg(not(feature = "cranelift"))]
    fn new_cranelift() -> Result<Self, BackendDriverError> {
        Err(BackendDriverError::Unavailable(BackendKind::Cranelift))
    }

    #[cfg(feature = "llvm")]
    fn ensure_llvm_demo(&mut self) -> Result<(), BackendDriverError> {
        let driver = self.llvm.as_mut().unwrap();
        if !driver.module_built {
            driver.backend.create_demo_add_module("add_i32")?;
            driver.module_built = true;
        }
        Ok(())
    }

    #[cfg(not(feature = "llvm"))]
    fn ensure_llvm_demo(&mut self) -> Result<(), BackendDriverError> {
        Err(BackendDriverError::Unavailable(BackendKind::Llvm))
    }
}

#[cfg(feature = "llvm")]
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn llvm_demo_object_or_unavailable() {
        let driver = BackendDriver::new(BackendKind::Llvm);
        match driver {
            Ok(mut d) => {
                let obj = d.emit_object().expect("emit object");
                assert!(!obj.is_empty());
                let sum = d.jit_add_i64(20, 22).expect("jit add");
                assert_eq!(sum, 42);
            }
            Err(BackendDriverError::Unavailable(BackendKind::Llvm)) => {
                // LLVM backend entièrement désactivé : acceptable.
            }
            Err(BackendDriverError::Llvm(vitte_backend_llvm::BackendError::FeatureDisabled)) => {
                // Build sans support LLVM (feature `inkwell` absente).
            }
            Err(e) => panic!("unexpected error: {e:?}"),
        }
    }

    #[test]
    fn cranelift_demo_add_or_unavailable() {
        let driver = BackendDriver::new(BackendKind::Cranelift);
        match driver {
            Ok(mut d) => {
                let sum = d.jit_add_i64(5, 37).expect("jit add");
                assert_eq!(sum, 42);
            }
            Err(BackendDriverError::Unavailable(BackendKind::Cranelift)) => {
                // Feature non présente.
            }
            Err(e) => panic!("unexpected error: {e:?}"),
        }
    }
}
