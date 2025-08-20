#![no_main]
use libfuzzer_sys::fuzz_target;

#[inline]
fn as_utf8(data: &[u8]) -> Option<&str> { std::str::from_utf8(data).ok() }

fuzz_target!(|data: &[u8]| {
    if let Some(src) = as_utf8(data) {
        #[cfg(not(feature = "shims"))]
        {
            #[allow(unused_imports)]
            use vitte_core::runtime::{parser, pretty};
            if let Ok(ast) = parser::parse(src) {
                let _ = pretty::format(&ast);
            }
        }
        #[cfg(feature = "shims")]
        {
            vitte_fuzz_shims::fuzz_pretty(src);
        }
    }
});
