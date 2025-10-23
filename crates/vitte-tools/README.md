

# vitte-tools

Ensemble **d’outils utilitaires et de développement** pour l’écosystème Vitte.  
`vitte-tools` regroupe les binaires, scripts et modules internes servant à automatiser les tâches de compilation, test, packaging et maintenance du langage.

---

## Objectifs

- Offrir une **boîte à outils complète pour les développeurs Vitte**.  
- Centraliser les utilitaires partagés entre `vitte-cli`, `vitte-build`, `vitte-docgen` et `vitte-analyzer`.  
- Automatiser les flux de travail (build, test, release, inspection).  
- Fournir des outils extensibles et composables.  
- Faciliter l’intégration CI/CD et la maintenance multi-crates.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `cmd`         | Définition et exécution des sous-commandes outils |
| `inspect`     | Lecture et analyse des fichiers binaires `.vitbc` |
| `package`     | Création, vérification et signature des paquets |
| `release`     | Automatisation du processus de versionnement |
| `lint`        | Vérification des conventions de code et dépendances |
| `bench`       | Outils de benchmark et profilage automatisés |
| `tests`       | Vérification de cohérence et validation croisée |

---

## Exemple d’utilisation

```bash
vitte tools inspect main.vitbc --json
vitte tools package --out dist/
vitte tools release --bump minor
```

### Exemple Rust

```rust
use vitte_tools::inspect;

fn main() -> anyhow::Result<()> {
    let info = inspect::analyze_file("target/main.vitbc")?;
    println!("{}", serde_json::to_string_pretty(&info)?);
    Ok(())
}
```

---

## Intégration

- `vitte-cli` : exécution des commandes utilisateur.  
- `vitte-build` : intégration des outils de packaging et versionnement.  
- `vitte-analyzer` : outils d’inspection binaire et symbolique.  
- `vitte-docgen` : génération de documentation automatisée.  
- `vitte-bench` : exécution et mesure des performances compilateur/runtime.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
toml = "0.8"
regex = "1.10"
chrono = "0.4"
semver = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `clap` pour gestion CLI, `indicatif` pour progression, `git2` pour intégration Git.

---

## Tests

```bash
cargo test -p vitte-tools
```

- Tests d’intégration sur packaging et inspection.  
- Tests de performance sur parsing binaire.  
- Tests de cohérence du workflow `release`.  
- Vérification des conventions de versionnement et de métadonnées.

---

## Roadmap

- [ ] Automatisation complète du processus de publication crates.io.  
- [ ] Intégration GitHub Actions et pipelines CI.  
- [ ] Visualisation des métadonnées binaires dans `vitte-studio`.  
- [ ] Génération automatique des changelogs.  
- [ ] Vérification de signature GPG des paquets.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau