use criterion::{criterion_group, criterion_main, Criterion};
use vitte_bench::benches::vm_hotpath;

fn run(c: &mut Criterion) {
    vm_hotpath::criterion_benchmark(c);
}

criterion_group!(benches, run);
criterion_main!(benches);
