#![no_main]
use libfuzzer_sys::fuzz_target;

#[inline]
fn as_utf8(data: &[u8]) -> Option<&str> { std::str::from_utf8(data).ok() }

fuzz_target!(|data: &[u8]| {
    if let Some(src) = as_utf8(data) {
        #[cfg(not(feature = "shims"))]
        {
            #[allow(unused_imports)]
            use vitte_wasm::compile_source;
            let _ = compile_source(src); // Ajuste selon ton API
        }
        #[cfg(feature = "shims")]
        { vitte_fuzz_shims::fuzz_wasm_emit(src); }
    }
});
