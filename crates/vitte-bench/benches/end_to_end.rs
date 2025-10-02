use criterion::{criterion_group, criterion_main, Criterion};
use vitte_bench::benches::end_to_end;

fn run(c: &mut Criterion) {
    end_to_end::criterion_benchmark(c);
}

criterion_group!(benches, run);
criterion_main!(benches);
