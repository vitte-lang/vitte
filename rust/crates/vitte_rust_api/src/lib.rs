// vitte/rust/crates/vitte_rust_api/src/lib.rs
//
// vitte_rust_api — the only Rust crate exposed to the C side (FFI surface)
//
// Goals:
//   - Provide a stable C ABI for selected Rust functionality:
//       * Muffin (.muf) parse / normalize
//       * Regex helpers (optional)
//       * UTF-8 validation helpers (optional)
//       * Common error model + last-error string
//   - Keep ABI stable with explicit versioning
//   - Avoid panics across FFI boundary
//
// Design:
//   - All extern "C" functions are `#[no_mangle]`
//   - Use explicit `vitte_abi_*` structs + error codes
//   - Provide "handle" types for owned Rust objects (regex, manifest)
//   - Provide thread-local last error string for debugging.
//
// Build:
//   - crate-type: ["staticlib", "cdylib"]
//
// Safety:
//   - All pointers are validated.
//   - Output buffers are bounded.
//   - Caller owns freeing where indicated.
//
// NOTE:
//   - This file expects vitte_common::ErrCode and VitteError as canonical error model.
//   - If you already have a C-side abi header, align names exactly with it.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate core;

#[cfg(feature = "alloc")]
extern crate alloc;

use core::ffi::c_void;
use core::ptr;

use vitte_common::err::{ErrCode, VitteErr, VitteError};

#[cfg(feature = "alloc")]
use alloc::string::String;

#[cfg(feature = "alloc")]
use alloc::boxed::Box;

// Optional backends
#[cfg(feature = "muf")]
use vitte_muf::{parse_manifest_str, pretty::manifest_to_string};

#[cfg(feature = "regex")]
use vitte_regex::{RegexFlags, VRegex};

// =============================================================================
// ABI constants
// =============================================================================

/// ABI version for vitte_rust_api (increment on breaking changes).
pub const VITTE_RUST_API_ABI_VERSION: u32 = 1;

// =============================================================================
// Last error (thread-local, debug support)
// =============================================================================

#[cfg(feature = "alloc")]
mod last_error {
    use super::*;
    use core::cell::RefCell;

    thread_local! {
        static LAST: RefCell<Option<String>> = RefCell::new(None);
    }

    pub fn set(msg: String) {
        LAST.with(|s| *s.borrow_mut() = Some(msg));
    }

    pub fn clear() {
        LAST.with(|s| *s.borrow_mut() = None);
    }

    pub fn get() -> Option<String> {
        LAST.with(|s| s.borrow().clone())
    }
}

#[cfg(not(feature = "alloc"))]
mod last_error {
    pub fn set(_: ()) {}
    pub fn clear() {}
    pub fn get() -> Option<()> { None }
}

// =============================================================================
// FFI-friendly types
// =============================================================================

/// C slice (ptr + len).
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct VitteSliceU8 {
    pub ptr: *const u8,
    pub len: usize,
}

/// Mutable C slice (ptr + len).
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct VitteSliceMutU8 {
    pub ptr: *mut u8,
    pub len: usize,
}

/// C string view (ptr + len), not null-terminated.
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct VitteStr {
    pub ptr: *const u8,
    pub len: usize,
}

/// Output: error code + optional bytes written.
#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub struct VitteStatus {
    pub code: i32,
    pub written: usize,
}

impl VitteStatus {
    #[inline]
    pub const fn ok_written(written: usize) -> Self {
        Self { code: 0, written }
    }

    #[inline]
    pub const fn err(code: ErrCode) -> Self {
        Self { code: code as i32, written: 0 }
    }
}

/// Opaque handle.
#[repr(C)]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct VitteHandle {
    pub raw: usize,
}

impl VitteHandle {
    #[inline]
    pub const fn null() -> Self {
        Self { raw: 0 }
    }

    #[inline]
    pub const fn is_null(self) -> bool {
        self.raw == 0
    }
}

// =============================================================================
// Internal helpers
// =============================================================================

#[inline]
fn clear_last_error() {
    last_error::clear();
}

#[cfg(feature = "alloc")]
fn set_last_error(e: &VitteError) {
    last_error::set(e.to_string());
}

#[cfg(not(feature = "alloc"))]
fn set_last_error(_: &VitteError) {}

#[inline]
fn null_ptr<T>() -> *mut T {
    ptr::null_mut()
}

#[inline]
unsafe fn slice_from_raw<'a>(s: VitteSliceU8) -> Result<&'a [u8], ErrCode> {
    if s.ptr.is_null() && s.len != 0 {
        return Err(ErrCode::NullPointer);
    }
    Ok(core::slice::from_raw_parts(s.ptr, s.len))
}

#[inline]
unsafe fn mut_slice_from_raw<'a>(s: VitteSliceMutU8) -> Result<&'a mut [u8], ErrCode> {
    if s.ptr.is_null() && s.len != 0 {
        return Err(ErrCode::NullPointer);
    }
    Ok(core::slice::from_raw_parts_mut(s.ptr, s.len))
}

#[cfg(feature = "alloc")]
#[inline]
unsafe fn str_from_raw(s: VitteStr) -> Result<&'static str, ErrCode> {
    let bytes = slice_from_raw::< 'static >(VitteSliceU8 { ptr: s.ptr, len: s.len })?;
    core::str::from_utf8(bytes).map_err(|_| ErrCode::Utf8Invalid)
}

#[cfg(not(feature = "alloc"))]
#[inline]
unsafe fn str_from_raw(_: VitteStr) -> Result<&'static str, ErrCode> {
    Err(ErrCode::Unsupported)
}

#[cfg(feature = "alloc")]
fn write_bytes(out: VitteSliceMutU8, bytes: &[u8]) -> Result<usize, ErrCode> {
    unsafe {
        let dst = mut_slice_from_raw(out)?;
        if dst.len() < bytes.len() {
            return Err(ErrCode::BufferTooSmall);
        }
        dst[..bytes.len()].copy_from_slice(bytes);
        Ok(bytes.len())
    }
}

// =============================================================================
// ABI info
// =============================================================================

#[no_mangle]
pub extern "C" fn vitte_rust_api_abi_version() -> u32 {
    VITTE_RUST_API_ABI_VERSION
}

#[no_mangle]
pub extern "C" fn vitte_rust_api_clear_last_error() {
    clear_last_error();
}

/// Write last error string into `out` (not NUL-terminated).
/// Returns VitteStatus { code, written }.
/// - code=0 => ok
/// - code=BufferTooSmall => required size is returned in `written`
#[no_mangle]
pub extern "C" fn vitte_rust_api_last_error(out: VitteSliceMutU8) -> VitteStatus {
    clear_last_error(); // no-op if none in no-alloc

    #[cfg(feature = "alloc")]
    {
        if let Some(msg) = last_error::get() {
            let bytes = msg.as_bytes();
            match write_bytes(out, bytes) {
                Ok(n) => VitteStatus::ok_written(n),
                Err(ErrCode::BufferTooSmall) => VitteStatus { code: ErrCode::BufferTooSmall as i32, written: bytes.len() },
                Err(c) => VitteStatus::err(c),
            }
        } else {
            VitteStatus::ok_written(0)
        }
    }

    #[cfg(not(feature = "alloc"))]
    {
        let _ = out;
        VitteStatus::ok_written(0)
    }
}

// =============================================================================
// Muffin (.muf) API
// =============================================================================

/// Parse a Muffin manifest and return a normalized string representation.
/// Inputs:
///   - `src`: utf-8 bytes (manifest text)
/// Output:
///   - writes normalized manifest to `out`
/// Return:
///   - code=0 success
///   - BufferTooSmall with required size in written
#[no_mangle]
pub extern "C" fn vitte_muf_parse_and_normalize(src: VitteStr, out: VitteSliceMutU8) -> VitteStatus {
    clear_last_error();

    #[cfg(feature = "muf")]
    {
        #[cfg(feature = "alloc")]
        {
            let s = unsafe {
                match str_from_raw(src) {
                    Ok(v) => v,
                    Err(c) => return VitteStatus::err(c),
                }
            };

            let m = match parse_manifest_str(s) {
                Ok(v) => v,
                Err(e) => {
                    set_last_error(&e);
                    return VitteStatus::err(e.code());
                }
            };

            let pretty = manifest_to_string(&m);
            let bytes = pretty.as_bytes();

            match write_bytes(out, bytes) {
                Ok(n) => VitteStatus::ok_written(n),
                Err(ErrCode::BufferTooSmall) => VitteStatus { code: ErrCode::BufferTooSmall as i32, written: bytes.len() },
                Err(c) => VitteStatus::err(c),
            }
        }

        #[cfg(not(feature = "alloc"))]
        {
            let _ = (src, out);
            VitteStatus::err(ErrCode::Unsupported)
        }
    }

    #[cfg(not(feature = "muf"))]
    {
        let _ = (src, out);
        VitteStatus::err(ErrCode::NotImplemented)
    }
}

// =============================================================================
// Regex API (handles)
// =============================================================================

#[cfg(feature = "alloc")]
struct RegexHandle {
    re: VRegex,
}

#[no_mangle]
pub extern "C" fn vitte_regex_compile(pattern: VitteStr, flags: u32, out_handle: *mut VitteHandle) -> i32 {
    clear_last_error();

    if out_handle.is_null() {
        return ErrCode::NullPointer as i32;
    }

    #[cfg(all(feature = "regex", feature = "alloc"))]
    {
        let pat = unsafe {
            match str_from_raw(pattern) {
                Ok(v) => v,
                Err(c) => return c as i32,
            }
        };

        let f = RegexFlags::from_bits_truncate(flags);
        let re = match VRegex::with_flags(pat, f) {
            Ok(v) => v,
            Err(e) => {
                set_last_error(&e);
                return e.code() as i32;
            }
        };

        let h = Box::new(RegexHandle { re });
        let raw = Box::into_raw(h) as usize;

        unsafe {
            *out_handle = VitteHandle { raw };
        }
        0
    }

    #[cfg(not(all(feature = "regex", feature = "alloc")))]
    {
        let _ = (pattern, flags);
        unsafe { *out_handle = VitteHandle::null(); }
        ErrCode::NotImplemented as i32
    }
}

#[no_mangle]
pub extern "C" fn vitte_regex_free(handle: VitteHandle) {
    clear_last_error();

    #[cfg(all(feature = "regex", feature = "alloc"))]
    {
        if handle.raw == 0 {
            return;
        }
        unsafe {
            let _ = Box::from_raw(handle.raw as *mut RegexHandle);
        }
    }

    #[cfg(not(all(feature = "regex", feature = "alloc")))]
    {
        let _ = handle;
    }
}

/// Find first match.
/// Returns code=0 and writes start/end into out params; else error code.
#[no_mangle]
pub extern "C" fn vitte_regex_find_first(
    handle: VitteHandle,
    haystack: VitteStr,
    out_start: *mut usize,
    out_end: *mut usize,
) -> i32 {
    clear_last_error();

    if out_start.is_null() || out_end.is_null() {
        return ErrCode::NullPointer as i32;
    }

    #[cfg(all(feature = "regex", feature = "alloc"))]
    {
        if handle.raw == 0 {
            return ErrCode::BadHandle as i32;
        }

        let s = unsafe {
            match str_from_raw(haystack) {
                Ok(v) => v,
                Err(c) => return c as i32,
            }
        };

        let h = unsafe { &*(handle.raw as *const RegexHandle) };
        if let Some(m) = h.re.find(s) {
            unsafe {
                *out_start = m.start;
                *out_end = m.end;
            }
            0
        } else {
            unsafe {
                *out_start = 0;
                *out_end = 0;
            }
            ErrCode::NotFound as i32
        }
    }

    #[cfg(not(all(feature = "regex", feature = "alloc")))]
    {
        let _ = (handle, haystack);
        unsafe {
            *out_start = 0;
            *out_end = 0;
        }
        ErrCode::NotImplemented as i32
    }
}

// =============================================================================
// UTF-8 validate helper (pure Rust)
// =============================================================================

#[no_mangle]
pub extern "C" fn vitte_utf8_validate(src: VitteSliceU8) -> i32 {
    clear_last_error();

    let bytes = unsafe {
        match slice_from_raw(src) {
            Ok(v) => v,
            Err(c) => return c as i32,
        }
    };

    // Strict UTF-8? Rust's validation is strict UTF-8 for byte sequences.
    // If you need "no surrogates / max codepoint" this already holds for UTF-8.
    match core::str::from_utf8(bytes) {
        Ok(_) => 1,
        Err(_) => 0,
    }
}
