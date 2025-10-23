

# vitte-c

Interface **C** pour le langage et le runtime Vitte.

`vitte-c` fournit des en-têtes (`.h`) et des bibliothèques natives (`.a`, `.so`, `.dylib`) permettant d’intégrer le moteur Vitte dans des applications écrites en C ou C++.  
Il constitue le pont bas-niveau entre `vitte-runtime`, `vitte-ffi`, et les environnements natifs.

---

## Objectifs

- Exposer une API C stable pour le moteur Vitte.  
- Permettre l’exécution de scripts et modules Vitte depuis C/C++.  
- Fournir une intégration native simple (`vitte_init`, `vitte_eval`, `vitte_free`).  
- Garantir la portabilité (macOS, Linux, Windows).  
- Supporter le linking statique et dynamique.  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `include/`     | Fichiers d’en-tête publics (`vitte.h`, `vitte_runtime.h`, etc.) |
| `ffi/`         | Fonctions exposées via `extern "C"` |
| `runtime/`     | Lien direct avec `vitte-runtime` |
| `bindings/`    | Génération automatique d’interfaces (C/C++/FFI) |
| `tests/`       | Exemples et tests d’intégration C |

---

## Exemple d’utilisation (C)

```c
#include "vitte.h"

int main(void) {
    vitte_context_t* ctx = vitte_init();
    vitte_eval(ctx, "print('Hello from Vitte in C');");
    vitte_free(ctx);
    return 0;
}
```

---

## Compilation et lien

```bash
# Compilation du runtime natif
cargo build -p vitte-c --release

# Exemple d’utilisation avec GCC
gcc -Iinclude -Ltarget/release main.c -lvitte_c -o main
```

---

## Fonctions principales

| Fonction | Description |
|-----------|--------------|
| `vitte_init()` | Initialise le contexte Vitte |
| `vitte_eval(ctx, code)` | Évalue du code Vitte |
| `vitte_load_module(ctx, path)` | Charge un module depuis le disque |
| `vitte_call(ctx, fn, args)` | Appelle une fonction exportée |
| `vitte_free(ctx)` | Libère la mémoire associée au contexte |

---

## Intégration

- `vitte-runtime` : moteur d’exécution principal.  
- `vitte-ffi` : implémentation Rust des appels C.  
- `vitte-embed` : wrappers C++ et Python.  
- `vitte-tools` : inspection et packaging des bibliothèques.  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }
vitte-ffi = { path = "../vitte-ffi", version = "0.1.0" }

cc = "1"
bindgen = "0.70"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-c
```

- Vérification du linking statique et dynamique.  
- Tests d’évaluation depuis C.  
- Vérification de compatibilité ABI.  

---

## Roadmap

- [ ] Génération automatique de headers versionnés.  
- [ ] Intégration C++ avec RAII.  
- [ ] Support de compilation croisée ARM/Linux.  
- [ ] Compatibilité avec `vitte-light` et `vitl`.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau