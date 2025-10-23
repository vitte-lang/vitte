

# vitte-pgo

Moteur **de profilage et d’optimisation guidée par le profil (PGO)** pour le compilateur Vitte.  
`vitte-pgo` permet de collecter des données d’exécution réelles et de les exploiter pour orienter les optimisations du compilateur, du runtime et du code généré.

---

## Objectifs

- Offrir une **infrastructure complète de profilage exécutable**.  
- Collecter les métriques d’exécution (fréquences, latences, branches, appels).  
- Générer des fichiers de profil utilisables par le compilateur (`.vitprof`).  
- Intégration directe avec `vitte-opt`, `vitte-compiler` et `vitte-metrics`.  
- Permettre des optimisations ciblées basées sur les données réelles d’utilisation.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `collector`   | Collecte et agrégation des données de profilage |
| `runtime`     | Instrumentation légère du code au moment de l’exécution |
| `merge`       | Fusion et pondération de multiples fichiers de profil |
| `export`      | Sérialisation des profils au format `.vitprof` ou JSON |
| `analyze`     | Analyse des hotspots, branches et boucles |
| `feedback`    | Injection des données PGO dans le compilateur |
| `tests`       | Vérification de cohérence et exactitude des mesures |

---

## Exemple d’utilisation

### Phase 1 — Compilation instrumentée

```bash
vitte build --profile-gen
```

### Phase 2 — Exécution instrumentée

```bash
./target/debug/mon_programme
```

### Phase 3 — Compilation optimisée

```bash
vitte build --profile-use=mon_programme.vitprof --release
```

---

## Exemple de profil généré

```json
{
  "functions": {
    "add": { "calls": 152343, "time_ns": 93210 },
    "compute": { "calls": 302, "time_ns": 12003221 }
  },
  "branches":  { "taken": 91.3, "not_taken": 8.7 },
  "loops": { "hot": ["main::loop", "engine::step"] }
}
```

---

## Intégration

- `vitte-compiler` : ajuste les heuristiques d’optimisation basées sur les profils.  
- `vitte-opt` : optimise les sections les plus utilisées selon les mesures.  
- `vitte-runtime` : instrumentation légère des fonctions critiques.  
- `vitte-metrics` : exposition temps réel des données de profilage.  
- `vitte-inspect` : visualisation des hotspots et du graphe d’appel.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-opt = { path = "../vitte-opt", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
rayon = "1"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `perf-event` pour intégration Linux, `measureme` pour compatibilité Rust.

---

## Tests

```bash
cargo test -p vitte-pgo
```

- Tests de cohérence des fichiers `.vitprof`.  
- Tests de fusion et pondération multi-profils.  
- Tests de précision des timings et compteurs.  
- Benchmarks avant/après PGO sur projets réels.

---

## Roadmap

- [ ] Intégration complète du mode PGO incrémental.  
- [ ] Visualisation graphique dans `vitte-studio`.  
- [ ] Support du profilage matériel (CPU counters).  
- [ ] Fusion automatique via CI/CD.  
- [ ] Export au format universel LLVM `.profdata`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau