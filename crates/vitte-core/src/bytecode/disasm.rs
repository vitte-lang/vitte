//! Minimal textual disassembly helpers used by the CLI tooling.

use crate::bytecode::chunk::{Chunk, ConstValue, Op};

#[cfg(not(feature = "std"))]
use alloc::{format, string::String};
#[cfg(feature = "std")]
use std::{format, string::String};

use core::fmt::Write;

/// Produce a multi-line, human readable disassembly with metadata.
pub fn disassemble_full(chunk: &Chunk, title: &str) -> String {
    let mut out = String::new();
    let _ = writeln!(
        out,
        "== {title} == (version={}, stripped={}, consts={}, ops={})",
        chunk.version(),
        chunk.flags().stripped,
        chunk.consts.len(),
        chunk.ops.len()
    );

    if let Some(main) = &chunk.debug.main_file {
        let _ = writeln!(out, ";; main_file: {main}");
    }
    if !chunk.debug.files.is_empty() {
        let _ = writeln!(out, ";; files: {:?}", chunk.debug.files);
    }
    if !chunk.debug.symbols.is_empty() {
        let _ = writeln!(out, ";; symbols: {:?}", chunk.debug.symbols);
    }

    if !chunk.consts.is_empty() {
        let _ = writeln!(out, "");
        let _ = writeln!(out, ";; constants");
        for (idx, value) in chunk.consts.iter() {
            let _ = writeln!(out, "const[{idx:04}] = {}", show_const(value));
        }
    }

    let _ = writeln!(out, "");
    let _ = writeln!(out, ";; ops");
    for (pc, op) in chunk.ops.iter().enumerate() {
        let line = chunk.lines.line_for_pc(pc as u32);
        let preview = match *op {
            Op::LoadConst(ix) => chunk.consts.get(ix).map(show_const),
            _ => None,
        };
        match preview {
            Some(p) => {
                let _ = writeln!(out, "{:04} | {:4} | {:?} ;; {p}", pc, line, op);
            }
            None => {
                let _ = writeln!(out, "{:04} | {:4} | {:?}", pc, line, op);
            }
        }
    }

    out
}

/// One-line-per-op variant used by quick CLI previews.
pub fn disassemble_compact(chunk: &Chunk) -> String {
    let mut out = String::new();
    for (pc, op) in chunk.ops.iter().enumerate() {
        let preview = match *op {
            Op::LoadConst(ix) => chunk.consts.get(ix).map(show_const),
            _ => None,
        };
        match preview {
            Some(p) => {
                let _ = writeln!(out, "{:04}: {:?} ;; {p}", pc, op);
            }
            None => {
                let _ = writeln!(out, "{:04}: {:?}", pc, op);
            }
        }
    }
    out
}

fn show_const(value: &ConstValue) -> String {
    match value {
        ConstValue::Null => "null".into(),
        ConstValue::Bool(b) => format!("{}", b),
        ConstValue::I64(i) => format!("{}", i),
        ConstValue::F64(f) => format!("{}", f),
        ConstValue::Str(s) => {
            if s.len() <= 64 {
                format!("\"{}\"", s)
            } else {
                format!("\"{}…\"", &s[..64])
            }
        }
        ConstValue::Bytes(bytes) => format!("bytes[{}]", bytes.len()),
    }
}
