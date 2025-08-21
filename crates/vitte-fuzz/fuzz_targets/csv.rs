#![no_main]
use libfuzzer_sys::fuzz_target;

#[inline]
fn as_utf8(data: &[u8]) -> Option<&str> { std::str::from_utf8(data).ok() }

fuzz_target!(|data: &[u8]| {
    if let Some(src) = as_utf8(data) {
        #[cfg(not(feature = "shims"))]
        {
            #[allow(unused_imports)]
            use vitte_stdlib_csv as csv; // si exposé côté Rust
            let _ = csv::parse(src);
        }
        #[cfg(feature = "shims")]
        { vitte_fuzz_shims::fuzz_csv(src); }
    }
});
