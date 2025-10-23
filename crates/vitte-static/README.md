

# vitte-static

Système **de gestion des ressources statiques et intégration embarquée** pour l’écosystème Vitte.  
`vitte-static` permet l’inclusion, la compression et la mise à disposition de fichiers statiques (HTML, CSS, images, templates, binaires) au sein des binaires et modules Vitte.

---

## Objectifs

- Offrir un **système fiable et performant** pour embarquer des ressources statiques.  
- Supporter la compression et la mise en cache automatique.  
- Intégration directe avec `vitte-docgen`, `vitte-web`, `vitte-assets` et `vitte-cli`.  
- Fournir une API simple pour charger, servir et mettre à jour les fichiers statiques.  
- Support du mode embarqué (`--embed`) et du mode externe (`--serve`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `embed`       | Inclusion des fichiers au moment de la compilation |
| `serve`       | Serveur HTTP statique pour développement et documentation |
| `compress`    | Compression gzip/zstd et cache de ressources |
| `fs`          | Gestion des chemins et vérification d’intégrité |
| `index`       | Génération automatique d’index et métadonnées |
| `tests`       | Vérification des intégrations et performances |

---

## Exemple d’utilisation

### Embedding à la compilation

```rust
use vitte_static::StaticFile;

fn main() -> anyhow::Result<()> {
    let file = StaticFile::embed!("assets/logo.png");
    println!("Nom : {} — Taille : {} octets", file.name(), file.data().len());
    Ok(())
}
```

### Mode serveur

```bash
vitte static serve ./assets --port 8080
```

---

## Intégration

- `vitte-docgen` : intégration des fichiers HTML/CSS générés.  
- `vitte-web` : diffusion des ressources Web embarquées.  
- `vitte-assets` : stockage des fichiers binaires et icônes.  
- `vitte-cli` : option `--embed` pour inclure les ressources dans les binaires.  
- `vitte-build` : génération du bundle complet avec cache.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

flate2 = "1.0"
zstd = "0.13"
walkdir = "2.5"
include_dir = "0.7"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `notify` pour watcher de fichiers, `tiny_http` pour serveur local, `sha2` pour validation d’intégrité.

---

## Tests

```bash
cargo test -p vitte-static
```

- Tests d’intégrité et d’embedding.  
- Tests de performances d’accès et de compression.  
- Tests d’intégration avec `vitte-docgen` et `vitte-web`.  
- Benchmarks de lecture embarquée vs externe.

---

## Roadmap

- [ ] Support du hot reload pour les ressources statiques.  
- [ ] Compression adaptative selon le contexte (LZ4, Brotli, Zstd).  
- [ ] Indexation avancée des ressources.  
- [ ] Visualisation des assets dans `vitte-studio`.  
- [ ] Support des fichiers multimédias et shaders.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau