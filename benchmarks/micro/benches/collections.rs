use criterion::{criterion_group, criterion_main, Criterion, black_box};

#[cfg(not(feature = "shims"))]
fn bench_collections(c: &mut Criterion) {
    c.bench_function("hashmap_fill_100k", |b| {
        b.iter(|| vitte_core::util::bench::hashmap_fill(black_box(100_000usize)) );
    });
}

#[cfg(feature = "shims")]
mod shim {
    pub use super::*;
    use crate::shims;
    pub fn run(c: &mut Criterion) {
        c.bench_function("hashmap_fill_100k_shim", |b| b.iter(|| shims::map_fill(black_box(100_000usize)) ));
    }
}

#[cfg(feature = "shims")]
fn bench_collections(c: &mut Criterion) { shim::run(c); }

criterion_group!(benches, bench_collections);
criterion_main!(benches);
