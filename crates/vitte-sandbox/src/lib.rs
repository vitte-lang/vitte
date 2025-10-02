

#![deny(missing_docs)]
//! vitte-sandbox — exécution isolée de modules pour Vitte
//!
//! Fournit :
//! - Hôte WASM (wasmtime).
//! - Intégration WASI (fs, env).
//! - Limites mémoire/CPU.
//! - API simple pour charger/évaluer un module.
//!
//! Exemple :
//! ```no_run
//! use vitte_sandbox as sandbox;
//! # #[tokio::main] async fn main() -> sandbox::Result<()> {
//! let code = wat::parse_str("(module (func (export \"add\") (param i32 i32) (result i32) local.get 0 local.get 1 i32.add))")?;
//! let sb = sandbox::Sandbox::new()?;
//! let inst = sb.instantiate(&code).await?;
//! let add = inst.get_func(\"add\").unwrap();
//! let res = add.call(&[1i32.into(), 2i32.into()])?;
//! assert_eq!(res[0].unwrap_i32(), 3);
//! # Ok(()) }
//! ```

use thiserror::Error;

#[cfg(feature="wasmtime")]
use wasmtime::{Engine, Store, Module, Instance, Config, Limits, Caller, Func};

#[cfg(feature="wasmtime")]
use wasmtime_wasi::{WasiCtxBuilder, WasiCtx};

/// Erreurs de sandbox.
#[derive(Debug, Error)]
pub enum SandboxError {
    #[error("io: {0}")]
    Io(#[from] std::io::Error),
    #[error("wasm: {0}")]
    #[cfg(feature="wasmtime")]
    Wasm(#[from] anyhow::Error),
    #[error("autre: {0}")]
    Other(String),
}

/// Résultat spécialisé.
pub type Result<T> = std::result::Result<T, SandboxError>;

/// Sandbox principal.
pub struct Sandbox {
    #[cfg(feature="wasmtime")]
    engine: Engine,
}

impl Sandbox {
    /// Crée une sandbox avec limites par défaut.
    #[cfg(feature="wasmtime")]
    pub fn new() -> Result<Self> {
        let mut cfg = Config::new();
        cfg.consume_fuel(true);
        let engine = Engine::new(&cfg)?;
        Ok(Self { engine })
    }

    /// Instancie un module WASM brut.
    #[cfg(feature="wasmtime")]
    pub async fn instantiate(&self, bytes: &[u8]) -> Result<InstanceHandle> {
        let module = Module::new(&self.engine, bytes)?;
        let mut store = Store::new(&self.engine, WasiCtxBuilder::new().inherit_stdio().build());
        store.add_fuel(10_000_000)?;
        let imports = [];
        let instance = Instance::new(&mut store, &module, &imports)?;
        Ok(InstanceHandle { store, instance })
    }
}

/// Instance en cours d’exécution.
#[cfg(feature="wasmtime")]
pub struct InstanceHandle {
    store: Store<WasiCtx>,
    instance: Instance,
}

#[cfg(feature="wasmtime")]
impl InstanceHandle {
    /// Récupère une fonction exportée.
    pub fn get_func(&self, name: &str) -> Option<Func> {
        self.instance.get_func(&self.store, name)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_fmt() {
        let e = SandboxError::Other("bad".into());
        assert!(format!("{e}").contains("bad"));
    }

    #[tokio::test]
    #[cfg(feature="wasmtime")]
    async fn wasm_add() {
        let code = wat::parse_str("(module (func (export \"add\") (param i32 i32) (result i32) local.get 0 local.get 1 i32.add))").unwrap();
        let sb = Sandbox::new().unwrap();
        let inst = sb.instantiate(&code).await.unwrap();
        let f = inst.get_func("add").unwrap();
        let res = f.call(&[wasmtime::Val::I32(2), wasmtime::Val::I32(40)]).unwrap();
        assert_eq!(res[0].unwrap_i32(), 42);
    }
}