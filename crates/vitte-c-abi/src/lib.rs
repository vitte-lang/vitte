//! vitte-c-abi — définition stable de l’ABI C pour Vitte
//!
//! Ce crate expose uniquement des **types FFI**, **constantes de version**,
//! et les **signatures** `extern "C"` attendues côté C. Aucune implémentation.
//!
//! Header C de référence: compatible avec `vitte_c.h`.
//!
//! Stabilité:
//! - Compat ascendante garantie dans la même série `ABI_MAJOR`.
//! - Incompatibilité binaire si `ABI_MAJOR` change.

#![forbid(unsafe_code)]
#![cfg_attr(not(feature = "std"), no_std)]

use core::ffi::c_void;
use core::ptr;
#[cfg(feature = "std")]
use std::ffi::CStr;
use core::ffi::{c_char, c_int, c_uchar};

/// Version ABI (binaire). Incrémentez `ABI_MAJOR` pour tout break.
pub const ABI_MAJOR: u16 = 1;
pub const ABI_MINOR: u16 = 0;
pub const ABI_PATCH: u16 = 0;

/// Encodage version sur 32 bits: 0xMMmmpp (M=major, m=minor, p=patch).
pub const fn abi_version_u32() -> u32 {
    ((ABI_MAJOR as u32) << 16) | ((ABI_MINOR as u32) << 8) | (ABI_PATCH as u32)
}

/// Codes de résultat C.
#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum vitte_c_result {
    VITTE_C_OK = 0,
    VITTE_C_ERR = 1,
}

/// Chaîne UTF-8 possédée par la bibliothèque.
/// À libérer via `vitte_c_free_string`.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct vitte_c_string {
    pub data: *mut c_uchar,
    pub len: usize,
    pub cap: usize,
}

/// Tampon binaire possédé par la bibliothèque.
/// À libérer via `vitte_c_free_buffer`.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct vitte_c_buffer {
    pub data: *mut c_uchar,
    pub len: usize,
    pub cap: usize,
}

/// Erreur structurée.
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct vitte_c_error {
    pub code: c_int,
    pub message: vitte_c_string,
}

/// Vue JSON non possédée (facultative).
#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct vitte_c_json {
    pub ptr: *const c_char,
    pub len: usize,
}

/// Contexte opaque côté C.
#[repr(C)]
pub struct vitte_c_ctx {
    _private: [u8; 0],
}

/// Callback de log optionnel: `level` arbitraire (0=info,1=warn,2=err).
pub type vitte_c_log_cb = Option<extern "C" fn(level: c_int, msg: *const c_char, user: *mut c_void)>;

/// Valeurs utilitaires.
pub const fn vitte_c_json_null() -> vitte_c_json {
    vitte_c_json { ptr: ptr::null(), len: 0 }
}

/* ───────────────────────────── Signatures FFI ───────────────────────────── */

extern "C" {
    /* Mémoire */
    pub fn vitte_c_free_string(s: vitte_c_string);
    pub fn vitte_c_free_buffer(b: vitte_c_buffer);

    /* Contexte explicite (recommandé) */
    pub fn vitte_c_ctx_new() -> *mut vitte_c_ctx;
    pub fn vitte_c_ctx_free(ctx: *mut vitte_c_ctx);

    pub fn vitte_c_ctx_set_config(ctx: *mut vitte_c_ctx, strict: c_int, allow_comments: c_int) -> vitte_c_result;
    pub fn vitte_c_ctx_set_log(ctx: *mut vitte_c_ctx, cb: vitte_c_log_cb, user: *mut c_void) -> vitte_c_result;

    pub fn vitte_c_ctx_version(ctx: *mut vitte_c_ctx, out: *mut vitte_c_string) -> vitte_c_result;
    pub fn vitte_c_ctx_last_error(ctx: *mut vitte_c_ctx, out: *mut vitte_c_string) -> vitte_c_result;

    pub fn vitte_c_ctx_eval_text(
        ctx: *mut vitte_c_ctx,
        src_utf8: *const c_char,
        out_json: *mut vitte_c_string,
        err: *mut vitte_c_error,
    ) -> vitte_c_result;

    pub fn vitte_c_ctx_eval_file(
        ctx: *mut vitte_c_ctx,
        path_utf8: *const c_char,
        out_json: *mut vitte_c_string,
        err: *mut vitte_c_error,
    ) -> vitte_c_result;

    /* Contexte global (compat) */
    pub fn vitte_c_init() -> vitte_c_result;
    pub fn vitte_c_shutdown() -> vitte_c_result;

    pub fn vitte_c_version(out: *mut vitte_c_string) -> vitte_c_result;
    pub fn vitte_c_last_error(out: *mut vitte_c_string) -> vitte_c_result;

    pub fn vitte_c_eval_text(
        src_utf8: *const c_char,
        out_json: *mut vitte_c_string,
        err: *mut vitte_c_error,
    ) -> vitte_c_result;
}

/* ───────────────────────────── Helpers Rust ───────────────────────────── */

impl vitte_c_string {
    /// Retourne une `&str` si UTF-8 valide. Aucun free.
    #[cfg(feature = "std")]
    pub unsafe fn as_str<'a>(&self) -> Option<&'a str> {
        if self.data.is_null() { return Some(""); }
        let s = core::slice::from_raw_parts(self.data, self.len);
        std::str::from_utf8(s).ok()
    }
}

impl vitte_c_error {
    /// Code égal à zéro si non initialisé.
    pub fn is_set(&self) -> bool { self.code != 0 }
}

/* ───────────────────────────── Tests d’ABI ───────────────────────────── */

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sizes_alignments() {
        use core::mem::{align_of, size_of};
        // Hypothèses de portabilité C:
        assert_eq!(size_of::<vitte_c_result>(), size_of::<c_int>());
        assert_eq!(size_of::<vitte_c_string>(), size_of::<*mut c_uchar>() * 1 + size_of::<usize>() * 2);
        assert_eq!(size_of::<vitte_c_buffer>(), size_of::<*mut c_uchar>() * 1 + size_of::<usize>() * 2);
        assert!(align_of::<vitte_c_string>() >= align_of::<*mut c_uchar>());
        assert!(align_of::<vitte_c_buffer>() >= align_of::<*mut c_uchar>());
        assert_eq!(size_of::<vitte_c_error>(), size_of::<c_int>() + size_of::<vitte_c_string>());
        let _v: vitte_c_json = super::vitte_c_json_null();
        assert_eq!(abi_version_u32(), ((ABI_MAJOR as u32) << 16) | ((ABI_MINOR as u32) << 8) | (ABI_PATCH as u32));
    }
}