//! vitte-web — VM compilée en Wasm
use wasm_bindgen::prelude::*;
#[wasm_bindgen]
pub fn run_vitte_source(src: &str) -> String {
    format!("Executed in Wasm: {}", src)
}
