// vitte/rust/crates/vitte_common/src/lib.rs
//
// vitte_common: shared primitives for the Vitte ecosystem.
//
// Goals:
//   - Minimal, reusable building blocks: error model, small utilities, versioning.
//   - Friendly to no_std (with optional alloc/std features).
//
// Suggested Cargo features (adapt to your workspace):
//   default = ["std"]
//   std     = ["alloc"]
//   alloc   = []
//
// This crate intentionally keeps its surface stable.

#![cfg_attr(not(feature = "std"), no_std)]

pub mod err;

pub use err::{ErrCode, ErrKind, Location, VResult, VitteErr, VitteError};

/// Crate version (from Cargo).
pub const VITTE_COMMON_VERSION: &str = env!("CARGO_PKG_VERSION");

/// Compile-time build info (optional).
pub mod build {
    /// Package name.
    pub const PKG_NAME: &str = env!("CARGO_PKG_NAME");
    /// Package version.
    pub const PKG_VERSION: &str = env!("CARGO_PKG_VERSION");
    /// Build profile if provided by cargo (not always set).
    pub const PROFILE: Option<&str> = option_env!("PROFILE");
    /// Target triple if provided.
    pub const TARGET: Option<&str> = option_env!("TARGET");
}

/// Simple prelude for common imports.
pub mod prelude {
    pub use crate::err::{ErrCode, ErrKind, Location, VResult, VitteErr, VitteError};
    pub use crate::{vitte_bail, vitte_err, vitte_loc};
}

// -----------------------------------------------------------------------------
// Optional tiny helpers (keep minimal)
// -----------------------------------------------------------------------------

/// A small helper for checking a pointer-like integer coming from FFI.
///
/// Convention:
///   - `0` is null / invalid.
///   - otherwise valid.
///
/// This is intentionally generic so it can be used by C ABI wrappers.
#[inline]
pub fn is_nonzero_handle(h: usize) -> bool {
    h != 0
}

/// Clamp `v` to [lo, hi].
#[inline]
pub fn clamp_u64(v: u64, lo: u64, hi: u64) -> u64 {
    if v < lo {
        lo
    } else if v > hi {
        hi
    } else {
        v
    }
}

/// Align `value` up to next multiple of `align` (power of two).
#[inline]
pub fn align_up_pow2(value: usize, align: usize) -> usize {
    debug_assert!(align.is_power_of_two());
    (value + (align - 1)) & !(align - 1)
}

/// Align `value` down to multiple of `align` (power of two).
#[inline]
pub fn align_down_pow2(value: usize, align: usize) -> usize {
    debug_assert!(align.is_power_of_two());
    value & !(align - 1)
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn err_basic() {
        let e = vitte_err!(ErrCode::InvalidArgument);
        assert_eq!(e.code(), ErrCode::InvalidArgument);

        #[cfg(feature = "alloc")]
        {
            let e2 = vitte_err!(ErrCode::NotFound, "missing");
            assert_eq!(e2.code(), ErrCode::NotFound);
            assert_eq!(e2.message(), Some("missing"));
        }
    }

    #[test]
    fn align_helpers() {
        assert_eq!(align_up_pow2(0, 16), 0);
        assert_eq!(align_up_pow2(1, 16), 16);
        assert_eq!(align_up_pow2(16, 16), 16);
        assert_eq!(align_up_pow2(17, 16), 32);

        assert_eq!(align_down_pow2(0, 16), 0);
        assert_eq!(align_down_pow2(1, 16), 0);
        assert_eq!(align_down_pow2(16, 16), 16);
        assert_eq!(align_down_pow2(31, 16), 16);
    }

    #[test]
    fn clamp() {
        assert_eq!(clamp_u64(5, 0, 10), 5);
        assert_eq!(clamp_u64(5, 6, 10), 6);
        assert_eq!(clamp_u64(50, 6, 10), 10);
    }

    #[test]
    fn ffi_err() {
        let ok = VitteErr::ok();
        assert!(ok.is_ok());
        let e = VitteErr::from_code(ErrCode::Utf8Invalid);
        assert!(!e.is_ok());
        assert_eq!(e.code, ErrCode::Utf8Invalid as i32);
    }
}
