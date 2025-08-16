# RFC 0001 — Core Syntax and Keywords

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0001  

---

## Résumé
Définit la syntaxe fondamentale et les mots-clés réservés de Vitte.

## Motivation
Proposer un langage expressif, sûr et lisible.

## Détails de conception
### Variables
```vitte
let x: i32 = 42;
let mut counter = 0;
```
### Fonctions
```vitte
fn add(a: i32, b: i32) -> i32 { return a+b; }
```
### Contrôle
```vitte
if x > 0 { … } else { … }
for i in 0..10 { … }
```
### Mots-clés
`let`, `mut`, `fn`, `if`, `else`, `for`, `while`, `match`, `return`, `break`, `continue`, `async`, `await`, `use`, `extern`  
### Types primitifs
`i32`, `u64`, `f64`, `bool`, `char`, `string`, `()`  

## Alternatives
Syntaxe C ou Python → rejeté.  

## Impact
Base de compatibilité.  

## Adoption
Implémentation immédiate.  

## Références
- Rust Reference  
- C99  
- Python keywords  
