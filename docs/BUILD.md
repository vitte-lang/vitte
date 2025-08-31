# Build

- **Profils** : voir `[profile.*]` dans `Cargo.toml` racine.
- **Lints** : `.clippy.toml` + `[workspace.lints]` interdisent `unsafe` et durcissent Clippy.
- **cargo-deny** : politique dans `deny.toml` (RustSec, licences, sources).
- **Docs** : `cargo doc --workspace --all-features --no-deps`.

## Commandes utiles
```bash
cargo fmt --all -- --check
cargo clippy --workspace --all-features -- -D warnings
cargo test  --workspace --all-features
cargo deny check advisories bans licenses sources
```

## Cross-compilation
Cibles courantes : Linux (gnu/musl), macOS (x86_64/arm64), Windows (MSVC), WebAssembly (wasm32-unknown-unknown / WASI), embarqué (thumb*/riscv*). Voir `rust-toolchain.toml`.
