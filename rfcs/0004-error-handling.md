# RFC 0004 — Error Handling

## Résumé
Ce RFC définit le modèle de gestion des erreurs du langage **Vitte**.  
Il propose un système combinant résultats explicites (`Result<T, E>`) et gestion optionnelle des erreurs critiques via `panic`.  
L’objectif est de fournir un mécanisme sûr, expressif et performant, adapté aussi bien aux systèmes embarqués qu’aux applications de haut niveau.

## Motivation
La gestion des erreurs est un pilier fondamental du design d’un langage :  
- **Sécurité** : éviter les crashs silencieux ou les comportements indéfinis.  
- **Lisibilité** : expliciter les cas d’échec dans les signatures de fonctions.  
- **Interopérabilité** : compatible avec FFI et conventions d’autres langages.  
- **Performance** : éviter le coût caché des exceptions.  

## Design proposé

### Types d’erreurs de base
- `Result<T, E>` : succès (`Ok(T)`) ou échec (`Err(E)`).  
- `Option<T>` : valeur présente (`Some(T)`) ou absente (`None`).  

```vitte
fn div(a: i32, b: i32) -> Result<i32, String> {
    if b == 0 {
        return Err("division par zéro".to_string())
    }
    Ok(a / b)
}
```

### Propagation des erreurs
- L’opérateur `?` propage automatiquement l’erreur.  

```vitte
fn calc() -> Result<i32, String> {
    let x = div(10, 2)?
    let y = div(x, 0)?
    Ok(y)
}
```

### Panic et erreurs fatales
- `panic(msg)` arrête immédiatement le programme.  
- À utiliser uniquement pour les invariants critiques ou erreurs internes.  

```vitte
fn get_value(v: Option<i32>) -> i32 {
    match v {
        Some(x) => x,
        None => panic("valeur manquante"),
    }
}
```

### Erreurs customisées
Les développeurs peuvent définir leurs propres enums d’erreurs.  

```vitte
enum FileError {
    NotFound,
    PermissionDenied,
    Corrupted,
}

fn open_file(path: &String) -> Result<File, FileError> {
    // implémentation simulée
    Err(FileError::NotFound)
}
```

### Conversion d’erreurs
- `From<E>` permet de convertir automatiquement entre types d’erreurs.  
- Favorise la composition et réduit le boilerplate.  

```vitte
impl From<IOError> for FileError { ... }
```

### Logging et diagnostics
- Les erreurs peuvent être enrichies de messages, traces et codes.  
- Intégration prévue avec le système de diagnostics du compilateur.  

## Alternatives considérées
- **Exceptions à la Java/C++** : rejetées pour leur coût caché et manque de prévisibilité.  
- **Retour de codes numériques (C)** : rejeté pour manque d’expressivité et de sûreté.  
- **Gestion implicite via GC** : rejetée pour incompatibilité avec les environnements systèmes.  

## Impact et compatibilité
- Impact positif sur la robustesse des programmes.  
- Compatible avec le modèle d’ownership et la compilation vers bytecode.  
- Introduit une légère complexité syntaxique (usage de `Result` et `?`), compensée par la sûreté accrue.  

## Références
- [Rust Error Handling](https://doc.rust-lang.org/book/ch09-00-error-handling.html)  
- [Go Error Values](https://go.dev/blog/error-handling-and-go)  
- [C++ Exceptions](https://en.cppreference.com/w/cpp/error/exception)  
