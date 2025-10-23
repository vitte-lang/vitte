# vitte-buildkit

Ensemble **d’outils avancés de construction** pour le compilateur et l’écosystème Vitte.

`vitte-buildkit` fournit les utilitaires, extensions et pipelines nécessaires à la mise en œuvre du système de build modulaire de Vitte.  
Il complète `vitte-build` en offrant des fonctionnalités spécialisées comme le packaging, la cross-compilation, le caching distribué et la gestion des artefacts binaires.

---

## Objectifs

- Étendre le moteur `vitte-build` avec des outils spécialisés.  
- Gérer le packaging multi-plateforme et les distributions.  
- Fournir des commandes CLI pour les builds complexes.  
- Offrir une API pour automatiser les workflows CI/CD.  
- Intégrer la détection d’environnement et de cibles.  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `package`      | Création et signature de paquets binaires |
| `target`       | Détection et configuration des plateformes (macOS, Linux, Windows, WASM) |
| `toolchain`    | Gestion des toolchains LLVM, Cranelift, GCC, etc. |
| `cache`        | Caching distribué (local / réseau) |
| `artifact`     | Compression, archivage et validation d’artefacts |
| `cli`          | Commandes `vitte buildkit package`, `cross`, `export` |
| `tests`        | Tests d’intégration sur divers environnements |

---

## Exemple d’utilisation

```bash
vitte buildkit package --target x86_64-linux --out dist/
vitte buildkit cross --target wasm64
vitte buildkit export --format tar.gz
```

Depuis Rust :
```rust
use vitte_buildkit::BuildKit;

fn main() -> anyhow::Result<()> {
    let kit = BuildKit::new()
        .target("aarch64-apple-darwin")
        .package("vitte-compiler");
    kit.build_and_export("dist/")?;
    Ok(())
}
```

---

## Intégration

- `vitte-build` : moteur de compilation principal.  
- `vitte-cli` : commandes utilisateur.  
- `vitte-backends` : génération multi-architecture.  
- `vitte-tools` : scripts d’automatisation.  
- `vitte-runtime` : empaquetage du runtime et des dépendances natives.  

---

## Dépendances

```toml
[dependencies]
vitte-core     = { path = "../vitte-core", version = "0.1.0" }
vitte-build    = { path = "../vitte-build", version = "0.1.0" }
vitte-utils    = { path = "../vitte-utils", version = "0.1.0" }

tar = "0.4"
flate2 = "1.0"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
colored = "2"
which = "6"
```

---

## Tests

```bash
cargo test -p vitte-buildkit
```

- Vérification des builds cross-platform.  
- Tests de packaging et de compression.  
- Validation de la signature et des métadonnées.  

---

## Roadmap

- [ ] Support natif pour `.deb`, `.rpm`, `.pkg`, `.msi`.  
- [ ] Builds distribués en cluster.  
- [ ] Intégration avec `vitte-bench` pour mesures de build.  
- [ ] Interface graphique de monitoring des builds.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
