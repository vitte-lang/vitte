

# vitte-matlab

Backend **MATLAB / Octave** et interopérabilité scientifique pour le langage Vitte.  
`vitte-matlab` permet la génération de scripts MATLAB/Octave à partir du code Vitte et la communication bidirectionnelle entre le runtime et les environnements de calcul scientifique.

---

## Objectifs

- Offrir une **interopérabilité native** entre Vitte et MATLAB/Octave.  
- Générer automatiquement des scripts `.m` depuis l’IR Vitte.  
- Exécuter du code MATLAB depuis un programme Vitte (FFI dynamique).  
- Faciliter le transfert de données (matrices, structures, graphiques).  
- Intégration directe avec `vitte-analyzer`, `vitte-codegen` et `vitte-linalg`.

---

## Architecture

| Module        | Rôle |
|---------------|------|
| `emit`        | Génération de scripts MATLAB (`.m`) |
| `interop`     | Liaison FFI avec le moteur MATLAB ou Octave |
| `mat`         | Conversion des matrices et tableaux Vitte ↔ MATLAB |
| `runtime`     | Chargement dynamique du moteur et gestion du contexte |
| `builder`     | Construction d’AST MATLAB depuis IR Vitte |
| `analyze`     | Validation des signatures et cohérence de types |
| `tests`       | Vérification de la génération et de l’exécution des scripts |

---

## Exemple d’utilisation

```rust
use vitte_matlab::MatlabBackend;
use vitte_codegen::Context;

fn main() -> anyhow::Result<()> {
    let mut ctx = Context::new_target("matlab");
    let backend = MatlabBackend::new();

    let func = ctx.create_function("square");
    backend.emit_function(&ctx, &func, "square.m")?;

    println!("Script MATLAB généré avec succès.");
    Ok(())
}
```

---

## Exemple de code généré

```matlab
function y = square(x)
    y = x .* x;
end
```

---

## Intégration

- `vitte-codegen` : génération automatique de code MATLAB.  
- `vitte-analyzer` : vérification des signatures et compatibilité des types.  
- `vitte-linalg` : transmission directe des matrices et vecteurs.  
- `vitte-runtime` : exécution dynamique des fonctions MATLAB.  
- `vitte-docgen` : génération de documentation pour API scientifiques.

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }
vitte-codegen = { path = "../vitte-codegen", version = "0.1.0" }
vitte-linalg = { path = "../vitte-linalg", version = "0.1.0" }

serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
libloading = "0.8"
anyhow = "1"
thiserror = "1"
``` 

> Optionnel : `matio-rs` pour lecture/écriture de fichiers `.mat`, `octave-sys` pour compatibilité GNU Octave.

---

## Tests

```bash
cargo test -p vitte-matlab
```

- Tests de génération de scripts `.m`.  
- Tests d’exécution via moteur MATLAB/Octave.  
- Tests de transfert de données et cohérence numérique.  
- Benchmarks de performance sur grands tableaux.

---

## Roadmap

- [ ] Support complet du moteur Octave natif.  
- [ ] Liaison en mémoire via FFI sans fichiers intermédiaires.  
- [ ] Conversion automatique des structures complexes.  
- [ ] Profilage des appels MATLAB depuis Vitte.  
- [ ] Génération automatique de notebooks hybrides MATLAB/Vitte.

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau