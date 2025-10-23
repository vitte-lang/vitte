

# vitte-package

Système **de packaging et distribution** pour l’écosystème Vitte.  
`vitte-package` gère la création, la signature et la publication des paquets binaires, sources ou modules (`.vitpkg`, `.vitmod`) utilisés dans l’environnement Vitte.

---

## Objectifs

- Fournir un **gestionnaire de paquets modulaire et sécurisé**.  
- Gérer la création, installation et mise à jour des paquets.  
- Supporter les formats `.vitpkg` (binaire) et `.vitmod` (source).  
- Intégration avec `vitte-build`, `vitte-install`, `vitte-key` et `vitte-cli`.  
- Vérification cryptographique via signatures (`vitte-key`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `manifest`    | Lecture et validation des fichiers `vitte.toml` |
| `builder`     | Construction et empaquetage des artefacts |
| `registry`    | Gestion du registre local et distant |
| `publisher`   | Publication sur les dépôts distants |
| `sign`        | Signature et vérification cryptographique |
| `extract`     | Extraction et installation des paquets |
| `cache`       | Mise en cache locale des paquets téléchargés |
| `tests`       | Tests fonctionnels et d’intégration |

---

## Exemple d’utilisation

### Création d’un paquet

```bash
vitte package build --manifest vitte.toml --out target/
```

### Publication

```bash
vitte package publish --token $VITTE_TOKEN --registry https://registry.vitte.dev
```

### Installation

```bash
vitte package install roussov/vitte-math
```

---

## Intégration

- `vitte-build` : empaquetage automatique des binaires.  
- `vitte-install` : extraction et installation locale.  
- `vitte-key` : signature et validation des paquets.  
- `vitte-cli` : commande `vitte package` avec sous-commandes (`build`, `publish`, `install`, `verify`).  
- `vitte-cache` : stockage local et invalidation des artefacts téléchargés.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-key = { path = "../vitte-key", version = "0.1.0" }
vitte-cli = { path = "../vitte-cli", version = "0.1.0" }

tar = "0.4"
flate2 = "1.0"
reqwest = { version = "0.12", features = ["json", "stream"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `zstd` pour compression avancée, `sha2` pour intégrité, `indicatif` pour affichage de progression.

---

## Tests

```bash
cargo test -p vitte-package
```

- Tests de création et extraction de paquets.  
- Tests de vérification de signature.  
- Tests d’installation multi-version.  
- Benchmarks de vitesse de compression/décompression.

---

## Roadmap

- [ ] Support du dépôt global `packages.vitte.dev`.  
- [ ] Gestion des dépendances transitives.  
- [ ] Compression adaptative (ZSTD/LZ4).  
- [ ] Intégration CI/CD pour publication automatique.  
- [ ] Support des paquets delta et différés.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau