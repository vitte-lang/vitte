

# vitte-debugger

Débogueur intégré pour **Vitte**, basé sur la machine virtuelle `vitte-vm` et le protocole DAP.

Ce crate fournit l’infrastructure complète de débogage local et distant pour les programmes écrits en Vitte. Il permet l’inspection d’états internes, la gestion de points d’arrêt, l’évaluation d’expressions à la volée et la visualisation des piles d’exécution.

---

## Objectifs

- Fournir un débogueur natif pour le langage Vitte  
- Supporter l’exécution pas à pas, les breakpoints, et l’inspection mémoire  
- Intégration directe avec le protocole DAP (`vitte-dap`)  
- Support du débogage distant via TCP et WebSocket  
- Fournir une interface CLI et API Rust  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `session`      | Gestion des sessions de débogage actives |
| `breakpoints`  | Points d’arrêt, conditions et filtres |
| `stack`        | Inspection des frames et variables locales |
| `eval`         | Évaluation d’expressions à la volée |
| `memory`       | Exploration mémoire et objets heap |
| `transport`    | Communication locale, TCP ou WebSocket |
| `cli`          | Interface en ligne de commande |
| `tests`        | Tests d’intégration multi-sessions |

---

## Exemple d’utilisation

### Lancement du débogueur intégré

```
vitte debug main.vit
```

### Débogage distant

```
vitte debug --remote 127.0.0.1:4711
```

### Exemple de session
```
> Breakpoint hit at src/math.vit:24
> print(x) → 12
> step into
> continue
> program exited (code 0)
```

---

## CLI

```
vitte debug <fichier.vit> [--remote <hôte:port>] [--dap]
vitte debug attach <pid>
vitte debug eval <expression>
vitte debug break <fichier:ligne>
vitte debug list
vitte debug info
```

Exemples :
```
vitte debug main.vit
vitte debug eval "sqrt(9)"
vitte debug break src/main.vit:12
```

---

## API Rust

```rust
use vitte_debugger::{Debugger, Breakpoint};

fn main() {
    let mut dbg = Debugger::new("main.vit").unwrap();
    dbg.add_breakpoint(Breakpoint::new("src/math.vit", 24));
    dbg.run().unwrap();
}
```

---

## Intégration

- `vitte-vm` : moteur d’exécution instrumenté  
- `vitte-dap` : interface DAP côté IDE  
- `vitte-cli` : commandes utilisateur  
- `vitte-profiler` : extensions d’analyse de performance  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-dap = { path = "../vitte-dap", version = "0.1.0" }
vitte-vm = { path = "../vitte-vm", version = "0.1.0" }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tokio = { version = "1", features = ["full"] }
futures = "0.3"
anyhow = "1"
colored = "2"
tracing = "0.1"
```

---

## Tests

```bash
cargo test -p vitte-debugger
vitte debug main.vit
```

- Tests de sessions DAP et locales  
- Simulation d’exécutions pas à pas  
- Tests mémoire et stack multi-threads  

---

## Roadmap

- [ ] Interface TUI interactive  
- [ ] Support complet WebSocket pour IDE distants  
- [ ] Profilage intégré en temps réel  
- [ ] Points d’arrêt conditionnels sur variables  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau