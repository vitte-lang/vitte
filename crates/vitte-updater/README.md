


# vitte-updater

Système **de mise à jour automatique et vérifiée** pour l’écosystème Vitte.  
`vitte-updater` gère la détection, le téléchargement, la vérification et l’installation des nouvelles versions des outils et composants Vitte (CLI, runtime, std, etc.).

---

## Objectifs

- Offrir une **solution de mise à jour transparente et sécurisée**.  
- Supporter les mises à jour manuelles, automatiques et programmées.  
- Vérifier les signatures et les checksums avant installation.  
- Intégration directe avec `vitte-cli`, `vitte-tools`, `vitte-build` et `vitte-studio`.  
- Support multi-plateforme (macOS, Linux, Windows).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `check`       | Détection des nouvelles versions disponibles |
| `download`    | Téléchargement des paquets binaires et sources |
| `verify`      | Vérification de l’intégrité et de la signature GPG |
| `install`     | Décompression et installation atomique |
| `rollback`    | Gestion des restaurations en cas d’erreur |
| `tests`       | Tests de cohérence et de sécurité |

---

## Exemple d’utilisation

### CLI

```bash
vitte update --check
vitte update --install latest
vitte update --rollback
```

### Exemple Rust

```rust
use vitte_updater::Updater;

fn main() -> anyhow::Result<()> {
    let mut updater = Updater::new("https://api.github.com/repos/vitte-lang/vitte/releases/latest");
    updater.check()?;
    updater.download_and_install()?;
    Ok(())
}
```

---

## Intégration

- `vitte-cli` : commande `vitte update` pour utilisateurs finaux.  
- `vitte-tools` : vérification et automatisation des mises à jour de dépendances.  
- `vitte-build` : synchronisation des versions du compilateur.  
- `vitte-studio` : notification visuelle des nouvelles versions.  
- `vitte-installer` : intégration dans les installeurs natifs.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

reqwest = { version = "0.12", features = ["json", "blocking"] }
tokio = { version = "1", features = ["full"] }
sha2 = "0.10"
flate2 = "1.0"
tar = "0.4"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `gpgme` pour vérification de signature, `zip` pour formats alternatifs, `chrono` pour planification.

---

## Tests

```bash
cargo test -p vitte-updater
```

- Tests de détection de version et parsing JSON.  
- Tests de vérification de checksum et signature.  
- Tests d’installation et rollback.  
- Benchmarks sur performance réseau et décompression.

---

## Roadmap

- [ ] Support des canaux bêta et nightly.  
- [ ] Téléchargements différés et delta updates.  
- [ ] Mises à jour planifiées (cron, tâche système).  
- [ ] Intégration complète avec `vitte-cloud`.  
- [ ] Interface graphique dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau