

# vitte-haskell

Backend **Haskell** et interopérabilité fonctionnelle pour le langage Vitte.  
`vitte-haskell` permet la génération de code Haskell depuis l’IR Vitte, ainsi que l’import/export de fonctions Haskell dans des modules Vitte.

---

## Objectifs

- Fournir une **interopérabilité bidirectionnelle** entre Vitte et Haskell.  
- Générer du code Haskell typé depuis l’IR du compilateur Vitte.  
- Permettre l’import de modules Haskell (`.hs`) vers le runtime Vitte.  
- Supporter la conversion des types primitifs, tuples, et structures génériques.  
- Intégration transparente dans le pipeline de build et d’analyse.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `ffi`         | Interface FFI entre Vitte et Haskell (via GHC runtime) |
| `emit`        | Génération de code Haskell depuis MIR/IR Vitte |
| `parser`      | Lecture des signatures et imports Haskell externes |
| `bridge`      | Conversion de types et adaptation des ABI |
| `runtime`     | Liaison avec GHC, initialisation et exécution |
| `interop`     | Fonctions utilitaires pour appels croisés |
| `tests`       | Tests de round-trip et d’intégration FFI |

---

## Exemple d’utilisation

```rust
use vitte_haskell::HaskellBackend;
use vitte_codegen::Context;

fn main() -> anyhow::Result<()> {
    let mut ctx = Context::new_target("haskell");
    let backend = HaskellBackend::new();

    let func = ctx.create_function("add");
    backend.emit_function(&ctx, &func, "Add.hs")?;

    println!("Code Haskell généré avec succès.");
    Ok(())
}
```

---

## Exemple de code généré

```haskell
add :: Int -> Int -> Int
add a b = a + b
```

---

## Intégration

- `vitte-codegen` : conversion MIR → Haskell.  
- `vitte-compiler` : intégration au pipeline de génération multi-langage.  
- `vitte-analyzer` : vérification des signatures importées.  
- `vitte-runtime` : exécution via GHC intégré.  
- `vitte-lsp` : inspection des symboles et signatures Haskell.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }

regex = "1"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `ghc-lib` pour intégration directe au compilateur GHC, `serde_yaml` pour métadonnées de binding.

---

## Tests

```bash
cargo test -p vitte-haskell
```

- Tests d’import et export FFI.  
- Tests de génération et exécution Haskell.  
- Vérification des conversions de types.  
- Round-trip IR ↔ Haskell.

---

## Roadmap

- [ ] Génération complète de modules Haskell (`.hs`) avec dépendances.  
- [ ] Liaison directe avec GHC API (`ghc-lib`).  
- [ ] Exécution hybride Haskell/Vitte via runtime unifié.  
- [ ] Support des monades et effets (`IO`, `Maybe`, `Either`).  
- [ ] Export de fonctions Vitte vers Haskell (`foreign export`).

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau