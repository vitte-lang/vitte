

# vitte-python

Backend **Python / C-API** pour le langage Vitte.  
`vitte-python` fournit une interopérabilité complète entre Vitte et l’écosystème Python, permettant la compilation vers des modules dynamiques, l’appel direct de fonctions Python, et l’échange transparent de données entre les deux langages.

---

## Objectifs

- Offrir un **pont bidirectionnel natif entre Vitte et Python**.  
- Générer automatiquement des modules Python depuis le code Vitte.  
- Supporter les appels de fonctions Vitte depuis Python et inversement.  
- Permettre la conversion des structures complexes (listes, dicts, tuples, classes).  
- Intégration directe avec `vitte-codegen`, `vitte-runtime` et `vitte-ffi`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération de modules dynamiques (`.pyd` / `.so`) |
| `ffi`         | Interface native avec l’interpréteur Python via C-API |
| `convert`     | Conversion des types Vitte ↔ Python (scalaires, objets, tableaux) |
| `runtime`     | Gestion du cycle d’exécution hybride Python/Vitte |
| `module`      | Création automatique des bindings et décorateurs Python |
| `loader`      | Chargement et exécution dynamique de code Python |
| `tests`       | Tests d’interopérabilité, conversion et performance |

---

## Exemple d’utilisation

### Compilation d’un module Python

```bash
vitte build --target python --out vitte_math.so
```

### Appel depuis Python

```python
import vitte_math
print(vitte_math.add(2, 3))  # 5
```

### Exemple Vitte source

```vitte
fn add(a: int, b: int) -> int {
    return a + b
}
```

---

## Intégration

- `vitte-codegen` : génération du code natif pour le backend Python.  
- `vitte-runtime` : gestion du contexte d’exécution mixte.  
- `vitte-ffi` : communication C entre les deux environnements.  
- `vitte-docgen` : génération de documentation Python (`.pyi`, docstrings).  
- `vitte-cli` : commande `vitte build --target python` et `vitte run --python`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

pyo3 = { version = "0.22", features = ["extension-module"] }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `maturin` pour empaquetage, `numpy` pour intégration scientifique, `pandas` pour manipulation de données.

---

## Tests

```bash
cargo test -p vitte-python
pytest tests/python_integration.py
```

- Tests d’import et d’exécution des modules générés.  
- Tests de conversion des types complexes.  
- Tests de compatibilité Python 3.8–3.13.  
- Benchmarks sur appels Vitte ↔ Python.

---

## Roadmap

- [ ] Support complet pour les classes et décorateurs Python.  
- [ ] Intégration directe avec `maturin` et `setuptools`.  
- [ ] Conversion automatique des exceptions Vitte ↔ Python.  
- [ ] Support async/await et coroutines Python.  
- [ ] Visualisation des modules dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau