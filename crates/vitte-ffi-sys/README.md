

# vitte-ffi-sys

Interface **bas-niveau FFI (Foreign Function Interface)** du langage Vitte.

`vitte-ffi-sys` expose les symboles C natifs du moteur Vitte pour l’intégration dans d’autres langages et environnements. Il constitue la couche la plus proche de l’ABI, utilisée par `vitte-ffi`, `vitte-embed`, et les bindings externes (C, C++, Python, WebAssembly).

---

## Objectifs

- Fournir une API C stable et minimale pour l’exécution du moteur Vitte.
- Garantir la compatibilité ABI sur les plateformes principales (macOS, Linux, Windows).
- Servir de base à la génération automatique de wrappers (`vitte-ffi`, `vitte-bindgen`).
- Faciliter l’intégration dans des langages externes (Python, Node.js, .NET, etc.).
- Supporter le mode statique et dynamique (libvitte.a / libvitte.so / vitte.dll).

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `headers`      | Fichiers d’en-tête C (`vitte_ffi.h`, `vitte_runtime.h`) |
| `bindings`     | Fonctions externes et pointeurs symboliques |
| `macros`       | Définitions de macros et constantes C |
| `types`        | Structures de base (`VitteContext`, `VitteValue`, `VitteError`) |
| `build.rs`     | Script de compilation et liaison |
| `tests`        | Tests FFI et vérification d’ABI |

---

## Exemple d’utilisation (C)

```c
#include <vitte_ffi.h>

int main(void) {
    VitteContext* ctx = vitte_new_context();
    vitte_eval(ctx, "print('Hello depuis vitte-ffi-sys')");
    vitte_free_context(ctx);
    return 0;
}
```

---

## Exemple d’utilisation (Rust)

```rust
use vitte_ffi_sys::*;

fn main() {
    unsafe {
        let ctx = vitte_new_context();
        vitte_eval(ctx, b"print('Hello FFI sys')\0".as_ptr() as *const _);
        vitte_free_context(ctx);
    }
}
```

---

## Build

Compilation en statique ou dynamique :

```bash
cargo build -p vitte-ffi-sys
cargo build -p vitte-ffi-sys --features dynamic
```

Sorties typiques :
- `libvitte.a` (statique)
- `libvitte.so` / `vitte.dll` (dynamique)

---

## Dépendances

```toml
[dependencies]
libc = "0.2"

[build-dependencies]
cc = "1.0"
bindgen = "0.70"
```

---

## Tests

```bash
cargo test -p vitte-ffi-sys
```

- Vérification de l’ABI et des symboles exportés.
- Tests de compatibilité multiplateforme.
- Vérification des conversions de chaînes.

---

## Roadmap

- [ ] Support complet pour WebAssembly (via `wasm32-wasi`).
- [ ] Génération automatique de `vitte_ffi.h` à partir du code Rust.
- [ ] Système de versionnage ABI.
- [ ] Intégration CI pour validation cross-platform.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau