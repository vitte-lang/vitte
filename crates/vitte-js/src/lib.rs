//! vitte-js — intégration Vitte <-> JavaScript/TypeScript
//!
//! Modes disponibles :
//! - WASM (via `wasm-bindgen`) pour navigateur/bundlers
//! - N-API (via `napi-rs`) pour Node.js
//!
//! Fonctions exposées :
//! - `compile_source` : compile du code source Vitte en bytecode
//! - `run_source` : exécute directement du code source
//!
//! Selon la feature, l’export se fait en `#[wasm_bindgen]` ou `#[napi]`.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use anyhow::Result;
use vitte_errors::Error;
use vitte_ffi_sys as ffi;

#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};

#[cfg(feature = "wasm")]
use wasm_bindgen::prelude::*;

#[cfg(feature = "napi")]
use napi::bindgen_prelude::*;

/// Compile une chaîne source en bytecode.
fn compile_internal(src: &str) -> Result<Vec<u8>> {
    use std::ffi::CString;
    use std::ptr;

    let csrc = CString::new(src)?;
    let mut buf = ffi::VitteBuffer { data: ptr::null_mut(), len: 0 };
    let mut comp: *mut ffi::VitteCompiler = std::ptr::null_mut();
    let code = unsafe { ffi::vitte_compiler_create(&mut comp) };
    if code != ffi::VitteErrorCode::Ok as i32 {
        return Err(Error::msg("compiler create failed").into());
    }
    let code = unsafe { ffi::vitte_compile_source_to_bytecode(comp, csrc.as_ptr(), &mut buf) };
    if code != ffi::VitteErrorCode::Ok as i32 {
        unsafe { ffi::vitte_compiler_destroy(comp) };
        return Err(Error::msg("compile failed").into());
    }
    let out = unsafe { std::slice::from_raw_parts(buf.data, buf.len as usize).to_vec() };
    unsafe { ffi::vitte_buffer_free(&mut buf) };
    unsafe { ffi::vitte_compiler_destroy(comp) };
    Ok(out)
}

/// Exécute du code source directement.
fn run_internal(src: &str) -> Result<()> {
    use std::ffi::CString;
    let csrc = CString::new(src)?;
    let mut vm: *mut ffi::VitteVm = std::ptr::null_mut();
    let code = unsafe { ffi::vitte_vm_create(&mut vm) };
    if code != ffi::VitteErrorCode::Ok as i32 {
        return Err(Error::msg("vm create failed").into());
    }
    let code = unsafe { ffi::vitte_run_source(vm, csrc.as_ptr()) };
    unsafe { ffi::vitte_vm_destroy(vm) };
    if code != ffi::VitteErrorCode::Ok as i32 {
        return Err(Error::msg("run failed").into());
    }
    Ok(())
}

// ----------------- Exports WASM -----------------

#[cfg(feature = "wasm")]
#[wasm_bindgen]
pub fn compile_source(src: &str) -> Result<Vec<u8>, JsValue> {
    compile_internal(src).map_err(|e| JsValue::from_str(&e.to_string()))
}

#[cfg(feature = "wasm")]
#[wasm_bindgen]
pub fn run_source(src: &str) -> Result<(), JsValue> {
    run_internal(src).map_err(|e| JsValue::from_str(&e.to_string()))
}

// ----------------- Exports N-API -----------------

#[cfg(feature = "napi")]
#[napi]
pub fn compile_source(src: String) -> napi::Result<Vec<u8>> {
    compile_internal(&src).map_err(|e| napi::Error::from_reason(e.to_string()))
}

#[cfg(feature = "napi")]
#[napi]
pub fn run_source(src: String) -> napi::Result<()> {
    run_internal(&src).map_err(|e| napi::Error::from_reason(e.to_string()))
}

// ----------------- Tests -----------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn smoke_compile() {
        let bc = compile_internal("fn main() {}");
        assert!(bc.is_ok());
        assert!(!bc.unwrap().is_empty());
    }

    #[test]
    fn smoke_run() {
        let r = run_internal("fn main() { }");
        assert!(r.is_ok());
    }
}