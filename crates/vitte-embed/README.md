

# vitte-embed

Bibliothèque **d’intégration et d’embarquement** du langage Vitte.

`vitte-embed` fournit une API C/Rust pour exécuter, embarquer et piloter des scripts Vitte depuis des applications externes. Il s’intègre dans des projets C/C++, Rust, Python ou WebAssembly, permettant d’utiliser Vitte comme moteur de scripting universel et sécurisé.

---

## Objectifs

- Intégration simple du moteur Vitte dans toute application native.
- API unifiée Rust, C et WebAssembly.
- Gestion fine des contextes, variables et exécution isolée.
- Interopérabilité entre Vitte ↔ C ↔ Rust ↔ Python.
- Sécurité mémoire et sandbox optionnelle.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `engine`       | Initialisation et exécution du moteur embarqué |
| `ffi`          | API C publique (`vitte_embed.h`) |
| `runtime`      | Gestion du contexte et des modules chargés |
| `bindings`     | Couche Rust <-> C pour intégration externe |
| `sandbox`      | Contrôles d’isolation, quotas, limites CPU/mémoire |
| `wasm`         | Support WebAssembly (via `vitte-wasm`) |
| `tests`        | Tests d’intégration inter-langages |

---

## Exemple d’intégration (C)

```c
#include <vitte_embed.h>

int main(void) {
    VitteContext* ctx = vitte_new_context();
    vitte_eval(ctx, "print('Hello from Vitte')");
    vitte_free_context(ctx);
    return 0;
}
```

---

## Exemple d’intégration (Rust)

```rust
use vitte_embed::Engine;

fn main() -> anyhow::Result<()> {
    let engine = Engine::new();
    engine.eval("print('Hello from Vitte')")?;
    Ok(())
}
```

---

## Intégration Python

```python
import vitte_embed

ctx = vitte_embed.Context()
ctx.eval("print('Hello from Vitte')")
```

---

## Intégration WebAssembly

- Génération via `wasm32-wasi` :
  ```bash
  cargo build -p vitte-embed --target wasm32-wasi
  ```
- Import dans le navigateur :
  ```js
  import init, { vitte_eval } from "./vitte_embed.js";
  await init();
  vitte_eval("print('Hello Web')");
  ```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-parser = { path = "../vitte-parser", version = "0.1.0" }
vitte-ffi = { path = "../vitte-ffi", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
anyhow = "1"
libc = "0.2"
once_cell = "1"
```

---

## CLI

```
vitte embed run <script.vit>
vitte embed eval "print(1+2)"
vitte embed ffi-info
```

---

## Tests

```bash
cargo test -p vitte-embed
```

- Tests d’exécution isolée.
- Tests FFI C et Rust.
- Tests WebAssembly.
- Vérification des limites de sandbox.

---

## Roadmap

- [ ] Support d’embedding Python natif via CFFI.
- [ ] API WebSocket pour exécution distante.
- [ ] Bindings .NET et Java.
- [ ] Profiling d’exécution intégré.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau