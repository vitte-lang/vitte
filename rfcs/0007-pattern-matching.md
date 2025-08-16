# RFC 0007 — Pattern Matching

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0007  

---

## Résumé
Ajoute le **pattern matching** comme mécanisme de contrôle puissant.  

## Motivation
Simplifie la déstructuration de données et remplace de nombreux `if/else`.  

## Détails
```vitte
enum Option<T> { Some(T), None }

fn demo(o: Option<i32>) {
    match o {
        Some(x) => print("valeur: " + x),
        None => print("aucune valeur")
    }
}
```

### Déstructuration
```vitte
let point = (1,2);
match point {
    (0, y) => print("axe Y"),
    (x, 0) => print("axe X"),
    (x, y) => print("point")
}
```

## Alternatives
Switch-case classique (limité).  

## Impact
Doit être intégré dans le parser + bytecode.  

## Adoption
Introduit avec le compilateur MVP.  

## Références
- Rust match  
- Haskell case  
- Scala pattern matching  
