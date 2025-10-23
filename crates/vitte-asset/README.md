# vitte-asset

Système **de gestion unifiée des ressources** pour l’écosystème Vitte.

`vitte-asset` gère le chargement, la mise en cache, la recherche et l’accès aux ressources logiques (fichiers, archives, mémoire) de manière portable, performante et extensible. Il constitue la couche centrale de gestion des assets utilisée par `vitte-docgen`, `vitte-site`, `vitte-compiler` et `vitte-runtime`.

---

## Objectifs

- Formats supportés : fichiers bruts, répertoires, archives `.vpak`.
- Résolution d’URI : `asset://`, `file://`, `mem://`, `http://`.
- Indexation et résolution multi-sources.
- Cache mémoire avec zéro-copie (`mmap`, `Arc<[u8]>`).
- Validation d’intégrité via hash, taille, version et timestamp.
- Support complet sync/async.
- Variantes : build-type (debug/release), localisation, densité (1x, 2x).

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `uri`          | Parsing et normalisation d’URI d’asset |
| `manifest`     | Définition des manifestes, tags et dépendances |
| `index`        | Index global des ressources et stratégies de recherche |
| `provider`     | Backends (fichiers, mémoire, archives `.vpak`, HTTP) |
| `cache`        | LRU, mmap et stratégies d’éviction adaptatives |
| `blob`         | Vue binaire avec slices et accès mémoire direct |
| `integrity`    | Calculs de hash (xxhash, blake3) et vérifications |
| `cli`          | Commandes CLI : `pack`, `list`, `verify` |
| `tests`        | Validation des backends et tests d’intégrité |

---

## Exemple d’utilisation

```rust
use vitte_asset::{AssetManager, AssetUri};

fn main() -> anyhow::Result<()> {
    let mut assets = AssetManager::default();
    assets.add_provider("file://assets");
    let uri = AssetUri::parse("asset://images/logo.png")?;
    let data = assets.load(&uri)?;
    println!("{} octets chargés", data.len());
    Ok(())
}
```

---

## CLI

```bash
vitte asset pack ./assets --out app.vpak
vitte asset list app.vpak
vitte asset verify app.vpak
```

---

## Intégration

- `vitte-docgen` : chargement de thèmes et templates.
- `vitte-site` : publication statique et empaquetage des ressources.
- `vitte-runtime` : chargement dynamique d’assets embarqués.
- `vitte-build` : génération et validation de manifestes.
- `vitte-tools` : inspection et vérification d’intégrité.

---

## Dépendances

```toml
[dependencies]
vitte-core        = { path = "../vitte-core", version = "0.1.0" }
vitte-utils       = { path = "../vitte-utils", version = "0.1.0" }
vitte-fs          = { path = "../vitte-fs", version = "0.1.0", optional = true }
vitte-http        = { path = "../vitte-http", version = "0.1.0", optional = true }
vitte-diagnostics = { path = "../vitte-diagnostics", version = "0.1.0" }

xxhash-rust = "0.8"
blake3 = "1.5"
lru = "0.12"
memmap2 = "0.9"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-asset
```

- Vérification des URIs et index multi-sources.
- Tests de cache et mmap.
- Validation des manifestes et intégrité.
- Benchmarks sur accès mémoire.

---

## Roadmap

- [ ] Support des bundles incrémentaux `.vpakdiff`.
- [ ] Compression transparente (zstd, gzip).
- [ ] HTTP range requests pour assets distants.
- [ ] Indexation hiérarchique et préchargement intelligent.
- [ ] API de streaming et hot-reload pour éditeurs.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
