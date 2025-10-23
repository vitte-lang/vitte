

# vitte-new

Outil **de création de projets et modules** pour l’écosystème Vitte.  
`vitte-new` initialise rapidement une arborescence de projet conforme aux conventions Vitte, incluant les fichiers de configuration, le manifeste et les modules de base.

---

## Objectifs

- Offrir un **générateur de projet rapide et cohérent**.  
- Supporter la création de crates, modules, bibliothèques et exécutables.  
- Générer automatiquement la structure standard (`src/`, `tests/`, `Cargo.toml`, `vitte.toml`).  
- Intégrer les modèles de base du compilateur et du runtime.  
- Faciliter l’amorçage de nouveaux projets open source Vitte.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `template`    | Gestion des modèles de projet et de fichiers |
| `init`        | Création et configuration initiale du projet |
| `manifest`    | Génération du fichier `vitte.toml` et `Cargo.toml` |
| `scaffold`    | Création des répertoires et fichiers par défaut |
| `detect`      | Détection du type de projet (lib, bin, workspace) |
| `cli`         | Commande `vitte new <nom>` et options associées |
| `tests`       | Tests de génération et validation de structure |

---

## Exemple d’utilisation

```bash
# Créer un nouveau projet exécutable
vitte new hello-world --bin

# Créer une bibliothèque
vitte new mylib --lib

# Créer un workspace complet
vitte new workspace --workspace
```

---

## Structure générée

```text
hello-world/
├── vitte.toml
├── Cargo.toml
├── src/
│   └── main.vitte
├── tests/
│   └── main_test.vitte
└── README.md
```

---

## Intégration

- `vitte-cli` : ajout automatique de la commande `vitte new`.  
- `vitte-build` : reconnaissance immédiate du manifeste généré.  
- `vitte-docgen` : génération automatique de la documentation initiale.  
- `vitte-lsp` : configuration initiale du workspace pour l’éditeur.  
- `vitte-cache` : initialisation du cache local du projet.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-cli = { path = "../vitte-cli", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
toml_edit = "0.22"
colored = "2.1"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `include_dir` pour embarquer des templates, `walkdir` pour inspection des répertoires.

---

## Tests

```bash
cargo test -p vitte-new
```

- Tests de génération de projet (lib, bin, workspace).  
- Vérification de cohérence des fichiers créés.  
- Tests de compatibilité avec `vitte-build` et `vitte-docgen`.  
- Benchmarks de performance de génération.

---

## Roadmap

- [ ] Support de templates personnalisés distants (Git).  
- [ ] Génération interactive avec `vitte-studio`.  
- [ ] Commande `vitte init` pour projets existants.  
- [ ] Intégration d’exemples et snippets par défaut.  
- [ ] Validation automatique des dépendances externes.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau