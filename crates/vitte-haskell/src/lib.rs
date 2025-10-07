//! vitte-haskell — FFI stable côté Haskell (c2hs/hsc2hs)
//!
//! API C minimaliste pour liaison Haskell via `foreign import ccall`.
//! Convention :
//! - Toutes les fonctions retournent `VitteHsStatus` (0 = OK).
//! - Les chaînes et buffers alloués par la lib doivent être libérés avec
//!   `vitte_hs_string_free` / `vitte_hs_buf_free`.
//! - `ctx` est un pointeur opaque obtenu par `vitte_hs_init` et libéré par
//!   `vitte_hs_shutdown`.
//!
//! Exemple Haskell (esquisse) :
//! ```hs
//! foreign import ccall unsafe "vitte_hs_init"        c_init        :: Ptr (Ptr ()) -> IO CInt
//! foreign import ccall unsafe "vitte_hs_shutdown"    c_shutdown    :: Ptr () -> IO CInt
//! foreign import ccall unsafe "vitte_hs_eval_expr"   c_evalExpr    :: Ptr () -> CString -> Ptr CString -> IO CInt
//! foreign import ccall unsafe "vitte_hs_string_free" c_strFree     :: CString -> IO ()
//! foreign import ccall unsafe "vitte_hs_last_error"  c_lastError   :: IO CString
//! ```

#![deny(unsafe_op_in_unsafe_fn)]
#![deny(missing_docs)]

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_uint, c_void};
use std::path::Path;
use std::ptr;
use std::sync::{Arc, Mutex, RwLock};
use thiserror::Error;

/// Opaque C user data passed back to callbacks.
/// Safety: the embedder guarantees thread-safety of this pointer usage.
#[repr(transparent)]
#[derive(Copy, Clone)]
struct CUserData(*mut c_void);
// Allow sharing across threads for the global callback slot.
unsafe impl Send for CUserData {}
unsafe impl Sync for CUserData {}

/// Codes de statut stables.
#[repr(i32)]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum VitteHsStatus {
    /// Succès.
    Ok = 0,
    /// Erreur générique.
    Err = 1,
    /// Argument invalide.
    InvalidArg = 2,
    /// Ressource non trouvée.
    NotFound = 3,
    /// Erreur I/O.
    Io = 4,
    /// Non supporté.
    Unsupported = 5,
}

impl VitteHsStatus {
    fn from_err(e: &HsError) -> Self {
        match e {
            HsError::InvalidArg(_) => Self::InvalidArg,
            HsError::NotFound(_) => Self::NotFound,
            HsError::Io(_) => Self::Io,
            HsError::Unsupported(_) => Self::Unsupported,
            HsError::Other(_) => Self::Err,
        }
    }
}

/// Erreurs internes.
#[derive(Error, Debug)]
pub enum HsError {
    /// Argument d'API invalide (ex: pointeur nul, UTF-8 invalide).
    #[error("invalid arg: {0}")]
    InvalidArg(&'static str),
    /// Ressource manquante (ex: fichier introuvable).
    #[error("not found: {0}")]
    NotFound(String),
    /// Erreur d'entrée/sortie retournée par le système de fichiers.
    #[error("io: {0}")]
    Io(String),
    /// Fonctionnalité non supportée sur cette plateforme ou build.
    #[error("unsupported: {0}")]
    Unsupported(&'static str),
    /// Erreur générique interne.
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

/// Buffer possédé par la bibliothèque.
#[repr(C)]
pub struct VitteHsBuf {
    /// Pointeur vers les octets.
    pub ptr: *mut u8,
    /// Longueur en octets.
    pub len: usize,
    /// Capacité allouée.
    pub cap: usize,
}

impl VitteHsBuf {
    fn from_vec(mut v: Vec<u8>) -> Self {
        let out = Self {
            ptr: v.as_mut_ptr(),
            len: v.len(),
            cap: v.capacity(),
        };
        std::mem::forget(v);
        out
    }
}

/// Contexte opaque partagé.
#[repr(C)]
pub struct VitteHsCtx {
    inner: Arc<Mutex<EngineState>>,
}

/// État interne simplifié.
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

type LogCb = Option<extern "C" fn(level: c_uint, msg: *const c_char, user_data: *mut c_void)>;

static LOG_SINK: RwLock<(LogCb, CUserData)> = RwLock::new((None, CUserData(ptr::null_mut())));

fn log_emit(level: LogLevel, msg: &str) {
    if let Ok(guard) = LOG_SINK.read() {
        if let Some(cb) = guard.0 {
            let lv = match level {
                LogLevel::Error => 0,
                LogLevel::Warn => 1,
                LogLevel::Info => 2,
                LogLevel::Debug => 3,
                LogLevel::Trace => 4,
            };
            if let Ok(c) = CString::new(msg) {
                cb(lv, c.as_ptr(), guard.1 .0);
            }
        }
    }
}

#[inline]
fn to_cstring(s: &str) -> Result<*mut c_char, HsError> {
    CString::new(s)
        .map(|c| c.into_raw())
        .map_err(|_| HsError::InvalidArg("string contains NUL"))
}

#[inline]
fn from_cstr<'a>(p: *const c_char) -> Result<&'a str, HsError> {
    if p.is_null() {
        return Err(HsError::InvalidArg("null cstring"));
    }
    unsafe { CStr::from_ptr(p).to_str().map_err(|_| HsError::InvalidArg("invalid utf-8")) }
}

fn status_from<T>(r: Result<T, HsError>) -> VitteHsStatus {
    match r {
        Ok(_) => VitteHsStatus::Ok,
        Err(e) => {
            set_last_error(e.to_string());
            VitteHsStatus::from_err(&e)
        }
    }
}

/// Version du crate.
#[no_mangle]
pub extern "C" fn vitte_hs_version() -> *mut c_char {
    let ver = option_env!("CARGO_PKG_VERSION").unwrap_or("0.0.0");
    to_cstring(ver).unwrap_or(ptr::null_mut())
}

/// Récupère et consomme le dernier message d’erreur.
#[no_mangle]
pub extern "C" fn vitte_hs_last_error() -> *mut c_char {
    let s = take_last_error().unwrap_or_default();
    to_cstring(&s).unwrap_or(ptr::null_mut())
}

/// Libère une chaîne allouée par la lib.
#[no_mangle]
pub unsafe extern "C" fn vitte_hs_string_free(s: *mut c_char) {
    if !s.is_null() {
        // Safety: provient d'un CString::into_raw
        unsafe { let _ = CString::from_raw(s); }
    }
}

/// Libère un buffer alloué par la lib.
#[no_mangle]
pub unsafe extern "C" fn vitte_hs_buf_free(buf: *mut VitteHsBuf) {
    if buf.is_null() {
        return;
    }
    // Safety: structure renvoyée par from_vec
    let b = unsafe { &mut *buf };
    if !b.ptr.is_null() && b.cap != 0 {
        unsafe { let _ = Vec::from_raw_parts(b.ptr, b.len, b.cap); }
    }
    b.ptr = ptr::null_mut();
    b.len = 0;
    b.cap = 0;
}

/// Initialise un contexte et le retourne via out_ctx.
#[no_mangle]
pub extern "C" fn vitte_hs_init(out_ctx: *mut *mut VitteHsCtx) -> VitteHsStatus {
    if out_ctx.is_null() {
        set_last_error("null out_ctx".into());
        return VitteHsStatus::InvalidArg;
    }
    let ctx = VitteHsCtx {
        inner: Arc::new(Mutex::new(EngineState::default())),
    };
    let boxed = Box::new(ctx);
    unsafe { *out_ctx = Box::into_raw(boxed) };
    VitteHsStatus::Ok
}

/// Arrête et libère un contexte.
#[no_mangle]
pub unsafe extern "C" fn vitte_hs_shutdown(ctx: *mut VitteHsCtx) -> VitteHsStatus {
    if ctx.is_null() {
        return VitteHsStatus::Ok;
    }
    // Safety: ctx provient de Box::into_raw
    unsafe { let _ = Box::from_raw(ctx); }
    VitteHsStatus::Ok
}

/// Définit le niveau de log: 0=Error..4=Trace.
#[no_mangle]
pub unsafe extern "C" fn vitte_hs_set_log_level(ctx: *mut VitteHsCtx, level: c_int) -> VitteHsStatus {
    if ctx.is_null() {
        set_last_error("null ctx".into());
        return VitteHsStatus::InvalidArg;
    }
    let lvl = match level {
        0 => LogLevel::Error,
        1 => LogLevel::Warn,
        2 => LogLevel::Info,
        3 => LogLevel::Debug,
        4 => LogLevel::Trace,
        _ => {
            set_last_error("invalid level".into());
            return VitteHsStatus::InvalidArg;
        }
    };
    let ctx = unsafe { &*ctx };
    match ctx.inner.lock() {
        Ok(mut st) => {
            st.log_level = lvl;
            VitteHsStatus::Ok
        }
        Err(_) => {
            set_last_error("mutex poisoned".into());
            VitteHsStatus::Err
        }
    }
}

/// Enregistre un callback de log optionnel côté Haskell.
#[no_mangle]
pub extern "C" fn vitte_hs_set_log_callback(cb: LogCb, user_data: *mut c_void) -> VitteHsStatus {
    if let Ok(mut g) = LOG_SINK.write() {
        *g = (cb, CUserData(user_data));
        VitteHsStatus::Ok
    } else {
        set_last_error("log sink lock failed".into());
        VitteHsStatus::Err
    }
}

/// Évalue une pseudo-expression et renvoie un JSON texte.
/// Sortie: *out_str à libérer avec `vitte_hs_string_free`.
#[no_mangle]
pub unsafe extern "C" fn vitte_hs_eval_expr(
    ctx: *mut VitteHsCtx,
    src: *const c_char,
    out_str: *mut *mut c_char,
) -> VitteHsStatus {
    if ctx.is_null() || out_str.is_null() {
        set_last_error("null ctx or out_str".into());
        return VitteHsStatus::InvalidArg;
    }
    let _ = ctx;
    let r = (|| -> Result<*mut c_char, HsError> {
        let s = from_cstr(src)?;
        let json = format!("{{\"ok\":true,\"echo\":\"{}\"}}", json_escape(s));
        to_cstring(&json).map_err(|e| HsError::Other(e.to_string()))
    })();

    match r {
        Ok(cptr) => {
            unsafe { *out_str = cptr };
            log_emit(LogLevel::Info, "eval_expr ok");
            VitteHsStatus::Ok
        }
        Err(e) => {
            set_last_error(e.to_string());
            VitteHsStatus::from_err(&e)
        }
    }
}

/// Compile un fichier (placeholder): renvoie un buffer binaire avec en-tête.
/// Buffer à libérer via `vitte_hs_buf_free`.
#[no_mangle]
pub unsafe extern "C" fn vitte_hs_compile_file(
    ctx: *mut VitteHsCtx,
    path: *const c_char,
    out_buf: *mut VitteHsBuf,
) -> VitteHsStatus {
    if ctx.is_null() || out_buf.is_null() {
        set_last_error("null ctx or out_buf".into());
        return VitteHsStatus::InvalidArg;
    }
    let _ = ctx;
    let r = (|| -> Result<VitteHsBuf, HsError> {
        let p = from_cstr(path)?;
        let pb = Path::new(p);
        if !pb.exists() {
            return Err(HsError::NotFound(p.to_string()));
        }
        let mut data = std::fs::read(pb).map_err(|e| HsError::Io(e.to_string()))?;
        let mut out = b"VITTEOBJ\0".to_vec();
        out.append(&mut data);
        Ok(VitteHsBuf::from_vec(out))
    })();

    match r {
        Ok(buf) => {
            unsafe { *out_buf = buf };
            log_emit(LogLevel::Debug, "compile_file ok");
            VitteHsStatus::Ok
        }
        Err(e) => {
            set_last_error(e.to_string());
            VitteHsStatus::from_err(&e)
        }
    }
}

/// Lit un fichier texte UTF-8 et retourne le contenu.
#[no_mangle]
pub unsafe extern "C" fn vitte_hs_read_text_file(
    _ctx: *mut VitteHsCtx,
    path: *const c_char,
    out_str: *mut *mut c_char,
) -> VitteHsStatus {
    if out_str.is_null() {
        set_last_error("null out_str".into());
        return VitteHsStatus::InvalidArg;
    }
    let r = (|| -> Result<*mut c_char, HsError> {
        let p = from_cstr(path)?;
        let s = std::fs::read_to_string(p).map_err(|e| HsError::Io(e.to_string()))?;
        to_cstring(&s).map_err(|e| HsError::Other(e.to_string()))
    })();

    match r {
        Ok(c) => {
            unsafe { *out_str = c };
            VitteHsStatus::Ok
        }
        Err(e) => {
            set_last_error(e.to_string());
            VitteHsStatus::from_err(&e)
        }
    }
}

/// Écrit un fichier binaire à partir d’un pointeur/longueur.
#[no_mangle]
pub unsafe extern "C" fn vitte_hs_write_binary_file(
    _ctx: *mut VitteHsCtx,
    path: *const c_char,
    data_ptr: *const u8,
    data_len: usize,
) -> VitteHsStatus {
    if data_ptr.is_null() && data_len != 0 {
        set_last_error("null data_ptr with non-zero len".into());
        return VitteHsStatus::InvalidArg;
    }
    let r = (|| -> Result<(), HsError> {
        let p = from_cstr(path)?;
        let slice = unsafe { std::slice::from_raw_parts(data_ptr, data_len) };
        std::fs::write(p, slice).map_err(|e| HsError::Io(e.to_string()))
    })();
    status_from(r)
}

// ---------------- Utils ----------------

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

// ---------------- Tests ----------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn error_roundtrip() {
        set_last_error("e".into());
        assert_eq!(take_last_error(), Some("e".into()));
        assert_eq!(take_last_error(), None);
    }

    #[test]
    fn json_escape_ok() {
        assert_eq!(json_escape("a\"b\\\n"), "a\\\"b\\\\\\n");
    }

    #[test]
    fn status_map() {
        assert_eq!(VitteHsStatus::from_err(&HsError::InvalidArg("x")), VitteHsStatus::InvalidArg);
        assert_eq!(VitteHsStatus::from_err(&HsError::NotFound("x".into())), VitteHsStatus::NotFound);
    }
}