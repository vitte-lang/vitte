# RFC 0048 — Brain-Inspired Computing & Neuromorphic Systems

## Résumé
Ce RFC introduit le support des **architectures neuromorphiques** et du **brain-inspired computing** dans le langage **Vitte**.  
Objectif : permettre l’utilisation de modèles computationnels inspirés du cerveau humain pour concevoir des applications massivement parallèles, sobres en énergie et adaptées à l’IA cognitive.

## Motivation
Les systèmes neuromorphiques imitent la structure et le fonctionnement du cerveau :  
- Traitement événementiel basé sur des **spiking neural networks (SNN)**,  
- Faible consommation énergétique (Intel Loihi, IBM TrueNorth),  
- Applications en robotique, edge AI, perception sensorielle,  
- Complémentarité avec le deep learning classique.  

Vitte doit offrir une API native pour programmer ces architectures émergentes.

## Design proposé

### Module `std::neuro`
- Définition de neurones, synapses, réseaux SNN.  
- Simulation et exécution sur hardware neuromorphique.  

```vitte
use std::neuro::{Neuron, Synapse, Network}

fn main() {
    let n1 = Neuron::new()
    let n2 = Neuron::new()
    let s = Synapse::connect(n1, n2, weight=0.8)

    let net = Network::new([n1, n2], [s])
    net.stimulate(n1, spike=1.0)
    net.run(100)
}
```

### Plasticité synaptique
- Règles biologiques (Hebbian, STDP).  

```vitte
use std::neuro::learning

fn main() {
    let mut net = learning::stdp_network()
    net.train(input_spikes, output_spikes)
}
```

### Intégration capteurs-robots
- Coupler perception événementielle (vision neuromorphique).  

```vitte
use std::neuro::vision

fn main() {
    let retina = vision::retina()
    for frame in retina.stream() {
        print("Événements visuels: {}", frame)
    }
}
```

### Exemple complet : reconnaissance événementielle
```vitte
use std::neuro::{Network, Neuron}
use std::neuro::learning

fn main() {
    let n1 = Neuron::new()
    let n2 = Neuron::new()
    let mut net = Network::new([n1, n2], [])

    learning::hebbian(&mut net, epochs=100)
    net.stimulate(n1, spike=1.0)
    net.run(200)
}
```

## Alternatives considérées
- **Support deep learning uniquement** : rejeté, manque de sobriété énergétique.  
- **Simulation via librairies externes** : rejeté, besoin d’intégration native.  
- **Ignorer neuromorphique** : rejeté, croissance prévue en edge AI.  

## Impact et compatibilité
- Adoption dans edge AI, robotique, cognition artificielle.  
- Compatible Intel Loihi, IBM TrueNorth, SpiNNaker.  
- Positionne Vitte comme langage de recherche + industrie en IA neuromorphique.  

## Références
- [Intel Loihi](https://www.intel.com/content/www/us/en/research/neuromorphic-computing.html)  
- [IBM TrueNorth](https://research.ibm.com/blog/brain-inspired-computing)  
- [SpiNNaker Project](http://apt.cs.manchester.ac.uk/projects/SpiNNaker/)  
- [Spiking Neural Networks](https://en.wikipedia.org/wiki/Spiking_neural_network)  
