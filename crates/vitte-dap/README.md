# vitte-dap

Implémentation du protocole **Debug Adapter Protocol (DAP)** pour Vitte.

Ce crate fournit un adaptateur de débogage complet conforme à la spécification du protocole DAP (Microsoft VSCode). Il permet de déboguer des programmes écrits en Vitte depuis des environnements externes tels que VSCode, Neovim, CLion, ou tout autre IDE compatible.

---

## Objectifs

- Offrir une implémentation complète du protocole DAP pour le langage Vitte  
- Supporter les points d’arrêt, la navigation pas à pas et l’inspection des variables  
- Fournir une communication bidirectionnelle via JSON-RPC  
- S’intégrer avec `vitte-vm` et `vitte-runtime` pour exécution instrumentée  
- Permettre l’extension via plugins et événements personnalisés  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `server`       | Serveur DAP TCP/STDIO principal |
| `protocol`     | Structures du protocole, sérialisation et parsing JSON-RPC |
| `handler`      | Gestion des commandes DAP (launch, step, pause, etc.) |
| `state`        | Représentation de la session de débogage |
| `bridge`       | Interface entre DAP et la machine virtuelle `vitte-vm` |
| `events`       | Émission d’événements et notifications côté IDE |
| `cli`          | Lancement manuel du serveur (`vitte dap serve`) |
| `tests`        | Tests d’intégration DAP complets avec scénarios VSCode |

---

## Exemple d’utilisation

### Lancement d’un serveur DAP
```bash
vitte dap serve --port 4711
```

### Configuration VSCode
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "type": "vitte",
      "request": "launch",
      "name": "Lancer le programme Vitte",
      "program": "${workspaceFolder}/main.vit",
      "dapServer": "localhost:4711"
    }
  ]
}
```

### Exemple de session
```
> Breakpoint hit at main.vit:12
> Step over → line 13
> Variable a = 42
> Continue → program finished
```

---

## Commandes CLI

```
vitte dap serve [--port <n>] [--stdio]
vitte dap inspect <pid>
vitte dap list-sessions
vitte dap kill <pid>
```

---

## Protocoles supportés

- JSON-RPC 2.0 (via STDIO ou TCP)  
- DAP 1.61 (VSCode)  
- Extensions spécifiques `vitte/stack`, `vitte/trace`, `vitte/heap`  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-vm = { path = "../vitte-vm", version = "0.1.0" }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
tokio = { version = "1", features = ["full"] }
tracing = "0.1"
futures = "0.3"
anyhow = "1"
colored = "2"
```

---

## Tests

```bash
cargo test -p vitte-dap
vitte dap serve --stdio
```

- Tests de conformité DAP automatisés  
- Tests d’intégration IDE (VSCode, Neovim)  
- Mock VM pour vérification des états internes  

---

## Roadmap

- [ ] Support du débogage des coroutines asynchrones  
- [ ] Inspection mémoire (heap / stack) temps réel  
- [ ] Profilage pas à pas avec `vitte-profiler`  
- [ ] Interface WebSocket pour clients distants  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
