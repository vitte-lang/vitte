# vitte-fs

Bibliothèque **de gestion du système de fichiers** pour l’écosystème Vitte.

`vitte-fs` fournit une API unifiée, sécurisée et multiplateforme pour la manipulation des fichiers, répertoires, liens symboliques et métadonnées. Il est utilisé par `vitte-compiler`, `vitte-docgen`, `vitte-runtime` et `vitte-tools` pour toutes les opérations d’I/O disque.

---

## Objectifs

- API de fichiers portable (macOS, Linux, Windows).
- Lecture/écriture asynchrone avec `vitte-async`.
- Gestion des permissions, métadonnées et verrous.
- Opérations récursives et observateurs de fichiers (watchers).
- Sécurité renforcée contre les chemins non autorisés.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `path`         | Normalisation, résolutions relatives et canoniques |
| `file`         | Lecture, écriture, suppression, append |
| `dir`          | Création, itération et suppression récursive |
| `meta`         | Métadonnées : taille, droits, horodatages |
| `watch`        | Observateur de changements (via `notify`) |
| `temp`         | Fichiers et répertoires temporaires sécurisés |
| `async`        | API asynchrone basée sur `tokio::fs` |
| `tests`        | Tests unitaires et d’intégration sur OS réels |

---

## Exemple d’utilisation

```rust
use vitte_fs::{read_to_string, write, create_dir_all};

fn main() -> anyhow::Result<()> {
    create_dir_all("target/output")?;
    write("target/output/hello.txt", "Bonjour Vitte!")?;
    let content = read_to_string("target/output/hello.txt")?;
    println!("{content}");
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : accès disque dans le runtime.
- `vitte-docgen` : génération et écriture de sites statiques.
- `vitte-project` : manipulation des workspaces et caches.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-async = { path = "../vitte-async", version = "0.1.0" }

tokio = { version = "1", features = ["fs", "macros"] }
notify = "6"
walkdir = "2.5"
tempfile = "3.10"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-fs
```

- Vérification de compatibilité multi-OS.
- Tests d’accès concurrent et verrouillage.
- Tests de watchers et suppression récursive.

---

## Roadmap

- [ ] API de snapshots et rollback.
- [ ] Compression intégrée (gzip/zstd).
- [ ] Observateur multiplateforme optimisé.
- [ ] Intégration avec `vitte-cache` et `vitte-project`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
