//! vitte-gui — graphical front-end stub for the Vitte workspace.
//!
//! The full crate integrates `egui`, `wgpu`, and `winit`. This placeholder keeps
//! the public surface minimal so dependent code continues to compile while GUI
//! support is being rebuilt.

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

/// Result alias for GUI operations.
pub type GuiResult<T> = Result<T, GuiError>;

/// Errors returned by the GUI subsystem.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GuiError {
    /// Feature not implemented in the stub.
    Unsupported(&'static str),
}

impl core::fmt::Display for GuiError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            GuiError::Unsupported(msg) => write!(f, "gui backend unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for GuiError {}

/// Trait implemented by applications rendering frames (compatible with egui apps).
pub trait GuiApp {
    /// Called each frame with the GUI context (stubbed).
    fn update(&mut self, _ctx: &StubContext) {}
}

/// Minimal context placeholder passed to [`GuiApp::update`].
#[derive(Debug, Default)]
pub struct StubContext;

/// Native back-end placeholder.
pub mod native {
    use super::*;

    /// Executes a GUI application. The stub returns [`GuiError::Unsupported`].
    pub fn run_native<F>(_title: &str, mut _ui: F) -> GuiResult<()>
    where
        F: FnMut(&crate::StubContext) + 'static,
    {
        Err(GuiError::Unsupported("native GUI backend not implemented"))
    }
}

/// Web/WASM back-end placeholder.
pub mod web {
    use super::*;

    /// Starts the GUI inside a web canvas (stub).
    pub fn start(_canvas_id: &str) -> GuiResult<()> {
        Err(GuiError::Unsupported("web GUI backend not implemented"))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn native_stub() {
        assert!(matches!(native::run_native("demo", |_| {}), Err(GuiError::Unsupported(_))));
    }
}
