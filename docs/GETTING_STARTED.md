# Getting Started

## Prérequis
- Rust (toolchain épinglée dans `rust-toolchain.toml`), MSRV : **1.80.0**
- `rustfmt`, `clippy`, `rust-src`, `llvm-tools-preview`
- Optionnels : `wasm-pack`, `npm`

## Clone & build
```bash
git clone https://github.com/vitte-lang/vitte.git
cd vitte
cargo build --workspace --all-features
cargo test  --workspace --all-features
```

## Utiliser la CLI
```bash
cargo run -p vitte-tools --features cli -- vitte --help
```

## Prochaines étapes
- Lire l’[Architecture](./ARCHITECTURE/overview.md)
- Installer les [complétions CLI](./COMPLETIONS.md)
- Parcourir la [VM](./VM/overview.md) et la [Spec VITBC](./VITBC/spec.md)
