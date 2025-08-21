#![no_main]
use libfuzzer_sys::fuzz_target;

#[inline]
fn as_utf8(data: &[u8]) -> Option<&str> { std::str::from_utf8(data).ok() }

fuzz_target!(|data: &[u8]| {
    #[cfg(not(feature = "shims"))]
    {
        #[allow(unused_imports)]
        use vitte_core::loader;
        let _ = loader::load_bytes(data); // Ajuste selon ton API
    }
    #[cfg(feature = "shims")]
    {
        vitte_fuzz_shims::fuzz_bytecode_loader(data);
    }
});
