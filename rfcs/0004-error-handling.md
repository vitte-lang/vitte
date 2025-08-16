# RFC 0004 — Error Handling

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0004  

---

## Résumé
Spécifie la gestion des erreurs dans Vitte (types `Result`, `Option`, `panic`).  

## Motivation
Fiabilité sans exceptions cachées.  

## Détails
### Types
```vitte
enum Option<T> { Some(T), None }
enum Result<T,E> { Ok(T), Err(E) }
```
### Utilisation
```vitte
fn div(a: i32, b: i32) -> Result<i32,string> {
    if b == 0 { return Err("division by zero"); }
    Ok(a/b)
}
```

- `?` opère comme en Rust.  
- `panic!()` existe mais doit être rare.  

## Alternatives
Exceptions Java → rejetées.  

## Impact
Contrôle strict des erreurs → meilleures APIs.  

## Adoption
Stdlib inclut `Result`, `Option`.  

## Références
- Rust Error Handling  
- Go errors  
- Swift Optionals  
