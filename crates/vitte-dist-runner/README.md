

# vitte-dist-runner

Exécution distribuée et orchestration de tâches pour **Vitte**.

Ce crate gère la planification, la distribution et la synchronisation des exécutions de projets Vitte sur plusieurs machines. Il sert de fondation aux fonctionnalités de compilation et test parallélisés sur réseau local ou cloud.

---

## Objectifs

- Fournir une exécution distribuée fiable pour `vitte build`, `vitte test`, et `vitte bench`  
- Optimiser le temps de build via la parallélisation sur plusieurs nœuds  
- Garantir la cohérence et la reproductibilité entre exécutions  
- Supporter les environnements hybrides (local + cloud)  
- Offrir une API extensible pour la gestion de clusters et nœuds  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `scheduler`    | Répartition des tâches et files d’exécution |
| `node`         | Représentation des nœuds distants (CPU, RAM, charge) |
| `executor`     | Lancement des jobs sur les nœuds |
| `network`      | Communication sécurisée entre les hôtes |
| `cache`        | Cache partagé et synchronisation des artefacts |
| `monitor`      | Suivi en temps réel des builds distribués |
| `cli`          | Interface ligne de commande |
| `tests`        | Tests d’intégration multi-nœuds |

---

## Exemple d’utilisation

### Lancement d’un cluster local

```bash
vitte dist start --nodes 4
vitte dist status
vitte build --dist
```

### Exécution sur un cluster distant

```bash
vitte dist connect --host cluster.vitte.dev
vitte dist run build
```

---

## Modes d’exécution

| Mode | Description |
|-------|-------------|
| `local` | Distribution sur plusieurs threads locaux |
| `cluster` | Répartition sur un ensemble de nœuds du réseau |
| `hybrid` | Combinaison des deux modes |
| `cloud` | Exécution via un backend cloud compatible (à venir) |

---

## CLI

```
vitte dist start [--nodes <n>]
vitte dist stop
vitte dist status
vitte dist connect --host <adresse>
vitte dist run <commande>
vitte dist logs [--follow]
```

Exemples :
```
vitte dist start --nodes 8
vitte dist run test
vitte dist logs --follow
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-config = { path = "../vitte-config", version = "0.1.0" }
tokio = { version = "1", features = ["full"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
reqwest = { version = "0.12", features = ["json", "rustls-tls"] }
anyhow = "1"
tracing = "0.1"
dashmap = "6"
colored = "2"
```

---

## Intégration

- `vitte-build` : exécution distribuée des étapes de compilation  
- `vitte-test` : distribution des tests unitaires et d’intégration  
- `vitte-bench` : benchmarks parallélisés multi-nœuds  
- `vitte-cli` : commandes utilisateur pour orchestration  

---

## Tests

```bash
cargo test -p vitte-dist-runner
vitte dist start --nodes 2
vitte build --dist
```

- Tests d’intégration réseau local  
- Simulation de défaillance de nœuds  
- Validation du cache partagé  

---

## Roadmap

- [ ] Support complet cloud (AWS, GCP, Fly.io)  
- [ ] Authentification et chiffrement TLS  
- [ ] Tableaux de bord web temps réel  
- [ ] Auto-scaling selon la charge  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau