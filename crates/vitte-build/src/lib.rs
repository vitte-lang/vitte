//! vitte-build — Build orchestration for the Vitte language
//!
//! Minimal, self-contained stubs so the crate compiles.
//! Flesh out modules later without changing the public API surface below.

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use std::fs;
use std::path::{Path, PathBuf};
use std::{error::Error as StdError, fmt};

/// Local result type for this crate.
pub type Result<T> = std::result::Result<T, BuildError>;

/// Errors that can occur during the build orchestration.
#[derive(Debug)]
pub enum BuildError {
    Io(std::io::Error),
    Pipeline(String),
    Packaging(String),
}

impl fmt::Display for BuildError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Io(e) => write!(f, "io error: {e}"),
            Self::Pipeline(m) => write!(f, "pipeline error: {m}"),
            Self::Packaging(m) => write!(f, "packaging error: {m}"),
        }
    }
}

impl StdError for BuildError {}
impl From<std::io::Error> for BuildError {
    fn from(e: std::io::Error) -> Self {
        Self::Io(e)
    }
}

// ========================= Submodules (stubs) =========================

/// Build configuration.
pub mod config {

    /// Build profiles.
    #[derive(Debug, Clone, Copy, Default)]
    pub enum Profile {
        #[default]
        Dev,
        Release,
    }

    /// Top-level build configuration.
    #[derive(Debug, Clone)]
    pub struct BuildConfig {
        pub profile: Profile,
    }

    impl Default for BuildConfig {
        fn default() -> Self {
            Self { profile: Profile::Dev }
        }
    }
}

/// Artifact description.
pub mod artifacts {
    use super::*;

    /// The kind of artifact produced by a stage.
    #[derive(Debug, Clone, Copy)]
    pub enum ArtifactKind {
        Object,
        Bytecode,
        Executable,
    }

    /// A produced artifact on disk.
    #[derive(Debug, Clone)]
    pub struct Artifact {
        pub kind: ArtifactKind,
        pub path: PathBuf,
    }
}

/// Packaging utilities.
pub mod packaging {
    use super::*;
    use crate::artifacts::Artifact;

    /// Package output format.
    #[derive(Debug, Clone, Copy)]
    pub enum PackageFormat {
        Dir,
    }

    /// A packaged build result.
    #[derive(Debug, Clone)]
    pub struct Package {
        pub format: PackageFormat,
        pub path: PathBuf,
        pub files: Vec<PathBuf>,
    }

    /// Create a package from artifacts. Minimal implementation that
    /// copies nothing but records paths.
    pub fn create_package(
        workspace: &Path,
        artifacts: &[Artifact],
        _config: &crate::config::BuildConfig,
    ) -> crate::Result<Package> {
        let out_dir = workspace.join("target/package");
        if !out_dir.exists() {
            fs::create_dir_all(&out_dir)?;
        }
        let files = artifacts.iter().map(|a| a.path.clone()).collect();
        Ok(Package { format: PackageFormat::Dir, path: out_dir, files })
    }
}

/// Build pipeline (lexer → parser → IR → codegen → VM). Stubbed.
pub mod pipeline {
    use super::*;
    use crate::artifacts::{Artifact, ArtifactKind};
    use crate::config::BuildConfig;

    /// A build stage placeholder.
    #[derive(Debug, Clone, Copy)]
    pub enum Stage {
        Lex,
        Parse,
        Ir,
        Codegen,
        Vm,
    }

    /// Orchestrates the stages.
    #[derive(Debug, Clone)]
    pub struct BuildPipeline {
        _cfg: BuildConfig,
    }

    impl BuildPipeline {
        pub fn new(cfg: &BuildConfig) -> Self {
            Self { _cfg: cfg.clone() }
        }

        /// Run the full pipeline and return produced artifacts.
        pub fn run(&self, workspace: &Path) -> crate::Result<Vec<Artifact>> {
            let obj = workspace.join("target/stub.o");
            if let Some(parent) = obj.parent() {
                fs::create_dir_all(parent)?;
            }
            // Touch the file so consumers can rely on its existence.
            if !obj.exists() {
                fs::write(&obj, b"stub")?;
            }
            Ok(vec![Artifact { kind: ArtifactKind::Object, path: obj }])
        }
    }
}

/// Misc utilities (placeholder).
pub mod utils {}

// ========================= Re-exports =========================

/// Re-export key build API.
pub use artifacts::{Artifact, ArtifactKind};
pub use config::{BuildConfig, Profile};
pub use packaging::{Package, PackageFormat};
pub use pipeline::{BuildPipeline, Stage};

/// Build context holding configuration and workspace path.
#[derive(Debug, Clone)]
pub struct BuildContext {
    pub config: BuildConfig,
    pub workspace: PathBuf,
}

impl BuildContext {
    /// Create a new build context with given workspace path.
    pub fn new<P: AsRef<Path>>(workspace: P, config: BuildConfig) -> Self {
        Self { config, workspace: workspace.as_ref().to_path_buf() }
    }

    /// Run a complete pipeline build (lex → parse → compile → package).
    pub fn build(&self) -> Result<Package> {
        let pipeline = BuildPipeline::new(&self.config);
        let artifacts = pipeline.run(&self.workspace)?;
        let pkg = packaging::create_package(&self.workspace, &artifacts, &self.config)?;
        Ok(pkg)
    }

    /// Clean build artifacts in the workspace.
    pub fn clean(&self) -> Result<()> {
        let target = self.workspace.join("target");
        if target.exists() {
            fs::remove_dir_all(&target)?;
        }
        Ok(())
    }
}

/// Smoke entrypoint, not used in real builds.
/// Demonstrates how to use the orchestration API.
pub fn main_smoke() -> Result<()> {
    let config = BuildConfig::default();
    let ctx = BuildContext::new(".", config);
    ctx.clean()?;
    let pkg = ctx.build()?;
    println!("Built package: {:?}", pkg);
    Ok(())
}
