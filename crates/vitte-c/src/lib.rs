//! vitte-c — C FFI stable pour Vitte (compilateur + VM)
//!
//! API C générable via `cbindgen`.
//! - Handles opaques (compiler, vm)
//! - Buffers transférables (data+len)
//! - Codes d’erreur stables
//! - Routines utilitaires (version, free, last_error)
//!
//! Convention:
//! - Toutes les fonctions retournent `int` (0 = OK, <0 = erreur).
//! - Les pointeurs de sortie sont `out**` et doivent être non-nuls.
//! - Les buffers renvoyés doivent être libérés avec `vitte_buffer_free`.
//!
//! Sécurité: FFI requiert `unsafe` aux frontières uniquement.

#![deny(unsafe_op_in_unsafe_fn)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::missing_errors_doc,
    clippy::doc_markdown
)]

use libc::{c_char, c_int, c_uchar, size_t};
use std::cell::RefCell;
use std::ffi::{CStr, CString};
use std::ptr;

//
// --- Types C stables -----------------------------------------------------
//

/// Codes d’erreur stables (négatifs).
#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum VitteErrorCode {
    Ok = 0,
    NullPtr = -1,
    InvalidUtf8 = -2,
    CompileFailed = -3,
    VmFailed = -4,
    Internal = -128,
}

/// Buffer C générique (propriété transférée à l’appelant).
#[repr(C)]
pub struct VitteBuffer {
    pub data: *mut c_uchar,
    pub len: size_t,
}

/// Handles opaques
#[repr(C)]
pub struct VitteCompiler {
    _priv: [u8; 0],
}

#[repr(C)]
pub struct VitteVm {
    _priv: [u8; 0],
}

//
// --- Stockage thread-local du dernier message d’erreur -------------------
//

thread_local! {
    static LAST_ERR: RefCell<Option<CString>> = RefCell::new(None);
}

fn set_last_error(msg: impl AsRef<str>) {
    let s = CString::new(msg.as_ref()).unwrap_or_else(|_| CString::new("error").unwrap());
    LAST_ERR.with(|slot| *slot.borrow_mut() = Some(s));
}

fn clear_last_error() {
    LAST_ERR.with(|slot| *slot.borrow_mut() = None);
}

fn get_last_error_cstr() -> *const c_char {
    LAST_ERR.with(|slot| {
        if let Some(s) = &*slot.borrow() {
            s.as_ptr()
        } else {
            static NO_ERR: &str = "ok";
            // SAFETY: static string with terminal NUL via concat!
            concat!("ok", "\0").as_ptr() as *const c_char
        }
    })
}

//
// --- Helpers conversion ---------------------------------------------------
//

fn cstr_to_str<'a>(p: *const c_char) -> Result<&'a str, VitteErrorCode> {
    if p.is_null() {
        return Err(VitteErrorCode::NullPtr);
    }
    // SAFETY: p non nul. CStr::from_ptr requiert un NUL final valide.
    let s = unsafe { CStr::from_ptr(p) };
    s.to_str().map_err(|_| VitteErrorCode::InvalidUtf8)
}

fn make_buffer(bytes: Vec<u8>) -> VitteBuffer {
    let len = bytes.len();
    let mut b = bytes.into_boxed_slice();
    let data = b.as_mut_ptr();
    std::mem::forget(b);
    VitteBuffer {
        data,
        len: len as size_t,
    }
}

//
// --- Liaison avec les crates internes (façade minimale) ------------------
//

mod core_api {
    // Ces imports supposent les APIs internes usuelles. Adaptez si nécessaire.
    pub use vitte_compiler::compile_to_bytecode;
    pub use vitte_parser::parse_source;
    pub use vitte_vm::{Bytecode, Vm};
}

// Opaque réels détenus côté Rust.
struct CompilerReal;
struct VmReal {
    vm: core_api::Vm,
}

//
// --- API C ---------------------------------------------------------------
//

/// Version ABI de l’API C.
#[no_mangle]
pub extern "C" fn vitte_c_abi_version() -> c_int {
    1
}

/// Version texte du projet (via env! si disponible).
#[no_mangle]
pub extern "C" fn vitte_c_version_string() -> *const c_char {
    // Construit une CString statique à l’init.
    static mut PTR: *const c_char = ptr::null();
    // SAFETY: idempotent, race bénigne si appelé très tôt. Acceptable pour FFI simple.
    unsafe {
        if PTR.is_null() {
            let v = option_env!("CARGO_PKG_VERSION").unwrap_or("0.0.0");
            let s = CString::new(v).unwrap();
            PTR = Box::leak(Box::new(s)).as_ptr();
        }
        PTR
    }
}

/// Dernier message d’erreur (thread-local). Ne pas free.
#[no_mangle]
pub extern "C" fn vitte_c_last_error_message() -> *const c_char {
    get_last_error_cstr()
}

/// Libère un buffer renvoyé par l’API.
#[no_mangle]
pub extern "C" fn vitte_buffer_free(buf: *mut VitteBuffer) {
    if buf.is_null() {
        return;
    }
    // SAFETY: pointeur fourni par l’API.
    let b = unsafe { &mut *buf };
    if !b.data.is_null() && b.len > 0 {
        // Recompose le Box<[u8]> pour libération.
        // SAFETY: data provient d’un Box<[u8]> oublié.
        unsafe {
            let slice = std::slice::from_raw_parts_mut(b.data, b.len as usize);
            drop(Box::from_raw(slice as *mut [u8]));
        }
    }
    // Optionnel: nullifier
    b.data = ptr::null_mut();
    b.len = 0;
}

/// Crée un compilateur.
#[no_mangle]
pub extern "C" fn vitte_compiler_create(out: *mut *mut VitteCompiler) -> c_int {
    clear_last_error();
    if out.is_null() {
        return VitteErrorCode::NullPtr as c_int;
    }
    // SAFETY: out valide
    unsafe {
        let real = Box::new(CompilerReal);
        let raw = Box::into_raw(real) as *mut VitteCompiler;
        *out = raw;
    }
    VitteErrorCode::Ok as c_int
}

/// Détruit un compilateur.
#[no_mangle]
pub extern "C" fn vitte_compiler_destroy(comp: *mut VitteCompiler) {
    if comp.is_null() {
        return;
    }
    // SAFETY: pointeur alloué par create.
    unsafe { drop(Box::from_raw(comp as *mut CompilerReal)) };
}

/// Compile une source Vitte en bytecode.
///
/// out_bc reçoit un buffer possédé par l’appelant (via vitte_buffer_free).
#[no_mangle]
pub extern "C" fn vitte_compile_source_to_bytecode(
    _comp: *mut VitteCompiler,
    source_utf8: *const c_char,
    out_bc: *mut VitteBuffer,
) -> c_int {
    clear_last_error();

    if _comp.is_null() || source_utf8.is_null() || out_bc.is_null() {
        return VitteErrorCode::NullPtr as c_int;
    }

    let src = match cstr_to_str(source_utf8) {
        Ok(s) => s,
        Err(code) => return code as c_int,
    };

    // Parse + compile
    let bytecode: Vec<u8> = match (|| {
        let ast = core_api::parse_source(src).map_err(|e| format!("parse: {e}"))?;
        let bc: core_api::Bytecode =
            core_api::compile_to_bytecode(&ast).map_err(|e| format!("compile: {e}"))?;
        Ok::<Vec<u8>, String>(bc.into_bytes())
    })() {
        Ok(b) => b,
        Err(msg) => {
            set_last_error(msg);
            return VitteErrorCode::CompileFailed as c_int;
        }
    };

    // SAFETY: out non nul.
    unsafe { *out_bc = make_buffer(bytecode) };
    VitteErrorCode::Ok as c_int
}

/// Crée une VM.
#[no_mangle]
pub extern "C" fn vitte_vm_create(out: *mut *mut VitteVm) -> c_int {
    clear_last_error();
    if out.is_null() {
        return VitteErrorCode::NullPtr as c_int;
    }
    // Init VM réelle
    let vm = match core_api::Vm::new() {
        Ok(v) => v,
        Err(e) => {
            set_last_error(format!("vm new: {e}"));
            return VitteErrorCode::Internal as c_int;
        }
    };
    let real = VmReal { vm };
    // SAFETY: out valide
    unsafe {
        *out = Box::into_raw(Box::new(real)) as *mut VitteVm;
    }
    VitteErrorCode::Ok as c_int
}

/// Détruit une VM.
#[no_mangle]
pub extern "C" fn vitte_vm_destroy(vm: *mut VitteVm) {
    if vm.is_null() {
        return;
    }
    // SAFETY: pointeur alloué par create.
    unsafe { drop(Box::from_raw(vm as *mut VmReal)) };
}

/// Exécute du bytecode en mémoire.
///
/// `bytecode` peut être NULL si `len`=0. Retourne 0 si OK.
#[no_mangle]
pub extern "C" fn vitte_vm_execute_bytecode(
    vm: *mut VitteVm,
    bytecode: *const c_uchar,
    len: size_t,
) -> c_int {
    clear_last_error();
    if vm.is_null() {
        return VitteErrorCode::NullPtr as c_int;
    }
    let real = // SAFETY: vm valide
        unsafe { &mut *(vm as *mut VmReal) };

    let slice = if len == 0 {
        &[][..]
    } else if bytecode.is_null() {
        return VitteErrorCode::NullPtr as c_int;
    } else {
        // SAFETY: buffer C supposé valide pour lecture.
        unsafe { std::slice::from_raw_parts(bytecode, len as usize) }
    };

    if let Err(e) = real.vm.execute_bytes(slice) {
        set_last_error(format!("vm exec: {e}"));
        return VitteErrorCode::VmFailed as c_int;
    }
    VitteErrorCode::Ok as c_int
}

/// Utilitaire: compile + exécute en une étape.
#[no_mangle]
pub extern "C" fn vitte_run_source(
    vm: *mut VitteVm,
    source_utf8: *const c_char,
) -> c_int {
    clear_last_error();
    if vm.is_null() {
        return VitteErrorCode::NullPtr as c_int;
    }
    let src = match cstr_to_str(source_utf8) {
        Ok(s) => s,
        Err(code) => return code as c_int,
    };

    // Compile
    let bc = match (|| {
        let ast = core_api::parse_source(src).map_err(|e| format!("parse: {e}"))?;
        let bc: core_api::Bytecode =
            core_api::compile_to_bytecode(&ast).map_err(|e| format!("compile: {e}"))?;
        Ok::<Vec<u8>, String>(bc.into_bytes())
    })() {
        Ok(b) => b,
        Err(msg) => {
            set_last_error(msg);
            return VitteErrorCode::CompileFailed as c_int;
        }
    };

    // Exec
    let code = vitte_vm_execute_bytecode(vm, bc.as_ptr(), bc.len() as size_t);
    if code != VitteErrorCode::Ok as c_int {
        return code;
    }
    VitteErrorCode::Ok as c_int
}

//
// --- Tests de fumée ------------------------------------------------------
//

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn smoke_create_and_free() {
        let mut comp = ptr::null_mut();
        assert_eq!(vitte_compiler_create(&mut comp), VitteErrorCode::Ok as c_int);
        assert!(!comp.is_null());
        vitte_compiler_destroy(comp);

        let mut vm = ptr::null_mut();
        assert_eq!(vitte_vm_create(&mut vm), VitteErrorCode::Ok as c_int);
        assert!(!vm.is_null());
        vitte_vm_destroy(vm);
    }

    #[test]
    fn smoke_compile_then_run() {
        let mut comp = ptr::null_mut();
        let mut vm = ptr::null_mut();
        assert_eq!(vitte_compiler_create(&mut comp), 0);
        assert_eq!(vitte_vm_create(&mut vm), 0);

        let src = CString::new("fn main() { /* noop */ }").unwrap();
        let mut bc = VitteBuffer { data: ptr::null_mut(), len: 0 };

        assert_eq!(
            vitte_compile_source_to_bytecode(comp, src.as_ptr(), &mut bc),
            VitteErrorCode::Ok as c_int
        );
        assert!(bc.len >= 0);

        assert_eq!(
            vitte_vm_execute_bytecode(vm, bc.data, bc.len),
            VitteErrorCode::Ok as c_int
        );

        vitte_buffer_free(&mut bc);
        vitte_vm_destroy(vm);
        vitte_compiler_destroy(comp);
    }
}