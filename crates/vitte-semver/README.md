

# vitte-semver

Bibliothèque **de gestion des versions sémantiques (Semantic Versioning)** pour l’écosystème Vitte.  
`vitte-semver` implémente le standard [SemVer 2.0.0](https://semver.org/lang/fr/) et fournit des outils de comparaison, tri, validation et manipulation des numéros de version utilisés dans les paquets, modules et registres.

---

## Objectifs

- Offrir une **implémentation complète du standard SemVer 2.0.0**.  
- Permettre la comparaison, le parsing et la sérialisation des versions.  
- Gérer les plages (`>=1.2.0 <2.0.0`), préversions et builds métadonnées.  
- Intégration directe avec `vitte-package`, `vitte-registry` et `vitte-cli`.  
- Fournir une API stable pour les outils de packaging et publication.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `version`     | Représentation et parsing des numéros de version |
| `range`       | Gestion des contraintes et comparaisons de plages |
| `parse`       | Validation des chaînes de version SemVer |
| `compare`     | Tri et comparaison lexicographique ou numérique |
| `serialize`   | Conversion en chaînes normalisées |
| `tests`       | Vérification de conformité SemVer 2.0.0 |

---

## Exemple d’utilisation

```rust
use vitte_semver::{Version, VersionReq};

fn main() -> anyhow::Result<()> {
    let v1 = Version::parse("1.2.3-alpha.1+2025")?;
    let v2 = Version::parse("1.3.0")?;
    let req = VersionReq::parse(">=1.0, <2.0")?;

    println!("{} compatible avec contrainte : {}", v1, req.matches(&v1));
    println!("{} > {} ? {}", v2, v1, v2 > v1);
    Ok(())
}
```

---

## Intégration

- `vitte-package` : résolution des dépendances et compatibilité des versions.  
- `vitte-registry` : indexation et comparaison des paquets publiés.  
- `vitte-cli` : vérification de compatibilité via `vitte version check`.  
- `vitte-build` : sélection automatique de la version la plus stable.  
- `vitte-docgen` : génération de documentation versionnée.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

semver = "1.0"
regex = "1.10"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `chrono` pour datation des builds, `indexmap` pour caches de versions.

---

## Tests

```bash
cargo test -p vitte-semver
```

- Tests de conformité au standard SemVer 2.0.0.  
- Tests de comparaison et tri.  
- Tests de parsing des préversions et métadonnées.  
- Benchmarks sur gros ensembles de dépendances.

---

## Roadmap

- [ ] Support complet des formats Vitte spécifiques (`vX.Y.Z-preview`).  
- [ ] Vérification de compatibilité binaire et ABI.  
- [ ] Mode strict pour builds reproductibles.  
- [ ] Export des graphes de compatibilité dans `vitte-studio`.  
- [ ] Outil CLI pour vérification en masse des dépendances.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau