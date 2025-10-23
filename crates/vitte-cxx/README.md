

# vitte-cxx

Pont C++ natif pour **Vitte** — intégration entre les modules Rust et C++.

Ce crate fournit les bindings, wrappers et générateurs nécessaires pour interfacer le langage Vitte et son runtime avec du code C++ moderne. Il permet d’écrire des extensions natives, d’intégrer des bibliothèques externes ou de compiler des modules Vitte vers des API C++.

---

## Objectifs

- Offrir une passerelle fiable entre Rust, C++ et Vitte  
- Générer automatiquement des en-têtes et wrappers compatibles C++17+  
- Fournir un FFI sécurisé et typé entre Vitte VM et C++  
- Supporter la compilation en mode `vitte build --target cxx`  
- Intégrer les ABI stables utilisées par `vitte-light` et `vitte-runtime`  

---

## Architecture

| Module         | Rôle |
|----------------|------|
| `ffi`          | Déclarations externes et macros de liaison |
| `bridge`       | Génération de wrappers C++ et Rust |
| `types`        | Conversion des types entre Rust, Vitte et C++ |
| `headers`      | Génération d’en-têtes C++ (vitte.h, vitte_vm.h, etc.) |
| `abi`          | Gestion des conventions d’appel et structures alignées |
| `link`         | Support des toolchains et options de linkage |
| `cli`          | Commandes de génération manuelle |
| `tests`        | Tests d’intégration FFI multi-langages |

---

## Exemple d’utilisation

### Génération d’un header C++

```
vitte cxx gen-header --out include/vitte.h
```

Résultat : un en-tête C++ contenant les définitions des structures publiques de la VM.

### Intégration dans un projet CMake

```cmake
find_package(Vitte REQUIRED)
target_link_libraries(mon_projet PRIVATE Vitte::Runtime)
```

### Liaison depuis C++

```cpp
#include "vitte.h"

int main() {
    vitte_vm* vm = vitte_vm_new();
    vitte_vm_eval(vm, "print(42)");
    vitte_vm_free(vm);
    return 0;
}
```

---

## Commandes CLI

```
vitte cxx gen-header [--out chemin]
vitte cxx gen-ffi --lang <cpp|rust>
vitte cxx build --target <arch>
vitte cxx doctor
```

Exemples :
```
vitte cxx gen-header --out include/vitte_vm.h
vitte cxx gen-ffi --lang cpp
```

---

## Types pris en charge

| Catégorie | Types |
|------------|--------|
| Entiers | `int8_t`, `int16_t`, `int32_t`, `int64_t` |
| Flottants | `float`, `double` |
| Chaînes | `const char*`, `vitte_str` |
| Structures | `vitte_value`, `vitte_frame`, `vitte_vm` |
| Tableaux | `vitte_array`, `vitte_buffer` |
| Fonctions | pointeurs typés, closures, callbacks |

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
cxx = "1.0"
bindgen = "0.70"
anyhow = "1"
serde = { version = "1.0", features = ["derive"] }
```

---

## Tests

```bash
cargo test -p vitte-cxx
vitte cxx gen-header
```

- Tests d’intégration sur Linux, macOS et Windows  
- Validation du binaire via `nm` et `objdump`  
- Tests CMake automatisés (`tests/cpp_integration/`)  

---

## Roadmap

- [ ] Génération automatique de bindings pour Qt et JUCE  
- [ ] Compilation directe de modules `.vit` en `.so` / `.dll` C++  
- [ ] Support complet des exceptions C++ dans Vitte VM  
- [ ] Liaison croisée avec `vitte-cross` pour cross-compilation  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau