//! vitte-bench — Benchmarks suite for the Vitte language
//!
//! This crate provides Criterion-based benchmarks covering
//! different layers of the Vitte toolchain:
//! - Lexer
//! - Parser
//! - IR passes
//! - VM hotpath and microbenchmarks
//! - End-to-end pipelines

#![forbid(unsafe_code)]
#![warn(clippy::all, clippy::pedantic, clippy::nursery)]
#![allow(
    clippy::module_name_repetitions,
    clippy::doc_markdown,
    clippy::too_many_lines
)]

pub mod benches {
    //! Individual benchmark modules.
    //!
    //! Each module defines a `criterion_benchmark` entrypoint.

    pub mod lexer;
    pub mod parser;
    pub mod ir_passes;
    pub mod vm_micro;
    pub mod vm_hotpath;
    pub mod e2e_pipeline;
    pub mod end_to_end;
}

pub mod utils {
    //! Shared utilities used in benchmarks.

    use rand::{rngs::StdRng, Rng, SeedableRng};
    use std::time::{Duration, Instant};

    /// Generate a random string of given length using alphanumeric chars.
    pub fn random_string(len: usize) -> String {
        let mut rng = StdRng::from_entropy();
        (0..len)
            .map(|_| rng.sample(rand::distributions::Alphanumeric) as char)
            .collect()
    }

    /// Repeat a string `n` times to form a larger input.
    pub fn repeat_input(base: &str, n: usize) -> String {
        base.repeat(n)
    }

    /// Simple timer for non-Criterion micro measurements.
    pub struct Timer {
        start: Instant,
    }

    impl Timer {
        /// Start a new timer.
        pub fn start() -> Self {
            Self { start: Instant::now() }
        }

        /// Elapsed duration since start.
        pub fn elapsed(&self) -> Duration {
            self.start.elapsed()
        }
    }

    /// Generate synthetic Vitte source code of approximate size.
    pub fn synthetic_source(size: usize) -> String {
        let base = "fn foo(x: i32) -> i32 { x + 1 }\n";
        repeat_input(base, size / base.len().max(1))
    }
}

/// Entry point for manual smoke runs.
///
/// In practice, benchmarks are executed with `cargo bench`.
pub fn main_smoke() {
    let src = utils::synthetic_source(1_000);
    println!("Synthetic source of len {} ready for benches.", src.len());
}