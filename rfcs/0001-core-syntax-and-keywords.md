# RFC 0001 — Core Syntax and Keywords

## Résumé
Ce RFC définit la syntaxe de base du langage Vitte ainsi que l’ensemble des mots-clés réservés.  
L’objectif est de poser les fondations syntaxiques minimales, lisibles, cohérentes et évolutives, en combinant des influences de C, Rust et C#.

## Motivation
Un langage de programmation ne peut exister sans une grammaire claire et des conventions lexicales solides.  
Définir la syntaxe et les mots-clés dès le départ permet :
- de garantir la cohérence du compilateur et des outils (LSP, IDE, formatter),
- d’éviter les collisions futures dans l’évolution du langage,
- d’offrir aux développeurs une lisibilité immédiate et une courbe d’apprentissage fluide,
- d’assurer une compatibilité future avec le bytecode et la VM.

## Design proposé

### Identificateurs
- Sensibles à la casse (`foo` ≠ `Foo`).
- Autorisés : lettres Unicode, chiffres (non initiaux), `_`.
- Style recommandé : `snake_case` pour variables/fonctions, `PascalCase` pour types, `SCREAMING_SNAKE_CASE` pour constantes.

### Littéraux
- Nombres entiers : `42`, `0xFF`, `0o77`, `0b1010`.
- Flottants : `3.14`, `1e-9`.
- Booléens : `true`, `false`.
- Caractères : `'a'`, `'
'`.
- Chaînes : `"texte"`, `r"chaine brute"`.

### Commentaires
- Ligne : `// commentaire`.
- Bloc : `/* commentaire */`.
- Documentation : `/// commentaire doc`.

### Mots-clés réservés
```txt
if, else, match, for, while, loop, break, continue,
fn, return, let, mut, const, static,
struct, enum, trait, impl, type, alias,
pub, use, mod, package,
async, await, spawn,
unsafe, extern, ffi,
null, true, false
```

### Déclarations de variables
```vitte
let x = 42          // immuable
let mut y = 3.14    // mutable
const PI = 3.14159  // constante
```

### Fonctions
```vitte
fn add(a: i32, b: i32) -> i32 {
    return a + b
}
```

### Structures et enums
```vitte
struct Point {
    x: f64,
    y: f64,
}

enum Result<T, E> {
    Ok(T),
    Err(E),
}
```

### Contrôle de flux
```vitte
if x > 0 {
    print("positif")
} else if x < 0 {
    print("négatif")
} else {
    print("zéro")
}

for i in 0..10 {
    print(i)
}

while condition {
    faire_truc()
}
```

### Pattern Matching
```vitte
match result {
    Ok(value) => print(value),
    Err(e)    => panic(e),
}
```

### Modules
```vitte
mod math {
    pub fn square(x: i32) -> i32 { x * x }
}

use math::square
```

## Alternatives considérées
- **Syntaxe inspirée uniquement de C** : rejetée car trop verbeuse et manque de sûreté.
- **Syntaxe Python-like (indentation obligatoire)** : rejetée pour éviter les ambiguïtés liées aux espaces.
- **Réduction du nombre de mots-clés** : rejetée car limite la lisibilité et l’évolution du langage.

## Impact et compatibilité
- Impact majeur : ce RFC constitue la **base du compilateur** et influence tous les autres RFCs.
- Compatibilité ascendante : les choix établis ici doivent rester stables pour éviter de casser les projets futurs.
- Performance : une syntaxe claire facilite l’optimisation du parser et du bytecode.
- Ergonomie : proche de Rust et C#, donc adoption rapide par les développeurs systèmes et applicatifs.

## Références
- [The Rust Reference](https://doc.rust-lang.org/reference/)
- [C# Language Specification](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/)
- [C99 Standard](https://www.open-std.org/jtc1/sc22/wg14/)
