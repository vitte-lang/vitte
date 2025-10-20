//! VM micro benchmarks focused on synthetic opcode workloads.

use criterion::{black_box, BenchmarkId, Criterion};
use vitte_compiler::Op;
use vitte_vm::Vm;

fn emit_const(code: &mut Vec<u8>, value: i64) {
    code.push(Op::ConstI64 as u8);
    code.extend_from_slice(&value.to_le_bytes());
}

fn emit_add(code: &mut Vec<u8>) {
    code.push(Op::AddI64 as u8);
}

fn emit_call(code: &mut Vec<u8>, target: u32) {
    code.push(Op::Call as u8);
    code.extend_from_slice(&target.to_le_bytes());
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

fn stack_program(iterations: usize) -> Vec<u8> {
    let mut code = Vec::with_capacity(iterations * 10);
    for i in 0..iterations {
        emit_const(&mut code, i as i64);
    }
    for _ in 1..iterations {
        emit_add(&mut code);
    }
    emit_ret(&mut code);
    wrap_vitbc(code)
}

fn arithmetic_program(iterations: usize) -> Vec<u8> {
    let mut code = Vec::with_capacity(iterations * 20);
    for i in 0..iterations {
        emit_const(&mut code, (i as i64) + 1);
        emit_const(&mut code, 2);
        emit_add(&mut code);
    }
    emit_ret(&mut code);
    wrap_vitbc(code)
}

fn call_program(iterations: usize) -> Vec<u8> {
    let mut code = Vec::with_capacity(iterations * 12);
    for i in 0..iterations {
        emit_const(&mut code, (i % 3) as i64);
        emit_call(&mut code, 0);
    }
    emit_ret(&mut code);
    wrap_vitbc(code)
}

/// Registers VM micro benchmarks with Criterion.
pub fn criterion_benchmark(c: &mut Criterion) {
    let cases = [
        ("stack-512", stack_program(512)),
        ("stack-2048", stack_program(2_048)),
        ("arith-512", arithmetic_program(512)),
        ("arith-2048", arithmetic_program(2_048)),
        ("call-512", call_program(512)),
        ("call-2048", call_program(2_048)),
    ];

    let mut group = c.benchmark_group("vm_micro");
    for (label, bytecode) in cases {
        group.bench_with_input(BenchmarkId::from_parameter(label), &bytecode, |b, code| {
            b.iter(|| {
                let mut vm = Vm::new();
                let exit_code = vm.run_bytecode(black_box(code)).expect("vm execution failed");
                black_box(exit_code);
            });
        });
    }
    group.finish();
}
