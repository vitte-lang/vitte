# RFC 0019 — Garbage Collection vs Ownership

- Auteur : Vincent  
- Statut : Draft  
- Date : 2025-08-16  
- Numéro : 0019  

---

## Résumé
Discute l’équilibre entre ownership et un GC optionnel.

## Motivation
Certains cas exigent un GC (interop, VM dynamique).

## Détails
- Ownership par défaut.  
- GC optionnel via feature `gc`.  
- VM peut activer GC mark & sweep.  

## Alternatives
Full GC (lent), pas de GC (trop strict).  

## Impact
Flexibilité accrue.  

## Adoption
Feature expérimentale.  

## Références
- Rust ownership  
- JVM GC  
- Go GC  
