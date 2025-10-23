

# vitte-serde

Système **de sérialisation et désérialisation unifiée** pour le langage Vitte.  
`vitte-serde` fournit une API commune pour convertir les structures internes Vitte vers et depuis plusieurs formats (JSON, TOML, YAML, CBOR, MessagePack, etc.).

---

## Objectifs

- Offrir une **infrastructure de sérialisation performante et portable**.  
- Supporter les formats textuels et binaires les plus courants.  
- Intégration directe avec `vitte-core`, `vitte-runtime`, `vitte-analyzer` et `vitte-docgen`.  
- Faciliter la persistence et le transfert des données entre modules.  
- Maintenir la compatibilité avec `serde` Rust et les formats standardisés.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `json`        | Conversion vers/depuis JSON (serde_json) |
| `toml`        | Gestion des formats de configuration |
| `yaml`        | Support YAML (via `serde_yaml`) |
| `binary`      | Encodage binaire compact (CBOR, MsgPack) |
| `traits`      | Définition des traits de sérialisation/désérialisation |
| `derive`      | Macros derive pour types Vitte |
| `tests`       | Tests de compatibilité et cohérence entre formats |

---

## Exemple d’utilisation

```rust
use vitte_serde::{to_json, from_json};

#[derive(Serialize, Deserialize, Debug, PartialEq)]
struct Config {
    project: String,
    version: String,
}

fn main() -> anyhow::Result<()> {
    let cfg = Config { project: "Vitte".into(), version: "0.1.0".into() };
    let json = to_json(&cfg)?;
    let parsed: Config = from_json(&json)?;
    assert_eq!(cfg, parsed);
    Ok(())
}
```

---

## Intégration

- `vitte-core` : structures et définitions fondamentales.  
- `vitte-analyzer` : exportation des graphes et résultats sous JSON.  
- `vitte-docgen` : sérialisation des documents et métadonnées.  
- `vitte-runtime` : communication inter-modules et persistence.  
- `vitte-cli` : lecture/écriture de fichiers de configuration.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
serde_yaml = "0.9"
toml = "0.8"
ciborium = "0.2"
rmp-serde = "1.3"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `bincode` pour compression maximale, `miniserde` pour builds légers.

---

## Tests

```bash
cargo test -p vitte-serde
```

- Tests croisés entre formats.  
- Tests de performance (taille et vitesse de sérialisation).  
- Tests de compatibilité entre versions du schéma.  
- Tests de cohérence pour les structures dérivées automatiquement.

---

## Roadmap

- [ ] Support des formats Protobuf et Avro.  
- [ ] Optimisation SIMD pour JSON et CBOR.  
- [ ] Vérification automatique des schémas entre builds.  
- [ ] Intégration directe avec `vitte-build` et `vitte-studio`.  
- [ ] Conversion JSON → Vitte AST native.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau