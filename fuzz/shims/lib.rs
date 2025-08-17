//! Shims — implémente ici les adaptateurs vers tes vraies APIs si besoin.
//! Active la feature `shims` pour utiliser ces chemins.

#[cfg(feature = "shims")]
pub fn fuzz_tokenizer(_src: &str) { /* TODO: appeler vitte_core::runtime::tokenizer::tokenize */ }

#[cfg(feature = "shims")]
pub fn fuzz_parser(_src: &str) { /* TODO: appeler vitte_core::runtime::parser::parse */ }

#[cfg(feature = "shims")]
pub fn fuzz_pretty(_src: &str) { /* TODO: parse + pretty::format */ }

#[cfg(feature = "shims")]
pub fn fuzz_bytecode_loader(_bytes: &[u8]) { /* TODO: vitte_core::loader::load */ }

#[cfg(feature = "shims")]
pub fn fuzz_disasm(_bytes: &[u8]) { /* TODO: vitte_core::bytecode::disasm */ }

#[cfg(feature = "shims")]
pub fn fuzz_vm_exec(_src: &str) { /* TODO: vitte_vm::interpreter::execute */ }

#[cfg(feature = "shims")]
pub fn fuzz_asm(_src: &str) { /* TODO: vitte_core::asm::assemble */ }

#[cfg(feature = "shims")]
pub fn fuzz_wasm_emit(_src: &str) { /* TODO: vitte_wasm::compile */ }

#[cfg(feature = "shims")]
pub fn fuzz_ini(_src: &str) { /* TODO: std formats::ini parse */ }

#[cfg(feature = "shims")]
pub fn fuzz_csv(_src: &str) { /* TODO: std formats::csv parse */ }
