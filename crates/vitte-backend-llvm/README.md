

# vitte-backend-llvm

Backend **LLVM** du compilateur Vitte.  
`vitte-backend-llvm` traduit le bytecode intermédiaire de Vitte (`.vitbc`) ou l’IR interne vers LLVM IR, puis produit du code machine optimisé pour plusieurs architectures cibles.

---

## Objectifs

- Génération de code LLVM IR portable et optimisé.  
- Support multi-architecture : `x86_64`, `aarch64`, `rv64`, `wasm64`.  
- Compilation **AOT** (ahead-of-time) et **JIT** (just-in-time).  
- Passes d’optimisation complètes (inline, dce, gvn, sroa, mem2reg).  
- Liaison directe avec `vitte-runtime` et `vitte-compiler`.  
- Génération d’objets, exécutables, bitcode et assembly humain.  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `context`      | Initialisation du contexte LLVM |
| `irbuilder`    | Construction du IR LLVM à partir de l’IR Vitte |
| `passes`       | Application des optimisations LLVM standard |
| `linker`       | Fusion et liaison d’unités compilées |
| `jit`          | Interface d’exécution dynamique (MCJIT/ORC) |
| `aot`          | Compilation vers objets et binaires |
| `verify`       | Validation IR et diagnostic des modules LLVM |
| `emit`         | Export LLVM IR, bitcode ou code machine |
| `tests`        | Validation et comparaison avec les backends alternatifs |

---

## Exemple d’utilisation

```rust
use vitte_backend_llvm::LlvmBackend;
use vitte_bytecode::BytecodeModule;

fn main() -> anyhow::Result<()> {
    let module = BytecodeModule::load("examples/add.vitbc")?;
    let backend = LlvmBackend::new_default();
    backend.compile_module(&module)?;
    backend.emit_object("output.o")?;
    Ok(())
}
```

---

## Intégration

- `vitte-compiler` : pipeline principal de génération LLVM.  
- `vitte-runtime` : exécution native du code produit.  
- `vitte-analyzer` : instrumentation et analyse des performances.  
- `vitte-debug` : désassemblage et inspection LLVM IR.  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-compiler = { path = "../vitte-compiler", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

inkwell = { version = "0.4", features = ["llvm16-0"] }
target-lexicon = "0.12"
object = "0.36"
anyhow = "1"
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-backend-llvm
```

- Tests d’intégration sur architectures simulées.  
- Vérification des instructions LLVM générées.  
- Comparaison de performance avec Cranelift.  
- Snapshots de désassemblage LLVM.  

---

## Roadmap

- [ ] Support complet de la toolchain LLVM 18+.  
- [ ] Compilation croisée (cross-targets).  
- [ ] Génération PDB/DWARF pour debug.  
- [ ] Visualisation du IR (Graphviz).  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau