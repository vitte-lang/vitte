//! vitte-core::prelude — exports pratiques du cœur Vitte.
//!
//! Usage :
//!     use vitte_core::prelude::*;
//!
//! Vous obtenez les types clés (Chunk, Op…), les helpers “batteries incluses”,
//! ainsi que `Result`/`Error`.

#![allow(unused_imports)]

/* ───────────────────────── Reexports cœur ───────────────────────── */

pub use crate::{
    // Accès aux sous-modules si besoin : bytecode::chunk::*, bytecode::op::*
    bytecode::{self, chunk, op},

    // Types principaux du format bytecode
    bytecode::chunk::{Chunk, ChunkFlags, ConstPool, ConstValue, LineTable},
    bytecode::op::Op,

    // Helpers haut-niveau (new_chunk, k_*, validate_chunk, BytecodeIo, …)
    helpers::*,

    // Ergonomie d’erreur
    Error, Result,
};

/* ───────────────────────── Version helpers ───────────────────────── */

#[cfg(feature = "std")]
pub use crate::version; // fn version() -> String

#[cfg(all(not(feature = "std"), feature = "alloc-only"))]
pub use crate::VERSION_NO_STD as version_no_std; // &str constant

/* ─────────────── Confort no_std + alloc (types usuels) ─────────────── */

#[cfg(all(not(feature = "std"), feature = "alloc-only"))]
pub use alloc::{
    borrow::ToOwned,
    boxed::Box,
    format,
    rc::Rc,
    string::String,
    sync::Arc,
    vec::Vec,
    collections::{BTreeMap, BTreeSet, BinaryHeap, VecDeque, LinkedList},
};
