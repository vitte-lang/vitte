

# vitte-go

Interface **Go (Golang)** pour l’écosystème Vitte.

`vitte-go` permet d’intégrer le moteur et le langage Vitte directement dans des projets Go via une interface FFI native. Il expose une API Go idiomatique pour exécuter des scripts, manipuler des valeurs Vitte et interagir avec le runtime.

---

## Objectifs

- Fournir une API Go idiomatique et stable.
- Liaison directe avec `vitte-ffi` et `vitte-embed`.
- Intégration transparente du runtime dans des applications Go.
- Communication bidirectionnelle entre Go et Vitte.
- Gestion mémoire sécurisée et types convertibles.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `engine`       | Initialisation du moteur et gestion du contexte |
| `ffi`          | Couche d’interopérabilité avec `vitte-ffi` |
| `value`        | Conversion des types Go ↔ Vitte |
| `runtime`      | Exécution des scripts et gestion des erreurs |
| `errors`       | Mappage des erreurs C vers Go |
| `examples`     | Cas d’utilisation pratiques |
| `tests`        | Tests d’intégration Go ↔ Vitte |

---

## Exemple d’utilisation

```go
package main

import (
    "fmt"
    "github.com/vitte-lang/vitte-go/vitte"
)

func main() {
    ctx := vitte.NewContext()
    defer ctx.Close()

    err := ctx.Eval(`print("Hello from Vitte inside Go!")`)
    if err != nil {
        fmt.Println("Erreur :", err)
    }
}
```

---

## Conversion des types

| Go Type | Vitte Type | Conversion |
|----------|-------------|-------------|
| `int` | `Int` | Automatique |
| `float64` | `Float` | Automatique |
| `string` | `String` | UTF-8 |
| `bool` | `Bool` | Direct |
| `map[string]any` | `Dict` | Reflété |
| `[]any` | `Array` | Reflété |

---

## Installation

```bash
go get github.com/vitte-lang/vitte-go
```

---

## Compilation

Le package s’appuie sur `vitte-ffi` (Rust/C) :

```bash
cargo build -p vitte-ffi --release
go build ./...
```

Assurez-vous que la bibliothèque native (`libvitte.so`, `libvitte.dylib`, ou `vitte.dll`) est dans le `LD_LIBRARY_PATH` ou `DYLD_LIBRARY_PATH`.

---

## Dépendances

```toml
[dependencies]
vitte-ffi = { path = "../vitte-ffi", version = "0.1.0" }
vitte-core = { path = "../vitte-core", version = "0.1.0" }
```

---

## Tests

```bash
go test ./...
```

- Tests d’intégration Go ↔ Vitte.
- Vérification des conversions et erreurs.
- Tests sur les appels FFI concurrents.

---

## Roadmap

- [ ] Support de goroutines ↔ coroutines Vitte.
- [ ] API streaming (canaux Go ↔ Vitte).
- [ ] Build statique sans dépendances externes.
- [ ] Packaging pour macOS/Linux/Windows.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau