

# vitte-path

Bibliothèque **de gestion des chemins, fichiers et répertoires** pour l’écosystème Vitte.  
`vitte-path` fournit des utilitaires robustes, multiplateformes et performants pour manipuler les chemins, les URLs, les environnements de fichiers et les accès symboliques.

---

## Objectifs

- Offrir une **API unifiée et sûre** pour la gestion des chemins et fichiers.  
- Supporter toutes les plateformes (Unix, macOS, Windows).  
- Gérer les chemins relatifs, absolus, canoniques et virtuels.  
- Fournir une intégration directe avec `vitte-build`, `vitte-cache` et `vitte-docgen`.  
- Inclure des fonctions d’inspection, de normalisation et de sécurité.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `path`        | Manipulation et concaténation de chemins |
| `fs`          | Opérations sur le système de fichiers (création, copie, suppression) |
| `normalize`   | Canonicalisation et sécurisation des chemins |
| `env`         | Détection des variables d’environnement et répertoires standards |
| `url`         | Conversion entre chemins locaux et URLs |
| `search`      | Résolution et recherche récursive de fichiers |
| `watch`       | Surveillance de fichiers (changement, suppression, ajout) |
| `tests`       | Tests de portabilité et cohérence entre OS |

---

## Exemple d’utilisation

```rust
use vitte_path::{PathExt, normalize};

fn main() -> anyhow::Result<()> {
    let path = "./src/../Cargo.toml";
    let normalized = normalize(path)?;
    println!("Chemin canonique : {}", normalized.display());
    Ok(())
}
```

---

## Intégration

- `vitte-build` : résolution et canonicalisation des chemins de projet.  
- `vitte-cache` : gestion des répertoires de cache compilé.  
- `vitte-docgen` : génération des chemins de sortie pour la documentation.  
- `vitte-cli` : expansion des chemins passés en arguments.  
- `vitte-lsp` : localisation des fichiers sources et imports relatifs.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

pathdiff = "0.2"
walkdir = "2.5"
notify = "6.1"
url = "2.5"
dirs = "5.0"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `ignore` pour gestion des `.gitignore`, `glob` pour recherche par motif.

---

## Tests

```bash
cargo test -p vitte-path
```

- Tests de portabilité Windows/macOS/Linux.  
- Tests de résolution et normalisation des chemins relatifs.  
- Tests de détection des fichiers cachés et symboliques.  
- Tests d’intégration avec `vitte-build` et `vitte-cache`.

---

## Roadmap

- [ ] Support des liens symboliques sécurisés.  
- [ ] Optimisation de la surveillance de fichiers (`watch`).  
- [ ] Support complet des chemins virtuels (`vfs://`).  
- [ ] API pour sandbox sécurisée (`vitte-sandbox`).  
- [ ] Intégration avec `vitte-studio` pour navigation de projets.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau