//! vitte-incremental — incremental build engine stub.
//!
//! The original crate implemented a dependency graph with fingerprints and task
//! execution. This placeholder keeps the core types (`Engine`, `NodeId`,
//! `Artifact`) but all operations return [`IncError::Unsupported`].

#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

use alloc::vec::Vec;

/// Result alias for incremental operations.
pub type IncResult<T> = Result<T, IncError>;

/// Errors emitted by the incremental engine stub.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum IncError {
    /// Placeholder error when functionality is missing.
    Unsupported(&'static str),
}

impl core::fmt::Display for IncError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            IncError::Unsupported(msg) => write!(f, "incremental engine unavailable: {msg}"),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for IncError {}

/// Identifier for nodes in the dependency graph.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct NodeId(pub usize);

/// Compiled artifact stored in the cache.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct Artifact {
    /// Serialized bytes of the artifact.
    pub bytes: Vec<u8>,
}

impl Artifact {
    /// Creates an artifact from bytes.
    pub fn from_bytes(bytes: impl AsRef<[u8]>) -> Self {
        Self { bytes: bytes.as_ref().to_vec() }
    }
}

/// Build engine placeholder.
#[derive(Debug, Default)]
pub struct Engine;

impl Engine {
    /// Creates a new engine.
    pub fn new() -> Self { Self }

    /// Adds a source file node (stub).
    pub fn add_source_file(&mut self, _path: &str) -> IncResult<NodeId> {
        Err(IncError::Unsupported("incremental engine not implemented"))
    }

    /// Adds a task node (stub).
    pub fn add_task<F>(&mut self, _deps: &[NodeId], _task: F) -> IncResult<NodeId>
    where
        F: Fn(&[Artifact]) -> IncResult<Artifact> + Send + Sync + 'static,
    {
        Err(IncError::Unsupported("incremental engine not implemented"))
    }

    /// Builds the requested targets.
    pub fn build(&mut self, _targets: &[NodeId]) -> IncResult<()> {
        Err(IncError::Unsupported("incremental engine not implemented"))
    }

    /// Retrieves the last artifact for a node.
    pub fn artifact(&self, _id: NodeId) -> IncResult<Artifact> {
        Err(IncError::Unsupported("incremental engine not implemented"))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn engine_stub() {
        let mut engine = Engine::new();
        assert!(matches!(engine.build(&[]), Err(IncError::Unsupported(_))));
    }
}
