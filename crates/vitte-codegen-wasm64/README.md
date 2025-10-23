

# vitte-codegen-wasm64

Backend **WebAssembly 64 bits (WASM64)** pour le compilateur Vitte.  
`vitte-codegen-wasm64` implémente la génération de bytecode WebAssembly à partir de l’IR Vitte, pour une exécution portable et sécurisée dans des environnements isolés.

---

## Objectifs

- Génération de **bytecode WASM64** depuis l’IR Vitte.  
- Support du **mode JIT** et **AOT** via Wasmtime ou Wasmer.  
- Émission de modules WebAssembly valides avec sections `type`, `func`, `memory`, `table`, `export`, `code`.  
- Gestion des imports/exports et des symboles globaux.  
- Intégration directe avec `vitte-runtime` pour l’exécution isolée.  
- Respect du modèle de sécurité sans accès mémoire arbitraire.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération des sections WASM et sérialisation binaire |
| `lower`       | Conversion de l’IR générique vers instructions WASM |
| `builder`     | Construction des modules, types et imports |
| `export`      | Gestion des symboles exportés et liaison dynamique |
| `memory`      | Gestion des segments mémoire et initialisation |
| `validate`    | Vérification des modules générés (structure et types) |
| `link`        | Assemblage des modules multiples et patchs de dépendances |
| `tests`       | Tests de génération et validation WAT/WASM |

---

## Exemple d’utilisation

```rust
use vitte_codegen_wasm64::WasmBackend;
use vitte_codegen::{Context, Target};

fn main() -> anyhow::Result<()> {
    let mut ctx = Context::new(Target::Wasm64);
    let backend = WasmBackend::new();

    let func = ctx.create_function("sum");
    let wasm = backend.compile(&ctx, &func)?;

    std::fs::write("sum.wasm", wasm)?;
    Ok(())
}
```

---

## Intégration

- `vitte-codegen` : fournit l’IR intermédiaire et les passes communes.  
- `vitte-runtime` : exécution sandboxée via moteurs Wasmtime/Wasmer.  
- `vitte-compiler` : compilation AOT/JIT vers cible WebAssembly.  
- `vitte-docgen` : rendu interactif des modules via WASM.  
- `vitte-lsp` : inspection du bytecode et des symboles exportés.

---

## Dépendances

```toml
[dependencies]
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

wasm-encoder = "0.206"
wasmparser = "0.206"
wasmtime-environ = "25"
anyhow = "1"
thiserror = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
``` 

> Optionnel : `wasmer` pour exécution embarquée, `wast` pour parsing texte.

---

## Tests

```bash
cargo test -p vitte-codegen-wasm64
```

- Tests de génération IR → WASM.  
- Validation binaire via `wasmparser`.  
- Tests d’exécution dans `wasmtime`.  
- Comparaison avec le backend `x86_64` pour cohérence.

---

## Roadmap

- [ ] Support complet WASI (filesystem, network, stdin/out).  
- [ ] Génération JIT avec cache binaire.  
- [ ] Support des symboles de débogage DWARF dans WASM.  
- [ ] Multi-mémoire et threads (`memory64`, `threads`).  
- [ ] Profilage via `vitte-analyzer` et intégration web.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau