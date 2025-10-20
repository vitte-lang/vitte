//! VM hotpath benchmarks assembled from real VITBC bytecode.

use criterion::{black_box, BenchmarkId, Criterion, Throughput};
use vitte_compiler::Op;
use vitte_vm::Vm;

fn emit_const(code: &mut Vec<u8>, value: i64) {
    code.push(Op::ConstI64 as u8);
    code.extend_from_slice(&value.to_le_bytes());
}

fn emit_add(code: &mut Vec<u8>) {
    code.push(Op::AddI64 as u8);
}

fn emit_ret(code: &mut Vec<u8>) {
    code.push(Op::Ret as u8);
}

fn wrap_vitbc(code: Vec<u8>) -> Vec<u8> {
    fn emit_section(out: &mut Vec<u8>, tag: &[u8; 4], payload: &[u8]) {
        out.extend_from_slice(tag);
        out.extend_from_slice(&(payload.len() as u32).to_le_bytes());
        out.extend_from_slice(payload);
    }

    let mut out = Vec::with_capacity(64 + code.len());
    out.extend_from_slice(b"VITBC\0");
    out.extend_from_slice(&2u16.to_le_bytes());
    emit_section(&mut out, b"INTS", &[]);
    emit_section(&mut out, b"FLTS", &[]);
    emit_section(&mut out, b"STRS", &[]);
    emit_section(&mut out, b"DATA", &[]);
    emit_section(&mut out, b"CODE", &code);
    emit_section(&mut out, b"NAME", &[]);
    out
}

fn hotpath_program(iterations: usize) -> Vec<u8> {
    let mut code = Vec::with_capacity(iterations * 1 + 16);
    for i in 0..iterations {
        emit_const(&mut code, i as i64);
        emit_const(&mut code, (i as i64) + 1);
        emit_add(&mut code);
    }
    emit_ret(&mut code);
    wrap_vitbc(code)
}

/// Registers VM hotpath benchmarks with Criterion.
pub fn criterion_benchmark(c: &mut Criterion) {
    let mut group = c.benchmark_group("vm/hotpath");
    let workloads: Vec<(usize, Vec<u8>)> =
        [64usize, 512, 4_096].into_iter().map(|iters| (iters, hotpath_program(iters))).collect();

    for (iters, bytecode) in workloads {
        group.throughput(Throughput::Bytes(bytecode.len() as u64));
        group.bench_with_input(BenchmarkId::from_parameter(iters), &bytecode, |b, code| {
            b.iter(|| {
                let mut vm = Vm::new();
                let exit_code = vm.run_bytecode(black_box(code)).expect("vm execution failed");
                black_box(exit_code);
            });
        });
    }

    group.finish();
}
