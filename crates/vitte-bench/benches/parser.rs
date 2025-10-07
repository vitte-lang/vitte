use criterion::{Criterion, criterion_group, criterion_main};
use vitte_bench::benches::parser;

fn run(c: &mut Criterion) {
    parser::criterion_benchmark(c);
}

criterion_group!(benches, run);
criterion_main!(benches);
