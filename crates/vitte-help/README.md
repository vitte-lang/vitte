

# vitte-help

Système **d’aide et de documentation interactive** pour la suite d’outils Vitte.  
`vitte-help` fournit les descriptions des commandes, options et sous-commandes CLI, ainsi que la navigation contextuelle dans la documentation locale ou en ligne.

---

## Objectifs

- Offrir une **aide contextuelle dynamique** intégrée à `vitte-cli`.  
- Afficher la documentation des sous-commandes et modules (`vitte build`, `vitte test`, etc.).  
- Supporter plusieurs modes de rendu : **CLI**, **man**, **HTML**, **JSON**.  
- Permettre la recherche plein texte et la navigation interactive.  
- Intégration complète avec `vitte-docgen` et `vitte-lsp`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `model`       | Représentation typée des commandes et options |
| `render`      | Génération du texte d’aide (ANSI, Markdown, JSON) |
| `context`     | Détection du contexte d’appel et de la sous-commande |
| `search`      | Recherche plein texte dans la base documentaire |
| `format`      | Mise en page CLI (indentation, coloration, table des options) |
| `loader`      | Chargement des données depuis `vitte-docgen` ou fichiers man |
| `tests`       | Validation du rendu et cohérence de la base documentaire |

---

## Exemple d’utilisation

```bash
# Aide globale
vitte help

# Aide sur une sous-commande
vitte help build

# Aide détaillée JSON pour scripts externes
vitte help build --format json
```

---

## Intégration

- `vitte-cli` : point d’entrée principal pour l’aide interactive.  
- `vitte-docgen` : génération des fichiers d’aide et manuels.  
- `vitte-lsp` : intégration des aides contextuelles IDE.  
- `vitte-build` : affichage des options avancées de compilation.  
- `vitte-completion` : génération des complétions basée sur la base d’aide.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-cli = { path = "../vitte-cli", version = "0.1.0" }
vitte-docgen = { path = "../vitte-docgen", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
regex = "1"
colored = "2.1"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `tabled` pour tableaux CLI, `indicatif` pour affichage interactif.

---

## Tests

```bash
cargo test -p vitte-help
```

- Tests de rendu CLI et alignement ANSI.  
- Tests d’export JSON et HTML.  
- Tests de cohérence avec `vitte-docgen`.  
- Vérification des alias et sous-commandes documentées.

---

## Roadmap

- [ ] Aide interactive avec recherche fuzzy.  
- [ ] Mode navigation `less` intégré.  
- [ ] Génération automatique de pages man Unix.  
- [ ] Intégration avec `vitte-studio` (aide contextuelle).  
- [ ] API HTTP locale pour aide dans l’éditeur.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau