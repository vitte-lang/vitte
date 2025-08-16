# RFC 0008 — Macro System

- Auteur : Vincent  
- Statut : Draft  
- Date : 2025-08-16  
- Numéro : 0008  

---

## Résumé
Définit un système de macros pour métaprogrammation.  

## Motivation
Permet la génération de code répétitif et la création d’extensions du langage.  

## Détails
### Macros par expansion de syntaxe
```vitte
macro_rules! vec {
    ($($x:expr),*) => { /* expansion */ }
}
```

### Macros procédurales
```vitte
#[derive(Show)]
struct Point { x: i32, y: i32 }
```

### Hygienic macros
Empêchent collisions de noms.  

## Alternatives
Préprocesseur type C → rejeté.  

## Impact
- Complexifie le compilateur.  
- Nécessite une API de manipulation AST.  

## Adoption
Phase ultérieure après MVP.  

## Références
- Rust Macros 1.1 / 2.0  
- Lisp macros  
- Template Haskell  
