

# vitte-pattern

Moteur **de correspondance de motifs (pattern matching)** pour le langage Vitte.  
`vitte-pattern` gère l’analyse, la résolution et l’évaluation des motifs utilisés dans les expressions `match`, les destructurations, les filtres et les gardes de flux de contrôle.

---

## Objectifs

- Offrir un **système de correspondance de motifs complet et performant**.  
- Supporter les motifs structurés (`struct`, `enum`, `tuple`, `array`, `range`, `or`, `guard`).  
- Intégration directe avec `vitte-analyzer`, `vitte-ast` et `vitte-compiler`.  
- Gestion précise des erreurs et des cas non exhaustifs.  
- Optimisation des tables de décision pour les expressions complexes.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `analyze`     | Analyse de la couverture et exhaustivité des motifs |
| `builder`     | Construction des arbres de décision pour `match` |
| `compile`     | Génération IR des blocs de correspondance |
| `expr`        | Gestion des expressions et motifs imbriqués |
| `guard`       | Évaluation des gardes et conditions secondaires |
| `pattern`     | Représentation interne des motifs typés |
| `tests`       | Vérification de cohérence et couverture de motifs |

---

## Exemple d’utilisation

```vitte
fn describe(x: Option<int>) -> str {
    match x {
        Some(0) => "zéro",
        Some(v) if v < 0 => "négatif",
        Some(v) => "positif",
        None => "aucune valeur",
    }
}
```

---

## Exemple IR simplifié

```text
MatchOption {
  arms: [
    Arm(Some(0), Return("zéro")),
    Arm(Some(v < 0), Return("négatif")),
    Arm(Some(_), Return("positif")),
    Arm(None, Return("aucune valeur"))
  ]
}
```

---

## Intégration

- `vitte-analyzer` : vérifie l’exhaustivité et la compatibilité des motifs.  
- `vitte-ast` : définit la structure syntaxique des motifs.  
- `vitte-compiler` : génère le code IR des branches `match`.  
- `vitte-lsp` : fournit des diagnostics pour les motifs manquants.  
- `vitte-docgen` : exporte les motifs dans la documentation interactive.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ast = { path = "../vitte-ast", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }

petgraph = "0.6"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
``` 

> Optionnel : `bitset` pour accélérer les tables de décision, `indexmap` pour ordonner les motifs.

---

## Tests

```bash
cargo test -p vitte-pattern
```

- Tests unitaires des motifs simples et imbriqués.  
- Tests d’exhaustivité et de couverture.  
- Tests d’intégration avec `vitte-analyzer` et `vitte-compiler`.  
- Benchmarks sur les arbres de décision.

---

## Roadmap

- [ ] Optimisation de la génération IR (fusion de branches).  
- [ ] Support des motifs dynamiques et destructuration complexe.  
- [ ] Mode exhaustif pour `vitte-lsp` avec visualisation.  
- [ ] Système de suggestion automatique de cas manquants.  
- [ ] Extension vers `vitte-ir` pour motifs paramétriques.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau