//! Micro-benchmarks (nanosecond-scale)

use crate::timing::Timer;
use crate::stats::Stats;
use crate::BenchResult;

/// Benchmark integer arithmetic throughput
pub fn bench_integer_arithmetic(iters: usize) -> BenchResult {
    let mut samples = Vec::new();

    for _ in 0..10 {
        let timer = Timer::start();
        let mut x: u64 = 0;
        for i in 0..iters {
            x = x.wrapping_add(i as u64);
            x = x.wrapping_mul(0xDEADBEEF);
        }
        samples.push(timer.elapsed_ns() as f64 / iters as f64);
    }

    let stats = Stats::new(samples.clone());
    BenchResult {
        name: "rust:arithmetic".to_string(),
        mean_ns: stats.mean(),
        median_ns: stats.median(),
        p95_ns: stats.p95(),
        min_ns: stats.min(),
        max_ns: stats.max(),
        samples: samples.len(),
    }
}

/// Benchmark bitwise operations
pub fn bench_bitwise_ops(iters: usize) -> BenchResult {
    let mut samples = Vec::new();

    for _ in 0..10 {
        let timer = Timer::start();
        let mut x: u32 = 0xAAAAAAAA;
        let mut y: u32 = 0x55555555;

        for _ in 0..iters {
            x = x ^ y;
            y = (y << 1) | (y >> 31);
            x = (x & y) | (!x & !y);
        }

        samples.push(timer.elapsed_ns() as f64 / iters as f64);
    }

    let stats = Stats::new(samples.clone());
    BenchResult {
        name: "rust:bitwise".to_string(),
        mean_ns: stats.mean(),
        median_ns: stats.median(),
        p95_ns: stats.p95(),
        min_ns: stats.min(),
        max_ns: stats.max(),
        samples: samples.len(),
    }
}

/// Benchmark memory access patterns
pub fn bench_memory_access(iters: usize) -> BenchResult {
    let mut samples = Vec::new();
    let data: Vec<u32> = (0..1024).map(|i| (i * 37) as u32).collect();

    for _ in 0..10 {
        let timer = Timer::start();
        let mut sum: u64 = 0;

        for _ in 0..iters {
            for val in &data {
                sum = sum.wrapping_add(*val as u64);
            }
        }

        samples.push(timer.elapsed_ns() as f64 / (iters * 1024) as f64);
    }

    let stats = Stats::new(samples.clone());
    BenchResult {
        name: "rust:memory_seq".to_string(),
        mean_ns: stats.mean(),
        median_ns: stats.median(),
        p95_ns: stats.p95(),
        min_ns: stats.min(),
        max_ns: stats.max(),
        samples: samples.len(),
    }
}

/// Benchmark function call overhead
#[inline(never)]
fn leaf_fn(x: u32) -> u32 {
    x.wrapping_mul(2).wrapping_add(1)
}

pub fn bench_function_calls(iters: usize) -> BenchResult {
    let mut samples = Vec::new();

    for _ in 0..10 {
        let timer = Timer::start();
        let mut sum: u32 = 0;

        for i in 0..iters {
            sum = sum.wrapping_add(leaf_fn(i as u32));
        }

        samples.push(timer.elapsed_ns() as f64 / iters as f64);
    }

    let stats = Stats::new(samples.clone());
    BenchResult {
        name: "rust:function_call".to_string(),
        mean_ns: stats.mean(),
        median_ns: stats.median(),
        p95_ns: stats.p95(),
        min_ns: stats.min(),
        max_ns: stats.max(),
        samples: samples.len(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_benchmarks_run() {
        let result = bench_integer_arithmetic(10000);
        assert!(result.mean_ns > 0.0);
        assert_eq!(result.samples, 10);
    }
}
