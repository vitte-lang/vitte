//! vitte-shell — command execution stub for the Vitte toolkit.
//!
//! The full crate provided rich process management (pipelines, PTY, async). To
//! keep the workspace compiling while those pieces are rebuilt, this stub keeps
//! the primary types (`Cmd`, `TextOutput`, etc.) but every execution method
//! returns [`ShellError::Unsupported`].

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::string::String;
use alloc::vec::Vec;

/// Result alias for shell operations.
pub type Result<T, E = ShellError> = core::result::Result<T, E>;

/// Errors that can arise when running shell commands (stubbed).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ShellError {
    /// The requested functionality is not available in the stub.
    Unsupported(&'static str),
}

impl core::fmt::Display for ShellError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            ShellError::Unsupported(msg) => write!(f, "shell backend unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for ShellError {}

/// Captured textual output from a command.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct TextOutput {
    /// Exit status (placeholder, always zero in stub).
    pub status: i32,
    /// Captured stdout.
    pub stdout: String,
    /// Captured stderr.
    pub stderr: String,
}

/// Captured binary output from a command.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct BinOutput {
    /// Exit status (placeholder).
    pub status: i32,
    /// Captured stdout bytes.
    pub stdout: Vec<u8>,
    /// Captured stderr bytes.
    pub stderr: Vec<u8>,
}

/// Command builder placeholder.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct Cmd {
    prog: String,
    args: Vec<String>,
}

impl Cmd {
    /// Creates a new command.
    pub fn new(prog: impl Into<String>) -> Self {
        Self { prog: prog.into(), args: Vec::new() }
    }

    /// Adds an argument to the command (builder pattern).
    pub fn arg(mut self, arg: impl Into<String>) -> Self {
        self.args.push(arg.into());
        self
    }

    /// Adds multiple arguments.
    pub fn args<I, S>(mut self, iter: I) -> Self
    where
        I: IntoIterator<Item = S>,
        S: Into<String>,
    {
        self.args.extend(iter.into_iter().map(Into::into));
        self
    }

    /// Executes the command (stubbed).
    pub fn run(&self) -> Result<i32> {
        Err(ShellError::Unsupported("command execution not implemented"))
    }

    /// Executes and captures textual output.
    pub fn run_capture_text(&self) -> Result<TextOutput> {
        Err(ShellError::Unsupported("command capture not implemented"))
    }

    /// Executes and captures binary output.
    pub fn run_capture_bin(&self) -> Result<BinOutput> {
        Err(ShellError::Unsupported("command capture not implemented"))
    }
}

/// Pipeline placeholder (sequence of commands).
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct Pipeline {
    cmds: Vec<Cmd>,
}

impl Pipeline {
    /// Creates a new empty pipeline.
    pub fn new() -> Self {
        Self { cmds: Vec::new() }
    }

    /// Adds a command stage.
    pub fn stage(mut self, cmd: Cmd) -> Self {
        self.cmds.push(cmd);
        self
    }

    /// Executes the pipeline (stubbed).
    pub fn run(&self) -> Result<i32> {
        Err(ShellError::Unsupported("pipelines not implemented"))
    }
}

/// Splits a shell-like string into arguments using a very simple whitespace rule.
/// This is only a placeholder and does not fully implement shell quoting.
pub fn shlex_split(input: &str) -> Result<Vec<String>> {
    let tokens = input.split_whitespace().map(|s| s.to_string()).collect::<Vec<_>>();
    Ok(tokens)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stub_cmd() {
        let cmd = Cmd::new("echo").arg("hello");
        assert!(matches!(cmd.run(), Err(ShellError::Unsupported(_))));
    }
}
