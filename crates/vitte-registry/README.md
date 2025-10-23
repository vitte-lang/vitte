# vitte-registry

Système **de registre et distribution de paquets** pour l’écosystème Vitte.  
`vitte-registry` gère l’hébergement, la publication et la résolution des modules et packages Vitte, utilisés par le compilateur, le gestionnaire de modules et les outils associés.

---

## Objectifs

- Offrir un **registre officiel pour les packages Vitte** (`registry.vitte.dev`).  
- Supporter la publication, la recherche et la synchronisation des paquets.  
- Gérer les métadonnées, signatures et dépendances de chaque module.  
- Fournir une API REST, CLI et Web compatible avec `vitte-package`.  
- Intégration complète avec `vitte-cli`, `vitte-build` et `vitte-key`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `server`      | Serveur HTTP principal du registre |
| `api`         | Endpoints REST pour publication et recherche |
| `storage`     | Gestion des fichiers de paquets (`.vitpkg`, `.vitmod`) |
| `index`       | Base d’indexation rapide des versions et dépendances |
| `auth`        | Authentification et gestion des tokens développeur |
| `sync`        | Synchronisation entre miroirs et registres distants |
| `tests`       | Tests de cohérence et intégration API |

---

## Exemple d’utilisation

### Publication d’un paquet

```bash
vitte package publish --registry https://registry.vitte.dev --token $VITTE_TOKEN
```

### Recherche

```bash
vitte registry search vitte-ast
```

### Téléchargement

```bash
vitte registry fetch vitte-ast@0.1.0
```

---

## Intégration

- `vitte-package` : publie et télécharge les paquets via le registre.  
- `vitte-cli` : commandes `vitte registry list`, `search`, `fetch`, `publish`.  
- `vitte-key` : signature et validation cryptographique des paquets.  
- `vitte-build` : résolution automatique des dépendances de registre.  
- `vitte-cache` : stockage local des paquets téléchargés.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-key = { path = "../vitte-key", version = "0.1.0" }

axum = "0.7"
tokio = { version = "1", features = ["full"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
reqwest = { version = "0.12", features = ["json", "stream"] }
tracing = "0.1"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `sled` pour indexation locale, `sha2` pour empreintes, `jsonwebtoken` pour auth JWT.

---

## Tests

```bash
cargo test -p vitte-registry
```

- Tests d’intégration API REST.  
- Tests de validation des signatures.  
- Tests de résolution et compatibilité des versions.  
- Benchmarks de performance sur publication et fetch.

---

## Roadmap

- [ ] Registre public officiel `registry.vitte.dev`.  
- [ ] Support complet des namespaces et organisations.  
- [ ] Mode miroir privé et offline.  
- [ ] Indexation incrémentale et recherche avancée.  
- [ ] Intégration visuelle avec `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
