# rust workspace (vitte)

- `vitte_rust_api` is the only crate exposing `extern "C"` symbols.
- Everything else is pure Rust library/tooling.

## Build
```sh
cargo build -p vitte_rust_api --release
```

## Generate header (optional)
Requires `cbindgen`:
```sh
cbindgen crates/vitte_rust_api -c cbindgen.toml -o include-gen/vitte_rust_api.h
```
