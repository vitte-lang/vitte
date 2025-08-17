use criterion::{criterion_group, criterion_main, Criterion, black_box};

#[cfg(not(feature = "shims"))]
fn bench_channels(c: &mut Criterion) {
    c.bench_function("channel_pingpong_100k", |b| {
        b.iter(|| vitte_core::concurrent::bench::pingpong(black_box(100_000usize)) );
    });
}

#[cfg(feature = "shims")]
mod shim {
    pub use super::*;
    use crate::shims;
    pub fn run(c: &mut Criterion) {
        c.bench_function("channel_pingpong_100k_shim", |b| b.iter(|| shims::channel_pingpong(black_box(100_000usize)) ));
    }
}

#[cfg(feature = "shims")]
fn bench_channels(c: &mut Criterion) { shim::run(c); }

criterion_group!(benches, bench_channels);
criterion_main!(benches);
