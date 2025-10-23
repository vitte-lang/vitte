# vitte-cache

Système **de cache global et incrémental** pour l’écosystème Vitte.  
`vitte-cache` gère le stockage, la récupération et l’invalidation des données compilées, artefacts binaires, métadonnées et analyses.  
Il est utilisé par `vitte-build`, `vitte-compiler`, `vitte-analyzer`, et `vitte-docgen` pour accélérer la compilation et la génération documentaire.

---

## Objectifs

- Caching multi-niveau (mémoire, disque, réseau).  
- Invalidation automatique basée sur contenu (`hash`, `mtime`, `deps`).  
- Support du cache incrémental par module ou fonction.  
- Compression et sérialisation optimisée des artefacts.  
- API unifiée pour lecture/écriture concurrente.  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `store`        | Interface générique de backend (filesystem, mémoire, HTTP) |
| `entry`        | Représentation des éléments de cache (clé, valeur, métadonnées) |
| `hash`         | Calcul de hash (BLAKE3, xxhash, CRC) |
| `policy`       | Stratégies d’expiration et de nettoyage |
| `serialize`    | Encodage/décodage rapide des artefacts |
| `async`        | Gestion asynchrone des lectures/écritures |
| `tests`        | Tests de cohérence et performance |

---

## Exemple d’utilisation

```rust
use vitte_cache::Cache;

fn main() -> anyhow::Result<()> {
    let cache = Cache::new("target/.vitte-cache")?;
    let key = "parser::symbols";
    if let Some(data) = cache.get(key)? {
        println!("Résultat trouvé dans le cache !");
    } else {
        let result = compute_symbols();
        cache.put(key, &result)?;
    }
    Ok(())
}
```

---

## Intégration

- `vitte-build` : stockage des artefacts compilés.  
- `vitte-analyzer` : cache des graphes sémantiques.  
- `vitte-docgen` : cache du rendu HTML/JSON.  
- `vitte-lsp` : cache mémoire pour l’indexation rapide.  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

blake3 = "1.5"
xxhash-rust = "0.8"
lz4_flex = "1.6"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tokio = { version = "1", features = ["full"] }
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-cache
```

- Tests de cohérence et invalidation.  
- Tests de performance sur grands ensembles.  
- Vérification des politiques de rétention.  

---

## Roadmap

- [ ] Backend réseau (Redis, HTTP, S3).  
- [ ] Compression adaptative LZ4/ZSTD.  
- [ ] Synchronisation incrémentale entre builds.  
- [ ] Cache partagé entre workspaces.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
