

# vitte-c-abi

Implémentation **de l’ABI C (Application Binary Interface)** du langage Vitte.  
`vitte-c-abi` définit la convention d’appel, la disposition mémoire, et les règles d’interopérabilité entre le runtime Vitte et les bibliothèques natives C/C++.

---

## Objectifs

- Fournir une ABI stable et documentée pour les appels inter-langages.  
- Gérer les conversions de types (`Vitte → C` et `C → Vitte`).  
- Assurer la compatibilité entre les environnements (macOS, Linux, Windows).  
- Servir de fondation à `vitte-c`, `vitte-ffi`, `vitte-embed` et `vitte-runtime`.  
- Permettre la génération automatique de bindings.  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `types`        | Définition des structures partagées (`VtValue`, `VtArray`, `VtString`, etc.) |
| `layout`       | Alignement, padding et taille des structures selon la plateforme |
| `calling`      | Conventions d’appel (cdecl, sysv, win64) |
| `marshal`      | Conversion entre types Rust et C |
| `error`        | Gestion des erreurs et codes de retour |
| `tests`        | Vérification des tailles et compatibilité ABI |

---

## Exemple d’utilisation

```c
#include "vitte_abi.h"

VtValue vitte_call(VtContext* ctx, const char* fn_name, VtValue* args, size_t argc);
```

```rust
use vitte_c_abi::{VtValue, VtString};

fn call_example() {
    let val = VtValue::from_str("Hello");
    println!("Value len = {}", val.size);
}
```

---

## Intégration

- `vitte-c` : interface C publique (`vitte.h`).  
- `vitte-ffi` : implémentation Rust des appels externes.  
- `vitte-runtime` : exécution et gestion mémoire.  
- `vitte-embed` : pont C++ et Python.  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

memoffset = "0.9"
libc = "0.2"
anyhow = "1"
serde = { version = "1.0", features = ["derive"] }
```

---

## Tests

```bash
cargo test -p vitte-c-abi
```

- Vérification des offsets de champs.  
- Tests de conversion de types.  
- Validation des conventions d’appel.  
- Compatibilité multi-architecture (x86_64, aarch64, rv64).  

---

## Roadmap

- [ ] Génération automatique des headers C depuis Rust.  
- [ ] Support de structures alignées SIMD.  
- [ ] ABI spécifique pour WebAssembly.  
- [ ] Outils de validation binaire et introspection dynamique.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau