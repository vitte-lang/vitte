//! Parser benchmarks.

use crate::utils;
use criterion::{black_box, BenchmarkId, Criterion, Throughput};
use vitte_core::SourceId;
use vitte_parser::Parser;

const PROGRAM_SNIPPET: &str = "fn foo() -> i64 { return 42; }\n";

/// Registers parser benchmarks with Criterion.
pub fn criterion_benchmark(c: &mut Criterion) {
    let mut group = c.benchmark_group("parser/parse_program");
    let inputs: Vec<(&str, String)> = vec![
        ("small", utils::repeat_input(PROGRAM_SNIPPET, 8)),
        ("large", utils::repeat_input(PROGRAM_SNIPPET, 128)),
    ];

    for (label, source) in &inputs {
        group.throughput(Throughput::Bytes(source.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(label), source, |b, input| {
            b.iter(|| {
                let mut parser = Parser::new(input.as_str(), SourceId(0));
                let program = parser.parse_program().expect("parser failure");
                black_box(program.items.len());
            });
        });
    }

    group.finish();
}
