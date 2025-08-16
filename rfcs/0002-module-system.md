# RFC 0002 — Module System

- Auteur : Vincent  
- Statut : Proposed  
- Date : 2025-08-16  
- Numéro : 0002  

---

## Résumé
Définit le système de modules et d’imports dans Vitte.  

## Motivation
Un langage moderne doit structurer le code en unités claires.  

## Détails
- `mod` déclare un module.  
- `use` importe un module.  
- Hiérarchie reflète le système de fichiers.  

```vitte
mod math {
    pub fn add(a: i32, b: i32) -> i32 { a+b }
}

use math::add;

fn main() {
    print(add(2,3));
}
```

## Alternatives
Namespaces à la C++ (trop lourds).  
Imports implicites (confus).  

## Impact
Impact sur compilation incrémentale, tooling.  

## Adoption
Compiler doit supporter `mod`, `use`, `pub`.  

## Références
- Rust modules  
- Go packages  
- C# namespaces  
