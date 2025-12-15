// vitte/rust/crates/vitte_common/src/err.rs
//
// Error model (core) for Vitte ecosystem.
//
// Goals:
//   - Single lightweight error type for libraries/tools
//   - Works in `no_std` (with alloc optional)
//   - Stable error codes for FFI (C ABI) integration
//   - Rich context chain (source errors) when std is available
//
// Notes:
//   - This module deliberately avoids depending on other crates in the workspace.
//   - Feature flags (suggested in Cargo.toml):
//       default = ["std"]
//       std     = ["alloc"]
//       alloc   = []
//
// If you don't use feature flags, this file still compiles under std.
// Adjust cfgs to your actual crate features.

#![allow(dead_code)]

use core::fmt;

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(feature = "alloc")]
use alloc::{boxed::Box, string::String};

// =============================================================================
// Error codes (stable across FFI boundary)
// =============================================================================

/// Stable, ABI-friendly error code space.
///
/// Rules:
/// - `0` is always success.
/// - Negative values are used for "system-ish" failures when mapping errno, etc.
/// - Positive values are library-domain failures.
///
/// Keep this stable once published.
#[repr(i32)]
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum ErrCode {
    Ok = 0,

    // Generic
    Unknown = 1,
    InvalidArgument = 2,
    OutOfRange = 3,
    Overflow = 4,
    Underflow = 5,
    NotFound = 6,
    AlreadyExists = 7,
    Unsupported = 8,
    NotImplemented = 9,
    PermissionDenied = 10,
    WouldBlock = 11,
    TimedOut = 12,
    Interrupted = 13,
    Cancelled = 14,
    CorruptData = 15,
    BadFormat = 16,
    BadState = 17,

    // I/O & filesystem
    Io = 50,
    Eof = 51,
    PathInvalid = 52,
    PathTooLong = 53,
    NoSpace = 54,

    // Encoding / text
    Utf8Invalid = 80,
    Utf16Invalid = 81,
    UnicodeInvalid = 82,

    // Parsing / syntax
    ParseError = 100,
    LexError = 101,
    SyntaxError = 102,

    // Toolchain / build
    ToolNotFound = 150,
    ToolFailed = 151,
    CompileError = 152,
    LinkError = 153,

    // Runtime / VM
    VmTrap = 200,
    VmPanic = 201,
    VmOom = 202,

    // FFI / ABI
    AbiMismatch = 250,
    AbiVersionUnsupported = 251,
    NullPointer = 252,
    BufferTooSmall = 253,
    BadHandle = 254,
}

impl ErrCode {
    #[inline]
    pub const fn as_i32(self) -> i32 {
        self as i32
    }

    #[inline]
    pub const fn is_ok(self) -> bool {
        matches!(self, ErrCode::Ok)
    }
}

// =============================================================================
// Location (source context)
// =============================================================================

/// Minimal source location.
///
/// `file` is `&'static str`, suitable for no_std. Use with `vitte_err!`.
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub struct Location {
    pub file: &'static str,
    pub line: u32,
    pub col: u32,
}

impl Location {
    #[inline]
    pub const fn new(file: &'static str, line: u32, col: u32) -> Self {
        Self { file, line, col }
    }
}

impl fmt::Display for Location {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}:{}", self.file, self.line, self.col)
    }
}

// =============================================================================
// Error kind + payload
// =============================================================================

/// Error class for pattern matching and grouping.
#[derive(Copy, Clone, Debug, PartialEq, Eq, Hash)]
pub enum ErrKind {
    Generic,
    Io,
    Parse,
    Utf,
    Toolchain,
    Vm,
    Abi,
}

impl fmt::Display for ErrKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let s = match self {
            ErrKind::Generic => "generic",
            ErrKind::Io => "io",
            ErrKind::Parse => "parse",
            ErrKind::Utf => "utf",
            ErrKind::Toolchain => "toolchain",
            ErrKind::Vm => "vm",
            ErrKind::Abi => "abi",
        };
        f.write_str(s)
    }
}

/// Optional extra information about an error.
#[derive(Clone, Debug)]
pub struct ErrCtx {
    pub code: ErrCode,
    pub kind: ErrKind,
    pub loc: Option<Location>,
    #[cfg(feature = "alloc")]
    pub msg: Option<String>,
    #[cfg(feature = "alloc")]
    pub details: Vec<(String, String)>,
}

#[cfg(feature = "alloc")]
use alloc::vec::Vec;

impl ErrCtx {
    #[inline]
    pub fn new(code: ErrCode) -> Self {
        Self {
            code,
            kind: kind_from_code(code),
            loc: None,
            #[cfg(feature = "alloc")]
            msg: None,
            #[cfg(feature = "alloc")]
            details: Vec::new(),
        }
    }

    #[inline]
    pub fn with_kind(mut self, kind: ErrKind) -> Self {
        self.kind = kind;
        self
    }

    #[inline]
    pub fn with_loc(mut self, loc: Location) -> Self {
        self.loc = Some(loc);
        self
    }

    #[cfg(feature = "alloc")]
    #[inline]
    pub fn with_msg(mut self, msg: impl Into<String>) -> Self {
        self.msg = Some(msg.into());
        self
    }

    #[cfg(feature = "alloc")]
    #[inline]
    pub fn add_detail(mut self, k: impl Into<String>, v: impl Into<String>) -> Self {
        self.details.push((k.into(), v.into()));
        self
    }
}

fn kind_from_code(code: ErrCode) -> ErrKind {
    use ErrCode::*;
    match code {
        // IO-ish
        Io | Eof | PathInvalid | PathTooLong | NoSpace => ErrKind::Io,

        // UTF-ish
        Utf8Invalid | Utf16Invalid | UnicodeInvalid => ErrKind::Utf,

        // Parse-ish
        ParseError | LexError | SyntaxError => ErrKind::Parse,

        // Toolchain-ish
        ToolNotFound | ToolFailed | CompileError | LinkError => ErrKind::Toolchain,

        // VM-ish
        VmTrap | VmPanic | VmOom => ErrKind::Vm,

        // ABI-ish
        AbiMismatch | AbiVersionUnsupported | NullPointer | BufferTooSmall | BadHandle => ErrKind::Abi,

        _ => ErrKind::Generic,
    }
}

// =============================================================================
// Core error type
// =============================================================================

/// Vitte error.
///
/// Under `std`, implements `std::error::Error` and can carry a source chain.
/// Under `no_std`, still supports code/kind/location and an optional message (alloc).
#[derive(Debug)]
pub struct VitteError {
    pub ctx: ErrCtx,

    #[cfg(feature = "alloc")]
    pub source: Option<Box<dyn core::error::Error + Send + Sync + 'static>>,

    #[cfg(not(feature = "alloc"))]
    pub source: Option<()>,
}

impl VitteError {
    #[inline]
    pub fn new(code: ErrCode) -> Self {
        Self {
            ctx: ErrCtx::new(code),
            #[cfg(feature = "alloc")]
            source: None,
            #[cfg(not(feature = "alloc"))]
            source: None,
        }
    }

    #[inline]
    pub fn code(&self) -> ErrCode {
        self.ctx.code
    }

    #[inline]
    pub fn kind(&self) -> ErrKind {
        self.ctx.kind
    }

    #[inline]
    pub fn location(&self) -> Option<Location> {
        self.ctx.loc
    }

    #[cfg(feature = "alloc")]
    #[inline]
    pub fn message(&self) -> Option<&str> {
        self.ctx.msg.as_deref()
    }

    #[cfg(feature = "alloc")]
    #[inline]
    pub fn details(&self) -> &[(String, String)] {
        &self.ctx.details
    }

    #[inline]
    pub fn with_loc(mut self, loc: Location) -> Self {
        self.ctx.loc = Some(loc);
        self
    }

    #[cfg(feature = "alloc")]
    #[inline]
    pub fn with_msg(mut self, msg: impl Into<String>) -> Self {
        self.ctx.msg = Some(msg.into());
        self
    }

    #[cfg(feature = "alloc")]
    #[inline]
    pub fn with_detail(mut self, k: impl Into<String>, v: impl Into<String>) -> Self {
        self.ctx.details.push((k.into(), v.into()));
        self
    }

    #[cfg(feature = "alloc")]
    #[inline]
    pub fn with_source<E>(mut self, err: E) -> Self
    where
        E: core::error::Error + Send + Sync + 'static,
    {
        self.source = Some(Box::new(err));
        self
    }
}

impl fmt::Display for VitteError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // Example:
        //   [parse:SyntaxError] message (file:line:col) {k=v, ...}
        write!(f, "[{}:{:?}]", self.kind(), self.code())?;

        #[cfg(feature = "alloc")]
        if let Some(m) = self.ctx.msg.as_deref() {
            write!(f, " {}", m)?;
        }

        if let Some(loc) = self.ctx.loc {
            write!(f, " ({})", loc)?;
        }

        #[cfg(feature = "alloc")]
        if !self.ctx.details.is_empty() {
            write!(f, " {{")?;
            for (i, (k, v)) in self.ctx.details.iter().enumerate() {
                if i != 0 {
                    write!(f, ", ")?;
                }
                write!(f, "{}={}", k, v)?;
            }
            write!(f, "}}")?;
        }

        Ok(())
    }
}

#[cfg(feature = "std")]
impl std::error::Error for VitteError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        // We store core::error::Error; in std builds it's compatible.
        #[cfg(feature = "alloc")]
        {
            self.source
                .as_ref()
                .map(|b| &**b as &(dyn std::error::Error + 'static))
        }
        #[cfg(not(feature = "alloc"))]
        {
            None
        }
    }
}

// =============================================================================
// Result alias
// =============================================================================

pub type VResult<T> = core::result::Result<T, VitteError>;

// =============================================================================
// Builders / macros
// =============================================================================

#[macro_export]
macro_rules! vitte_loc {
    () => {
        $crate::err::Location::new(file!(), line!(), column!())
    };
}

#[macro_export]
macro_rules! vitte_err {
    ($code:expr) => {
        $crate::err::VitteError::new($code).with_loc($crate::vitte_loc!())
    };
    ($code:expr, $msg:expr) => {{
        let e = $crate::err::VitteError::new($code).with_loc($crate::vitte_loc!());
        #[cfg(feature = "alloc")]
        {
            e.with_msg($msg)
        }
        #[cfg(not(feature = "alloc"))]
        {
            e
        }
    }};
}

#[macro_export]
macro_rules! vitte_bail {
    ($code:expr) => {
        return Err($crate::vitte_err!($code));
    };
    ($code:expr, $msg:expr) => {
        return Err($crate::vitte_err!($code, $msg));
    };
}

// =============================================================================
// Conversions
// =============================================================================

impl From<ErrCode> for VitteError {
    #[inline]
    fn from(code: ErrCode) -> Self {
        VitteError::new(code)
    }
}

#[cfg(feature = "std")]
impl From<std::io::Error> for VitteError {
    fn from(e: std::io::Error) -> Self {
        let mut ve = VitteError::new(ErrCode::Io).with_loc(Location::new(file!(), line!(), column!()));
        #[cfg(feature = "alloc")]
        {
            ve.ctx.msg = Some(e.to_string());
            ve.source = Some(Box::new(e));
        }
        ve
    }
}

// Optional: mapping from errno-like negative codes (FFI) to ErrCode
pub fn map_errno_to_code(errno: i32) -> ErrCode {
    // Keep this small; extend as needed.
    // errno is typically positive; some APIs use negative.
    let e = if errno < 0 { -errno } else { errno };
    match e {
        2 => ErrCode::NotFound,          // ENOENT
        13 => ErrCode::PermissionDenied, // EACCES
        12 => ErrCode::VmOom,            // ENOMEM (map to oom-ish)
        28 => ErrCode::NoSpace,          // ENOSPC
        _ => ErrCode::Io,
    }
}

// =============================================================================
// FFI helpers (optional)
// =============================================================================

/// ABI-friendly "flat" error value (code only).
///
/// Use when returning errors across C ABI boundaries without allocations.
#[repr(C)]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub struct VitteErr {
    pub code: i32,
}

impl VitteErr {
    #[inline]
    pub const fn ok() -> Self {
        Self { code: 0 }
    }

    #[inline]
    pub const fn from_code(code: ErrCode) -> Self {
        Self { code: code as i32 }
    }

    #[inline]
    pub const fn is_ok(self) -> bool {
        self.code == 0
    }
}

impl From<ErrCode> for VitteErr {
    fn from(c: ErrCode) -> Self {
        VitteErr::from_code(c)
    }
}

impl From<VitteError> for VitteErr {
    fn from(e: VitteError) -> Self {
        VitteErr::from_code(e.code())
    }
}
