

# vitte-fronted-wasm

Front-end **WebAssembly (WASM)** pour la génération intermédiaire du langage Vitte.

`vitte-fronted-wasm` traduit l’AST produit par `vitte-fronted` en IR WebAssembly binaire ou textuelle (`.wasm` / `.wat`). Il permet la compilation des programmes Vitte vers une cible universelle portable, utilisable dans les navigateurs, environnements WASI ou serveurs.

---

## Objectifs

- Génération de code WebAssembly conforme à la spécification WASM 2.0.
- Support de WASI, import/export, mémoire et appels de fonctions.
- Intégration complète avec `vitte-compiler` et `vitte-runtime`.
- Génération binaire (`.wasm`) et textuelle (`.wat`).
- Compatibilité avec les outils `wasmtime`, `wasmer`, `node --experimental-wasm`.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `builder`      | Génération des instructions WASM |
| `context`      | Gestion du module, mémoire et tables |
| `types`        | Mapping des types Vitte vers WASM |
| `encode`       | Sérialisation binaire (`.wasm`) |
| `emit`         | Émission textuelle (`.wat`) |
| `verify`       | Validation du module avant export |
| `tests`        | Cas de compilation et vérification binaire |

---

## Exemple d’utilisation

```rust
use vitte_fronted_wasm::WasmGenerator;
use vitte_fronted::Parser;

fn main() -> anyhow::Result<()> {
    let src = "fn add(a: Int, b: Int) -> Int { a + b }";
    let ast = Parser::parse_source(src)?;
    let mut wasm = WasmGenerator::new();
    wasm.emit_module(&ast)?;
    wasm.save_binary("add.wasm")?;
    Ok(())
}
```

---

## Compilation CLI

```bash
vitte build main.vit --target wasm32-wasi
vitte run main.vit --target wasm
```

Génère :
```
target/wasm32-wasi/debug/main.wasm
```

---

## Dépendances

```toml
[dependencies]
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }
vitte-fronted = { path = "../vitte-fronted", version = "0.1.0" }
vitte-core = { path = "../vitte-core", version = "0.1.0" }

wasm-encoder = "0.45"
wasmparser = "0.130"
anyhow = "1"
serde = { version = "1.0", features = ["derive"] }
```

---

## Tests

```bash
cargo test -p vitte-fronted-wasm
```

- Vérification binaire et textuelle.
- Validation du module via `wasmparser`.
- Tests de compatibilité WASI.

---

## Roadmap

- [ ] Support de threading et mémoire partagée.
- [ ] Compilation directe vers WASIX.
- [ ] Intégration avec `vitte-embed` (exécution JS).
- [ ] Optimisations de taille (shrink pass).

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau