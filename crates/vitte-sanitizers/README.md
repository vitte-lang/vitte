

# vitte-sanitizers

Infrastructure **de vérification, instrumentation et détection d’erreurs** pour le langage Vitte.  
`vitte-sanitizers` fournit des outils d’analyse dynamique visant à identifier les comportements indéfinis, fuites mémoire, dépassements de bornes et corruptions de pile au moment de l’exécution.

---

## Objectifs

- Offrir une **suite de sanitizers performante et intégrée**.  
- Détecter les erreurs mémoire, d’accès concurrent, et de logique.  
- Intégration directe avec `vitte-runtime`, `vitte-compiler`, `vitte-profiler` et `vitte-analyzer`.  
- Permettre l’instrumentation automatique à la compilation (`--sanitize`).  
- Fournir un mode “strict” pour la validation CI/CD et production sécurisée.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `address`     | Détection des dépassements de mémoire et use-after-free |
| `thread`      | Analyse des accès concurrents et verrous incorrects |
| `memory`      | Vérification des fuites et corruptions de heap |
| `undefined`   | Repérage des comportements indéfinis |
| `report`      | Génération de rapports détaillés et JSON |
| `hooks`       | Points d’instrumentation runtime et intégration profiler |
| `tests`       | Validation, fuzzing et tests de stress |

---

## Exemple d’utilisation

### Compilation instrumentée

```bash
vitte build --sanitize=address,thread,undefined
```

### Exemple de sortie

```text
==vitte== AddressSanitizer: heap-buffer-overflow on address 0x602000000014
READ of size 4 at 0x602000000014 by thread T0
    in fn add at src/math.vitte:12
```

---

## Intégration

- `vitte-runtime` : instrumentation et hooks mémoire/threads.  
- `vitte-compiler` : injection des appels de vérification dans le code généré.  
- `vitte-profiler` : corrélation des erreurs avec les traces de performance.  
- `vitte-analyzer` : enrichissement des rapports statiques avec anomalies dynamiques.  
- `vitte-studio` : visualisation interactive des erreurs et contextes d’exécution.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

backtrace = "0.3"
tracing = "0.1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `mimalloc` pour profiling mémoire, `rayon` pour analyse parallèle, `colored` pour sortie enrichie.

---

## Tests

```bash
cargo test -p vitte-sanitizers
```

- Tests d’instrumentation mémoire et concurrence.  
- Tests de détection d’UB et segmentation fautives.  
- Tests d’intégration avec le runtime et le profiler.  
- Benchmarks de surcharge induite par instrumentation.

---

## Roadmap

- [ ] Mode kernel-level pour environnements sécurisés.  
- [ ] Extension fuzzing intégrée (`vitte fuzz --sanitize`).  
- [ ] Support WASM et architectures embarquées.  
- [ ] Export vers `vitte-studio` pour analyse graphique.  
- [ ] Corrélation automatique entre erreurs et symboles.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau