

# vitte-wasm

Backend **WebAssembly (WASM)** pour le langage Vitte.  
`vitte-wasm` permet la compilation, la génération et l’exécution de modules Vitte sous WebAssembly, compatibles navigateur et runtimes natifs.

---

## Objectifs

- Offrir une **génération binaire WASM stable, rapide et portable**.  
- Supporter la compilation depuis le bytecode `.vitbc` ou depuis l’IR intermédiaire.  
- Intégration directe avec `vitte-compiler`, `vitte-runtime`, `vitte-tools` et `vitte-studio`.  
- Gérer les exports, imports, mémoire linéaire et appels externes.  
- Fournir un mode exécution sandboxé et reproductible.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération du bytecode WASM à partir de l’IR Vitte |
| `link`        | Liaison et résolution des imports/exports entre modules |
| `runtime`     | Exécution et interopérabilité avec `vitte-runtime` |
| `memory`      | Gestion de la mémoire linéaire et des segments statiques |
| `validate`    | Vérification du module et compatibilité WASM MVP+ |
| `tests`       | Validation de conformité et performance |

---

## Exemple d’utilisation

### Compilation d’un module vers WebAssembly

```bash
vitte build --target wasm32 --out main.wasm
```

### Exemple Rust

```rust
use vitte_wasm::WasmEmitter;
use vitte_ir::Module;

fn main() -> anyhow::Result<()> {
    let ir = Module::from_file("target/main.vitbc")?;
    let wasm = WasmEmitter::emit(&ir)?;
    std::fs::write("target/main.wasm", wasm)?;
    Ok(())
}
```

### Exemple d’import dans JavaScript

```js
import fs from 'fs';
import { WebAssembly } from 'wasi';

const bytes = fs.readFileSync('./target/main.wasm');
const mod = await WebAssembly.instantiate(bytes, {});
mod.instance.exports.main();
```

---

## Intégration

- `vitte-compiler` : production de bytecode WASM à partir de l’IR.  
- `vitte-runtime` : exécution native via WASI.  
- `vitte-tools` : inspection des sections et symboles WASM.  
- `vitte-analyzer` : analyse et validation du code généré.  
- `vitte-studio` : visualisation graphique du module et des dépendances.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-ir = { path = "../vitte-ir", version = "0.1.0" }

wasm-encoder = "0.38"
wasmparser = "0.119"
wat = "1.0"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `wasmtime` pour exécution native, `wasmer` pour intégration runtime, `wit-bindgen` pour génération de bindings.

---

## Tests

```bash
cargo test -p vitte-wasm
```

- Tests de génération et parsing WASM.  
- Tests de compatibilité avec navigateurs et runtimes.  
- Tests de performance sur modules lourds.  
- Validation de conformité WABT et WASI.

---

## Roadmap

- [ ] Support complet WASM GC et composants.  
- [ ] Génération automatique de bindings JS/TS.  
- [ ] Optimisations spécifiques WASM SIMD et threads.  
- [ ] Intégration avec `vitte-cloud` pour exécution distante.  
- [ ] Débogage interactif dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau