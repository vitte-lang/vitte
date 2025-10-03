//! vitte-gpu — GPU abstraction stub for the Vitte workspace.
//!
//! The real crate wraps `wgpu` to provide device/queue helpers. This placeholder
//! keeps a small subset of the API so dependent crates compile while all
//! methods simply return [`GpuError::Unsupported`].

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

/// Result alias used by the GPU helpers.
pub type GpuResult<T> = core::result::Result<T, GpuError>;

/// Errors that can occur within the GPU abstraction.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GpuError {
    /// GPU functionality is not available in the stub.
    Unsupported(&'static str),
}

impl core::fmt::Display for GpuError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            GpuError::Unsupported(msg) => write!(f, "gpu backend unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for GpuError {}

/// Placeholder GPU context.
#[derive(Debug, Default, Clone)]
pub struct GpuContext;

impl GpuContext {
    /// Creates a new GPU context. Always returns [`GpuError::Unsupported`].
    pub async fn init(_high_perf: bool) -> GpuResult<Self> {
        Err(GpuError::Unsupported("wgpu backend not enabled"))
    }

    /// Submits a no-op command (stubbed).
    pub fn submit_noop(&self) {
        let _ = self;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use pollster::block_on;

    #[test]
    fn init_stub() {
        let fut = GpuContext::init(false);
        let res = block_on(fut);
        assert!(matches!(res, Err(GpuError::Unsupported(_))));
    }
}
