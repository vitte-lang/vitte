

# vitte-codegen

Backend **générique de génération de code** pour le langage Vitte.  
`vitte-codegen` fournit les abstractions, IR intermédiaires et pipelines de génération vers les cibles machine et virtuelles.

---

## Objectifs

- Fournir un **backend unifié** pour toutes les architectures supportées (`x86_64`, `aarch64`, `rv64`, `wasm64`).  
- Traduction efficace de l’IR Vitte vers du code machine, bytecode ou JIT.  
- Architecture modulaire et extensible via traits et registres de passes.  
- Gestion des optimisations SSA, scheduling et registre-allocation.  
- Support des modes **AOT**, **JIT**, et **interprété**.  
- Intégration directe avec `vitte-compiler`, `vitte-build`, et `vitte-runtime`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `ir`          | Représentation intermédiaire (SSA, blocs, instructions) |
| `passes`      | Passes d’optimisation (simplification, DCE, inlining) |
| `emit`        | Émission finale : code machine, bytecode ou WASM |
| `isa`         | Description des architectures (regs, ABI, calling conv) |
| `abi`         | Implémentation des conventions d’appel par cible |
| `builder`     | API pour construire et manipuler le code IR |
| `context`     | Gestion des symboles, fonctions et constantes |
| `link`        | Liaison et patch des sections générées |
| `verify`      | Vérification de la cohérence IR et CFG |
| `tests`       | Tests d’intégration et comparaison cross-backend |

---

## Exemple d’utilisation

```rust
use vitte_codegen::{Context, FunctionBuilder, Target};

fn main() -> anyhow::Result<()> {
    // 1) Créer un contexte de compilation
    let mut ctx = Context::new(Target::X86_64);

    // 2) Créer une fonction IR
    let mut func = ctx.create_function("add");
    {
        let mut b = FunctionBuilder::new(&mut func);
        let a = b.param_i64();
        let b_ = b.param_i64();
        let sum = b.add(a, b_);
        b.ret(sum);
    }

    // 3) Compiler et émettre le code machine
    let code = ctx.compile(&func)?;
    std::fs::write("add.o", code)?;

    Ok(())
}
```

---

## Intégration

- `vitte-compiler` : transformation MIR → IR → code machine.  
- `vitte-build` : gestion multi-cibles et linking.  
- `vitte-runtime` : chargement et exécution JIT ou AOT.  
- `vitte-analyzer` : visualisation IR et CFG.  
- `vitte-lsp` : inspection symbolique et débogage pas-à-pas.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ir = { path = "../vitte-ir", version = "0.1.0" }

cranelift-codegen = "0.110"
cranelift-frontend = "0.110"
cranelift-module = "0.110"
cranelift-object = "0.110"
wasmtime-environ = "25"
anyhow = "1"
thiserror = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
``` 

> Optionnel : `rayon` pour parallélisme de passes, `faerie` ou `object` pour backends custom.

---

## Tests

```bash
cargo test -p vitte-codegen
```

- Tests d’émission multi-ISA (`x86_64`, `aarch64`, `rv64`, `wasm64`).  
- Tests d’optimisation et vérification CFG.  
- Comparaison entre sorties Cranelift et LLVM.  
- Benchmarks sur IR de référence (`fib`, `nbody`, `matrix`).

---

## Roadmap

- [ ] Backend LLVM complet.  
- [ ] Support JIT multi-arch intégré à `vitte-runtime`.  
- [ ] Passes SSA globales et DCE avancé.  
- [ ] Backend SPIR-V expérimental.  
- [ ] Génération DWARF pour debug.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau