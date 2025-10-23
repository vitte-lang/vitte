

# vitte-repro

Système **de reproductibilité des builds et environnements** pour l’écosystème Vitte.  
`vitte-repro` garantit la cohérence des compilations, dépendances, métadonnées et artefacts à travers différents systèmes, plateformes et versions du compilateur.

---

## Objectifs

- Offrir une **reproductibilité complète des builds** sur toute la chaîne Vitte.  
- Vérifier la cohérence entre sources, dépendances et résultats.  
- Supporter les empreintes de build (`hash`, `fingerprint`) et les manifestes verrouillés.  
- Intégration directe avec `vitte-build`, `vitte-cache`, `vitte-package` et `vitte-ci`.  
- Détection automatique des écarts environnementaux (OS, toolchain, version).  

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `fingerprint` | Calcul des empreintes de build (hash global) |
| `manifest`    | Gestion des manifestes figés et empreintes de dépendances |
| `lock`        | Vérification et régénération des fichiers `vitte.lock` |
| `env`         | Collecte et comparaison des variables d’environnement |
| `report`      | Génération de rapports de dérive et d’incohérences |
| `cli`         | Interface de commande pour audit et vérification |
| `tests`       | Tests de cohérence inter-plateformes et CI |

---

## Exemple d’utilisation

### Vérifier la reproductibilité d’un build

```bash
vitte repro check
```

### Générer un manifeste verrouillé

```bash
vitte repro lock
```

### Comparer deux environnements de build

```bash
vitte repro diff --from build-a.json --to build-b.json
```

---

## Intégration

- `vitte-build` : validation du fingerprint et des artefacts générés.  
- `vitte-cache` : liaison entre empreinte de build et cache partagé.  
- `vitte-package` : création de paquets reproductibles.  
- `vitte-ci` : vérification des builds distants dans la chaîne CI/CD.  
- `vitte-studio` : visualisation des différences d’environnement.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-cache = { path = "../vitte-cache", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
blake3 = "1.5"
chrono = "0.4"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `sha2` pour compatibilité, `reqwest` pour audit distant, `colored` pour affichage CLI.

---

## Tests

```bash
cargo test -p vitte-repro
```

- Tests de cohérence des empreintes de build.  
- Tests de validation de manifestes verrouillés.  
- Tests de dérive environnementale multi-plateforme.  
- Benchmarks de recalcul des empreintes et comparaison.

---

## Roadmap

- [ ] Mode de vérification CI/CD complet.  
- [ ] Support des conteneurs reproductibles (Docker, Podman).  
- [ ] Génération automatique de `vitte.lock`.  
- [ ] Intégration graphique des comparaisons dans `vitte-studio`.  
- [ ] Empreintes cryptographiques signées (vérifiables).

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau