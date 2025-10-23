

# vitte-packager

Outil **de création, validation et distribution de paquets** pour l’écosystème Vitte.  
`vitte-packager` automatise le processus d’empaquetage, de signature et de publication des projets et modules Vitte pour la distribution publique ou privée.

---

## Objectifs

- Offrir un **outil de packaging complet** pour les développeurs Vitte.  
- Créer des paquets binaires (`.vitpkg`) et sources (`.vitmod`).  
- Signer les paquets via `vitte-key` pour garantir l’intégrité et l’authenticité.  
- Gérer la publication vers les registres distants (HTTP, Git, S3).  
- Intégration directe avec `vitte-build`, `vitte-install` et `vitte-cli`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `builder`     | Construction et empaquetage du contenu du projet |
| `manifest`    | Lecture du manifeste `vitte.toml` et extraction des métadonnées |
| `compress`    | Compression et archivage des fichiers (TAR/LZ4/ZSTD) |
| `sign`        | Signature et validation des paquets via `vitte-key` |
| `publish`     | Envoi des paquets vers les registres distants |
| `verify`      | Vérification des signatures et intégrité après extraction |
| `cli`         | Interface en ligne de commande `vitte packager` |
| `tests`       | Tests d’intégration et vérification de cohérence |

---

## Exemple d’utilisation

### Création d’un paquet

```bash
vitte packager build --manifest vitte.toml --out target/
```

### Signature et validation

```bash
vitte packager sign target/vitte-0.1.0.vitpkg --key ~/.vitte/keys/dev.json
vitte packager verify target/vitte-0.1.0.vitpkg
```

### Publication

```bash
vitte packager publish --registry https://registry.vitte.dev --token $VITTE_TOKEN
```

---

## Intégration

- `vitte-build` : export des artefacts compilés vers le packager.  
- `vitte-key` : signature des paquets et vérification cryptographique.  
- `vitte-cli` : sous-commandes `vitte packager build`, `sign`, `publish`, `verify`.  
- `vitte-install` : installation automatique à partir de paquets `.vitpkg`.  
- `vitte-cache` : stockage local des artefacts signés.

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
lz4_flex = "1.6"
zstd = "0.13"
reqwest = { version = "0.12", features = ["json", "stream"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `indicatif` pour barres de progression, `sha2` pour hachage, `base64` pour signatures.

---

## Tests

```bash
cargo test -p vitte-packager
```

- Tests de création et extraction de paquets.  
- Tests de signature et validation d’intégrité.  
- Tests de compatibilité multi-versions.  
- Benchmarks de compression et upload.

---

## Roadmap

- [ ] Support du registre officiel `packages.vitte.dev`.  
- [ ] Paquets delta pour mises à jour rapides.  
- [ ] Déploiement multi-architecture (x86_64, arm64, wasm).  
- [ ] Publication via GitHub Releases et CI.  
- [ ] Intégration visuelle avec `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau