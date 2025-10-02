//! vitte-ffi — C ABI étendue pour piloter le runtime Vitte depuis d’autres langages
//!
//! Surface C stable couvrant : création, configuration (timeout), chargement de
//! code (source + fichier), appel de fonctions avec valeurs riches, variables
//! globales simples, et gestion mémoire (strings, octets, tableaux).
//!
//! ABI (résumé) — tous les symboles sont `extern "C"` :
//! - Runtime      : `vitte_ffi_new`, `vitte_ffi_free`, `vitte_ffi_version`
//! - Erreurs      : `vitte_ffi_last_error` (C string possédée par le handle)
//! - Config       : `vitte_ffi_set_timeout_ms`
//! - Code         : `vitte_ffi_eval`, `vitte_ffi_load_file`
//! - Appels       : `vitte_ffi_call` (args/ret via `VitteValue`)
//! - Variables    : `vitte_ffi_set_var`, `vitte_ffi_get_var`
//! - Mémoire      : `vitte_ffi_string_free`, `vitte_ffi_bytes_free`, `vitte_ffi_value_free`
//!
//! Sécurité FFI : aucune panique ne traverse l’ABI (tout est encapsulé via
//! `catch_unwind`). Toutes les sorties allouées par Rust doivent être libérées
//! par les fonctions *free* correspondantes.

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

extern crate alloc;

use alloc::{boxed::Box, format, string::String, vec::Vec};

use core::ffi::c_char;
use core::panic::{catch_unwind, AssertUnwindSafe};
use core::ptr;

#[cfg(feature = "std")]
use std::ffi::{CStr, CString};

use vitte_runtime::Runtime;
use vitte_runtime::Value as RtValue;

/* ------------------------------ Codes & tags ----------------------------- */

/// Codes de retour (0 = Ok).
#[repr(i32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum VitteCode {
    /// Succès.
    Ok = 0,
    /// Entrée invalide.
    Invalid = -1,
    /// Erreur d'exécution/runtime.
    Runtime = -2,
    /// Erreur d'appel ou symbole absent.
    Call = -3,
    /// I/O (ex: lecture fichier).
    Io = -4,
    /// Panique interne interceptée.
    Panic = -101,
}

/// Discriminant d’une valeur C.
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum VitteTag {
    /// Unit.
    Unit = 0,
    /// Booléen.
    Bool = 1,
    /// Entier 64 bits.
    I64  = 2,
    /// Float 64 bits.
    F64  = 3,
    /// Chaîne UTF-8.
    Str  = 4,
    /// Octets arbitraires.
    Bytes = 5,
    /// Tableau de valeurs.
    Array = 6,
}

/* ------------------------------ Blocs C types ---------------------------- */

/// Chaîne UTF-8 (ptr + len). Si `owned != 0`, libérer via `vitte_ffi_string_free`.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct VitteString {
    /// Pointeur vers octets UTF-8 (peut être NULL si len=0).
    pub ptr: *mut c_char,
    /// Longueur en octets.
    pub len: usize,
    /// 0=empruntée, 1=possédée (alloc Rust).
    pub owned: i32,
}
impl VitteString {
    fn empty() -> Self { Self { ptr: ptr::null_mut(), len: 0, owned: 0 } }
}

/// Tampon d’octets. Si `owned != 0`, libérer via `vitte_ffi_bytes_free`.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct VitteBytes {
    /// Pointeur vers les octets (peut être NULL si len=0).
    pub ptr: *mut u8,
    /// Longueur.
    pub len: usize,
    /// 0=emprunté, 1=possédé.
    pub owned: i32,
}
impl VitteBytes {
    fn empty() -> Self { Self { ptr: ptr::null_mut(), len: 0, owned: 0 } }
}

/// Tableau contigu de `VitteValue`. Si `owned != 0`, libérer via `vitte_ffi_value_free`
/// sur chaque élément puis `libc::free` style n’est **pas** requis : utilisez `vitte_ffi_value_free`
/// sur la valeur *Array* parent pour tout libérer correctement.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct VitteArray {
    /// Pointeur vers le premier élément.
    pub ptr: *mut VitteValue,
    /// Nombre d’éléments.
    pub len: usize,
    /// 0=emprunté, 1=possédé.
    pub owned: i32,
}
impl VitteArray {
    fn empty() -> Self { Self { ptr: ptr::null_mut(), len: 0, owned: 0 } }
}

/// Valeur générique transmise via C.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct VitteValue {
    /// Discriminant.
    pub tag: VitteTag,
    /// Bool (0/1).
    pub as_bool: i32,
    /// I64.
    pub as_i64: i64,
    /// F64 (bits exacts).
    pub as_f64: f64,
    /// String.
    pub s: VitteString,
    /// Bytes.
    pub b: VitteBytes,
    /// Array.
    pub a: VitteArray,
}

impl VitteValue {
    fn unit() -> Self {
        Self { tag: VitteTag::Unit, as_bool: 0, as_i64: 0, as_f64: 0.0, s: VitteString::empty(), b: VitteBytes::empty(), a: VitteArray::empty() }
    }
    fn from_bool(x: bool) -> Self { Self { tag: VitteTag::Bool, as_bool: x as i32, ..Self::unit() } }
    fn from_i64(x: i64) -> Self { Self { tag: VitteTag::I64, as_i64: x, ..Self::unit() } }
    fn from_f64(x: f64) -> Self { Self { tag: VitteTag::F64, as_f64: x, ..Self::unit() } }

    #[cfg(feature = "std")]
    fn from_owned_string(s: String) -> Self {
        let c = CString::new(s).unwrap_or_else(|_| CString::new(Vec::<u8>::new()).unwrap());
        let len = c.as_bytes().len();
        let ptr = c.into_raw();
        let mut v = Self::unit();
        v.tag = VitteTag::Str;
        v.s = VitteString { ptr, len, owned: 1 };
        v
    }

    #[cfg(feature = "std")]
    fn from_owned_bytes(mut v: Vec<u8>) -> Self {
        // Transférer l'allocation à C : Box<[u8]> -> *mut u8
        let len = v.len();
        let ptr = v.as_mut_ptr();
        core::mem::forget(v);
        let mut out = Self::unit();
        out.tag = VitteTag::Bytes;
        out.b = VitteBytes { ptr, len, owned: 1 };
        out
    }

    #[cfg(feature = "std")]
    fn from_owned_array(mut items: Vec<VitteValue>) -> Self {
        let len = items.len();
        let ptr = items.as_mut_ptr();
        core::mem::forget(items);
        let mut out = Self::unit();
        out.tag = VitteTag::Array;
        out.a = VitteArray { ptr, len, owned: 1 };
        out
    }
}

/* --------------------------- Conversions runtime ------------------------- */

fn rt_from_c(v: &VitteValue) -> core::result::Result<RtValue, String> {
    match v.tag {
        VitteTag::Unit => Ok(RtValue::Unit),
        VitteTag::Bool => Ok(RtValue::Bool(v.as_bool != 0)),
        VitteTag::I64  => Ok(RtValue::Int(v.as_i64)),
        VitteTag::F64  => Ok(RtValue::F64(v.as_f64)),
        VitteTag::Str  => {
            #[cfg(feature = "std")]
            unsafe {
                let (ptr, len) = (v.s.ptr, v.s.len);
                if ptr.is_null() && len == 0 { return Ok(RtValue::Str(String::new())); }
                let slice = core::slice::from_raw_parts(ptr as *const u8, len);
                let s = core::str::from_utf8(slice).map_err(|_| "invalid utf-8".to_string())?;
                Ok(RtValue::Str(s.to_owned()))
            }
            #[cfg(not(feature = "std"))]
            { Err("strings require std".into()) }
        }
        VitteTag::Bytes => {
            #[cfg(feature = "std")]
            unsafe {
                let (ptr, len) = (v.b.ptr, v.b.len);
                if ptr.is_null() { return Ok(RtValue::Bytes(Vec::new())); }
                let slice = core::slice::from_raw_parts(ptr, len);
                Ok(RtValue::Bytes(slice.to_vec()))
            }
            #[cfg(not(feature = "std"))]
            { Err("bytes require std".into()) }
        }
        VitteTag::Array => {
            #[cfg(feature = "std")]
            unsafe {
                let (ptr, len) = (v.a.ptr, v.a.len);
                if ptr.is_null() && len == 0 { return Ok(RtValue::Array(Vec::new())); }
                let items = core::slice::from_raw_parts(ptr, len);
                let mut out = Vec::with_capacity(len);
                for it in items { out.push(rt_from_c(it)?); }
                Ok(RtValue::Array(out))
            }
            #[cfg(not(feature = "std"))]
            { Err("array require std".into()) }
        }
    }
}

fn rt_to_c(v: RtValue) -> VitteValue {
    match v {
        RtValue::Unit        => VitteValue::unit(),
        RtValue::Bool(b)     => VitteValue::from_bool(b),
        RtValue::Int(i)      => VitteValue::from_i64(i),
        RtValue::F64(x)      => VitteValue::from_f64(x),
        RtValue::Str(s)      => { #[cfg(feature="std")] { return VitteValue::from_owned_string(s) } VitteValue::unit() }
        RtValue::Bytes(b)    => { #[cfg(feature="std")] { return VitteValue::from_owned_bytes(b) } VitteValue::unit() }
        RtValue::Array(xs)   => {
            #[cfg(feature="std")] {
                let items: Vec<VitteValue> = xs.into_iter().map(rt_to_c).collect();
                return VitteValue::from_owned_array(items);
            }
            #[allow(unreachable_code)]
            VitteValue::unit()
        }
        // Ajoutez d'autres variantes si votre runtime en expose.
    }
}

/* ------------------------------- Handle ---------------------------------- */

/// Contexte opaque de runtime.
pub struct VitteHandle {
    rt: Runtime,
    #[cfg(feature = "std")]
    last_err: Option<CString>,
    /// Timeout en millisecondes (best-effort, géré par le runtime si dispo).
    timeout_ms: u64,
}

impl VitteHandle {
    fn new() -> Self {
        Self {
            rt: Runtime::new(),
            #[cfg(feature = "std")]
            last_err: None,
            timeout_ms: 0,
        }
    }
    #[cfg(feature = "std")]
    fn set_err(&mut self, msg: String) {
        self.last_err = Some(CString::new(msg).unwrap_or_else(|_| CString::new("error").unwrap()));
    }
}

/* ------------------------------ Utilitaires ------------------------------ */

#[inline] fn ok() -> VitteCode { VitteCode::Ok }
#[inline] fn invalid() -> VitteCode { VitteCode::Invalid }
#[inline] fn rt_err() -> VitteCode { VitteCode::Runtime }
#[inline] fn call_err() -> VitteCode { VitteCode::Call }
#[inline] fn io_err() -> VitteCode { VitteCode::Io }
#[inline] fn panic_err() -> VitteCode { VitteCode::Panic }

#[cfg(feature = "std")]
unsafe fn cstr<'a>(p: *const c_char) -> Result<&'a str, ()> {
    if p.is_null() { return Err(()); }
    CStr::from_ptr(p).to_str().map_err(|_| ())
}

/* --------------------------------- API C --------------------------------- */

/// Version sous forme de C string statique "name version".
#[no_mangle]
pub extern "C" fn vitte_ffi_version() -> *const c_char {
    #[cfg(feature = "std")]
    {
        static mut VER: *const c_char = ptr::null();
        unsafe {
            if VER.is_null() {
                let s = CString::new(concat!(env!("CARGO_PKG_NAME"), " ", env!("CARGO_PKG_VERSION"))).unwrap();
                VER = Box::leak(Box::new(s)).as_ptr();
            }
            VER
        }
    }
    #[cfg(not(feature = "std"))]
    { ptr::null() }
}

/// Crée un runtime Vitte et retourne un handle opaque.
#[no_mangle]
pub extern "C" fn vitte_ffi_new() -> *mut VitteHandle {
    Box::into_raw(Box::new(VitteHandle::new()))
}

/// Détruit le handle.
#[no_mangle]
pub extern "C" fn vitte_ffi_free(h: *mut VitteHandle) {
    if !h.is_null() { unsafe { drop(Box::from_raw(h)); } }
}

/// Dernière erreur sous forme de C string. Propriété : handle.
/// Pointeur valide jusqu’au prochain changement d’erreur ou `free`.
#[no_mangle]
pub extern "C" fn vitte_ffi_last_error(h: *mut VitteHandle) -> *const c_char {
    #[cfg(feature = "std")]
    unsafe {
        if h.is_null() { return ptr::null(); }
        match (&*h).last_err.as_ref() {
            Some(c) => c.as_ptr(),
            None => ptr::null(),
        }
    }
    #[cfg(not(feature = "std"))]
    { ptr::null() }
}

/// Configure un timeout global en millisecondes (best-effort).
#[no_mangle]
pub extern "C" fn vitte_ffi_set_timeout_ms(h: *mut VitteHandle, ms: u64) -> VitteCode {
    if h.is_null() { return invalid(); }
    let res = catch_unwind(AssertUnwindSafe(|| {
        unsafe { (*h).timeout_ms = ms; }
        // Si votre runtime supporte un paramètre de deadline, appliquez-le ici.
        Ok::<(), ()>(())
    }));
    match res { Ok(_) => ok(), Err(_) => panic_err() }
}

/// Évalue du code source UTF-8.
#[no_mangle]
pub extern "C" fn vitte_ffi_eval(h: *mut VitteHandle, src: *const c_char) -> VitteCode {
    if h.is_null() || src.is_null() { return invalid(); }
    let res = catch_unwind(AssertUnwindSafe(|| {
        #[cfg(feature = "std")]
        unsafe {
            let s = cstr(src).map_err(|_| "invalid utf-8".to_string())?;
            let hh = &mut *h;
            hh.rt.eval(s).map_err(|e| format!("{e:?}"))?;
            hh.last_err = None;
            Ok::<(), String>(())
        }
        #[cfg(not(feature="std"))]
        { Err::<(), String>("std required".into()) }
    }));
    match res {
        Ok(Ok(())) => ok(),
        Ok(Err(msg)) => { #[cfg(feature="std")] unsafe { (&mut *h).set_err(msg); } rt_err() }
        Err(_) => { #[cfg(feature="std")] unsafe { (&mut *h).set_err("panic in eval".into()); } panic_err() }
    }
}

/// Charge et évalue un fichier source (chemin UTF-8).
#[no_mangle]
pub extern "C" fn vitte_ffi_load_file(h: *mut VitteHandle, path: *const c_char) -> VitteCode {
    if h.is_null() || path.is_null() { return invalid(); }
    #[cfg(not(feature="std"))]
    { return invalid(); }
    #[cfg(feature="std")]
    let res = catch_unwind(AssertUnwindSafe(|| unsafe {
        let p = cstr(path).map_err(|_| "invalid utf-8 path".to_string())?;
        let src = std::fs::read_to_string(p).map_err(|e| e.to_string())?;
        let hh = &mut *h;
        hh.rt.eval(&src).map_err(|e| format!("{e:?}"))?;
        hh.last_err = None;
        Ok::<(), String>(())
    }));
    #[cfg(feature="std")]
    match res {
        Ok(Ok(())) => ok(),
        Ok(Err(msg)) => { unsafe { (&mut *h).set_err(msg); } io_err() },
        Err(_) => { unsafe { (&mut *h).set_err("panic in load_file".into()); } panic_err() }
    }
}

/// Appel générique d’une fonction.
/// - `func` : nom UTF-8
/// - `args_ptr/args_len` : tableau d’arguments (peut être NULL/0)
/// - `out_ret` : rempli en cas de succès (propriété : appelant, libérer via `vitte_ffi_value_free`)
#[no_mangle]
pub extern "C" fn vitte_ffi_call(
    h: *mut VitteHandle,
    func: *const c_char,
    args_ptr: *const VitteValue,
    args_len: usize,
    out_ret: *mut VitteValue,
) -> VitteCode {
    if h.is_null() || func.is_null() || out_ret.is_null() { return invalid(); }
    let res = catch_unwind(AssertUnwindSafe(|| {
        #[cfg(feature="std")]
        unsafe {
            let hh = &mut *h;
            let name = cstr(func).map_err(|_| "invalid utf-8 func".to_string())?;
            let args_slice = if args_len == 0 || args_ptr.is_null() {
                &[][..]
            } else {
                core::slice::from_raw_parts(args_ptr, args_len)
            };
            let mut rt_args = Vec::with_capacity(args_slice.len());
            for a in args_slice { rt_args.push(rt_from_c(a)?); }
            // Optionnel : appliquer le timeout au runtime si disponible.
            let rv = hh.rt.call(name, &rt_args).map_err(|e| format!("{e:?}"))?;
            *out_ret = rt_to_c(rv);
            hh.last_err = None;
            Ok::<(), String>(())
        }
        #[cfg(not(feature="std"))]
        { Err::<(), String>("std required".into()) }
    }));
    match res {
        Ok(Ok(())) => ok(),
        Ok(Err(msg)) => { #[cfg(feature="std")] unsafe { (&mut *h).set_err(msg); } call_err() }
        Err(_) => { #[cfg(feature="std")] unsafe { (&mut *h).set_err("panic in call".into()); } panic_err() }
    }
}

/// Définit une variable globale simple (si supporté par le runtime).
#[no_mangle]
pub extern "C" fn vitte_ffi_set_var(
    h: *mut VitteHandle,
    name: *const c_char,
    val: *const VitteValue,
) -> VitteCode {
    if h.is_null() || name.is_null() || val.is_null() { return invalid(); }
    let res = catch_unwind(AssertUnwindSafe(|| {
        #[cfg(feature="std")]
        unsafe {
            let hh = &mut *h;
            let nm = cstr(name).map_err(|_| "invalid utf-8 name".to_string())?;
            let v = rt_from_c(&*val)?;
            hh.rt.set_var(nm, v).map_err(|e| format!("{e:?}"))?;
            hh.last_err = None;
            Ok::<(), String>(())
        }
        #[cfg(not(feature="std"))]
        { Err::<(), String>("std required".into()) }
    }));
    match res {
        Ok(Ok(())) => ok(),
        Ok(Err(msg)) => { #[cfg(feature="std")] unsafe { (&mut *h).set_err(msg); } rt_err() }
        Err(_) => { #[cfg(feature="std")] unsafe { (&mut *h).set_err("panic in set_var".into()); } panic_err() }
    }
}

/// Récupère une variable globale simple (si supporté par le runtime).
#[no_mangle]
pub extern "C" fn vitte_ffi_get_var(
    h: *mut VitteHandle,
    name: *const c_char,
    out_val: *mut VitteValue,
) -> VitteCode {
    if h.is_null() || name.is_null() || out_val.is_null() { return invalid(); }
    let res = catch_unwind(AssertUnwindSafe(|| {
        #[cfg(feature="std")]
        unsafe {
            let hh = &mut *h;
            let nm = cstr(name).map_err(|_| "invalid utf-8 name".to_string())?;
            let v = hh.rt.get_var(nm).map_err(|e| format!("{e:?}"))?;
            *out_val = rt_to_c(v);
            hh.last_err = None;
            Ok::<(), String>(())
        }
        #[cfg(not(feature="std"))]
        { Err::<(), String>("std required".into()) }
    }));
    match res {
        Ok(Ok(())) => ok(),
        Ok(Err(msg)) => { #[cfg(feature="std")] unsafe { (&mut *h).set_err(msg); } rt_err() }
        Err(_) => { #[cfg(feature="std")] unsafe { (&mut *h).set_err("panic in get_var".into()); } panic_err() }
    }
}

/* ------------------------------- Free helpers ---------------------------- */

/// Libère une `VitteString` possédée.
#[no_mangle]
pub extern "C" fn vitte_ffi_string_free(s: VitteString) {
    #[cfg(feature="std")]
    unsafe {
        if s.owned != 0 && !s.ptr.is_null() {
            let _ = CString::from_raw(s.ptr); // drop
        }
    }
}

/// Libère un tampon d’octets possédé.
#[no_mangle]
pub extern "C" fn vitte_ffi_bytes_free(b: VitteBytes) {
    #[cfg(feature="std")]
    unsafe {
        if b.owned != 0 && !b.ptr.is_null() {
            drop(Vec::from_raw_parts(b.ptr, b.len, b.len));
        }
    }
}

/// Libère récursivement une `VitteValue` possédée (strings/bytes/array).
#[no_mangle]
pub extern "C" fn vitte_ffi_value_free(v: VitteValue) {
    match v.tag {
        VitteTag::Str   => vitte_ffi_string_free(v.s),
        VitteTag::Bytes => vitte_ffi_bytes_free(v.b),
        VitteTag::Array => {
            #[cfg(feature="std")]
            unsafe {
                if v.a.owned != 0 && !v.a.ptr.is_null() {
                    let items = Vec::from_raw_parts(v.a.ptr, v.a.len, v.a.len);
                    for it in items { vitte_ffi_value_free(it); }
                }
            }
        }
        _ => {}
    }
}

/* ---------------------------------- Tests -------------------------------- */

#[cfg(all(test, feature="std"))]
mod tests {
    use super::*;

    #[test]
    fn call_add_and_free() {
        let h = unsafe { vitte_ffi_new() };
        let src = r#"fn add(a: Int, b: Int) -> Int { a + b }"#;
        assert_eq!(vitte_ffi_eval(h, CString::new(src).unwrap().as_ptr()), VitteCode::Ok);

        let args = [VitteValue::from_i64(40), VitteValue::from_i64(2)];
        let mut out = VitteValue::unit();
        let rc = vitte_ffi_call(h, CString::new("add").unwrap().as_ptr(), args.as_ptr(), args.len(), &mut out);
        assert_eq!(rc, VitteCode::Ok);
        assert_eq!(out.tag, VitteTag::I64);
        assert_eq!(out.as_i64, 42);
        vitte_ffi_value_free(out);
        unsafe { vitte_ffi_free(h) };
    }

    #[test]
    fn strings_bytes_arrays() {
        let h = unsafe { vitte_ffi_new() };
        let src = r#"
            fn greet(n: Str) -> Str { "hi, " + n }
            fn echo_bytes(x: Bytes) -> Bytes { x }
            fn sum(xs: Array[Int]) -> Int { xs.fold(0, |a,b| a+b) }
        "#;
        assert_eq!(vitte_ffi_eval(h, CString::new(src).unwrap().as_ptr()), VitteCode::Ok);

        // Str
        let name = CString::new("vitte").unwrap();
        let arg_str = VitteValue {
            tag: VitteTag::Str, as_bool:0, as_i64:0, as_f64:0.0,
            s: VitteString { ptr: name.as_ptr() as *mut c_char, len: 5, owned: 0 },
            b: VitteBytes::empty(), a: VitteArray::empty()
        };
        let mut out = VitteValue::unit();
        let rc = vitte_ffi_call(h, CString::new("greet").unwrap().as_ptr(), &arg_str, 1, &mut out);
        assert_eq!(rc, VitteCode::Ok);
        assert_eq!(out.tag, VitteTag::Str);
        unsafe {
            let s = core::slice::from_raw_parts(out.s.ptr as *const u8, out.s.len);
            assert_eq!(core::str::from_utf8(s).unwrap(), "hi, vitte");
        }
        vitte_ffi_value_free(out);

        // Bytes roundtrip
        let data = vec![1u8,2,3,4];
        let arg_bytes = VitteValue {
            tag: VitteTag::Bytes, as_bool:0, as_i64:0, as_f64:0.0,
            s: VitteString::empty(),
            b: VitteBytes { ptr: data.as_ptr() as *mut u8, len: data.len(), owned: 0 },
            a: VitteArray::empty()
        };
        core::mem::forget(data); // emprunt non possédé
        let rc = vitte_ffi_call(h, CString::new("echo_bytes").unwrap().as_ptr(), &arg_bytes, 1, &mut out);
        assert_eq!(rc, VitteCode::Ok);
        assert_eq!(out.tag, VitteTag::Bytes);
        unsafe {
            let s = core::slice::from_raw_parts(out.b.ptr as *const u8, out.b.len);
            assert_eq!(s, &[1,2,3,4]);
        }
        vitte_ffi_value_free(out);

        // Array[Int]
        let mut elems = vec![VitteValue::from_i64(10), VitteValue::from_i64(32)];
        let arr = VitteValue {
            tag: VitteTag::Array, as_bool:0, as_i64:0, as_f64:0.0,
            s: VitteString::empty(), b: VitteBytes::empty(),
            a: VitteArray { ptr: elems.as_mut_ptr(), len: elems.len(), owned: 0 }
        };
        core::mem::forget(elems);
        let rc = vitte_ffi_call(h, CString::new("sum").unwrap().as_ptr(), &arr, 1, &mut out);
        assert_eq!(rc, VitteCode::Ok);
        assert_eq!(out.tag, VitteTag::I64);
        assert_eq!(out.as_i64, 42);
        vitte_ffi_value_free(out);

        unsafe { vitte_ffi_free(h) };
    }
}