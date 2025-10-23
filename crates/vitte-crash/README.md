

# vitte-crash

Gestion des crashs et rapports d’erreurs fatales pour **Vitte**.

Ce crate fournit l’infrastructure de capture, sérialisation et restitution des crashs au sein du compilateur, du runtime et des outils CLI. Il vise à rendre les plantages reproductibles, traçables et exploitables dans la chaîne CI/CD.

---

## Objectifs

- Intercepter les panic, erreurs non rattrapées et signaux système  
- Générer des rapports structurés (JSON, texte, symboliqué)  
- Associer chaque crash à un contexte (build, commit, machine, commande)  
- Fournir une interface CLI et API unifiée  
- Intégration directe dans `vitte-compiler`, `vitte-cli`, et `vitte-vm`

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `handler`      | Capture et traitement des panic et signaux POSIX |
| `backtrace`    | Collecte symbolique et dé-mangling des traces |
| `report`       | Sérialisation des rapports (texte, JSON) |
| `store`        | Sauvegarde locale et gestion des sessions |
| `cli`          | Commandes de diagnostic et d’analyse |
| `upload`       | Intégration distante (GitHub, Sentry, fichiers locaux) |
| `tests`        | Tests unitaires et crash simulés |

---

## Exemple d’utilisation

### Intégration dans un binaire

```rust
use vitte_crash::CrashHandler;

fn main() {
    let _guard = CrashHandler::install();
    panic!("Erreur fatale simulée");
}
```

Un rapport est automatiquement généré dans `target/crash-reports/`.

### Exemple de rapport JSON

```json
{
  "timestamp": "2025-10-22T13:05:42Z",
  "version": "v0.1.0",
  "module": "vitte-compiler",
  "thread": "main",
  "signal": "SIGSEGV",
  "message": "segmentation fault",
  "backtrace": [
    "vitte_core::span::Span::new",
    "vitte_ast::parse_expr",
    "vitte_compiler::run"
  ],
  "system": {
    "os": "macOS 15.0",
    "arch": "arm64"
  }
}
```

---

## CLI

```
vitte crash list
vitte crash show <id>
vitte crash upload <id> [--to sentry|file|github]
vitte crash clear
```

Exemples :

```
vitte crash list
vitte crash show 2025-10-22T13-05-42
vitte crash upload latest --to sentry
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
backtrace = "0.3"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
chrono = "0.4"
uuid = { version = "1", features = ["v4"] }
thiserror = "1"
anyhow = "1"
colored = "2"
dirs = "5"
```

---

## Intégration

- `vitte-compiler`: capture les erreurs de compilation non prévues  
- `vitte-vm`: trace les crashs d’exécution et dumps mémoire  
- `vitte-cli`: expose la commande `vitte crash` pour inspection  
- `vitte-runtime`: gère les erreurs d’allocation et panics noyaux  

---

## Tests

```bash
cargo test -p vitte-crash
```

- Tests unitaires : validation des traces et formats de sortie  
- Tests d’intégration : crashs simulés multi-threads  
- Benchmarks : génération de backtraces complexes  

---

## Roadmap

- [ ] Support des dumps minidump (Windows, macOS, Linux)  
- [ ] Envoi automatique vers Sentry et GitHub Issues  
- [ ] Mode silencieux CI  
- [ ] Filtrage par module ou type de signal  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau