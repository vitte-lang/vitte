//! Macro-benchmarks (millisecond-scale)

use crate::timing::Timer;
use crate::stats::Stats;
use crate::BenchResult;

/// Benchmark string operations (concatenation, search, case conversion)
pub fn bench_string_operations(iterations: usize) -> BenchResult {
    let mut samples = Vec::new();

    for _ in 0..5 {
        let timer = Timer::start();

        for _ in 0..iterations {
            let mut result = String::new();
            for i in 0..100 {
                result.push_str(&format!("item_{} ", i));
            }
            let upper = result.to_uppercase();
            let lower = upper.to_lowercase();
            let _ = lower.contains("item_50");
        }

        samples.push(timer.elapsed_ns() as f64 / iterations as f64 / 1000.0); // Convert to µs
    }

    let stats = Stats::new(samples.clone());
    BenchResult {
        name: "rust:string_ops".to_string(),
        mean_ns: stats.mean() * 1000.0,
        median_ns: stats.median() * 1000.0,
        p95_ns: stats.p95() * 1000.0,
        min_ns: stats.min() * 1000.0,
        max_ns: stats.max() * 1000.0,
        samples: samples.len(),
    }
}

/// Benchmark sorting algorithms
pub fn bench_sorting(iterations: usize) -> BenchResult {
    let mut samples = Vec::new();

    for _ in 0..5 {
        let timer = Timer::start();

        for iter in 0..iterations {
            let mut data: Vec<u32> = (0..1000)
                .map(|i| (i as u32 * 37 + 11 + iter as u32) % 2000)
                .collect();

            // Simple sort (compiler may optimize differently than C)
            data.sort_unstable();
        }

        samples.push(timer.elapsed_ns() as f64 / iterations as f64 / 1000.0);
    }

    let stats = Stats::new(samples.clone());
    BenchResult {
        name: "rust:sorting".to_string(),
        mean_ns: stats.mean() * 1000.0,
        median_ns: stats.median() * 1000.0,
        p95_ns: stats.p95() * 1000.0,
        min_ns: stats.min() * 1000.0,
        max_ns: stats.max() * 1000.0,
        samples: samples.len(),
    }
}

/// Benchmark JSON-like parsing (simulated)
pub fn bench_parsing(iterations: usize) -> BenchResult {
    let mut samples = Vec::new();

    let json_sample = r#"{"users":[{"id":1,"name":"Alice","age":30},{"id":2,"name":"Bob","age":25}],"count":2}"#;

    for _ in 0..5 {
        let timer = Timer::start();

        for _ in 0..iterations {
            // Simple state machine parsing
            let mut depth: i32 = 0;
            let mut in_string = false;

            for ch in json_sample.chars() {
                match ch {
                    '"' => in_string = !in_string,
                    '{' | '[' if !in_string => depth += 1,
                    '}' | ']' if !in_string => depth = depth.saturating_sub(1),
                    _ => {}
                }
            }
        }

        samples.push(timer.elapsed_ns() as f64 / iterations as f64 / 1000.0);
    }

    let stats = Stats::new(samples.clone());
    BenchResult {
        name: "rust:parsing".to_string(),
        mean_ns: stats.mean() * 1000.0,
        median_ns: stats.median() * 1000.0,
        p95_ns: stats.p95() * 1000.0,
        min_ns: stats.min() * 1000.0,
        max_ns: stats.max() * 1000.0,
        samples: samples.len(),
    }
}

/// Benchmark vector operations
pub fn bench_vectors(iterations: usize) -> BenchResult {
    let mut samples = Vec::new();

    for _ in 0..5 {
        let timer = Timer::start();

        for _ in 0..iterations {
            let mut vec = Vec::new();
            for i in 0..1000 {
                vec.push(i * 37 + 11);
            }
            vec.iter().sum::<usize>();
            vec.sort_unstable();
        }

        samples.push(timer.elapsed_ns() as f64 / iterations as f64 / 1000.0);
    }

    let stats = Stats::new(samples.clone());
    BenchResult {
        name: "rust:vectors".to_string(),
        mean_ns: stats.mean() * 1000.0,
        median_ns: stats.median() * 1000.0,
        p95_ns: stats.p95() * 1000.0,
        min_ns: stats.min() * 1000.0,
        max_ns: stats.max() * 1000.0,
        samples: samples.len(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_macro_benchmarks() {
        let result = bench_sorting(10);
        assert!(result.mean_ns > 0.0);
    }
}
