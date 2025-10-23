

# vitte-cov

Outil **d’analyse de couverture de code** pour le langage Vitte.  
`vitte-cov` collecte, agrège et visualise les informations de couverture issues des exécutions de tests unitaires et d’intégration.

---

## Objectifs

- Fournir un système complet de **mesure de couverture** (lignes, blocs, fonctions).  
- Supporter les rapports **HTML**, **JSON**, **LCOV** et **textuels**.  
- Intégration directe avec `vitte-test`, `vitte-build` et `vitte-analyzer`.  
- Compatible avec le runtime Vitte natif et le backend JIT.  
- Support multi‑fichiers et multi‑modules, avec filtrage par crate.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `collector`   | Instrumentation et collecte des compteurs de couverture |
| `merge`       | Fusion des résultats de plusieurs exécutions |
| `analyze`     | Calcul des métriques (lignes couvertes, ratio, densité) |
| `render`      | Génération des rapports (HTML, JSON, LCOV) |
| `filter`      | Sélection des fichiers et fonctions analysées |
| `cli`         | Interface en ligne de commande (`vitte cov …`) |
| `tests`       | Tests de cohérence et validation sur projets de référence |

---

## Exemple d’utilisation

```bash
# Lancer les tests avec instrumentation
vitte test --with-cov

# Générer un rapport HTML
vitte cov --format html --output target/coverage

# Générer un rapport LCOV pour CI
vitte cov --format lcov --output coverage.info

# Rapport JSON détaillé
vitte cov --format json --output coverage.json
```

---

## Intégration

- `vitte-test` : instrumentation des exécutions.  
- `vitte-analyzer` : annotation du code source avec ratios de couverture.  
- `vitte-docgen` : intégration visuelle dans la documentation générée.  
- `vitte-cli` : sous-commande intégrée (`vitte cov`).  
- `vitte-build` : support CI/CD via rapports LCOV.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
walkdir = "2.5"
regex = "1"
indicatif = "0.17"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `plotters` pour graphes SVG, `tabled` pour affichage CLI.

---

## Tests

```bash
cargo test -p vitte-cov
```

- Tests de fusion de rapports et ratios.  
- Tests sur projets multi‑modules.  
- Validation LCOV et export JSON.  
- Tests d’intégration avec `vitte-test` et `vitte-cli`.

---

## Roadmap

- [ ] Génération interactive HTML avec surbrillance du code.  
- [ ] Support du format `cobertura.xml` pour CI externes.  
- [ ] Intégration directe avec GitHub Actions.  
- [ ] Comparaison de couverture entre commits.  
- [ ] Visualisation intégrée dans `vitte-docgen`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau