//! vitte-codegen-wasm64 — WebAssembly 64-bit backend for Vitte
//!
//! Goals
//! - Deterministic wasm64 codegen for IR lowering.
//! - Support wasm MVP + optional: simd128, threads, ref types, bulk memory.
//! - Separation: Lowering → Encode → Section builder → Emit → Module.
//!
//! Status
//! - Implements core wasm64 integer/fp ops, control flow, memory ops.
//! - Encodes into raw wasm binary sections (not textual `.wat`).
//! - Pseudo lowering for Vitte IR (demo only).
//!
//! Copyright
//! - MIT licensed (workspace).

#![cfg_attr(not(feature = "std"), no_std)]
#![deny(missing_docs)]
#![forbid(unsafe_op_in_unsafe_fn)]

#[cfg(not(feature = "std"))]
extern crate alloc;

#[cfg(not(feature = "std"))]
use alloc::{string::String, vec, vec::Vec};
#[cfg(feature = "std")]
use std::vec::Vec;
use std::{string::String, vec};

/// Result alias for backend.
pub type Result<T, E = CodegenError> = core::result::Result<T, E>;

/// Backend error.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CodegenError {
    /// Invalid input.
    Invalid(String),
    /// Unsupported feature/opcode.
    Unsupported(String),
    /// Buffer error.
    Buf(String),
    /// Relocation or linking error.
    Reloc(String),
    /// Lowering failure.
    Lower(String),
}

/// WebAssembly value types (64-bit aware).
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum ValType {
    /// i32
    I32,
    /// i64
    I64,
    /// f32
    F32,
    /// f64
    F64,
    /// reference (if ref_types)
    Ref,
}

/// Numeric opcodes (subset).
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum NumOp {
    /// i32.add
    I32Add,
    /// i64.add
    I64Add,
    /// f32.add
    F32Add,
    /// f64.add
    F64Add,
    /// i64.const
    I64Const,
    /// i32.const
    I32Const,
    /// f64.const
    F64Const,
}

/// Memory opcodes.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum MemOp {
    /// load i64
    I64Load,
    /// store i64
    I64Store,
}

/// Control flow ops.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CtrlOp {
    /// return
    Return,
    /// call index
    Call(u32),
    /// end
    End,
}

/// High-level op.
#[derive(Debug, Clone, PartialEq)]
pub enum Op {
    /// Const numeric
    NumConstI64(i64),
    /// Const i32
    NumConstI32(i32),
    /// Const f64
    NumConstF64(f64),
    /// Binary arithmetic
    NumBin(NumOp),
    /// Memory load/store
    Mem(MemOp, u32), // offset
    /// Control
    Ctrl(CtrlOp),
}

/// Simple code buffer.
#[derive(Debug, Default)]
pub struct CodeBuf {
    bytes: Vec<u8>,
}

impl CodeBuf {
    /// Create.
    pub fn new() -> Self {
        Self { bytes: Vec::new() }
    }
    /// Write raw byte.
    pub fn put_u8(&mut self, b: u8) {
        self.bytes.push(b);
    }
    /// Write LE32.
    pub fn put_u32(&mut self, v: u32) {
        self.bytes.extend_from_slice(&v.to_le_bytes());
    }
    /// Write LEB128 signed.
    pub fn put_leb128_i64(&mut self, mut val: i64) {
        let mut more = true;
        while more {
            let byte = (val as u8) & 0x7f;
            val >>= 7;
            let done = (val == 0 && (byte & 0x40) == 0) || (val == -1 && (byte & 0x40) != 0);
            let out = if done { byte } else { byte | 0x80 };
            self.bytes.push(out);
            more = !done;
        }
    }
    /// Write LEB128 unsigned.
    pub fn put_leb128_u32(&mut self, mut val: u32) {
        loop {
            let mut byte = (val & 0x7f) as u8;
            val >>= 7;
            if val != 0 {
                byte |= 0x80;
            }
            self.bytes.push(byte);
            if val == 0 {
                break;
            }
        }
    }
    /// Access bytes.
    pub fn bytes(&self) -> &[u8] {
        &self.bytes
    }
}

/// Encoder.
pub mod enc {
    use super::{CodeBuf, CtrlOp, MemOp, NumOp, Op};

    /// Encode one op.
    pub fn encode(op: &Op, buf: &mut CodeBuf) -> super::Result<()> {
        match *op {
            Op::NumConstI64(v) => {
                buf.put_u8(0x42); // i64.const
                buf.put_leb128_i64(v);
            }
            Op::NumConstI32(v) => {
                buf.put_u8(0x41);
                buf.put_leb128_i64(v as i64);
            }
            Op::NumConstF64(f) => {
                buf.put_u8(0x44);
                buf.bytes.extend_from_slice(&f.to_le_bytes());
            }
            Op::NumBin(opc) => {
                let code = match opc {
                    NumOp::I32Add => 0x6a,
                    NumOp::I64Add => 0x7c,
                    NumOp::F32Add => 0x92,
                    NumOp::F64Add => 0xa0,
                    _ => return Err(super::CodegenError::Unsupported("binop".into())),
                };
                buf.put_u8(code);
            }
            Op::Mem(mem, offset) => {
                match mem {
                    MemOp::I64Load => buf.put_u8(0x29),
                    MemOp::I64Store => buf.put_u8(0x37),
                }
                // align=3 for i64, reserved, then offset
                buf.put_leb128_u32(3);
                buf.put_leb128_u32(offset);
            }
            Op::Ctrl(c) => match c {
                CtrlOp::Return => buf.put_u8(0x0f),
                CtrlOp::End => buf.put_u8(0x0b),
                CtrlOp::Call(idx) => {
                    buf.put_u8(0x10);
                    buf.put_leb128_u32(idx);
                }
            },
        }
        Ok(())
    }
}

/// Lowering from minimal IR.
pub mod lower {
    use super::{CtrlOp, MemOp, NumOp, Op};
    #[cfg(not(feature = "std"))]
    use alloc::{vec, vec::Vec};
    #[cfg(feature = "std")]
    use std::vec::Vec;

    /// Minimal IR subset for wasm.
    #[derive(Debug, Clone, PartialEq)]
    pub enum Ir {
        /// Push an i64 constant onto the stack.
        ConstI64(i64),
        /// Pop two i64s and push their sum.
        AddI64,
        /// Store i64 at memory[base+offset] (base is on stack).
        StoreI64 {
            /// Byte offset from the memory base.
            offset: u32,
        },
        /// Load i64 from memory[base+offset] (base is on stack).
        LoadI64 {
            /// Byte offset from the memory base.
            offset: u32,
        },
        /// Return from the current function.
        Ret,
    }

    /// Lower IR → wasm ops.
    pub fn lower(ir: &[Ir]) -> super::Result<Vec<Op>> {
        let mut out = Vec::new();
        for n in ir {
            match n {
                Ir::ConstI64(v) => out.push(Op::NumConstI64(*v)),
                Ir::AddI64 => out.push(Op::NumBin(NumOp::I64Add)),
                Ir::StoreI64 { offset } => out.push(Op::Mem(MemOp::I64Store, *offset)),
                Ir::LoadI64 { offset } => out.push(Op::Mem(MemOp::I64Load, *offset)),
                Ir::Ret => {
                    out.push(Op::Ctrl(CtrlOp::Return));
                    out.push(Op::Ctrl(CtrlOp::End));
                }
            }
        }
        Ok(out)
    }
}

/// Module builder (very minimal).
pub mod module {
    use super::{enc, CodeBuf, Op};
    #[cfg(not(feature = "std"))]
    use alloc::{vec, vec::Vec};
    #[cfg(feature = "std")]
    use std::vec::Vec;

    /// A function body.
    #[derive(Debug)]
    pub struct Func {
        /// Instruction stream for this function.
        pub ops: Vec<Op>,
    }

    /// A module.
    #[derive(Debug, Default)]
    pub struct Module {
        /// Functions contained in this module in declaration order.
        pub funcs: Vec<Func>,
    }

    impl Module {
        /// Encode module to raw wasm bytes.
        pub fn encode(&self) -> super::Result<Vec<u8>> {
            let mut buf = CodeBuf::new();
            // magic + version
            buf.bytes.extend_from_slice(&[0x00, 0x61, 0x73, 0x6d]); // \0asm
            buf.bytes.extend_from_slice(&[0x01, 0x00, 0x00, 0x00]); // version 1

            // Type section (func type [] -> [])
            buf.put_u8(0x01);
            let mut tmp = CodeBuf::new();
            tmp.put_leb128_u32(1); // one type
            tmp.put_u8(0x60); // func
            tmp.put_leb128_u32(0); // no params
            tmp.put_leb128_u32(0); // no results
            buf.put_leb128_u32(tmp.bytes().len() as u32);
            buf.bytes.extend_from_slice(tmp.bytes());

            // Function section
            buf.put_u8(0x03);
            let mut tmp = CodeBuf::new();
            tmp.put_leb128_u32(self.funcs.len() as u32);
            for _ in &self.funcs {
                tmp.put_leb128_u32(0); // type idx
            }
            buf.put_leb128_u32(tmp.bytes().len() as u32);
            buf.bytes.extend_from_slice(tmp.bytes());

            // Code section
            buf.put_u8(0x0a);
            let mut tmp = CodeBuf::new();
            tmp.put_leb128_u32(self.funcs.len() as u32);
            for f in &self.funcs {
                let mut body = CodeBuf::new();
                body.put_leb128_u32(0); // locals
                for op in &f.ops {
                    enc::encode(op, &mut body)?;
                }
                let body_bytes = body.bytes();
                tmp.put_leb128_u32(body_bytes.len() as u32);
                tmp.bytes.extend_from_slice(body_bytes);
            }
            buf.put_leb128_u32(tmp.bytes().len() as u32);
            buf.bytes.extend_from_slice(tmp.bytes());

            Ok(buf.bytes)
        }
    }
}

/// Codegen facade.
pub struct Codegen;

impl Codegen {
    /// Compile IR → wasm64 module bytes.
    pub fn compile(ir: &[lower::Ir]) -> Result<Vec<u8>> {
        let ops = lower::lower(ir)?;
        let module = module::Module { funcs: vec![module::Func { ops }] };
        module.encode()
    }
}

#[cfg(test)]
mod tests {
    use super::lower::Ir;
    use super::Codegen;

    #[test]
    fn compile_simple() {
        // const 42; ret
        let ir = vec![Ir::ConstI64(42), Ir::Ret];
        let wasm = Codegen::compile(&ir).unwrap();
        assert!(wasm.starts_with(&[0x00, 0x61, 0x73, 0x6d])); // magic
    }
}
