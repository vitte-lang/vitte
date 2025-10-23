

# vitte-man

Système **de documentation manuelle et pages d’aide** pour l’écosystème Vitte.  
`vitte-man` génère et gère les pages de documentation de type *man*, ainsi que les aides contextuelles intégrées aux outils CLI (`vitte`, `vitte-build`, `vitte-docgen`, etc.).

---

## Objectifs

- Offrir un **système d’aide intégré** accessible depuis la CLI.  
- Générer automatiquement les pages *man* à partir des métadonnées des commandes.  
- Supporter plusieurs formats de sortie : man(1), Markdown, HTML, JSON.  
- Intégration complète avec `vitte-docgen` et `vitte-cli`.  
- Gestion multilingue et versionnée de la documentation.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `parser`      | Lecture des métadonnées de commandes et options |
| `render`      | Génération des pages man, Markdown et HTML |
| `registry`    | Index des pages disponibles et versions associées |
| `cache`       | Stockage local des pages compilées |
| `cli`         | Interface `vitte man <cmd>` pour affichage direct |
| `tests`       | Vérification du rendu et cohérence des métadonnées |

---

## Exemple d’utilisation

```bash
# Afficher la documentation de vitte build
vitte man build

# Générer toutes les pages man locales
vitte man --generate

# Exporter la documentation au format Markdown
vitte man export --format md --output docs/
```

---

## Intégration

- `vitte-cli` : génération et affichage des aides de sous-commandes.  
- `vitte-docgen` : export de documentation manuelle dans le site.  
- `vitte-build` : génération automatique des aides liées aux flags.  
- `vitte-lsp` : intégration du contenu dans les tooltips contextuels.  
- `vitte-cache` : stockage et invalidation du rendu compilé.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-cli = { path = "../vitte-cli", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tera = "1.20"
chrono = "0.4"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `pulldown-cmark` pour export Markdown, `man` pour rendu local.

---

## Tests

```bash
cargo test -p vitte-man
```

- Tests de génération et rendu man/HTML.  
- Tests de cohérence des métadonnées CLI.  
- Tests d’intégration avec `vitte-docgen`.  
- Vérification des traductions multilingues.

---

## Roadmap

- [ ] Support complet des sous-commandes dynamiques.  
- [ ] Indexation multiversion.  
- [ ] Export PDF via `vitte-docgen`.  
- [ ] Génération automatique depuis les attributs de code Rust.  
- [ ] Recherche en ligne intégrée via `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau