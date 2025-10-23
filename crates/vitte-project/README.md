

# vitte-project

Système **de gestion de projets et workspaces** pour l’écosystème Vitte.  
`vitte-project` centralise la détection, la configuration et la gestion des projets Vitte, incluant les dépendances, les chemins de build, et les métadonnées de workspace.

---

## Objectifs

- Offrir une **infrastructure standardisée de projet** pour tout l’écosystème.  
- Gérer les workspaces multi-crates et les projets isolés.  
- Détecter automatiquement la racine du projet et les fichiers manifestes (`vitte.toml`, `Cargo.toml`).  
- Intégration directe avec `vitte-build`, `vitte-lsp`, `vitte-docgen` et `vitte-cli`.  
- Support complet des environnements monorepo.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `manifest`    | Lecture et validation du manifeste `vitte.toml` |
| `workspace`   | Gestion des projets multiples et des chemins partagés |
| `config`      | Chargement et fusion des configurations globales |
| `resolver`    | Résolution des dépendances internes et externes |
| `env`         | Détection de l’environnement d’exécution (paths, toolchains) |
| `layout`      | Validation de la structure du projet et conventions |
| `tests`       | Vérification de cohérence et compatibilité cross-platform |

---

## Exemple d’utilisation

### Détection automatique du projet

```bash
vitte project info
```

### Création d’un nouveau workspace

```bash
vitte project init --workspace
```

### Inspection du manifeste

```bash
vitte project show-manifest
```

---

## Exemple de structure

```text
monorepo/
├── vitte.toml
├── crates/
│   ├── vitte-core/
│   ├── vitte-utils/
│   └── vitte-analyzer/
└── target/
```

---

## Intégration

- `vitte-build` : résolution des chemins de build et artefacts.  
- `vitte-lsp` : détection automatique du projet ouvert.  
- `vitte-docgen` : récupération des métadonnées pour la documentation.  
- `vitte-cache` : gestion du cache de workspace.  
- `vitte-cli` : commandes globales `vitte project`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
toml = "0.8"
walkdir = "2.5"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `ignore` pour compatibilité `.gitignore`, `dirs` pour gestion des chemins utilisateur.

---

## Tests

```bash
cargo test -p vitte-project
```

- Tests de détection de la racine du projet.  
- Tests de parsing du manifeste.  
- Tests de compatibilité multi-workspaces.  
- Benchmarks sur la résolution des dépendances.

---

## Roadmap

- [ ] Support du manifest `vitte.lock` pour dépendances figées.  
- [ ] Validation croisée avec `Cargo.toml` et `package.json`.  
- [ ] Mode projet distant (Git/HTTP).  
- [ ] Gestion de workspaces distribués.  
- [ ] Visualisation dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau