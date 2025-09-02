# RFC 0032 — AI & Machine Learning Integration

## Résumé
Ce RFC définit l’intégration de l’**intelligence artificielle (IA)** et du **machine learning (ML)** dans le langage **Vitte**.  
Objectif : fournir aux développeurs des primitives, bibliothèques et outils pour créer, entraîner et déployer des modèles directement en Vitte, tout en restant compatibles avec les frameworks existants.

## Motivation
L’IA est devenue centrale dans :  
- la vision par ordinateur,  
- le traitement du langage naturel,  
- la robotique et les systèmes autonomes,  
- les applications web et mobiles intelligentes.  

Vitte doit proposer un support natif pour l’IA afin de rester compétitif.

## Design proposé

### Module `std::ai`
- Gestion des tenseurs et opérations basiques.  
- Accélération GPU/TPU si disponible.  

```vitte
use std::ai::{Tensor, matmul}

fn main() {
    let a = Tensor::from([[1,2],[3,4]])
    let b = Tensor::from([[5,6],[7,8]])
    let c = matmul(a,b)
    print(c)
}
```

### Chargement de modèles pré-entraînés
- Support formats : ONNX, TensorFlow Lite, PyTorch.  
- Exemple :  

```vitte
use std::ai::model

fn main() {
    let m = model::load("resnet50.onnx")
    let res = m.run("chaton.jpg")
    print(res)
}
```

### Entraînement en Vitte
- API haut niveau type Keras.  
- Exemple :  

```vitte
use std::ai::{Model, Dense, Relu, Softmax}

fn main() {
    let mut m = Model::new([
        Dense::new(128),
        Relu::new(),
        Dense::new(10),
        Softmax::new()
    ])

    m.train("mnist.train", epochs=5)
    m.save("mnist.vitml")
}
```

### Inference optimisée
- Quantization, pruning, graph optimization intégrés.  
- Compilation JIT possible.  

### Intégration avec async/await
- Inference non bloquante :  

```vitte
async fn predict(img: String) -> Label {
    let m = model::load("mobilenet.onnx")
    m.run_async(img).await
}
```

### Exemple complet : chatbot minimal
```vitte
use std::ai::model
use std::net

fn main() {
    let m = model::load("gpt2.onnx")
    let server = net::Server::bind("0.0.0.0:9000")
    server.run(|req| {
        let answer = m.run(req.text)
        req.reply(answer)
    })
}
```

## Alternatives considérées
- **Pas de support AI natif** : rejeté, IA incontournable.  
- **Bindings externes uniquement** : rejeté, manque de cohérence et d’optimisation.  
- **Support inference seulement** : rejeté, besoin d’entraînement aussi.  

## Impact et compatibilité
- Impact fort : Vitte devient un langage viable pour IA embarquée et cloud.  
- Compatible avec ONNX, TensorFlow Lite, PyTorch.  
- Peut élargir la communauté Vitte aux data scientists.  

## Références
- [ONNX](https://onnx.ai/)  
- [TensorFlow Lite](https://www.tensorflow.org/lite)  
- [PyTorch](https://pytorch.org/)  
- [Hugging Face Transformers](https://huggingface.co/docs/transformers)  
