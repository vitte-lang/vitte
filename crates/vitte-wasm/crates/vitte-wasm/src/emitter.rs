//! emitter.rs — Traduit du bytecode vers instructions Wasm
pub fn emit_const_i32(out: &mut Vec<u8>, val: i32) {
    out.push(0x41); // i32.const
    out.push(val as u8);
}
