# vitte-errors

Gestionnaire **d’erreurs unifié** pour l’écosystème Vitte.

`vitte-errors` centralise la représentation, la propagation et le formatage des erreurs à travers tous les modules de Vitte. Il fournit des types structurés, des conversions automatiques (`From`, `Into`), et des intégrations avec `anyhow`, `thiserror` et `vitte-core`.

---

## Objectifs

- API d’erreurs commune à tout l’écosystème.
- Compatibilité avec les backends `anyhow` et `thiserror`.
- Gestion des traces de pile et codes d’erreurs unifiés.
- Formatage coloré et contextuel en CLI.
- Support des erreurs multi-langues et JSON.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `error`        | Définition des types d’erreurs (`VitteError`, `ErrorKind`) |
| `context`      | Ajout de contexte et traçabilité |
| `format`       | Rendu coloré, JSON et texte brut |
| `convert`      | Conversions automatiques depuis erreurs externes |
| `panic`        | Gestion des panics et récupération |
| `macros`       | Macros `bail!`, `ensure!`, `error!`, `context!` |
| `tests`        | Vérification des conversions et formats |

---

## Exemple d’utilisation

```rust
use vitte_errors::{VitteError, bail, ensure};

fn divide(a: i32, b: i32) -> Result<i32, VitteError> {
    ensure!(b != 0, "division par zéro");
    Ok(a / b)
}

fn main() -> Result<(), VitteError> {
    divide(10, 0)?;
    Ok(())
}
```

---

## Formatage CLI

Les erreurs peuvent être rendues colorées ou sérialisées :

```bash
vitte run script.vit --error-format=pretty
vitte run script.vit --error-format=json
```

Sortie JSON exemple :
```json
{
  "error": "division par zéro",
  "kind": "RuntimeError",
  "file": "main.vit",
  "line": 42
}
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
thiserror = "1.0"
anyhow = "1"
colored = "2"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
backtrace = "0.3"
```

---

## Tests

```bash
cargo test -p vitte-errors
```

- Vérification du formatage coloré.
- Tests des conversions depuis `io::Error`, `ParseIntError`, etc.
- Tests de rendu JSON et Pretty.

---

## Roadmap

- [ ] Codes d’erreurs standardisés entre modules.
- [ ] Support complet pour la sérialisation en Protobuf.
- [ ] Intégration avec `vitte-analyzer` pour les diagnostics IDE.
- [ ] Capture automatique des traces via `vitte-runtime`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
