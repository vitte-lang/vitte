# vitte-backend-cranelift

Backend **Cranelift JIT/AOT** pour le langage Vitte.  
Ce crate permet la compilation dynamique et native du bytecode Vitte (`.vitbc`) en code machine via la bibliothèque [Cranelift](https://github.com/bytecodealliance/wasmtime/tree/main/cranelift).

---

## Objectifs

- Génération de code machine portable et rapide.  
- Compilation **AOT** (ahead-of-time) et **JIT** (just-in-time).  
- Support multi-architecture : `x86_64`, `aarch64`, `rv64`, `wasm64`.  
- Intégration directe avec `vitte-compiler` et `vitte-runtime`.  
- Support complet des optimisations SSA et IR passes.  
- Interface de debug et désassemblage Cranelift IR.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `builder`      | Construction du module Cranelift IR |
| `irgen`        | Conversion du bytecode Vitte → IR Cranelift |
| `passes`       | Application des optimisations Cranelift |
| `link`         | Liaison des symboles et exports/imports |
| `jit`          | Interface d’exécution dynamique (JIT) |
| `aot`          | Compilation vers objets natifs |
| `verify`       | Validation IR et intégrité du code généré |
| `tests`        | Snapshots et exécution sur architectures simulées |

---

## Exemple d’utilisation

```rust
use vitte_backend_cranelift::CraneliftBackend;
use vitte_bytecode::BytecodeModule;

fn main() -> anyhow::Result<()> {
    let module = BytecodeModule::load("examples/add.vitbc")?;
    let backend = CraneliftBackend::new_default();
    let compiled = backend.compile_module(&module)?;
    compiled.run_entry("main", &[])?;
    Ok(())
}
```

---

## Intégration

- `vitte-compiler` : génération AOT/JIT.  
- `vitte-runtime` : exécution directe en mémoire.  
- `vitte-debug` : désassemblage et inspection IR.  
- `vitte-gc` : interopérabilité mémoire pour objets alloués.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-compiler = { path = "../vitte-compiler", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

cranelift-codegen = "0.104"
cranelift-module = "0.104"
cranelift-object = "0.104"
cranelift-jit = "0.104"
target-lexicon = "0.12"
object = "0.36"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-backend-cranelift
```

- Compilation AOT et exécution native.  
- Validation des instructions générées.  
- Tests de compatibilité sur plateformes simulées.  
- Benchmarks comparatifs avec LLVM backend.  

---

## Roadmap

- [ ] Optimisations spécifiques à chaque architecture.  
- [ ] Compilation incrémentale et caching.  
- [ ] Support complet du mode debug (DWARF).  
- [ ] Analyse des performances via `vitte-analyzer`.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
