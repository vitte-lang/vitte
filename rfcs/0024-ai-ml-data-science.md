# RFC 0024 — AI/ML and Data Science Support

## Résumé
Ce RFC définit le support de **l’intelligence artificielle (IA)**, du **machine learning (ML)** et de la **science des données** dans le langage **Vitte**.  
L’objectif est de permettre l’utilisation de Vitte pour des applications de data science, d’IA embarquée et de calcul haute performance.

## Motivation
Les développeurs attendent qu’un langage moderne puisse :  
- charger et manipuler efficacement des données,  
- interagir avec des frameworks IA existants,  
- exécuter des modèles ML de manière performante,  
- offrir une API ergonomique pour l’analyse statistique.  

Sans cela, Vitte serait cantonné au bas niveau et perdrait des utilisateurs au profit de Python ou Julia.

## Design proposé

### Bibliothèque standard `std::data` et `std::ml`
- Structures pour vecteurs (`Tensor`), matrices, séries temporelles.  
- Fonctions statistiques de base : moyenne, variance, corrélation.  

### Interopérabilité Python et C++
- Importation de bibliothèques IA existantes :  
```vitte
@ffi_import("numpy")
fn np_array(data: &[f64]) -> Tensor
```

- Appel de PyTorch, TensorFlow, ONNX via bindings.  

### Module `std::ml::tensor`
- Tenseur multidimensionnel optimisé.  
- Opérations vectorisées (`matmul`, `conv2d`, `relu`).  

```vitte
use std::ml::tensor

fn main() {
    let a = tensor::from([[1,2],[3,4]])
    let b = tensor::from([[5,6],[7,8]])
    let c = a.matmul(&b)
    print(c)
}
```

### Accélération matérielle
- Support GPU via CUDA, OpenCL et Vulkan.  
- API portable via `std::ml::gpu`.  
- Possibilité d’utiliser CPU SIMD (AVX, Neon).  

### Exemple : régression linéaire
```vitte
use std::ml

fn main() {
    let x = ml::tensor::from([1.0, 2.0, 3.0, 4.0])
    let y = ml::tensor::from([2.0, 4.0, 6.0, 8.0])
    let model = ml::linear_regression(x, y)
    print(model.predict([5.0])) // ~10.0
}
```

### Outils de data science
- `vitpm data csv import` → charge un dataset.  
- `vitpm ml train` → entraîne un modèle basique.  
- Export en `.onnx` ou `.pt` pour interop.  

### Intégration CI/CD
- Tests de modèles reproductibles.  
- Support de datasets mock pour CI.  

## Alternatives considérées
- **Pas de support natif** : rejeté, perdrait les data scientists.  
- **Libs externes uniquement** : rejeté pour manque d’uniformité.  
- **Support CPU only** : rejeté, GPU indispensable pour ML moderne.  

## Impact et compatibilité
- Impact fort : ouvre Vitte aux domaines IA et science des données.  
- Compatible avec Python et ONNX.  
- Introduit une certaine complexité mais nécessaire pour adoption.  

## Références
- [NumPy](https://numpy.org/)  
- [PyTorch](https://pytorch.org/)  
- [TensorFlow](https://www.tensorflow.org/)  
- [ONNX](https://onnx.ai/)  
- [Julia ML](https://julialang.org/)  
