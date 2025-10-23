

# vitte-snapshot

Système **de snapshots et état persistant** pour l’écosystème Vitte.  
`vitte-snapshot` permet la capture, la restauration et la comparaison des états internes du compilateur, du runtime et des projets, pour le débogage, la reproductibilité et l’analyse d’évolution.

---

## Objectifs

- Offrir une **infrastructure fiable pour les snapshots d’état**.  
- Capturer la mémoire, les métadonnées et les artefacts du compilateur.  
- Comparer deux snapshots pour détecter les dérives et régressions.  
- Intégration directe avec `vitte-build`, `vitte-analyzer`, `vitte-repro` et `vitte-profiler`.  
- Fournir un format portable et sérialisable (`.vitsnap`).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `capture`     | Extraction et sérialisation de l’état complet |
| `restore`     | Restauration à partir d’un snapshot sauvegardé |
| `compare`     | Différenciation et analyse de dérive entre deux états |
| `meta`        | Métadonnées, version, timestamp et empreintes |
| `storage`     | Gestion des fichiers de snapshots sur disque |
| `tests`       | Validation d’intégrité et cohérence des captures |

---

## Exemple d’utilisation

### Création d’un snapshot

```bash
vitte snapshot create --output build/vitte_state.vitsnap
```

### Comparaison entre deux snapshots

```bash
vitte snapshot diff old.vitsnap new.vitsnap
```

### Restauration d’un état antérieur

```bash
vitte snapshot restore build/vitte_state.vitsnap
```

---

## Intégration

- `vitte-build` : enregistrement automatique de l’état du build.  
- `vitte-analyzer` : capture des graphes sémantiques et IR.  
- `vitte-repro` : vérification de cohérence entre builds.  
- `vitte-profiler` : analyse comparative des performances.  
- `vitte-studio` : visualisation graphique des différences entre snapshots.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
blake3 = "1.5"
flate2 = "1.0"
tar = "0.4"
chrono = "0.4"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `zstd` pour compression rapide, `diffy` pour comparaison textuelle, `walkdir` pour snapshots étendus.

---

## Tests

```bash
cargo test -p vitte-snapshot
```

- Tests de capture et restauration complètes.  
- Tests de cohérence et compatibilité inter-version.  
- Tests de comparaison binaire et métadonnées.  
- Benchmarks de performance sur grands projets.

---

## Roadmap

- [ ] Support incrémental (diff partiel entre snapshots).  
- [ ] Compression adaptative (zstd, brotli, lz4).  
- [ ] Visualisation détaillée dans `vitte-studio`.  
- [ ] Intégration CI/CD avec `vitte-repro`.  
- [ ] Export des snapshots au format JSON pour API externes.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau