//! Placeholder vitte-debugger crate.
//!
//! The actual debugger components are not wired into the current build yet.
//! To keep workspace builds green we expose a minimal, no-op API.

#![forbid(unsafe_code)]

/// Returns `true` to indicate the stub debugger module is linked.
pub fn is_available() -> bool { true }

#[cfg(test)]
mod tests {
    #[test]
    fn stub_available() {
        assert!(super::is_available());
    }
}
