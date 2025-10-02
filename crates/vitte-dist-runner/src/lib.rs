//! vitte-dist-runner — distributed execution stubs for Vitte.
//!
//! The original crate orchestrated local/SSH/Docker/Kubernetes runners. While
//! those backends are being reworked, we expose a lightweight placeholder so the
//! rest of the workspace can compile. The public API keeps the main data
//! structures (`Target`, `JobSpec`, `Runner`, etc.) but every operation simply
//! reports that the feature is unavailable.

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]

extern crate alloc;

use alloc::{string::String, vec, vec::Vec};

/// Crate-wide result type.
pub type Result<T, E = Error> = core::result::Result<T, E>;

/// Errors produced by the stub implementation.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    /// The requested capability is not implemented in the placeholder.
    #[error("distributed runner unavailable: {0}")]
    Unsupported(&'static str),
}

/// Kinds of execution backends.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum BackendKind {
    /// Run commands locally.
    Local,
    /// Placeholder variants to keep the original surface.
    #[cfg(feature = "ssh")]
    Ssh,
    #[cfg(feature = "docker")]
    Docker,
    #[cfg(feature = "k8s")]
    K8s,
}

/// Execution target metadata.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Target {
    /// Logical name of the node/context.
    pub name: String,
    /// Backend kind.
    pub kind: BackendKind,
}

impl Target {
    /// Creates a new target.
    pub fn new(name: impl Into<String>, kind: BackendKind) -> Self {
        Self { name: name.into(), kind }
    }
}

/// Resource limits (best effort, unused in the stub).
#[derive(Debug, Clone, Copy, Default)]
pub struct Limits {
    /// Timeout (seconds) placeholder.
    pub timeout_secs: Option<u64>,
    /// Memory limit in MiB.
    pub memory_mib: Option<u64>,
    /// CPU milli-cores.
    pub cpu_millis: Option<u32>,
}

/// File transfer specification (kept for compatibility).
#[derive(Debug, Clone, Default)]
pub struct FileSpec {
    /// Local source path (UTF-8 string for the stub).
    pub local: String,
    /// Remote destination path.
    pub remote: String,
    /// Whether the file should be readonly.
    pub readonly: bool,
}

/// Job description passed to runners.
#[derive(Debug, Clone)]
pub struct JobSpec {
    /// Command to execute.
    pub cmd: String,
    /// Arguments.
    pub args: Vec<String>,
    /// Environment variables.
    pub env: Vec<(String, String)>,
    /// Optional working directory.
    pub cwd: Option<String>,
    /// Files to stage.
    pub files: Vec<FileSpec>,
    /// Resource limits.
    pub limits: Limits,
}

impl Default for JobSpec {
    fn default() -> Self {
        Self {
            cmd: "true".into(),
            args: vec![],
            env: vec![],
            cwd: None,
            files: vec![],
            limits: Limits::default(),
        }
    }
}

/// Result of a job execution (placeholder content).
#[derive(Debug, Clone, Default)]
pub struct RunResult {
    /// Exit code.
    pub code: i32,
    /// Captured stdout.
    pub stdout: Vec<u8>,
    /// Captured stderr.
    pub stderr: Vec<u8>,
}

/// Trait implemented by execution backends.
pub trait Runner {
    /// Runs the provided job.
    fn run(&self, job: &JobSpec) -> Result<RunResult>;
}

/// Local runner placeholder.
#[derive(Debug, Default, Clone)]
pub struct LocalRunner;

impl Runner for LocalRunner {
    fn run(&self, job: &JobSpec) -> Result<RunResult> {
        let _ = job;
        Err(Error::Unsupported("local runner inactive"))
    }
}

/// Collection of nodes handled together (stub).
pub struct Cluster {
    nodes: Vec<Target>,
}

impl Cluster {
    /// Creates an empty cluster.
    pub fn new() -> Self {
        Self { nodes: Vec::new() }
    }

    /// Adds a new target to the cluster.
    pub fn add_target(mut self, target: Target) -> Self {
        self.nodes.push(target);
        self
    }

    /// Returns an immutable view of the targets.
    pub fn targets(&self) -> &[Target] {
        &self.nodes
    }

    /// Executes a job on the cluster (unsupported in the stub).
    pub fn run(&self, _job: &JobSpec) -> Result<RunResult> {
        Err(Error::Unsupported("cluster execution not implemented"))
    }
}

impl core::fmt::Debug for Cluster {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        f.debug_struct("Cluster")
            .field("targets", &self.nodes)
            .finish()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn local_runner_stub() {
        let runner = LocalRunner::default();
        let job = JobSpec::default();
        assert!(matches!(runner.run(&job), Err(Error::Unsupported(_))));
    }

    #[test]
    fn cluster_stub() {
        let cluster = Cluster::new().add_target(Target::new("local", BackendKind::Local));
        assert_eq!(cluster.targets().len(), 1);
        assert!(matches!(cluster.run(&JobSpec::default()), Err(Error::Unsupported(_))));
    }
}
