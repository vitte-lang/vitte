//! Benchmarks the compiler's front/middle passes (symbol collection + typing).

use crate::utils;
use criterion::{black_box, BenchmarkId, Criterion, Throughput};
use vitte_compiler::{Compiler, CompilerOptions};
use vitte_core::SourceId;
use vitte_parser::Parser;

/// Registers IR pass benchmarks with Criterion.
pub fn criterion_benchmark(c: &mut Criterion) {
    let snippet = r#"
fn add(a: i64, b: i64) -> i64 {
    return a + b;
}

fn main() -> i64 {
    let result = add(4, 5);
    return result;
}
"#;

    let sources = vec![
        ("ir-small", snippet.to_owned()),
        ("ir-medium", utils::repeat_input(snippet, 8)),
        ("ir-large", utils::repeat_input(snippet, 32)),
    ];

    let programs: Vec<_> = sources
        .into_iter()
        .map(|(label, src)| {
            let mut parser = Parser::new(&src, SourceId(0));
            let program = parser.parse_program().expect("parser failure");
            (label, src.len(), program)
        })
        .collect();

    let mut group = c.benchmark_group("compiler/ir_passes");
    for (label, src_len, program) in &programs {
        group.throughput(Throughput::Bytes(*src_len as u64));
        group.bench_with_input(BenchmarkId::from_parameter(label), program, |b, prog| {
            b.iter(|| {
                let mut compiler = Compiler::new(CompilerOptions::default());
                let bytecode = compiler.compile(prog).expect("compile failure");
                let bytes = bytecode.to_bytes(false);
                black_box(bytes.len());
            });
        });
    }
    group.finish();
}
