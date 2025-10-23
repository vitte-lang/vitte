

# vitte-ir

Représentation intermédiaire **basse-niveau (IR)** du compilateur Vitte.  
`vitte-ir` formalise la structure interne du code après analyse sémantique et avant génération de code (MIR, LLVM, Cranelift, etc.).

---

## Objectifs

- Fournir une représentation **intermédiaire stable et portable** du langage Vitte.  
- Servir de base pour les optimisations et transformations.  
- Supporter la génération multi-cibles (x86_64, AArch64, RISC-V, WASM).  
- Offrir un modèle typé, SSA-like et compatible avec les backends LLVM et Cranelift.  
- Intégrer la sérialisation et l’inspection via `vitte-inspect`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `builder`     | Construction de l’IR depuis le HIR |
| `value`       | Représentation des valeurs et constantes |
| `instr`       | Instructions de bas-niveau (load, store, call, jump, phi, etc.) |
| `block`       | Structure des blocs de base et contrôle de flux |
| `function`    | Définition des fonctions IR et signatures |
| `type`        | Système de types pour IR (entiers, flottants, pointeurs, agrégats) |
| `ssa`         | Transformation et vérification SSA |
| `analyze`     | Passes d’analyse et validation IR |
| `serialize`   | Sérialisation binaire et JSON pour inspection |
| `tests`       | Tests de cohérence et d’intégration backend |

---

## Exemple d’utilisation

```rust
use vitte_ir::{IRBuilder, Module};

fn main() -> anyhow::Result<()> {
    let mut module = Module::new("main");
    let mut builder = IRBuilder::new(&mut module);

    let func = builder.begin_function("add", &["a", "b"], "i32");
    let a = builder.param(0);
    let b = builder.param(1);
    let sum = builder.add(a, b);
    builder.ret(sum);
    builder.end_function();

    println!("IR généré : {} fonctions", module.functions().len());
    Ok(())
}
```

---

## Intégration

- `vitte-hir` : source directe de l’IR.  
- `vitte-analyzer` : vérification sémantique et typage avant IR.  
- `vitte-compiler` : pipeline principal d’optimisation et codegen.  
- `vitte-codegen-*` : génération machine selon architecture cible.  
- `vitte-inspect` : visualisation et diagnostic de l’IR généré.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-hir = { path = "../vitte-hir", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
``` 

> Optionnel : `petgraph` pour CFG/SSA, `rayon` pour passes parallèles.

---

## Tests

```bash
cargo test -p vitte-ir
```

- Tests de transformation HIR → IR.  
- Tests de cohérence et validation SSA.  
- Tests d’intégration avec `vitte-codegen-*`.  
- Benchmarks sur gros modules pour profiling.

---

## Roadmap

- [ ] Passes d’optimisation bas-niveau (const-folding, DCE).  
- [ ] Support complet SSA avec dominance tree.  
- [ ] IR portable pour exécution JIT.  
- [ ] Compatibilité MIR/LLVM IR.  
- [ ] Visualisation Graphviz via `vitte-inspect`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau