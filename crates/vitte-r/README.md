

# vitte-r

Backend **R / CRAN** pour le langage Vitte.  
`vitte-r` permet la génération de packages R à partir du code Vitte et l’interopérabilité native entre le runtime Vitte et l’environnement R, utilisé pour l’analyse statistique et la visualisation de données.

---

## Objectifs

- Offrir une **intégration fluide entre Vitte et R**.  
- Générer automatiquement des packages R (`.tar.gz`) depuis le code Vitte.  
- Permettre l’appel de fonctions R depuis Vitte et inversement.  
- Supporter la conversion automatique des types (`numeric`, `data.frame`, `matrix`, etc.).  
- Intégration complète avec `vitte-codegen`, `vitte-linalg` et `vitte-analyzer`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération de code R et wrappers `.R` |
| `ffi`         | Liaison directe avec R via `libR` et `.Call` interface |
| `convert`     | Conversion des types R ↔ Vitte (scalaires, vecteurs, tables) |
| `runtime`     | Gestion de l’exécution intégrée du moteur R |
| `package`     | Création automatique des structures CRAN et DESCRIPTION |
| `analyze`     | Validation des signatures et dépendances R |
| `tests`       | Vérification de compatibilité et performance |

---

## Exemple d’utilisation

### Compilation d’un module R

```bash
vitte build --target r --out vitte_stats.tar.gz
```

### Utilisation dans R

```r
library(vitte_stats)
add(2, 3) # 5
```

### Exemple de code Vitte

```vitte
fn add(a: f64, b: f64) -> f64 {
    a + b
}
```

---

## Intégration

- `vitte-codegen` : production des bindings R natifs.  
- `vitte-runtime` : exécution des appels R/Vitte en mémoire.  
- `vitte-linalg` : passage de matrices et vecteurs.  
- `vitte-analyzer` : validation des signatures et compatibilités.  
- `vitte-cli` : commande `vitte build --target r`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

libR-sys = "0.4"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `extendr-api` pour génération de bindings R, `ggplot2` pour intégration graphique.

---

## Tests

```bash
cargo test -p vitte-r
Rscript tests/test_integration.R
```

- Tests d’import et d’exécution des fonctions.  
- Tests de compatibilité R 4.x.  
- Tests de conversion de types complexes.  
- Benchmarks sur grands ensembles de données.

---

## Roadmap

- [ ] Génération automatique de packages CRAN complets.  
- [ ] Intégration `ggplot2` et `tidyverse`.  
- [ ] Support Rcpp et C API native.  
- [ ] Intégration graphique dans `vitte-studio`.  
- [ ] Documentation R auto-générée (`man/`, `NAMESPACE`).

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau