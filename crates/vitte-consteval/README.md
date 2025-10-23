

# vitte-consteval

Moteur **d’évaluation constante** pour le langage Vitte.  
`vitte-consteval` exécute les expressions à la compilation et gère la propagation des valeurs constantes dans l’IR, le typage et la génération de code.

---

## Objectifs

- Permettre l’évaluation **compile-time** des expressions (`const fn`, `constexpr`, `static`).  
- Supporter les primitives de contrôle (`if`, `match`, boucles bornées).  
- Gestion complète des types numériques, booléens, pointeurs et structures immuables.  
- Optimiser le code via **propagation constante** et **folding arithmétique**.  
- Fournir une API interne utilisée par `vitte-analyzer`, `vitte-compiler` et `vitte-lsp`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `engine`      | Moteur principal d’évaluation et pile d’exécution |
| `context`     | Contexte d’évaluation et table des symboles |
| `value`       | Représentation interne des valeurs constantes |
| `interpreter` | Évaluation pas-à-pas d’expressions et d’opérateurs |
| `ops`         | Implémentation des opérations primitives (`add`, `cmp`, `cast`, etc.) |
| `fold`        | Passes de propagation et simplification constantes |
| `types`       | Compatibilité et coercions de types en contexte constant |
| `errors`      | Gestion des diagnostics et erreurs d’évaluation |
| `tests`       | Vérification de la justesse et des optimisations |

---

## Exemple d’utilisation

```rust
use vitte_consteval::ConstEngine;
use vitte_consteval::value::Value;

fn main() -> anyhow::Result<()> {
    let mut engine = ConstEngine::default();
    let expr = "1 + 2 * 3";

    let result = engine.eval(expr)?;
    assert_eq!(result, Value::Int(7));

    println!("Résultat compile-time: {:?}", result);
    Ok(())
}
```

---

## Intégration

- `vitte-analyzer` : évaluation de constantes dans l’AST/HIR.  
- `vitte-compiler` : propagation IR et folding de code mort.  
- `vitte-lsp` : surbrillance et inspection de valeurs constantes.  
- `vitte-docgen` : rendu des valeurs dans la documentation.  
- `vitte-clippy` : vérification des expressions non constantes.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }

thiserror = "1"
anyhow = "1"
num = "0.4"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
``` 

> Optionnel : `rug` pour arithmétique multiprécision, `smallvec` pour évaluation rapide.

---

## Tests

```bash
cargo test -p vitte-consteval
```

- Tests unitaires sur opérations arithmétiques et logiques.  
- Tests d’évaluation de blocs `const` et `constexpr fn`.  
- Benchmarks d’optimisation constante sur gros modules.  
- Vérification de la cohérence entre IR et MIR.

---

## Roadmap

- [ ] Support des tableaux et slices constants.  
- [ ] Évaluation des lambdas et closures `const`.  
- [ ] Exécution symbolique limitée pour propagation conditionnelle.  
- [ ] Mémorisation et cache global des résultats constants.  
- [ ] Support complet des floats et `NaN` constants.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau