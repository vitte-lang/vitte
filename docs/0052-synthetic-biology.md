# RFC 0052 — Synthetic Biology & DNA Computing

## Résumé
Ce RFC explore l’intégration de la **biologie de synthèse** et du **calcul basé sur l’ADN** dans le langage **Vitte**.  
Objectif : offrir des abstractions permettant de concevoir, simuler et compiler des programmes biologiques et moléculaires.

## Motivation
La convergence informatique + biologie ouvre de nouveaux horizons :  
- Programmation de séquences d’ADN pour stockage ou logique,  
- Conception de circuits biologiques (biobricks, CRISPR),  
- Simulations biomoléculaires,  
- Thérapie génique et biotechnologies.  

Vitte doit permettre d’unifier calcul numérique et calcul biologique.

## Design proposé

### Module `std::bio`
- Manipulation de séquences ADN/ARN/Protéines.  
- Compilation en biobricks ou séquences CRISPR.  

```vitte
use std::bio::{DNA, Protein}

fn main() {
    let gene = DNA::from("ATCGGCTAGC")
    let prot = gene.translate()
    print("Protéine: {}", prot)
}
```

### Calcul ADN
- Logique moléculaire basée sur hybridation ADN.  

```vitte
use std::bio::dna_compute

fn main() {
    let input = dna_compute::encode("1010")
    let output = dna_compute::logic_and(input, "1100")
    print("Résultat ADN: {}", output)
}
```

### Biocircuits
- Construction de circuits biologiques modulaires.  

```vitte
use std::bio::circuit

fn main() {
    let toggle = circuit::toggle_switch(promoter="pLac", repressor="TetR")
    toggle.simulate()
}
```

### Exemple complet : stockage ADN
```vitte
use std::bio::storage

fn main() {
    let cid = storage::encode("Hello World!")
    let msg = storage::decode(cid)
    print("Message récupéré: {}", msg)
}
```

## Alternatives considérées
- **Langages spécialisés (Biopython, SBOL)** : rejeté, manque de sûreté mémoire et de performance.  
- **Outils séparés (lab only)** : rejeté, besoin de compiler de l’ADN comme du code.  
- **Ignorer biologie** : rejeté, révolution en cours.  

## Impact et compatibilité
- Applications en médecine, stockage de données, bioengineering.  
- Compatible SBOL, GenBank, CRISPR/Cas9.  
- Positionne Vitte comme langage bio-numérique.  

## Références
- [Synthetic Biology Open Language (SBOL)](https://sbolstandard.org/)  
- [DNA Computing](https://en.wikipedia.org/wiki/DNA_computing)  
- [CRISPR Technology](https://www.nature.com/subjects/crispr-cas9)  
- [iGEM Foundation](https://igem.org/)  
