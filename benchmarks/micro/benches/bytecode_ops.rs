use criterion::{criterion_group, criterion_main, Criterion, black_box};

#[cfg(not(feature = "shims"))]
fn bench_bc_ops(c: &mut Criterion) {
    c.bench_function("bytecode_add_loop", |b| {
        b.iter(|| vitte_core::bytecode::ops::bench_add_loop(black_box(1_000_000u32)) );
    });
}

#[cfg(feature = "shims")]
mod shim {
    pub use super::*;
    use crate::shims;
    pub fn run(c: &mut Criterion) {
        c.bench_function("bytecode_add_loop_shim", |b| b.iter(|| shims::bytecode_add_loop(black_box(1_000_000)) ));
    }
}

#[cfg(feature = "shims")]
fn bench_bc_ops(c: &mut Criterion) { shim::run(c); }

criterion_group!(benches, bench_bc_ops);
criterion_main!(benches);
