//! Micro-benchmark for the `engine::vm::preview_source_head` fast-path
//! to keep track of improvements on large source files.

use criterion::{black_box, criterion_group, criterion_main, Criterion, Throughput};
use std::fs;
use std::io::{BufWriter, Write};
use std::sync::Arc;
use tempfile::NamedTempFile;
use vitte_bin::engine::vm;

fn prepare_fixture(target_bytes: usize) -> (Arc<tempfile::TempPath>, u64) {
    let mut tmp = NamedTempFile::new().expect("tmp file");
    {
        let mut writer = BufWriter::new(&mut tmp);
        writer.write_all(b"// fixture: first line kept small for preview\n").expect("write header");
        let chunk = vec![b'a'; 4096];
        let mut remaining = target_bytes;
        while remaining > 0 {
            let take = remaining.min(chunk.len());
            writer.write_all(&chunk[..take]).expect("write chunk");
            writer.write_all(b"\n").expect("write newline");
            remaining = remaining.saturating_sub(take + 1);
        }
        writer.flush().expect("flush");
    }
    let temp_path = tmp.into_temp_path();
    let size = fs::metadata(&temp_path).expect("metadata").len();
    (Arc::new(temp_path), size)
}

fn bench_preview_source_head(c: &mut Criterion) {
    let (temp_path, size) = prepare_fixture(4 * 1024 * 1024); // ~4 MiB payload
    let path_buf = temp_path.as_ref().to_path_buf();

    let mut group = c.benchmark_group("engine_vm_preview");
    group.throughput(Throughput::Bytes(size));
    {
        let preview_path = path_buf.clone();
        let keep = temp_path.clone();
        group.bench_function("preview_first_line", move |b| {
            let _keep_file_alive = keep.clone();
            b.iter(|| {
                let stats = vm::preview_source_head(&preview_path).expect("preview");
                black_box((stats.bytes_read, stats.file_size));
            });
        });
    }
    {
        let read_path = path_buf.clone();
        let keep = temp_path.clone();
        group.bench_function("read_full_file", move |b| {
            let _keep_file_alive = keep.clone();
            b.iter(|| {
                let data = fs::read_to_string(&read_path).expect("read_to_string");
                black_box(data.len());
            });
        });
    }
    group.finish();
}

criterion_group!(run_source, bench_preview_source_head);
criterion_main!(run_source);
