//! End-to-end benchmarks for a minimal Vitte pipeline.

use criterion::{black_box, BenchmarkId, Criterion, Throughput};
use vitte_compiler::{Compiler, CompilerOptions};
use vitte_core::SourceId;
use vitte_lexer::{Lexer, LexerOptions};
use vitte_parser::Parser;
use vitte_vm::Vm;

fn build_pipeline_source(extra_helpers: usize) -> String {
    use std::fmt::Write;

    let mut src = String::from("fn add(x: i64, y: i64) -> i64 { return x + y; }\n");
    for idx in 0..extra_helpers {
        let _ = writeln!(src, "fn helper_{idx}(value: i64) -> i64 {{ return value + {idx}; }}");
    }

    src.push_str("fn main() -> i64 {\n    let step0 = add(10, 32);\n");
    for idx in 0..extra_helpers {
        let prev = format!("step{idx}");
        let next = format!("step{}", idx + 1);
        let _ = writeln!(src, "    let {next} = helper_{idx}({prev});");
    }
    let final_name = format!("step{}", extra_helpers);
    src.push_str(&format!("    return {final_name};\n}}\n"));
    src
}

/// Registers end-to-end Vitte toolchain benchmarks.
pub fn criterion_benchmark(c: &mut Criterion) {
    let mut group = c.benchmark_group("pipeline/end_to_end");
    let programs: Vec<(usize, String)> =
        vec![(0, build_pipeline_source(0)), (4, build_pipeline_source(4))];

    for (helpers, source) in &programs {
        group.throughput(Throughput::Bytes(source.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(helpers), source, |b, input| {
            b.iter(|| {
                let tokens =
                    Lexer::with_options(input.as_str(), SourceId(0), LexerOptions::default())
                        .tokenize()
                        .expect("lexing failed");
                black_box(tokens.len());

                let mut parser = Parser::new(input.as_str(), SourceId(0));
                let program = parser.parse_program().expect("parse failure");

                let mut compiler = Compiler::new(CompilerOptions::default());
                let bytecode = compiler.compile(&program).expect("compile failure");
                let bytes = bytecode.to_bytes(false);

                let mut vm = Vm::new();
                let exit_code =
                    vm.run_bytecode(black_box(bytes.as_slice())).expect("vm execution failed");
                black_box(exit_code);
            });
        });
    }

    group.finish();
}
