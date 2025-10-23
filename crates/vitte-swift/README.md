

# vitte-swift

Backend **Swift / Xcode** pour le langage Vitte.  
`vitte-swift` permet la génération, l’intégration et l’interopérabilité entre le code Vitte et l’écosystème Swift (macOS, iOS, watchOS, tvOS).

---

## Objectifs

- Offrir une **interopérabilité complète entre Vitte et Swift**.  
- Générer du code Swift ou Objective‑C compatible Xcode à partir de modules Vitte.  
- Permettre la compilation de bibliothèques Vitte en frameworks Swift natifs.  
- Supporter les appels bidirectionnels Vitte ↔ Swift avec conversion automatique des types.  
- Intégration directe avec `vitte-codegen`, `vitte-ffi`, et `vitte-runtime`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération des wrappers Swift et Objective‑C |
| `ffi`         | Interface avec le runtime Swift et la Swift ABI |
| `convert`     | Conversion des structures et types Swift ↔ Vitte |
| `framework`   | Création des frameworks `.xcframework` ou `.framework` |
| `bindings`    | Génération automatique de headers C et Swift pour interop |
| `tests`       | Vérification des appels FFI et compatibilité Xcode |

---

## Exemple d’utilisation

### Compilation d’un module Swift

```bash
vitte build --target swift --out VitteCore.framework
```

### Exemple d’appel Swift vers Vitte

```swift
import VitteCore

let result = VitteCore.add(3, 4)
print("Résultat : \(result)") // Résultat : 7
```

### Exemple Vitte source

```vitte
fn add(a: int, b: int) -> int {
    return a + b
}
```

---

## Intégration

- `vitte-codegen` : production des wrappers Swift et Objective‑C.  
- `vitte-runtime` : exécution du code Vitte depuis le runtime Swift.  
- `vitte-ffi` : appels directs entre langages via C interop layer.  
- `vitte-build` : génération automatique de `.framework` ou `.xcframework`.  
- `vitte-cli` : commande `vitte build --target swift`.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-runtime = { path = "../vitte-runtime", version = "0.1.0" }

cc = "1.1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
```

> Optionnel : `bindgen` pour génération automatique des en-têtes, `swift-bindgen` pour intégration Xcode.

---

## Tests

```bash
cargo test -p vitte-swift
```

- Tests d’interop Vitte → Swift et Swift → Vitte.  
- Tests de compatibilité des structures et signatures de fonction.  
- Tests de génération et d’importation de frameworks Xcode.  
- Benchmarks sur le coût d’appel FFI.

---

## Roadmap

- [ ] Génération automatique de modules Swift Package Manager.  
- [ ] Support complet des closures et génériques Swift.  
- [ ] Interopérabilité Swift async/await ↔ Vitte async.  
- [ ] Validation et signature des frameworks sur macOS/iOS.  
- [ ] Visualisation des bindings dans `vitte-studio`.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau