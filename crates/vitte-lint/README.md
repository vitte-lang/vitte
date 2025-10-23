

# vitte-lint

Analyseur **statique et stylistique** du langage Vitte.  
`vitte-lint` détecte les erreurs de style, les mauvaises pratiques et les incohérences sémantiques dans le code source Vitte, en s’appuyant sur le HIR et les règles du compilateur.

---

## Objectifs

- Fournir un **moteur de linting modulaire et extensible**.  
- Détecter les erreurs de style, d’usage et de logique.  
- Offrir une intégration complète avec `vitte-cli` et `vitte-lsp`.  
- Supporter la configuration par projet (`vitte.toml`, `.vitte-lint.yml`).  
- Produire des rapports structurés (texte, JSON, HTML).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `rules`       | Ensemble des règles de lint (syntaxe, style, performance, sécurité) |
| `engine`      | Moteur d’analyse basé sur le HIR et les symboles |
| `config`      | Gestion des fichiers de configuration et profils de lint |
| `report`      | Système de rapport et sortie multi-format |
| `visitor`     | Parcours du HIR pour collecte et vérification |
| `severity`    | Gestion des niveaux de gravité (info, warning, error) |
| `tests`       | Tests unitaires et cas de validation sur code source |

---

## Exemple d’utilisation

```bash
# Analyse du code source courant
vitte lint src/

# Rapport détaillé JSON
vitte lint src/ --format json --output target/lint-report.json

# Lint avec configuration spécifique
vitte lint --config .vitte-lint.yml
```

---

## Intégration

- `vitte-cli` : commande `vitte lint`.  
- `vitte-lsp` : diagnostics en temps réel dans l’éditeur.  
- `vitte-analyzer` : réutilisation du HIR et des symboles.  
- `vitte-build` : exécution automatique en CI/CD.  
- `vitte-docgen` : intégration des rapports dans la documentation.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-hir = { path = "../vitte-hir", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_yaml = "0.9"
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
colored = "2.1"
``` 

> Optionnel : `rayon` pour exécution parallèle, `regex` pour lint textuel.

---

## Tests

```bash
cargo test -p vitte-lint
```

- Tests de règles syntaxiques et sémantiques.  
- Tests de configuration et filtrage.  
- Tests de génération des rapports JSON et HTML.  
- Benchmarks de performance sur grands projets.

---

## Roadmap

- [ ] Lint incrémental via cache HIR.  
- [ ] Mode fix automatique (`--fix`).  
- [ ] Export des résultats vers SARIF.  
- [ ] Intégration avec `vitte-studio` et GitHub Actions.  
- [ ] Règles communautaires externes.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau