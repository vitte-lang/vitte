

# vitte-config

Système **de configuration centralisée** pour l’écosystème Vitte.  
`vitte-config` gère la lecture, la validation et la fusion des fichiers de configuration, variables d’environnement et options CLI.

---

## Objectifs

- Fournir un modèle de configuration **unifié, typé et hiérarchique**.  
- Supporter les formats **TOML**, **YAML**, **JSON** et fichiers `.env`.  
- Fusionner proprement les configurations locales, globales et projet.  
- Permettre la surcharge via **variables d’environnement** ou **arguments CLI**.  
- Validation stricte avec schémas typés et diagnostics précis.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `model`       | Structures de données et schémas typés |
| `loader`      | Chargement des fichiers et parsing multi-format |
| `env`         | Extraction des variables d’environnement |
| `merge`       | Fusion hiérarchique des configurations |
| `validate`    | Vérification des contraintes et valeurs obligatoires |
| `defaults`    | Valeurs par défaut et profils prédéfinis |
| `cli`         | Injection des options passées via CLI |
| `tests`       | Tests de validation et scénarios multi-sources |

---

## Exemple d’utilisation

```rust
use vitte_config::Config;

fn main() -> anyhow::Result<()> {
    let cfg = Config::load_from("vitte.toml")?
        .with_env_prefix("VITTE")?
        .merge_cli(std::env::args())?
        .validate()?;

    println!("Mode: {:?}", cfg.build.mode);
    Ok(())
}
```

---

## Exemple de fichier `vitte.toml`

```toml
[build]
mode = "release"
threads = 4

[cache]
path = "~/.cache/vitte"
compress = true

[compiler]
target = "x86_64-unknown-linux-gnu"
opt_level = 3
```

---

## Intégration

- `vitte-build` : paramètres de compilation et cache.  
- `vitte-cli` : lecture et surcharge CLI.  
- `vitte-compiler` : choix des cibles et optimisations.  
- `vitte-docgen` : configuration du rendu et thèmes.  
- `vitte-lsp` : initialisation du serveur selon profil utilisateur.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
toml = "0.8"
yaml-rust = "0.4"
dotenvy = "0.15"
thiserror = "1"
anyhow = "1"
``` 

> Optionnel : `schemars` pour validation par schéma JSON.

---

## Tests

```bash
cargo test -p vitte-config
```

- Tests de fusion multi-format.  
- Tests de validation et schémas.  
- Tests d’intégration CLI/env.  
- Vérification des profils par défaut.

---

## Roadmap

- [ ] Validation dynamique via JSON Schema.  
- [ ] Hot-reload des fichiers de configuration.  
- [ ] Intégration avec `vitte-lsp` pour suggestions de clés.  
- [ ] Gestion des profils utilisateurs multiples.  
- [ ] Encryption partielle des sections sensibles.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau