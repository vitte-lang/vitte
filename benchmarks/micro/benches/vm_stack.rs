use criterion::{criterion_group, criterion_main, Criterion, black_box};

#[cfg(not(feature = "shims"))]
fn bench_vm_stack(c: &mut Criterion) {
    c.bench_function("vm_stack_sum_1e6", |b| {
        b.iter(|| {
            let n = black_box(1_000_000u64);
            let _ = vitte_vm::stack::sum_loop(n);
        });
    });
}

#[cfg(feature = "shims")]
mod shim {
    pub use super::*;
    use crate::shims;
    pub fn run(c: &mut Criterion) {
        c.bench_function("vm_stack_sum_1e6_shim", |b| b.iter(|| shims::vm_exec_sum(black_box(1_000_000)) ));
    }
}

#[cfg(feature = "shims")]
fn bench_vm_stack(c: &mut Criterion) { shim::run(c); }

criterion_group!(benches, bench_vm_stack);
criterion_main!(benches);
