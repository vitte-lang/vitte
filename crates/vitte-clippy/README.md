

# vitte-clippy

Linter **avancé et extensible** pour le langage Vitte.  
`vitte-clippy` analyse le code source Vitte et détecte les erreurs, mauvaises pratiques, optimisations manquées et incohérences stylistiques.

---

## Objectifs

- Offrir des **diagnostics riches** et précis (style, sémantique, performance).  
- Fournir une **API d’extensions** pour lint personnalisés.  
- Fonctionner en ligne de commande et via `vitte-lsp`.  
- Intégration complète avec `vitte-analyzer` et `vitte-build`.  
- Support de la **désactivation granulaire** (`#[allow]`, `#[deny]`, `#[warn]`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `driver`      | Entrée principale CLI et intégration `vitte-analyzer` |
| `rules`       | Définition des lint : style, performance, sûreté, idiomes |
| `context`     | Contexte d’analyse, symboles et environnement |
| `visitor`     | Parcours de l’AST et collecte des diagnostics |
| `report`      | Rendu des avertissements et erreurs (TTY, JSON, SARIF) |
| `suppress`    | Gestion des attributs `#[allow]`, `#[deny]`, `#[warn]` |
| `fix`         | Suggestions automatiques et corrections (`--fix`) |
| `tests`       | Tests de lint et golden tests |

---

## Exemple d’utilisation

```bash
# Lancer l’analyse sur un projet complet
vitte clippy

# Lancer un lint spécifique
vitte clippy --lint unused_imports

# Corriger automatiquement les problèmes simples
vitte clippy --fix

# Sortie JSON pour CI
vitte clippy --format json > report.json

# Ignorer un fichier
#[allow(clippy::module_name_repetitions)]
```

---

## Intégration

- `vitte-analyzer` : partage du moteur d’analyse sémantique.  
- `vitte-build` : lint post-compilation.  
- `vitte-lsp` : publication de diagnostics en temps réel.  
- `vitte-docgen` : extraction de documentation annotée.  
- `vitte-cli` : exécution via sous-commande `vitte clippy`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
thiserror = "1"
anyhow = "1"
colored = "2"
termcolor = "1"
rayon = "1"
```

---

## Tests

```bash
cargo test -p vitte-clippy
```

- Tests unitaires sur les règles de lint.  
- Golden tests de sortie TTY et JSON.  
- Tests de performance multi-fichiers.  
- Vérification de la conformité stylistique.

---

## Roadmap

- [ ] Intégration complète SARIF pour GitHub Actions.  
- [ ] Lints automatiques basés sur `vitte-analyzer` (dataflow, borrowck).  
- [ ] Règles communautaires chargeables dynamiquement.  
- [ ] Commande `vitte clippy --explain` pour documentation détaillée.  
- [ ] Support des profils de lint (`strict`, `pedantic`, `nursery`).

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau