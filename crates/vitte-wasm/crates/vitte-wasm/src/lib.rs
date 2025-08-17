//! vitte-wasm — Backend WebAssembly
//! Génère un module Wasm à partir d'un Chunk Vitte.

pub mod leb128;
pub mod module;
pub mod emitter;

pub fn compile_dummy() -> Vec<u8> {
    // binaire vide Wasm minimal
    let mut bytes = vec![0x00, 0x61, 0x73, 0x6D];
    bytes.extend_from_slice(&[0x01,0x00,0x00,0x00]);
    bytes
}
