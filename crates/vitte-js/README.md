

# vitte-js

Backend **JavaScript/WebAssembly** pour le langage Vitte.  
`vitte-js` génère du code JavaScript optimisé et du bytecode WebAssembly depuis l’IR Vitte, permettant l’exécution du langage dans les environnements web et Node.js.

---

## Objectifs

- Offrir une **cible Web complète** pour le compilateur Vitte.  
- Générer du code JavaScript moderne (ES2020+) et modulaire.  
- Fournir une intégration WebAssembly pour performances natives.  
- Assurer la compatibilité totale avec les navigateurs et Node.js.  
- Intégration fluide avec `vitte-docgen`, `vitte-studio` et `vitte-runtime`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération de code JavaScript et WASM |
| `builder`     | Transformation IR → AST JS et compilation WASM |
| `interop`     | Conversion de types et API FFI JS ↔ Vitte |
| `runtime`     | Glue code pour chargement dynamique et appels WebAssembly |
| `analyze`     | Validation du code généré et des dépendances JS |
| `opt`         | Optimisations JS/WASM (inlining, dead-code, minification) |
| `tests`       | Vérification du bytecode et exécution dans moteurs JS |

---

## Exemple d’utilisation

```rust
use vitte_js::JsBackend;
use vitte_codegen::Context;

fn main() -> anyhow::Result<()> {
    let mut ctx = Context::new_target("wasm");
    let backend = JsBackend::new();

    let func = ctx.create_function("main");
    backend.emit_function(&ctx, &func, "main.js")?;

    println!("Code JavaScript généré avec succès.");
    Ok(())
}
```

---

## Exemple de code généré

```js
export function main() {
    console.log("Bonjour depuis Vitte.js !");
}
```

---

## Intégration

- `vitte-compiler` : génération multi-cible (x86, WASM, JS).  
- `vitte-runtime` : exécution du bytecode WebAssembly.  
- `vitte-docgen` : intégration du rendu HTML/JS dynamique.  
- `vitte-studio` : prévisualisation et exécution locale.  
- `vitte-cli` : export direct vers modules JS (`.mjs`, `.wasm`).

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }

wasm-encoder = "0.42"
wasmparser = "0.125"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `swc` pour minification JS, `wasm-opt` pour post-traitement WASM.

---

## Tests

```bash
cargo test -p vitte-js
```

- Tests de génération et exécution JS.  
- Tests de compatibilité Node.js et navigateur.  
- Tests de validation WASM via `wasmparser`.  
- Benchmarks d’exécution WebAssembly.

---

## Roadmap

- [ ] Compilation complète en module ES.  
- [ ] Support `import/export` dynamique.  
- [ ] Intégration `vitte-runtime` pour appels interlangages.  
- [ ] Génération de packages NPM automatiques.  
- [ ] Profilage WebAssembly intégré.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau