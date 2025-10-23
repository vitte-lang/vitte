

# vitte-macro

Système **de macros et de métaprogrammation** pour le langage Vitte.  
`vitte-macro` fournit les primitives d’expansion, d’analyse et de génération de code au moment de la compilation, permettant la création de macros procédurales, dérivées et déclaratives.

---

## Objectifs

- Offrir un **système de macros robuste et sécurisé**.  
- Supporter les macros procédurales (AST → AST) et dérivées (`#[derive(...)]`).  
- Fournir une API pour la génération de code Vitte à partir du HIR/AST.  
- Intégration complète avec `vitte-compiler`, `vitte-analyzer` et `vitte-ast`.  
- Garantir la reproductibilité et la sécurité d’exécution des macros.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `parser`      | Analyse et validation des macros déclarées |
| `expand`      | Système d’expansion et d’injection AST |
| `derive`      | Gestion des macros dérivées (implémentations automatiques) |
| `context`     | Gestion du contexte d’expansion (fichier, portée, symboles) |
| `builder`     | Génération et transformation du code source |
| `sandbox`     | Isolation et sécurité des macros procédurales |
| `tests`       | Tests d’intégration et d’expansion complexe |

---

## Exemple d’utilisation

```vitte
#[derive(Display, Clone)]
struct Point {
    x: f32,
    y: f32,
}

macro_rules! say_hello {
    () => {
        println!("Bonjour depuis une macro Vitte!");
    };
}
```

---

## Intégration

- `vitte-compiler` : expansion des macros avant la génération IR.  
- `vitte-ast` : manipulation du code source en arbres syntaxiques.  
- `vitte-analyzer` : vérification des macros dérivées et résolues.  
- `vitte-docgen` : génération de documentation à partir de macros publiques.  
- `vitte-lsp` : complétion et navigation dans les macros.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }

syn = "2.0"
quote = "1.0"
proc-macro2 = "1.0"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
``` 

> Optionnel : `wasmtime` pour sandboxing des macros procédurales externes.

---

## Tests

```bash
cargo test -p vitte-macro
```

- Tests d’expansion déclarative et procédurale.  
- Tests d’interopérabilité entre macros dérivées et manuelles.  
- Tests de compatibilité avec `vitte-ast` et `vitte-analyzer`.  
- Benchmarks de performance sur expansions massives.

---

## Roadmap

- [ ] Système de cache pour macros dérivées.  
- [ ] Sandbox WASM pour macros externes sécurisées.  
- [ ] Support des attributs imbriqués et génératifs.  
- [ ] Documentation automatique via `vitte-docgen`.  
- [ ] Visualisation des expansions dans `vitte-inspect`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau