

# vitte-playground

Environnement **d’exécution interactive et expérimentation du langage Vitte**.  
`vitte-playground` fournit un moteur REPL, un exécuteur de snippets et un environnement sécurisé pour tester, profiler et visualiser le code Vitte en direct.

---

## Objectifs

- Offrir un **REPL complet et interactif** pour Vitte.  
- Supporter l’évaluation incrémentale des expressions et fonctions.  
- Gérer la compilation à la volée et la persistance du contexte.  
- Intégration directe avec `vitte-runtime`, `vitte-analyzer` et `vitte-lsp`.  
- Permettre l’exécution sécurisée dans un environnement sandboxé.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `repl`        | Boucle interactive (lecture, évaluation, affichage) |
| `eval`        | Évaluation dynamique des expressions et fonctions |
| `context`     | Gestion du contexte de session (variables, modules, imports) |
| `sandbox`     | Isolation et sécurité des exécutions |
| `history`     | Historique des commandes et des résultats |
| `print`       | Formatage des sorties et erreurs |
| `tests`       | Tests d’intégration et validation des sessions interactives |

---

## Exemple d’utilisation

### Lancement du REPL

```bash
vitte repl
```

### Exemple de session

```text
Vitte v0.1.0 — Playground interactif
>>> let x = 21
>>> x * 2
42
>>> fn square(n: i32) -> i32 { n * n }
>>> square(5)
25
```

---

## Intégration

- `vitte-runtime` : exécution dynamique des blocs de code.  
- `vitte-analyzer` : validation sémantique et suggestions en direct.  
- `vitte-compiler` : compilation incrémentale pour le REPL.  
- `vitte-lsp` : support de l’autocomplétion et de la navigation.  
- `vitte-cli` : commande `vitte repl` ou `vitte playground`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-analyzer = { path = "../vitte-analyzer", version = "0.1.0" }

rustyline = "12.0"
tokio = { version = "1", features = ["full"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `colored` pour la coloration syntaxique, `wasmtime` pour l’exécution sandboxée.

---

## Tests

```bash
cargo test -p vitte-playground
```

- Tests d’évaluation d’expressions simples et imbriquées.  
- Tests de persistance du contexte entre exécutions.  
- Tests d’intégration avec le compilateur et le runtime.  
- Benchmarks de performance d’évaluation dynamique.

---

## Roadmap

- [ ] Support complet du mode graphique (Vitte Studio).  
- [ ] Historique persistant entre sessions.  
- [ ] Import direct de fichiers `.vitte`.  
- [ ] Profiling et inspection temps réel.  
- [ ] Mode collaboratif multi-utilisateurs via WebSocket.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau