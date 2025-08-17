use criterion::{criterion_group, criterion_main, Criterion, black_box};

#[cfg(not(feature = "shims"))]
fn bench_parser(c: &mut Criterion) {
    let src = include_str!("../../macro/programs/json_parse.vitte");
    c.bench_function("parser", |b| {
        b.iter(|| {
            let _ = vitte_core::runtime::parser::parse(black_box(src));
        });
    });
}

#[cfg(feature = "shims")]
mod shim {
    pub use super::*;
    use crate::shims;
    pub fn run(c: &mut Criterion) {
        let src = include_str!("../../macro/programs/json_parse.vitte");
        c.bench_function("parser_shim", |b| b.iter(|| shims::parse(black_box(src)) ));
    }
}

#[cfg(feature = "shims")]
fn bench_parser(c: &mut Criterion) { shim::run(c); }

criterion_group!(benches, bench_parser);
criterion_main!(benches);
