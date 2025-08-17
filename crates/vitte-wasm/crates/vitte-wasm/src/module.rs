//! module.rs — helpers pour construire un module Wasm

pub struct WasmModule { pub bytes: Vec<u8> }

impl WasmModule {
    pub fn new() -> Self {
        let mut m = Self { bytes: vec![] };
        m.bytes.extend_from_slice(&[0x00,0x61,0x73,0x6D]);
        m.bytes.extend_from_slice(&[0x01,0x00,0x00,0x00]);
        m
    }
    pub fn push_section(&mut self, id: u8, payload: Vec<u8>) {
        self.bytes.push(id);
        self.bytes.push(payload.len() as u8);
        self.bytes.extend(payload);
    }
}
