//! vitte-go — FFI stable pour clients Go via cgo
//!
//! Fournit une ABI C propre : gestion mémoire, erreurs, buffers, contexte opaque,
//! fonctions sync pour compiler/évaluer et I/O simples.
//!
//! Liens côté Go (exemple):
//!   // #cgo LDFLAGS: -lvitte_go
//!   // #include "vitte_go.h"
//!   import "C"
//!
//! Notes ABI:
//! - Toutes les fonctions retournent `VitteGoStatus` (0 = OK).
//! - Les chaînes et buffers retournés doivent être libérés par l’appelant via
//!   `vitte_go_string_free` / `vitte_go_buf_free`.
//! - `ctx` est un pointeur opaque à gérer via `vitte_go_init`/`vitte_go_shutdown`.
//!
//! Build header auto: activez la feature "cbind" et ajoutez un build.rs cbindgen.

#![deny(unsafe_op_in_unsafe_fn)]
#![forbid(unsafe_code)] // FFI impose `unsafe extern "C"` signatures mais code interne safe

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_uint, c_void};
use std::path::Path;
use std::ptr;
use std::sync::{Arc, Mutex};
use thiserror::Error;

// ---------- Statuts ----------

/// Codes de statut stables pour l’ABI.
#[repr(i32)]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum VitteGoStatus {
    Ok = 0,
    Err = 1,
    InvalidArg = 2,
    NotFound = 3,
    Io = 4,
    Unsupported = 5,
}

impl VitteGoStatus {
    #[inline]
    fn from_err(e: &VgError) -> Self {
        match e {
            VgError::InvalidArg(_) => Self::InvalidArg,
            VgError::NotFound(_) => Self::NotFound,
            VgError::Io(_) => Self::Io,
            VgError::Unsupported(_) => Self::Unsupported,
            VgError::Other(_) => Self::Err,
        }
    }
}

// ---------- Erreurs ----------

#[derive(Error, Debug)]
pub enum VgError {
    #[error("invalid arg: {0}")]
    InvalidArg(&'static str),
    #[error("not found: {0}")]
    NotFound(String),
    #[error("io: {0}")]
    Io(String),
    #[error("unsupported: {0}")]
    Unsupported(&'static str),
    #[error("error: {0}")]
    Other(String),
}

thread_local! {
    static LAST_ERROR: std::cell::RefCell<Option<String>> = const { std::cell::RefCell::new(None) };
}

fn set_last_error(msg: String) {
    LAST_ERROR.with(|s| *s.borrow_mut() = Some(msg));
}

fn take_last_error() -> Option<String> {
    LAST_ERROR.with(|s| s.borrow_mut().take())
}

// ---------- Mémoire partagée ----------

/// Buffer C possédé par la lib. L’appelant doit appeler `vitte_go_buf_free`.
#[repr(C)]
pub struct VitteGoBuf {
    pub ptr: *mut u8,
    pub len: usize,
    pub cap: usize,
}

impl VitteGoBuf {
    fn from_vec(mut v: Vec<u8>) -> Self {
        let buf = VitteGoBuf {
            ptr: v.as_mut_ptr(),
            len: v.len(),
            cap: v.capacity(),
        };
        std::mem::forget(v);
        buf
    }
}

// ---------- Contexte opaque ----------

/// État interne. Étendez ici selon votre moteur Vitte.
#[derive(Default)]
struct EngineState {
    log_level: LogLevel,
}

#[derive(Copy, Clone, Debug)]
enum LogLevel {
    Error,
    Warn,
    Info,
    Debug,
    Trace,
}
impl Default for LogLevel {
    fn default() -> Self {
        LogLevel::Warn
    }
}

/// Contexte partagé thread-safe.
#[repr(C)]
pub struct VitteGoCtx {
    inner: Arc<Mutex<EngineState>>,
}

// Helpers

#[inline]
fn to_cstring(s: &str) -> Result<*mut c_char, VgError> {
    CString::new(s).map(|c| c.into_raw()).map_err(|_| VgError::InvalidArg("string contains NUL"))
}

#[inline]
fn from_cstr<'a>(p: *const c_char) -> Result<&'a str, VgError> {
    if p.is_null() {
        return Err(VgError::InvalidArg("null cstring"));
    }
    unsafe {
        CStr::from_ptr(p)
            .to_str()
            .map_err(|_| VgError::InvalidArg("invalid utf-8"))
    }
}

#[inline]
fn status_from<T>(res: Result<T, VgError>) -> VitteGoStatus {
    match res {
        Ok(_) => VitteGoStatus::Ok,
        Err(e) => {
            set_last_error(e.to_string());
            VitteGoStatus::from_err(&e)
        }
    }
}

// ---------- API publique C ----------

#[no_mangle]
pub extern "C" fn vitte_go_version() -> *mut c_char {
    // version du crate (injectée par Cargo) ou fallback
    let ver = option_env!("CARGO_PKG_VERSION").unwrap_or("0.0.0");
    to_cstring(ver).unwrap_or(ptr::null_mut())
}

/// Récupère et consomme le dernier message d’erreur (ou chaîne vide).
#[no_mangle]
pub extern "C" fn vitte_go_last_error() -> *mut c_char {
    let s = take_last_error().unwrap_or_default();
    to_cstring(&s).unwrap_or(ptr::null_mut())
}

/// Libère une chaîne allouée par la lib.
#[no_mangle]
pub unsafe extern "C" fn vitte_go_string_free(s: *mut c_char) {
    if !s.is_null() {
        // # Safety: s provient d’un CString::into_raw
        let _ = unsafe { CString::from_raw(s) };
    }
}

/// Libère un buffer alloué par la lib.
#[no_mangle]
pub unsafe extern "C" fn vitte_go_buf_free(buf: *mut VitteGoBuf) {
    if buf.is_null() {
        return;
    }
    // # Safety: buf a été rempli par from_vec et appartient à l’appelant désormais
    let b = unsafe { &mut *buf };
    if !b.ptr.is_null() && b.cap != 0 {
        // reconstruire Vec pour libération
        unsafe { let _ = Vec::from_raw_parts(b.ptr, b.len, b.cap); }
    }
    b.ptr = ptr::null_mut();
    b.len = 0;
    b.cap = 0;
}

/// Initialise un contexte et le retourne via **out_ctx**.
#[no_mangle]
pub extern "C" fn vitte_go_init(out_ctx: *mut *mut VitteGoCtx) -> VitteGoStatus {
    if out_ctx.is_null() {
        set_last_error("null out_ctx".into());
        return VitteGoStatus::InvalidArg;
    }
    let ctx = VitteGoCtx { inner: Arc::new(Mutex::new(EngineState::default())) };
    let boxed = Box::new(ctx);
    unsafe { *out_ctx = Box::into_raw(boxed) };
    VitteGoStatus::Ok
}

/// Arrête et libère un contexte.
#[no_mangle]
pub unsafe extern "C" fn vitte_go_shutdown(ctx: *mut VitteGoCtx) -> VitteGoStatus {
    if ctx.is_null() {
        return VitteGoStatus::Ok;
    }
    // # Safety: ctx provient de Box::into_raw
    unsafe { let _ = Box::from_raw(ctx); }
    VitteGoStatus::Ok
}

/// Définit le niveau de log: 0=Error 1=Warn 2=Info 3=Debug 4=Trace.
#[no_mangle]
pub unsafe extern "C" fn vitte_go_set_log_level(ctx: *mut VitteGoCtx, level: c_int) -> VitteGoStatus {
    if ctx.is_null() {
        set_last_error("null ctx".into());
        return VitteGoStatus::InvalidArg;
    }
    let lvl = match level {
        0 => LogLevel::Error,
        1 => LogLevel::Warn,
        2 => LogLevel::Info,
        3 => LogLevel::Debug,
        4 => LogLevel::Trace,
        _ => {
            set_last_error("invalid level".into());
            return VitteGoStatus::InvalidArg;
        }
    };
    let ctx = unsafe { &*ctx };
    if let Ok(mut st) = ctx.inner.lock() {
        st.log_level = lvl;
        VitteGoStatus::Ok
    } else {
        set_last_error("mutex poisoned".into());
        VitteGoStatus::Err
    }
}

/// Évalue une “expression” virtuelle (placeholder) et renvoie résultat texte.
/// Entrées: `src` UTF-8.
/// Sortie: `*out_str` string possédée par la lib, à libérer par vitte_go_string_free.
#[no_mangle]
pub unsafe extern "C" fn vitte_go_eval_expr(
    ctx: *mut VitteGoCtx,
    src: *const c_char,
    out_str: *mut *mut c_char,
) -> VitteGoStatus {
    if ctx.is_null() || out_str.is_null() {
        set_last_error("null ctx or out_str".into());
        return VitteGoStatus::InvalidArg;
    }
    let _ = ctx; // non utilisé dans ce squelette
    let res = (|| -> Result<*mut c_char, VgError> {
        let s = from_cstr(src)?;
        // Exemple minimal: renvoyer un JSON {"ok":true,"echo":<src>}
        let json = format!("{{\"ok\":true,\"echo\":\"{}\"}}", json_escape(s));
        to_cstring(&json).map_err(|e| VgError::Other(e.to_string()))
    })();

    match res {
        Ok(cptr) => {
            unsafe { *out_str = cptr };
            VitteGoStatus::Ok
        }
        Err(e) => {
            set_last_error(e.to_string());
            VitteGoStatus::from_err(&e)
        }
    }
}

/// Compile un fichier (placeholder). Écrit un binaire imaginaire en mémoire.
/// Sortie: VitteGoBuf à libérer avec vitte_go_buf_free.
#[no_mangle]
pub unsafe extern "C" fn vitte_go_compile_file(
    ctx: *mut VitteGoCtx,
    path: *const c_char,
    out_buf: *mut VitteGoBuf,
) -> VitteGoStatus {
    if ctx.is_null() || out_buf.is_null() {
        set_last_error("null ctx or out_buf".into());
        return VitteGoStatus::InvalidArg;
    }
    let _ = ctx;
    let status = (|| -> Result<VitteGoBuf, VgError> {
        let p = from_cstr(path)?;
        let pb = Path::new(p);
        if !pb.exists() {
            return Err(VgError::NotFound(p.to_string()));
        }
        let data = std::fs::read(pb).map_err(|e| VgError::Io(e.to_string()))?;
        // “Compilation” factice: préfixer un header magique
        let mut out = b"VITTEOBJ\0".to_vec();
        out.extend_from_slice(&data);
        Ok(VitteGoBuf::from_vec(out))
    })();

    match status {
        Ok(buf) => {
            unsafe { *out_buf = buf };
            VitteGoStatus::Ok
        }
        Err(e) => {
            set_last_error(e.to_string());
            VitteGoStatus::from_err(&e)
        }
    }
}

/// Lit un fichier texte UTF-8 et retourne le contenu.
#[no_mangle]
pub unsafe extern "C" fn vitte_go_read_text_file(
    _ctx: *mut VitteGoCtx,
    path: *const c_char,
    out_str: *mut *mut c_char,
) -> VitteGoStatus {
    if out_str.is_null() {
        set_last_error("null out_str".into());
        return VitteGoStatus::InvalidArg;
    }
    let status = (|| -> Result<*mut c_char, VgError> {
        let p = from_cstr(path)?;
        let s = std::fs::read_to_string(p).map_err(|e| VgError::Io(e.to_string()))?;
        to_cstring(&s).map_err(|e| VgError::Other(e.to_string()))
    })();

    match status {
        Ok(c) => {
            unsafe { *out_str = c };
            VitteGoStatus::Ok
        }
        Err(e) => {
            set_last_error(e.to_string());
            VitteGoStatus::from_err(&e)
        }
    }
}

/// Écrit un fichier binaire.
#[no_mangle]
pub unsafe extern "C" fn vitte_go_write_binary_file(
    _ctx: *mut VitteGoCtx,
    path: *const c_char,
    data_ptr: *const u8,
    data_len: usize,
) -> VitteGoStatus {
    if data_ptr.is_null() && data_len != 0 {
        set_last_error("null data_ptr with non-zero len".into());
        return VitteGoStatus::InvalidArg;
    }
    let status = (|| -> Result<(), VgError> {
        let p = from_cstr(path)?;
        let slice = unsafe { std::slice::from_raw_parts(data_ptr, data_len) };
        std::fs::write(p, slice).map_err(|e| VgError::Io(e.to_string()))
    })();
    status_from(status)
}

// ---------- Callbacks (optionnel) ----------

/// Prototype de callback de log côté Go.
/// level: 0..4 ; msg: C string nul-terminated ; user_data : opaq.
pub type VitteGoLogCb = Option<extern "C" fn(level: c_uint, msg: *const c_char, user_data: *mut c_void)>;

static LOG_CB: parking_lot::RwLock<(VitteGoLogCb, *mut c_void)> = parking_lot::RwLock::new((None, ptr::null_mut()));

#[no_mangle]
pub extern "C" fn vitte_go_set_log_callback(cb: VitteGoLogCb, user_data: *mut c_void) -> VitteGoStatus {
    *LOG_CB.write() = (cb, user_data);
    VitteGoStatus::Ok
}

fn log_emit(level: LogLevel, msg: &str) {
    let (cb, ud) = *LOG_CB.read();
    if let Some(cb) = cb {
        let lv = match level { LogLevel::Error=>0, LogLevel::Warn=>1, LogLevel::Info=>2, LogLevel::Debug=>3, LogLevel::Trace=>4 };
        if let Ok(c) = CString::new(msg) { cb(lv, c.as_ptr(), ud); }
    }
}

// ---------- Utils ----------

fn json_escape(s: &str) -> String {
    let mut out = String::with_capacity(s.len() + 8);
    for ch in s.chars() {
        match ch {
            '\\' => out.push_str("\\\\"),
            '"' => out.push_str("\\\""),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            c if c.is_control() => out.push_str(&format!("\\u{:04x}", c as u32)),
            c => out.push(c),
        }
    }
    out
}

// ---------- Tests basiques ----------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn last_error_roundtrip() {
        set_last_error("x".into());
        assert_eq!(take_last_error(), Some("x".into()));
        assert_eq!(take_last_error(), None);
    }

    #[test]
    fn json_escape_ok() {
        assert_eq!(json_escape("a\"b\\c\n"), "a\\\"b\\\\c\\n");
    }
}