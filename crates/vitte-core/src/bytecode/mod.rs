//! Bytecode helpers (chunk structure, validation, asm/disasm stubs).
//!
//! The original tooling expected a fairly rich API. To keep the workspace
//! compiling while the new core is rolled out, we provide a lightweight
//! implementation that covers the pieces currently used by the CLI crates.
 
/// Chunk representation plus binary roundtrip helpers.
pub mod chunk;
pub mod helpers;
pub mod disasm;
pub mod asm;
pub mod runtime;

pub use chunk::{Chunk, ChunkError, ChunkFlags, ConstPool, ConstValue, DebugInfo, LineTable, Op};
