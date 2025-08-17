use criterion::{criterion_group, criterion_main, Criterion, black_box};

#[cfg(not(feature = "shims"))]
fn bench_tokenizer(c: &mut Criterion) {
    let src = include_str!("../../macro/programs/fib.vitte");
    c.bench_function("tokenizer", |b| {
        b.iter(|| {
            let _ = vitte_core::runtime::tokenizer::Tokenizer::new(black_box(src));
        });
    });
}

#[cfg(feature = "shims")]
mod shim {
    pub use super::*;
    use crate::shims;
    pub fn run(c: &mut Criterion) {
        let src = include_str!("../../macro/programs/fib.vitte");
        c.bench_function("tokenizer_shim", |b| b.iter(|| shims::tokenize(black_box(src)) ));
    }
}

#[cfg(feature = "shims")]
fn bench_tokenizer(c: &mut Criterion) { shim::run(c); }

criterion_group!(benches, bench_tokenizer);
criterion_main!(benches);
