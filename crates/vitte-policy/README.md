

# vitte-policy

Système **de gestion et validation des politiques de sécurité, compilation et exécution** pour l’écosystème Vitte.  
`vitte-policy` définit les règles, permissions et contraintes appliquées lors du build, de l’exécution et de la publication des projets Vitte.

---

## Objectifs

- Offrir un **moteur de politiques configurable et extensible**.  
- Définir des règles de sécurité, d’accès, de dépendances et de signature.  
- Intégration directe avec `vitte-compiler`, `vitte-runtime` et `vitte-key`.  
- Validation statique des politiques à la compilation et dynamique à l’exécution.  
- Support des politiques locales, projet et globales.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `rules`       | Définition des politiques (compilation, runtime, publication) |
| `loader`      | Chargement des fichiers de politique (`policy.toml`, `policy.json`) |
| `validator`   | Application et vérification des contraintes |
| `enforce`     | Exécution des politiques en runtime |
| `sign`        | Vérification des signatures et empreintes cryptographiques |
| `context`     | Gestion du contexte d’application (workspace, global, user) |
| `tests`       | Tests d’intégration et cohérence des politiques |

---

## Exemple d’utilisation

### Déclaration d’une politique de compilation

```toml
# vitte-policy.toml
[build]
allow_network = false
allow_unsafe  = false
require_signatures = true
trusted_keys = ["roussov@vitte.dev"]
```

### Application de la politique

```bash
vitte build --policy vitte-policy.toml
```

### Exemple d’exécution avec politique runtime

```bash
vitte run --policy ~/.config/vitte/policy-runtime.toml
```

---

## Intégration

- `vitte-compiler` : vérifie les autorisations d’accès au réseau et aux fichiers.  
- `vitte-runtime` : applique les politiques de sécurité au moment de l’exécution.  
- `vitte-key` : valide les signatures des modules et artefacts.  
- `vitte-package` : vérifie les règles de publication et les licences.  
- `vitte-cli` : expose la commande `vitte policy` (lint, validate, sign, apply).

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-key = { path = "../vitte-key", version = "0.1.0" }

toml = "0.8"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `jsonschema` pour validation avancée, `chrono` pour dates d’expiration.

---

## Tests

```bash
cargo test -p vitte-policy
```

- Tests de validation de politiques complexes.  
- Tests de compatibilité avec `vitte-compiler` et `vitte-runtime`.  
- Tests de performance de chargement.  
- Vérification des signatures et règles de sécurité.

---

## Roadmap

- [ ] Support de politiques dynamiques (basées sur le contexte d’exécution).  
- [ ] Interface visuelle de gestion dans `vitte-studio`.  
- [ ] Support multi-utilisateur et hiérarchie d’héritage.  
- [ ] Validation cryptographique étendue.  
- [ ] Intégration CI/CD pour règles de build et release.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau