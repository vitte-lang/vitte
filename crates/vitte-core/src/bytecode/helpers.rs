//! Helper validations reused by tooling.

use crate::{bytecode::chunk::{Chunk, Op}, CoreError, CoreResult};

#[cfg(not(feature = "std"))]
use alloc::format;

/// Basic structural validation of a chunk.
///
/// This mirrors the historical helper that tooling relied upon. It keeps the
/// checks lightweight: indices must be in range and metadata stays coherent.
pub fn validate_chunk(chunk: &Chunk) -> CoreResult<()> {
    if chunk.ops.len() != chunk.lines.len() {
        return Err(CoreError::corrupted("line/op length mismatch"));
    }

    let const_count = chunk.consts.len() as u32;
    for (pc, op) in chunk.ops.iter().enumerate() {
        if let Op::LoadConst(ix) = *op {
            if ix >= const_count {
                return Err(CoreError::corrupted(format!(
                    "op {pc} references const {ix} but pool size is {const_count}"
                )));
            }
        }
    }

    for (name, pc) in &chunk.debug.symbols {
        if (*pc as usize) >= chunk.ops.len() {
            return Err(CoreError::corrupted(format!(
                "symbol `{name}` points to invalid pc {pc}"
            )));
        }
    }

    Ok(())
}
