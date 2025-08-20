// crates/vitte-core/src/bytecode/mod.rs

pub mod chunk;
pub mod op;
pub mod format;

// Reexports publics (garde uniquement ceux de chunk)
pub use chunk::{
    CHUNK_MAGIC, CHUNK_VERSION,
    Chunk, ChunkFlags, ChunkLoadError,
    ConstPool, ConstValue, LineTable,
};

