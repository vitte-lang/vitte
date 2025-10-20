use criterion::{criterion_group, criterion_main, Criterion};
use vitte_bench::benches::ir_passes;

fn run(c: &mut Criterion) {
    ir_passes::criterion_benchmark(c);
}

criterion_group!(benches, run);
criterion_main!(benches);
