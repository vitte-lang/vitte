

# vitte-borrowck

Moteur **d’analyse d’emprunts et de possession** du langage Vitte.

`vitte-borrowck` implémente le système d’analyse des références, de mutabilité et de durée de vie, inspiré du modèle Rust, adapté au langage Vitte.  
Il garantit la sûreté mémoire, la non-concurrence des accès et la validité des pointeurs à la compilation.

---

## Objectifs

- Assurer la **sécurité mémoire statique** sans garbage collector.  
- Détecter les violations d’emprunt (`alias mut`, `dangling ref`, etc.).  
- Gérer la mutabilité, les durées de vie et les régions lexicales.  
- Fournir des diagnostics précis avec suggestion de correctifs.  
- Intégration complète avec le compilateur et l’analyseur sémantique.  

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `region`       | Gestion des régions de vie et des sous-portées |
| `borrow`       | Suivi des emprunts et vérification d’exclusivité |
| `ownership`    | Mécanismes de transfert et de libération |
| `lifetime`     | Vérification des durées de vie et relations hiérarchiques |
| `diagnostics`  | Messages d’erreur et conseils de correction |
| `tests`        | Validation des scénarios d’emprunt et fuite mémoire |

---

## Exemple d’utilisation

```rust
use vitte_borrowck::BorrowChecker;
use vitte_ast::parse;

fn main() -> anyhow::Result<()> {
    let src = "let mut x = 5; let y = &x; x = 10;";
    let ast = parse(src)?;
    let mut checker = BorrowChecker::new();
    checker.check(&ast)?;
    Ok(())
}
```

Sortie attendue :
```
Erreur [E0506]: impossible de modifier `x` car il est encore emprunté
 --> main.vitte:1:23
  |
1 | let mut x = 5; let y = &x; x = 10;
  |                       ^^ emprunté ici
```

---

## Intégration

- `vitte-analyzer` : utilise le borrow checker pour l’analyse sémantique.  
- `vitte-compiler` : valide la sûreté avant génération de code.  
- `vitte-lsp` : diagnostics interactifs pour IDE.  
- `vitte-tests` : vérification automatique des scénarios mémoire.  

---

## Dépendances

```toml
[dependencies]
vitte-core       = { path = "../vitte-core", version = "0.1.0" }
vitte-analyzer   = { path = "../vitte-analyzer", version = "0.1.0" }
vitte-ast        = { path = "../vitte-ast", version = "0.1.0" }
vitte-diagnostics = { path = "../vitte-diagnostics", version = "0.1.0" }
vitte-utils      = { path = "../vitte-utils", version = "0.1.0" }

anyhow = "1"
indexmap = "2.5"
serde = { version = "1.0", features = ["derive"] }
```

---

## Tests

```bash
cargo test -p vitte-borrowck
```

- Détection des violations d’emprunts.  
- Tests des durées de vie imbriquées.  
- Snapshots des diagnostics.  
- Vérification de cohérence avec `vitte-analyzer`.  

---

## Roadmap

- [ ] Emprunts dynamiques pour types complexes.  
- [ ] Analyse inter-procédurale des durées de vie.  
- [ ] Suggestions automatiques de fix (`auto-mut`, `clone`, `move`).  
- [ ] Visualisation graphique des relations d’emprunt.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau