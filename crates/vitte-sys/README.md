

# vitte-sys

Bibliothèque **bas niveau de liaison système (system bindings)** pour le langage Vitte.  
`vitte-sys` fournit les appels système, interfaces POSIX/Win32 et primitives natives utilisées par le runtime, le compilateur et les bibliothèques standard.

---

## Objectifs

- Offrir une **abstraction portable des appels système** pour Vitte.  
- Supporter Linux, macOS, Windows, BSD et plateformes embarquées.  
- Fournir des interfaces stables pour la mémoire, les threads, les fichiers et les sockets.  
- Intégration directe avec `vitte-runtime`, `vitte-ffi`, `vitte-stdlib` et `vitte-os`.  
- Garantir la compatibilité binaire et l’isolation des couches haut niveau.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `ffi`         | Déclarations externes C et interop avec libc / WinAPI |
| `platform`    | Implémentations spécifiques (unix, darwin, win32, bsd) |
| `memory`      | Allocation, mmap, protection et libération mémoire |
| `thread`      | Gestion des threads, mutex, conditions et sémaphores |
| `fs`          | Accès aux fichiers, répertoires et métadonnées |
| `net`         | Sockets TCP/UDP, poll/select, transport bas niveau |
| `time`        | Interfaces temps réel et horloges système |
| `tests`       | Vérification de portabilité et cohérence inter-OS |

---

## Exemple d’utilisation

```rust
use vitte_sys::fs;

fn main() -> anyhow::Result<()> {
    let path = "/tmp/example.txt";
    fs::write(path, b"Hello Vitte!")?;
    let data = fs::read(path)?;
    println!("{}", String::from_utf8_lossy(&data));
    Ok(())
}
```

---

## Intégration

- `vitte-runtime` : accès système de bas niveau pour les appels internes.  
- `vitte-ffi` : exposition des fonctions natives à C/Rust.  
- `vitte-stdlib` : réimplémentation sécurisée des appels systèmes.  
- `vitte-os` : couche d’abstraction multiplateforme.  
- `vitte-analyzer` : validation des dépendances système et imports externes.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

libc = "0.2"
winapi = { version = "0.3", optional = true }
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `nix` pour extensions UNIX, `windows-sys` pour compatibilité moderne Windows, `once_cell` pour globales sécurisées.

---

## Tests

```bash
cargo test -p vitte-sys
```

- Tests sur chaque plateforme cible (Linux/macOS/Windows/BSD).  
- Tests de performance sur IO et synchronisation.  
- Tests de compatibilité FFI.  
- Vérification des appels mémoire et sécurité d’accès.

---

## Roadmap

- [ ] Couverture complète des appels POSIX et WinAPI.  
- [ ] Support des signaux et interruptions.  
- [ ] Abstraction pour les périphériques spéciaux.  
- [ ] Intégration avec `vitte-sandbox` pour sécurité d’exécution.  
- [ ] Génération automatique des bindings via `vitte-bindgen`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau