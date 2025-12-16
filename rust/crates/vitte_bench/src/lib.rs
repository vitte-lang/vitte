//! Vitte Benchmark Library
//!
//! Pure Rust benchmarking utilities for Vitte compiler and runtime.
//! This complements the C-based benchmark suite in /bench/src/

pub mod micro;
pub mod macro_bench;
pub mod stats;
pub mod timing;

pub use stats::Stats;
pub use timing::Timer;

/// Benchmark result
#[derive(Debug, Clone)]
pub struct BenchResult {
    pub name: String,
    pub mean_ns: f64,
    pub median_ns: f64,
    pub p95_ns: f64,
    pub min_ns: f64,
    pub max_ns: f64,
    pub samples: usize,
}

impl BenchResult {
    pub fn to_csv_line(&self) -> String {
        format!(
            "{},{:.2},{:.2},{:.2},{:.2},{:.2}",
            self.name, self.mean_ns, self.median_ns, self.p95_ns, self.min_ns, self.max_ns
        )
    }

    pub fn to_string_pretty(&self) -> String {
        format!(
            "{:<30} mean={:>10.2} ns  p95={:>10.2} ns  min={:>10.2} ns  max={:>10.2} ns  ({} samples)",
            self.name, self.mean_ns, self.p95_ns, self.min_ns, self.max_ns, self.samples
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_bench_result_csv() {
        let result = BenchResult {
            name: "test".to_string(),
            mean_ns: 100.0,
            median_ns: 95.0,
            p95_ns: 120.0,
            min_ns: 80.0,
            max_ns: 150.0,
            samples: 10,
        };
        assert!(result.to_csv_line().contains("test"));
    }
}
