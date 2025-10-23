

# vitte-fronted-llvm

Front-end **LLVM** pour la génération intermédiaire du langage Vitte.

`vitte-fronted-llvm` traduit l’AST produit par `vitte-fronted` en IR LLVM optimisée, prête à être compilée en code machine pour les backends pris en charge (`x86_64`, `aarch64`, `rv64`, `wasm64`). Ce crate agit comme passerelle entre le parseur et le générateur de code, en exploitant les API LLVM via `inkwell`.

---

## Objectifs

- Génération IR LLVM conforme et optimisée.
- Support multi-architecture (`x86_64`, `aarch64`, `rv64`, `wasm64`).
- Analyse et validation du graphe de contrôle (CFG).
- Intégration directe avec `vitte-codegen-*` et `vitte-compiler`.
- Compatibilité totale avec LLVM 16+.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `builder`      | Génération des instructions LLVM IR |
| `context`      | Gestion du contexte LLVM et du module IR |
| `types`        | Mapping des types Vitte vers LLVM |
| `values`       | Création et gestion des constantes et variables |
| `irpass`       | Passes d’optimisation et vérifications |
| `verify`       | Validation IR et diagnostic |
| `tests`        | Vérification de la génération IR |

---

## Exemple d’utilisation

```rust
use vitte_fronted_llvm::LlvmGenerator;
use vitte_fronted::Parser;

fn main() -> anyhow::Result<()> {
    let source = "fn add(a: Int, b: Int) -> Int { a + b }";
    let ast = Parser::parse_source(source)?;
    let mut gen = LlvmGenerator::new("add_module");
    let ir = gen.emit_module(&ast)?;
    println!("{}", ir);
    Ok(())
}
```

---

## Intégration

- `vitte-fronted` : entrée AST.
- `vitte-codegen-x86_64` / `vitte-codegen-aarch64` : compilation native.
- `vitte-compiler` : orchestration complète de la chaîne build.

---

## Dépendances

```toml
[dependencies]
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }
vitte-fronted = { path = "../vitte-fronted", version = "0.1.0" }
vitte-core = { path = "../vitte-core", version = "0.1.0" }
inkwell = { version = "0.4", features = ["llvm16-0"] }
anyhow = "1"
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-fronted-llvm
```

- Tests de génération IR.
- Vérification des types et signatures LLVM.
- Validation CFG et optimisations.

---

## Roadmap

- [ ] Passes LLVM personnalisées (`Inline`, `DCE`, `GVN`).
- [ ] Export vers fichiers `.bc` et `.ll`.
- [ ] Visualisation CFG.
- [ ] Intégration totale avec `vitte-analyzer`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau