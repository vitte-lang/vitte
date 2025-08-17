# RFC 0007 — Pattern Matching

## Résumé
Ce RFC définit le système de **pattern matching** du langage **Vitte**.  
Il propose une approche expressive et sûre permettant de déconstruire des valeurs, de contrôler le flux d’exécution et de rendre le code plus lisible.  
Inspiré de Rust, Haskell et Scala, il constitue un élément clé du langage pour manipuler des enums, des tuples, des constantes et des structures.

## Motivation
Le pattern matching permet :  
- une alternative claire et sûre aux chaînes de `if/else`,  
- une déconstruction élégante des types complexes,  
- une garantie d’exhaustivité grâce à la vérification par le compilateur,  
- un support avancé pour les enums, utile pour la gestion des erreurs (`Result`, `Option`).  

## Design proposé

### Syntaxe de base
```vitte
let x = 3

match x {
    0      => print("zéro"),
    1 | 2  => print("un ou deux"),
    3..=9  => print("entre trois et neuf"),
    _      => print("autre"),
}
```

### Match sur enums
```vitte
enum Result<T, E> {
    Ok(T),
    Err(E),
}

fn process(r: Result<i32, String>) {
    match r {
        Ok(v)  => print("Valeur:", v),
        Err(e) => print("Erreur:", e),
    }
}
```

### Match sur tuples
```vitte
let point = (0, 5)

match point {
    (0, y) => print("Sur l’axe Y avec", y),
    (x, 0) => print("Sur l’axe X avec", x),
    (x, y) => print("Point générique:", x, y),
}
```

### Match avec guard (`if`)
```vitte
let n = 10

match n {
    x if x % 2 == 0 => print("pair"),
    _               => print("impair"),
}
```

### Déstructuration de structures
```vitte
struct Point { x: i32, y: i32 }

fn main() {
    let p = Point { x: 3, y: 7 }
    match p {
        Point { x: 0, y } => print("Sur Y:", y),
        Point { x, y: 0 } => print("Sur X:", x),
        Point { x, y }    => print("Coord:", x, y),
    }
}
```

### Binding (`@`)
Permet de capturer une valeur tout en la comparant.  

```vitte
let n = 42

match n {
    v @ 0..=100 => print("Nombre dans [0,100]:", v),
    _           => print("Autre"),
}
```

### Exhaustivité
Le compilateur vérifie que toutes les branches sont couvertes.  
Sinon, une erreur de compilation est générée.  

### `if let` et `while let`
```vitte
if let Some(x) = option {
    print("Option contient", x)
}

while let Some(v) = iter.next() {
    print("Valeur:", v)
}
```

## Alternatives considérées
- **Switch classique (C, Java)** : rejeté pour manque d’expressivité.  
- **Pattern matching dynamique (Python 3.10)** : rejeté pour manque de typage statique strict.  
- **Macros uniquement** : rejetées car trop limitées pour des cas complexes.  

## Impact et compatibilité
- Impact fort sur la lisibilité et la sûreté du langage.  
- Compatible avec les enums, tuples, structs et constantes.  
- Vérification d’exhaustivité renforce la robustesse du code.  
- Introduit une légère complexité syntaxique mais intuitive pour les développeurs venant de Rust/Scala.  

## Références
- [Rust Patterns](https://doc.rust-lang.org/book/ch18-00-patterns.html)  
- [Haskell Pattern Matching](https://wiki.haskell.org/Pattern_matching)  
- [Scala Match Expressions](https://docs.scala-lang.org/tour/pattern-matching.html)  
