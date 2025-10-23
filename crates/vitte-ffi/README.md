# vitte-ffi

Bibliothèque **d’interfaçage C/Rust** pour l’écosystème Vitte.

`vitte-ffi` fournit les définitions, types et wrappers nécessaires pour exposer le langage Vitte à des environnements externes (C, C++, Python, etc.). Il constitue la couche FFI de bas niveau utilisée par `vitte-embed`, `vitte-runtime`, et les bindings générés automatiquement.

---

## Objectifs

- Fournir une interface stable entre Vitte et le monde C.
- Offrir des types et structures C garantis ABI-compatibles.
- Faciliter la création de bindings dans d’autres langages.
- Gérer la mémoire et les conversions de chaînes UTF-8/C.
- Servir de base au runtime embarqué (`vitte-embed`).

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `types`        | Définitions des structures C (`VitteValue`, `VitteError`, etc.) |
| `bridge`       | Fonctions externes pour exécuter et interagir avec le moteur |
| `convert`      | Conversion entre types Rust et C |
| `string`       | Gestion sécurisée des chaînes et buffers |
| `allocator`    | Allocateur mémoire C personnalisé |
| `macros`       | Macros de génération FFI pour Vitte |
| `tests`        | Vérifications ABI et appels croisés |

---

## Exemple d’utilisation (C)

```c
#include <vitte_ffi.h>

int main(void) {
    VitteContext* ctx = vitte_new_context();
    vitte_eval(ctx, "print('Hello depuis FFI')");
    vitte_free_context(ctx);
    return 0;
}
```

---

## Exemple d’utilisation (Rust)

```rust
use vitte_ffi::{vitte_new_context, vitte_eval, vitte_free_context};
use std::ffi::CString;

fn main() {
    unsafe {
        let ctx = vitte_new_context();
        let code = CString::new("print('Hello FFI')").unwrap();
        vitte_eval(ctx, code.as_ptr());
        vitte_free_context(ctx);
    }
}
```

---

## Interface publique (`vitte_ffi.h`)

```c
typedef struct VitteContext VitteContext;

VitteContext* vitte_new_context(void);
void vitte_free_context(VitteContext*);
int  vitte_eval(VitteContext*, const char* code);
const char* vitte_last_error(VitteContext*);
```

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
libc = "0.2"
once_cell = "1"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-ffi
```

- Vérification de la compatibilité ABI.
- Tests d’allocation/désallocation mémoire.
- Tests d’exécution basique via FFI.

---

## Roadmap

- [ ] Génération automatique d’en-têtes C.
- [ ] Bindings Python (CFFI) et Node.js (N-API).
- [ ] Export dynamique de symboles pour shared libs.
- [ ] Intégration complète avec `vitte-embed`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau
