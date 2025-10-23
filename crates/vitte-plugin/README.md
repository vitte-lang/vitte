

# vitte-plugin

Système **de plugins dynamiques et d’extensions** pour l’écosystème Vitte.  
`vitte-plugin` permet de charger, exécuter et gérer des extensions externes ou internes au runtime, compilateur et environnement de développement Vitte.

---

## Objectifs

- Offrir une **API unifiée pour les extensions dynamiques**.  
- Supporter les plugins C, Vitte, WASM et Python.  
- Permettre l’intégration modulaire du compilateur et du runtime.  
- Fournir un système de découverte, chargement et signature sécurisée des plugins.  
- Intégration complète avec `vitte-loader`, `vitte-runtime` et `vitte-studio`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `registry`    | Gestion du registre des plugins installés |
| `loader`      | Chargement dynamique (C, WASM, Vitte) |
| `ffi`         | Interface FFI pour interaction native |
| `manifest`    | Métadonnées et dépendances du plugin (`plugin.toml`) |
| `context`     | Gestion du cycle de vie des plugins (init, exec, unload) |
| `sandbox`     | Sécurité et isolement des plugins externes |
| `tests`       | Vérification de compatibilité et tests de performance |

---

## Exemple d’utilisation

### Chargement manuel

```bash
vitte plugin load myplugin.vitplug
```

### Exemple de plugin Vitte

```vitte
#[plugin(name = "hello", version = "0.1.0")]
fn on_load() {
    println!("Plugin chargé : hello.vitplug");
}
```

---

## Intégration

- `vitte-runtime` : exécution des plugins au sein du runtime.  
- `vitte-compiler` : extension des passes d’analyse et de génération.  
- `vitte-loader` : gestion des dépendances et du chargement dynamique.  
- `vitte-studio` : découverte et activation des extensions UI.  
- `vitte-cli` : commande `vitte plugin` (load, unload, list, info, verify).

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-loader = { path = "../vitte-loader", version = "0.1.0" }

libloading = "0.8"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `wasmtime` pour plugins WASM, `pyo3` pour extensions Python.

---

## Tests

```bash
cargo test -p vitte-plugin
```

- Tests de chargement/déchargement de plugins.  
- Tests de compatibilité inter-langages.  
- Tests de sécurité et sandboxing.  
- Benchmarks sur le temps d’initialisation des plugins.

---

## Roadmap

- [ ] Support complet WASM et Python.  
- [ ] Registre global des plugins (`plugins.vitte.dev`).  
- [ ] Mode hot-reload pour le runtime.  
- [ ] Interface graphique dans `vitte-studio`.  
- [ ] Système de permissions granulaire pour les plugins externes.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau