use criterion::{Criterion, criterion_group, criterion_main};
use vitte_bench::benches::lexer;

fn run(c: &mut Criterion) {
    lexer::criterion_benchmark(c);
}

criterion_group!(benches, run);
criterion_main!(benches);
