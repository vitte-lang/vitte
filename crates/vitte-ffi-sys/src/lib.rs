//! vitte-ffi-sys — bindings FFI bas-niveau pour Vitte
//!
//! Ce crate expose les symboles C de `vitte-c` en Rust,
//! sans aucune garantie de sûreté.  
//! L’objectif est de fournir une interface brute pour
//! générer du code Rust <-> C ou alimenter d’autres bindings.
//!
//! Typiquement, on importe ici les `extern "C"`
//! et on re-déclare les structs C opaques.
//!
//! Exemple d’usage :
//! ```ignore
//! use vitte_ffi_sys::*;
//! unsafe {
//!     let mut comp: *mut VitteCompiler = std::ptr::null_mut();
//!     let code = vitte_compiler_create(&mut comp);
//!     assert_eq!(code, VitteErrorCode::Ok as i32);
//!     vitte_compiler_destroy(comp);
//! }
//! ```

#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(clippy::module_name_repetitions)]

use libc::{c_char, c_int, c_uchar, size_t};

/// Codes d’erreur exportés par vitte-c.
#[repr(i32)]
#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub enum VitteErrorCode {
    Ok = 0,
    NullPtr = -1,
    InvalidUtf8 = -2,
    CompileFailed = -3,
    VmFailed = -4,
    Internal = -128,
}

/// Buffer générique (ownership à libérer via vitte_buffer_free).
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct VitteBuffer {
    pub data: *mut c_uchar,
    pub len: size_t,
}

/// Handles opaques.
#[repr(C)]
pub struct VitteCompiler {
    _priv: [u8; 0],
}

#[repr(C)]
pub struct VitteVm {
    _priv: [u8; 0],
}

extern "C" {
    // --- Erreurs / version ---
    pub fn vitte_c_abi_version() -> c_int;
    pub fn vitte_c_version_string() -> *const c_char;
    pub fn vitte_c_last_error_message() -> *const c_char;

    // --- Buffers ---
    pub fn vitte_buffer_free(buf: *mut VitteBuffer);

    // --- Compiler ---
    pub fn vitte_compiler_create(out: *mut *mut VitteCompiler) -> c_int;
    pub fn vitte_compiler_destroy(comp: *mut VitteCompiler);
    pub fn vitte_compile_source_to_bytecode(
        comp: *mut VitteCompiler,
        source_utf8: *const c_char,
        out_bc: *mut VitteBuffer,
    ) -> c_int;

    // --- VM ---
    pub fn vitte_vm_create(out: *mut *mut VitteVm) -> c_int;
    pub fn vitte_vm_destroy(vm: *mut VitteVm);
    pub fn vitte_vm_execute_bytecode(
        vm: *mut VitteVm,
        bytecode: *const c_uchar,
        len: size_t,
    ) -> c_int;

    pub fn vitte_run_source(vm: *mut VitteVm, source_utf8: *const c_char) -> c_int;
}

// --- Tests ----------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    #[test]
    fn abi_version_nonzero() {
        let v = unsafe { vitte_c_abi_version() };
        assert!(v >= 1);
    }

    #[test]
    fn create_and_destroy_compiler() {
        let mut comp: *mut VitteCompiler = std::ptr::null_mut();
        let code = unsafe { vitte_compiler_create(&mut comp) };
        assert_eq!(code, VitteErrorCode::Ok as c_int);
        assert!(!comp.is_null());
        unsafe { vitte_compiler_destroy(comp) };
    }

    #[test]
    fn create_and_destroy_vm() {
        let mut vm: *mut VitteVm = std::ptr::null_mut();
        let code = unsafe { vitte_vm_create(&mut vm) };
        assert_eq!(code, VitteErrorCode::Ok as c_int);
        assert!(!vm.is_null());
        unsafe { vitte_vm_destroy(vm) };
    }

    #[test]
    fn compile_smoke() {
        let mut comp: *mut VitteCompiler = std::ptr::null_mut();
        unsafe { vitte_compiler_create(&mut comp) };
        let src = CString::new("fn main() {}").unwrap();
        let mut buf = VitteBuffer { data: std::ptr::null_mut(), len: 0 };
        let code = unsafe { vitte_compile_source_to_bytecode(comp, src.as_ptr(), &mut buf) };
        assert_eq!(code, VitteErrorCode::Ok as c_int);
        unsafe { vitte_buffer_free(&mut buf) };
        unsafe { vitte_compiler_destroy(comp) };
    }
}