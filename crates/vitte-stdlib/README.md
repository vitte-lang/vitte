

# vitte-stdlib

Bibliothèque **standard du langage Vitte**, fournissant les types fondamentaux, fonctions utilitaires et modules de base nécessaires à tout programme Vitte.  
`vitte-stdlib` constitue la fondation de l’écosystème Vitte et assure la compatibilité entre les différentes cibles (`native`, `wasm64`, `rv64`, `aarch64`, `x86_64`).

---

## Objectifs

- Offrir une **base cohérente et performante** pour tous les projets Vitte.  
- Regrouper les primitives de manipulation mémoire, chaînes, collections et IO.  
- Servir de point d’entrée pour les imports implicites (`use std::*`).  
- Fournir des modules hautement optimisés et testés.  
- Support complet du mode asynchrone, SIMD, et multi-plateforme.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `core`        | Types fondamentaux et traits de base (Option, Result, Iterator, etc.) |
| `mem`         | Gestion mémoire bas niveau et allocation |
| `str`         | Chaînes, encodages et manipulation de texte |
| `vec`         | Structures de données dynamiques et algorithmes |
| `io`          | Lecture/écriture fichiers, flux et terminaux |
| `net`         | Réseaux TCP/UDP et sockets asynchrones |
| `math`        | Fonctions mathématiques, SIMD et statistiques |
| `async`       | Gestion des tâches, awaiters et futures |
| `time`        | Gestion des durées, horloges et timers |
| `fs`          | Accès au système de fichiers portable |
| `os`          | Interface système et gestion des processus |
| `tests`       | Vérification de stabilité et compatibilité interplateforme |

---

## Exemple d’utilisation

```vitte
use std::vec::Vec;
use std::io;

fn main() -> io::Result {
    let mut data = Vec::new();
    data.push(1);
    data.push(2);
    println!("Longueur : {}", data.len());
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : exécution native des primitives du stdlib.  
- `vitte-compiler` : import implicite dans tous les programmes.  
- `vitte-ffi` : exposition des types standard vers C/Rust.  
- `vitte-docgen` : documentation automatique des API du standard.  
- `vitte-analyzer` : vérification statique des appels aux API du stdlib.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-async = { path = "../vitte-async", version = "0.1.0" }
vitte-math = { path = "../vitte-math", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `tokio` pour IO asynchrone, `rayon` pour parallélisme, `mimalloc` pour allocation optimisée.

---

## Tests

```bash
cargo test -p vitte-stdlib
```

- Tests de compatibilité inter-architecture.  
- Tests de performance sur primitives clés.  
- Tests de cohérence avec le compilateur et le runtime.  
- Benchmarks mémoire et IO.

---

## Roadmap

- [ ] Extensions pour le support WASM et RV64 complet.  
- [ ] Module math avancé (complexes, FFT, matrices).  
- [ ] Optimisation SIMD auto-détectée.  
- [ ] Documentation complète `std::*` sur `vitte-docgen`.  
- [ ] Intégration des tests dans `vitte-bench` et `vitte-profiler`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau