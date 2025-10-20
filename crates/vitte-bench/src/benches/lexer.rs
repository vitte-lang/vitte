//! Lexer benchmarks.

use crate::utils;
use criterion::{black_box, BenchmarkId, Criterion, Throughput};
use vitte_core::SourceId;
use vitte_lexer::{Lexer, LexerOptions, TokenKind};

/// Registers lexer benchmarks with Criterion.
pub fn criterion_benchmark(c: &mut Criterion) {
    let mut group = c.benchmark_group("lexer/tokenize");
    let cases: Vec<(&str, String)> =
        vec![("1kb", utils::synthetic_source(1_024)), ("32kb", utils::synthetic_source(32_768))];

    for (label, source) in &cases {
        group.throughput(Throughput::Bytes(source.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(label), source, |b, input| {
            b.iter(|| {
                let mut lexer =
                    Lexer::with_options(input.as_str(), SourceId(0), LexerOptions::default());
                while let Some(token) = lexer.next().expect("lexing failed") {
                    let is_eof = matches!(token.value, TokenKind::Eof);
                    black_box(token.span.start.0);
                    if is_eof {
                        break;
                    }
                }
            });
        });
    }

    group.finish();
}
