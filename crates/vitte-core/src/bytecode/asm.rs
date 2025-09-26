//! Extremely small assembly helper used to keep historical tooling compiling.
//!
//! The supported syntax is intentionally tiny and only meant for smoke tests:
//!
//! ```text
//! LOAD_CONST "hello"
//! PRINT
//! RETURN
//! ```
//!
//! Additional helpers:
//! - `CONST "foo"` adds a constant without emitting an opcode.
//! - `LOAD <idx>` loads an explicit constant index.
//! - Lines beginning with `;` are ignored as comments.

use crate::{
    bytecode::chunk::{Chunk, ChunkFlags, ConstValue, Op},
    CoreError, CoreResult,
};

#[cfg(not(feature = "std"))]
use alloc::{format, string::String};
#[cfg(feature = "std")]
use std::{format, string::String};

/// Assemble a source snippet into a [`Chunk`].
///
/// This is **not** a full assembler – it merely preserves the historical API so
/// the CLI tools keep working during the refactor. Unsupported instructions are
/// reported as an error.
pub fn assemble(source: &str) -> CoreResult<Chunk> {
    let mut chunk = Chunk::new(ChunkFlags { stripped: false });

    for (idx, raw_line) in source.lines().enumerate() {
        let line_no = (idx + 1) as u32;
        let line = raw_line.trim();
        if line.is_empty() || line.starts_with(';') {
            continue;
        }

        if let Some(rest) = line.strip_prefix("CONST ") {
            let literal = rest.trim();
            let value = parse_string_literal(literal)?;
            chunk.add_const(ConstValue::Str(value));
            continue;
        }

        if let Some(rest) = line.strip_prefix("LOAD_CONST ") {
            let literal = rest.trim();
            let value = parse_string_literal(literal)?;
            let idx = chunk.add_const(ConstValue::Str(value));
            chunk.push_op(Op::LoadConst(idx), line_no);
            continue;
        }

        if let Some(rest) = line.strip_prefix("LOAD ") {
            let ix = rest.trim().parse::<u32>()
                .map_err(|_| CoreError::corrupted(format!("LOAD attend un index: `{rest}`")))?;
            chunk.push_op(Op::LoadConst(ix), line_no);
            continue;
        }

        match line.to_ascii_uppercase().as_str() {
            "PRINT" => chunk.push_op(Op::Print, line_no),
            "RETURN" => chunk.push_op(Op::Return, line_no),
            other => {
                return Err(CoreError::corrupted(format!(
                    "instruction non supportée: `{other}`"
                )));
            }
        }
    }

    Ok(chunk)
}

fn parse_string_literal(input: &str) -> CoreResult<String> {
    let bytes = input.as_bytes();
    if bytes.len() < 2 || bytes[0] != b'"' || bytes[bytes.len() - 1] != b'"' {
        return Err(CoreError::corrupted("littéral string attendu"));
    }

    let mut out = String::new();
    let mut escaping = false;
    for &b in &bytes[1..bytes.len() - 1] {
        if escaping {
            let c = match b {
                b'\"' => '"',
                b'\\' => '\\',
                b'n' => '\n',
                b't' => '\t',
                other => other as char,
            };
            out.push(c);
            escaping = false;
            continue;
        }
        if b == b'\\' {
            escaping = true;
        } else {
            out.push(b as char);
        }
    }

    if escaping {
        return Err(CoreError::corrupted("séquence d'échappement incomplète"));
    }

    Ok(out)
}
