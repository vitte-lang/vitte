# RFC 0030 — Quantum Computing and Future Extensions

## Résumé
Ce RFC définit le support du **calcul quantique** et propose des pistes pour les **extensions futures** du langage **Vitte**.  
Objectif : offrir une passerelle vers les ordinateurs quantiques, préparer le langage aux paradigmes émergents et assurer sa pérennité.

## Motivation
Le calcul quantique devient incontournable dans certains domaines :  
- cryptographie post-quantique,  
- optimisation et recherche opérationnelle,  
- simulation moléculaire et physique,  
- machine learning quantique.  

Un langage moderne doit offrir des primitives pour interagir avec ces systèmes.

## Design proposé

### Module `std::quantum`
- Primitives de base : qubits, portes, mesures.  

```vitte
use std::quantum::{Qubit, H, X, measure}

fn main() {
    let mut q = Qubit::new()
    H(&mut q)        // Hadamard gate
    let r = measure(&q)
    print("Result: {}", r)
}
```

### Circuits quantiques
- Construction déclarative de circuits :  

```vitte
use std::quantum::{Circuit, H, CNOT, measure_all}

fn main() {
    let mut c = Circuit::new(2)
    c.add(H(0))
    c.add(CNOT(0,1))
    let res = measure_all(&c)
    print(res)
}
```

### Compilation et backends
- `vitpm quantum run --backend qiskit` → exécution via IBM Q.  
- `vitpm quantum run --backend braket` → exécution AWS Braket.  
- `vitpm quantum sim` → simulateur local.  

### Post-quantum cryptography
- Intégration d’algorithmes résistants au quantique (`Kyber`, `Dilithium`).  
- Disponibles dans `std::crypto::pq`.  

```vitte
use std::crypto::pq

fn main() {
    let (pk, sk) = pq::kyber::keygen()
    let ct = pq::kyber::encrypt("secret", pk)
    let msg = pq::kyber::decrypt(ct, sk)
    print(msg)
}
```

### Futur : extensions possibles
- **Neural accelerators** : support natif pour TPUs et NPUs.  
- **Bio-computing** : primitives pour ADN et biologie synthétique.  
- **Edge/IoT** : optimisation pour processeurs ultra-faibles.  
- **Programmation probabiliste** : intégrée au langage.  

### Exemple complet : téléportation quantique
```vitte
use std::quantum::{Qubit, H, CNOT, measure}

fn teleport() {
    let mut q1 = Qubit::from_state(1)
    let mut q2 = Qubit::new()
    let mut q3 = Qubit::new()

    H(&mut q2)
    CNOT(&mut q2, &mut q3)

    CNOT(&mut q1, &mut q2)
    H(&mut q1)

    let m1 = measure(&q1)
    let m2 = measure(&q2)

    if m2 == 1 { X(&mut q3) }
    if m1 == 1 { Z(&mut q3) }

    print("Teleported state: {:?}", q3)
}
```

## Alternatives considérées
- **Pas de support quantique** : rejeté, futur du calcul.  
- **Support uniquement simulateur** : rejeté, besoin backends réels.  
- **Langage séparé pour quantique** : rejeté, perte d’unicité.  

## Impact et compatibilité
- Impact fort : Vitte devient pionnier en support natif du quantique.  
- Compatible avec backends existants (Qiskit, Braket, Cirq).  
- Introduit des concepts nouveaux mais optionnels.  

## Références
- [Qiskit](https://qiskit.org/)  
- [AWS Braket](https://aws.amazon.com/braket/)  
- [Google Cirq](https://quantumai.google/cirq)  
- [NIST Post-Quantum Cryptography](https://csrc.nist.gov/projects/post-quantum-cryptography)  
