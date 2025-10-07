#[allow(dead_code)]
#[cfg(all(feature = "std", feature = "errors"))]
#[inline]
fn map_io() -> impl FnOnce(std::io::Error) -> Error {
    |e| Error::from(e)
}

#[allow(dead_code)]
#[cfg(all(feature = "std", not(feature = "errors")))]
#[inline]
fn map_io() -> impl FnOnce(std::io::Error) -> &'static str {
    |_e| "io error"
}