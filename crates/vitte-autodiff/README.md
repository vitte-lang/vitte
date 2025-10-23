

# vitte-autodiff

Moteur **d’auto-différentiation (Automatic Differentiation)** pour l’écosystème Vitte.

`vitte-autodiff` fournit un système complet de dérivation automatique pour les calculs numériques, algébriques et symboliques effectués dans le langage Vitte.  
Il prend en charge les modes **forward** et **reverse**, avec support des graphes computationnels dynamiques et des fonctions différentiables de haut niveau.

---

## Objectifs

- Calcul des dérivées exactes sans approximation numérique.  
- Support des deux modes : **Forward (JVP)** et **Reverse (VJP)**.  
- Gestion des graphes computationnels dynamiques.  
- API haut niveau pour la différentiation de fonctions arbitraires.  
- Intégration directe avec `vitte-math`, `vitte-gpu`, et `vitte-runtime`.  
- Compatibilité avec le compilateur et l’optimiseur LLVM.

---

## Architecture

| Module        | Rôle |
|----------------|------|
| `graph`        | Représentation du graphe de calcul différentiable |
| `node`         | Noeuds et opérations élémentaires (add, mul, exp, sin...) |
| `tape`         | Journal des opérations pour mode reverse |
| `grad`         | Calcul du gradient et propagation arrière |
| `jacobian`     | Calcul des Jacobiennes et Hessiennes |
| `engine`       | Moteur de dérivation et gestion des contextes |
| `tests`        | Cas de vérification, comparaison analytique et numérique |

---

## Exemple d’utilisation

```rust
use vitte_autodiff::AutoDiff;

fn main() -> anyhow::Result<()> {
    let mut ad = AutoDiff::new();
    let x = ad.variable(2.0);
    let y = ad.variable(3.0);
    let z = (x * y.sin()) + (y * x.exp());
    let dz_dx = ad.grad(&z, &x)?;
    println!("dz/dx = {}", dz_dx);
    Ok(())
}
```

---

## Intégration

- `vitte-math` : opérations mathématiques différentiables.  
- `vitte-gpu` : calcul des gradients sur GPU.  
- `vitte-compiler` : propagation des gradients dans le pipeline LLVM.  
- `vitte-analyzer` : validation des graphes différentiables.  

---

## Dépendances

```toml
[dependencies]
vitte-core = { path = "../vitte-core", version = "0.1.0" }
vitte-math = { path = "../vitte-math", version = "0.1.0" }
vitte-utils = { path = "../vitte-utils", version = "0.1.0" }

ndarray = "0.15"
nalgebra = "0.33"
num-traits = "0.2"
anyhow = "1"
```

---

## Tests

```bash
cargo test -p vitte-autodiff
```

- Comparaison analytique vs. autodiff.  
- Vérification des gradients et Hessiennes.  
- Tests sur graphes dynamiques et composition de fonctions.  
- Benchmarks sur CPU et GPU.  

---

## Roadmap

- [ ] Support des tenseurs et du calcul symbolique.  
- [ ] Intégration avec `vitte-gpu` pour backprop accélérée.  
- [ ] Visualisation de graphes différentiables.  
- [ ] Export ONNX et compatibilité machine learning.  

---

## Licence

SPDX-License-Identifier: MIT  
Copyright (c) 2025 Vincent Rousseau