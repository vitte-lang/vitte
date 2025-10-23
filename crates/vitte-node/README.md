

# vitte-node

Backend **Node.js / JavaScript** pour le langage Vitte.  
`vitte-node` permet la compilation, l’interopérabilité et l’exécution de code Vitte dans les environnements Node.js et ECMAScript modernes.

---

## Objectifs

- Offrir un **ciblage Node.js natif** pour les projets Vitte.  
- Générer du code JavaScript optimisé depuis l’IR Vitte.  
- Permettre la communication bidirectionnelle entre Vitte et JS via FFI.  
- Supporter les modules ESM, CommonJS et WebAssembly.  
- Fournir un pont d’exécution asynchrone (promises ↔ futures).

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération de code JS/ESM depuis l’IR |
| `ffi`         | Interfaçage Vitte ↔ JavaScript via N-API / wasm-bindgen |
| `runtime`     | Gestion du cycle de vie Node (import, exec, context) |
| `interop`     | Conversion de types et sérialisation JSON / Buffer |
| `bridge`      | Liaison entre runtime JS et tâches asynchrones Vitte |
| `loader`      | Chargement et initialisation des modules Vitte dans Node |
| `tests`       | Vérification d’interopérabilité et génération du code |

---

## Exemple d’utilisation

### Exécution directe

```bash
# Compiler un module Vitte pour Node.js
vitte build --target node --out main.js

# Lancer le module
node main.js
```

### Exemple de code généré

```javascript
import { add } from './main.vit.js';
console.log(add(2, 3)); // 5
```

---

## Intégration

- `vitte-codegen` : génération du code JavaScript depuis IR.  
- `vitte-runtime` : gestion des appels inter-langages.  
- `vitte-analyzer` : validation des types et compatibilité symbolique.  
- `vitte-docgen` : documentation automatique des bindings JS.  
- `vitte-cli` : commande `vitte build --target node`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
wasm-bindgen = "0.2"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `napi-rs` pour interopérabilité native, `tokio` pour exécution asynchrone.

---

## Tests

```bash
cargo test -p vitte-node
```

- Tests de génération JS/ESM.  
- Tests d’interopérabilité avec Node.js.  
- Tests de conversion de types et FFI.  
- Benchmarks de performance sur appels croisés.

---

## Roadmap

- [ ] Support WebAssembly + Node runtime hybride.  
- [ ] Génération automatique de wrappers TypeScript.  
- [ ] Exécution directe des modules `.vit` depuis Node.  
- [ ] Support Deno et Bun.  
- [ ] Intégration complète avec `vitte-studio` pour debug JS/Vitte.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau