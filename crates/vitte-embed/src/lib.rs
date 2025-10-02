//! vitte-embed — API d’intégration du runtime Vitte
//!
//! Objectifs
//! - Intégrer le runtime Vitte dans une appli hôte (Rust, C, C++).
//! - Contrôler l’exécution (charger code, exécuter fonctions).
//! - Échanger des valeurs simples entre hôte et VM.
//! - Option `ffi` expose une ABI C minimale (`vitte_embed_*`).
//!
//! Exemple Rust :
//! ```ignore
//! use vitte_embed::{Vm, Value};
//! let mut vm = Vm::new();
//! vm.eval("1+2").unwrap();
//! let v = vm.call("main", &[]).unwrap();
//! println!("{:?}", v);
//! ```

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

extern crate alloc;
use alloc::{string::String, vec::Vec};

use vitte_runtime::{Runtime, Value as RtValue};

/// Résultat.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Erreurs d’intégration.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// Code invalide.
    #[error("invalid code: {0}")]
    Invalid(String),
    /// Appel échoué.
    #[error("call failed: {0}")]
    Call(String),
    /// Runtime erreur.
    #[error("runtime error: {0}")]
    Runtime(String),
    /// Fonctionnalité absente.
    #[error("unsupported: {0}")]
    Unsupported(&'static str),
}

/// Valeurs simples échangées entre VM et hôte.
#[derive(Debug, Clone)]
pub enum Value {
    /// Entier.
    Int(i64),
    /// Booléen.
    Bool(bool),
    /// Chaîne.
    Str(String),
    /// Unit.
    Unit,
}

/// Machine virtuelle embarquée.
pub struct Vm {
    rt: Runtime,
}

impl Vm {
    /// Crée une nouvelle VM.
    pub fn new() -> Self {
        Self { rt: Runtime::new() }
    }

    /// Évalue du code source et l’ajoute au runtime.
    pub fn eval(&mut self, src: &str) -> Result<()> {
        let _ = src;
        Err(Error::Unsupported("eval"))
    }

    /// Appelle une fonction par nom avec arguments.
    pub fn call(&mut self, name: &str, args: &[Value]) -> Result<Value> {
        let rt_args: Vec<RtValue> = args.iter().map(to_rt_value).collect();
        let value = self
            .rt
            .call(name, &rt_args)
            .map_err(|e| Error::Call(format!("{e:?}")))?;
        from_rt_value(value)
    }
}

fn to_rt_value(v: &Value) -> RtValue {
    match v {
        Value::Int(i) => RtValue::I64(*i),
        Value::Bool(b) => RtValue::Bool(*b),
        Value::Str(s) => RtValue::Str(s.clone()),
        Value::Unit => RtValue::Null,
    }
}

fn from_rt_value(v: RtValue) -> Result<Value> {
    Ok(match v {
        RtValue::I64(i) => Value::Int(i),
        RtValue::Bool(b) => Value::Bool(b),
        RtValue::Str(s) => Value::Str(s),
        RtValue::Null => Value::Unit,
        RtValue::F64(_) | RtValue::Bytes(_) => return Err(Error::Unsupported("value type")),
    })
}

/* ------------------------------------------------------------------------- */
/* FFI minimal (C ABI)                                                       */
/* ------------------------------------------------------------------------- */
#[cfg(feature = "ffi")]
pub mod ffi {
    use super::*;
    use core::ffi::c_char;
    use std::os::raw::c_int;
    use std::ptr;

    /// Handle VM opaque.
    pub struct VmHandle(Vm);

    /// Crée une VM et retourne un pointeur opaque.
    #[no_mangle]
    pub extern "C" fn vitte_embed_new() -> *mut VmHandle {
        Box::into_raw(Box::new(VmHandle(Vm::new())))
    }

    /// Détruit une VM.
    #[no_mangle]
    pub extern "C" fn vitte_embed_free(vm: *mut VmHandle) {
        if !vm.is_null() {
            unsafe { drop(Box::from_raw(vm)); }
        }
    }

    /// Évalue une chaîne source UTF-8.
    #[no_mangle]
    pub extern "C" fn vitte_embed_eval(vm: *mut VmHandle, src: *const c_char) -> c_int {
        if vm.is_null() || src.is_null() {
            return -1;
        }
        let s = unsafe { std::ffi::CStr::from_ptr(src).to_string_lossy().into_owned() };
        let handle = unsafe { &mut *vm };
        match handle.0.eval(&s) {
            Ok(_) => 0,
            Err(_) => -1,
        }
    }

    /// Appel simplifié de fonction sans args, renvoie int.
    #[no_mangle]
    pub extern "C" fn vitte_embed_call_int(vm: *mut VmHandle, name: *const c_char, out: *mut i64) -> c_int {
        if vm.is_null() || name.is_null() || out.is_null() {
            return -1;
        }
        let nm = unsafe { std::ffi::CStr::from_ptr(name).to_string_lossy().into_owned() };
        let handle = unsafe { &mut *vm };
        match handle.0.call(&nm, &[]) {
            Ok(Value::Int(i)) => { unsafe { *out = i; }; 0 }
            _ => -1,
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn vm_eval_call() {
        let mut vm = Vm::new();
        assert!(matches!(vm.eval("fn main() -> Int { 123 }"), Err(Error::Unsupported("eval"))));
        assert!(matches!(vm.call("main", &[]), Err(Error::Call(_))));
    }
}
