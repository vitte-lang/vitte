# RFC 0009 — Standard Library Structure

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0009  

---

## Résumé
Définit la structure de la bibliothèque standard (stdlib) de Vitte.  

## Motivation
Une stdlib cohérente rend le langage utilisable immédiatement.  

## Détails
### Modules principaux
- `core` : types primitifs, Option, Result, traits de base.  
- `collections` : Vec, Map, Set.  
- `io` : fichiers, sockets.  
- `net` : TCP, UDP, HTTP client minimal.  
- `sync` : mutex, channels.  
- `time` : dates et durées.  

### Exemple
```vitte
use std::collections::Vec;

fn main() {
    let mut v = Vec::new();
    v.push(1);
}
```

## Alternatives
Stdlib minimale (trop pauvre).  
Stdlib géante (trop lourde).  

## Impact
Définition du périmètre officiel.  

## Adoption
Livraison avec `vittec 1.0`.  

## Références
- Rust stdlib  
- Go stdlib  
- Java core packages  
