//! Lightweight runtime helpers to keep the historical API surface available.

/// Minimal evaluator used by CLI helpers to support `--engine eval`.
pub mod eval {
    use crate::{
        bytecode::chunk::{Chunk, ConstValue, Op},
        CoreError, CoreResult,
    };

    #[cfg(not(feature = "std"))]
    use alloc::{format, string::String, vec::Vec};
    #[cfg(feature = "std")]
    use std::{format, string::String, vec::Vec};

    /// Options accepted by the legacy evaluator (currently only a stub).
    #[derive(Debug, Clone, Default)]
    pub struct EvalOptions {
        /// When true (default) the printed output is accumulated in the result.
        pub capture_stdout: bool,
    }

    /// Result of evaluating a chunk.
    #[derive(Debug, Clone, Default)]
    pub struct EvalOutput {
        /// Text captured from `Print` instructions.
        pub stdout: String,
    }

    /// Execute a chunk in a tiny stack-based interpreter.
    pub fn eval_chunk(chunk: &Chunk, _opts: EvalOptions) -> CoreResult<EvalOutput> {
        let mut stack: Vec<ConstValue> = Vec::new();
        let mut out = EvalOutput::default();

        for op in &chunk.ops {
            match *op {
                Op::LoadConst(ix) => {
                    let value = chunk
                        .consts
                        .get(ix)
                        .ok_or_else(|| CoreError::corrupted(format!("const {ix} inexistant")))?;
                    stack.push(value.clone());
                }
                Op::Print => {
                    let value = stack.pop().ok_or_else(|| CoreError::corrupted("stack underflow"))?;
                    if !out.stdout.is_empty() {
                        out.stdout.push_str("\n");
                    }
                    out.stdout.push_str(&format_const(&value));
                }
                Op::Return => break,
            }
        }

        Ok(out)
    }

    fn format_const(value: &ConstValue) -> String {
        match value {
            ConstValue::Null => "null".into(),
            ConstValue::Bool(b) => format!("{}", b),
            ConstValue::I64(i) => format!("{}", i),
            ConstValue::F64(f) => format!("{}", f),
            ConstValue::Str(s) => s.clone(),
            ConstValue::Bytes(bytes) => format!("<bytes:{}>", bytes.len()),
        }
    }
}
