

# vitte-compiler

Cœur **du compilateur Vitte**.  
`vitte-compiler` orchestre toutes les phases de compilation : analyse lexicale, parsing, typage, optimisation, génération de code et intégration des backends (`vitte-codegen-*`).

---

## Objectifs

- Offrir une **chaîne de compilation complète** et modulaire.  
- Supporter les modes de compilation : **AOT**, **JIT**, **interprété**.  
- Architecture en passes claires et indépendantes.  
- Optimisations SSA et gestion avancée des dépendances.  
- Intégration directe avec `vitte-build`, `vitte-analyzer`, `vitte-cache` et `vitte-cap`.  
- API stable pour les outils externes (`vitte-lsp`, `vitte-docgen`, `vitte-clippy`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `driver`      | Entrée principale du compilateur et coordination des passes |
| `session`     | Contexte global : options, erreurs, cache, capabilities |
| `lexer`       | Analyse lexicale et tokenisation |
| `parser`      | Parsing du code source en AST |
| `resolve`     | Résolution des symboles, imports et dépendances |
| `types`       | Vérification et inférence des types |
| `hir`         | Représentation intermédiaire haute-niveau |
| `mir`         | IR moyenne (optimisable, SSA-ready) |
| `codegen`     | Interface avec les backends (`x86_64`, `aarch64`, `rv64`, `wasm64`) |
| `link`        | Édition de liens et création des binaires finaux |
| `passes`      | Pipeline d’optimisation configurable |
| `diagnostics` | Gestion structurée des erreurs et avertissements |
| `tests`       | Tests unitaires et intégration multi-backends |

---

## Exemple d’utilisation

```rust
use vitte_compiler::Compiler;
use vitte_core::config::BuildMode;

fn main() -> anyhow::Result<()> {
    let mut compiler = Compiler::new()?;

    compiler.compile_file(
        "src/main.vitte",
        BuildMode::Aot,
        Some("target/main.vitbc"),
    )?;

    println!("Compilation terminée avec succès.");
    Ok(())
}
```

---

## Intégration

- `vitte-build` : pilotage du processus de compilation.  
- `vitte-analyzer` : récupération et validation des graphes sémantiques.  
- `vitte-lsp` : diagnostics et navigation IDE.  
- `vitte-docgen` : extraction et génération documentaire.  
- `vitte-cap` : exécution confinée et autorisations de compilation.  
- `vitte-cache` : stockage et réutilisation des artefacts IR/MIR.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }
vitte-hir = { path = "../vitte-hir", version = "0.1.0" }
vitte-mir = { path = "../vitte-mir", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }
vitte-cache = { path = "../vitte-cache", version = "0.1.0" }
vitte-cap = { path = "../vitte-cap", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
rayon = "1"
parking_lot = "0.12"
``` 

> Optionnel : `cranelift`, `llvm-sys` et `wasmparser` pour les backends spécifiques.

---

## Tests

```bash
cargo test -p vitte-compiler
```

- Tests de compilation complète (syntaxe → code machine).  
- Tests d’optimisation IR/MIR.  
- Tests de cross-compilation sur toutes les cibles.  
- Benchmarks de performances sur projets réels.

---

## Roadmap

- [ ] Compilation incrémentale et parallélisée.  
- [ ] Mode interactif REPL intégré.  
- [ ] Profilage des passes et visualisation graphique.  
- [ ] Support des modules externes signés.  
- [ ] Intégration LLVM complète et JIT multi-arch.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau