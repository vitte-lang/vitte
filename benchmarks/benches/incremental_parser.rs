use criterion::{black_box, criterion_group, criterion_main, BatchSize, Criterion};
use vitte_syntax::{parse_module, IncrementalParser, TextEdit};

fn large_source(fn_count: usize) -> String {
    let mut src = String::with_capacity(fn_count * 48);
    for i in 0..fn_count {
        // simple function with predictable formatting for easy offsets
        src.push_str(&format!("fn f{i:04}() -> int {{ return {i}; }}\n"));
    }
    src
}

fn bench_incremental_parser(c: &mut Criterion) {
    const FN_COUNT: usize = 2000;
    let source = large_source(FN_COUNT);
    let base_parser = IncrementalParser::new(source.clone());

    // edit inside the middle function literal
    let target_literal = "return 1000;";
    let literal_start = source.find(target_literal).expect("literal location") + "return ".len();
    let literal_end = literal_start + "1000".len();
    let literal_edit = TextEdit { range: literal_start..literal_end, replacement: "424242".into() };

    // insert a new function before fn f1500
    let insert_anchor = "fn f1500";
    let insert_pos = source.find(insert_anchor).expect("anchor");
    let new_fn = "fn f1499_new() -> int { return 1499; }\n";
    let insert_edit = TextEdit { range: insert_pos..insert_pos, replacement: new_fn.to_string() };

    // delete an existing function
    let delete_snippet = "fn f1501() -> int { return 1501; }\n";
    let delete_start = source.find(delete_snippet).expect("delete snippet start");
    let delete_end = delete_start + delete_snippet.len();
    let delete_edit = TextEdit { range: delete_start..delete_end, replacement: String::new() };

    let mut group = c.benchmark_group("syntax_incremental_large");

    group.bench_function("edit_middle_literal", |b| {
        b.iter_batched(
            || base_parser.clone(),
            |mut parser| {
                let delta = parser.apply_edit(literal_edit.clone());
                black_box(delta.changed_functions.len());
            },
            BatchSize::SmallInput,
        );
    });

    group.bench_function("insert_function", |b| {
        b.iter_batched(
            || base_parser.clone(),
            |mut parser| {
                let delta = parser.apply_edit(insert_edit.clone());
                black_box(delta.inserted_functions.len());
            },
            BatchSize::SmallInput,
        );
    });

    group.bench_function("delete_function", |b| {
        b.iter_batched(
            || base_parser.clone(),
            |mut parser| {
                let delta = parser.apply_edit(delete_edit.clone());
                black_box(delta.removed_functions.len());
            },
            BatchSize::SmallInput,
        );
    });

    group.bench_function("full_reparse", |b| {
        b.iter(|| {
            let module = parse_module(&source);
            black_box(module.items.len())
        });
    });

    group.finish();
}

criterion_group!(incremental, bench_incremental_parser);
criterion_main!(incremental);
