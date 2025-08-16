# RFC 0003 — Memory Model and Ownership

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0003  

---

## Résumé
Définit le modèle mémoire de Vitte basé sur **ownership & borrowing** inspiré de Rust.  

## Motivation
Éviter fuites mémoire et data races.  

## Détails
- Chaque valeur a un propriétaire unique.  
- `&` → emprunt immutable, `&mut` → emprunt mutable unique.  
- Durées de vie vérifiées à la compilation.  

```vitte
fn main() {
    let mut s = "hello".to_string();
    let r1 = &s;     // ok immuable
    let r2 = &s;     // ok immuable
    let r3 = &mut s; // erreur
}
```

## Alternatives
GC pur (trop lent).  
Ref-count généralisé (coûteux).  

## Impact
Compile-time checks → perf maximale.  

## Adoption
Core du compilateur, VM n’a pas besoin de GC par défaut.  

## Références
- Rust Borrow Checker  
- C++ RAII  
- Cyclone Language  
