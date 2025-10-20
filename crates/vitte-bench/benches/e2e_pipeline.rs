use criterion::{criterion_group, criterion_main, Criterion};
use vitte_bench::benches::e2e_pipeline;

fn run(c: &mut Criterion) {
    e2e_pipeline::criterion_benchmark(c);
}

criterion_group!(benches, run);
criterion_main!(benches);
