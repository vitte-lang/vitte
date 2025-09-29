//! vitte-build — Build orchestration for the Vitte language
//!
//! This crate provides high-level orchestration for building,
//! packaging, and managing compiler pipelines of Vitte.
//!
//! Components include:
//! - Pipeline management (lexer → parser → IR → codegen → VM)
//! - Artifact handling (object files, bytecode, executables)
//! - Packaging (distributable bundles, docs, metadata)
//! - Utilities for configuration and reproducible builds

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

use std::path::{Path, PathBuf};
use std::fs;

pub mod pipeline;
pub mod artifacts;
pub mod packaging;
pub mod config;
pub mod utils;

/// Re-export key build API.
pub use pipeline::{BuildPipeline, Stage};
pub use artifacts::{Artifact, ArtifactKind};
pub use packaging::{Package, PackageFormat};
pub use config::{BuildConfig, Profile};

/// Build context holding configuration and workspace path.
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
    pub fn build(&self) -> anyhow::Result<Package> {
        let pipeline = BuildPipeline::new(&self.config);
        let artifacts = pipeline.run(&self.workspace)?;
        let pkg = packaging::create_package(&self.workspace, &artifacts, &self.config)?;
        Ok(pkg)
    }

    /// Clean build artifacts in the workspace.
    pub fn clean(&self) -> anyhow::Result<()> {
        let target = self.workspace.join("target");
        if target.exists() {
            fs::remove_dir_all(&target)?;
        }
        Ok(())
    }
}

/// Smoke entrypoint, not used in real builds.
/// Demonstrates how to use the orchestration API.
pub fn main_smoke() -> anyhow::Result<()> {
    let config = BuildConfig::default();
    let ctx = BuildContext::new(".", config);
    ctx.clean()?;
    let pkg = ctx.build()?;
    println!("Built package: {:?}", pkg);
    Ok(())
}