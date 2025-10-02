use criterion::{criterion_group, criterion_main, Criterion};
use vitte_bench::benches::vm_micro;

fn run(c: &mut Criterion) {
    vm_micro::criterion_benchmark(c);
}

criterion_group!(benches, run);
criterion_main!(benches);
