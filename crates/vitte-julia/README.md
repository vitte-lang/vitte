

# vitte-julia

Backend **Julia** et interopérabilité scientifique pour le langage Vitte.  
`vitte-julia` permet la génération de code Julia à partir de l’IR Vitte, ainsi que l’appel de fonctions Julia depuis les modules Vitte et inversement.

---

## Objectifs

- Offrir une **interopérabilité bidirectionnelle** entre Vitte et Julia.  
- Générer du code Julia typé et optimisé depuis l’IR.  
- Permettre l’import de fonctions Julia externes dans les programmes Vitte.  
- Supporter la compilation croisée et le partage de modules scientifiques.  
- Intégration directe avec `vitte-codegen`, `vitte-runtime` et `vitte-analyzer`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération du code Julia (fonctions, types, modules) |
| `interop`     | Conversion de types et appels FFI Julia ↔ Vitte |
| `runtime`     | Intégration avec le runtime Julia via `libjulia` |
| `builder`     | Construction de l’AST Julia depuis l’IR |
| `analyze`     | Validation des signatures et dépendances externes |
| `tests`       | Tests d’intégration, génération et exécution Julia |

---

## Exemple d’utilisation

```rust
use vitte_julia::JuliaBackend;
use vitte_codegen::Context;

fn main() -> anyhow::Result<()> {
    let mut ctx = Context::new_target("julia");
    let backend = JuliaBackend::new();

    let func = ctx.create_function("square");
    backend.emit_function(&ctx, &func, "square.jl")?;

    println!("Code Julia généré avec succès.");
    Ok(())
}
```

---

## Exemple de code généré

```julia
function square(x::Int)
    return x * x
end
```

---

## Intégration

- `vitte-codegen` : génération du code Julia à partir du MIR.  
- `vitte-compiler` : sélection de la cible `julia`.  
- `vitte-runtime` : interaction avec `libjulia` pour exécution dynamique.  
- `vitte-analyzer` : vérification des signatures et conversions de type.  
- `vitte-lsp` : support des diagnostics interlangages.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
anyhow = "1"
thiserror = "1"
libjulia-sys = "0.9"
``` 

> Optionnel : `cc` pour liaison native, `rayon` pour génération parallèle.

---

## Tests

```bash
cargo test -p vitte-julia
```

- Tests de génération et d’exécution Julia.  
- Tests de conversion FFI et cohérence de types.  
- Tests d’intégration avec `libjulia`.  
- Benchmarks de performance pour code généré.

---

## Roadmap

- [ ] Export automatique de modules Julia depuis Vitte.  
- [ ] Compilation croisée Julia → Vitte.  
- [ ] Support des types complexes et tableaux multidimensionnels.  
- [ ] JIT partagé entre Vitte et Julia.  
- [ ] Profilage et visualisation de performances Julia/Vitte.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau