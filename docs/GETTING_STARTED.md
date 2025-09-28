# Getting Started

## Prérequis
- **Rust** (toolchain épinglée dans `rust-toolchain.toml`)  
  MSRV : **1.82.0**
- Composants Rust : `rustfmt`, `clippy`, `rust-src`, `llvm-tools-preview`
- Optionnels : `wasm-pack`, `npm` (pour intégration WebAssembly et bindings JS)

---

## Cloner et compiler le projet
```bash
git clone https://github.com/vitte-lang/vitte.git
cd vitte
cargo build --workspace --all-features
cargo test  --workspace --all-features
```

---

## Utiliser la CLI
```bash
cargo run -p vitte-cli -- vitte --help
```
> Note : certains outils additionnels se trouvent dans le package `vitte-tools`.

---

## Prochaines étapes
- Lire l’[Architecture](./ARCHITECTURE/overview.md)
- Installer les [complétions CLI](./COMPLETIONS.md)
- Explorer la [VM](./VM/overview.md)
- Étudier la [Spécification VITBC](./VITBC/spec.md)
- Consulter les [Benchmarks](./BENCHMARKS.md)
- Suivre le [Guide de Build](./BUILD.md)
- Découvrir la [Stdlib](./STDLIB/guide.md)
- Plonger dans le [Runtime](./RUNTIME/overview.md)
