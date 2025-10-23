

# vitte-sandbox

Moteur **d’exécution isolée et sécurisée** pour le langage Vitte.  
`vitte-sandbox` permet d’exécuter du code Vitte dans un environnement restreint, contrôlé et audité, garantissant la sécurité des processus, la limitation des ressources et la traçabilité des opérations.

---

## Objectifs

- Fournir une **sandbox sécurisée et configurable** pour l’exécution de code Vitte.  
- Empêcher tout accès non autorisé au système hôte.  
- Gérer la mémoire, les IO, le CPU et le réseau de façon isolée.  
- Intégration directe avec `vitte-runtime`, `vitte-playground` et `vitte-profiler`.  
- Support des environnements embarqués, serveurs et CI/CD.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `process`     | Gestion des processus isolés et du sandboxing OS |
| `memory`      | Limitation mémoire et surveillance d’allocation |
| `fs`          | Système de fichiers virtuel et accès restreint |
| `net`         | Filtrage réseau et blocage sélectif des connexions |
| `policy`      | Application des règles de sécurité et profils d’exécution |
| `audit`       | Journalisation et supervision des actions exécutées |
| `tests`       | Vérification de robustesse et sécurité |

---

## Exemple d’utilisation

### Exécution isolée d’un script

```bash
vitte sandbox run script.vitte --max-mem 64M --timeout 5s
```

### Exemple en code

```rust
use vitte_sandbox::Sandbox;

fn main() -> anyhow::Result<()> {
    let mut sandbox = Sandbox::new()
        .max_memory(64 * 1024 * 1024)
        .timeout(std::time::Duration::from_secs(5));

    sandbox.run_file("examples/secure.vitte")?;
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : exécution contrôlée des modules compilés.  
- `vitte-playground` : sandbox automatique pour l’évaluation de snippets.  
- `vitte-profiler` : mesure des performances isolées.  
- `vitte-lsp` : exécution sécurisée lors des suggestions automatiques.  
- `vitte-ci` : exécution confinée dans les pipelines de test.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

sysinfo = "0.30"
seccomp = "0.3"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `nix` pour gestion bas-niveau des processus, `capctl` pour capacités Linux, `tokio` pour sandbox asynchrone.

---

## Tests

```bash
cargo test -p vitte-sandbox
```

- Tests de limitations mémoire et CPU.  
- Tests d’accès fichiers et réseau.  
- Tests de robustesse contre les boucles infinies.  
- Tests d’intégration avec `vitte-runtime`.

---

## Roadmap

- [ ] Support complet Windows/macOS sandbox API.  
- [ ] Isolation par namespace Linux et chroot.  
- [ ] Intégration Docker / Podman pour sandbox étendue.  
- [ ] Profiling de sécurité intégré dans `vitte-studio`.  
- [ ] Exécution distribuée des sandboxes en cluster.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau